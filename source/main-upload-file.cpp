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
#include "multipart_parser.h"
#include "http_response_multipart_parser.h"

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

static cStorage *myStorage = NULL;
DigitalOut led(LED1);

int main_upload_file() {
    NetworkInterface* network = grInitEth();

    while (1) {
        printf("Wait for button\r\n");
        while (isButtonRelease()) {
            Thread::wait(100);
        }

        {
            printf("Start POST to server\n");
            HttpRequest* post_req = new HttpRequest(network, HTTP_POST, "http://192.168.1.162:8080/upload");

            post_req->set_header("Content-Type", "multipart/form-data; boundary=123456789");
            post_req->set_header("Accept-Language", "en-US,en;q=0.5");
            post_req->set_header("Accept-Encoding", "gzip, deflate");
            post_req->set_header("Connection","keep-alive");

            HttpResponse* post_res;
            Timer t;
            t.start();
            for(int k = 0;k < 100;k++)
            {
                //printf("POST FILE %d\n",k);
                char body1[4096];
                int firstPartLen;
                sprintf(body1,"%s%d%s",
                    "--123456789\r\n"
                    "Content-Disposition: form-data;"
                    " name=\"text\"\r\n\r\nmy text here\r\n"
                    "--123456789\r\n"
                    "Content-Disposition: form-data; name=\"files\"; filename=\"test",
                    k,
                    ".txt\"\r\nContent-Type: text/plain\r\n\r\nHello world!\n");

                firstPartLen = strlen(body1);
                uint8_t tmp[100];
                for (int loop = 0; loop < 100; loop++) {
                    tmp[loop] = loop;
                }
                for (int loop = 0; loop < 6; loop++) {
                    memcpy(body1+firstPartLen, tmp, 100);
                    firstPartLen += 100;
                }

                memcpy(body1+firstPartLen, "\r\n\r\n--123456789", strlen("\r\n\r\n--123456789"));
                firstPartLen += strlen("\r\n\r\n--123456789");

                post_res = post_req->send(body1, firstPartLen);
                if (!post_res) {
                    printf("HttpRequest failed (error code %d)\n", post_req->get_error());
                    return 1;
                }

                post_req->clearLastResponse();
            }
            t.stop();
            printf("\n----- Done HTTPS POST response -----%f\n",t.read());
            dump_response(post_res);

            delete post_req;
        }
    }

}
