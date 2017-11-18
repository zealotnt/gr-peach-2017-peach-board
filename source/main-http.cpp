/*
* @Author: zealotnt
* @Date:   2017-09-27 11:22:21
* @Last Modified by:   Tran Tam
* @Last Modified time: 2017-09-27 11:22:39
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

#define DUMP_HTTP_BODY 0

mbedtls_sha1_context httpBodyHashCtx;
unsigned char hashOutput[20];

static void on_body_cb(const char *at, size_t length)
{
#if DUMP_HTTP_BODY
    char *startMark = ">>>>>>";
    char *endMark = "<<<<<<";
    printf("On body: %d\r\n", length);
    printf("%sStart body%s\r\n", startMark, startMark);
    DumpHex(at, length);
    printf("%sEnd body%s\r\n", endMark, endMark);
#endif
    mbedtls_sha1_update(&httpBodyHashCtx, (const unsigned char*)at, length);
}

int main_http() {
    mbedtls_sha1_init(&httpBodyHashCtx);
    mbedtls_sha1_starts(&httpBodyHashCtx);

    NetworkInterface* network = grInitEth();

    while (true) {
        #if 0
        waitShortPress();
        // Do a GET request to httpbin.org
        {
            // By default the body is automatically parsed and stored in a buffer, this is memory heavy.
            // To receive chunked response, pass in a callback as last parameter to the constructor.
            // HttpRequest* get_req = new HttpRequest(network, HTTP_GET, "http://esp8266-8109ea.local", on_body_cb);
            HttpRequest* get_req = new HttpRequest(network, HTTP_GET, ADDRESS_HTTP_SERVER, on_body_cb);

            HttpResponse* get_res = get_req->send();
            if (!get_res) {
                printf("HttpRequest failed (error code %d)\n", get_req->get_error());
                return 1;
            }

            printf("\n----- HTTP GET response -----\n");
            dump_response(get_res);
            printf("\n----- END HTTP GET response -----\n");

            delete get_req;
        }

        mbedtls_sha1_finish(&httpBodyHashCtx, hashOutput);
        printf("Sha1: \r\n");
        DumpHex(hashOutput, sizeof(hashOutput));
        #endif

        waitShortPress();
        {
            HttpRequest* post_req = new HttpRequest(network, HTTP_POST, ADDRESS_HTTP_SERVER "/status/update");
            post_req->set_header("Content-Type", "application/json");

            const char body[] = "{\"hello\":\"world\"}";

            HttpResponse* post_res = post_req->send(body, strlen(body));
            if (!post_res) {
                printf("HttpRequest failed (error code %d)\n", post_req->get_error());
                return 1;
            }

            printf("\n----- HTTP POST response -----\n");
            dump_response(post_res);

            delete post_req;
        }
    }

    Thread::wait(osWaitForever);
}
