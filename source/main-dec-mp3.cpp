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

extern int audio_stream_consumer(const char *recv_buf, ssize_t bytes_read,
        void *user_data);
void mp3_decoder_task(void const* pvParameters);

void usbReadTask(void const* pvParameters)
{
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

    printf("usbReadTask: player_config=0x%x\r\n", (uint32_t)player_config);
    while (1) {
        Thread::wait(10);
        int sizeRead = fread(buffRead, sizeof(char), 1024, fpMp3);
        if (sizeRead <= 0) {
            player_config->command = CMD_STOP;
            printf("SizeRead = %d\r\n", sizeRead);
            Thread::wait(osWaitForever);
        }
        printf("Read success: %d\r\n", sizeRead);
        audio_stream_consumer((char *)buffRead, sizeRead, (void *)player_config);
    }
}

int main_dec_mp3()
{
    player_t player_config;
    player_config.command = CMD_START;
    player_config.decoder_status = INITIALIZED;
    player_config.decoder_command = CMD_START;
    player_config.buffer_pref = BUF_PREF_SAFE;
    player_config.media_stream = (media_stream_t *)calloc(1, sizeof(media_stream_t));

    Thread UsbReadTask(usbReadTask, (void *)&player_config, osPriorityAboveNormal, 1024*32);
    Thread mp3DecoderTask(mp3_decoder_task, (void *)&player_config, osPriorityNormal, 8448*32);

    printf("Task created\r\n");

    while (true) {
        Thread::wait(osWaitForever);
    }

    return 0;
}
