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
	NodeDevice(std::string ip, std::string name, uint32_t id);
	uint32_t Id();
	std::string Name();
	std::string Ip();
	bool RelayStatusGet();
	void RelayStatusSet(bool status);

private:
	std::string ip;
	std::string name;
	uint32_t id;
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

private:
	uint32_t node_count;
	NodeDevice *node_list;
	NetworkInterface *network;
};

#endif /* _C_NODE_MANAGER_H_ */
