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

/* Convert buffer from camera to Mat */
Mat _m_cvtRgb5652Rgb(uint8_t *_src, int _w, int _h, int _bytes_per_pixel)
{
    Mat _res(_h,_w,CV_8UC3,Scalar(0,0,0));
    const uint16_t lc_RedMask = 0xF800;
    const uint16_t lc_GreenMask = 0x07E0;
    const uint16_t lc_BlueMask =  0x001F;
    int idx = 0;
    for(int i = 0; i < _w*_h*_bytes_per_pixel; i+=2, idx++)
    {
        uint16_t lo_PixelVal = (_src[ i ]) | (_src[ i + 1 ] << 8);
        uint8_t lo_RedVal = (lo_PixelVal & lc_RedMask) >> 11;
        uint8_t lo_GreenVal = (lo_PixelVal & lc_GreenMask) >> 5;
        uint8_t lo_BlueVal = (lo_PixelVal & lc_BlueMask);

        lo_RedVal <<= 3;
        lo_GreenVal <<= 2;
        lo_BlueVal <<= 3;
        _res.at<Vec3b>(idx)[0] = lo_BlueVal;
        _res.at<Vec3b>(idx)[1] = lo_GreenVal;
        _res.at<Vec3b>(idx)[2] = lo_RedVal;
    }
    return _res;
}
/* Return value of char */
int returnValue(char ch)
{
    int value;
    if(ch >= '0' && ch <= '9')
    {
        value = ch - '0';
    }
    else
    {
        value = ch - 'a' + 10;
    }
    return value;
}

/* Read info in file as unsigned char */
int readHex(FILE* f, int& value, unsigned int& pos) // read "0x0a, "
{
    fseek(f,pos,SEEK_SET);
    char ch;
    value = -1;
    if((ch = fgetc(f)) == EOF)//0
        return -1;
    else if ((ch = fgetc(f)) == EOF)//x
        return -1;
    else if((ch = fgetc(f)) == EOF)//0
        return -1;
    else
    {
        value = returnValue(ch);

        if((ch = fgetc(f)) == EOF)//a
        {
            value = -1;
            return -1;
        }
        else
        {
            value = value*16 + returnValue(ch);
        }

        if((ch = fgetc(f)) == EOF)//,
            return -1;
        if((ch = fgetc(f)) == EOF)//' '
            return -1;
        else
        {
            pos += 6;
            return 0;
        }
    }
}

/**
 * @brief processor::cvtMat2Text for face recognition
 * @param img
 * @param filename
 * @param isAppend
 * @param addHeader  True if user want to add header
 * Header: | 1 byte channel | 1 byte label | 2 bytes width | 2 bytes height |
 * 2 bytes: Lower byte first
 */
void cvtMat2Text(Mat img,char* filename,int label, bool isAppend,bool addHeader)
{
    FILE *myFileW = fopen(filename,"a");
    char header[40];
    int w, h;
    if(myFileW != NULL)
    {
        printf("Save Mat To Db!\n");
        if(isAppend)
            fseek(myFileW,0,SEEK_END);
        if(addHeader) // add header
        {
            w = img.cols;
            h = img.rows;
            sprintf(header,"0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, ",
                    ((img.channels())&0xff), (label&0xff), (w & 0xff), (w>>8)&0xff,
                    (h&0xff),(h>>8)&0xff);
            fprintf(myFileW,"%s",header);
        }

        for(int i = 0; i < h; i++)
        {
            for(int j = 0; j < w;j++)
            {
                fprintf(myFileW,"0x%02x, ",((int)img.at<uchar>(i,j))&0xff);
            }
        }

        printf("Save Mat To Db Done!\n");
        fclose(myFileW);
    }
    else
    {
        printf("cvtMat2Text:: Cannot open file!\n");
    }
}

/**
 * @brief processor::txt2Label
 * @param filename
 * @return
 */
vector<infor_label_st> txt2Label(char *filename, int &maxLabel)
{
    vector<infor_label_st> result;
    FILE *myFileR = fopen(filename,"r");
    int w, h, label, channel, value_1, value_2;
    Mat img;
    unsigned int pos = 0;
    bool exit = false;
    maxLabel = -1;
    printf("READING DB!!!\n");
    if(myFileR != NULL){
        while(!exit)
        {
            if(readHex(myFileR,channel,pos) != 0)
                break;
            if(readHex(myFileR,label,pos) != 0)
                break;

            if(readHex(myFileR,value_1,pos) != 0)
                break;

            if(readHex(myFileR,value_2,pos) != 0)
                break;
            w = (value_2 << 8) | value_1;

            if(readHex(myFileR,value_1,pos) != 0)
                break;

            if(readHex(myFileR,value_2,pos) != 0)
                break;
            h = (value_2 << 8) | value_1;

            printf("Mat %d, %d\n", h,w);

            if(h > 70 || w > 70)
            {
                fclose(myFileR);
                return result;
            }
            // Current for gray scale image

            img = Mat(h,w,CV_8UC1);

            exit = false;
            for(int i = 0; i < h; i++)
            {
                for(int j = 0; j < w; j++)
                {
                    int tmp;
                    if (readHex(myFileR,tmp,pos) == 0)
                    {
                        img.at<uchar>(i,j) = (unsigned char)tmp;
                    }
                    else
                    {
                        exit = true;
                        break;
                    }
                }

                if ( exit == true)
                    break;
            }

            if( exit == false)
            {
                result.push_back(infor_label_st(img,label));

                if(label > maxLabel)
                {
                    maxLabel = label;
                }
            }
        }

        fclose(myFileR);
    }
    else
    {
        printf("txt2Label:: Failed to open file label!\n");
    }

    printf("READING DB DONE!!!\n");
    return result;
}

void saveFaceDb(face_database_st facesDb, int label, char* filename)
{
    for(int i = 0; i < (int)facesDb.database_label.size();i++)
    {
        if(facesDb.database_label[i] == label)
        {
            // Save faces
            cvtMat2Text(facesDb.database_image[i],filename,label);
        }
    }
}
