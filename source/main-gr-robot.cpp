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
#include "Websocket.h"
#include <string>

#define WRITE_BUFF_NUM         (16)
#define READ_BUFF_SIZE         (4096)
#define READ_BUFF_NUM          (16)
#define MAIL_QUEUE_SIZE        (WRITE_BUFF_NUM + READ_BUFF_NUM + 1)
#define INFO_TYPE_WRITE_END    (0)
#define INFO_TYPE_READ_END     (1)
#define DBG_INFO(...)           printf(__VA_ARGS__)

//4 bytes aligned! No cache memory
static uint8_t audio_read_buff[READ_BUFF_NUM][READ_BUFF_SIZE] __attribute((section("NC_BSS"),aligned(4)));

typedef struct {
    uint32_t info_type;
    void *   p_data;
    int32_t  result;
} mail_t;
extern Mail<mail_t, MAIL_QUEUE_SIZE> mail_box;

static void callback_audio_tans_end(void * p_data, int32_t result, void * p_app_data) {
    mail_t *mail = mail_box.alloc();

    if (result < 0) {
        return;
    }
    if (mail == NULL) {
        // consumer not consume fast enough, report error
        return;
    }

    mail->info_type = (uint32_t)p_app_data;
    mail->p_data    = p_data;
    mail->result    = result;
    mail_box.put(mail);
}

static void audio_stream_task(void const*) {
    NetworkInterface* network = grInitEth();
    bool isStreaming = true;
    Websocket ws(ADDRESS_WS_SERVER, network);

    while (1) {
        isStreaming = true;
        DBG_INFO("Trying to connect\r\n");
        int connect_error = ws.connect();
        if (!connect_error) {
            DBG_INFO("error connecting to server\r\n");
            Thread::wait(500);
            continue;
        }

        while (isStreaming) {
            osEvent evt = mail_box.get();
            mail_t *mail = (mail_t *)evt.value.p;

            int error_c = ws.send((uint8_t*)mail->p_data, mail->result);
            mail_box.free(mail);
            if (error_c == -1) {
                DBG_INFO("error when sending\r\n");
                Thread::wait(500);
                ws.close();
                isStreaming = false;
                break;
            }

            if (isButtonPressed()) {
                DBG_INFO("Button pressed, close socket and stream again\r\n");
                ws.close();
                isStreaming = false;
                break;
            }
        }
    }
}

static void audio_read_task(void const*) {
    uint32_t cnt;
    grEnableAudio();
    TLV320_RBSP *audio = grAudio();
    // Microphone
    audio->mic(true);   // Input select for ADC is microphone.
    audio->format(16);
    audio->power(0x00); // mic on
    audio->inputVolume(0.8, 0.8);
    audio->frequency(16000);
    rbsp_data_conf_t audio_read_data  = {&callback_audio_tans_end, (void *)INFO_TYPE_READ_END};

    while (1) {
        for (cnt = 0; cnt < READ_BUFF_NUM; cnt++) {
            if (audio->read(audio_read_buff[cnt], READ_BUFF_SIZE, &audio_read_data) < 0) {
                DBG_INFO("read error\n");
            }
        }
    }
}

int main_gr_robot() {
    NetworkInterface* network = grInitEth();
    char serverResp[1024];

//    Thread audioReadTask(audio_read_task, NULL, osPriorityNormal, 1024*32);
//    Thread audioStreamTask(audio_stream_task, NULL, osPriorityNormal, 1024*32);

    while(1) {
        if (grHttpGet(network, "/", serverResp, sizeof(serverResp)) != false) {
            printf("Get: %s\r\n", serverResp);
        }
        Thread::wait(500);
    }
}
