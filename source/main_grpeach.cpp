#include "mbed.h"
#include "TLV320_RBSP.h"
#include "cProcess.hpp"
#include "mStorage.hpp"
#include "mWav.hpp"
#include "define.h"
#include "rtos.h"
#include "grHwSetup.h"
#include "grUtility.h"
#include "Json.h"
#include "cJson.h"
#include "cNodeManager.h"
#include <string>

#define WRITE_BUFF_NUM         (16)
#define READ_BUFF_SIZE         (4096)
#define READ_BUFF_NUM          (16)
#define MAIL_QUEUE_SIZE        (WRITE_BUFF_NUM + READ_BUFF_NUM + 1)
#define INFO_TYPE_WRITE_END    (0)
#define INFO_TYPE_READ_END     (1)
#define DBG_INFO(...)			printf(__VA_ARGS__)

// Signal for main thread
bool isDisableRecording = false;
osThreadId mainThreadID;
uint8_t userPress = 0;
rtos::Mutex muOverRecord;
bool isOverRecord;
NodeManager *peachDeviceManager;

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

void disableRecording() {
    isDisableRecording = true;
}

void enableRecording() {
    isDisableRecording = false;
}

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

    // rbsp_data_conf_t audio_write_data = {&callback_audio_tans_end, (void *)INFO_TYPE_WRITE_END};
    rbsp_data_conf_t audio_read_data  = {&callback_audio_tans_end, (void *)INFO_TYPE_READ_END};

    // Read buffer setting
    for (cnt = 0; cnt < READ_BUFF_NUM; cnt++) {
        if (audio->read(audio_read_buff[cnt], READ_BUFF_SIZE, &audio_read_data) < 0) {
            DBG_INFO("read error\n");
        }
    }

    while (1) {
        if (isDisableRecording == true) {
            Thread::wait(100);
        }
        osEvent evt = mail_box.get();
        // if(userButton == 0x00)
        // 	press+=1;

        // if (evt.status == osEventMail) {
            mail_t *mail = (mail_t *)evt.value.p;

            // if ((mail->info_type == INFO_TYPE_READ_END) && (mail->result > 0))
            // {
            //     audio->write(mail->p_data, mail->result, &audio_write_data);

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
            // } else {
                audio->read(mail->p_data, READ_BUFF_SIZE, &audio_read_data);     // Resetting read buffer
            // }
            mail_box.free(mail);
        // }
    }
}

void do_action(char* action, char* toStr)
{
	printf("Action:%s\n", action);
	char *pt;
	bool found = false;
	if(strcmp(action,"on") == 0)
	{
		DBG_INFO("Do on device!\n");
		pt = strtok (toStr,",");

	    while (pt != NULL) {
	        DBG_INFO("%s\n", pt);
	        string name(pt);
	        int id = peachDeviceManager->getNodeIdByName(name);
	        printf("Id: %d\n", id);
	        if( id >= 0)
	        {
	        	printf("IP: %s\n", peachDeviceManager->getIpDevice(id).c_str());
	        	peachDeviceManager->NodeRelayOn(peachDeviceManager->getIpDevice(id));
	        }
	        else
	        	printf("Dont know device!\n");
	        //on off
	        pt = strtok (NULL, ",");
	    }
	}
	else if (strcmp(action,"off") == 0)
	{
		DBG_INFO("Do off device\n");
		pt = strtok (toStr,",");
	    while (pt != NULL) {
	        DBG_INFO("%s\n", pt);
	        string name(pt);
	        int id = peachDeviceManager->getNodeIdByName(name);
	        if( id >= 0)
	        {
	        	printf("IP: %s\n", peachDeviceManager->getIpDevice(id).c_str());
	        	peachDeviceManager->NodeRelayOff(peachDeviceManager->getIpDevice(id));
	        }
	        else
	        	printf("Dont know device!\n");
	        pt = strtok (NULL, ",");
	    }
	}
	else
	{
		DBG_INFO("Dont know action\n");
	}
}
void processResponseFromServer(char* body)
{
	Json json (body, strlen (body));
	char valueAction[10] = "";
	char valueTo[100] = "";
	char keyAction[7]="action";
	char keyTo[3]="to";

	if(json.isValidJson())
	{
		DBG_INFO("Json valid!\n");
		int keyIndex = json.findKeyIndexIn ( keyAction , 0 );
	    if ( keyIndex == -1 )
	    {
	        // Error handling part ...
	        printf("\"%s\" does not exist ... do something!!",keyAction);
	    }
	    else
	    {
	        // Find the first child index of key-node "city"
	        int valueIndex = json.findChildIndexOf ( keyIndex, -1 );
	        if ( valueIndex > 0 )
	        {
	            const char * valueStart  = json.tokenAddress ( valueIndex );
	            int          valueLength = json.tokenLength ( valueIndex );
	            strncpy ( valueAction, valueStart, valueLength );
	            valueAction [ valueLength ] = 0; // NULL-terminate the string
	        }
	    }

	    keyIndex = json.findKeyIndexIn ( keyTo , 0 );
	    if ( keyIndex == -1 )
	    {
	        // Error handling part ...
	        printf("\"%s\" does not exist ... do something!!",keyTo);
	    }
	    else
	    {
	        // Find the first child index of key-node "city"
	        int valueIndex = json.findChildIndexOf ( keyIndex, -1 );
	        if ( valueIndex > 0 )
	        {
	            const char * valueStart  = json.tokenAddress ( valueIndex );
	            int          valueLength = json.tokenLength ( valueIndex );
	            strncpy ( valueTo, valueStart, valueLength );
	            valueTo [ valueLength ] = 0; // NULL-terminate the string

	            do_action(valueAction,valueTo);
	        }
	    }
	}
	else
		DBG_INFO("Json invalid!\n");
}
void getHeader(int size_of_data, unsigned char* header)
{
	unsigned char wav_header[44] = {
	0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00, 0x57, 0x41, 0x56,
	0x45, 0x66, 0x6d, 0x74, 0x20, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00,
	0x02, 0x00, 0x44, 0xac, 0x00, 0x00, 0x10, 0xb1, 0x02, 0x00, 0x04,
	0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61, 0x00, 0x00, 0x00, 0x00
	};

	memcpy(header,wav_header,44*sizeof(unsigned char));

	int overallSize = size_of_data - 8;
    unsigned char buffSize[4];
    unsigned char buffOverall[4];

    buffSize[0] = size_of_data & 0xff;
    buffSize[1] = (size_of_data >> 8) & 0xff;
    buffSize[2] = (size_of_data >> 16) & 0xff;
    buffSize[3] = (size_of_data >> 24) & 0xff;
    memcpy(header + 40,buffSize, 4 * sizeof(unsigned char));

    buffOverall[0] = overallSize & 0xff;
    buffOverall[1] = (overallSize >> 8) & 0xff;
    buffOverall[2] = (overallSize >> 16) & 0xff;
    buffOverall[3] = (overallSize >> 24) & 0xff;
    memcpy(header + 4,buffSize, 4 * sizeof(unsigned char));
}

int main_test_json_the()
{
	NetworkInterface* network = grInitEth();
	peachDeviceManager = new NodeManager(network);
    peachDeviceManager->NodeAdd(DEV_1_IP, DEV_1_NAME);
    peachDeviceManager->NodeAdd(DEV_2_IP, DEV_2_NAME);
	char body[1024] = "{\"response\":\"hello the!\",\"action\":\"off\",\"to\":\"fan,lamp,act\"}";
	processResponseFromServer(body);
	return 1;
}

int main_grpeach() {
    Timer countTimer;
	char serverResponse[1024];
	unsigned char wavBuffHeader[44];

	NetworkInterface* network = grInitEth();
	grSetupUsb();

	peachDeviceManager = new NodeManager(network);
    peachDeviceManager->NodeAdd(DEV_1_IP, DEV_1_NAME);
    peachDeviceManager->NodeAdd(DEV_2_IP, DEV_2_NAME);

    Thread audioReadTask(audio_read_task, NULL, osPriorityNormal, 1024*32);

    mainThreadID = osThreadGetId();
    // main processing
    bool isRecordDone = false;
    while(1) {
    	wavSize = 0;
        enableRecording();

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

        disableRecording();

    	// Start request to server
    	// Get a wav file
    	getHeader(wavSize*READ_BUFF_SIZE,wavBuffHeader);
    	memcpy(audio_frame_backup,wavBuffHeader,44);
    	grStartUpload(network);
    	grUploadFile(network, audio_frame_backup, wavSize*READ_BUFF_SIZE + 44);

    	// Get respone from server
    	grEndUploadWithBody(network,serverResponse);
    	DBG_INFO("response:%s\n",serverResponse);

    	// process response from server
    	//processResponseFromServer(res);

    	// Download and play file audio
        Thread::wait(200);
        countTimer.start();
        grDownloadFile(network, "file_to_write.txt", ADDRESS_SERVER);
        countTimer.stop();
        printf("Download done in %d ms, play file\r\n", countTimer.read_ms());
        countTimer.reset();
        grPlayWavFile("file_to_write.txt");
    }
}
