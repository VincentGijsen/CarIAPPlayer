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
	SongDetails song;

}static _MMstate; //__attribute__((section("ccmram")));

tMusicDB musicDb;
uint8_t _MMboot = 0;

//waste rsult var
FRESULT fr;

static IdxRepository MyMusicDB;
#define MAX_DIR_NAME_LEN (13+4)

int8_t mmGetDirectoryInRootNameByIdx(const uint8_t dirIdx, char *pName,
		uint8_t *pNameLen, uint8_t *pLastDirFoundCnt);

int8_t mmGetFileInDir(uint8_t fileIdx, char *dirPath,
		uint8_t *pLastFileFoundCnt, uint8_t bUpdateRepo, char *retVal) {

	char path[20];
	path[0] = '0';
	path[1] = ':';
	path[2] = '/'; //"0:/";

	uint8_t len = strlen(dirPath);
	strncpy(&path[3], dirPath, len);
	path[3+len] = '\0'; //add aditional zero for proper 'string'
	//by now the path should equal root+dir
	DIR dir;
	FRESULT res;
	xprintf("opening dir: %s\n", path);
	res = f_opendir(&dir, path);

	if (res != FR_OK) {
		xprintf("Error: mmGetFileInDirectoryByIdxe f_opendir: %s\n", path);
		return -1;
	}

	uint8_t searchIdx = 0;
	while (searchIdx < 254) {

		FILINFO file_info;
		res = f_readdir(&dir, &file_info);
		if (res != FR_OK) {
			xprintf("Error: f_readdir\n");
			return -1;
		}

		//break on end of dir
		if (file_info.fname[0] == '\0') {

			return -1;
		}
		if (!(file_info.fattrib & AM_DIR)) {

			*pLastFileFoundCnt = searchIdx;

			xprintf("found file %s \n", file_info.fname);

			uint8_t len = strlen(&file_info.fname);
			if (len >= 5 && len <= MAX_FILE_PATH_LENGTH
					&& strcmp(&file_info.fname[len - 4], ".WAV") == 0) {

				//WE HAVE A PLAYABLE FILE
				//thisi is the dir # we searched for
				uint8_t len = strlen(file_info.fname);
				strncpy(&(retVal), file_info.fname, len);

				//return 1 to indicate we found the requested file
				//important to check here as wel!!
				if (searchIdx == fileIdx) {

					return 1;
				}

			}					//not a 'playable file'
			else {

			}

		}
		//return 1 to indicate we found the requested file
		if (searchIdx == fileIdx) {

			return 1;
		}
		searchIdx++;

	};
	return -1;

}

/*
 * returns;
 * -1 ERROR
 * 0 NOT FOUND THIS ITERATION
 * 1 COMPLETED
 */

int8_t mmGetDirectoryInRootNameByIdx(const uint8_t dirIdx, char *pName,
		uint8_t *pNameLen, uint8_t *pLastDirFoundCnt) {
	FRESULT res;
	//const char *path = "0:/";
	pName[0] = '0';
	pName[1] = ':';
	pName[2] = '/';
	pName[3] = 0;
	DIR dir;

	res = f_opendir(&dir, pName);
	if (res != FR_OK) {
		xprintf("Error: f_opendir: %s\n", pName);
		return -1;
	}

	uint8_t searchIdx = 0;
	while (searchIdx < 254) {

		FILINFO file_info;
		res = f_readdir(&dir, &file_info);
		if (res != FR_OK) {
			xprintf("Error: f_readdir\n");
			//f_closedir(dir);
			return -1;
		}

		//break on end of dir
		if (file_info.fname[0] == '\0') {
			//f_closedir(dir);
			return -1;
		}
		MyMusicDB.rootInodes[searchIdx].isUsefull = 0;
		if ((file_info.fattrib & AM_DIR)) {
			*pLastDirFoundCnt = searchIdx + 1;
			xprintf("found %d :  dir %s \n", searchIdx, file_info.fname);

			//todo: should be implemented in the music-file-scanner, otherwise emty/non rerelvant will show up
			MyMusicDB.rootInodes[searchIdx].isUsefull = 1;
			if (searchIdx == dirIdx) {
				//thisi is the dir # we searched for
				uint8_t length = strlen(file_info.fname);
				if (length > MAX_DIR_NAME_LEN)
					length = MAX_DIR_NAME_LEN;

				*pNameLen = strlen(file_info.fname);
				strncpy(&(pName[0]), file_info.fname, *pNameLen);
				pName[(*pNameLen)] = 0;
				*pNameLen += 1;
				return 1;
			}
			searchIdx++;
		}
	}
	return -1;
}

uint8_t runIndexer() {
	xprintf("Indexer Started...\n");
	uint32_t start, stop = 0;
	start = uwTick;
	uint8_t lastCount = 0;
	//todo fix for long-filename-support
	uint8_t len = 0;
	char dirname[MAX_DIR_NAME_LEN];

	mmGetDirectoryInRootNameByIdx(255, &dirname, &len, &lastCount);
	MyMusicDB.numOfRootInodes = lastCount;

	char fName[MAX_DIR_NAME_LEN];
	uint8_t fnameLen, pLas;

	return;
	for (uint8_t it = 0; it < MyMusicDB.numOfRootInodes; it++) {
		uint8_t current[20];
		uint8_t len=0;
		uint8_t foundSongs =0;
		if(MyMusicDB.rootInodes[it].isUsefull){
			mmGetDirectoryInRootNameByIdx(it, &current, &len, 0);
			mmGetFileInDir(255, &current, &foundSongs, 1, 0);
			xprintf("found in %s, %d songs\n", current, foundSongs);
		}
		//	uint8_t x = it;
	//	int8_t ret = 7;
		//ret = mmGetFileInDirectoryByIdxes //(x, 255, &fName, &len, &lastCount, 1);
	}
	int8_t ret = 7;

	/*mmGetFileInDir(uint8_t fileIdx, char *dirPath,
	 uint8_t *pLastFileFoundCnt, uint8_t bUpdateRepo)
	 */

	mmGetDirectoryInRootNameByIdx(5, &dirname, &len, &lastCount);

	char pad[] = "DEMO";
	char fileN[MAX_DIR_NAME_LEN];
	mmGetFileInDir(255, pad, &len, 1, &fileN);

	for (uint8_t x = 0; x < MyMusicDB.numOfRootInodes; x++) {

		//ret = mmGetFileInDirectoryByIdxes(x, 255, &fName, &len, &lastCount, 1);
		xprintf("x: %d - dir: %s\n", x, fName);
	}

	/*for (uint8_t x = 0; x < 8; x++) {
	 xprintf("x: %d\n", x);
	 ret = mmGetFileInDirectoryByIdxes(x, 255, &fName, &len, &lastCount, 1);
	 }
	 char n[MAX_DIR_NAME_LEN];
	 mmGetDirectoryInRootNameByIdx(6, &n, 0, 0);
	 xprintf("Dir #5 name: %s\n", n);

	 mmGetFileInDirectoryByIdxes(6, 3, &n, 0, 0, 0);
	 xprintf("Dir #5, File  #3 name: %s\n", n);

	 xprintf("found :%d of files\n", lastCount);
	 xprintf("filename: %s \n", fName);*/
	stop = uwTick;
	xprintf("indexing took: %d\n", (stop - start));

}

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

		runIndexer();

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

uint8_t mmGetActualInodeIdxFromRelative(uint8_t requested) {
	uint8_t usableCnt = 0;

	for (uint8_t x = 0; x < MyMusicDB.numOfRootInodes; x++) {
		if (MyMusicDB.rootInodes[x].isUsefull) {
			usableCnt++;
			if (usableCnt == requested) {
				return usableCnt;
			}
		}
	}
}

void MMResetSelections() {
	MyMusicDB.selectedRootInode = 0;
}
void MMSelectItem(uint8_t category, uint32_t item) {
	uint8_t req = mmGetActualInodeIdxFromRelative(item);
	MyMusicDB.selectedRootInode = req;
}
/*
 * track stuff
 */

uint8_t MMgetNumberOfTracks() {
	return MyMusicDB.rootInodes[MyMusicDB.selectedRootInode].numberOfTracks;
}

/*
 * Playlist sjizzle
 */
void MMgetPlaylistItem(uint8_t idx, char *content, uint8_t *len) {
	uint8_t req = mmGetActualInodeIdxFromRelative(idx);
	mmGetDirectoryInRootNameByIdx(req, content, len, 0);

}

uint8_t MMgetNumberOfPLaylists() {
	uint8_t usable = 0;
	for (uint8_t x = 0; x < MyMusicDB.numOfRootInodes; x++) {
		if (MyMusicDB.rootInodes[x].isUsefull) {
			usable++;
		}
	}

	return usable;
}

tMusicDB* MMgetMusicDB() {
	return &musicDb;
}
SongDetails* MMgetSongDetailsOfCurrent() {
	return &_MMstate.song;
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
	_MMstate.song.trackIdx = 0xffffffFe;
	musicDb.category[DB_CAT_PLAYLIST] = 1;
	musicDb.category[DB_CAT_TRACK] = 20;

	xprintf("Mounting FS \n");

	fr = f_mount(&SDFatFS, "", 0);

	if (fr != FR_OK) {
		xprintf("Failed to mount FS\n");
		while (1) {
		};
	}
//init codec stufs (like buffers)
	WavInit(&_MMstate.song);

	xprintf("Starting Music Manager\n");

	runIndexer();

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

	for (uint8_t x = 0; x < 21; x++) {
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
