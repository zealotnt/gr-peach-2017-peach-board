#include "cProcess.hpp"
#include "mStorage.hpp"

void toggle_led(DigitalOut led)
{
    led = 1;
    wait(0.1);
    led = 0;
    wait(0.1);
}

void toggle_led_mix(DigitalOut led1, DigitalOut led2, float seconds)
{
    led1 = 1;
    led2 = 1;
    wait(seconds/2);
    led1 = 0;
    led2 = 0;
    wait(seconds/2);   
}

bool initSdCard(cStorage *myStorage)
{
    if(myStorage == NULL)
    {
        return false;
    }
    if (myStorage->isConnectSdCard() == STORG_PASS) 
    {
        if(myStorage->mountSdCard() == STORG_PASS)
        {
            return true;
        }
        else
        {
            // mount sdcard failed!
            return false;
        }
    }
    else
    {
        // no sdcard!
        return false;
    }
}

bool writeToFile(char* filePath, char* data)
{
    bool res = false;
    FILE *myFileW = fopen(filePath,"w");
    if(myFileW != NULL){
        fprintf(myFileW, "%s", data);
        fclose(myFileW);
        res = true;
    }

    return res;
}

bool writeAppendFile(char* filePath, char* data)
{
    bool res = false;
    FILE *myFileW = fopen(filePath,"a");
    if(myFileW != NULL){
        fprintf(myFileW, "%s", data);
        fclose(myFileW);
        res = true;
    }
    return res;   
}

bool writeAppendFile(FILE* f, char* data)
{
    if(fprintf(f, "%s\n",data) <= 0)
        return false;
    else
        return true;
}
bool writeAppendFile(char* filePath, unsigned char* data)
{
    bool res = false;
    FILE *myFileW = fopen(filePath,"a");
    if(myFileW != NULL){
        for(int i=0;i < sizeof(data);i++)
        {
            fprintf(myFileW, "%c", data[i]);
        }
        fclose(myFileW);
        res = true;
    }
    return res;   
}