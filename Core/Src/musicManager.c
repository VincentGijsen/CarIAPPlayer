/*

 * musicManager.c
 *
 *  Created on: 12 Apr 2021
 *      Author: vtjgi
 */
#include "ff.h"
#include "fatfs.h"
#include <stdint.h>
#include "xprintf.h"
#include "MusicManager.h"
#include "lingos.h"
#include "tasks.h"

#define MAX_FILES_COUNT 10
#define MAX_FILE_PATH_LENGTH  20

typedef struct {
	char files[MAX_FILES_COUNT][MAX_FILE_PATH_LENGTH + 1];
	int count;
} Files;

struct {
	uint8_t isInitialized;
	uint8_t numberOfRootFolders; //max
	uint8_t current_file_index;
	MMplayserState playerState;
	Files files;

}static _MMstate; //__attribute__((section("ccmram")));

uint8_t _MMboot = 0;

//waste rsult var
FRESULT fr;

void FindWavFiles(const char *path, Files *files) {
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
		//break on error
		if (res != FR_OK) {
			xprintf("Error: f_readdir\n");
			return;
		}

		//break on end of dir
		if (file_info.fname[0] == '\0') {
			break;
		}

		if (!(file_info.fattrib & AM_DIR)) {
			int len = strlen(file_info.fname);
			xprintf("checking file %s, len: %d\n", file_info.fname, len);
			if (len >= 5 && len <= MAX_FILE_PATH_LENGTH
					&& strcmp(&file_info.fname[len - 4], ".WAV") == 0) {
				xprintf("Found: %s\n", file_info.fname);
				strncpy(files->files[files->count], file_info.fname,
				MAX_FILE_PATH_LENGTH);
				files->count++;
			}
		} else {
			//found dir:
			xprintf("dir: %s\n", file_info.fname);
		}
	}

	f_closedir(&dir);
}

void MMindexerRootfolders() {
	static uint8_t isDone = 0;
	if (!isDone) {
		FindWavFiles("0:/DEMO", &_MMstate.files);

	}
	isDone = 1;

	if (_MMstate.files.count == 0) {
		//no files found
		xprintf("No files found, waiting infinite\n");
		while (1) {
			//
		}
	}
}

//call to start, only after accesoire-init!!
void MMInit() {
	if (_MMboot == 1)
		return;

	_MMboot = 1;

	_MMstate.isInitialized = 0;
	_MMstate.numberOfRootFolders = 0;
	_MMstate.current_file_index = 0;
	_MMstate.playerState = MMStartUp;

	xprintf("Mounting FS \n");

	fr = f_mount(&SDFatFS, "", 0);

	if (fr != FR_OK) {
		xprintf("Failed to mount FS\n");
		while (1) {
		};
	}
//init codec stufs (like buffers)
	WavInit();

	xprintf("Starting Music Manager\n");

}

void MMOpenFile() {

	fr = f_open(&SDFile, "1.wav", FA_READ);
	if (fr != FR_OK) {
		xprintf("Failed to mount file\n");
		while (1) {
		};
	}
}

const char* MMGetCurrentFilePath() {
	static char path[7 + MAX_FILE_PATH_LENGTH + 1];
	//snprintf(path, sizeof(path), "0:/%s", files.files[current_file_index]);
	path[0] = '0';
	path[1] = ':';
	path[2] = '/';
	path[3] = 'D';
	path[4] = 'E';
	path[5] = 'M';
	path[6] = 'O';
	path[7] = '/';

	for (uint8_t x = 0; x < 51; x++) {
		uint8_t c = _MMstate.files.files[_MMstate.current_file_index][x];
		if (c != '\0' && c != '\n') {
			path[8 + x] = c;
		} else {
			path[8 + x] = '\0';
		}

	}
	return path;
}

void MMPlayBackStateChanged() {

	//notify acc
	typeDefTask task;
	task.f = &taskTrackChanged;
	task.scheduledAfter = uwTick + 10;
	scheduleTasks(task);

	WavPlayFile();
}

void processMusicManager() {
	if (_MMboot == 0)
		return;

	static uint8_t running = 0;

	MMindexerRootfolders();

	if (!_MMboot) {
		_MMstate.playerState = MMPlaying;

		WavPlayFile();
	}

	switch (_MMstate.playerState) {
	case MMStopped:
		break;

	case MMStartUp:
		_MMstate.playerState = MMPlaying;

		WavPlayFile();
		break;

	case MMPlaying:
		//fill buffer with data
		WavPlayNonBlocking();
		//if we finished, proceed to next track!
		if (!WavIsPlaying()) {
			xprintf("wave done, proceeding to next file\n");
			_MMstate.playerState = MMNext;

		}

		break;

	case MMNext: {
		WavStop();
		if (_MMstate.current_file_index < _MMstate.files.count)
			_MMstate.current_file_index++;
		else
			_MMstate.current_file_index = 0;

		MMPlayBackStateChanged();
		_MMstate.playerState = MMPlaying;

	}
		break;

	case MMPrevious: {
		WavStop();
		//_MMstate.current_file_index =
		if (_MMstate.current_file_index > 0)
			_MMstate.current_file_index--;

		else
			_MMstate.current_file_index = _MMstate.files.count - 1;
		_MMstate.playerState = MMPlaying;
		MMPlayBackStateChanged();

	}
		break;

	}

}

void MMSendEvent(MMplayserState evt) {
	_MMstate.playerState = evt;
}
