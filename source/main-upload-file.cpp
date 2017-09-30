#include "mbed.h"

#include <string>
#include <vector>
#include <map>

#include "http_request.h"
#include "http_parser.h"
#include "http_response.h"
#include "http_request_builder.h"
#include "http_response_parser.h"
#include "http_parsed_url.h"
#include "http_response_multipart_parser.h"
#include "multipart_parser.h"

#include "mbedtls/platform.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/sha1.h"

#include "cProcess.hpp"
#include "define.h"
#include "grUtility.h"
#include "grHwSetup.h"

int main_upload_file() {
    #define BUFF_SIZE       500000
    uint8_t *buffToSend = (uint8_t *)malloc(BUFF_SIZE);
    NetworkInterface* network = grInitEth();

    for (int i = 0; i < BUFF_SIZE; i++) {
        buffToSend[i] = i;
    }

    while (1) {
        printf("Wait for button\r\n");
        while (isButtonRelease()) {
            Thread::wait(100);
        }

        grUploadFile(network, buffToSend, BUFF_SIZE);
    }
}

int main_upload_file_from_usb() {
    uint8_t *fileBuff;
    uint32_t fileLen;
    char file_path[sizeof(FLD_PATH) + FILE_NAME_LEN];
    Timer countTimer;

    NetworkInterface* network = grInitEth();
    grEnableUSB1();
    grEnableAudio();
    grSetupUsb();

    strcpy(file_path, FLD_PATH);
    strcat(file_path, "good.wav");

    grReadFile(file_path, &fileBuff, &fileLen);

    while (1) {
        waitShortPress();
        grStartUpload(network);

        grUploadFile(network, fileBuff, fileLen);

        grEndUpload(network);

        Thread::wait(200);
        countTimer.start();
        grDownloadFile(network, "file_to_write.txt", ADDRESS_SERVER);
        countTimer.stop();
        printf("Download done in %d ms, play file\r\n", countTimer.read_ms());
        countTimer.reset();
        grPlayWavFile("file_to_write.txt");
    }
}
