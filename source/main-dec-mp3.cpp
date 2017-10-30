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

#include "mp3_decoder.h"
#include "audio_player.h"
#include "spiram_fifo.h"

player_t *pPlayerConfig;

extern int audio_stream_consumer(const char *recv_buf, ssize_t bytes_read,
        void *user_data);
void mp3_decoder_task(void const* pvParameters);

void usbReadTask(void const* pvParameters)
{
    uint32_t totalSize = 0;
    player_t *player_config = (player_t *)pvParameters;
    grEnableUSB1();
    grEnableAudio();
    grSetupUsb();

    FILE * fpMp3 = NULL;
    char file_path[sizeof(FLD_PATH) + FILE_NAME_LEN];
    uint8_t buffRead[1024];

    spiRamFifoInit();

    strcpy(file_path, FLD_PATH);
    strcat(file_path, "NoiNayCoAnh.mp3");
    fpMp3 = fopen(file_path, "r");
    if (fpMp3 == NULL) {
        printf("open %s to write failed\r\n", file_path);
    }

    printf("usbReadTask: id=0x%x player_config=0x%x\r\n", Thread::gettid(), (uint32_t)player_config);
    while (1) {
        Thread::wait(10);
        int sizeRead = fread(buffRead, sizeof(char), 1024, fpMp3);
        totalSize += sizeRead;
        if (sizeRead <= 0) {
            player_config->command = CMD_STOP;
            player_config->decoder_command = CMD_STOP;
            printf("SizeRead = %d\r\n", sizeRead);
            Thread::wait(osWaitForever);
        }
        printf("Read success: %d\r\n", totalSize);
        audio_stream_consumer((char *)buffRead, sizeRead, (void *)player_config);
    }
}

int main_dec_mp3_usb()
{
    player_t player_config;
    player_config.command = CMD_START;
    player_config.decoder_status = INITIALIZED;
    player_config.decoder_command = CMD_START;
    player_config.buffer_pref = BUF_PREF_SAFE;
    player_config.media_stream = (media_stream_t *)calloc(1, sizeof(media_stream_t));

    Thread UsbReadTask(usbReadTask, (void *)&player_config, osPriorityAboveNormal, 4096*2);
    Thread mp3DecoderTask(mp3_decoder_task, (void *)&player_config, osPriorityNormal, 8448*2);

    printf("Task created\r\n");

    while (true) {
        Thread::wait(osWaitForever);
    }

    return 0;
}

static void on_body_cb(const char *at, size_t length)
{
    audio_stream_consumer((char *)at, length, (void *)pPlayerConfig);
}

void httpDownloadTask(void const* pvParameters)
{
    uint32_t totalSize = 0;
    player_t *player_config = (player_t *)pvParameters;
    pPlayerConfig = player_config;
    grEnableUSB1();
    grEnableAudio();
    grSetupUsb();
    NetworkInterface* network = grInitEth();

    printf("httpDownloadTask: id=0x%x player_config=0x%x\r\n",
        Thread::gettid(),
        (uint32_t)player_config);

    {
        // Do a GET request to wav file
        // By default the body is automatically parsed and stored in a buffer, this is memory heavy.
        // To receive chunked response, pass in a callback as last parameter to the constructor.
        HttpRequest* get_req = new HttpRequest(network, HTTP_GET, ADDRESS_SERVER, on_body_cb);

        HttpResponse* get_res = get_req->send();
        if (!get_res) {
            printf("HttpRequest failed (error code %d)\n", get_req->get_error());
            delete get_req;
        }
        printf("\n----- HTTP GET response -----\n");
        delete get_req;
    }
}

int main_dec_mp3_http()
{
    player_t player_config;
    player_config.command = CMD_START;
    player_config.decoder_status = INITIALIZED;
    player_config.decoder_command = CMD_START;
    player_config.buffer_pref = BUF_PREF_SAFE;
    player_config.media_stream = (media_stream_t *)calloc(1, sizeof(media_stream_t));

    Thread HttpDownloadTask(httpDownloadTask, (void *)&player_config, osPriorityAboveNormal, 4096*2);
    Thread mp3DecoderTask(mp3_decoder_task, (void *)&player_config, osPriorityNormal, 8448*2);

    printf("Task created\r\n");

    while (true) {
        Thread::wait(osWaitForever);
    }

    return 0;
}
