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


typedef struct {
    char files[MAX_FILES_COUNT][MAX_FILE_PATH_LENGTH + 1];
    int count;
} Files;

void FindFlacFiles(const char *path, Files *files);


#endif /* INC_UTILS_H_ */
