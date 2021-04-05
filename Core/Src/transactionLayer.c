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

//src MFiAccessoryFirmwareSpecificationR46NoRestriction. p109
//idx'es
#define CMD_PKG_HID_ID 0
#define CMD_PKG_PREAMBLE0 1
#define CMD_PKG_PREAMBLE1 2
#define CMD_PKG_LEN 3 //can be 3 bytes (if pos1 of 3 = 0x00), then 3byte

#define CMD_PKG_LINGO_ID 4 //or with offset
#define CMD_PKG_COMMAND_ID 5
#define CMD_PKG_TRANSACTION_ID 6 //two bytes
#define CMD_PKG_CHECKSUM CMD_PKG_LINGO_ID /*if added with length points to idx of checksum */

typedef enum {
	UNDEF = 0, INIT, TRANSPORT_RESET, NEGOTIATE1,

} typeDefTransportStatus;

typedef struct {
	uint16_t oldestTransActionNumber;
	uint8_t lastCmd;

} typedefTransportState;

/* src https://github.com/oandrew/ipod/blob/master/hid/report_def.go
 * 	ReportDef{ID: 0x01, Len: 12, Dir: ReportDirAccIn},
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

void processInbound(uint8_t *pkg, uint8_t len) {
	uint8_t offset = 0;
	uint8_t idx_payload = 0;
	uint16_t length = 0;
	uint8_t remaining_copy_bytes;
	uint8_t lingoID = 0;

	uint16_t commandID = 0;
	uint16_t transID = 0;
	uint8_t checksum = 0;

	//helpers
	uint8_t extraBytes = 0;

	if (len < 5) {
		return;
	}
	//check preamble
	if (!(pkg[CMD_PKG_PREAMBLE0] == HID_LINKCTRL_DONE
			|| pkg[CMD_PKG_PREAMBLE0] == HID_LINKCTRL_CONTINUE
			|| pkg[CMD_PKG_PREAMBLE0] == HID_LINKCTRL_MORE_TO_FOLLOW
			|| pkg[CMD_PKG_PREAMBLE0] == HID_LINKCTRL_ONLY_DATA)) {
		//invalid pkg
		xprintf("invalid package start\n");
		return;

	}

	/*
	 * this section reads the extended package header
	 */
	if (pkg[CMD_PKG_PREAMBLE0] == HID_LINKCTRL_DONE
			|| pkg[CMD_PKG_PREAMBLE0] == HID_LINKCTRL_MORE_TO_FOLLOW) {
		//length check
		if (pkg[CMD_PKG_LEN] == 0x00) {
			//3byte pkg length
			length = pkg[CMD_PKG_LEN + 1] << 8;
			length += pkg[CMD_PKG_LEN + 2];
			offset += 2;
			extraBytes = 3;
			remaining_copy_bytes = length + offset;
		} else {
			length = pkg[CMD_PKG_LEN];
			offset += 0;
			extraBytes = 1;
			remaining_copy_bytes = length + offset;
		}
		//add extra bytes [hid +linkbyte +start + first_len_byte
		remaining_copy_bytes += 4;

		lingoID = pkg[CMD_PKG_LINGO_ID + offset];

		//commandID checks
		commandID = pkg[CMD_PKG_COMMAND_ID + offset];
		//todo: offset checks for part

		transID = (pkg[CMD_PKG_TRANSACTION_ID + offset] << 8);
		transID += pkg[CMD_PKG_TRANSACTION_ID + 1 + offset];

		for (uint8_t x = 0; x < 4; x++) {
			if (store.set[x] == EMPTY || store.set[x] == MORE_DATA_PENDING) {

				store.msg[x].len = len;
				store.msg[x].lingoID = lingoID;
				store.msg[x].commandId = commandID;
				store.msg[x].transID = transID;
				store.msg[x].length = length;
				store.msg[x].remaining_copy_bytes = remaining_copy_bytes;
				store.msg[x].nextWrite = 0;

				if (pkg[CMD_PKG_PREAMBLE0] == HID_LINKCTRL_DONE) {
					//only copy sthufs for a 'small/single pkg'
					uint8_t it = 0;
					while (store.msg[x].remaining_copy_bytes > 0) {
						//for (int y = 0; y < (len); y++) {
						store.msg[x].raw[it] = pkg[it];
						store.msg[x].nextWrite++;
						store.msg[x].remaining_copy_bytes--;
						it++;

					}
					//next_write should point to crc (last byte)
					checksum = pkg[store.msg[x].nextWrite]; //includes any offsets after length idx
					uint8_t checksum_val = calcCRC(&pkg[CMD_PKG_LEN],
							(length + extraBytes));

					if (checksum != checksum_val) {
						xprintf(
								"checksum Failed: provided: %X  calculated %X \n",
								checksum, checksum_val);
						return;

					} else {

						//flag as ready-to-rpocess
						store.set[x] = READY_TO_PROCESS;
						return;
					}
				} else {
					//done elsewhere
					/*
					 * Process payload of Type 2
					 * HID_LINKCTRL_MORE_TO_FOLLOW
					 */

					uint8_t it = 0;
					while (store.msg[x].remaining_copy_bytes > 0 && it < len) {
						//for (int y = 0; y < (len); y++) {
						store.msg[x].raw[it] = pkg[it];
						store.msg[x].nextWrite++;
						store.msg[x].remaining_copy_bytes--;
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
		if (pkg[CMD_PKG_PREAMBLE0] == HID_LINKCTRL_ONLY_DATA) {
			//END_OF_PREVIOUS_TRANSFER
			for (uint8_t x = 0; x < 4; x++) {
				if (store.set[x] == MORE_DATA_PENDING) { //2 indicates partial entry already
					uint8_t startPos = store.msg[x].nextWrite; //nextwRITE FILLED AT PREVIOUS ENTRY
					const uint8_t offsetForHIDIdAndLinkByte = 2;

					for (uint8_t y = 0; y < (len - offsetForHIDIdAndLinkByte);
							y++) { //copy whole frame, minus hid-rep and linkbyte
						store.msg[x].raw[startPos + y] = pkg[y
								+ offsetForHIDIdAndLinkByte];
						store.msg[x].nextWrite++; //increment last write slot in-case 3rd packet comes
						remaining_copy_bytes--; //decrement total amount of bytes to copy
					}
					//don't finalise pkg, more should come
					store.set[x] = MORE_DATA_PENDING;
					store.msg[x].remaining_copy_bytes = remaining_copy_bytes;
					return;
					break;
				}
			}

		}

	}

	/*
	 * End of (multiple) packages, copy remaining bytes and 'submit'
	 */
	if (pkg[CMD_PKG_PREAMBLE0] == HID_LINKCTRL_CONTINUE) {
		for (uint8_t x = 0; x < 4; x++) {
			if (store.set[x] == MORE_DATA_PENDING) { //2 indicates partial entry already
				uint8_t startPos = store.msg[x].nextWrite; //nextwRITE FILLED AT PREVIOUS ENTRY

				const uint8_t offSetHidLcb = 2;

				uint8_t it = 0;
				while (store.msg[x].remaining_copy_bytes > 0) { //copy whole frame, minus hid-rep and linkbyte
					store.msg[x].raw[store.msg[x].nextWrite] = pkg[it
							+ offSetHidLcb];
					store.msg[x].nextWrite++; //increment last write slot in-case 3rd packet comes
					store.msg[x].remaining_copy_bytes--; //decrement total amount of bytes to copy
					it++;
				}
				//update record, make sure its processed further down
				//store.set[x] = 1;

				if (length < 252)
					extraBytes = 1;
				else
					extraBytes = 3;
				checksum = pkg[it + offSetHidLcb]; //includes any offsets after length idx

				uint8_t checksum_val = calcCRC(&store.msg[x].raw[CMD_PKG_LEN],
						(store.msg[x].length + extraBytes));

				if (checksum != checksum_val) {
					xprintf("checksum Failed: provided: %X  calculated %X \n",
							checksum, checksum_val);
					return;
				} else {
					//ok
					store.set[x] = READY_TO_PROCESS;

					return;
					break;
				}
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

void processTransport() {

	for (uint8_t x = 0; x < 4; x++) {
		if (store.set[x] == READY_TO_PROCESS) {
			switch (store.msg[x].lingoID) {
			case 0x00:
				//call lingo generic
				processLingoGeneral(store.msg[x]);
				store.set[x] = EMPTY;
				break;

			default:
				//not implemented
				;
			}
		}
	}

}
