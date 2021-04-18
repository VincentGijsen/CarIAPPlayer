/*
 * MusicManager.h
 *
 *  Created on: 12 Apr 2021
 *      Author: vtjgi
 */

#ifndef INC_MUSICMANAGER_H_
#define INC_MUSICMANAGER_H_



#define DB_CAT_TOP 0x00
#define DB_CAT_PLAYLIST 0x01
#define DB_CAT_ARTIST  0x02
#define DB_CAT_ALBUM 0x03
#define DB_CAT_GENRE 0x04
#define DB_CAT_TRACK 0x05
#define DB_CAT_COMPOSER 0x06
#define DB_CAT_AUDIO_BOOK 0x07
#define DB_CAT_PODCAST 0x08
#define DB_CAT_NESTED_PLAYLIST 0x09
#define DB_CAT_GENIUSPLAYLIST 0x0A
#define DB_CAT_ITUNES 0x0B


/*
 * types
 *
 */
typedef enum {
	MMStopped = 0, //
	MMPlaying,
	MMPause,
	MMNext,
	MMPrevious,
	MMStartUp,

} MMplayserState;

#define SONG_DATA_MAX_LEN 30

typedef struct{
	uint8_t artist[SONG_DATA_MAX_LEN];
	uint8_t album[SONG_DATA_MAX_LEN];
	uint8_t trackName[SONG_DATA_MAX_LEN];
	uint8_t trackNumber;
	uint32_t timeDurationTotalMs;
	uint32_t timePositionMs;
	uint32_t trackIdx;
}SongDetails;


typedef struct{
	uint16_t entries;

}tDBcategory;

typedef struct {
	uint32_t category[0x0b];
}tMusicDB;


//index
//basically the 'playlists'
typedef struct{
	uint8_t subDirInodes[50];
	uint8_t numberOfTracks;
	uint8_t isUsefull;
}IdxInodes;

typedef struct {
	IdxInodes rootInodes[50];
	uint8_t numOfRootInodes;
	uint8_t selectedRootInode;
}IdxRepository;

/*
 * codec stuf
 */

uint8_t WavIsPlaying();
void WavPlayNonBlocking() ;
void WavPlayFile();
void WavStop() ;
void WavInit(SongDetails *songDetails);

/*
 * interfaces for other parts
 */
void MMSendEvent(MMplayserState evt);
void MMInit();
SongDetails* MMgetSongDetailsOfCurrent();
tMusicDB* MMgetMusicDB();
uint8_t MMgetNumberOfPLaylists();
uint8_t MMgetNumberOfTracks();
void MMgetPlaylistItem(uint8_t idx, char *content, uint8_t *len);
void MMgetTracklistItem(uint8_t idx, char *content, uint8_t *len);
void MMSelectItem(uint8_t category, uint32_t item);
void MMResetSelections();
/*
 * logic for musicmanager
 */
void processMusicManager();
const char* MMGetCurrentFilePath();

#endif /* INC_MUSICMANAGER_H_ */
