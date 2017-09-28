#ifndef C_MWAV_HPP
#define C_MWAV_HPP
#include "mbed.h"
struct HEADER {
    unsigned char riff[4];                      // RIFF string
    unsigned int overall_size   ;               // overall size of file in bytes
    unsigned char wave[4];                      // WAVE string
    unsigned char fmt_chunk_marker[4];          // fmt string with trailing null char
    unsigned int length_of_fmt;                 // length of the format data
    unsigned int format_type;                   // format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
    unsigned int channels;                      // no.of channels
    unsigned int sample_rate;                   // sampling rate (blocks per second)
    unsigned int byterate;                      // SampleRate * NumChannels * BitsPerSample/8
    unsigned int block_align;                   // NumChannels * BitsPerSample/8
    unsigned int bits_per_sample;               // bits per sample, 8- 8bits, 16- 16 bits etc
    unsigned char data_chunk_header [4];        // DATA string or FLLR string
    unsigned int data_size;                     // NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
};

class cWav
{
private:
    char fileName[50];
    int overallSize;
    int dataSize; 
    bool isInitDone;
    void writeHeader( char* riff, int overall_size, char* wave, char* fmt_chunk_marker,
                    int length_of_fmt, int format_type, int channels, int sample_rate,
                    int byterate, int block_align, int bits_per_sample, char* data_chunk_header,int data_size = 0
                    );
public:
    /* Constructor */
    cWav(char *file_name);
    
    /* Destructor */
    ~cWav(void);
    
    /*Write data to file*/
    bool writeAudio(unsigned char* p_data);
    bool writeAudio(uint8_t* p_data,int size);
    int getDataSize(){return dataSize;}
};

#endif