#include "select-demo.h"
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

Mutex test1Mutex;
Mutex test2Mutex;

Semaphore test1Sem(1);
Semaphore test2Sem(1);

void test1(void const* pvParameters)
{
    while (1) {
        test1Sem.wait();
        printf("Test1\r\n");
        Thread::wait(1000);
        test2Sem.release();
    }
}

void test2(void const* pvParameters)
{
    while(1) {
        test2Sem.wait();
        printf("Test2\r\n");
        // test2Sem.wait();
        test1Sem.release();
    }
}
