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

		uint8_t report[13] = { 0x01, 0x0, 0x55, pkg_len, lingo, cmd, 0x00, 0x01,
				cmdstatus, origCmdID, 0x00 /*checksum spot */, 0, 0 };
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

		uint8_t report[] = { 0x03, 0x0, 0x55, pkg_len, lingo, cmd, 0x00, 0x01,
				maxHi, maxLo, 0x00 /*checksum spot */, 0, 0 };
		//addchecksum byes
		report[10] = calcCRC(&report[3], pkg_len + 1);

		USBD_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
	}
		break;

	case 0x4b: //GetiPodOptionsForLingo:
	{ //return RetiPodOptionsForLingo 0x4C
	  //options are requested for:
		uint8_t reqestedLingoOpts = msg.raw[8];

		uint64_t features;
		if (reqestedLingoOpts == 0) {
			features = (1 << 23);

			//see RetiPodOptionsForLingo p192
		} else if (reqestedLingoOpts == 0x2) {
			//simple remote
			features = (1 << 01); //audio media control
		} else if (reqestedLingoOpts == 0x3) {
			//display_remote
			features = 0;

		} else if (reqestedLingoOpts == 0x04) {
			//extended control
			features = (1 << 06); //enable play control in extended?
		} else if (reqestedLingoOpts == 0x0a) {
			//Digital Audio funct
			features = (1 << 02); //supports SampleRate reporting via  FID token
		} else {
			xprintf("inimplemented request for RetiPodOptionsForLingo %x \n",
					reqestedLingoOpts);
			;
		}

		uint8_t pkg_len = 13;
		uint8_t report[20] = { 0x01, 0x0, 0x55, pkg_len, msg.lingoID, 0x4c,
				0x00, 0x01, reqestedLingoOpts, (features >> (8 * 7)) & 0xff,
				(features >> (8 * 6)) & 0xff, (features >> (8 * 5)) & 0xff,
				(features >> (8 * 4)) & 0xff, (features >> (8 * 3)) & 0xff,
				(features >> (8 * 2)) & 0xff, (features >> (8 * 1)) & 0xff,
				(features >> (8 * 0)) & 0xff, 0x00 /*checksum spot */, 0, 0 };

		report[18] = calcCRC(&report[3], pkg_len + 1);
		USBD_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
	}
		break;

	case 0x3b: //EndIDPS
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

