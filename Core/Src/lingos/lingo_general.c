/*
 * lingo_general.c
 *
 *  Created on: 4 Apr 2021
 *      Author: vtjgi
 */
#include "lingos.h"
#include "stdint.h"

#include "xprintf.h"

typedef enum {
	UNDEF = 0, StartIDPS_INIT, TRANSPORT_RESET, NEGOTIATE1,

} typeDefTransportStatus;

typeDefTransportStatus lingoGeneral_state = UNDEF;

#define LingoGeneral 0x00

void processLingoGeneral(IAPmsg msg) {

	switch (msg.commandId) {

	case 0x38: {
		//startDS
		//p252 MFiAccessoryFirmwareSpecificationR46NoRestriction.
		//STARTIPS process stap
		uint8_t pkg_len = 6; //lingo, cmd, 2(trans) payload(2),
		uint8_t lingo = 0; //general
		uint8_t cmd = 0x02; //ipod ack
		uint8_t cmdstatus = 0x00; //success
		uint8_t origCmdID = 0x38; //origCmdID?

		uint8_t report[13] = { 0x01, 0x0, START_OF_FRAME, pkg_len, lingo, cmd,
				GEN_TRANSACTIONBYTES(msg.transID), cmdstatus, origCmdID,
				0x00 /*checksum spot */, 0, 0 };
		//addchecksum byes
		report[10] = calcCRC(&report[3], pkg_len + 1);

		USBD_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
		xprintf("send IPOD_ACK\n");
		//_transState.lastCmd = UNDEF;
	}
		break;

	case 0x39: //SetFIDTokenValues
	{ //ack with: AckFIDTokenValues 0x03a
		uint8_t numFIDTokenValues = msg.raw[8];
		uint8_t tokenStartIdx = 9;
		for (int x = 0; x < numFIDTokenValues; x++) {
			uint8_t tLeng = msg.raw[tokenStartIdx];
			uint8_t tFidType = msg.raw[tokenStartIdx + 1];
			uint8_t tFidSubType = msg.raw[tokenStartIdx + 2];
			uint8_t tdataStart = msg.raw[tokenStartIdx + 3];

			//prep for next token
			tokenStartIdx += (tLeng + 1);

			if (tFidType == 0x00) {
				if (tFidSubType == 0x00) {
					xprintf("IdentityToken : ");
				} else if (tFidSubType == 0x01) {
					xprintf("AccessoryCapsToken: ");
				} else if (tFidSubType == 0x02) {
					xprintf("AccessoryInfoToken: ");
				}
				if (tFidType == 0x00 && tFidSubType == 0x0e) {
					xprintf("AccessoryDigital-AudioSampleRates-Token\n");
				}

			}
			xprintf("len: %X, type: %X, subType: %x \n", tLeng, tFidType,
					tFidSubType);
		}

		//send acced tokens: AckFIDTokenValues
		//AckFIDTokenValues0x3a

	}
		break;

	case 0x11: //RequestTransport- Acc to Dev {transID:2} 	MaxPayloadSize
	{ //respond 0x12; {transID:2, maxSize:2}

		//MFiAccessoryFirmwareSpecificationR46NoRestriction p94

		uint8_t pkg_len = 6; //lingo, cmd, 2(trans) payload(2),
		uint8_t lingo = 0; //general
		uint8_t cmd = 0x12; //ipod ack
		uint8_t maxHi = 0x00; //success
		uint8_t maxLo = 0x36; //origCmdID?

		uint8_t report[] = { 0x03, 0x0, START_OF_FRAME, pkg_len, lingo, cmd,
				GEN_TRANSACTIONBYTES(msg.transID), maxHi, maxLo,
				0x00 /*checksum spot */, 0, 0 };
		//addchecksum byes
		report[10] = calcCRC(&report[3], pkg_len + 1);

		USBD_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
	}
		break;

	case 0x4b: //GetiPodOptionsForLingo:
	{
		//GetiPodOptionsForLingo
		const uint8_t RetiPodOptionsForLingoCmd = 0x4c;
		uint8_t reqestedLingoOpts = msg.raw[8];
		uint8_t pkg_len = 14;
		uint8_t caps[8] = { 0 };

		switch (reqestedLingoOpts) {
		case 0x00: //lingo 0
		{

		}
			break;
		}

		uint8_t report[21] = { 0x03 /*0*/, HID_LINKCTRL_DONE, START_OF_FRAME,
				pkg_len /*3*/,
				LingoGeneral/*4*/, RetiPodOptionsForLingoCmd/*5*/,
				GEN_TRANSACTIONBYTES(msg.transID)/* 6-7 */,
				reqestedLingoOpts /*8*/,
				caps /*9-16*/, 0 /* 17 chk*/, 0, 0,0 };

		report[17] = calcCRC(&report[3], pkg_len + 1);
		USBD_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
		return;

	}
		break;

	case 0x3b:
		//EndIDPS
	{
		uint8_t accEndIDPSStatus = msg.raw[8];
		switch (accEndIDPSStatus) {
		case 0x00:
			//ok, continue
			xprintf("acc ok, proceed authentication please...\n");
			break;
		case 0x01:
			//request reset ipss
			xprintf("acc requested reset of IDPS\n");
			break;

		default:
			xprintf("issue to connect IDPS\n");
		}

	}
		break;

	default:
		xprintf("unknown cmnd: %x", msg.commandId);
		;
	}

	return;

}

