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






void processLingoAdditional(IAPmsg msg) {
	xprintf("||OTHER LINGO :)\n");
	switch (msg.commandId) {
	case 0x03://RetAccessorySampleRateCaps
	{
		xprintf("RetAccessorySampleRateCaps: \n");
	}
	break;


	default:
		xprintf("Error processing lingo@Additional: %x, cmd: %x", LingoAdditional, msg.commandId);
	}
}
