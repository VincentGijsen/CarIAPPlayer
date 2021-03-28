/*
 * flacHandler.h
 *
 *  Created on: Feb 12, 2021
 *      Author: vtjgi
 */

#ifndef INC_FLACHANDLER_H_
#define INC_FLACHANDLER_H_

#define FLAC_16BIT 16
#define FLAC_24BIT 24
#define FLAC_HZ_48000   			48000
#define FLAC_HZ_44100				44100
#define USB_SAMPLES_AT_44100RATE_A_MS 88 /*44.1 * 2 CHANNELS of samples*/
#define USB_SAMPLES_AT_44100RATE_EXTRA_SAMPLE_INTERVAL 10 /*EVERY 10 iteratations ADD ADITIAL FRAME(s) TO COMPENSATE FOR FRACTURE */
#define USB_SAMPLES_AT_44100RATE_EXTRA_TO_INCLUDE 2 /*GET AT interval X amount of extra frames (assumes two cahnnels), hence 2*/
#define USB_SAMPLES_AT_44100_AT_16SINT_OF_BYTES 2 /*USB accepts uint8_t bytes, so for 16bit audio, two bytes are needed */
#define USB_SAMPLES_AT_48000RATE_A_MS 96
#define USB_SAMPLES_AT_48000RATE_EXTRA_SAMPLE_INTERVAL 0
#define USB_SAMPLES_AT_48000RATE_EXTRA_TO_INCLUDE 0
#define USB_SAMPLES_AT_48000_AT_16SINT_OF_BYTES 2

#include "dr_flac.h"

/*
 * Holds all state of current song for the decoder
 */
typedef struct appData {
	drflac *pFlac;
	FIL *currentFile;
	uint8_t framecnt;
	uint8_t interval_for_extra_frames;
	uint8_t aditional_frames_on_interval;
	uint8_t bytes_per_sample;
	uint16_t framesToRead; //doublebuf size

} appData;


#define MAX_META_STR_LEN 20

typedef struct _SongData {
	char artists[MAX_META_STR_LEN];
	char title[MAX_META_STR_LEN];
	char album[MAX_META_STR_LEN];
	uint8_t fieldsInited;
} SongData;

typedef struct {
	uint32_t freq;
	uint8_t bits;
	uint8_t channels;
} SongStreamProperties;

typedef enum {
	FLAC_DECODER_READY, FLAC_DECODER_ERROR
} FLAC_HANDLER_STATUS;

typedef enum {
	FLAC_PCM_STATUS_FILLED, FLAC_PCM_STATUS_PARTIAL_FILLED, FLAC_PCM_STATUS_ERROR,
} FLAC_PCM_STATUS;

FLAC_HANDLER_STATUS drFlac_play(FIL *file);
FLAC_PCM_STATUS drFlac_updatePCMBatch();

#endif /* INC_FLACHANDLER_H_ */
