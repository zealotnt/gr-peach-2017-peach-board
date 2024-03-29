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

#ifndef _C_NODE_MANAGER_H_
#define _C_NODE_MANAGER_H_

/*****************************************************************************/
/* INCLUSIONS                                                                */
/*****************************************************************************/
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include "mbed.h"

/*****************************************************************************/
/* DEFINITION OF COMPILE SWITCH                                              */
/*****************************************************************************/


/*****************************************************************************/
/* DEFINITION OF CONSTANTS                                                   */
/*****************************************************************************/
#define MAX_NODE_DEVICE				2

#define DEV_1_IP            "http://192.168.1.177"
#define DEV_1_NAME          "light"

#define DEV_2_IP            "http://192.168.1.178"
#define DEV_2_NAME          "fan"

/*****************************************************************************/
/* DEFINITION OF TYPES                                                       */
/*****************************************************************************/


/*****************************************************************************/
/* DEFINITION OF MACROS                                                      */
/*****************************************************************************/


/*****************************************************************************/
/* DECLARATION OF VARIABLES (Only external global variables)                 */
/*****************************************************************************/


/*****************************************************************************/
/* DECLARATION OF GLOBALES FUNCTIONS (APIs, Callbacks & MainFunctions)       */
/*****************************************************************************/
class NodeDevice {
public:
	NodeDevice(std::string ip, std::string name, int id);
	int Id();
	std::string Name();
	std::string Ip();
	bool RelayStatusGet();
    char *RelayStatusGetChar();
	void RelayStatusSet(bool status);

private:
	std::string ip;
	std::string name;
	int id;
	bool relayStatus;
};

class NodeManager {
public:
	NodeManager(NetworkInterface *network);
	uint32_t NodeCount();
	int NodeAdd(std::string ip, std::string name);
	int NodeStatusUpdate(std::string ip, bool *status);
	int NodeRelayControl(std::string ip, bool status);
	int NodeRelayOn(std::string ip);
	int NodeRelayOff(std::string ip);
	bool NodeRelayStatus(std::string ip);
	int getNodeId(std::string ip);
	int getNodeIdByName(const string name);
	string getIpDevice(int id);

    int PostNodeStatus(std::string server_add);

private:
	uint32_t node_count;
	NodeDevice *node_list[MAX_NODE_DEVICE];
	NetworkInterface *network;
};

#endif /* _C_NODE_MANAGER_H_ */
