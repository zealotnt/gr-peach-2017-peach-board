#include "mbed.h"
#include "TLV320_RBSP.h"
#include "cProcess.hpp"
#include "mStorage.hpp"
#include "mWav.hpp"
#include "define.h"
#include "rtos.h"
#include "grHwSetup.h"
#include "grUtility.h"
#include "Json.h"
#include "cJson.h"
#include "cNodeManager.h"
#include "Websocket.h"
#include <string>
#include "gr-robot-led.hpp"

typedef enum {
    LED_MAILID_DUMMY = 0,
    LED_MAIL_WW_DETECTED,
    LED_MAIL_RECORDING,
    LED_MAIL_PLAYING,
    LED_MAILID_NUM
} LED_MAIL_ID;

typedef struct {
    LED_MAIL_ID     mail_id;
} led_mail_t;
static Mail<led_mail_t, 2> ledMailBox;

DigitalInOut ledR(LED_RED, PIN_OUTPUT, OpenDrain, 0);
DigitalInOut ledG(LED_GREEN, PIN_OUTPUT, OpenDrain, 0);
DigitalInOut ledB(LED_BLUE, PIN_OUTPUT, OpenDrain, 0);

#define ledOffAll()   ledOff(ledR); ledOff(ledG); ledOff(ledB);

static void ledStateToggle(DigitalInOut led)
{
    int value = led.read();
    led.write(value ^ 1);
}

static void ledOff(DigitalInOut led)
{
    led.write(0);
}

static void ledSet(ledColor_t color)
{
    ledR.write(color & 0x01);
    ledG.write(color & 0x02);
    ledB.write(color & 0x04);
}

void ledPlayingSound()
{
    // we do rainbow here
    ledColor_t colorList[] = {
            COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_PINK, COLOR_CYAN, COLOR_YELLOW, COLOR_WHITE
    };

    for (int i = 0; i < ARRAY_LEN(colorList); i++) {
        ledSet(colorList[i]);
        Thread::wait(250);
    }
}

void ledWakewordDetected()
{
    for (int i = 0; i < 5; i++) {
        ledSet(COLOR_RED);
        Thread::wait(50);
        ledSet(COLOR_OFF);
        Thread::wait(50);
    }
}

bool ledEnable = false;
bool isLedEnable()
{
    return ledEnable;
}

void ledCancelPattern()
{
    ledEnable = false;
}

void grRobot_led_blinker_task(void const*) {

    while (1) {

    }
}
