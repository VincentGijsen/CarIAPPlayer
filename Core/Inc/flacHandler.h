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
#define FLAC_HZ_48000 48000
#define FLAC_HZ_44100	44100
#define FLAC_SAMPLES_MS_44100_16BIT 88
#define FLAC_SAMPLES_MS_44100_16BIT_EXTRA_EACH_FRAMES 10
#define FLAC_SAMPLES_MS_44100_16BIT_EXTRA_EACH_FRAMES_TO_INSERT 2
#define FLAC_SAMPLES_MS_44100_16BIT_BYTES_PER_SAMPLE 2
#define FLAC_SAMPLES_MS_48000_16BIT 96
#define FLAC_SAMPLES_MS_48000_16BIT_EXTRA_EACH_FRAMES 0
#define FLAC_SAMPLES_MS_48000_16BIT_EXTRA_EACH_FRAMES_TO_INSERT 0
#define FLAC_SAMPLES_MS_48000_16BIT_BYTES_PER_SAMPLE 2

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
//uint64_t totalPCMFrames;
//uint32_t currentPCM;

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
