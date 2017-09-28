#ifndef C_PROCESS_HPP
#define C_PROCESS_HPP
#include "mStorage.hpp"
void toggle_led(DigitalOut led);
void toggle_led_mix(DigitalOut led1, DigitalOut led2, float seconds);
bool initSdCard(cStorage *myStorage);
bool writeToFile(char* filePath, char* data);
bool writeAppendFile(char* filePath, char* data);
bool writeAppendFile(char* filePath, unsigned char* data);
bool writeAppendFile(FILE* f, char* data);

#endif //C_PROCESS_HPP