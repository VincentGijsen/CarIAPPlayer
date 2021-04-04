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


typedef enum{
	UNDEF=0,
	INIT,
	TRANSPORT_RESET,
	NEGOTIATE1,

}typeDefTransportStatus;

typedef struct {
	uint16_t oldestTransActionNumber;
	uint8_t lastCmd;

}typedefTransportState;


static typeDefTransportStatus _transStatus = INIT;
static typedefTransportState _transState;

extern USBD_HandleTypeDef hUsbDeviceFS;

void processInbound(uint8_t *pkg, uint8_t len) {
	uint8_t offset = 0;
	uint8_t idx_payload = 0;
	uint16_t length = 0;
	uint8_t lingoID = 0;

	uint16_t commandID = 0;
	uint16_t transID = 0;
	uint8_t checksum = 0;

	if (len < 5) {
		return;
	}
	//check preamble
	if (!(pkg[CMD_PKG_PREAMBLE0] == 0x00 && pkg[CMD_PKG_PREAMBLE1] == 0x55)) {
		//invalid pkg
		return;

	}

	//length check
	if (pkg[CMD_PKG_LEN] == 0x00) {
		//3byte pkg
		length = pkg[CMD_PKG_LEN + 1] << 8;
		length += pkg[CMD_PKG_LEN + 2];
		offset += 3;
	} else {
		length = pkg[CMD_PKG_LEN];
	}

	lingoID = pkg[CMD_PKG_LINGO_ID + offset];

	//commandID checks
	commandID = pkg[CMD_PKG_COMMAND_ID + offset];
	//todo: offset checks for part

	transID = (pkg[CMD_PKG_TRANSACTION_ID + offset] << 8);
	transID += pkg[CMD_PKG_TRANSACTION_ID + 1 + offset];

	//payload shizzle
	//payload

	checksum = pkg[CMD_PKG_LINGO_ID + length]; //includes any offsets after length idx

	//checksum check
	int8_t checksum_calc = length + lingoID + commandID;
	//todo: not fully correct for 2-byte cmds!
	for (uint8_t it = 0; it < (length -4); it++) {
		checksum_calc += pkg[(CMD_PKG_TRANSACTION_ID + 2) + offset + it];
	}
	checksum_calc = 0xff - checksum_calc + 1;

	xprintf("checksum provided: %X  calculated %X \n", checksum, checksum_calc);

	//vieze hack
	if(commandID == 0x38){
		_transState.lastCmd = INIT;
		_transState.oldestTransActionNumber = transID;
	}
}


//p150MFiAccessoryFirmwareSpecificationR46NoRestriction
#define IAP_CMD_CODE_SUCCESS 0x00
#define IAP_CMD_CODE_ERROR_UNKNOWN_SESSIONID 0x01
#define IAP_CMD_CODE_ERROR_CMD_FAILED 0x03
void processTransport(){
	if(_transState.lastCmd> UNDEF){

		switch(_transState.lastCmd)
		{
		case INIT:
		{
			//p252 MFiAccessoryFirmwareSpecificationR46NoRestriction.
			//STARTIPS process stap
			uint8_t report[] = {0x05,00,0x55,0x04, 0x03, 0x38, 0x00,0x01};
			//addchecksum byes
			USBD_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
		}
			break;
		}
	}

}
