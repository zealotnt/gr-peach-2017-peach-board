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

#define LED_MAIL_CHECK_INTERVAL     10
#define LED_MAIL_SIZE               3

typedef enum {
    LED_MAILID_DUMMY = 0,
    LED_MAIL_WW_DETECTED,
    LED_MAIL_RECORDING,
    LED_MAIL_PLAYING,
    LED_MAIL_IDLE,
    LED_MAILID_NUM
} LED_MAIL_ID;

typedef struct {
    LED_MAIL_ID     mail_id;
} led_mail_t;
static Mail<led_mail_t, LED_MAIL_SIZE> ledMailBox;

typedef void (*pLedControlHandle_t)();

DigitalInOut ledR(LED_RED, PIN_OUTPUT, OpenDrain, 0);
DigitalInOut ledG(LED_GREEN, PIN_OUTPUT, OpenDrain, 0);
DigitalInOut ledB(LED_BLUE, PIN_OUTPUT, OpenDrain, 0);

#define ledOffAll()                 ledOff(ledR); ledOff(ledG); ledOff(ledB);

static bool send_led_mail(const LED_MAIL_ID mail_id)
{
    bool            ret = false;
    osStatus        stat;
    led_mail_t      * const p_mail = ledMailBox.alloc();

    if (p_mail != NULL) {
        p_mail->mail_id = mail_id;
        stat = ledMailBox.put(p_mail);
        if (stat == osOK) {
            ret = true;
        } else {
            (void) ledMailBox.free(p_mail);
        }
    }
    return ret;
}

static bool ledRecvMail(LED_MAIL_ID * const p_mail_id)
{
    bool            ret = false;
    osEvent         evt;
    led_mail_t      *p_mail;

    if (p_mail_id != NULL) {
        evt = ledMailBox.get(LED_MAIL_CHECK_INTERVAL);
        if (evt.status == osEventMail) {
            p_mail = (led_mail_t *)evt.value.p;
            if (p_mail != NULL) {
                *p_mail_id = p_mail->mail_id;
                ret = true;
            }
            (void) ledMailBox.free(p_mail);
        }
    }
    return ret;
}

static void ledSet(ledColor_t color)
{
    ledR.write(color & 0x01);
    ledG.write(color & 0x02);
    ledB.write(color & 0x04);
}

void ledIdle()
{
    ledSet(COLOR_OFF);
}

void ledPlayingSound()
{
    // we do rainbow here
    ledColor_t colorList[] = {
            COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_PINK, COLOR_CYAN, COLOR_YELLOW, COLOR_WHITE
    };

    for (int i = 0; i < ARRAY_LEN(colorList); i++) {
        ledSet(colorList[i]);
        Thread::wait(50);
    }
}

void ledRecording() {
    ledSet(COLOR_GREEN);
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

void grRobot_SetLedIdle() {
    send_led_mail(LED_MAIL_IDLE);
}

void grRobot_SetLedPlaying() {
    send_led_mail(LED_MAIL_PLAYING);
}

void grRobot_SetLedRecording() {
    send_led_mail(LED_MAIL_RECORDING);
}

void grRobot_SetLedWwDetected() {
    send_led_mail(LED_MAIL_WW_DETECTED);
}

void grRobot_led_blinker_task(void const*) {

    LED_MAIL_ID ledMailId = LED_MAIL_IDLE;
    pLedControlHandle_t ledFunc = ledIdle;
    while (1) {
        if (ledRecvMail(&ledMailId) == true) {
            switch (ledMailId)
            {
                case LED_MAIL_IDLE:
                    ledFunc = ledIdle;
                    break;
                case LED_MAIL_PLAYING:
                    ledFunc = ledPlayingSound;
                    break;
                case LED_MAIL_RECORDING:
                    ledFunc = ledRecording;
                    break;
                case LED_MAIL_WW_DETECTED:
                    ledFunc = ledWakewordDetected;
                    break;
                default:
                    break;
            }
        }

        // just do last state
        ledFunc();
    }
}
