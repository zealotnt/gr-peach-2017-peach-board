#include "mbed.h"
#include "DisplayBace.h"
#include "rtos.h"
#include "define.h"
#include "cProcess.hpp"
#include "mStorage.hpp"

/* Opencv */
#include "core.hpp"
#include "imgproc.hpp"
#include "objdetect.hpp"
#include "face.hpp"

#include "grUtility.h"
#include "grHwSetup.h"
#include "define_opencv_task.hpp"

using namespace cv;
using namespace face;

#define DBG_INFO(...)           printf("FACE::"); printf(__VA_ARGS__)
#define FACE_DB_FILE            "/usb/faces.data"

/* Hardware */
static DisplayBase Display;

/* global variables*/
static int write_buff_num = 0;
static int read_buff_num = 0;
static bool graphics_init_end = false;
static bool g_is_processing = false;
static bool g_add_person = false;
static int g_label_of_person;
/* Threads */
static Thread *    p_VideoCaptureTask = NULL;
static Thread *    p_UserButtonTask = NULL;

/* Semaphore and mutex */
rtos::Mutex mMutexProcess;
rtos::Mutex mMutexAddperson;
rtos::Mutex mWhoPerson;
static Semaphore semProcessThread(0);


/* APIs*/
void addPersonStart()
{
    mMutexAddperson.lock();
    g_add_person = true;
    mMutexAddperson.unlock();
}

int whoIsPersion(int time_ms)
{
    Timer t;

    mWhoPerson.lock();
    g_label_of_person = -1;
    mWhoPerson.unlock();

    while(t.read_ms() < time_ms)
    {
        // Mutex here
        mWhoPerson.lock();
        if(g_label_of_person != -1)
        {
            mWhoPerson.unlock();
            return g_label_of_person;
        }
        mWhoPerson.unlock();
    }
    return g_label_of_person;
}
bool isAddPersonDone()
{
    return !g_add_person;
}
/* End APIs*/

/* Function callback for camera init */
static void IntCallbackFunc_Vfield(DisplayBase::int_type_t int_type) {
    /* Interrupt callback function */
#if VIDEO_INPUT_METHOD == VIDEO_CVBS
    if (vfield_count == 0) {
        vfield_count = 1;
    } else {
        vfield_count = 0;
#else
    {
#endif
        if (p_VideoCaptureTask != NULL) {
            p_VideoCaptureTask->signal_set(1);
        }
    }
}

/****** cache control ******/
static void dcache_clean(void * p_buf, uint32_t size) {
    uint32_t start_addr = (uint32_t)p_buf & 0xFFFFFFE0;
    uint32_t end_addr   = (uint32_t)p_buf + size;
    uint32_t addr;

    /* Data cache clean */
    for (addr = start_addr; addr < end_addr; addr += 0x20) {
        __v7_clean_dcache_mva((void *)addr);
    }
}

/*Init camera video*/
void init_video() {
	DisplayBase::graphics_error_t error;

    /* Graphics initialization process */
    if (graphics_init_end == false) {
        /* When not initializing LCD, this processing is needed. */
        error = Display.Graphics_init(NULL);
        if (error != DisplayBase::GRAPHICS_OK) {
            printf("Line %d, error %d\n", __LINE__, error);
            mbed_die();
        }
        graphics_init_end = true;
    }

#if VIDEO_INPUT_METHOD == VIDEO_CVBS
    error = Display.Graphics_Video_init( DisplayBase::INPUT_SEL_VDEC, NULL);
    if( error != DisplayBase::GRAPHICS_OK ) {
        printf("Line %d, error %d\n", __LINE__, error);
        mbed_die();
    }
#elif VIDEO_INPUT_METHOD == VIDEO_CMOS_CAMERA
    DisplayBase::video_ext_in_config_t ext_in_config;
    PinName cmos_camera_pin[11] = {
        /* data pin */
        P2_7, P2_6, P2_5, P2_4, P2_3, P2_2, P2_1, P2_0,
        /* control pin */
        P10_0,      /* DV0_CLK   */
        P1_0,       /* DV0_Vsync */
        P1_1        /* DV0_Hsync */
    };

    /* MT9V111 camera input config */
    ext_in_config.inp_format     = DisplayBase::VIDEO_EXTIN_FORMAT_BT601; /* BT601 8bit YCbCr format */
    ext_in_config.inp_pxd_edge   = DisplayBase::EDGE_RISING;              /* Clock edge select for capturing data          */
    ext_in_config.inp_vs_edge    = DisplayBase::EDGE_RISING;              /* Clock edge select for capturing Vsync signals */
    ext_in_config.inp_hs_edge    = DisplayBase::EDGE_RISING;              /* Clock edge select for capturing Hsync signals */
    ext_in_config.inp_endian_on  = DisplayBase::OFF;                      /* External input bit endian change on/off       */
    ext_in_config.inp_swap_on    = DisplayBase::OFF;                      /* External input B/R signal swap on/off         */
    ext_in_config.inp_vs_inv     = DisplayBase::SIG_POL_NOT_INVERTED;     /* External input DV_VSYNC inversion control     */
    ext_in_config.inp_hs_inv     = DisplayBase::SIG_POL_INVERTED;         /* External input DV_HSYNC inversion control     */
    ext_in_config.inp_f525_625   = DisplayBase::EXTIN_LINE_525;           /* Number of lines for BT.656 external input */
    ext_in_config.inp_h_pos      = DisplayBase::EXTIN_H_POS_CRYCBY;       /* Y/Cb/Y/Cr data string start timing to Hsync reference */
    ext_in_config.cap_vs_pos     = 6;                                     /* Capture start position from Vsync */
    ext_in_config.cap_hs_pos     = 150;                                   /* Capture start position form Hsync */
#if (LCD_TYPE == 0)
    /* The same screen ratio as the screen ratio of the LCD. */
    ext_in_config.cap_width      = 640;                                   /* Capture width */
    ext_in_config.cap_height     = 363;                                   /* Capture height Max 468[line]
                                                                             Due to CMOS(MT9V111) output signal timing and VDC5 specification */
#else
    ext_in_config.cap_width      = 640;                                   /* Capture width  */
    ext_in_config.cap_height     = 468;                                   /* Capture height Max 468[line]
                                                                             Due to CMOS(MT9V111) output signal timing and VDC5 specification */
#endif
    error = Display.Graphics_Video_init( DisplayBase::INPUT_SEL_EXT, &ext_in_config);
    if( error != DisplayBase::GRAPHICS_OK ) {
        printf("Line %d, error %d\n", __LINE__, error);
        mbed_die();
    }

    /* Camera input port setting */
    error = Display.Graphics_Dvinput_Port_Init(cmos_camera_pin, 11);
    if( error != DisplayBase::GRAPHICS_OK ) {
        printf("Line %d, error %d\n", __LINE__, error);
        mbed_die();
    }
#endif

#if(0) /* When needing video Vsync interrupt, please make it effective. */
    /* Interrupt callback function setting (Vsync signal input to scaler 0) */
    error = Display.Graphics_Irq_Handler_Set(DisplayBase::INT_TYPE_S0_VI_VSYNC,
                                                 0, IntCallbackFunc_ViVsync);
    if (error != DisplayBase::GRAPHICS_OK) {
        printf("Line %d, error %d\n", __LINE__, error);
        mbed_die();
    }
#endif

    /* Interrupt callback function setting (Field end signal for recording function in scaler 0) */
    error = Display.Graphics_Irq_Handler_Set(VIDEO_INT_TYPE, 0, 
                                                        IntCallbackFunc_Vfield);
    if (error != DisplayBase::GRAPHICS_OK) {
        printf("Line %d, error %d\n", __LINE__, error);
        mbed_die();
    }
}

static void Start_Video(uint8_t * p_buf) {
    DisplayBase::graphics_error_t error;

    /* Video capture setting (progressive form fixed) */
    error = Display.Video_Write_Setting(
                VIDEO_INPUT_CH,
                COL_SYS,
                p_buf,
                FRAME_BUFFER_STRIDE,
                VIDEO_FORMAT,
                WR_RD_WRSWA,
                VIDEO_PIXEL_VW,
                VIDEO_PIXEL_HW
            );
    if (error != DisplayBase::GRAPHICS_OK) {
        printf("Line %d, error %d\n", __LINE__, error);
        mbed_die();
    }

    /* Video write process start */
    error = Display.Video_Start(VIDEO_INPUT_CH);
    if (error != DisplayBase::GRAPHICS_OK) {
        printf("Line %d, error %d\n", __LINE__, error);
        mbed_die();
    }

    /* Video write process stop */
    error = Display.Video_Stop(VIDEO_INPUT_CH);
    if (error != DisplayBase::GRAPHICS_OK) {
        printf("Line %d, error %d\n", __LINE__, error);
        mbed_die();
    }

    /* Video write process start */
    error = Display.Video_Start(VIDEO_INPUT_CH);
    if (error != DisplayBase::GRAPHICS_OK) {
        printf("Line %d, error %d\n", __LINE__, error);
        mbed_die();
    }
}

/****** Video input is output to LCD ******/
void video_capture_task(void) {
	DisplayBase::graphics_error_t error;
    int wk_num;

    /* Initialization memory */
    for (int i = 0; i < FRAME_BUFFER_NUM; i++) {
        memset(FrameBufferTbl[i], 0, (FRAME_BUFFER_STRIDE * VIDEO_PIXEL_VW));
        dcache_clean(FrameBufferTbl[i],(FRAME_BUFFER_STRIDE * VIDEO_PIXEL_VW));
    }

    /* Start of Video */
    Start_Video(FrameBufferTbl[write_buff_num]);

    /* Wait for first video drawing */
    Thread::signal_wait(1);
    write_buff_num++;
    if (write_buff_num >= FRAME_BUFFER_NUM) {
        write_buff_num = 0;
    }
    error = Display.Video_Write_Change(VIDEO_INPUT_CH, 
                        FrameBufferTbl[write_buff_num], FRAME_BUFFER_STRIDE);
    if (error != DisplayBase::GRAPHICS_OK) {
        printf("Line %d, error %d\n", __LINE__, error);
        mbed_die();
    }

    /* Backlight on */
    Thread::wait(200);

    while (1) {
        Thread::signal_wait(1);
        wk_num = write_buff_num + 1;
        if (wk_num >= FRAME_BUFFER_NUM) {
            wk_num = 0;
        }

        /* If the next buffer is empty, it's changed. */
        if (wk_num != read_buff_num) {
            read_buff_num  = write_buff_num;
            write_buff_num = wk_num;
            
            /* Change video buffer */
            error = Display.Video_Write_Change(VIDEO_INPUT_CH, 
                    FrameBufferTbl[0/*write_buff_num*/], FRAME_BUFFER_STRIDE);
            if (error != DisplayBase::GRAPHICS_OK) {
                printf("Line %d, error %d\n", __LINE__, error);
                mbed_die();
            }
            
            // Read buffer from camera here
            mMutexProcess.lock();
            if ( g_is_processing == false)
            {
                g_is_processing = true;    
                mMutexProcess.unlock();

                memcpy((void *)processing_frame,(void*)FrameBufferTbl[0],
                                        FRAME_BUFFER_STRIDE * VIDEO_PIXEL_VW);
                semProcessThread.release();
            }
            else
            {
                mMutexProcess.unlock();
            }
        }
    }
}

void writeImageTest(void)
{
    while (1) {
        semProcessThread.wait();

        // Notify process done
        mMutexProcess.lock();
        g_is_processing = false;
        mMutexProcess.unlock();
    }
}

void task_capture_user_button()
{
    while(1)
    {
        if (isButtonPressed())
        {
            printf("USER BUTTON\n");
            addPersonStart();
        }
        wait(0.1);
    }
}

void faceRecognition()
{
    Size face_size(VIDEO_PIXEL_HW/IMG_DOWN_SAMPLE,
                                            VIDEO_PIXEL_VW/IMG_DOWN_SAMPLE);
    Size regSize(70,70);
    Mat smallImage = Mat::zeros(face_size, CV_8UC1);
    CascadeClassifier haar_cascade;
    vector<Rect> faces;
    
    // For face database
    int numberLabel;
    face_database_st face_database;
    bool is_trained = false;

    //Init recognizer
    int radius = 2, neighbors = 4, gridx = 8, gridy = 8, thresmax = 50;
    Ptr<LBPHFaceRecognizer> modelReg = createLBPHFaceRecognizer(radius, neighbors,
                                    gridx, gridy, thresmax);

    // For add person
    int countAddPerson = 0;

    if (!haar_cascade.load("/usb/lbpcascade_frontalface.xml"))
    {
        // load failed
        printf("faceRecognition(): Load detect model failed!\n");
        return;
    }
    else{
        printf("faceRecognition(): Load detect model success!\n");   
    }

    // Load database faces and train model
    face_database.database_image.clear();
    face_database.database_label.clear();
    
    {
        vector<infor_label_st> listDb;
        listDb = txt2Label(FACE_DB_FILE,numberLabel);
        // If database has more than one face
        if(numberLabel != -1)
        {   
            for(int i = 0; i < (int)listDb.size();i++)
            {
                face_database.database_image.push_back(listDb[i].img);
                face_database.database_label.push_back(listDb[i].label);
            }        
            
            numberLabel++;
            modelReg->train(face_database.database_image, face_database.database_label);
            is_trained = true;
        }
    }

    printf("Load faces db done!, %d faces\n", numberLabel);

    // End load database

    /* Start tast capture user button */
    p_UserButtonTask = new Thread();
    p_UserButtonTask->start(task_capture_user_button);
    
    /* Let start */
    faces.clear();

    while (1) {
        semProcessThread.wait();

        Mat gray;
        {
            Mat res = _m_cvtRgb5652Rgb(processing_frame,(int)VIDEO_PIXEL_HW,
                                                    (int)VIDEO_PIXEL_VW,2);
            cvtColor(res,gray,COLOR_RGB2GRAY);
            res.release();
        }

        equalizeHist(gray,gray);
        resize(gray, smallImage, face_size);

        haar_cascade.detectMultiScale(smallImage,faces,1.1,
                                        2,0|CV_HAAR_SCALE_IMAGE,Size(10,10));

        if( countAddPerson > 0) // Adding person
        {
            // Add person, NEED TO CHECK MAXIMUM PERSON LATER
            if(faces.size() == 1)
            {
                int _x = faces[0].x*IMG_DOWN_SAMPLE;
                int _y = faces[0].y*IMG_DOWN_SAMPLE;
                int _w = faces[0].width*IMG_DOWN_SAMPLE;
                int _h = faces[0].height*IMG_DOWN_SAMPLE;
                Rect roi(_x,_y,_w,_h);
                
                Mat imgReg = gray(roi);
                resize(imgReg,imgReg,regSize);
                
                if(numberLabel == -1) // For empty database
                    numberLabel = 0;

                face_database.database_image.push_back(imgReg);
                face_database.database_label.push_back(numberLabel);
                
                if(is_trained == true)
                {
                    DBG_INFO("UPDATE!!");
                    modelReg->update(face_database.database_image, face_database.database_label);
                }
                else
                {
                    DBG_INFO("TRAIN!!");
                    modelReg->train(face_database.database_image, face_database.database_label);
                    is_trained = true;
                }

                countAddPerson++;
                printf("Added %d sample\n", countAddPerson);
                
                // Counter added samples
                if(countAddPerson == 5)
                {   
                    // Save database
                    DBG_INFO("SAVE to db %d\n",numberLabel);
                    saveFaceDb(face_database,numberLabel,FACE_DB_FILE);
                    DBG_INFO("SAVE to db DONE\n");
                    
                    numberLabel++;
                    countAddPerson = 0;

                    mMutexAddperson.lock();
                    g_add_person = false;
                    mMutexAddperson.unlock();
                }
            }

        }
        else 
        {
            mMutexAddperson.lock();
            if(g_add_person == true) // Check if user wants to add person
            {
                printf("START ADD PERSON......................\n");
                mMutexAddperson.unlock();
                countAddPerson++;
            }
            else // No adding person
            {
                mMutexAddperson.unlock();
                
                // Process recognize person
                if(faces.size() > 0)
                {
                    printf("DETECTED:   %d faces!\n",(int)faces.size());
                    if (is_trained == true)
                    {
                        DBG_INFO("RECOG!!!!!\n");
                        for(int i = 0; i < faces.size();i++)
                        {
                            int _x = faces[i].x*IMG_DOWN_SAMPLE;
                            int _y = faces[i].y*IMG_DOWN_SAMPLE;
                            int _w = faces[i].width*IMG_DOWN_SAMPLE;
                            int _h = faces[i].height*IMG_DOWN_SAMPLE;
                            
                            Rect roi(_x,_y,_w,_h);
                            Mat target_face = gray(roi);
                            resize(target_face,target_face,regSize);
                            int predicted = -1;
                            double confidence_level = 0;
                            modelReg->predict(target_face, predicted, confidence_level);

                            if(predicted != -1)
                            {
                                if (confidence_level < thresmax)
                                {
                                    mWhoPerson.lock();
                                    g_label_of_person = predicted;
                                    mWhoPerson.unlock();
                                    // if(predicted == 1)
                                    //     printf("Found carot! :)\n");
                                    // else if (predicted == 0)
                                    //     printf("Found The :)\n");
                                    // else
                                    //     printf("Founded Person LABEL%d\n", predicted + 1);
                                }
                            }
                            else
                            {
                                DBG_INFO("PREDICTED -1!!!!!\n");
                            }
                        }
                    }
                }
            }
        }

        /* Reset flag */
        faces.clear();
        mMutexProcess.lock();
        g_is_processing = false;
        mMutexProcess.unlock();
    }
}
