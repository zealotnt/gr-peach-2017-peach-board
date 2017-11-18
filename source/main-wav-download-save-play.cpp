/*
* @Author: zealotnt
* @Date:   2017-09-27 11:20:27
* @Last Modified by:   Tran Tam
* @Last Modified time: 2017-09-27 11:23:17
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

int main_wav_player_func() {
    printf("main_wav_player_func\r\n");

    grEnableUSB1();
    grEnableAudio();
    grSetupUsb();

    grPlayWavFile("NoiNayCoAnh.wav");
    // grPlayWavFile("good_8khz.wav");
}

int main_download_save_play() {
    Timer countTimer;
    NetworkInterface* network = grInitEth();
    grEnableUSB1();
    grEnableAudio();
    grSetupUsb();

    while (1) {
        printf("Wait for button\r\n");
        while (isButtonRelease()) {
            Thread::wait(100);
        }

        printf("Button press\r\n");
        countTimer.start();
        grDownloadFile(network, "file_to_write.txt", ADDRESS_HTTP_SERVER);
        countTimer.stop();
        printf("Download done in %d ms, play file\r\n", countTimer.read_ms());
        countTimer.reset();
        grPlayWavFile("file_to_write.txt");
    }

    Thread::wait(osWaitForever);
}

int main_save_file() {
    printf("Get Wav file\r\n");
    FILE * fp = NULL;
    DIR  * d = NULL;
    char file_path[sizeof(FLD_PATH) + FILE_NAME_LEN];

    //Audio Shield USB1 enable
    grEnableUSB1();

    // try to connect a MSD device
    grSetupUsb();

    strcpy(file_path, FLD_PATH);
    strcat(file_path, "file_to_write.txt");
    fp = fopen(file_path, "w");
    if (fp == NULL) {
        printf("open %s to write failed\r\n", file_path);
    }

    char *data_to_write = "Hello world";
    int written = fwrite(data_to_write, strlen(data_to_write), 1, fp);
    printf("Write %d bytes to file\r\n", written*strlen(data_to_write));
    fclose(fp);
    grUnmoutUsb();
    printf("Write done, loop forerver\r\n");
    Thread::wait(osWaitForever);
}
