/*
 * gr-robot-led.hpp
 *
 *  Created on: Nov 18, 2017
 *      Author: zealot
 */

#ifndef SOURCE_GR_ROBOT_GR_ROBOT_LED_HPP_
#define SOURCE_GR_ROBOT_GR_ROBOT_LED_HPP_

typedef enum
{
    COLOR_OFF = 0,
    COLOR_RED = 0x01,
    COLOR_GREEN = 0x02,
    COLOR_BLUE = 0x04,
    COLOR_PINK = 0x05,
    COLOR_CYAN = 0x06,
    COLOR_YELLOW = 0x03,
    COLOR_WHITE = 0x07,
} ledColor_t;

void grRobot_led_blinker_task(void const*);
void grRobot_SetLedIdle();
void grRobot_SetLedPreparePlaying();
void grRobot_SetLedPlaying();
void grRobot_SetLedRecording();
void grRobot_SetLedWwDetected();


#endif /* SOURCE_GR_ROBOT_GR_ROBOT_LED_HPP_ */
