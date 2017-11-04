#include "select-demo.h"
#include <string>
#include <vector>
#include <map>

#include "mbed.h"

#include "http_request.h"
#include "http_parser.h"
#include "http_response.h"
#include "http_request_builder.h"
#include "http_response_parser.h"
#include "http_parsed_url.h"

#include "mbedtls/platform.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/sha1.h"

#include "TLV320_RBSP.h"
#include "FATFileSystem.h"
#include "USBHostMSD.h"
#include "usb_host_setting.h"
#include "dec_wav.h"
#include "grUtility.h"
#include "grHwSetup.h"

int main_http();
int main_save_file();
int main_wav_player();
int main_download_save_play();
int main_wav_player_func();
int main_broadcast_receive();
int main_test_json();
int main_test_node_devices();
int main_upload_file();
int main_upload_file_from_usb();
int main_grpeach();
int main_sound_record_play_echo();
int main_dec_mp3_usb();
int main_dec_mp3_http();
int main_stream_audio();

int main() {
    // return main_http();
    // return main_save_file();
    // return main_wav_player();
    // return main_download_save_play();
    // return main_wav_player_func();
    // return main_broadcast_receive();
    // return main_test_json();
    // return main_test_node_devices();
    // return main_upload_file();
    // return main_upload_file_from_usb();
    // return main_sound_record_play_echo();
    // return main_dec_mp3();
    // return main_dec_mp3_http();
    // return main_grpeach();
    return main_stream_audio();
}
