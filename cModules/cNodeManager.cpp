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
#include "cNodeManager.h"
#include "grUtility.h"

#include "Json.h"

#include "http_request.h"
#include "http_parser.h"
#include "http_response.h"
#include "http_request_builder.h"
#include "http_response_parser.h"
#include "http_parsed_url.h"

/******************************************************************************/
/* LOCAL CONSTANT AND COMPILE SWITCH SECTION                                  */
/******************************************************************************/


/******************************************************************************/
/* LOCAL TYPE DEFINITION SECTION                                              */
/******************************************************************************/


/******************************************************************************/
/* LOCAL MACRO DEFINITION SECTION                                             */
/******************************************************************************/
#define logError(...)       printf(__VA_ARGS__); printf("\r\n");
#define logInfo(...)        printf(__VA_ARGS__); printf("\r\n");

#define RELAY_STATUS_KEY_STR       "relay_status"

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
NodeDevice::NodeDevice(std::string ip, std::string name, int id)
    : ip(ip), name(name), id(id)
{

}

bool NodeDevice::RelayStatusGet()
{
    return this->relayStatus;
}

void NodeDevice::RelayStatusSet(bool status)
{
    this->relayStatus = status;
}

NodeManager::NodeManager(NetworkInterface *network)
{
    this->network = network;
    this->node_count = 0;
}

uint32_t NodeManager::NodeCount()
{
    return this->node_count;
}

int NodeManager::NodeAdd(std::string ip, std::string name)
{
    if (this->node_count >= MAX_NODE_DEVICE) {
        return -1;
    }

    NodeDevice *device = new NodeDevice(ip, name, this->node_count);
    this->node_list[node_count] = device;
    this->node_count ++;

    return 0;
}

int NodeManager::NodeStatusUpdate(std::string ip, bool *status)
{
    int ret = -1;
    int nodeId = getNodeId(ip);
    if (nodeId == -1) {
        ret = -1;
        goto exit;
    }

    {
        // Do a GET request to node device EP
        HttpRequest* get_req = new HttpRequest(network, HTTP_GET, ip.c_str());

        HttpResponse* get_res = get_req->send();
        if (!get_res) {
            printf("HttpRequest failed (error code %d)\n", get_req->get_error());
            delete get_req;
            ret = -2;
            goto exit;
        }

        dump_response(get_res);

        if (get_res->get_status_code() != 200) {
            ret = -3;
            goto exit;
        }

        char *jsonSource = new char[get_res->get_body_as_string().length() + 1];
        strcpy(jsonSource, get_res->get_body_as_string().c_str());

        Json json(jsonSource, strlen(jsonSource), 100);

        if (!json.isValidJson()) {
            logError ("Invalid JSON: %s", jsonSource);
            ret = -1;
            delete [] jsonSource;
            delete get_req;
            goto exit;
        }

        if (json.type (0) != JSMN_OBJECT) {
            logError ("Invalid JSON.  ROOT element is not Object: %s", jsonSource);
            ret = -1;
            delete [] jsonSource;
            delete get_req;
            goto exit;
        }

        // Let's get the value of key "RELAY_STATUS_KEY_STR" in ROOT object, and copy into
        char relayValue[32];

        // ROOT object should have '0' tokenIndex, and -1 parentIndex
        int relayKeyIndex = json.findKeyIndexIn (RELAY_STATUS_KEY_STR, 0);
        if (relayKeyIndex == -1) {
            logError ("\"%s\" does not exist ... do something!!", RELAY_STATUS_KEY_STR);
            ret = -1;
            delete [] jsonSource;
            delete get_req;
            goto exit;
        }

        // Find the first child index of key-node "city"
        int relayValueIndex = json.findChildIndexOf(relayKeyIndex, -1);
        if (relayValueIndex > 0)
        {
            const char * valueStart  = json.tokenAddress(relayValueIndex);
            int          valueLength = json.tokenLength(relayValueIndex);
            strncpy (relayValue, valueStart, valueLength);
            relayValue[valueLength] = 0; // NULL-terminate the string

            if (strcmp(relayValue, "on") == 0) {
                *status = true;
            } else if (strcmp(relayValue, "off") == 0) {
                *status = false;
            } else {
                logError ("Invalid value of relay_staus: %s", relayValue);
                ret = -1;
                delete [] jsonSource;
                delete get_req;
                goto exit;
            }
        }

        delete get_req;
    }

    this->node_list[nodeId]->RelayStatusSet(*status);
    ret = 0;
exit:
    return ret;
}

int NodeManager::NodeRelayControl(std::string ip, bool status)
{
    if (status) {
        this->NodeRelayOn(ip);
    } else {
        this->NodeRelayOff(ip);
    }
}

int NodeManager::NodeRelayOn(std::string ip)
{
    int nodeId = getNodeId(ip);
    if (nodeId == -1) {
        return -1;
    }
    this->node_list[nodeId]->RelayStatusSet(true);
}

int NodeManager::NodeRelayOff(std::string ip)
{
    int nodeId = getNodeId(ip);
    if (nodeId == -1) {
        return -1;
    }
    this->node_list[nodeId]->RelayStatusSet(false);
}

bool NodeManager::NodeRelayStatus(std::string ip)
{
    int nodeId = getNodeId(ip);
    if (nodeId == -1) {
        return -1;
    }
    return this->node_list[nodeId]->RelayStatusGet();
}

int NodeManager::getNodeId(std::string ip)
{
    return 0;
}

/************************* End of File ****************************************/
