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

/******************************************************************************/
/* INCLUSIONS                                                                 */
/******************************************************************************/
#include "grHwSetup.h"

/******************************************************************************/
/* LOCAL CONSTANT AND COMPILE SWITCH SECTION                                  */
/******************************************************************************/


/******************************************************************************/
/* LOCAL TYPE DEFINITION SECTION                                              */
/******************************************************************************/


/******************************************************************************/
/* LOCAL MACRO DEFINITION SECTION                                             */
/******************************************************************************/


/******************************************************************************/
/* MODULE'S LOCAL VARIABLE DEFINITION SECTION                                 */
/******************************************************************************/
TLV320_RBSP audio(P10_13, I2C_SDA, I2C_SCL, P4_4, P4_5, P4_7, P4_6,
                  0x80, (AUDIO_WRITE_BUFF_NUM - 1), 0);
DigitalIn  button(USER_BUTTON0);
DigitalOut usb1en(P3_8);
Serial pc(USBTX, USBRX, 115200);
FATFileSystem usbfs("usb");
USBHostMSD msd;

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
bool isButtonPressed() {
	return (button.read() == 0);
}

bool isButtonRelease() {
	return (button.read() == 1);
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
        Thread::wait(500);
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


/************************* End of File ****************************************/
