#ifndef DEFINE_H
#define DEFINE_H

#define SDCARD_NAME "SD"
#define SDCARD_PATH "/SD"

#define SIZE_OF_AUDIO_FRAME		(4096*300)

static uint8_t audio_frame_read[SIZE_OF_AUDIO_FRAME]__attribute((aligned(32)));
static uint8_t audio_frame_backup[SIZE_OF_AUDIO_FRAME + 44]__attribute((aligned(32)));

// Define for camera
#define VIDEO_CVBS             (0)                 /* Analog  Video Signal */
#define VIDEO_CMOS_CAMERA      (1)                 /* Digital Video Signal */
#define VIDEO_YCBCR422         (0)
#define VIDEO_RGB888           (1)
#define VIDEO_RGB565           (2)

/**** User Selection *********/
/** Camera setting **/
#define VIDEO_INPUT_METHOD     (VIDEO_CMOS_CAMERA) /* Select  VIDEO_CVBS or VIDEO_CMOS_CAMERA                       */
#define VIDEO_INPUT_FORMAT     (VIDEO_RGB565 )    /* Select  VIDEO_YCBCR422 or VIDEO_RGB888 or VIDEO_RGB565        */
#define USE_VIDEO_CH           (0)                 /* Select  0 or 1            If selecting VIDEO_CMOS_CAMERA, should be 0.)               */
#define VIDEO_PAL              (0)                 /* Select  0(NTSC) or 1(PAL) If selecting VIDEO_CVBS, this parameter is not referenced.) */
/** LCD setting **/
#define LCD_TYPE               (0)                 /* Select  0(4.3inch) or 1(7.1inch) */
/*****************************/

/** Video and Grapics (GRAPHICS_LAYER_0) parameter **/
/* video input */
#if USE_VIDEO_CH == (0)
  #define VIDEO_INPUT_CH       (DisplayBase::VIDEO_INPUT_CHANNEL_0)
  #define VIDEO_INT_TYPE       (DisplayBase::INT_TYPE_S0_VFIELD)
#else
  #define VIDEO_INPUT_CH       (DisplayBase::VIDEO_INPUT_CHANNEL_1)
  #define VIDEO_INT_TYPE       (DisplayBase::INT_TYPE_S1_VFIELD)
#endif

/* NTSC or PAL */
#if VIDEO_PAL == 0
  #define COL_SYS              (DisplayBase::COL_SYS_NTSC_358)
#else
  #define COL_SYS              (DisplayBase::COL_SYS_PAL_443)
#endif

/* Video input and LCD layer 0 output */
#if VIDEO_INPUT_FORMAT == VIDEO_YCBCR422
  #define VIDEO_FORMAT         (DisplayBase::VIDEO_FORMAT_YCBCR422)
  #define GRAPHICS_FORMAT      (DisplayBase::GRAPHICS_FORMAT_YCBCR422)
  #define WR_RD_WRSWA          (DisplayBase::WR_RD_WRSWA_NON)
#elif VIDEO_INPUT_FORMAT == VIDEO_RGB565
  #define VIDEO_FORMAT         (DisplayBase::VIDEO_FORMAT_RGB565)
  #define GRAPHICS_FORMAT      (DisplayBase::GRAPHICS_FORMAT_RGB565)
  #define WR_RD_WRSWA          (DisplayBase::WR_RD_WRSWA_32_16BIT)
#else
  #define VIDEO_FORMAT         (DisplayBase::VIDEO_FORMAT_RGB888)
  #define GRAPHICS_FORMAT      (DisplayBase::GRAPHICS_FORMAT_RGB888)
  #define WR_RD_WRSWA          (DisplayBase::WR_RD_WRSWA_32BIT)
#endif

/* The size of the video input is adjusted to the LCD size. */
#define VIDEO_PIXEL_HW                (480u)	//Width
#define VIDEO_PIXEL_VW                (272u)	//Heigth

/*! Frame buffer stride: Frame buffer stride should be set to a multiple of 32 or 128
    in accordance with the frame buffer burst transfer mode. */
/* FRAME BUFFER Parameter GRAPHICS_LAYER_0 */
#define FRAME_BUFFER_NUM              (3u)
#if ( VIDEO_INPUT_FORMAT == VIDEO_YCBCR422 || VIDEO_INPUT_FORMAT == VIDEO_RGB565 )
  #define FRAME_BUFFER_BYTE_PER_PIXEL (2u)
#else
  #define FRAME_BUFFER_BYTE_PER_PIXEL (4u)
#endif
#define FRAME_BUFFER_STRIDE           (((VIDEO_PIXEL_HW * FRAME_BUFFER_BYTE_PER_PIXEL) + 31u) & ~31u)

/* Scale image to detect */
#define IMG_DOWN_SAMPLE                (2u)

#if defined(__ICCARM__)
/* 32 bytes aligned */
#pragma data_alignment=32
static uint8_t user_frame_buffer0[FRAME_BUFFER_STRIDE * VIDEO_PIXEL_VW];
#pragma data_alignment=32
static uint8_t user_frame_buffer1[FRAME_BUFFER_STRIDE * VIDEO_PIXEL_VW];
#pragma data_alignment=32
static uint8_t user_frame_buffer2[FRAME_BUFFER_STRIDE * VIDEO_PIXEL_VW];
#else
/* 32 bytes aligned */
static uint8_t user_frame_buffer0[FRAME_BUFFER_STRIDE * VIDEO_PIXEL_VW]__attribute((aligned(32)));
static uint8_t user_frame_buffer1[FRAME_BUFFER_STRIDE * VIDEO_PIXEL_VW]__attribute((aligned(32)));
static uint8_t user_frame_buffer2[FRAME_BUFFER_STRIDE * VIDEO_PIXEL_VW]__attribute((aligned(32)));
#endif
static uint8_t * FrameBufferTbl[FRAME_BUFFER_NUM] = {user_frame_buffer0, user_frame_buffer1, user_frame_buffer2};
static uint8_t processing_frame[FRAME_BUFFER_STRIDE * VIDEO_PIXEL_VW]__attribute((aligned(32)));
// End define for camera

#endif //DEFINE_H
