#ifndef C_PROCESS_HPP
#define C_PROCESS_HPP
#include "mStorage.hpp"
#include "define_opencv_task.hpp"

void toggle_led(DigitalOut led);
void toggle_led_mix(DigitalOut led1, DigitalOut led2, float seconds);
bool initSdCard(cStorage *myStorage);
bool writeToFile(char* filePath, char* data);
bool writeAppendFile(char* filePath, char* data);
bool writeAppendFile(char* filePath, unsigned char* data);
bool writeAppendFile(FILE* f, char* data);

// Add new for face recognition
Mat _m_cvtRgb5652Rgb(uint8_t *_src, int _w, int _h, int _bytes_per_pixel);
int returnValue(char ch);
int readHex(FILE* f, int& value, unsigned int& pos);
void cvtMat2Text(Mat img, char *filename, int label = 0, bool isAppend = true, bool addHeader = true);
vector<infor_label_st> txt2Label(char *filename, int& maxLabel);
void saveFaceDb(face_database_st facesDb, int label, char* filename);

#endif //C_PROCESS_HPP