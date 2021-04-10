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

struct {
	uint8_t type;
	uint32_t start;
	uint32_t count;
} open_query;

struct {
	uint8_t idx[4];
	uint8_t content[50];
}__ALIGN_BEGIN db_record_to_send __ALIGN_END;

void processLingoExtendedInterface(IAPmsg msg) {
	xprintf("||ExtendedLingo LINGO :)\n");

	switch (msg.commandId) {
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

	case 0x18: //GetNumberCategorizedDBRecords
	{
		uint8_t dbCategoryRequested = msg.raw[9];
		xprintf("requested category: %d\n", dbCategoryRequested);

		uint8_t numberOfRecords[] = { 0, 0, 0, 1 };
		const uint8_t returnNumberCategorizedDBRecordsCmd = 0x19;
		initResponse(LingoExtended, returnNumberCategorizedDBRecordsCmd,
				msg.transID);

		addResponsePayload(&numberOfRecords, sizeof(numberOfRecords));
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
		if ((idx+1) == len || idx == 48) {
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

		case DB_CAT_ARTIST: {
			prepareRecord(open_query.start, "Artist", 6);

		}
			break;

		case DB_CAT_TRACK: {
			prepareRecord(open_query.start, "Track something", 15);

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
