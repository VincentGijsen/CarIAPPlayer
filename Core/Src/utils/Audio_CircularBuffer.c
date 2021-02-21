/*

 * Audio_CircularBuffer.c
 *
 *  Created on: Feb 16, 2021
 *      Author: vtjgi
 */

#include "utils.h"
//#include "FreeRTOS.h"

//prototypes
BUFFER_RESULT _putSinglePkg(void *pkg);

//global data structures
static volatile AUDIO_CircularBuffer_t _buffer;

//#define CIRC_BUFFER_SIZE 1540 //192 *8 + some more // ((MAX_freq * CHANNELS *BYTESPerFrame )
//static uint8_t _actual_buffer[CIRC_BUFFER_SIZE];

//volatile _USB_AUDIO_BUFFER usbBuffer;

BUFFER_RESULT initBuffer() {
	_buffer.pRead = 0;
	_buffer.pWrite = 0;
	_buffer.slots = AUDIO_CircularBuffer_SLOTS;
//	_buffer.data = _actual_buffer;
	_buffer.Isinitialized = 1;
//	usbBuffer.activeRead = 0;
//	usbBuffer.activeWrite = 0;
//	usbBuffer.slots = _USB_AUDIO_BUFFER_SLOTS;
//	usbBuffer.initialized = 1;
	return BUFFER_OK;
}

/*BUFFER_RESULT getPackageFromBuffer(uint8_t *data) {
 if (!_buffer.Isinitialized) {
 return BUFFER_ERROR;

 if (_buffer.pRead == _buffer.pWrite) {
 return BUFFER_UNDERRUN;
 }

 }
 }*/

uint16_t getFreeSlotsInBuffer() {

	if (!_buffer.Isinitialized) {
		return 0;
	}

	if (_buffer.pRead < _buffer.pWrite) {
		//freespace is parts between writepointer and end-of-buffer + the stuff before from 0 -> readpointer
		return ((_buffer.slots - _buffer.pWrite) + _buffer.pRead - 1);
	} else if (_buffer.pRead > _buffer.pWrite) {
		//we asume the writepointer has wrapped, as the readpointer shall never be past the wr_ptr
		//only space between write-pointer -> read->pointer is free,
		return (_buffer.pRead - _buffer.pWrite - 1);
	} else {
		//(_buffer.pRead == _buffer.pWrite)
		return _buffer.slots;
	}
	// zero content
	//return (_buffer.packetSize);

	/*//return (_buffer.capacity - _buffer.count);
	 if (usbBuffer.activeRead < usbBuffer.activeWrite) {
	 return (usbBuffer.slots - usbBuffer.activeWrite) + usbBuffer.activeRead;
	 } else if (usbBuffer.activeRead > usbBuffer.activeWrite) {
	 return (usbBuffer.activeRead - usbBuffer.activeWrite);

	 } else {
	 //both are equal
	 return (usbBuffer.slots);
	 }*/

}

uint16_t getOccupiedSlotsInBuffer() {
	return _buffer.slots - getFreeSlotsInBuffer();
	//return usbBuffer.slots - getFreeSlotsInBuffer();
}
void _guard_write_and_increment() {
	if ((_buffer.pWrite + 1) == _buffer.slots) {
		_buffer.pWrite = 0;
	} else
		_buffer.pWrite++;

}

//assumes free slots are available!
//getting refrence to new WRITE-ref also increments the slot
BUFFER_RESULT putBuffer(uint16_t *pcmSamples, uint8_t len) {
	if (!_buffer.Isinitialized)
		return BUFFER_ERROR;

	//te first time, we start at frame 1
	_guard_write_and_increment();
	for (uint16_t x = 0; x < len; x++) {
		uint16_t doubleIdx = x * 2;
		_buffer.data[_buffer.pWrite].frame[doubleIdx] = (pcmSamples[x] & 0xff);
		_buffer.data[_buffer.pWrite].frame[doubleIdx + 1] =
				(pcmSamples[x] >> 8);

	}
	_buffer.data[_buffer.pWrite].len = len;

	return BUFFER_OK;
}

//assumes the requested data is available slots are available!
//doesn't increment pointer!!
void _guard_read_and_increment() {
	if ((_buffer.pRead + 1) == _buffer.slots) {
		_buffer.pRead = 0;
	} else
		_buffer.pRead++;

}

BUFFER_RESULT getBuffer(_AUDIO_FRAME volatile **ptr) {
	if (!_buffer.Isinitialized)
		return BUFFER_ERROR;

	/*
	 for (uint16_t x = 0; x < len; (x += 2)) {
	 _guard_read_and_increment();
	 ptr[x] = _buffer.pData[_buffer.pRead];
	 _guard_read_and_increment();
	 ptr[x + 1] = _buffer.pData[_buffer.pRead];
	 }
	 */

	*ptr = &_buffer.data[_buffer.pRead];
	//*len = _buffer.data[_buffer.pRead].len;
	return BUFFER_OK;

}

BUFFER_RESULT flagDataAsRead() {
	if (getOccupiedSlotsInBuffer() > 0) {
		_guard_read_and_increment();
		return BUFFER_OK;
	} else {
		return BUFFER_ERROR;
	}

}

