/*
* @Author: zealotnt
* @Date:   2017-09-27 15:24:35
* @Last Modified by:   Tran Tam
* @Last Modified time: 2017-09-27 15:24:54
*/
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
            peachDeviceManager->NodeRelayOn(DEV_1_IP "/control");
        } else {
            peachDeviceManager->NodeRelayOff(DEV_1_IP "/control");
        }
    }
    return 0;
}
