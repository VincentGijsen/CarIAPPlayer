/*
 * transactionLayer.c
 *
 *  Created on: 3 Apr 2021
 *      Author: vtjgi
 */

#include "xprintf.h"
#include <stdint.h>
#include "transactionlayer.h"

#include "usbd_def.h"

#include "lingos.h"
#include "tasks.h"

//src MFiAccessoryFirmwareSpecificationR46NoRestriction. p109
//idx'es
#define CMD_PKG_HID_ID 0
#define CMD_PKG_PREAMBLE0 1
#define CMD_PKG_PREAMBLE1 2
#define CMD_PKG_LEN 3 //can be 3 bytes (if pos1 of 3 = 0x00), then 3byte

#define CMD_PKG_LINGO_ID 4 //or with offset
#define CMD_PKG_COMMAND_ID 5
#define CMD_PKG_TRANSACTION_ID 6 //two bytes
#define CMD_PKG_PAYLOAD_BYTE0_IF_SET 8 //two bytes

#define CMD_PKG_CHECKSUM CMD_PKG_LINGO_ID /*if added with length points to idx of checksum */

//global vars
uint16_t podTransactionCounter = 0;

#define TAKSITEMS_SIZE 10
typeDefTaskListItem TaskItems[TAKSITEMS_SIZE];

typedef enum {
	UNDEF = 0, INIT, TRANSPORT_RESET, NEGOTIATE1,

} typeDefTransportStatus;

typedef struct {
	uint16_t oldestTransActionNumber;
	uint8_t lastCmd;

} typedefTransportState;

/* src https://github.com/oandrew/ipod/blob/master/hid/report_def.go
 *
 ReportDef{ID: 0x01, Len: 12, Dir: ReportDirAccIn},
 ReportDef{ID: 0x02, Len: 14, Dir: ReportDirAccIn},
 ReportDef{ID: 0x03, Len: 20, Dir: ReportDirAccIn},
 ReportDef{ID: 0x04, Len: 63, Dir: ReportDirAccIn},

 ReportDef{ID: 0x05, Len: 8, Dir: ReportDirAccIn},
 ReportDef{ID: 0x06, Len: 10, Dir: ReportDirAccIn},
 ReportDef{ID: 0x07, Len: 14, Dir: ReportDirAccIn},
 ReportDef{ID: 0x08, Len: 20, Dir: ReportDirAccIn},
 ReportDef{ID: 0x09, Len: 63, Dir: ReportDirAccIn},
 */

static typeDefTransportStatus _transStatus = INIT;
static typedefTransportState _transState;

uint8_t calcCRC(uint8_t *b, uint8_t len) {
	//checksum check
	int8_t checksum_calc = 0;
	//int8_t checksum_calc = length + lingoID + commandID + transID;
	//todo: not fully correct for 2-byte cmds!
	for (uint8_t it = 0; it < len; it++) {
		//checksum_calc += pkg[(CMD_PKG_TRANSACTION_ID + 2) + offset + it];
		checksum_calc += b[it];
	}
	checksum_calc = -checksum_calc;
	return checksum_calc;
}

MsgStore store;

void transportInit() {
	store.set[0] = 0;
	store.set[1] = 0;
	store.set[2] = 0;
	store.set[3] = 0;
}

void processInbound(uint8_t *usbPkg, uint8_t usbPkgLen) {
	/*
	 * Constants
	 *
	 */
	const uint8_t offsetForHIDIdAndLinkByte = 2;

	/*
	 * local counters
	 */
	uint8_t offset = 0; //additional byte-index to use
	//uint8_t idx_payload = 0;
	uint16_t length = 0; //local length of whole package
	uint8_t remaining_copy_bytes;
	uint8_t lingoID = 0;

	uint16_t commandID = 0;
	uint16_t transID = 0;
	//uint8_t checksum = 0;

	//helpers
	uint8_t extraBytes = 0;

	if (usbPkgLen < 5) {
		return;
	}
	//check preamble
	if (!(usbPkg[CMD_PKG_PREAMBLE0] == HID_LINKCTRL_DONE
			|| usbPkg[CMD_PKG_PREAMBLE0] == HID_LINKCTRL_CONTINUE
			|| usbPkg[CMD_PKG_PREAMBLE0] == HID_LINKCTRL_MORE_TO_FOLLOW
			|| usbPkg[CMD_PKG_PREAMBLE0] == HID_LINKCTRL_ONLY_DATA)) {
		//invalid pkg
		xprintf("invalid package start\n");
		return;

	}

	/*
	 * this section reads the extended package header
	 */
	if (usbPkg[CMD_PKG_PREAMBLE0] == HID_LINKCTRL_DONE
			|| usbPkg[CMD_PKG_PREAMBLE0] == HID_LINKCTRL_MORE_TO_FOLLOW) {
		for (uint8_t x = 0; x < 4; x++) {
			if (store.set[x] == EMPTY || store.set[x] == MORE_DATA_PENDING) {

				store.msg[x].crcCalc = 0;
				//length check
				if (usbPkg[CMD_PKG_LEN] == 0x00) {
					//3byte pkg length
					length = usbPkg[CMD_PKG_LEN + 1] << 8;
					length += usbPkg[CMD_PKG_LEN + 2];
					offset += 2;
					extraBytes = 3;
					store.msg[x].crcCalc += usbPkg[CMD_PKG_LEN + 1] << 8;
					store.msg[x].crcCalc += usbPkg[CMD_PKG_LEN + 2];

				} else { //small package <252
					length = usbPkg[CMD_PKG_LEN];
					offset += 0;
					extraBytes = 1;
					store.msg[x].crcCalc += usbPkg[CMD_PKG_LEN];
				}

				//add extra bytes [hid +linkbyte +start + first_len_byte
				//remaining_copy_bytes += 4;

				lingoID = usbPkg[CMD_PKG_LINGO_ID + offset];
				store.msg[x].crcCalc += usbPkg[CMD_PKG_LINGO_ID + offset];

				if (lingoID == 0x04) { //in extended mode, the commands are two bytes :(
					commandID = ((usbPkg[CMD_PKG_COMMAND_ID + offset]) << 8);
					commandID += (usbPkg[CMD_PKG_COMMAND_ID + 1 + offset]);

					store.msg[x].crcCalc += usbPkg[CMD_PKG_COMMAND_ID + offset];
					store.msg[x].crcCalc += usbPkg[CMD_PKG_COMMAND_ID + 1
							+ offset];

					offset += 1;
				} else {
					//commandID checks
					commandID = usbPkg[CMD_PKG_COMMAND_ID + offset];
					store.msg[x].crcCalc += usbPkg[CMD_PKG_COMMAND_ID + offset];

				}

				remaining_copy_bytes = length + offset;

				if (remaining_copy_bytes > (MAX_RAW_SIZE - 2)) {
					xprintf("Message to big to process!\n");
				}

				//todo: offset checks for part

				transID = (usbPkg[CMD_PKG_TRANSACTION_ID + offset] << 8);
				transID += usbPkg[CMD_PKG_TRANSACTION_ID + 1 + offset];

				store.msg[x].crcCalc += usbPkg[CMD_PKG_TRANSACTION_ID + offset];
				store.msg[x].crcCalc += usbPkg[CMD_PKG_TRANSACTION_ID + 1
						+ offset];

				//store.msg[x].len = usbPkgLen;
				store.msg[x].lingoID = lingoID;
				store.msg[x].commandId = commandID;
				store.msg[x].transID = transID;
				store.msg[x].length = length;
				store.msg[x].remainingPayLoadSize = (length - 4 -offset ); //carefull, remove offset; as we shift right
				store.msg[x].offset = offset;
				store.msg[x].nextWrite = 0;

				if (usbPkg[CMD_PKG_PREAMBLE0] == HID_LINKCTRL_DONE) {
					//only copy sthufs for a 'small/single pkg'
					uint8_t it = 0; //will fit due to small package
					uint8_t crcIdxoffset = store.msg[x].remainingPayLoadSize +offset;
					while (store.msg[x].remainingPayLoadSize > 0) {

						uint8_t b = usbPkg[CMD_PKG_PAYLOAD_BYTE0_IF_SET + offset
								+ it];
						store.msg[x].payload[store.msg[x].nextWrite] = b;
						store.msg[x].crcCalc += b;
						store.msg[x].nextWrite++;
						store.msg[x].remainingPayLoadSize--;
						it++; //not used, as remaining should run out with this type of HID frame

					}
					//final checksum step:
					store.msg[x].crcCalc = -store.msg[x].crcCalc;
					//next_write should point to crc (last byte)
					uint8_t checksum = usbPkg[CMD_PKG_PAYLOAD_BYTE0_IF_SET
							+ crcIdxoffset]; //includes any offsets after length idx
					//uint8_t checksum_val = calcCRC(&usbPkg[CMD_PKG_LEN],
					//(length + extraBytes));

					if (checksum != (uint8_t) (store.msg[x].crcCalc)) {
						xprintf(
								"checksum Failed: provided: %X  calculated %X \n",
								checksum, (uint8_t) store.msg[x].crcCalc);
						return;

					} else {

						//flag as ready-to-rpocess
						store.set[x] = READY_TO_PROCESS;
						return;
					}
				} else {
					//HID_LINKCTRL_MORE_TO_FOLLOW
					//done elsewhere
					/*
					 * Process payload of Type 2
					 * HID_LINKCTRL_MORE_TO_FOLLOW
					 */

					uint8_t it = 0;
					uint8_t dataStartingAt = CMD_PKG_PAYLOAD_BYTE0_IF_SET
							+ offset;
					while (store.msg[x].remainingPayLoadSize > 0
							&& (it + dataStartingAt) < usbPkgLen) {

						uint8_t b = usbPkg[dataStartingAt + it];

						store.msg[x].payload[store.msg[x].nextWrite] = b;
						store.msg[x].crcCalc += b;
						//for (int y = 0; y < (len); y++) {
						//store.msg[x].raw[it] = usbPkg[it];
						store.msg[x].nextWrite++;
						store.msg[x].remainingPayLoadSize--;
						it++;

					}
					store.set[x] = MORE_DATA_PENDING;
					return;
				}

			}
		}

	} else {
		/*
		 * Intransit package, so we need to add to a prior one(s)
		 */
		if (usbPkg[CMD_PKG_PREAMBLE0] == HID_LINKCTRL_ONLY_DATA) {
			//END_OF_PREVIOUS_TRANSFER
			for (uint8_t x = 0; x < 4; x++) {
				if (store.set[x] == MORE_DATA_PENDING) { //2 indicates partial entry already
					uint8_t startPos = store.msg[x].nextWrite; //nextwRITE FILLED AT PREVIOUS ENTRY

					uint8_t it = offsetForHIDIdAndLinkByte;
					while (store.msg[x].remainingPayLoadSize > 0 && (it  < usbPkgLen)) { //copy whole frame, minus hid-rep and linkbyte
						uint8_t b = usbPkg[it];

						store.msg[x].payload[store.msg[x].nextWrite] = b;
						store.msg[x].crcCalc += b;

						store.msg[x].nextWrite++; //increment last write slot in-case 3rd packet comes
						store.msg[x].remainingPayLoadSize--; //decrement total amount of bytes to copy
						it++;

					}

					return;
					break;
				}
			}

		}

	}

	/*
	 * End of (multiple) packages, copy remaining bytes and 'submit'
	 */
	if (usbPkg[CMD_PKG_PREAMBLE0] == HID_LINKCTRL_CONTINUE) {
		for (uint8_t x = 0; x < 4; x++) {
			if (store.set[x] == MORE_DATA_PENDING) { //2 indicates partial entry already
				uint8_t startPos = store.msg[x].nextWrite; //nextwRITE FILLED AT PREVIOUS ENTRY

				uint8_t it = offsetForHIDIdAndLinkByte;
				uint8_t crcIdxoffset = store.msg[x].remainingPayLoadSize;
				while (store.msg[x].remainingPayLoadSize > 0) { //copy whole frame, minus hid-rep and linkbyte
					//store.msg[x].raw[store.msg[x].nextWrite] = usbPkg[it
					//		+ offSetHidLcb];
					uint8_t b = usbPkg[it];
					store.msg[x].payload[store.msg[x].nextWrite] = b;
					store.msg[x].crcCalc += b;

					store.msg[x].nextWrite++; //increment last write slot in-case 3rd packet comes
					store.msg[x].remainingPayLoadSize--; //decrement total amount of bytes to copy
					it++;
				}
				//update record, make sure its processed further down
				//store.set[x] = 1;

				//final checksum step:
				store.msg[x].crcCalc = -store.msg[x].crcCalc;

				uint8_t checksum = usbPkg[offsetForHIDIdAndLinkByte
						+ crcIdxoffset]; //includes any offsets after length idx

				if (checksum != (uint8_t) (store.msg[x].crcCalc)) {
					xprintf(
							"checksum multipkg Failed: provided: %X  calculated %X \n",
							checksum, (uint8_t) store.msg[x].crcCalc);
					return;

				}

				//ok
				store.set[x] = READY_TO_PROCESS;

				return;
				break;
			}
		}

	}

//vieze hack
//if (commandID == 0x38) {
//	_transState.lastCmd = INIT;
////	_transState.oldestTransActionNumber = transID;
//}
}

//p150MFiAccessoryFirmwareSpecificationR46NoRestriction
#define IAP_CMD_CODE_SUCCESS 0x00
#define IAP_CMD_CODE_ERROR_UNKNOWN_SESSIONID 0x01
#define IAP_CMD_CODE_ERROR_CMD_FAILED 0x03

/*
 * outbound message handling
 */

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

/*
 * Add payload to the package
 */
void addResponsePayload(uint8_t *bytes, uint8_t len) {

	for (uint8_t idx = 0; idx < len; idx++) {
		response_state.buffer[response_state.writePtr] = bytes[idx];
		response_state.writePtr++;
	}
}

/*
 * Send package to Accessory
 */
void transmitToAcc() {
	/*
	 * calc length
	 */
	uint8_t outBuf[64] = { 0 };

	const uint8_t USABLE_BYTES = 64;
	uint16_t totalBytes = 3; //hid + linkbyte + start of frame
	uint8_t reportSize = 0;

	uint8_t extendedLingoCmdoffset = 0;

//check for extended lingos
	if (response_state.lingo == 0x04) {
		extendedLingoCmdoffset++;

	}

	if ((response_state.writePtr + 4) <= 252) {
		totalBytes += 5 + extendedLingoCmdoffset; //length(1x)+lingo+cmd+transId(2byte)
	} else {
		totalBytes += 7 + extendedLingoCmdoffset; //length(3x)+lingo+cmd+transId(2byte)
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
		outBuf[3] = response_state.writePtr + 4 + extendedLingoCmdoffset; //payload length + lingo+cmd+transactionbytes
		outBuf[4] = response_state.lingo;
		if (extendedLingoCmdoffset > 0) {

			outBuf[5] = 0;
		}
		outBuf[5 + extendedLingoCmdoffset] = response_state.cmd;
		outBuf[6 + extendedLingoCmdoffset] = (uint8_t) ((response_state.transId
				>> 8) & 0xff); //msb
		outBuf[7 + extendedLingoCmdoffset] = (uint8_t) ((response_state.transId)
				& 0xff); //lsb

		//crc calc
		for (uint8_t i = 3; i <= 7 + extendedLingoCmdoffset; i++) {
			checksum_calc += outBuf[i + extendedLingoCmdoffset];
		}

		if (!multiFrame) {
			//write payload
			for (uint8_t idx = 0; idx < response_state.writePtr; idx++) {
				outBuf[8 + idx + extendedLingoCmdoffset] =
						response_state.buffer[idx];
				checksum_calc += response_state.buffer[idx];
			}

			//ad checksum as last:
			outBuf[8 + response_state.writePtr + extendedLingoCmdoffset] =
					(-checksum_calc);
			//send first frame out
			USBD_HID_SendReport(&hUsbDeviceFS, outBuf, reportSize);

			//we are done with small frame handling
			return;
		} else {
			uint8_t byteCntCompentsation = 8 + extendedLingoCmdoffset;
			remaining_Bytes -= byteCntCompentsation; //decrement for first
			byteCounterProcessed = byteCntCompentsation;
			//in multi frame, write full set
			//fill up the frame
			for (uint8_t idx = 0;
					(idx < response_state.writePtr
							&& (byteCntCompentsation + idx) < reportSize);
					idx++) {
				outBuf[byteCntCompentsation + idx] = response_state.buffer[idx];
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
}

/*
 * LINGO processing of inboud messages and delivery to helpers
 */

void processTransport() {

	for (uint8_t x = 0; x < 4; x++) {
		if (store.set[x] == READY_TO_PROCESS) {
			switch (store.msg[x].lingoID) {
			case LingoGeneral:
				//call lingo generic
				processLingoGeneral(store.msg[x]);
				break;
			case LingoExtended:
				processLingoExtendedInterface(store.msg[x]);
				break;

			case LingoDigitalAudio:
				processLingoDigitalAudio(store.msg[x]);
				break;

			default:
				//not implemented
				xprintf("unimplemented lingo: %x\n", store.msg[x].lingoID);
				;
			}
			//flag as done
			store.set[x] = EMPTY;
		}
	}

}

void scheduleTasks(typeDefTask task) {
	for (uint8_t i = 0; i < TAKSITEMS_SIZE; i++) {
		if (TaskItems[i].active == 0) {
			TaskItems[i].active = 1;
			TaskItems[i].task = task;
			return;
		}
	}
}

void processTasks() {
	for (uint8_t i = 0; i < TAKSITEMS_SIZE; i++) {
		if (TaskItems[i].active == 1) {
			if (TaskItems[i].task.scheduledAfter <= uwTick) {
				//taskRunner(TaskItems[i]);
				TaskItems[i].task.f();
				TaskItems[i].active = 0;
			}
		}
	}
//uwTick
}
