/*
 * MusicManager.h
 *
 *  Created on: 12 Apr 2021
 *      Author: vtjgi
 */

#ifndef INC_MUSICMANAGER_H_
#define INC_MUSICMANAGER_H_

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



/*
 * codec stuf
 */

uint8_t WavIsPlaying();
void WavPlayNonBlocking() ;
void WavPlayFile();
void WavStop() ;

/*
 * interfaces for other parts
 */
void MMSendEvent(MMplayserState evt);
void MMInit();

/*
 * logic for musicmanager
 */
void processMusicManager();
const char* MMGetCurrentFilePath();

#endif /* INC_MUSICMANAGER_H_ */
