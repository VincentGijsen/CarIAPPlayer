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
struct {
	uint8_t cmd;
	uint8_t lingo;
	uint16_t transId;
	uint8_t writePtr;
	uint8_t buffer[128];
} response_state;

/*
 * Creates the header. the length is calculated
 */
void initResponse(uint8_t lingo, uint8_t cmd, uint16_t transId) {
	response_state.cmd = cmd;
	response_state.lingo = lingo;
	response_state.transId = transId;
	response_state.writePtr = 0;
	//memset(response_state.buffer, 0x00, 128);
}
void addResponsePayload(uint8_t *bytes, uint8_t len) {

	for (uint8_t idx = 0; idx < len; idx++) {
		response_state.buffer[response_state.writePtr] = bytes[idx];
		response_state.writePtr++;
	}
}

void transmitToAcc() {
	/*
	 * calc length
	 */
	uint8_t outBuf[64] = { 0 };

	const uint8_t USABLE_BYTES = 64;
	uint16_t totalBytes = 3; //hid + linkbyte + start of frame
	uint8_t reportSize = 0;
	if ((response_state.writePtr + 4) <= 252) {
		totalBytes += 5; //length(1x)+lingo+cmd+transId(2byte)
	} else {
		totalBytes += 7; //length(3x)+lingo+cmd+transId(2byte)
	}
	totalBytes += response_state.writePtr;

	uint8_t remaining_Bytes = totalBytes + 1; //add checksum in frame cutting
	uint8_t byteCounterProcessed = 0;
	uint8_t firstFrame = 1;
	uint8_t multiFrame = 0;

	int8_t checksum_calc = 0;
	///
	/// Write Header
	///
	if (firstFrame) {

		if (totalBytes < USABLE_BYTES) {
			//small pkg
			if (totalBytes <= 12) {
				outBuf[0] = 0x01;
				reportSize = 13;
			} else if (totalBytes <= 14) {
				outBuf[0] = 0x02;
				reportSize = 15;
			} else if (totalBytes <= 20) {
				outBuf[0] = 0x03;
				reportSize = 21;
			} else if (totalBytes <= 63) {
				outBuf[0] = 0x04;
				reportSize = 64;
			}
			outBuf[1] = HID_LINKCTRL_DONE;
		} else {
			//the frame doesnt fit completely:
			outBuf[0] = 0x04;
			outBuf[1] = HID_LINKCTRL_MORE_TO_FOLLOW; //start of frame, more to come
			reportSize = 64;

			multiFrame = 1;
		}

		outBuf[2] = START_OF_FRAME;
		outBuf[3] = response_state.writePtr + 4; //payload length + lingo+cmd+transactionbytes
		outBuf[4] = response_state.lingo;
		outBuf[5] = response_state.cmd;
		outBuf[6] = (uint8_t) ((response_state.transId >> 8) & 0xff); //msb
		outBuf[7] = (uint8_t) ((response_state.transId) & 0xff); //lsb

		//crc calc
		for (uint8_t i = 3; i <= 7; i++) {
			checksum_calc += outBuf[i];
		}

		if (!multiFrame) {
			//write payload
			for (uint8_t idx = 0; idx < response_state.writePtr; idx++) {
				outBuf[8 + idx] = response_state.buffer[idx];
				checksum_calc += response_state.buffer[idx];
			}

			//ad checksum as last:
			outBuf[8 + response_state.writePtr] = (-checksum_calc);

		} else {
			remaining_Bytes -= 8; //decrement for first
			byteCounterProcessed = 8;
			//in multi frame, write full set
			//fill up the frame
			for (uint8_t idx = 0;
					(idx < response_state.writePtr && (8 + idx) < reportSize);
					idx++) {
				outBuf[8 + idx] = response_state.buffer[idx];
				checksum_calc += response_state.buffer[idx];
				byteCounterProcessed++;
				remaining_Bytes--;
			}
			firstFrame = 0;

		}

		//send first frame out
		USBD_HID_SendReport(&hUsbDeviceFS, outBuf, reportSize);

		//process rest of multi-frame;
		while (remaining_Bytes > 0) {
			reportSize = 64;
			//clear out buffer
			memset(outBuf, 0, USABLE_BYTES);

			if (remaining_Bytes < USABLE_BYTES) {
				// done with the extended pkg
				outBuf[0] = 0x04; //just get biggest one, not so great...
				outBuf[1] = HID_LINKCTRL_CONTINUE;
			} else {
				//full 64byte payload
				outBuf[0] = 0x04; //just get biggest one, not so great...
				outBuf[1] = HID_LINKCTRL_ONLY_DATA;
			}

			uint8_t idx = 0;
			while (remaining_Bytes > 0) {
				outBuf[2 + idx] = response_state.buffer[idx];
				checksum_calc += response_state.buffer[idx];
				byteCounterProcessed++;
				idx++;
				remaining_Bytes--;
				if (idx == (USABLE_BYTES - 2)) {
					break;
				}
			}

			if (remaining_Bytes == 0x00) {
				//last byte is processed, put crc to it, idx points to slot in this frame
				outBuf[idx] = (-checksum_calc);
			}
			//send out this frame:
			USBD_HID_SendReport(&hUsbDeviceFS, outBuf, reportSize);
		}

	} else {
		xprintf("PKG len to big!NOT IMPLEMENTED YET!\n");
	}

	;
}

void processLingoGeneral(IAPmsg msg) {

	switch (msg.commandId) {

	case 0x38: { //startDS
		//p252 MFiAccessoryFirmwareSpecificationR46NoRestriction.
		//STARTIPS process stap
		uint8_t pkg_len = 6; //lingo, cmd, 2(trans) payload(2),
		uint8_t lingo = 0; //general
		uint8_t cmd = 0x02; //ipod ack
		uint8_t cmdstatus = 0x00; //success
		uint8_t origCmdID = 0x38; //origCmdID?
		uint8_t pL[2] = { cmdstatus, origCmdID };

		//USBD_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
		initResponse(lingo, cmd, msg.transID);
		addResponsePayload(&pL, sizeof(pL));
		transmitToAcc();

		xprintf("send IPOD_ACK\n");
		//_transState.lastCmd = UNDEF;
	}
		break;

	case 0x39: //SetFIDTokenValues
	{ //ack with: AckFIDTokenValues 0x03a
		uint8_t numFIDTokenValues = msg.raw[8];
		uint8_t tokenStartIdx = 9;
		const uint8_t lingogeneric = 0x00;
		const uint8_t cmdId = 0x3a;

		//uint8_t ackTokenTemplate[5];

		initResponse(lingogeneric, cmdId, msg.transID);
		addResponsePayload(numFIDTokenValues, 1);

		for (int x = 0; x < numFIDTokenValues; x++) {
			uint8_t tLeng = msg.raw[tokenStartIdx];
			uint8_t tFidType = msg.raw[tokenStartIdx + 1];
			uint8_t tFidSubType = msg.raw[tokenStartIdx + 2];
			uint8_t tdataStart = msg.raw[tokenStartIdx + 3];

			if (tFidType == 0x00) {
				if (tFidSubType == 0x00) {
					xprintf("IdentityToken : ");
					uint8_t identAck[] = { 0x03, tFidType, tFidSubType, 0 };
					addResponsePayload(identAck, sizeof(identAck));

				} else if (tFidSubType == 0x01) {
					xprintf("AccessoryCapsToken:\n");
					uint8_t accCapAck[] = { 0x03, tFidType, tFidSubType, 0 };
					addResponsePayload(accCapAck, sizeof(accCapAck));
					for (uint8_t x = 0; x < 8; x++) {
						xprintf("cap b%d: %b \n",
								msg.raw[tokenStartIdx + 3 + x]);
					}

				} else if (tFidSubType == 0x02) {

					xprintf("AccessoryInfoToken: ");
					uint8_t accInfoAck[] = { 0x04, tFidType, tFidSubType, 0,
							tdataStart };
					addResponsePayload(accInfoAck, sizeof(accInfoAck));

					if (tdataStart == 0x01) {
						xprintf("name: ");
						uint8_t idx = 1;
						while (idx) {
							char c = msg.raw[tokenStartIdx + 3 + idx];
							xprintf("%c", c);
							if (c == '\0')
								break;
							idx++;
						}

					}
					if (tdataStart == 0x04) {
						xprintf("FW: %x %x %x ", msg.raw[tokenStartIdx + 3 + 1],
								msg.raw[tokenStartIdx + 3 + 2],
								msg.raw[tokenStartIdx + 3 + 3]);
					}
					if (tdataStart == 0x06) {
						xprintf("Manufact: ");
						uint8_t idx = 1;
						while (idx) {
							char c = msg.raw[tokenStartIdx + 3 + idx];
							xprintf("%c", c);
							if (c == '\0')
								break;
							idx++;
						}

					}

				} else if (tFidSubType == 0x03) {
					xprintf("iPodPreferenceToken: ");
					uint8_t accipodPrefTokenInfoAck[] = { 0x04, tFidType,
							tFidSubType, 0, tdataStart };
					addResponsePayload(accipodPrefTokenInfoAck,
							sizeof(accipodPrefTokenInfoAck));

				} else if (tFidSubType == 0x04) {
					xprintf("EAProtocolToken: ");
					uint8_t accEapProAcc[] = { 0x04, tFidType, tFidSubType, 0,
							tdataStart };
					addResponsePayload(accEapProAcc, sizeof(accEapProAcc));
				} else {
					xprintf("Other Token: ");
				}
				if (tFidType == 0x00 && tFidSubType == 0x0e) {
					xprintf("AccessoryDigital-AudioSampleRates-Token\n");
				}

			}
			xprintf("len: %d, type: %X, subType: %x \n", tLeng, tFidType,
					tFidSubType);

			//prep for next token
			tokenStartIdx += (tLeng + 1);
		}
		transmitToAcc();
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

		uint8_t pl[2] = { maxHi, maxLo };
		initResponse(lingo, cmd, msg.transID);
		addResponsePayload(&pl, sizeof(pl));
		transmitToAcc();

	}
		break;

	case 0x4b: //GetiPodOptionsForLingo:
	{
		//GetiPodOptionsForLingo
		const uint8_t RetiPodOptionsForLingoCmd = 0x4c;
		uint8_t reqestedLingoOpts = msg.raw[8];
		uint8_t pkg_len = 14;
		uint8_t caps[9] = { 0 };
		caps[0] = reqestedLingoOpts;

		switch (reqestedLingoOpts) {
		case 0x00: //lingo 0
		{

		}
			break;
		}

		//uint8_t pl[2] = { maxHi, maxLo };
		initResponse(LingoGeneral, RetiPodOptionsForLingoCmd, msg.transID);
		addResponsePayload(caps, sizeof(caps));
		transmitToAcc();

		return;

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

