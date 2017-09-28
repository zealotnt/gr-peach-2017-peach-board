/*==============================================================================
**
**                      Proprietary - Copyright (C) 2017
**------------------------------------------------------------------------------
** Supported MCUs      :
** Supported Compilers : GCC
**------------------------------------------------------------------------------
** File name         :
**
** Module name       :
**
**
** Summary:
**
**= History ====================================================================
** @date
** @author	zealot
** - Development
==============================================================================*/

/******************************************************************************/
/* INCLUSIONS                                                                 */
/******************************************************************************/
#include "grUtility.h"

#include "http_request.h"
#include "http_parser.h"
#include "http_response.h"
#include "http_request_builder.h"
#include "http_response_parser.h"
#include "http_parsed_url.h"
#include "http_response_multipart_parser.h"
#include "multipart_parser.h"

/******************************************************************************/
/* LOCAL CONSTANT AND COMPILE SWITCH SECTION                                  */
/******************************************************************************/
#define MAX_POST_PAYLOAD    600

/******************************************************************************/
/* LOCAL TYPE DEFINITION SECTION                                              */
/******************************************************************************/


/******************************************************************************/
/* LOCAL MACRO DEFINITION SECTION                                             */
/******************************************************************************/


/******************************************************************************/
/* MODULE'S LOCAL VARIABLE DEFINITION SECTION                                 */
/******************************************************************************/


/******************************************************************************/
/* LOCAL (STATIC) VARIABLE DEFINITION SECTION                                 */
/******************************************************************************/


/******************************************************************************/
/* LOCAL (STATIC) FUNCTION DECLARATION SECTION                                */
/******************************************************************************/


/******************************************************************************/
/* LOCAL FUNCTION DEFINITION SECTION                                          */
/******************************************************************************/


/******************************************************************************/
/* GLOBAL FUNCTION DEFINITION SECTION                                         */
/******************************************************************************/
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

void dump_response(HttpResponse* res) {
    printf("Status: %d - %s\n", res->get_status_code(), res->get_status_message().c_str());

    printf("Headers:\n");
    for (size_t ix = 0; ix < res->get_headers_length(); ix++) {
        printf("\t%s: %s\n", res->get_headers_fields()[ix]->c_str(), res->get_headers_values()[ix]->c_str());
    }
    printf("\nBody (%d bytes):\n\n%s\n", res->get_body_length(), res->get_body_as_string().c_str());
}

int grUploadFile(NetworkInterface* network, uint8_t *buff, uint32_t buffLen)
{
    int packetIdx = 0;
    int packetToSend;
    int lastPacketSize;

    packetToSend = buffLen / MAX_POST_PAYLOAD;
    packetToSend += ((buffLen % MAX_POST_PAYLOAD) != 0);
    lastPacketSize = buffLen % MAX_POST_PAYLOAD;

    printf("Start POST to server, need to send %d bytes in %d times\r\n", buffLen, packetToSend);

    HttpRequest* post_req = new HttpRequest(network, HTTP_POST, "http://192.168.1.162:8080/upload");

    post_req->set_header("Content-Type", "multipart/form-data; boundary=123456789");
    post_req->set_header("Accept-Language", "en-US,en;q=0.5");
    post_req->set_header("Accept-Encoding", "gzip, deflate");
    post_req->set_header("Connection","keep-alive");

    HttpResponse* post_res;
    Timer t;
    t.start();
    for(int k = 0; k < packetToSend; k++)
    {
        char body[4096];
        int firstPartLen;
        sprintf(body,"%s%04d%s",
            "--123456789\r\n"
            "Content-Disposition: form-data; name=\"files\"; filename=\"f",
            k,
            "\"\r\nContent-Type: text/plain\r\n\r\n");

        firstPartLen = strlen(body);

        if (k == (packetToSend - 1)) {
            // last packet
            memcpy(body + firstPartLen, buff + packetIdx, lastPacketSize);
            firstPartLen += lastPacketSize;
        } else {
            memcpy(body + firstPartLen, buff + packetIdx, MAX_POST_PAYLOAD);
            firstPartLen += MAX_POST_PAYLOAD;
            packetIdx += MAX_POST_PAYLOAD;
        }

        memcpy(body + firstPartLen, "\r\n--123456789", strlen("\r\n--123456789"));
        firstPartLen += strlen("\r\n--123456789");

        post_res = post_req->send(body, firstPartLen);
        if (!post_res) {
            printf("HttpRequest failed (error code %d)\n", post_req->get_error());
            return 1;
        }

        post_req->clearLastResponse();
    }
    t.stop();
    printf("\nDone Upload file takes %f sec\r\n",t.read());

    delete post_req;
}

int grReadFile(char *filePath, uint8_t **fileVal, uint32_t *fileLen)
{
    FILE * fp = NULL;
    uint32_t fileSize;
    int ret = -1;

    fp = fopen(filePath, "r");
    if (fp == NULL) {
        printf("read file %s failed\r\n", filePath);
        return -1;
    }

    /* Get size of file */
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    *fileVal = (uint8_t*)malloc(fileSize + 1);
    if (*fileVal == NULL) {
        printf("Out of memory\r\n");
        return -1;
    }
    memset(*fileVal, 0x00, fileSize + 1);

    /* Seek to beginning of file */
    fseek(fp, SEEK_SET, 0);

    /* Read content of file */
    fread(*fileVal, 1, fileSize, fp);

    if (fileLen != NULL)
        *fileLen = fileSize;
    if (fp)
        fclose(fp);
    return ret;
}
/************************* End of File ****************************************/
