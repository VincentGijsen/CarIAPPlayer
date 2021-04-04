/*
 * lingos.h
 *
 *  Created on: 4 Apr 2021
 *      Author: vtjgi
 */

#ifndef INC_LINGOS_H_
#define INC_LINGOS_H_
#include "stdint.h"
#include "usbd_def.h"

typedef struct {
	uint8_t raw[128];
	uint8_t len;
	uint16_t lingoID;
	uint16_t commandId;
	uint16_t transID;
	uint8_t nextWrite;
	uint8_t length;

} IAPmsg;

typedef struct {
	uint8_t set[4];
	IAPmsg msg[5];
} MsgStore;


extern USBD_HandleTypeDef hUsbDeviceFS;


//protos
void processLingoGeneral(typeDefMessage);

#endif /* INC_LINGOS_H_ */
