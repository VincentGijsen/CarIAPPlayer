/*
 * lingo_extended_interface.c
 *
 *  Created on: 10 Apr 2021
 *      Author: vtjgi
 */

#include "lingos.h"
#include "transactionlayer.h"
#include "stdint.h"
#include "xprintf.h"

#define EXTENDED_ACK_CMD 0x01

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

void processLingoExtendedInterface(IAPmsg msg) {

	switch (msg.commandId) {

	case 0x0c: //GetIndexedPlayingTrackInfo
	{
		uint8_t trackInfoType = msg.raw[9];
		uint32_t trackIdx = (msg.raw[10] << 24) | (msg.raw[11] << 16)
				| (msg.raw[12] << 8) | (msg.raw[13]);
		uint16_t chapter = (msg.raw[14] << 8) | (msg.raw[15]);

		const uint8_t ReturnIndexedPlayingTrackInfo = 0xd;
		initResponse(LingoExtended, ReturnIndexedPlayingTrackInfo, msg.transID);

		switch (trackInfoType) {
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x05:
		case 0x06:
		case 0x07: {
			uint8_t empty[1] = { 0 };
			addResponsePayload(&empty, sizeof(empty));

		}
			break;

		case 0x03:
		case 0x04: {
			uint8_t empty[5] = { 0 };
			addResponsePayload(&empty, sizeof(empty));

		}
		}
		transmitToAcc();
	}
		break;

	case 0x0e: //GetArtworkFormats
	{
		xprintf("GetArtWorkFormats to be implemented\n");
	}
		break;

	case 0x16: //ResetDBSelection
	{
		xprintf("ResetDB\n");
		const cmdToAck = 0x16;
		const uint8_t success = { 0x00, 0x00, cmdToAck };

		//USBD_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
		initResponse(LingoExtended, EXTENDED_ACK_CMD, msg.transID);
		addResponsePayload(&success, sizeof(success));
		transmitToAcc();

	}
		break;

	case 0x17: //SelectDBRecord
	{
		xprintf("selectDBRecords \n");
		uint8_t dbCategoryRequested = msg.raw[9];
		int32_t dbRecordIdx = (msg.raw[10] << 24) | (msg.raw[11] << 16)
				| (msg.raw[12] << 8) | (msg.raw[13]);

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
		uint8_t dbCategoryRequested = msg.raw[9];
		xprintf("requested category: %d\n", dbCategoryRequested);

		const uint8_t returnNumberCategorizedDBRecordsCmd = 0x19;
		initResponse(LingoExtended, returnNumberCategorizedDBRecordsCmd,
				msg.transID);

		switch (dbCategoryRequested) {
		case DB_CAT_TOP:
		case DB_CAT_ALBUM:
		case DB_CAT_ARTIST:
		case DB_CAT_TRACK:
		{
			uint8_t numberOfRecords[] = { 0, 0, 0, 1 };
			addResponsePayload(&numberOfRecords, sizeof(numberOfRecords));

		}
			break;

		default: {
			uint8_t numberOfRecords[] = { 0, 0, 0, 0 };
			addResponsePayload(&numberOfRecords, sizeof(numberOfRecords));
		}

		}

		transmitToAcc();

	}
		break;

	case 0x1a: //RetrieveCategorizedDatabaseRecords
	{
		uint8_t dbCatType = msg.raw[9];
		uint32_t dbRecordStart = (msg.raw[10] << 24) | (msg.raw[11] << 16)
				| (msg.raw[12] << 8) | (msg.raw[13]);
		uint32_t dbRecordCount = (msg.raw[14] << 24) | (msg.raw[15] << 16)
				| (msg.raw[16] << 8) | (msg.raw[17]);

		open_query.type = dbCatType;
		open_query.start = dbRecordStart;
		open_query.count = dbRecordCount;
		xprintf("ass requested record type: %d | start: %d | count: %d\n",
				dbCatType, dbRecordStart, dbRecordCount);

		typeDefTask task;
		task.f = &taskProcessQuery;
		task.scheduledAfter = uwTick + 10;
		scheduleTasks(task);
	}
		break;

	case 0x1e: //GetCurrentPlayingTrackIndex
	{
		const uint8_t ReturnCurrentPlayingTrackIndexCmd = 0x1f;
		initResponse(LingoExtended, ReturnCurrentPlayingTrackIndexCmd,
				msg.transID);

		uint8_t res[4] = { 0xff, 0xff, 0xff, 0xff };

		if (playbackStatus.playingOrPause) {
			res[0] = (playbackStatus.trackId >> 24);
			res[1] = (playbackStatus.trackId >> 16);
			res[2] = (playbackStatus.trackId >> 8);
			res[3] = (playbackStatus.trackId & 0xff);

		} else {
			//init value is correct for stopped state
		}

		addResponsePayload(&res, sizeof(res));
		transmitToAcc();
	}
		break;

	case 0x3b: //ResetDBSelectionHierarchy
	{
		if (msg.raw[9] == 0x01) {
			//req for audio db
		} else {
			//reg for video-db
		}
	}
		break;

	case 0x26: //setPlayStatusChangeNotification
	{
		xprintf("ass requested play-event-updates\n");

		const uint8_t success = 0x00;

		//USBD_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
		initResponse(LingoExtended, EXTENDED_ACK_CMD, msg.transID);
		addResponsePayload(&success, sizeof(success));
		transmitToAcc();
	}

		break;

	case 0x29: //PlayControl
	{
		uint8_t playCMD = msg.raw[9];
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
	case 0x2c: //GetShuffle
	{
		const uint8_t ReturnShuffleCMD = 0x2D;
		const uint8_t shuffleTracks = 0x01;

		//USBD_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
		initResponse(LingoExtended, ReturnShuffleCMD, msg.transID);
		addResponsePayload(&shuffleTracks, sizeof(shuffleTracks));
		transmitToAcc();
	}

		break;

	case 0x31: //Set Repeat
	{
		uint8_t newRepeateState = msg.raw[9];
		uint8_t setOnRestore = msg.raw[10];
	}
		break;

	case 0x2e: //SetShuffle
	{
		uint8_t newShuffleState = msg.raw[9];
		uint8_t setOnRestore = msg.raw[10];
	}
		break;

		break;

	case 0x2f: //GetRepeat
	{
		const uint8_t ReturnRepeatCMD = 0x30;
		const uint8_t repeatTracks = 0x00;

		//USBD_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
		initResponse(LingoExtended, ReturnRepeatCMD, msg.transID);
		addResponsePayload(&repeatTracks, sizeof(repeatTracks));
		transmitToAcc();
	}
		break;

	default:
		xprintf("Extended Lingo cmd: %x not implemented\n", msg.commandId);
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

		switch (open_query.type) {
		case DB_CAT_TOP: {
			prepareRecord(open_query.start, "TOP", 3);

		}
			break;

		case DB_CAT_ALBUM: {
			prepareRecord(open_query.start, "Beatles", 7);
		}
			break;
		case DB_CAT_PLAYLIST: {
			prepareRecord(open_query.start, "pl1", 7);
		}
			break;

		case DB_CAT_ARTIST: {
			prepareRecord(open_query.start, "Vincent", 7);

		}
			break;

		case DB_CAT_TRACK: {
			prepareRecord(open_query.start, "Track something", 15);
			{
				xprintf("bogus track-change on track-reading\n");
				typeDefTask task;
				task.f = &taskTrackChanged;
				task.scheduledAfter = uwTick + 2000;
				scheduleTasks(task);
			}

		}
			break;

		default:
			xprintf("query cat type not implemented \n");
			return;
		}
		xprintf("sending record to Acc\n");
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
	xprintf("handeling playcontrol req\n");
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
		trackCHangeAction = 1;
		break;
	case 0x09: //prev
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
	xprintf("TrackChanged\n");
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
