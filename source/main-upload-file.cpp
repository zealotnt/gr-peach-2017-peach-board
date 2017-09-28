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
    uint8_t buffToSend[5000];
    NetworkInterface* network = grInitEth();

    for (int i = 0; i < sizeof(buffToSend); i++) {
        buffToSend[i] = i;
    }

    while (1) {
        printf("Wait for button\r\n");
        while (isButtonRelease()) {
            Thread::wait(100);
        }

        grUploadFile(network, buffToSend, sizeof(buffToSend));
    }
}
