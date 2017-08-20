#include "select-demo.h"

#if DEMO == DEMO_HTTP

#include "mbed.h"
#include "easy-connect.h"
#include "http_request.h"


#include <string>
#include <vector>
#include <map>
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

mbedtls_sha1_context httpBodyHashCtx;
unsigned char hashOutput[20];

Serial pc(USBTX, USBRX);

void dump_response(HttpResponse* res) {
    printf("Status: %d - %s\n", res->get_status_code(), res->get_status_message().c_str());

    printf("Headers:\n");
    for (size_t ix = 0; ix < res->get_headers_length(); ix++) {
        printf("\t%s: %s\n", res->get_headers_fields()[ix]->c_str(), res->get_headers_values()[ix]->c_str());
    }
    printf("\nBody (%d bytes):\n\n%s\n", res->get_body_length(), res->get_body_as_string().c_str());
}

// [Ref-Source](https://gist.github.com/ccbrown/9722406)
void DumpHex(const void* data, size_t size) {
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++i) {
        printf("%02X ", ((unsigned char*)data)[i]);
        if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
            ascii[i % 16] = ((unsigned char*)data)[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i+1) % 8 == 0 || i+1 == size) {
            printf(" ");
            if ((i+1) % 16 == 0) {
                printf("|  %s \n", ascii);
            } else if (i+1 == size) {
                ascii[(i+1) % 16] = '\0';
                if ((i+1) % 16 <= 8) {
                    printf(" ");
                }
                for (j = (i+1) % 16; j < 16; ++j) {
                    printf("   ");
                }
                printf("|  %s \n", ascii);
            }
        }
    }
}

static void on_body_cb(const char *at, size_t length)
{
    char *startMark = ">>>>>>";
    char *endMark = "<<<<<<";

    // return audio_stream_consumer(at, length, parser->data);
    printf("On body: %d\r\n", length);
    printf("%sStart body%s\r\n", startMark, startMark);
    DumpHex(at, length);
    printf("%sEnd body%s\r\n", endMark, endMark);
    mbedtls_sha1_update(&httpBodyHashCtx, (const unsigned char*)at, length);
}

int main() {
    mbedtls_sha1_init(&httpBodyHashCtx);
    mbedtls_sha1_starts(&httpBodyHashCtx);

    pc.baud(115200);
    // Connect to the network (see mbed_app.json for the connectivity method used)
    NetworkInterface* network = easy_connect(true);
    if (!network) {
        printf("Cannot connect to the network, see serial output");
        return 1;
    }

    // Do a GET request to httpbin.org
    {
        // By default the body is automatically parsed and stored in a buffer, this is memory heavy.
        // To receive chunked response, pass in a callback as last parameter to the constructor.
        HttpRequest* get_req = new HttpRequest(network, HTTP_GET, "http://192.168.1.162:8080", on_body_cb);

        HttpResponse* get_res = get_req->send();
        if (!get_res) {
            printf("HttpRequest failed (error code %d)\n", get_req->get_error());
            return 1;
        }

        printf("\n----- HTTP GET response -----\n");
        dump_response(get_res);

        delete get_req;
    }

    mbedtls_sha1_finish(&httpBodyHashCtx, hashOutput);
    printf("Sha1: \r\n");
    DumpHex(hashOutput, sizeof(hashOutput));

    Thread::wait(osWaitForever);
}

#endif
