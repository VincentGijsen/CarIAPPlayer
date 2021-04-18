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
#include "tasks.h"

#define HID_LINKCTRL_DONE 0x00
#define HID_LINKCTRL_CONTINUE 0x01
#define HID_LINKCTRL_MORE_TO_FOLLOW 0x02
#define HID_LINKCTRL_ONLY_DATA 0x03
#define START_OF_FRAME 0x55

#define GEN_TRANSACTIONBYTES(x) (uint8_t)((x >> 8) &0xff),(uint8_t)(x &0xff)

typedef enum {
	EMPTY = 0, READY_TO_PROCESS, MORE_DATA_PENDING,
} MsgState;

#define MAX_RAW_SIZE 512
typedef struct {
	//uint8_t raw[MAX_RAW_SIZE];
	uint8_t payload[MAX_RAW_SIZE]; //inbound payload after pkg-struture
	//uint8_t len;
	uint16_t length; //lingo+cmd+trans bytes + payload -minus total size of pkg stream;
	uint16_t lingoID; //inbound lingo id
	uint16_t commandId; //inbound command id
	uint16_t transID; //inbound transaction ID of acc
	uint8_t nextWrite; // points to payload position witin payload copy
	uint8_t crcProvided; //inbound crc from acc
	int8_t crcCalc; //used to calc inflight crc
	uint8_t remainingPayLoadSize; //bytes of actual payload stufs
	uint8_t offset;//offset due to muti-byte span of leng/cmd

} IAPmsg;

typedef struct {
	MsgState set[4];
	IAPmsg msg[4];
} MsgStore;

extern USBD_HandleTypeDef hUsbDeviceFS;
extern uint16_t podTransactionCounter;

//#define LingoAdditional 0x0a
#define LingoGeneral 0x00
#define LingoExtended 0x04
#define LingoDigitalAudio 0x0a

//protos
void processLingoGeneral( typeDefMessage);
void processLingoDigitalAudio(IAPmsg msg);
void processLingoExtendedInterface(IAPmsg msg);

void scheduleTasks(typeDefTask task);

//general
void taskInitializeAuthentication();

//extended
void taskProcessQuery();
void taskPlayControlCommand();
void taskTrackChanged();

#endif /* INC_LINGOS_H_ */
