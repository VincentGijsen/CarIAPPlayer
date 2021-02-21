/*
 * utils.h
 *
 *  Created on: Feb 12, 2021
 *      Author: vtjgi
 */

#ifndef INC_UTILS_H_
#define INC_UTILS_H_

#define MAX_FILES_COUNT 10
#define MAX_FILE_PATH_LENGTH  50

#include "stdint.h"

typedef struct {
    char files[MAX_FILES_COUNT][MAX_FILE_PATH_LENGTH + 1];
    int count;
} Files;

typedef struct {
	uint16_t pWrite;
	uint16_t pRead;
	uint16_t slots;
	uint8_t *pData;
	uint8_t Isinitialized;


}AUDIO_CircularBuffer_t;

typedef enum{
	BUFFER_OK,
	BUFFER_ERROR,
	BUFFER_UNDERRUN
}BUFFER_RESULT;

void FindFlacFiles(const char *path, Files *files);


#endif /* INC_UTILS_H_ */
