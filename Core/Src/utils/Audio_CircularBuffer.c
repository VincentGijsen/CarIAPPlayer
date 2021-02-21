/*

 * Audio_CircularBuffer.c
 *
 *  Created on: Feb 16, 2021
 *      Author: vtjgi

#include "utils.h"
#include "FreeRTOS.h"

//prototypes
BUFFER_RESULT _putSinglePkg(void *pkg);

//global data structures
static AUDIO_CircularBuffer_t _buffer;

#define CIRC_BUFFER_SIZE 1//1540 //192 *8 + some more // ((MAX_freq * CHANNELS *BYTESPerFrame )
static uint8_t _actual_buffer[CIRC_BUFFER_SIZE];

volatile _USB_AUDIO_BUFFER usbBuffer;

BUFFER_RESULT initBuffer(uint16_t pkgSize, uint16_t numberOfPackages) {

	usbBuffer.activeRead = 0;
	usbBuffer.activeWrite = 0;
	usbBuffer.slots = _USB_AUDIO_BUFFER_SLOTS;
	usbBuffer.initialized = 1;

}

 BUFFER_RESULT getPackageFromBuffer(uint8_t *data) {
 if (!_buffer.isInitialized) {
 return BUFFER_ERROR;

 if (_buffer.rd_ptr == _buffer.wr_ptr) {
 return BUFFER_UNDERRUN;
 }

 }
 }

uint16_t getFreeSlotsInBuffer() {

	 if (!_buffer.isInitialized) {
	 return 0;
	 }

	 if (_buffer.rd_ptr < _buffer.wr_ptr) {
	 //freespace is parts between writepointer and end-of-buffer + the stuff before from 0 -> readpointer
	 return ((_buffer.size - _buffer.wr_ptr) + _buffer.rd_ptr);
	 } else if (_buffer.rd_ptr > _buffer.wr_ptr) {
	 //we asume the writepointer has wrapped, as the readpointer shall never be past the wr_ptr
	 //only space between write-pointer -> read->pointer is free,
	 return (_buffer.rd_ptr - _buffer.wr_ptr);
	 }
	 // zero content
	 return (_buffer.packetSize);

	//return (_buffer.capacity - _buffer.count);
	if (usbBuffer.activeRead < usbBuffer.activeWrite) {
		return (usbBuffer.slots - usbBuffer.activeWrite) + usbBuffer.activeRead;
	} else if (usbBuffer.activeRead > usbBuffer.activeWrite) {
		return (usbBuffer.activeRead - usbBuffer.activeWrite);

	} else {
		//both are equal
		return (usbBuffer.slots);
	}

}

uint16_t getOccupiedSlotsInBuffer() {
	//return _buffer.packetSize - getFreeSlotsInBuffer();
	return usbBuffer.slots - getFreeSlotsInBuffer();
}

//assumes free slots are available!
//getting refrence to new WRITE-ref also increments the slot
BUFFER_RESULT putBuffer(_USB_AUDIO_FRAME **p_pframe) {
	if(!usbBuffer.initialized)
		return BUFFER_ERROR;

	if (getFreeSlotsInBuffer() > 0) {
		//inc write buf
		usbBuffer.activeWrite++;
		//check and wrap
		if (usbBuffer.activeWrite == usbBuffer.slots)
			usbBuffer.activeWrite = 0;
		*p_pframe = &(usbBuffer.frames[usbBuffer.activeWrite]);
	}

	return BUFFER_OK;
}

//assumes the requested data is available slots are available!
//doesn't increment pointer!!
BUFFER_RESULT getBuffer(_USB_AUDIO_FRAME **p_pframe) {
	if(!usbBuffer.initialized)
			return BUFFER_ERROR;

	if (getOccupiedSlotsInBuffer() > 0) {
		*p_pframe = &(usbBuffer.frames[usbBuffer.activeRead]);

		return BUFFER_OK;
	} else {
		return BUFFER_ERROR;
	}
}

BUFFER_RESULT flagDataAsRead() {
	if (getOccupiedSlotsInBuffer() > 0) {
		usbBuffer.activeRead++;

		if (usbBuffer.activeRead == usbBuffer.slots)
			usbBuffer.activeRead = 0;

		return BUFFER_OK;
	} else {
		return BUFFER_ERROR;
	}
}


 for (uint16_t i = 0; i < size; i++) {
 if (_buffer.rd_ptr != _buffer.packetSize) {
 //we have not reached end of buffer yet
 _buffer.rd_ptr++;
 } else {
 //we are at the end, wrap it.
 _buffer.rd_ptr = 0;
 }
 //write value
 data[i] = _buffer.data[_buffer.rd_ptr];

 }

 return BUFFER_OK;
 }

*/
