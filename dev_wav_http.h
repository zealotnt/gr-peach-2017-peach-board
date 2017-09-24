/**************************************************************************//**
* @file          dev_wav.h
* @brief         wav
******************************************************************************/
#include "mbed.h"

/** A class to communicate a dec_wav
 *
 */
class dec_wav_http {
public:

    /** analyze header
     *
     * @param p_title title tag buffer
     * @param p_artist artist tag buffer
     * @param p_album album tag buffer
     * @param tag_size tag buffer size
     * @param fp file pointer
     * @return true = success, false = failure
     */
    bool AnalyzeHeder(uint8_t * p_title, uint8_t * p_artist, uint8_t * p_album, uint16_t tag_size, FILE * fp) {
        printf("dev_wav_http AnalyzeHeder\r\n");
        bool result = false;
        size_t read_size;
        uint8_t wk_read_buff[36];
        uint8_t *data;
        uint32_t chunk_size;
        uint32_t sub_chunk_size;
        uint32_t list_index_max;
        bool list_ok = false;
        uint32_t read_index = 0;
        uint32_t data_index = 0;
        uint16_t wk_len;
        uint8_t while_loop = 0;

        if (fp == NULL) {
            return false;
        }
        music_data_size  = 0;
        music_data_index = 0;
        wav_fp = fp;
        if (p_title != NULL) {
            p_title[0] = '\0';
        }
        if (p_artist != NULL) {
            p_artist[0] = '\0';
        }
        if (p_album != NULL) {
            p_album[0] = '\0';
        }

        read_size = fread(&wk_read_buff[0], sizeof(char), 36, wav_fp);
        if (read_size < 36) {
            // do nothing
        } else if (memcmp(&wk_read_buff[0], "RIFF", 4) != 0) {
            // do nothing
        } else if (memcmp(&wk_read_buff[8], "WAVE", 4) != 0) {
            // do nothing
        } else if (memcmp(&wk_read_buff[12], "fmt ", 4) != 0) {
            // do nothing
        } else {
            read_index += 36;
            channel = ((uint32_t)wk_read_buff[22] << 0) + ((uint32_t)wk_read_buff[23] << 8);
            sampling_rate = ((uint32_t)wk_read_buff[24] << 0)
                          + ((uint32_t)wk_read_buff[25] << 8)
                          + ((uint32_t)wk_read_buff[26] << 16)
                          + ((uint32_t)wk_read_buff[27] << 24);
            block_size = ((uint32_t)wk_read_buff[34] << 0) + ((uint32_t)wk_read_buff[35] << 8);

            read_size = fread(&wk_read_buff[0], sizeof(char), 8, wav_fp);
            read_index += 8;
            if (read_size < 8) {
                // do nothing
            } else {
                chunk_size = ((uint32_t)wk_read_buff[4] << 0)
                           + ((uint32_t)wk_read_buff[5] << 8)
                           + ((uint32_t)wk_read_buff[6] << 16)
                           + ((uint32_t)wk_read_buff[7] << 24);
                if (memcmp(&wk_read_buff[0], "data", 4) == 0) {
                    result = true;
                    music_data_size = chunk_size;
                    data_index = read_index;
                    printf("fseek %d, %d\r\n", __LINE__, chunk_size);
                    read_index += chunk_size;
                } else {
                    printf("Wav header parser: not support header, quitr\r\n");
                    return false;
                }
            }

            if (data_index != 0) {
                printf("fseek %d, %d\r\n", __LINE__, data_index);
                fseek(wav_fp, data_index, SEEK_SET);
            }
        }

        return result;
    };

    /** get next data
     *
     * @param buf data buffer address
     * @param len data buffer length
     * @return get data size
     */
    size_t GetNextData(void *buf, size_t len) {
        if (block_size == 24) {
            // Add padding
            int write_index = 0;
            int wavfile_index;
            int read_len;
            int pading_index = 0;
            uint8_t * p_buf = (uint8_t *)buf;
            size_t ret;

            if ((music_data_index + len) > music_data_size) {
                len = music_data_size - music_data_index;
            }
            while (write_index < len) {
                read_len = (len - write_index) * 3 / 4;
                if (read_len > sizeof(wk_wavfile_buff)) {
                    read_len = sizeof(wk_wavfile_buff);
                }
                music_data_index += read_len;
                ret = fread(wk_wavfile_buff, sizeof(char), read_len, wav_fp);
                if (ret < read_len) {
                    break;
                }
                wavfile_index = 0;
                while ((write_index < len) && (wavfile_index < read_len)) {
                    if (pading_index == 0) {
                        p_buf[write_index] = 0;
                    } else {
                        p_buf[write_index] = wk_wavfile_buff[wavfile_index];
                        wavfile_index++;
                    }
                    if (pading_index < 3) {
                        pading_index++;
                    } else {
                        pading_index = 0;
                    }
                    write_index++;
                }
            }

            return write_index;
        } else {
            if ((music_data_index + len) > music_data_size) {
                len = music_data_size - music_data_index;
            }
            music_data_index += len;

            return fread(buf, sizeof(char), len, wav_fp);
        }
    };

    /** get channel
     *
     * @return channel
     */
    uint16_t GetChannel() {
        return channel;
    };

    /** get block size
     *
     * @return block size
     */
    uint16_t GetBlockSize() {
        return block_size;
    };

    /** get sampling rate
     *
     * @return sampling rate
     */
    uint32_t GetSamplingRate() {
        return sampling_rate;
    };

private:
    #define FILE_READ_BUFF_SIZE    (3072)

    FILE * wav_fp;
    uint32_t music_data_size;
    uint32_t music_data_index;
    uint16_t channel;
    uint16_t block_size;
    uint32_t sampling_rate;
    uint8_t wk_wavfile_buff[FILE_READ_BUFF_SIZE];
};
