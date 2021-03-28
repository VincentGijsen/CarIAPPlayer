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
volatile AUDIO_CircularBuffer_t _buffer ;// __attribute__((section("ccmram")));
;
;

BUFFER_RESULT initBuffer() {
	_buffer.pRead = 0;
	_buffer.pWrite = 0;
	_buffer.slots = AUDIO_CircularBuffer_SLOTS;
	_buffer.Isinitialized = 1;

	return BUFFER_OK;
}

uint16_t getFreeSlotsInBuffer() {

	if (!_buffer.Isinitialized) {
		return 0;
	}

	if (_buffer.pRead < _buffer.pWrite) {
		//freespace is parts between writepointer and end-of-buffer + the stuff before from 0 -> readpointer
		return ((_buffer.slots - _buffer.pWrite) + _buffer.pRead);
	} else if (_buffer.pRead > _buffer.pWrite) {
		//we asume the writepointer has wrapped, as the readpointer shall never be past the wr_ptr
		//only space between write-pointer -> read->pointer is free,
		return (_buffer.pRead - _buffer.pWrite);
	} else {
		//(_buffer.pRead == _buffer.pWrite)
		return _buffer.slots;
	}

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
BUFFER_RESULT putBufferAs8Bytes(uint8_t *pcmSamples, uint8_t len){
	if (!_buffer.Isinitialized)
			return BUFFER_ERROR;

		//th e first time, we start at frame 1
		_guard_write_and_increment();

	for (uint8_t x = 0; x < len; x++) {
		_buffer.data[_buffer.pWrite].frame[x] = pcmSamples[x];//MSB;
		//_buffer.data[_buffer.pWrite].frame[(x * 2) + 1] = LSB;

	}
	_buffer.data[_buffer.pWrite].len = (len); //we doubled # of frames as 16bit ->2x 8bit

	return BUFFER_OK;
}


/*
 * Function converts PCM data (16byte), to two bytes of 8uint8, as used by USB
 * //assumes free slots are available!
 * //getting refrence to new WRITE-ref also increments the slot
 *
 */
BUFFER_RESULT putBuffer(uint16_t *pcmSamples, uint8_t len,
		uint8_t bytesPerSample) {
	if (!_buffer.Isinitialized)
		return BUFFER_ERROR;

	//th e first time, we start at frame 1
	_guard_write_and_increment();

#define MSB (pcmSamples[x] & 0xff)
#define LSB ((pcmSamples[x] >> 8)&0xff)


	for (uint8_t x = 0; x < len; x++) {
		_buffer.data[_buffer.pWrite].frame[(x * 2)] = MSB;
		_buffer.data[_buffer.pWrite].frame[(x * 2) + 1] = LSB;

	}
	_buffer.data[_buffer.pWrite].len = (len * bytesPerSample); //we doubled # of frames as 16bit ->2x 8bit

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

