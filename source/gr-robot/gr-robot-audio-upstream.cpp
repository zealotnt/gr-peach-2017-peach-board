/*
 * gr-robot-audio-upstream.cpp
 *
 *  Created on: Nov 18, 2017
 *      Author: zealot
 */

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
#define MODULE_PREFIX           "[WS] "
#define DBG_INFO(...)           printf(MODULE_PREFIX); printf(__VA_ARGS__)

typedef struct {
    uint32_t info_type;
    void *   p_data;
    int32_t  result;
} mail_t;

//4 bytes aligned! No cache memory
static uint8_t audio_read_buff[READ_BUFF_NUM][READ_BUFF_SIZE] __attribute((section("NC_BSS"),aligned(4)));
static bool isEnableRead = true;
static bool isReallyDisable = false;
extern Mail<mail_t, MAIL_QUEUE_SIZE> mail_box;
extern void notifyMain_websocketClose(uint32_t reasonId);

static void callback_audio_tans_end(void * p_data, int32_t result, void * p_app_data) {
    if (result < 0) {
        printf("error with read thread!!!\r\n");
        return;
    }

    mail_t *mail = mail_box.alloc();
    if (mail == NULL) {
        // consumer not consume fast enough, report error
        return;
    }

    mail->info_type = (uint32_t)p_app_data;
    mail->p_data    = p_data;
    mail->result    = result;
    mail_box.put(mail);
}

void grRobot_audio_stream_task(void const*) {
    NetworkInterface* network = grInitEth();
    bool isStreaming = true;
    Websocket ws(ADDRESS_WS_SERVER, network);

    while (1) {
        Thread::signal_wait(0x1);

        isStreaming = true;
        DBG_INFO("Trying to connect\r\n");
        int connect_error = ws.connect();
        if (!connect_error) {
            DBG_INFO("error connecting to server\r\n");
            Thread::wait(500);
            notifyMain_websocketClose(0);
            continue;
        }
        DBG_INFO("Ws voice streaming\r\n");

        while (isStreaming) {
            osEvent evt = mail_box.get();
            mail_t *mail = (mail_t *)evt.value.p;

            int error_c = ws.send((uint8_t*)mail->p_data, mail->result);
            mail_box.free(mail);
            if (error_c == -1) {
                ws.close();
                Thread::wait(100);
                notifyMain_websocketClose(0);
                isStreaming = false;
                break;
            }

            if (isButtonPressed()) {
                DBG_INFO("Button pressed, close socket and stream again\r\n");
                ws.close();
                Thread::wait(500);
                notifyMain_websocketClose(0);
                isStreaming = false;
                break;
            }
        }
    }
}

void grRobot_audioDisableRead()
{
    isEnableRead = false;
    while (isReallyDisable == false) {
        Thread::wait(10);
    }
}

void grRobot_audioEnableRead()
{
    TLV320_RBSP *audio = grAudio();
    // Microphone
    audio->mic(true);   // Input select for ADC is microphone.
    audio->format(16);
    audio->power(0x00); // mic on
    audio->inputVolume(0.8, 0.8);
    audio->frequency(16000);
    isEnableRead = true;
}

void grRobot_audio_read_task(void const*) {
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
        if (isEnableRead == false) {
            isReallyDisable = true;
            Thread::wait(100);
        }

        isReallyDisable = false;
        for (cnt = 0; cnt < READ_BUFF_NUM && isEnableRead == true; cnt++) {
            if (audio->read(audio_read_buff[cnt], READ_BUFF_SIZE, &audio_read_data) < 0) {
                DBG_INFO("read error\n");
            }
        }
    }
}
