#ifndef DEFINE_H
#define DEFINE_H

#define SDCARD_NAME "SD"
#define SDCARD_PATH "/SD"

#define SIZE_OF_AUDIO_FRAME		(4096*400)

static uint8_t audio_frame_read[SIZE_OF_AUDIO_FRAME]__attribute((aligned(32)));
static uint8_t audio_frame_backup[SIZE_OF_AUDIO_FRAME]__attribute((aligned(32)));

#endif //DEFINE_H
