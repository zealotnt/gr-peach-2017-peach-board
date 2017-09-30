#include "mbed.h"
#include "TLV320_RBSP.h"
#include "cProcess.hpp"
#include "mStorage.hpp"
#include "mWav.hpp"
#include "define.h"
#include "rtos.h"
#include "grHwSetup.h"
#include "grUtility.h"

#define WRITE_BUFF_NUM         (16)
#define READ_BUFF_SIZE         (4096)
#define READ_BUFF_NUM          (16)
#define MAIL_QUEUE_SIZE        (WRITE_BUFF_NUM + READ_BUFF_NUM + 1)
#define INFO_TYPE_WRITE_END    (0)
#define INFO_TYPE_READ_END     (1)
#define DBG_INFO(...)			printf(__VA_ARGS__)

// Signal for main thread
osThreadId mainThreadID;
uint8_t userPress = 0;
rtos::Mutex muOverRecord;
bool isOverRecord;

static int wavSize = 0; 
rtos::Mutex mutexUserButton;
DigitalIn  userButton(USER_BUTTON0);

typedef struct {
    uint32_t info_type;
    void *   p_data;
    int32_t  result;
} mail_t;
Mail<mail_t, MAIL_QUEUE_SIZE> mail_box;

static bool isStopWrite = true;
//cWav *myWavObject;

#if defined(__ICCARM__)
#pragma data_alignment=4
static uint8_t audio_read_buff[READ_BUFF_NUM][READ_BUFF_SIZE]@ ".mirrorram";  //4 bytes aligned! No cache memory
#else
static uint8_t audio_read_buff[READ_BUFF_NUM][READ_BUFF_SIZE] __attribute((section("NC_BSS"),aligned(4)));  //4 bytes aligned! No cache memory
#endif

static void callback_audio_tans_end(void * p_data, int32_t result, void * p_app_data) {
    mail_t *mail = mail_box.alloc();

    if (result < 0) {
        return;
    }
    if (mail == NULL) {
        DBG_INFO("error mail alloc\n");
    } else {
        mail->info_type = (uint32_t)p_app_data;
        mail->p_data    = p_data;
        mail->result    = result;
        mail_box.put(mail);
    }
}

void audio_read_task(void const*) {
    uint32_t cnt;
    grEnableAudio();
    TLV320_RBSP *audio = grAudio();
    // Microphone
    audio->mic(true);   // Input select for ADC is microphone.
    audio->format(16);
    audio->power(0x00); // mic on
    audio->inputVolume(0.8, 0.8);

    rbsp_data_conf_t audio_write_data = {&callback_audio_tans_end, (void *)INFO_TYPE_WRITE_END};
    rbsp_data_conf_t audio_read_data  = {&callback_audio_tans_end, (void *)INFO_TYPE_READ_END};

    // Read buffer setting
    for (cnt = 0; cnt < READ_BUFF_NUM; cnt++) {
        if (audio->read(audio_read_buff[cnt], READ_BUFF_SIZE, &audio_read_data) < 0) {
            DBG_INFO("read error\n");
        }
    }

    while (1) {
        osEvent evt = mail_box.get();
        // if(userButton == 0x00)
        // 	press+=1;

        if (evt.status == osEventMail) {
            mail_t *mail = (mail_t *)evt.value.p;

            if ((mail->info_type == INFO_TYPE_READ_END) && (mail->result > 0)) 
            {   
                audio->write(mail->p_data, mail->result, &audio_write_data);
                
                if(isOverRecord == false && userPress != 0)
                {
	                if(userPress == 1)
	                {
	                	memcpy(audio_frame_read+wavSize*READ_BUFF_SIZE,(uint8_t*)mail->p_data,READ_BUFF_SIZE);
	                	wavSize += 1;
		                if(wavSize >= 300)
		                {
		                	DBG_INFO("**Thread:Oversize!\n");
		                	wavSize = 0;
		                	isOverRecord = true;
		                	userPress = 0;
		                }
	                }
	                else if(userPress == 2)
	                {
	                	DBG_INFO("**Thread:End record\n");
	                	userPress = 0;
	                	memcpy(audio_frame_read+wavSize*READ_BUFF_SIZE,(uint8_t*)mail->p_data,READ_BUFF_SIZE);
	                	wavSize += 1;
	                    memcpy(audio_frame_backup + 44,audio_frame_read,wavSize*READ_BUFF_SIZE);
	                  	osSignalSet(mainThreadID, 0x1);
	                }
                }
            } else {
                audio->read(mail->p_data, READ_BUFF_SIZE, &audio_read_data);     // Resetting read buffer
            }
            mail_box.free(mail);
        }
    }
}

int main_grpeach() {

	NetworkInterface* network = grInitEth();
    Thread audioReadTask(audio_read_task, NULL, osPriorityNormal, 1024*32);

    mainThreadID = osThreadGetId();
    // main processing
    bool isRecordDone = false;
    while(1) {
    	// button
    	wavSize = 0;

    	while(1)
    	{
    		isRecordDone = false;
	    	
	    	DBG_INFO("Main: Wait press time 1!\n");
	    	waitShortPress();
	    	userPress = 1;
	    	isOverRecord = false;

	    	DBG_INFO("Main: Start record!\n");

	    	while (1) {
	    		if(isOverRecord == true)//userPress auto = 0
	    			break;
		        if (isButtonPressed()) {
		        	userPress = 2;
		        	isRecordDone = true;
		        	break;
		        }
		        Thread::wait(50);
	    	}

	    	if(isRecordDone == true)
	    		break;
    	}
    	DBG_INFO("Main: Wait send signal!\n");
    	osSignalWait(0x1, osWaitForever);
    	DBG_INFO("Main: Start send to server!\n");

    	cWav myWavObject("");
    	unsigned char* buff = myWavObject.getHeader(wavSize*READ_BUFF_SIZE);
    	memcpy(audio_frame_backup,buff,44);

    	grStartUpload(network);
    	grUploadFile(network, audio_frame_backup, wavSize*READ_BUFF_SIZE + 44);
    	grEndUpload(network);

    	// Send outdio to server and get response

        Thread::wait(100);
    }
}