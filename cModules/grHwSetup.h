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

#ifndef GR_HW_SETUP_H_
#define GR_HW_SETUP_H_

/*****************************************************************************/
/* INCLUSIONS                                                                */
/*****************************************************************************/
#include "TLV320_RBSP.h"
#include "FATFileSystem.h"
#include "USBHostMSD.h"
#include "usb_host_setting.h"

/*****************************************************************************/
/* DEFINITION OF COMPILE SWITCH                                              */
/*****************************************************************************/


/*****************************************************************************/
/* DEFINITION OF CONSTANTS                                                   */
/*****************************************************************************/
#define AUDIO_WRITE_BUFF_SIZE  (4096)
#define AUDIO_WRITE_BUFF_NUM   (9)
#define FILE_NAME_LEN          (64)
#define TEXT_SIZE              (64 + 1) //null-terminated
#define FLD_PATH               "/usb/"
#define USB_HOST_CH     		1

/*****************************************************************************/
/* DEFINITION OF TYPES                                                       */
/*****************************************************************************/


/*****************************************************************************/
/* DEFINITION OF MACROS                                                      */
/*****************************************************************************/


/*****************************************************************************/
/* DECLARATION OF VARIABLES (Only external global variables)                 */
/*****************************************************************************/
extern uint8_t title_buf[TEXT_SIZE];
extern uint8_t artist_buf[TEXT_SIZE];
extern uint8_t album_buf[TEXT_SIZE];
extern uint8_t audio_write_buff[AUDIO_WRITE_BUFF_NUM][AUDIO_WRITE_BUFF_SIZE]
                __attribute((section("NC_BSS"),aligned(4)));

/*****************************************************************************/
/* DECLARATION OF GLOBALES FUNCTIONS (APIs, Callbacks & MainFunctions)       */
/*****************************************************************************/
bool isButtonPressed();
bool isButtonRelease();

NetworkInterface *grInitEth();
void grEnableUSB1();
void grEnableAudio();
void grSetupUsb();
void grUnmoutUsb();
TLV320_RBSP *grAudio();
int grPlayWavFile(char *fileName);
int grDownloadFile(NetworkInterface* network, char *fileName, char *address);

#endif /* GR_HW_SETUP_H_ */
