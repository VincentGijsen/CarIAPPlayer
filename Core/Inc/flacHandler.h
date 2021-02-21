/*
 * flacHandler.h
 *
 *  Created on: Feb 12, 2021
 *      Author: vtjgi
 */

#ifndef INC_FLACHANDLER_H_
#define INC_FLACHANDLER_H_

#include "dr_flac.h"

typedef struct appData {
	drflac *pFlac;
	FIL *currentFile;
	uint8_t framesToRead; //doublebuf size

} appData;

typedef struct _SongData {
	char *artists;
	char *title;
	char *album;
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
	BUFFER_FILLED, BUFFER_PARTIAL_FILLED, BUFFER_ERROR,
} FLAC_PCM_STATUS;

FLAC_HANDLER_STATUS drFlac_play(FIL *file);
FLAC_PCM_STATUS drFlac_updatePCMBatch();

#endif /* INC_FLACHANDLER_H_ */
