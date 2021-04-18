/*
 * lingo_aditional.c
 *
 *  Created on: 7 Apr 2021
 *      Author: vtjgi
 */


#include "lingos.h"
#include "transactionlayer.h"

#include "stdint.h"

#include "xprintf.h"






void processLingoDigitalAudio(IAPmsg msg) {

	switch (msg.commandId) {

	case 0x00://AccessoryAck
	{
		uint8_t b0 = msg.payload[0];
		uint8_t b1 = msg.payload[1];
		xprintf("ACC ack for TrackNewAudioAttributes\n");
	}
	break;


	case 0x03://RetAccessorySampleRateCaps
	{
		xprintf("RetAccessorySampleRateCaps: \n");
		//uint8_t b0 = msg.raw[8];
		//list of uint32_t blocks of supported rates
	}
	break;



	default:
		xprintf("Error processing digitalaudio cmd: %x",  msg.commandId);
	}
}
