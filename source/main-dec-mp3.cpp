#include "mbed.h"
#include "TLV320_RBSP.h"
#include "cProcess.hpp"
#include "mStorage.hpp"
#include "mWav.hpp"
#include "define.h"
#include "rtos.h"
#include "grHwSetup.h"
#include "grUtility.h"
#include "Json.h"
#include "cJson.h"
#include "cNodeManager.h"
#include <string>
#include "mp3_decoder.h"

extern int audio_stream_consumer(const char *recv_buf, ssize_t bytes_read,
        void *user_data);

int main_dec_mp3()
{
    // mp3_decoder_task(NULL);
    audio_stream_consumer(NULL, 0, NULL);
    return 0;
}
