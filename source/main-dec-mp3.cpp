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
player_t gPlayer_Config;

#define TAG "[mp3-feeder] "
#define LOGI(tag, ...)                           printf(tag); printf(__VA_ARGS__); printf("\r\n");

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
        LOGI(TAG, "open %s to write failed", file_path);
    }

    LOGI(TAG, "usbReadTask: id=0x%x player_config=0x%x", Thread::gettid(), (uint32_t)player_config);
    while (1) {
        Thread::wait(10);
        int sizeRead = fread(buffRead, sizeof(char), 1024, fpMp3);
        totalSize += sizeRead;
        if (sizeRead <= 0) {
            player_config->command = CMD_STOP;
            player_config->decoder_command = CMD_STOP;
            LOGI(TAG, "SizeRead = %d", sizeRead);
            Thread::wait(osWaitForever);
        }
        LOGI(TAG, "Read success: %d", totalSize);
        audio_stream_consumer((char *)buffRead, sizeRead, (void *)player_config);
    }
}

static void on_body_cb(const char *at, size_t length)
{
    audio_stream_consumer((char *)at, length, (void *)pPlayerConfig);
}

void http_download_task(void const* pvParameters)
{
    uint32_t totalSize = 0;
    player_t *player_config = (player_t *)pvParameters;
    pPlayerConfig = player_config;
    grEnableAudio();
    NetworkInterface* network = grInitEth();

    LOGI(TAG, "httpDownloadTask: id=0x%x player_config=0x%x",
        Thread::gettid(),
        (uint32_t)player_config);

    {
        // Do a GET request to wav file
        // By default the body is automatically parsed and stored in a buffer, this is memory heavy.
        // To receive chunked response, pass in a callback as last parameter to the constructor.
        HttpRequest* get_req = new HttpRequest(network, HTTP_GET, ADDRESS_HTTP_SERVER "/audio", on_body_cb);

        HttpResponse* get_res = get_req->send();
        if (!get_res) {
            LOGI(TAG, "HttpRequest failed (error code %d)", get_req->get_error());
        }
        while (spiRamFifoFill() != 0)
        {
            Thread::wait(50);
        }
        Thread::wait(100);
        player_config->command = CMD_STOP;
        player_config->decoder_command = CMD_STOP;
        delete get_req;
    }
    LOGI(TAG, "http downloader stopped");
}

void initPlayerConfig()
{
    gPlayer_Config.media_stream = (media_stream_t *)calloc(1, sizeof(media_stream_t));
}

void preparePlayerConfig()
{
    gPlayer_Config.command = CMD_START;
    gPlayer_Config.decoder_status = INITIALIZED;
    gPlayer_Config.decoder_command = CMD_START;
    gPlayer_Config.buffer_pref = BUF_PREF_SAFE;
}

#define HTTP_DOWNLOAD_STACK_SIZE            4096*2
#define MP3_DECODER_STACK_SIZE              8448*2
uint8_t httpDownloadTaskStk[HTTP_DOWNLOAD_STACK_SIZE];
uint8_t mp3DecoderStk[MP3_DECODER_STACK_SIZE];

void grRobot_mp3_player()
{
    #define TAG "[grRobot_mp3_player] "
    LOGI(TAG, "Task creating");
    preparePlayerConfig();
    Thread httpDownloadTask(http_download_task,
                            (void *)&gPlayer_Config,
                            osPriorityAboveNormal,
                            HTTP_DOWNLOAD_STACK_SIZE,
                            httpDownloadTaskStk);
    Thread mp3DecoderTask(mp3_decoder_task,
                          (void *)&gPlayer_Config,
                          osPriorityNormal,
                          MP3_DECODER_STACK_SIZE,
                          mp3DecoderStk);
    LOGI(TAG, "Task created");
    mp3DecoderTask.join();
    mp3DecoderTask.terminate();
    httpDownloadTask.join();
    httpDownloadTask.terminate();
    LOGI(TAG, "Done playing");
    #undef TAG
}

int main_dec_mp3_usb()
{
    #define TAG "[main_dec_mp3_usb] "
    initPlayerConfig();
    preparePlayerConfig();

    Thread UsbReadTask(usbReadTask, (void *)&gPlayer_Config, osPriorityAboveNormal, 4096*2);
    Thread mp3DecoderTask(mp3_decoder_task, (void *)&gPlayer_Config, osPriorityNormal, 8448*2);

    LOGI(TAG, "Task created");

    while (true) {
        Thread::wait(osWaitForever);
    }
    #undef TAG

    return 0;
}

int main_dec_mp3_http()
{
    #define TAG "[main_dec_mp3_http] "
    int playedTimes = 0;

    initPlayerConfig();

    while (true) {
        grRobot_mp3_player();
        playedTimes ++;
        LOGI(TAG, "Done play %d times", playedTimes);
    }
    #undef TAG
    return 0;
}
