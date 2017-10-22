/*
 * audio_player.c
 *
 *  Created on: 12.03.2017
 *      Author: michaelboeckling
 */

#include <stdlib.h>
#include "audio_player.h"
#include "spiram_fifo.h"
#include "mp3_decoder.h"

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

#define TAG "[audio_player] "

#define ESP_LOGI(tag, ...)           printf(tag); printf(__VA_ARGS__); printf("\r\n");
#define ESP_LOGE(tag, ...)           printf(tag); printf(__VA_ARGS__); printf("\r\n");

// #define PRIO_MAD configMAX_PRIORITIES - 2

static player_t *player_instance = NULL;
static component_status_t player_status = UNINITIALIZED;

static int start_decoder_task(player_t *player)
{
    ESP_LOGI(TAG, "creating decoder task");

    Thread mp3DecoderTask(mp3_decoder_task, (void *)player, osPriorityNormal, 8448*32);

    return 0;
}

static int t;

/* Writes bytes into the FIFO queue, starts decoder task if necessary. */
int audio_stream_consumer(const char *recv_buf, ssize_t bytes_read,
        void *user_data)
{
    player_t *player = (player_t *)user_data;

    // don't bother consuming bytes if stopped
    if(player->command == CMD_STOP) {
        player->decoder_command = CMD_STOP;
        player->command = CMD_NONE;
        return -1;
    }

    if (bytes_read > 0) {
        spiRamFifoWrite(recv_buf, bytes_read);
    }

    int bytes_in_buf = spiRamFifoFill();
    uint8_t fill_level = (bytes_in_buf * 100) / spiRamFifoLen();

    // seems 4k is enough to prevent initial buffer underflow
    uint8_t min_fill_lvl = player->buffer_pref == BUF_PREF_FAST ? 20 : 90;
    bool enough_buffer = fill_level > min_fill_lvl;

    bool early_start = (bytes_in_buf > 1028 && player->media_stream->eof);

    t = (t + 1) & 255;
    if (t == 0) {
        ESP_LOGI(TAG, "Buffer fill %u%%, %d bytes", fill_level, bytes_in_buf);
    }

    return 0;
}

void audio_player_init(player_t *player)
{
    player_instance = player;
    player_status = INITIALIZED;
}

void audio_player_destroy()
{
    // renderer_destroy();
    player_status = UNINITIALIZED;
}

void audio_player_start()
{
    // renderer_start();
    player_status = RUNNING;
}

void audio_player_stop()
{
    // renderer_stop();
    player_instance->command = CMD_STOP;
    // player_status = STOPPED;
}

component_status_t get_player_status()
{
    return player_status;
}

