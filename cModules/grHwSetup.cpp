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
** @author  zealot
** - Development
==============================================================================*/

/******************************************************************************/
/* INCLUSIONS                                                                 */
/******************************************************************************/
#include "grHwSetup.h"
#include "grUtility.h"
#include "dec_wav.h"
#include "easy-connect.h"

#include "http_request.h"
#include "http_parser.h"
#include "http_response.h"
#include "http_request_builder.h"
#include "http_response_parser.h"
#include "http_parsed_url.h"

/******************************************************************************/
/* LOCAL CONSTANT AND COMPILE SWITCH SECTION                                  */
/******************************************************************************/
#define WRITE_BUFF_NUM         (16)
#define READ_BUFF_SIZE         (4096)
#define READ_BUFF_NUM          (16)

/******************************************************************************/
/* LOCAL TYPE DEFINITION SECTION                                              */
/******************************************************************************/


/******************************************************************************/
/* LOCAL MACRO DEFINITION SECTION                                             */
/******************************************************************************/


/******************************************************************************/
/* MODULE'S LOCAL VARIABLE DEFINITION SECTION                                 */
/******************************************************************************/
TLV320_RBSP audio(P10_13, I2C_SDA, I2C_SCL, P4_4, P4_5, P4_7, P4_6, 0x80, WRITE_BUFF_NUM, READ_BUFF_NUM); // I2S Codec
DigitalIn  button(USER_BUTTON0);
DigitalOut usb1en(P3_8);
Serial pc(USBTX, USBRX, 115200);
FATFileSystem usbfs("usb");
USBHostMSD msd;

//Tag buffer
uint8_t title_buf[TEXT_SIZE];
uint8_t artist_buf[TEXT_SIZE];
uint8_t album_buf[TEXT_SIZE];
uint8_t audio_write_buff[AUDIO_WRITE_BUFF_NUM][AUDIO_WRITE_BUFF_SIZE]
                __attribute((section("NC_BSS"),aligned(4)));

/******************************************************************************/
/* LOCAL (STATIC) VARIABLE DEFINITION SECTION                                 */
/******************************************************************************/


/******************************************************************************/
/* LOCAL (STATIC) FUNCTION DECLARATION SECTION                                */
/******************************************************************************/
static void callback_audio_write_end(void * p_data, int32_t result, void * p_app_data);


/******************************************************************************/
/* LOCAL FUNCTION DEFINITION SECTION                                          */
/******************************************************************************/
static void callback_audio_write_end(void * p_data, int32_t result, void * p_app_data) {
    if (result < 0) {
        printf("audio write callback error %d\n", result);
    }
}

/******************************************************************************/
/* GLOBAL FUNCTION DEFINITION SECTION                                         */
/******************************************************************************/
bool isButtonPressed() {
    if (button.read() == 0) {
        Thread::wait(200);
        if (button.read() == 0) {
            return true;
        }
    }
    
    return false;
}

bool isButtonRelease() {
    return (button.read() == 1);
}

NetworkInterface *grInitEth() {
    // Connect to the network (see mbed_app.json for the connectivity method used)
    NetworkInterface* network = easy_connect(true);
    if (!network) {
        printf("Cannot connect to the network, see serial output");
        return NULL;
    }
    return network;
}

void grEnableUSB1() {
    //Audio Shield USB1 enable
    usb1en = 1;        //Outputs high level
    Thread::wait(5);
    usb1en = 0;        //Outputs low level
}

void grEnableAudio() {
    audio.power(0x02); // mic off
    audio.inputVolume(0.7, 0.7);
}

void grSetupUsb() {
    // try to connect a MSD device
    printf("Waiting for usb\r\n");
    while(!msd.connect()) {
        Thread::wait(200);
    }

    printf("USB connected \r\n");
    // Now that the MSD device is connected, file system is mounted.
    usbfs.mount(&msd);
}

void grUnmoutUsb() {
    usbfs.unmount();
}

TLV320_RBSP *grAudio() {
    return &audio;
}

static FILE * fp_download = NULL;
static uint32_t totalDownload = 0;
static void on_body_save_file_cb(const char *at, size_t length)
{
    int written = fwrite(at, length, 1, fp_download);
    totalDownload += length;
}

int grDownloadFile(NetworkInterface* network, char *fileName, char *address) {
    int ret = -1;
    char file_path[sizeof(FLD_PATH) + FILE_NAME_LEN];

    totalDownload = 0;

    strcpy(file_path, FLD_PATH);
    strcat(file_path, fileName);
    fp_download = fopen(file_path, "w");
    if (fp_download == NULL) {
        printf("open %s to write failed\r\n", file_path);
        goto clean_up;
    }

    {
        // Do a GET request to wav file
        // By default the body is automatically parsed and stored in a buffer, this is memory heavy.
        // To receive chunked response, pass in a callback as last parameter to the constructor.
        HttpRequest* get_req = new HttpRequest(network, HTTP_GET, address, on_body_save_file_cb);

        HttpResponse* get_res = get_req->send();
        if (!get_res) {
            printf("HttpRequest failed (error code %d)\n", get_req->get_error());
            ret = -1;
            delete get_req;
            goto clean_up;
        }
        printf("\n----- HTTP GET response -----\n");
        dump_response(get_res);
        printf("Download %d bytes\r\n", totalDownload);
        ret = 0;
        delete get_req;
    }

clean_up:
    if (fp_download) {
        fclose(fp_download);
        fp_download = NULL;
    }
    return ret;
}

int grPlayWavFile(char *fileName) {
    TLV320_RBSP *audio = grAudio();
    FILE * fp = NULL;
    dec_wav wav_file;
    int buff_index = 0;
    size_t audio_data_size;
    rbsp_data_conf_t audio_write_async_ctl = {&callback_audio_write_end, NULL};

    if (fileName == NULL) {
        return -1;
    }

    char file_path[sizeof(FLD_PATH) + FILE_NAME_LEN];

    strcpy(file_path, FLD_PATH);
    strcat(file_path, fileName);

    fp = fopen(file_path, "r");
    if (wav_file.AnalyzeHeder(title_buf, artist_buf, album_buf,
                               TEXT_SIZE, fp) == false) {
        fclose(fp);
        fp = NULL;
    } else if ((wav_file.GetChannel() != 2)
            || (audio->format(wav_file.GetBlockSize()) == false)
            || (audio->frequency(wav_file.GetSamplingRate()) == false)) {
        printf("Error File  :%s\n", fileName);
        printf("Audio Info  :%dch, %dbit, %dHz\n", wav_file.GetChannel(),
                wav_file.GetBlockSize(), wav_file.GetSamplingRate());
        printf("\n");
        fclose(fp);
        fp = NULL;
    } else {
        printf("File        :%s\n", fileName);
        printf("Audio Info  :%dch, %dbit, %dHz\n", wav_file.GetChannel(),
                wav_file.GetBlockSize(), wav_file.GetSamplingRate());
        printf("Title       :%s\n", title_buf);
        printf("Artist      :%s\n", artist_buf);
        printf("Album       :%s\n", album_buf);
        printf("\n");
    }

    while (1) {
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
        if ((audio_data_size < AUDIO_WRITE_BUFF_SIZE)) {
            fclose(fp);
            fp = NULL;
            Thread::wait(500);
            printf("Done playing file\r\n");
            return 0;
        }
    }
}

/************************* End of File ****************************************/
