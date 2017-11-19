/*==============================================================================
**
**                      Proprietary - Copyright (C) 2017
**------------------------------------------------------------------------------
** Supported MCUs      :
** Supported Compilers : GCC
**------------------------------------------------------------------------------
** File name         : template.h
**
** Module name       : template
**
**
** Summary:
**
**= History ====================================================================
** @date
** @author	zealot
** - Development
==============================================================================*/

#ifndef GR_UTILITY_H_
#define GR_UTILITY_H_

/*****************************************************************************/
/* INCLUSIONS                                                                */
/*****************************************************************************/
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include "mbed.h"
#include "mbed_stats.h"

#include "http_request.h"
#include "http_response.h"

/*****************************************************************************/
/* DEFINITION OF COMPILE SWITCH                                              */
/*****************************************************************************/


/*****************************************************************************/
/* DEFINITION OF CONSTANTS                                                   */
/*****************************************************************************/
#define HTTP_PROTO              "http://"
#define WS_PROTO                "ws://"
#define HTTP_PORT               ":8080"
#define WS_PORT                 ":9003"
#define WS_PATH                 "/"
#define IP_SERVER               "10.42.0.1"
#define ADDRESS_HTTP_SERVER     HTTP_PROTO IP_SERVER HTTP_PORT
#define ADDRESS_WS_SERVER       WS_PROTO IP_SERVER WS_PORT WS_PATH

/*****************************************************************************/
/* DEFINITION OF TYPES                                                       */
/*****************************************************************************/


/*****************************************************************************/
/* DEFINITION OF MACROS                                                      */
/*****************************************************************************/
#define ARRAY_LEN(arr) ((int) (sizeof (arr) / sizeof (arr)[0]))

/*****************************************************************************/
/* DECLARATION OF VARIABLES (Only external global variables)                 */
/*****************************************************************************/


/*****************************************************************************/
/* DECLARATION OF GLOBALES FUNCTIONS (APIs, Callbacks & MainFunctions)       */
/*****************************************************************************/
void DumpHex(const void* data, size_t size);
void dump_response(HttpResponse* res);
void waitShortPress();

HttpRequest *grHttpPost(NetworkInterface *network, char *address, char *body);
HttpRequest *grHttpPostJson(NetworkInterface *network, char *address, char *body);
HttpRequest *grHttpGet(NetworkInterface* network, char *end_point);
bool grHttpGet(NetworkInterface* network, char *end_point, char *body, uint32_t buffSize);
void printHeadStat();

int grStartUpload(NetworkInterface* network);
int grEndUpload(NetworkInterface* network);
void grEndUploadWithBody(NetworkInterface* network, char* body);
int grUploadFile(NetworkInterface* network, uint8_t *buff, uint32_t buffLen);
int grReadFile(char *filePath, uint8_t **fileVal, uint32_t *fileLen);

#endif /* GR_UTILITY_H_ */
