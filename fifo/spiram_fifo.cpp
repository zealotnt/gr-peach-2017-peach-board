/******************************************************************************
 * Copyright 2013-2015 Espressif Systems
 *
 * FileName: user_main.c
 *
 * Description: Routines to use a SPI RAM chip as a big FIFO buffer. Multi-
 * thread-aware: the reading and writing can happen in different threads and
 * will block if the fifo is empty and full, respectively.
 *
 * Modification history:
 *     2015/06/02, v1.0 File created.
*******************************************************************************/
#include "string.h"
#include <stdio.h>
#include "rtos.h"
#include "mbed.h"

#include "spiram_fifo.h"
#include "spiram.h"

#define SPIREADSIZE 64
#define FAKE_SPI_BUFF

static int fifoRpos;
static int fifoWpos;
static int fifoFill;
Semaphore mux(1);
Semaphore semCanWrite(1);
Semaphore semCanRead(1);
static long fifoOvfCnt, fifoUdrCnt;

//Low watermark where we restart the reader thread.
#define FIFO_LOWMARK (112*1024)

#ifdef FAKE_SPI_BUFF
//Re-define a bunch of things so we use the internal buffer
#undef SPIRAMSIZE
//allocate enough for about one mp3 frame
//#define SPIRAMSIZE 1850
#define SPIRAMSIZE 32000
static char fakespiram[SPIRAMSIZE];
#define spiRamInit() while(0)
#define spiRamTest() 1
#define spiRamWrite(pos, buf, n) memcpy(&fakespiram[pos], buf, n)
#define spiRamRead(pos, buf, n) memcpy(buf, &fakespiram[pos], n)
#endif

//Initialize the FIFO
int spiRamFifoInit() {
    fifoRpos=0;
    fifoWpos=0;
    fifoFill=0;
    fifoOvfCnt=0;
    fifoUdrCnt=0;
    spiRamInit();
    return (spiRamTest());
}

void spiRamFifoReset() {
    mux.wait();
    fifoRpos=0;
    fifoWpos=0;
    fifoFill=0;
    fifoOvfCnt=0;
    fifoUdrCnt=0;
    semCanWrite.release();
    semCanRead.wait();
    mux.release();
}

//Read bytes from the FIFO
void spiRamFifoRead(char *buff, int len) {
    int n;
    while (len > 0) {
        n = len;
        if (n>SPIREADSIZE) n=SPIREADSIZE;           //don't read more than SPIREADSIZE
        if (n>(SPIRAMSIZE-fifoRpos)) n = SPIRAMSIZE - fifoRpos; //don't read past end of buffer
        mux.wait();
        if (fifoFill < n) {
//          printf("FIFO empty.\n");
            //Drat, not enough data in FIFO. Wait till there's some written and try again.
            fifoUdrCnt++;
            mux.release();
            if (fifoFill < FIFO_LOWMARK) semCanRead.wait();
        } else {
            //Read the data.
            spiRamRead(fifoRpos, buff, n);
            buff += n;
            len -= n;
            fifoFill -= n;
            fifoRpos += n;
            if (fifoRpos>=SPIRAMSIZE) fifoRpos=0;
            mux.release();
            semCanWrite.release(); //Indicate writer thread there's some free room in the fifo
        }
    }
}

//Write bytes to the FIFO
void spiRamFifoWrite(const char *buff, int buffLen) {
    int n;
    while (buffLen > 0) {
        n = buffLen;

        // don't read more than SPIREADSIZE
        if (n > SPIREADSIZE) n = SPIREADSIZE;

        // don't read past end of buffer
        if (n > (SPIRAMSIZE - fifoWpos)) {
            n = SPIRAMSIZE - fifoWpos;
        }

        mux.wait();
        if ((SPIRAMSIZE - fifoFill) < n) {
            // printf("FIFO full.\n");
            // Drat, not enough free room in FIFO. Wait till there's some read and try again.
            fifoOvfCnt++;
            mux.release();
            semCanWrite.wait();
            // Thread::yield();
        } else {
            // Write the data.
            spiRamWrite(fifoWpos, buff, n);
            buff += n;
            buffLen -= n;
            fifoFill += n;
            fifoWpos += n;
            if (fifoWpos >= SPIRAMSIZE) fifoWpos = 0;
            mux.release();
            semCanRead.release(); // Tell reader thread there's some data in the fifo.
        }
    }
}

//Get amount of bytes in use
int spiRamFifoFill() {
    int ret;
    mux.wait();
    ret=fifoFill;
    mux.release();
    return ret;
}

int spiRamFifoFree() {
    return (SPIRAMSIZE-spiRamFifoFill());
}

int spiRamFifoLen() {
    return SPIRAMSIZE;
}

long spiRamGetOverrunCt() {
    long ret;
    mux.wait();
    ret=fifoOvfCnt;
    mux.release();
    return ret;
}

long spiRamGetUnderrunCt() {
    long ret;
    mux.wait();
    ret=fifoUdrCnt;
    mux.release();
    return ret;
}

