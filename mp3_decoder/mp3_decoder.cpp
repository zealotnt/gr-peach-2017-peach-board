/*
 * mp3_decoder.c
 *
 *  Created on: 13.03.2017
 *      Author: michaelboeckling
 */
#include "TLV320_RBSP.h"
#include "grUtility.h"
#include "grHwSetup.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "rtos.h"
#include "mbed.h"

#include "../mad/mad.h"
#include "../mad/stream.h"
#include "../mad/frame.h"
#include "../mad/synth.h"

#include "audio_player.h"
#include "spiram_fifo.h"
#include "mp3_decoder.h"
#include "common_buffer.h"

#include <string>
#include <vector>
#include <map>

#define TAG "[mad_decoder] "

#define ESP_LOGI(tag, ...)                           printf(tag); printf(__VA_ARGS__); printf("\r\n");
#define ESP_LOGE(tag, ...)                           printf(tag); printf(__VA_ARGS__); printf("\r\n");
#define renderer_zero_dma_buffer(...)
#define vTaskDelete(...)

// The theoretical maximum frame size is 2881 bytes,
// MPEG 2.5 Layer II, 8000 Hz @ 160 kbps, with a padding slot plus 8 byte MAD_BUFFER_GUARD.
#define MAX_FRAME_SIZE (2889)

// The theoretical minimum frame size of 24 plus 8 byte MAD_BUFFER_GUARD.
#define MIN_FRAME_SIZE (32)

static long buf_underrun_cnt;

static enum mad_flow input(struct mad_stream *stream, common_buffer_t *buf, player_t *player)
{
    int bytes_to_read;

    // next_frame is the position MAD is interested in resuming from
    uint32_t bytes_consumed  = stream->next_frame - stream->buffer;
    buf_seek_rel(buf, bytes_consumed);

    while (1) {

        // stop requested, terminate immediately
        if(player->decoder_command == CMD_STOP) {
            return MAD_FLOW_STOP;
        }

        // Calculate amount of bytes we need to fill buffer.
        bytes_to_read = min(buf_free_capacity_after_purge(buf), spiRamFifoFill());

        // Can't take anything?
        if (bytes_to_read == 0) {

            // EOF reached, stop decoder when all frames have been consumed
            if(player->media_stream->eof) {
                return MAD_FLOW_STOP;
            }

            //Wait until there is enough data in the buffer. This only happens when the data feed
            //rate is too low, and shouldn't normally be needed!
            ESP_LOGE(TAG, "Buffer underflow, need %d bytes.", buf_free_capacity_after_purge(buf));
            buf_underrun_cnt++;
            //We both silence the output as well as wait a while by pushing silent samples into the i2s system.
            //This waits for about 200mS
            renderer_zero_dma_buffer();
            Thread::wait(200);
        } else {
            //Read some bytes from the FIFO to re-fill the buffer.
            fill_read_buffer(buf);

            // break the loop if we have at least enough to decode a few of the smallest possible frame
            if(buf_data_unread(buf) >= (MIN_FRAME_SIZE * 4))
                break;
        }
    }

    // Okay, let MAD decode the buffer.
    mad_stream_buffer(stream, (unsigned char*) buf->read_pos, buf_data_unread(buf));
    return MAD_FLOW_CONTINUE;
}


//Routine to print out an error
static enum mad_flow error(void *data, struct mad_stream *stream, struct mad_frame *frame) {
    ESP_LOGE(TAG, "dec err 0x%04x (%s)", stream->error, mad_stream_errorstr(stream));
    return MAD_FLOW_CONTINUE;
}

static void set_dac_sample_rate(int rate)
{
    // mad_buffer_fmt.sample_rate = rate;
}

static uint32_t audioWriteIdx = 0;
static uint32_t buff_index = 0;

static void callback_audio_write_end(void * p_data, int32_t result, void * p_app_data) {
    if (result < 0) {
        printf("audio write callback error %d\n", result);
    }
}

rbsp_data_conf_t audio_write_async_ctl = {&callback_audio_write_end, NULL};

/* render callback for the libmad synth */
static void render_sample_block(short *sample_buff_ch0, short *sample_buff_ch1, int num_samples, unsigned int num_channels)
{
    char *ptr_l = (char*)sample_buff_ch0;
    char *ptr_r = (char*)sample_buff_ch1;
    int bytes_pushed = 0;
    for (int i = 0; i < num_samples; i++) {
        uint8_t *p_buf = audio_write_buff[buff_index];
        p_buf[audioWriteIdx] = ptr_l[0];
        p_buf[audioWriteIdx+1] = ptr_l[1];
        p_buf[audioWriteIdx+2] = ptr_r[0];
        p_buf[audioWriteIdx+3] = ptr_r[1];
        bytes_pushed = 4;
        audioWriteIdx += 4;
        if (audioWriteIdx >= AUDIO_WRITE_BUFF_SIZE) {
            audioWriteIdx = 0;
            grAudio()->write(p_buf, AUDIO_WRITE_BUFF_SIZE, &audio_write_async_ctl);
            buff_index++;
            if (buff_index >= AUDIO_WRITE_BUFF_NUM) {
                buff_index = 0;
            }
        }

        // DMA buffer full - retry
        if (bytes_pushed == 0) {
            i--;
        } else {
            ptr_r += 2;
            ptr_l += 2;
        }
    }
}

//This is the main mp3 decoding task. It will grab data from the input buffer FIFO in the SPI ram and
//output it to the I2S port.
void mp3_decoder_task(void const* pvParameters)
{
    ESP_LOGI(TAG, "mp3_decoder_task: id=0x%x pvParameters=0x%x", Thread::gettid(), (uint32_t)pvParameters);
    player_t *player = (player_t *)pvParameters;

    int ret;
    struct mad_stream *stream;
    struct mad_frame *frame;
    struct mad_synth *synth;

    register_set_dac_sample_rate(set_dac_sample_rate);
    register_render_sample_block(render_sample_block);

    //Allocate structs needed for mp3 decoding
    stream = (struct mad_stream *)malloc(sizeof(struct mad_stream));
    frame = (struct mad_frame *)malloc(sizeof(struct mad_frame));
    synth = (struct mad_synth *)malloc(sizeof(struct mad_synth));
    common_buffer_t *buf = buf_create(MAX_FRAME_SIZE);

    if (stream==NULL) { ESP_LOGE(TAG, "malloc(stream) failed\n"); return; }
    if (synth==NULL) { ESP_LOGE(TAG, "malloc(synth) failed\n"); return; }
    if (frame==NULL) { ESP_LOGE(TAG, "malloc(frame) failed\n"); return; }
    if (buf==NULL) { ESP_LOGE(TAG, "buf_create() failed\n"); return; }

    buf_underrun_cnt = 0;

    ESP_LOGI(TAG, "decoder start");

    //Initialize mp3 parts
    mad_stream_init(stream);
    mad_frame_init(frame);
    mad_synth_init(synth);


    while(1) {

        // calls mad_stream_buffer internally
        if (input(stream, buf, player) == MAD_FLOW_STOP ) {
            break;
        }

        // decode frames until MAD complains
        while(1) {

            if(player->decoder_command == CMD_STOP) {
                goto abort;
            }

            // returns 0 or -1
            ret = mad_frame_decode(frame, stream);
            if (ret == -1) {
                if (!MAD_RECOVERABLE(stream->error)) {
                    //We're most likely out of buffer and need to call input() again
                    break;
                }
                error(NULL, stream, frame);
                continue;
            }
            mad_synth_frame(synth, frame);
        }
        // ESP_LOGI(TAG, "RAM left %d", esp_get_free_heap_size());
    }

    abort:
    // avoid noise
    renderer_zero_dma_buffer();

    free(synth);
    free(frame);
    free(stream);
    buf_destroy(buf);

    // clear semaphore for reader task
    spiRamFifoReset();

    player->decoder_status = STOPPED;
    player->decoder_command = CMD_NONE;
    ESP_LOGI(TAG, "decoder stopped");

    // ESP_LOGI(TAG, "MAD decoder stack: %d\n", uxTaskGetStackHighWaterMark(NULL));
    vTaskDelete(NULL);
}
