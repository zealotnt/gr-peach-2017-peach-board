#include "mWav.hpp"
#include "cProcess.hpp"
#include "define.h"

cWav::cWav(char* path)
{
    if(strlen(path) < 50){
        strcpy(fileName,path);
    }

    // create file header
    writeHeader("RIFF", SIZE_OF_AUDIO_FRAME - 8,"WAVE","fmt ",16,1,2,
                44100,176400,4,16,"data",SIZE_OF_AUDIO_FRAME);
    overallSize = 0;
    dataSize = 0;
}

cWav::~cWav(void)
{
}

// bool cWav::writeAudio(void* p_data)
// {
//     if(!isInitDone)
//         return false;
//     if(writeAppendFile(fileName,(char*)p_data))
//         return true;
//     else
//         return false;
// }

void cWav::writeHeader( char* riff, int overall_size, char* wave, char* fmt_chunk_marker,
                    int length_of_fmt, int format_type, int channels, int sample_rate,
                    int byterate, int block_align, int bits_per_sample, char* data_chunk_header,int data_size
                    )
{
    FILE* f = fopen(fileName,"w");
    if(f != NULL)
    {
        if(fprintf(f,riff) < 0 )
        {
            fclose(f);
            return;
        }

        // write overallsize
        {
            unsigned char buff[4];
            buff[0] = overall_size & 0xff;
            buff[1] = (overall_size >> 8) & 0xff;
            buff[2] = (overall_size >> 16) & 0xff;
            buff[3] = (overall_size >> 24) & 0xff;

            if(fprintf(f,"%c%c%c%c",buff[0],buff[1],buff[2],buff[3]) < 0)
            {
                fclose(f);
                return;
            }
        }

        //write WAVE
        {
            if(fprintf(f,wave) < 0)
            {
                fclose(f);
                return;
            }
        }

        //write fmt chunk
        {
            if(fprintf(f,fmt_chunk_marker) < 0)
            {
                fclose(f);
                return;
            }
        }

        //write length_of_fmt
        {
            unsigned char buff[4];
            buff[0] = length_of_fmt & 0xff;
            buff[1] = (length_of_fmt >> 8) & 0xff;
            buff[2] = (length_of_fmt >> 16) & 0xff;
            buff[3] = (length_of_fmt >> 24) & 0xff;

            if(fprintf(f,"%c%c%c%c",buff[0],buff[1],buff[2],buff[3]) < 0)
            {
                fclose(f);
                return;
            }
        }

        //write format type
        {
            unsigned char buff[2];
            buff[0] = format_type & 0xff;
            buff[1] = (format_type >> 8) & 0xff;

            if(fprintf(f,"%c%c",buff[0],buff[1]) < 0)
            {
                return;
                fclose(f);
            }
        }

        //write channels
        {
            unsigned char buff[2];
            buff[0] = channels & 0xff;
            buff[1] = (channels >> 8) & 0xff;

            if(fprintf(f,"%c%c",buff[0],buff[1]) < 0)
            {
                fclose(f);
                return;
            }
        }

        //write sample rate
        {
            unsigned char buff[2];
            buff[0] = sample_rate & 0xff;
            buff[1] = (sample_rate >> 8) & 0xff;
            buff[2] = (sample_rate >> 16) & 0xff;
            buff[3] = (sample_rate >> 24) & 0xff;

            if(fprintf(f,"%c%c%c%c",buff[0],buff[1],buff[2],buff[3]) < 0)
            {
                fclose(f);
                return;
            }
        }

        // write byte_rate
        {
            unsigned char buff[2];
            buff[0] = byterate & 0xff;
            buff[1] = (byterate >> 8) & 0xff;
            buff[2] = (byterate >> 16) & 0xff;
            buff[3] = (byterate >> 24) & 0xff;

            if(fprintf(f,"%c%c%c%c",buff[0],buff[1],buff[2],buff[3]) < 0)
            {
                fclose(f);
                return;
            }
        }
        //block_align
        {
            unsigned char buff[2];
            buff[0] = block_align & 0xff;
            buff[1] = (block_align >> 8) & 0xff;

            if(fprintf(f,"%c%c",buff[0],buff[1]) < 0)
            {
                fclose(f);
                return;
            }
        }

        //write bit_per_sample
        {
            unsigned char buff[2];
            buff[0] = bits_per_sample & 0xff;
            buff[1] = (bits_per_sample >> 8) & 0xff;

            if(fprintf(f,"%c%c",buff[0],buff[1]) < 0)
            {
                fclose(f);
                return;
            }
        }

        //write data maker
        {
            if(fprintf(f,data_chunk_header) < 0)
            {
                fclose(f);
                return;
            }
        }

        //write data size 
        {
            unsigned char buff[4];
            buff[0] = data_size & 0xff;
            buff[1] = (data_size >> 8) & 0xff;
            buff[2] = (data_size >> 16) & 0xff;
            buff[3] = (data_size >> 24) & 0xff;

            if(fprintf(f,"%c%c%c%c",buff[0],buff[1],buff[2],buff[3]) < 0)
            {
                fclose(f);
                return;
            }
        }

        isInitDone = true;
        fclose(f);
        //toggle_led(LED3);
    }
    else
    {
        toggle_led(LED2);
    }
}

bool cWav::writeAudio(unsigned char* p_data)
{
    bool res = true;
    int size = sizeof(p_data);
    dataSize += size;
    if(dataSize > 8)
        overallSize = dataSize - 8;
    return true;
    if (dataSize % 512 == 0)
        toggle_led(LED2);
    FILE* f = fopen(fileName,"a");
    if(f != NULL)
    {
        for(int i = 0; i < size; i++)
        {
            fprintf(f, "%c", p_data[i]);
        }

        // write overall size
        fseek(f,4,SEEK_SET);
        {
            unsigned char buff[4];
            buff[0] = overallSize & 0xff;
            buff[1] = (overallSize >> 8) & 0xff;
            buff[2] = (overallSize >> 16) & 0xff;
            buff[3] = (overallSize >> 24) & 0xff;

            fprintf(f,"%c%c%c%c",buff[0],buff[1],buff[2],buff[3]);
        }

        // write data size
        fseek(f,40,SEEK_SET);
        {
            unsigned char buff[4];
            buff[0] = dataSize & 0xff;
            buff[1] = (dataSize >> 8) & 0xff;
            buff[2] = (dataSize >> 16) & 0xff;
            buff[3] = (dataSize >> 24) & 0xff;

            fprintf(f,"%c%c%c%c",buff[0],buff[1],buff[2],buff[3]);
        }
        fclose(f);
    }
    return res;
}

bool cWav::writeAudio(uint8_t* p_data,int size)
{
    toggle_led(LED3);
    FILE* f = fopen(fileName,"a");
    if(f != NULL)
    {
        for(int i = 0; i < size; i++)
        {
            fprintf(f, "%c", p_data[i]);
        }
        fclose(f);
        toggle_led(LED2);
        return true;
    }
    toggle_led(LED1);
    return false;
}

unsigned char* cWav::getHeader(int size_of_data)
{
    int overallSize = size_of_data - 8;
    unsigned char buffSize[4];
    unsigned char buffOverall[4];

    buffSize[0] = size_of_data & 0xff;
    buffSize[1] = (size_of_data >> 8) & 0xff;
    buffSize[2] = (size_of_data >> 16) & 0xff;
    buffSize[3] = (size_of_data >> 24) & 0xff;
    memcpy(wav_header + 40,buffSize, 4 * sizeof(unsigned char));

    buffOverall[0] = overallSize & 0xff;
    buffOverall[1] = (overallSize >> 8) & 0xff;
    buffOverall[2] = (overallSize >> 16) & 0xff;
    buffOverall[3] = (overallSize >> 24) & 0xff;
    memcpy(wav_header + 4,buffSize, 4 * sizeof(unsigned char));

    return wav_header;
}