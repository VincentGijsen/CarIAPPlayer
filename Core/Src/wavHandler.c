/*
 * wavHandler.c
 *
 *  Created on: Mar 28, 2021
 *      Author: vtjgi
 */

#include "ff.h"
#include "fatfs.h"
#include "xprintf.h"
#include "utils.h"

typedef enum {
	WAV_BUFFER_OK, WAV_BUFFER_ERROR, WAV_BUFFER_EOF
} WAV_BUFFER_STATUS;

struct header {
	char fileformat[4];
	int file_size;
	char subformat[4];
	char subformat_id[4];
	int chunk_bits;     				// 16or18or40 due to pcm it is 16 here
	short int audio_format;    					// little or big endian
	short int num_channels;     				// 2 here for left and right
	int sample_rate;				// sample_rate denotes the sampling rate.
	int byte_rate;           					// bytes  per second
	short int bytes_per_frame;
	short int bits_per_sample;
	char data_id[4];    					// "data" written in ascii
	int data_size;
} head;

/*
 * protos
 */

WAV_BUFFER_STATUS WavUpdateBuffers();

void WavInit() {
	FRESULT fr;

	xprintf("Mounting FS \n");

	fr = f_mount(&SDFatFS, "", 0);

	if (fr != FR_OK) {
		xprintf("Failed to mount FS\n");
		while (1) {
		};
	}

	fr = f_open(&SDFile, "1.wav", FA_READ);
	if (fr != FR_OK) {
		xprintf("Failed to mount file\n");
		while (1) {
		};
	}

	/*
	 * read header
	 */
	UINT a = 0;
	char read[4];

	f_read(&SDFile, &head, sizeof head, &a);
	if (fr != FR_OK)
		return 1;

	if (head.data_id[0] != 'L')
		return 1;

	while (read[0] != 'd') {
		f_read(&SDFile, read, sizeof read, &a);
	}
}

void WavPlayBlocking() {
	xprintf("going to play");
	uint8_t run = 1;
	initBuffer();

	switch (head.bits_per_sample) {

	case 16: {
		if (head.sample_rate == 48000) {
			while (run) {
				uint8_t slots_to_be_filled = getFreeSlotsInBuffer();

				if (slots_to_be_filled > 1) {
					xprintf("topping op buffer\n");
					WAV_BUFFER_STATUS res;
					res = WavUpdateBuffers();
					if (res != WAV_BUFFER_OK) {
						run = 0;
					}

				}
				{

				}

			}
			xprintf("Run false, stopping playback\n");
		}
	}
	}
}

WAV_BUFFER_STATUS WavUpdateBuffers() {
	BUFFER_RESULT res;
	//check for available space

	/*while (i < head.file_size) {
	 if (i != j) {
	 f_read(&fil, &data, sizeof data, &a);
	 TIM_SetCompare2(TIM2, data / 66);
	 j = i;
	 //Toggle_Gpio(GPIOC,15);
	 }
	 }
	 */
	if (head.byte_rate == 192000) {
		UINT actual_read = 0;
		uint8_t data[192]; //max rate 2 channels at 48khz
		f_read(&SDFile, &data, sizeof(data), &actual_read);

		res = putBufferAs8Bytes(&data, sizeof(data));

		if (res == BUFFER_OK) {
			if (actual_read == sizeof(data)) {
				return WAV_BUFFER_OK;
			} else {
				return WAV_BUFFER_EOF;
			}
		}
		return WAV_BUFFER_ERROR;

	}
	return WAV_BUFFER_ERROR;
}

void WaveStop() {

}
