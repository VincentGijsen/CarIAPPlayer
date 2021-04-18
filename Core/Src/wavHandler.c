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

#include "MusicManager.h"

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

struct {
	uint8_t isPLaying;
} _wavPlayerState;
/*
 * protos
 */

WAV_BUFFER_STATUS WavUpdateBuffers();
SongDetails *_songDetails;

void WavInit(SongDetails *songDetails) {
	_songDetails = songDetails;
	xprintf("WavInit\n");
	_wavPlayerState.isPLaying = 0;
	initBuffer();
	//xprintf("file setup, ready to fill buffers\n");
}

void WavPlayFile() {
	xprintf("WavPlayFile: file: %s, \n", MMGetCurrentFilePath());

	FRESULT res = f_open(&SDFile, MMGetCurrentFilePath(), FA_READ);

	if (res != FR_OK)
			return 1;

	/*
	 * read header
	 */
	UINT a = 0;
	char read[4];

	res = f_read(&SDFile, &head, sizeof head, &a);
	if (res != FR_OK)
		return 1;

	//initBuffer();
	_wavPlayerState.isPLaying = 1;

	/*set details */
	uint16_t seconds = head.data_size / (head.byte_rate);
	_songDetails->timeDurationTotalMs = seconds *10;
	_songDetails->timePositionMs=0;

	if (head.data_id[0] != 'L')
		return 1;

	while (read[0] != 'd') {
		f_read(&SDFile, read, sizeof read, &a);
	}

}

/*
 * returns 1 if track is playing
 */
inline uint8_t WavIsPlaying(){
	return _wavPlayerState.isPLaying;
}


/*
 * called often enough to top-up buffer
 */
void WavPlayNonBlocking() {
	if (WavIsPlaying) {
		switch (head.bits_per_sample) {

		case 16: {
			if (head.sample_rate == 48000) {
				//xprintf("starting to fill buffer\n");

				uint8_t slots_to_be_filled = getFreeSlotsInBuffer();

				if (slots_to_be_filled > 1) {
					//xprintf("topping op buffer\n");
					WAV_BUFFER_STATUS res;
					res = WavUpdateBuffers();
					//every buffer etnry is 1ms of time, so simply increment untill done...
					_songDetails->timePositionMs++;

					if (res != WAV_BUFFER_OK) {
						_wavPlayerState.isPLaying = 0;
						xprintf("Buffer running low or ERR\n");
						WavStop();
					}

				}

			}

		}
			break;

		default:
			xprintf("this file cannot be played..\n");
			_wavPlayerState.isPLaying=0;
		}
	} else {
		//nothing to do
		_wavPlayerState.isPLaying=0;

	}
}

/*
 * Blocks return untill all bytes from WAV are read
 */
void WavPlayBlocking() {
	xprintf("going to play");
	uint8_t run = 1;
	//initBuffer();

	switch (head.bits_per_sample) {

	case 16: {
		if (head.sample_rate == 48000) {
			xprintf("starting to fill buffer\n");
			while (run) {
				uint8_t slots_to_be_filled = getFreeSlotsInBuffer();

				if (slots_to_be_filled > 1) {
					//xprintf("topping op buffer\n");
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

void WavStop() {
	_wavPlayerState.isPLaying=0;

	FRESULT res = f_close(&SDFile); //(&SDFile, MMGetCurrentFilePath(), FA_READ);

}
