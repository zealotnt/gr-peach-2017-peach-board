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

int main_http();
int main_save_file();
int main_wav_player();
int main_download_save_play();
int main_wav_player_func();
int main_broadcast_receive();
int main_test_json();
int main_test_node_devices();

int main() {
    // return main_http();
    // return main_save_file();
    // return main_wav_player();
    // return main_download_save_play();
    // return main_wav_player_func();
    // return main_broadcast_receive();
    // return main_test_json();
    return main_test_node_devices();
}

#include "cNodeManager.h"

#define DEV_1_IP            "http://192.168.1.177"
#define DEV_1_NAME          "lamp-1"

int main_test_node_devices() {
    NetworkInterface* network = grInitEth();

    NodeManager *peachDeviceManager = new NodeManager(network);
    peachDeviceManager->NodeAdd(DEV_1_IP, DEV_1_NAME);

    bool status;
    if (peachDeviceManager->NodeStatusUpdate(DEV_1_IP, &status) != 0) {
        printf("Can't update node relay status\r\n");
        return -1;
    }

    printf("Node %s: relay_status = %s\r\n", DEV_1_NAME, (status == true) ? "on" : "off");

    while(1) {
        while (isButtonRelease()) {
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
