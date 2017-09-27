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
#include "grUtility.h"

#define USB_HOST_CH     1
#define DUMP_HTTP_BODY 1

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
Serial pc(USBTX, USBRX, 115200);

int playWavFile(char *fileName);

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

FILE * fp_download = NULL;
uint32_t totalDownload = 0;
static void on_body_save_file_cb(const char *at, size_t length)
{
    int written = fwrite(at, length, 1, fp_download);
    totalDownload += length;
}

int main_save_file();
int main_wav_player();
int main_http();
int main_download_and_save_file();
int main_wav_player_func();
int main_broadcast_receive();
int main_test_json();
int main_test_node_devices();

int main() {
    // return main_http();
    // return main_save_file();
    // return main_wav_player();
    // return main_download_and_save_file();
    // return main_wav_player_func();
    // return main_broadcast_receive();
    // return main_test_json();
    return main_test_node_devices();
}

#include "cNodeManager.h"

#define DEV_1_IP            "192.168.1.177"
#define DEV_1_NAME          "lamp-1"

int main_test_node_devices() {
    NetworkInterface* network = easy_connect(true);
    if (!network) {
        printf("Cannot connect to the network, see serial output");
        return 1;
    }

    NodeManager *peachDeviceManager = new NodeManager(network);
    peachDeviceManager->NodeAdd(DEV_1_IP, DEV_1_NAME);

    bool status;
    if (peachDeviceManager->NodeStatusUpdate(DEV_1_IP, &status) != 0) {
        printf("Can't update node relay status\r\n");
        return -1;
    }

    printf("Node %s: relay_status = %s\r\n", DEV_1_NAME, (status == true) ? "on" : "off");

    while(1) {
        while (button.read() == 1) {
            Thread::wait(100);
        }

        if (peachDeviceManager->NodeRelayStatus(DEV_1_IP) == false) {
            peachDeviceManager->NodeRelayOn(DEV_1_IP);
        } else {
            peachDeviceManager->NodeRelayOff(DEV_1_IP);
        }
    }
    return 0;
}

#include "Json.h"

#define logError(...)       printf(__VA_ARGS__); printf("\r\n");
#define logInfo(...)        printf(__VA_ARGS__); printf("\r\n");

int main_test_json() {
    // Note that the JSON object is 'escaped'.  One doesn't get escaped JSON
    // directly from the webservice, if the response type is APPLICATION/JSON
    // Just a little thing to keep in mind.
    const char * jsonSource = "{\"team\":\"Night Crue\",\"company\":\"TechShop\",\"city\":\"San Jose\",\"state\":\"California\",\"country\":\"USA\",\"zip\":95113,\"active\":true,\"members\":[{\"firstName\":\"John\",\"lastName\":\"Smith\",\"active\":false,\"hours\":18.5,\"age\":21},{\"firstName\":\"Foo\",\"lastName\":\"Bar\",\"active\":true,\"hours\":25,\"age\":21},{\"firstName\":\"Peter\",\"lastName\":\"Jones\",\"active\":false}]}";

    Json json ( jsonSource, strlen ( jsonSource ), 20 );

    if ( !json.isValidJson () )
    {
        logError ( "Invalid JSON: %s", jsonSource );
        return -1;
    }

    if ( json.type (0) != JSMN_OBJECT )
    {
        logError ( "Invalid JSON.  ROOT element is not Object: %s", jsonSource );
        return -1;
    }

    // Let's get the value of key "city" in ROOT object, and copy into
    // cityValue
    char cityValue [ 32 ];

    logInfo ( "Finding \"city\" Key ... " );
    // ROOT object should have '0' tokenIndex, and -1 parentIndex
    int cityKeyIndex = json.findKeyIndexIn ( "city", 0 );
    if ( cityKeyIndex == -1 )
    {
        // Error handling part ...
        logError ( "\"city\" does not exist ... do something!!" );
    }
    else
    {
        // Find the first child index of key-node "city"
        int cityValueIndex = json.findChildIndexOf ( cityKeyIndex, -1 );
        if ( cityValueIndex > 0 )
        {
            const char * valueStart  = json.tokenAddress ( cityValueIndex );
            int          valueLength = json.tokenLength ( cityValueIndex );
            strncpy ( cityValue, valueStart, valueLength );
            cityValue [ valueLength ] = 0; // NULL-terminate the string

            //let's print the value.  It should be "San Jose"
            logInfo ( "city: %s", cityValue );
        }
    }

    // More on this example to come, later.
    return 0;
}

#if 0
#include "mbed.h"
#include "EthernetInterface.h"
const int BROADCAST_PORT = 58083;

// https://os.mbed.com/handbook/Socket#broadcasting
// https://os.mbed.com/users/mbed_official/code/BroadcastReceive/docs/28ba970b3e23/main_8cpp_source.html
int main_broadcast_receive() {
    EthernetInterface eth;
    eth.init(); //Use DHCP
    eth.connect();

    UDPSocket socket;
    socket.bind(BROADCAST_PORT);
    socket.set_broadcasting();

    Endpoint broadcaster;
    char buffer[256];
    while (true) {
        printf("\nWait for packet...\n");
        int n = socket.receiveFrom(broadcaster, buffer, sizeof(buffer));
        buffer[n] = '\0';
        printf("Packet from \"%s\": %s\n", broadcaster.get_address(), buffer);
    }
}
#endif

int main_download_and_save_file() {
    pc.baud(115200);
    printf("Get Wav file\r\n");
    DIR  * d = NULL;
    char file_path[sizeof(FLD_PATH) + FILE_NAME_LEN];
    FATFileSystem fs("usb");
    USBHostMSD msd;

    //Audio Shield USB1 enable
    usb1en = 1;        //Outputs high level
    Thread::wait(5);
    usb1en = 0;        //Outputs low level

    audio.power(0x02); // mic off
    audio.inputVolume(0.7, 0.7);

    // try to connect a MSD device
    printf("Waiting for usb\r\n");
    while(!msd.connect()) {
        Thread::wait(500);
    }

    printf("USB connected \r\n");
    // Now that the MSD device is connected, file system is mounted.
    fs.mount(&msd);

    // Connect to the network (see mbed_app.json for the connectivity method used)
    NetworkInterface* network = easy_connect(true);
    if (!network) {
        printf("Cannot connect to the network, see serial output");
        return 1;
    }

    while (1) {
        printf("Wait for button\r\n");
        while (button.read() == 1) {
            Thread::wait(100);
        }
        printf("Button press\r\n");

        strcpy(file_path, FLD_PATH);
        strcat(file_path, "file_to_write.txt");
        fp_download = fopen(file_path, "w");
        if (fp_download == NULL) {
            printf("open %s to write failed\r\n", file_path);
        }

        // Do a GET request to wav file
        {
            // By default the body is automatically parsed and stored in a buffer, this is memory heavy.
            // To receive chunked response, pass in a callback as last parameter to the constructor.
            HttpRequest* get_req = new HttpRequest(network, HTTP_GET, "http://192.168.1.162:8080", on_body_save_file_cb);

            HttpResponse* get_res = get_req->send();
            if (!get_res) {
                printf("HttpRequest failed (error code %d)\n", get_req->get_error());
                delete get_req;
                continue;
            }

            printf("\n----- HTTP GET response -----\n");
            dump_response(get_res);

            delete get_req;
        }
        printf("Download %d bytes\r\n", totalDownload);

        fclose(fp_download);

        printf("Download done, play file\r\n");
        playWavFile("file_to_write.txt");
    }

    fs.unmount();
    Thread::wait(osWaitForever);
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

int playWavFile(char *fileName) {
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
            || (audio.format(wav_file.GetBlockSize()) == false)
            || (audio.frequency(wav_file.GetSamplingRate()) == false)) {
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
            printf("Done playing file\r\n");
            return 0;
        }
    }
}

int main_wav_player_func() {
    pc.baud(115200);
    printf("main_wav_player_func\r\n");

    FATFileSystem fs("usb");
    USBHostMSD msd;

    //Audio Shield USB1 enable
    usb1en = 1;        //Outputs high level
    Thread::wait(5);
    usb1en = 0;        //Outputs low level

    audio.power(0x02); // mic off
    audio.inputVolume(0.7, 0.7);

    // try to connect a MSD device
    printf("Waiting for usb\r\n");
    while(!msd.connect()) {
        Thread::wait(500);
    }

    printf("USB connected \r\n");
    // Now that the MSD device is connected, file system is mounted.
    fs.mount(&msd);

    playWavFile("NoiNayCoAnh.wav");
}

int main_wav_player() {
    pc.baud(115200);
    printf("main_wav_player\r\n");
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
        // HttpRequest* get_req = new HttpRequest(network, HTTP_GET, "http://esp8266-8109ea.local", on_body_cb);
        HttpRequest* get_req = new HttpRequest(network, HTTP_GET, "http://192.168.1.177", on_body_cb);

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
