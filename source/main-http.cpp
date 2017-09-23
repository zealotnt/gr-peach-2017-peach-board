#include "select-demo.h"

#if DEMO == DEMO_HTTP

#include <string>
#include <vector>
#include <map>

#include "mbed.h"
#include "easy-connect.h"
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

#define USB_HOST_CH     1
#define DUMP_HTTP_BODY 0

#define AUDIO_WRITE_BUFF_SIZE  (4096)
#define AUDIO_WRITE_BUFF_NUM   (9)
#define FILE_NAME_LEN          (64)
#define TEXT_SIZE              (64 + 1) //null-terminated
#define FLD_PATH               "/usb/"

static uint8_t audio_write_buff[AUDIO_WRITE_BUFF_NUM][AUDIO_WRITE_BUFF_SIZE]
                __attribute((section("NC_BSS"),aligned(4)));
//Tag buffer
static uint8_t title_buf[TEXT_SIZE];
static uint8_t artist_buf[TEXT_SIZE];
static uint8_t album_buf[TEXT_SIZE];

TLV320_RBSP audio(P10_13, I2C_SDA, I2C_SCL, P4_4, P4_5, P4_7, P4_6,
                  0x80, (AUDIO_WRITE_BUFF_NUM - 1), 0);
DigitalIn  button(USER_BUTTON0);
DigitalOut usb1en(P3_8);
mbedtls_sha1_context httpBodyHashCtx;
unsigned char hashOutput[20];
Serial pc(USBTX, USBRX);

static void callback_audio_write_end(void * p_data, int32_t result, void * p_app_data) {
    if (result < 0) {
        printf("audio write callback error %d\n", result);
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

int main_save_file();
int main_wav_player();
int main_http();
int main_download_and_save_file();

int main() {
    // return main_http();
    return main_save_file();
    // return main_wav_player();
    // return main_download_and_save_file();
}

int main_download_and_save_file() {
    // Connect to the network (see mbed_app.json for the connectivity method used)
    NetworkInterface* network = easy_connect(true);
    if (!network) {
        printf("Cannot connect to the network, see serial output");
        return 1;
    }

    // Do a GET request to wav file
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
}

int main_save_file() {
    pc.baud(115200);
    printf("Get Wav file\r\n");
    FILE * fp = NULL;
    DIR  * d = NULL;
    char file_path[sizeof(FLD_PATH) + FILE_NAME_LEN];
    FATFileSystem fs("usb");
    USBHostMSD msd;

    //Audio Shield USB1 enable
    usb1en = 1;        //Outputs high level
    Thread::wait(5);
    usb1en = 0;        //Outputs low level

    // try to connect a MSD device
    printf("Waiting for usb\r\n");
    while(!msd.connect()) {
        Thread::wait(500);
    }

    printf("USB connected \r\n");
    // Now that the MSD device is connected, file system is mounted.
    fs.mount(&msd);

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

    printf("Write done, loop forerver\r\n");
    fs.unmount();
    Thread::wait(osWaitForever);
}

int main_wav_player() {
    pc.baud(115200);
    printf("Hello world\r\n");
    rbsp_data_conf_t audio_write_async_ctl = {&callback_audio_write_end, NULL};
    FILE * fp = NULL;
    DIR  * d = NULL;
    char file_path[sizeof(FLD_PATH) + FILE_NAME_LEN];
    int buff_index = 0;
    size_t audio_data_size;
    dec_wav wav_file;

    FATFileSystem fs("usb");
    USBHostMSD msd;

    //Audio Shield USB1 enable
    usb1en = 1;        //Outputs high level
    Thread::wait(5);
    usb1en = 0;        //Outputs low level

    audio.power(0x02); // mic off
    audio.inputVolume(0.7, 0.7);

    while(1) {
        // try to connect a MSD device
        printf("Waiting for usb\r\n");
        while(!msd.connect()) {
            Thread::wait(500);
        }

        printf("USB connected \r\n");
        // Now that the MSD device is connected, file system is mounted.
        fs.mount(&msd);

        // in a loop, append a file
        // if the device is disconnected, we try to connect it again
        while(1) {
            // if device disconnected, try to connect again
            if (!msd.connected()) {
                break;
            }
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
                                || (audio.format(wav_file.GetBlockSize()) == false)
                                || (audio.frequency(wav_file.GetSamplingRate()) == false)) {
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
                    audio.write(p_buf, audio_data_size, &audio_write_async_ctl);
                    buff_index++;
                    if (buff_index >= AUDIO_WRITE_BUFF_NUM) {
                        buff_index = 0;
                    }
                }

                // file close
                if ((audio_data_size < AUDIO_WRITE_BUFF_SIZE) || (button == 0)) {
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

int main_http() {
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
