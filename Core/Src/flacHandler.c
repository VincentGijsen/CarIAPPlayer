//#include "freertos.h"
#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"
#include "ff.h"
#include "xprintf.h"

#include "flacHandler.h"

/*
 * DRflac settings
 */
void* my_malloc(size_t sz, void *pUserData) {
	return malloc(sz);
	//return pvPortMalloc(sz);
}
void* my_realloc(void *p, size_t sz, void *pUserData) {
	//return realloc(p, sz);
	//return pv
}
void my_free(void *p, void *pUserData) {
	free(p);
	//vPortFree(p);
}

/*global vars
 *
 */

static drflac_allocation_callbacks allocationCallbacks;
static uint8_t drFlac_allocationConfigured = 0;

static appData myData;
__attribute__((section("CCMRAM")));
static SongData songData;
__attribute__((section("ccmram")));
static SongStreamProperties songStreamProperties __attribute__((section("ccmram")));
;

static volatile drflac_int16 flacPCMBuffer[96 * 1] __attribute__((section("ccmram")));
;

void _setupDr_flac() {
	allocationCallbacks.pUserData = &myData;
	allocationCallbacks.onMalloc = my_malloc;
	//Passing in null for the allocation callbacks object will cause dr_flac to use defaults which is the same as DRFLAC_MALLOC,
	allocationCallbacks.onRealloc = NULL;
	allocationCallbacks.onFree = my_free;
	//drflac *pFlac = drflac_open_file("my_file.flac", &allocationCallbacks);
}

//dirty implemenation
//no checking for non-happy flow
//returns 1 if equal
uint8_t startsWith(const char *s1, const char *s2, uint8_t len) {
	for (uint8_t x = 0; x < len; x++) {
		if (s1[x] != s2[x])
			return 0;
	}
	return 1;

}
//drflac_meta_proc
//The callback to fire for each metadata block.
void _drflac_onMeta(void *pUserData, drflac_metadata *pMetadata) {

	uint32_t type = pMetadata->type;
	switch (type) {
	case DRFLAC_METADATA_BLOCK_TYPE_STREAMINFO: {
		//handle properties of stream, for buffer sizes etc.
		//SongStreamProperties
		songStreamProperties.bits = pMetadata->data.streaminfo.bitsPerSample;
		songStreamProperties.channels = pMetadata->data.streaminfo.channels;
		songStreamProperties.freq = pMetadata->data.streaminfo.sampleRate;
	}
		break;

	case DRFLAC_METADATA_BLOCK_TYPE_VORBIS_COMMENT:
//tagging stufs
	{
		drflac_vorbis_comment_iterator pIt;
		drflac_init_vorbis_comment_iterator(&pIt,
				pMetadata->data.vorbis_comment.commentCount,
				pMetadata->data.vorbis_comment.pComments);

		for (int x = 0; x < pMetadata->data.vorbis_comment.commentCount; x++) {
			drflac_uint32 pCommentLengthOut;
			//used for extracting info
			uint8_t maxTmpStrLen = MAX_META_STR_LEN + 7;
			uint8_t contentStrLeng = 0;
			const char *s = drflac_next_vorbis_comment(&pIt,
					&pCommentLengthOut);

			char temp[MAX_META_STR_LEN + 7];

			//cap length of target to max size
			if (pCommentLengthOut > maxTmpStrLen - 1) {
				pCommentLengthOut = maxTmpStrLen - 1;
			}
			memcpy(&temp, s, pCommentLengthOut);
			//add string terminator to final 'store'
			temp[pCommentLengthOut - 1] = '\0';

			xprintf("meta: %s\n", temp);

#define ALBUM_STR_VAL "ALBUM="
#define ALBUM_STR_LEN sizeof(ALBUM_STR_VAL) -1 //don't include \0

#define TITLE_STR_VAL "TITLE="
#define TITLE_STR_LEN sizeof(ALBUM_STR_VAL) -1 //don't include \0

#define ARTIST_STR_VAL "ARTIST="
#define ARTIST_STR_LEN sizeof(ALBUM_STR_VAL) -1 //don't include \0

			if (startsWith(&temp[0], ALBUM_STR_VAL, ALBUM_STR_LEN)) {

				//start copieing from after key value
				contentStrLeng = pCommentLengthOut - ALBUM_STR_LEN;
				if (contentStrLeng >= contentStrLeng)
					contentStrLeng = contentStrLeng - 1;
				memcpy(songData.album, &temp[ALBUM_STR_LEN], contentStrLeng);
				songData.album[contentStrLeng - 1] = '\0';

				songData.fieldsInited++;

			} else if (startsWith(&temp[0], TITLE_STR_VAL, TITLE_STR_LEN)) {
//TODO: implement
				//songData.title = malloc(pCommentLengthOut + 1 - ALBUM_STR_LEN);
				songData.fieldsInited++;
			} else if (startsWith(&temp[0], ARTIST_STR_VAL, ARTIST_STR_LEN)) {
//TODO: implement
				//songData.artists = malloc(
				//pCommentLengthOut + 1 - ARTIST_STR_LEN
				//);
				songData.fieldsInited++;
			}
		}
	}
		break;

	case DRFLAC_METADATA_BLOCK_TYPE_PICTURE:
//image stuffs
		break;

	default:
		break;
	}

//set frames to read somewhere
	myData.framesToRead = 88; //arbitrary number ATm!

}

size_t _drflac_onRead(void *pUserData, void *pBufferOut, size_t bytesToRead) {
	UINT bytes_read;
	FRESULT rc = f_read(myData.currentFile, pBufferOut, (UINT) bytesToRead,
			&bytes_read);

	if (rc != FR_OK) {
		xprintf("ERROR: f_read failed\n");
		return -1;
	}

	return (int) bytes_read;
}

size_t _drflac_onSeek(void *pUserData, int offset, drflac_seek_origin origin) {
//TODO: implement
	switch (origin) {
	case drflac_seek_origin_current: {
		FRESULT r = f_lseek(myData.currentFile,
		f_tell(myData.currentFile) + offset);
		if (r != FR_OK) {
			return 0;
		}
	}
		break;

	case drflac_seek_origin_start: {
		FRESULT r = f_lseek(myData.currentFile, offset);
		if (r != FR_OK) {
			return 0;
		}
		break;
	}

	}
	//seek ok
	return 1;
}

void drFlac_stop() {
//clean up

	//free(songData.title);
	//free(songData.album);
	//free(songData.artists);
	songData.fieldsInited = 0;

	free(myData.currentFile);
}

/*
 * @returns FLAC_DECODER_READY if successfully parsed headers and ready to process PCM
 */
FLAC_HANDLER_STATUS drFlac_play(FIL *file) {
	if (!drFlac_allocationConfigured) {
		_setupDr_flac();
		drFlac_allocationConfigured = 1;
	}
	drFlac_stop();

	//reset decoder stufs;
	myData.currentFile = file;
	myData.aditional_frames_on_interval = 0;
	myData.framecnt = 0;
	myData.interval_for_extra_frames = 0;
	myData.bytes_per_sample = 0;

//TODO: implemeent stop/clear check before start

	myData.pFlac = drflac_open_with_metadata_relaxed(_drflac_onRead,
			_drflac_onSeek, _drflac_onMeta, drflac_container_native, &myData,
			&allocationCallbacks);

	if (myData.pFlac) {
		return FLAC_DECODER_READY;
	} else {

	}
	return FLAC_DECODER_ERROR;

}

FLAC_PCM_STATUS drFlac_updatePCMBatch() {

	switch (songStreamProperties.freq) {
	case FLAC_HZ_44100:
		myData.framesToRead = USB_SAMPLES_AT_44100RATE_A_MS;
		myData.aditional_frames_on_interval =
		USB_SAMPLES_AT_44100RATE_EXTRA_SAMPLE_INTERVAL;
		myData.interval_for_extra_frames =
		USB_SAMPLES_AT_44100RATE_EXTRA_TO_INCLUDE;
		myData.bytes_per_sample = USB_SAMPLES_AT_44100_AT_16SINT_OF_BYTES;
		break;

	case FLAC_HZ_48000:
		myData.framesToRead = USB_SAMPLES_AT_48000RATE_A_MS;
		myData.aditional_frames_on_interval =
		USB_SAMPLES_AT_48000RATE_EXTRA_SAMPLE_INTERVAL;
		myData.interval_for_extra_frames =
		USB_SAMPLES_AT_48000RATE_EXTRA_TO_INCLUDE;
		myData.bytes_per_sample = USB_SAMPLES_AT_48000_AT_16SINT_OF_BYTES;
		break;

	default:
		//error:
		return FLAC_PCM_STATUS_ERROR;
		;
	}
	uint8_t frames_to_get = myData.framesToRead;

	//additional frame logic
	if (myData.interval_for_extra_frames > 0) {
		myData.framecnt++;

		if (myData.framecnt == myData.interval_for_extra_frames
				&& (myData.framecnt > 0)) {
			myData.framecnt = 1; //carefull, 0 is reserved for disabled;
			frames_to_get += myData.aditional_frames_on_interval;
		}
	}
	uint16_t samplesProvided = drflac_read_pcm_frames_s16(myData.pFlac,
			frames_to_get, flacPCMBuffer);

	//check result of decoding run
	if (samplesProvided == 0) {
		return FLAC_PCM_STATUS_ERROR;
	} else if (samplesProvided < frames_to_get) {

		//set rest of buffer to zero
		memset(flacPCMBuffer[samplesProvided], 0,
				(frames_to_get - samplesProvided));
//setnd FLAC_PCM_STATUS_PARTIAL
	} else {
		//all samples filled

	}

	putBuffer(&flacPCMBuffer, frames_to_get, myData.bytes_per_sample);
	return FLAC_PCM_STATUS_FILLED;
	//}

//other bit stuff not imlemented
	//return FLAC_PCM_STATUS_ERROR;

}
