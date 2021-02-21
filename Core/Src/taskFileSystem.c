/*
 * taskFileSystem.c
 *
 *  Created on: Feb 12, 2021
 *      Author: vtjgi
 */

#include "utils.h"
#include "xprintf.h"
#include "stdint.h"
#include "ff.h"

//#include "audiodev.h"
#include "flacHandler.h"

/*
 * Prototypes
 */
static const char* GetCurrentFilePath(void);

/*
 * Statics vars
 */
static FATFS fs;
static Files files;
static int current_file_index = 0;

static uint8_t bench = 0;
//__audiodev audiodev;

static const char* GetCurrentFilePath(void) {
	static char path[3 + MAX_FILE_PATH_LENGTH + 1];
	//snprintf(path, sizeof(path), "0:/%s", files.files[current_file_index]);
	path[0] = '0';
	path[1] = ':';
	path[2] = '/';

	for (uint8_t x = 0; x < 51; x++) {
		uint8_t c = files.files[current_file_index][x];
		if (c != '\0' && c != '\n') {
			path[3 + x] = c;
		} else {
			path[3 + x] = '\0';
		}

	}
	return path;
}

void StartTaskFS(void *argument) {
	/* USER CODE BEGIN StartTaskFS */

	/* Handle for the mounted filesystem */
	xprintf("Task: Filesystem starting\n");

	/* FatFs return code */
	volatile FRESULT fr;

	/* File handle */
	FIL fil;

	/* Used for bytes written, and bytes read */
	UINT bw = 999;
	UINT br = 0;

	/* Sets a valid date for when writing to file */
	//set_fattime (1980, 1, 1, 0, 0, 0);
	xprintf("Mounting FS \n");
	fr = f_mount(&fs, "", 1); //Mount storage device

	if (fr == FR_OK) {
		xprintf("FS mounted successfully\n");
		//led_set(2);
	} else {
		xprintf("Failed to mount disk\n");
		//led_set(1);
		while (1) {
			//hold 4ever
			//vTaskDelay(100);
		}
	}

	char str[23] = { '\0' };
	fr = f_getlabel("", str, 0);

	xprintf("Disk Label: %s\n", str);

	FindFlacFiles("0:", &files);

	xprintf("found %d files\n", files.count);

	current_file_index = 0;
//	flac_play_song(GetCurrentFilePath());

	FRESULT res = f_open(&fil, GetCurrentFilePath(), FA_READ);
	if (res == FR_OK) {
		xprintf("starting to play: %s\n");
		if (drFlac_play(&fil) == FLAC_DECODER_READY) {
			uint8_t run = 1, divider = 0;
			bench = 1;
		//	TickType_t start = xTaskGetTickCount();
		//	TickType_t end = 0;
			xprintf("starting PCM decoding\n");
		//	start = xTaskGetTickCount();
			while (run) {

				FLAC_PCM_STATUS res = drFlac_updatePCMBatch();

				switch (res) {
				case BUFFER_FILLED:
					//all ok
					divider = (divider + 1) % 10;
					if (divider == 0) {
						if (bench) {
							bench = 0;
						} else
							bench = 1;
					}
					break;
				case BUFFER_PARTIAL_FILLED:
					//last decoder session
					xprintf("Buffer Partly filled\n");
					bench = 250;
					break;

				case BUFFER_ERROR:
					xprintf("Error getting PCM\n");
				//	end =xTaskGetTickCount();
				//	xprintf("total decoding time of song: %d\n", (end - start));
					//error
					//handle error
					bench = 100;
					while (1) {
						//vTaskDelay(1000);
					}
					break;
				default:
					//
					//bench +0;
					;
				}
			}

		}

	} else {
		xprintf("failed to open file:%s\n");
	}

	/* Infinite loop */
	int x = 0;
	for (;;) {
		xprintf("%d TaskFS ping\n", x);
		x++;
		x = x % 100;
		//osDelay(5000);
	}
	/* USER CODE END StartTaskFS */
}
