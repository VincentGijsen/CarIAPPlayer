/*
 * findPayableFiles.c
 *
 *  Created on: Feb 12, 2021
 *      Author: vtjgi
 */

#include "utils.h"
#include "ff.h"
#include "xprintf.h"

void FindFlacFiles(const char *path, Files *files) {
    files->count = 0;
    FRESULT res;

    DIR dir;
    res = f_opendir(&dir, path);
    if (res != FR_OK) {
        xprintf("Error: f_opendir\n");
        return;
    }

    while (files->count < MAX_FILES_COUNT - 1) {
        FILINFO file_info;
        res = f_readdir(&dir, &file_info);
        if (res != FR_OK) {
        	xprintf("Error: f_readdir\n");
            return;
        }

        if (file_info.fname[0] == '\0') {
            break;
        }

        if (!(file_info.fattrib & AM_DIR)) {
            int len = strlen(file_info.fname);
            xprintf("checking file %s, len: %d\n", file_info.fname, len);
            if (len >= 5 && len <= MAX_FILE_PATH_LENGTH && strcmp(&file_info.fname[len - 4], ".FLA") == 0) {
                xprintf("Found: %s\n", file_info.fname);
                strncpy(files->files[files->count], file_info.fname, MAX_FILE_PATH_LENGTH);
                files->count++;
            }
        }
    }

    f_closedir(&dir);
}
