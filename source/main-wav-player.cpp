/*
* @Author: zealotnt
* @Date:   2017-09-27 10:45:02
* @Last Modified by:   Tran Tam
* @Last Modified time: 2017-09-27 10:47:06
*/
#include <string>
#include <vector>
#include <map>

#include "mbed.h"
#include "http_request.h"

#include "http_parser.h"
#include "http_response.h"
#include "http_request_builder.h"
#include "http_response_parser.h"
#include "http_parsed_url.h"

#include "mbedtls/platform.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/sha1.h"

#include "TLV320_RBSP.h"
#include "FATFileSystem.h"
#include "USBHostMSD.h"
#include "usb_host_setting.h"
#include "dec_wav.h"
#include "grUtility.h"
#include "grHwSetup.h"

static void callback_audio_write_end(void * p_data, int32_t result, void * p_app_data) {
    if (result < 0) {
        printf("audio write callback error %d\n", result);
    }
}

int main_wav_player() {
    printf("main_wav_player\r\n");
    rbsp_data_conf_t audio_write_async_ctl = {&callback_audio_write_end, NULL};
    FILE * fp = NULL;
    DIR  * d = NULL;
    char file_path[sizeof(FLD_PATH) + FILE_NAME_LEN];
    int buff_index = 0;
    size_t audio_data_size;
    dec_wav wav_file;
    TLV320_RBSP *audio = grAudio();

    grEnableUSB1();
    grEnableAudio();
    grSetupUsb();

    while(1) {

        // in a loop, append a file
        // if the device is disconnected, we try to connect it again
        while(1) {
            if (fp == NULL) {
                // file search
                if (d == NULL) {
                    d = opendir(FLD_PATH);
                }
                struct dirent * p;
                while ((p = readdir(d)) != NULL) {
                    size_t len = strlen(p->d_name);
                    if ((len > 4) && (len < FILE_NAME_LEN)
                        && (memcmp(&p->d_name[len - 4], ".wav", 4) == 0)) {
                        strcpy(file_path, FLD_PATH);
                        strcat(file_path, p->d_name);
                        printf("Opening path: \"%s\"\r\n", file_path);
                        fp = fopen(file_path, "r");
                        if (wav_file.AnalyzeHeder(title_buf, artist_buf, album_buf,
                                                   TEXT_SIZE, fp) == false) {
                            fclose(fp);
                            fp = NULL;
                        } else if ((wav_file.GetChannel() != 2)
                                || (audio->format(wav_file.GetBlockSize()) == false)
                                || (audio->frequency(wav_file.GetSamplingRate()) == false)) {
                            printf("Error File  :%s\n", p->d_name);
                            printf("Audio Info  :%dch, %dbit, %dHz\n", wav_file.GetChannel(),
                                    wav_file.GetBlockSize(), wav_file.GetSamplingRate());
                            printf("\n");
                            fclose(fp);
                            fp = NULL;
                        } else {
                            printf("File        :%s\n", p->d_name);
                            printf("Audio Info  :%dch, %dbit, %dHz\n", wav_file.GetChannel(),
                                    wav_file.GetBlockSize(), wav_file.GetSamplingRate());
                            printf("Title       :%s\n", title_buf);
                            printf("Artist      :%s\n", artist_buf);
                            printf("Album       :%s\n", album_buf);
                            printf("\n");
                            break;
                        }
                    }
                }
                if (p == NULL) {
                    closedir(d);
                    d = NULL;
                }
            } else {
                // file read
                uint8_t * p_buf = audio_write_buff[buff_index];

                audio_data_size = wav_file.GetNextData(p_buf, AUDIO_WRITE_BUFF_SIZE);
                if (audio_data_size > 0) {
                    audio->write(p_buf, audio_data_size, &audio_write_async_ctl);
                    buff_index++;
                    if (buff_index >= AUDIO_WRITE_BUFF_NUM) {
                        buff_index = 0;
                    }
                }

                // file close
                if (audio_data_size < AUDIO_WRITE_BUFF_SIZE) {
                    fclose(fp);
                    fp = NULL;
                    Thread::wait(500);
                }
            }
        }

        // close check
        if (fp != NULL) {
            fclose(fp);
            fp = NULL;
        }
        if (d != NULL) {
            closedir(d);
            d = NULL;
        }
    }
}