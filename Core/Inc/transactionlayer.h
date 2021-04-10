/*
 * transactionlayer.h
 *
 *  Created on: 3 Apr 2021
 *      Author: vtjgi
 */

#ifndef INC_TRANSACTIONLAYER_H_
#define INC_TRANSACTIONLAYER_H_


//types
struct {
	uint8_t cmd;
	uint8_t lingo;
	uint16_t transId;
	uint8_t writePtr;
	uint8_t buffer[128];
} response_state;

//protos

void processInbound(uint8_t *pkg, uint8_t len);
void transmitToAcc();
void addResponsePayload(uint8_t *bytes, uint8_t len);
void initResponse(uint8_t lingo, uint8_t cmd, uint16_t transId);

#endif /* INC_TRANSACTIONLAYER_H_ */
