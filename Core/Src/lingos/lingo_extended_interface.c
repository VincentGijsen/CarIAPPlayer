/*
 * lingo_extended_interface.c
 *
 *  Created on: 10 Apr 2021
 *      Author: vtjgi
 */

#include "lingos.h"
#include "transactionlayer.h"
#include "stdint.h"
#include "log.h"

#include "MusicManager.h"

#define EXTENDED_ACK_CMD 0x01

struct {
	uint8_t type;
	uint32_t start;
	uint32_t count;
} open_query;

struct {
	uint8_t idx[4];
	uint8_t content[50];
}__ALIGN_BEGIN db_record_to_send __ALIGN_END;

struct {
	uint8_t playingOrPause;
	uint32_t trackId;

	uint8_t lastReceivedCommand;
	uint16_t lastTransactionId;
} playbackStatus;

/*
 * pass in first byte as point, to get index back
 */
uint32_t _getIndexFromData(uint8_t *data) {
	return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3]);
}

void processLingoExtendedInterface(IAPmsg msg) {

	switch (msg.commandId) {

	case 0x0c: //GetIndexedPlayingTrackInfo
	{
		uint8_t trackInfoType = msg.payload[0];
		uint32_t trackIdx = _getIndexFromData(&(msg.payload[1]));

		uint16_t chapter = (msg.payload[5] << 8) | (msg.payload[6]);

		const uint8_t ReturnIndexedPlayingTrackInfo = 0xd;
		initResponse(LingoExtended, ReturnIndexedPlayingTrackInfo, msg.transID);

		LOG("trackInfoRequested: %x\n", trackInfoType);
		/*
		 * TODO: plug in meta shizzle
		 */
		uint8_t payload[30] = { 0 };
		uint8_t sendLengthPayload = 0;

		switch (trackInfoType) {
		case 0x00: //capabilities
		{
			payload[0] = trackInfoType;
			payload[1] = 0;
			payload[2] = 0;
			payload[3] = 0;
			payload[4] = 0;
			payload[5] =
					(MMgetSongDetailsOfCurrent()->timeDurationTotalMs >> 24);
			payload[6] =
					(MMgetSongDetailsOfCurrent()->timeDurationTotalMs >> 16);
			payload[7] =
					(MMgetSongDetailsOfCurrent()->timeDurationTotalMs >> 8);
			payload[8] = (MMgetSongDetailsOfCurrent()->timeDurationTotalMs
					& 0x00);
			payload[9] =0;
			payload[10] = 0;
			sendLengthPayload = 11;
			addResponsePayload(&payload, sizeof(sendLengthPayload));
		}
			break;

		case 0x06: // trac composer
		{
			uint8_t empty[2] = { trackInfoType, 0 };
			addResponsePayload(&empty, sizeof(empty));
		}
			break;

		case 0x07: //track artwork count
		{
			uint8_t empty[5] = { trackInfoType, 0, 0, 0, 0 };
			addResponsePayload(&empty, sizeof(empty));

		}
			break;

		case 0x04: //lyrics
		{
			uint8_t empty[5] = { 0 };
			addResponsePayload(&empty, sizeof(empty));

		}
			break;

		case 0x01: //podcastname
		case 0x02: //track release date
		case 0x05: //track genre

		case 0x03: //description

		default:
			LOG("unsure how to proceed\n");
		}

		transmitToAcc();
	}
		break;

	case 0x0e: //GetArtworkFormats
	{
		const uint8_t RetArtworkFormats = 0x0f;
		initResponse(LingoExtended, RetArtworkFormats, msg.transID);
		const uint8_t pixel_format = 0x02;
		uint8_t artwork[] = { 0, 1, pixel_format, 0, 16, 0, 16 };
		addResponsePayload(&artwork, sizeof(artwork));
		transmitToAcc();

		LOG("GetArtWorkFormats to be implemented\n");
	}
		break;

	case 0x16: //ResetDBSelection
	{
		LOG("ResetDB\n");
		const cmdToAck = 0x16;
		const uint8_t success = { 0x00, 0x00, cmdToAck };

		//USBD_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
		initResponse(LingoExtended, EXTENDED_ACK_CMD, msg.transID);
		addResponsePayload(&success, sizeof(success));
		transmitToAcc();

		MMResetSelections();

		//kickoff Music Manager
		MMInit();
	}
		break;

	case 0x17: //SelectDBRecord
	{
		LOG("selectDBRecords \n");
		uint8_t dbCategoryRequested = msg.payload[0];
		int32_t dbRecordIdx = _getIndexFromData(&(msg.payload[1]));
		/*(msg.payload[1] << 24) | (msg.payload[2] << 16)
		 | (msg.payload[3] << 8) | (msg.payload[4]);
		 */
		MMSelectItem(dbCategoryRequested, dbRecordIdx);
		//acc selected record
		const cmdToAck = 0x17;
		const uint8_t success = { 0x00, 0x00, cmdToAck };
		initResponse(LingoExtended, EXTENDED_ACK_CMD, msg.transID);
		addResponsePayload(&success, sizeof(success));
		transmitToAcc();

	}
		break;

	case 0x18: //GetNumberCategorizedDBRecords
	{
		uint8_t catReq = msg.payload[0];
		LOG("GetNumberCategorizedDBRecords cat: %d\n", catReq);

		const uint8_t returnNumberCategorizedDBRecordsCmd = 0x19;
		initResponse(LingoExtended, returnNumberCategorizedDBRecordsCmd,
				msg.transID);

		uint8_t numberOfRecords[4];
		uint32_t numberOfTracks = 0;

		switch (catReq) {
		case DB_CAT_PLAYLIST:
			numberOfTracks = MMgetNumberOfPLaylists();
			break;

		case DB_CAT_TRACK:
			numberOfTracks = MMgetNumberOfTracks();
		default:
			//to implement
			;
		}
		numberOfRecords[0] = numberOfTracks >> 24;
		numberOfRecords[1] = numberOfTracks >> 16;
		numberOfRecords[2] = numberOfTracks >> 8;
		numberOfRecords[3] = numberOfTracks & 0xff;

		addResponsePayload(&numberOfRecords, sizeof(numberOfRecords));

		transmitToAcc();

	}
		break;

	case 0x1a: //RetrieveCategorizedDatabaseRecords
	{
		uint8_t dbCatType = msg.payload[0];
		uint32_t dbRecordStart = _getIndexFromData(&(msg.payload[1]));

		uint32_t dbRecordCount = _getIndexFromData(&(msg.payload[5]));

		open_query.type = dbCatType;
		open_query.start = dbRecordStart;
		open_query.count = dbRecordCount;
		LOG("requested record type: %d | s: %d | cnt: %d\n", dbCatType,
				dbRecordStart, dbRecordCount);

		typeDefTask task;
		task.f = &taskProcessQuery;
		task.scheduledAfter = uwTick + 10;
		scheduleTasks(task);
	}
		break;

	case 0x1C: //GetPlayStatus
	{
		LOG("Get play status\n");
		const uint8_t ReturnPlayStatus = 0x1d;
		initResponse(LingoExtended, ReturnPlayStatus, msg.transID);

		uint8_t res[9]; // = { 0xFf, 0xff, 0xff, 0xff };
		res[0] = (MMgetSongDetailsOfCurrent()->timeDurationTotalMs >> 24);
		res[1] = (MMgetSongDetailsOfCurrent()->timeDurationTotalMs >> 16);
		res[2] = (MMgetSongDetailsOfCurrent()->timeDurationTotalMs >> 8);
		res[3] = (MMgetSongDetailsOfCurrent()->timeDurationTotalMs & 0xff);

		res[4] = (MMgetSongDetailsOfCurrent()->timePositionMs >> 24);
		res[5] = (MMgetSongDetailsOfCurrent()->timePositionMs >> 16);
		res[6] = (MMgetSongDetailsOfCurrent()->timePositionMs >> 8);
		res[7] = (MMgetSongDetailsOfCurrent()->timePositionMs & 0xff);

		//todo: get from MusicManager
		//const uint8_t isPlaying = 0x01;

		if (!MMIsPaused())
			res[8] = (MMIsPlaying() << 0) | (MMisStopped() << 2);
		else {
			res[8] = 0; //paused
		}
		addResponsePayload(&res, sizeof(res));
		transmitToAcc();

	}
		break;

	case 0x1e: //GetCurrentPlayingTrackIndex
	{
		LOG("GetCurrentPlayingTrackIndex\n");
		const uint8_t ReturnCurrentPlayingTrackIndexCmd = 0x1f;
		initResponse(LingoExtended, ReturnCurrentPlayingTrackIndexCmd,
				msg.transID);

		uint8_t res[4] = { 0xFf, 0xff, 0xff, 0xff }; //initialize as 'nothing is playing'

		if (MMIsPaused() || MMIsPlaying()) {
			res[0] = (MMgetSongDetailsOfCurrent()->trackIdx >> 24);
			res[1] = (MMgetSongDetailsOfCurrent()->trackIdx >> 16);
			res[2] = (MMgetSongDetailsOfCurrent()->trackIdx >> 8);
			res[3] = (MMgetSongDetailsOfCurrent()->trackIdx & 0xff);
			LOG("track IDX %d playing/paused \n",
					MMgetSongDetailsOfCurrent()->trackIdx);

		} else {
			//init value is correct for stopped state
			//default init implements -1 (signed)
		}

		addResponsePayload(&res, sizeof(res));
		transmitToAcc();
	}
		break;

	case 0x22: //GetIndexedTrackArtistName
	{
		LOG("GetIndexedTrackArtistName\n");
		const uint8_t ReturnIndexedPlayingTrackArtistName = 0x23;
		uint16_t reqItemIdx = _getIndexFromData(&(msg.payload[0]));
		/*msg.payload[0] << 24 | msg.payload[1] << 16
		 | msg.payload[3] << 8 | msg.payload[3];
		 */
		//assume alsways ok;
		initResponse(LingoExtended, ReturnIndexedPlayingTrackArtistName,
				msg.transID);
		//get payload; and 0-termiante

		uint8_t res[] = "TODOArtistName";
		addResponsePayload(&res, sizeof(res));
		transmitToAcc();
	}
		break;

	case 0x24: //GetIndexedTrackAlbumName
	{
		LOG("GetIndexedTrackAlbumName\n");
		const uint8_t ReturnIndexedPlayingAlbumName = 0x25;
		uint16_t reqItemIdx = _getIndexFromData(&(msg.payload[0]));/*msg.payload[0] << 24 | msg.payload[1] << 16
		 | msg.payload[3] << 8 | msg.payload[3];
		 */

		//assume alsways ok;
		initResponse(LingoExtended, ReturnIndexedPlayingAlbumName, msg.transID);
		//get payload; and 0-termiante

		uint8_t res[] = "TODOAlbumName";
		addResponsePayload(&res, sizeof(res));
		transmitToAcc();
	}
		break;

	case 0x26: //setPlayStatusChangeNotification
	{
		LOG("subscribe play-event-updates\n");

		const uint8_t success = 0x00;

		//USBD_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
		initResponse(LingoExtended, EXTENDED_ACK_CMD, msg.transID);
		addResponsePayload(&success, sizeof(success));
		transmitToAcc();
	}

		break;

	case 0x28: //PlayCurrentSelection [@deprecated]
	{
		const cmdToAck = 0x28;
		const uint8_t success = { 0x00, 0x00, cmdToAck };

		uint32_t idx = _getIndexFromData(&(msg.payload[0]));

		//set PLaying track:

		LOG("Play current Selection..idx: %d\n", idx);
		//USBD_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
		initResponse(LingoExtended, EXTENDED_ACK_CMD, msg.transID);
		addResponsePayload(&success, sizeof(success));
		transmitToAcc();

	}
		break;

	case 0x29: //PlayControl
	{
		uint8_t playCMD = msg.payload[0];
		playbackStatus.lastReceivedCommand = playCMD;
		playbackStatus.lastTransactionId = msg.transID;

		const playControlCMD = 0x29;
		const uint8_t success = { 0x00, 0x00, playControlCMD };

		//USBD_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
		initResponse(LingoExtended, EXTENDED_ACK_CMD,
				playbackStatus.lastTransactionId);
		addResponsePayload(&success, sizeof(success));
		transmitToAcc();

		typeDefTask task;
		task.f = &taskPlayControlCommand;
		task.scheduledAfter = uwTick + 1;
		scheduleTasks(task);

	}
		break;

	case 0x2e: //SetShuffle
	{
		LOG("set shuffle\n");
		uint8_t newShuffleState = msg.payload[0];
		uint8_t setOnRestore = msg.payload[1];

	}
		break;

	case 0x2f: //GetRepeat
	{
		const uint8_t ReturnRepeatCMD = 0x30;
		const uint8_t repeatTracks = 0x00;
		LOG("get repeat\n");

		//USBD_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
		initResponse(LingoExtended, ReturnRepeatCMD, msg.transID);
		addResponsePayload(&repeatTracks, sizeof(repeatTracks));
		transmitToAcc();
	}
		break;

	case 0x2c: //GetShuffle
	{
		const uint8_t ReturnShuffleCMD = 0x2D;
		const uint8_t shuffleTracks = 0x01;

		//USBD_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
		initResponse(LingoExtended, ReturnShuffleCMD, msg.transID);
		addResponsePayload(&shuffleTracks, sizeof(shuffleTracks));
		transmitToAcc();

		LOG("bogus track-change on track-reading\n");
		typeDefTask task;
		task.f = &taskTrackChanged;
		task.scheduledAfter = uwTick + 2000;
		scheduleTasks(task);
	}

		break;

	case 0x31: //Set Repeat
	{
		LOG("set repeat\n");
		uint8_t newRepeateState = msg.payload[0];
		uint8_t setOnRestore = msg.payload[1];
	}
		break;

	case 0x3b: //ResetDBSelectionHierarchy
	{
		LOG("ResetDBSelectionHierarchy \n");
		LOG("TODO: IMPLEMENT\n");
		if (msg.payload[0] == 0x01) {
			//req for audio db
		} else {
			//reg for video-db
		}
	}
		break;

	case 0x35: //GetNumPlayingTracks
	{
		//view tracks in ithin the 'now playing list'
		const uint8_t ReturnNumPlayingTracks = 0x36;
		uint8_t res[4] = { 0xFf, 0xff, 0xff, 0xff };

		LOG("GetNumPlayingTracks\n");
		LOG("TODO: IMPLEMENT\n");
		//todo
		uint8_t static_number_of_songs_queued = 99;

		res[0] = (static_number_of_songs_queued >> 24);
		res[1] = (static_number_of_songs_queued >> 16);
		res[2] = (static_number_of_songs_queued >> 8);
		res[3] = (static_number_of_songs_queued & 0xff);

		initResponse(LingoExtended, ReturnNumPlayingTracks, msg.transID);
		addResponsePayload(&res, sizeof(res));
		transmitToAcc();

	}
		break;

	case 0x37: //SetCurrentPlayingTrack
	{
		//jumps into now-playing list;
		LOG("SetCurrentPlayingTrack\n");
		LOG("TODO: IMPLEMENT\n");

		const cmdToAck = 0x37;

		initResponse(LingoExtended, EXTENDED_ACK_CMD, msg.transID);
		const uint8_t success = { 0x00, 0x00, cmdToAck };
		addResponsePayload(&success, sizeof(success));
		transmitToAcc();
		//return ACK
	}
		break;

	default:
		LOG("Ext Lingo cmd: %x not implm.\n", msg.commandId);
	}
}

void prepareRecord(uint32_t id, uint8_t *val, uint8_t len) {

	db_record_to_send.idx[0] = (id >> 24);
	db_record_to_send.idx[1] = (id >> 16);
	db_record_to_send.idx[2] = (id >> 8);
	db_record_to_send.idx[3] = (id & 0xff);

	uint8_t idx = 0;
	while (idx < 49) {
		db_record_to_send.content[idx] = val[idx];
		if ((idx + 1) == len || idx == 48) {
			//last byte must be null terminated
			db_record_to_send.content[idx + 1] = 0x00;
			break;
		}
		idx++;
	}
}

void taskProcessQuery() {
	if (open_query.count > 0) {

		//prepare requestd record(s)
		const uint8_t returnCategorizedDatabaseRecordCmd = 0x1b;

		podTransactionCounter += +1;
		initResponse(LingoExtended, returnCategorizedDatabaseRecordCmd,
				podTransactionCounter);

		uint8_t content[50];
		uint8_t len = 0;

		switch (open_query.type) {
		case DB_CAT_TOP: {
			prepareRecord(open_query.start, "TOP", 3);

		}

			break;

		case DB_CAT_PLAYLIST: {
			MMgetPlaylistItem(open_query.start, &content, &len);
			prepareRecord(open_query.start, &content, len);
			{

			}
		}
			break;

		case DB_CAT_ALBUM: {
			prepareRecord(open_query.start, "Beatles", 7);
		}
			break;

		case DB_CAT_ARTIST: {
			prepareRecord(open_query.start, "Vincent", 7);

		}
			break;

		case DB_CAT_TRACK: {
			//prepareRecord(open_query.start, "Track something", 15);

			MMgetTracklistItem(open_query.start, &content, &len);
			//MMgetPlaylistItem(open_query.start, &content, &len);
			prepareRecord(open_query.start, &content, len);
			{

			}

		}
			break;

		default:
			LOG("query cat type not implemented \n");
			return;
		}
		//LOG("sending record\n");
		addResponsePayload(&db_record_to_send, sizeof(db_record_to_send));
		transmitToAcc();

		//update remainders
		open_query.start++;
		open_query.count--;

		if (open_query.count > 0) {
			typeDefTask task;
			task.f = &taskProcessQuery;
			task.scheduledAfter = uwTick + 1;
			scheduleTasks(task);
		}

	}
}

void taskPlayControlCommand() {
	LOG("handeling playcontrol req\n");
	uint8_t trackCHangeAction = 0;
	switch (playbackStatus.lastReceivedCommand) {

	case 0x01: //toggle pay pause

		break;

	case 0x02: //stop
		break;

	case 0x05: //startFF
		break;
	case 0x06: //start REW
		break;
	case 0x07: //end ff / rew
		break;
	case 0x08: //next
		MMSendEvent(MMNext);
		trackCHangeAction = 1;
		break;
	case 0x09: //prev
		MMSendEvent(MMPrevious);
		trackCHangeAction = 1;
		break;
	case 0x1a: //play

		trackCHangeAction = 1;

		break;
	case 0x1b: //pause
		break;
	default:
		//not implemented stufs;
		;
	}

	if (trackCHangeAction) {
		typeDefTask task;
		task.f = &taskTrackChanged;
		task.scheduledAfter = uwTick + 1;
		scheduleTasks(task);
	}

}

void taskTrackChanged() {
	LOG("TrackChanged\n");
	//do something
	const uint8_t TrackNewAudioAttributes = 0x04;
	uint8_t payload[12] = { 0 };
	//set samplrate on first 4 bytes
	payload[0] = (48000 >> 24);
	payload[1] = (48000 >> 16);
	payload[2] = (48000 >> 8);
	payload[3] = (48000 & 0xff);
	//soundcheck volume/

	podTransactionCounter++;
	initResponse(LingoDigitalAudio, TrackNewAudioAttributes,
			podTransactionCounter);

	addResponsePayload(&payload, sizeof(payload));
	transmitToAcc();

}
