/*
* @Author: zealotnt
* @Date:   2017-09-27 15:24:35
* @Last Modified by:   Tran Tam
* @Last Modified time: 2017-09-29 18:35:37
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


int main_test_node_devices() {
    NetworkInterface* network = grInitEth();

    NodeManager *peachDeviceManager = new NodeManager(network);
    peachDeviceManager->NodeAdd(DEV_1_IP, DEV_1_NAME);
    peachDeviceManager->NodeAdd(DEV_2_IP, DEV_2_NAME);

    bool status1;
    bool status2;

    if (peachDeviceManager->NodeStatusUpdate(DEV_1_IP, &status1) != 0) {
        printf("Can't update node relay status\r\n");
        return -1;
    }

    if (peachDeviceManager->NodeStatusUpdate(DEV_2_IP, &status2) != 0) {
        printf("Can't update node relay status\r\n");
        return -1;
    }

    while(1) {
        waitShortPress();

        if (peachDeviceManager->NodeStatusUpdate(DEV_1_IP, &status1) != 0) {
            printf("Can't update node relay status\r\n");
            return -1;
        }

        if (peachDeviceManager->NodeStatusUpdate(DEV_2_IP, &status2) != 0) {
            printf("Can't update node relay status\r\n");
            return -1;
        }
        printf("Node %s: relay_status = %s\r\n", DEV_1_NAME, (status1 == true) ? "on" : "off");
        printf("Node %s: relay_status = %s\r\n", DEV_2_NAME, (status2 == true) ? "on" : "off");

        peachDeviceManager->PostNodeStatus(ADDRESS_HTTP_SERVER);

        // if (peachDeviceManager->NodeRelayStatus(DEV_1_IP) == false) {
        //     peachDeviceManager->NodeRelayOn(DEV_1_IP);
        // } else {
        //     peachDeviceManager->NodeRelayOff(DEV_1_IP);
        // }

        // if (peachDeviceManager->NodeRelayStatus(DEV_2_IP) == false) {
        //     peachDeviceManager->NodeRelayOn(DEV_2_IP);
        // } else {
        //     peachDeviceManager->NodeRelayOff(DEV_2_IP);
        // }
    }
    return 0;
}
