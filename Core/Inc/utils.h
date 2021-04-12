/*
 * utils.h
 *
 *  Created on: Feb 12, 2021
 *      Author: vtjgi
 */

#ifndef INC_UTILS_H_
#define INC_UTILS_H_


#include "stdint.h"


#define AUDIO_CircularBuffer_SLOTS 20
typedef struct {
	uint8_t frame[192];
	uint8_t len;
}_AUDIO_FRAME;


typedef struct {
	uint8_t volatile pWrite;
	uint8_t volatile pRead;
	uint8_t volatile slots;
	_AUDIO_FRAME volatile data[AUDIO_CircularBuffer_SLOTS];
	uint8_t volatile Isinitialized;
}AUDIO_CircularBuffer_t;

typedef enum{
	BUFFER_OK,
	BUFFER_ERROR,
	BUFFER_UNDERRUN
}BUFFER_RESULT;

//void FindFlacFiles(const char *path, Files *files);

BUFFER_RESULT initBuffer();
BUFFER_RESULT getBuffer(_AUDIO_FRAME volatile **ptr);
BUFFER_RESULT flagDataAsRead();
BUFFER_RESULT putBuffer(uint16_t *pcmSamples, uint8_t len, uint8_t bytesPerSample);
uint16_t getOccupiedSlotsInBuffer();
uint16_t getFreeSlotsInBuffer();

#endif /* INC_UTILS_H_ */
