/*
 * lingo_general.c
 *
 *  Created on: 4 Apr 2021
 *      Author: vtjgi
 */
#include "lingos.h"
#include "transactionlayer.h"
#include "tasks.h"

#include "stdint.h"

#include "xprintf.h"

typedef enum {
	UNDEF = 0, StartIDPS_INIT, TRANSPORT_RESET, NEGOTIATE1,

} typeDefTransportStatus;

typeDefTransportStatus lingoGeneral_state = UNDEF;

static uint16_t transactionAuthstartId = 1;
static uint16_t transactionChallengeId = 1;

void processLingoGeneral(IAPmsg msg) {

	switch (msg.commandId) {

	case 0x03: //RequestExtendedInterfaceMode
	{
		xprintf("extended mode requested\n");
		const uint8_t returnExtendedINterfaceMode = 0x04;
		const uint8_t true = 1;
		initResponse(LingoGeneral, returnExtendedINterfaceMode, msg.transID);
		addResponsePayload(&true, sizeof(true));
		transmitToAcc();
	}
		break;

	case 0x07: //RequestIpodName
	{
		const uint8_t returnIpodNameCmd = 0x08;
		const uint8_t name[] = { 'C', 'a', 'r', 'p', 'o', 'd', 0 };
		initResponse(LingoGeneral, returnIpodNameCmd, msg.transID);
		addResponsePayload(&name, sizeof(name));
		transmitToAcc();
	}
		break;

	case 0x0f: //	RequestLingoProtocolVersion
	{
		uint8_t requested_lingo_version = msg.payload[0];
		xprintf("requested lingo version for : %x\n", requested_lingo_version);

		uint8_t pL[3] = { requested_lingo_version, 0, 0 };

		switch (requested_lingo_version) {
		case 0x00: //general
		pL[1] = 1;
		pL[2] = 9;
		break;

	case 0x03: //display remote
		pL[1] = 1;
		pL[2] = 0;
		break;

	case 0x04: //extended lingo
		pL[1] = 1;
		pL[2] = 14;
		break;

	case 0x10: //digital audio
		pL[1] = 1;
		pL[2] = 2;
		break;

	default:
		xprintf("unimplemented lingo-version-req");
		}

		addResponsePayload(&pL, sizeof(pL));
		transmitToAcc();

	}
		break;

	case 0x38: { //startDS
		//p252 MFiAccessoryFirmwareSpecificationR46NoRestriction.
		//STARTIPS process stap
		uint8_t pkg_len = 6; //lingo, cmd, 2(trans) payload(2),
		uint8_t lingo = 0; //general
		uint8_t cmd = 0x02; //ipod ack
		uint8_t cmdstatus = 0x00; //success
		uint8_t origCmdID = 0x38; //origCmdID?
		uint8_t pL[2] = { cmdstatus, origCmdID };

		//USBD_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
		initResponse(lingo, cmd, msg.transID);
		addResponsePayload(&pL, sizeof(pL));
		transmitToAcc();

		xprintf("send IPOD_ACK\n");
		//_transState.lastCmd = UNDEF;
	}
		break;

	case 0x39: //SetFIDTokenValues
	{ //ack with: AckFIDTokenValues 0x03a
		uint8_t numFIDTokenValues = msg.payload[0]; //msg.raw[8];
		uint8_t tokenStartIdx = 1;
		const uint8_t lingogeneric = 0x00;
		const uint8_t cmdId = 0x3a;

		//uint8_t ackTokenTemplate[5];

		initResponse(lingogeneric, cmdId, msg.transID);
		addResponsePayload(numFIDTokenValues, 1);

		for (int x = 0; x < numFIDTokenValues; x++) {
			uint8_t tLeng = msg.payload[tokenStartIdx];
			uint8_t tFidType = msg.payload[tokenStartIdx + 1];
			uint8_t tFidSubType = msg.payload[tokenStartIdx + 2];
			uint8_t tdataStart = msg.payload[tokenStartIdx + 3];

			if (tFidType == 0x00) {
				if (tFidSubType == 0x00) {
					xprintf("IdentityToken : ");
					uint8_t identAck[] = { 0x03, tFidType, tFidSubType, 0 };
					addResponsePayload(identAck, sizeof(identAck));
					xprintf("num lingos: %d\n", tdataStart);
					xprintf("supported lingos: ");
					for (uint8_t it = 0; it < tdataStart; it++) {
						xprintf("%d ", msg.payload[tokenStartIdx + 4 + it]);
					}

					uint8_t devOptIdx = msg.payload[tokenStartIdx + 3
							+ tdataStart + 4];
					xprintf("device options: %b\n", +msg.payload[devOptIdx]);

				} else if (tFidSubType == 0x01) {
					xprintf("AccessoryCapsToken:\n");
					uint8_t accCapAck[] = { 0x03, tFidType, tFidSubType, 0 };
					addResponsePayload(accCapAck, sizeof(accCapAck));
					for (uint8_t x = 0; x < 8; x++) {
						//xprintf("cap b%d: %b \n",
						//		msg.raw[tokenStartIdx + 3 + x]);
					}

				} else if (tFidSubType == 0x02) {

					xprintf("AccessoryInfoToken: ");
					uint8_t accInfoAck[] = { 0x04, tFidType, tFidSubType, 0,
							tdataStart };
					addResponsePayload(accInfoAck, sizeof(accInfoAck));

					if (tdataStart == 0x01) {
						xprintf("name: ");
						uint8_t idx = 1;
						while (idx) {
							char c = msg.payload[tokenStartIdx + 3 + idx];
							xprintf("%c", c);
							if (c == '\0')
								break;
							idx++;
						}

					}
					if (tdataStart == 0x04) {
						xprintf("FW: %x %x %x ",
								msg.payload[tokenStartIdx + 3 + 1],
								msg.payload[tokenStartIdx + 3 + 2],
								msg.payload[tokenStartIdx + 3 + 3]);
					}
					if (tdataStart == 0x06) {
						xprintf("Manufact: ");
						uint8_t idx = 1;
						while (idx) {
							char c = msg.payload[tokenStartIdx + 3 + idx];
							xprintf("%c", c);
							if (c == '\0')
								break;
							idx++;
						}

					}

				} else if (tFidSubType == 0x03) {
					xprintf("iPodPreferenceToken: ");
					uint8_t accipodPrefTokenInfoAck[] = { 0x04, tFidType,
							tFidSubType, 0, tdataStart };
					addResponsePayload(accipodPrefTokenInfoAck,
							sizeof(accipodPrefTokenInfoAck));

				} else if (tFidSubType == 0x04) {
					xprintf("EAProtocolToken: ");
					uint8_t accEapProAcc[] = { 0x04, tFidType, tFidSubType, 0,
							tdataStart };
					addResponsePayload(accEapProAcc, sizeof(accEapProAcc));
				} else {
					xprintf("Other Token: ");
				}
				if (tFidType == 0x00 && tFidSubType == 0x0e) {
					xprintf("AccessoryDigital-AudioSampleRates-Token\n");
				}

			}
			xprintf("len: %d, type: %X, subType: %x \n", tLeng, tFidType,
					tFidSubType);

			//prep for next token
			tokenStartIdx += (tLeng + 1);
		}
		transmitToAcc();
		//send acced tokens: AckFIDTokenValues
		//AckFIDTokenValues0x3a

	}
		break;

	case 0x11: //RequestTransport- Acc to Dev {transID:2} 	MaxPayloadSize
	{ //respond 0x12; {transID:2, maxSize:2}

		//MFiAccessoryFirmwareSpecificationR46NoRestriction p94

		uint8_t pkg_len = 6; //lingo, cmd, 2(trans) payload(2),
		uint8_t lingo = 0; //general
		uint8_t cmd = 0x12; //ipod ack
		uint8_t maxHi = (128 >> 8) & 0xff; //success
		uint8_t maxLo = (128 & 0xff); //origCmdID?

		uint8_t pl[2] = { maxHi, maxLo };
		initResponse(lingo, cmd, msg.transID);
		addResponsePayload(&pl, sizeof(pl));
		transmitToAcc();

	}
		break;

	case 0x4b: //GetiPodOptionsForLingo:
	{
		//GetiPodOptionsForLingo
		const uint8_t RetiPodOptionsForLingoCmd = 0x4c;
		uint8_t reqestedLingoOpts = msg.payload[0];
		uint8_t pkg_len = 14;
		uint8_t caps[9] = { 0 };
		caps[0] = reqestedLingoOpts;

		switch (reqestedLingoOpts) {
		case 0x00: //lingo 0
		{
			caps[1] = (1 << 0);
			caps[2] = 0;
		}
			break;
		case 0x0a: //digital audio lingo
		{
			xprintf("digtal audio supported by pod\n");
			caps[1] = (1 << 0); //digital audio on
		}
			break;

		}

		//uint8_t pl[2] = { maxHi, maxLo };
		initResponse(LingoGeneral, RetiPodOptionsForLingoCmd, msg.transID);
		addResponsePayload(caps, sizeof(caps));
		transmitToAcc();

		return;

	}
		break;

	case 0x3b: //EndIDPS

	{
		uint8_t accEndIDPSStatus = msg.payload[0];
		switch (accEndIDPSStatus) {
		case 0x00: {
			//ok, continue
			xprintf("acc ok, proceed authentication please...\n");
			const uint8_t cmd = 0x3c; //IDPSstatus
			const uint8_t status = 0x00; //we're happy with tokens

			initResponse(LingoGeneral, cmd, msg.transID);
			addResponsePayload(status, 1);
			transmitToAcc();

			//initiate authentication
			typeDefTask task;
			task.f = &taskInitializeAuthentication;
			task.scheduledAfter = uwTick + 100;
			scheduleTasks(task);

		}
			break;

		case 0x01: //request reset ipss
			xprintf("acc requested reset of IDPS\n");
			break;

		default:
			xprintf("issue to connect IDPS\n");
		}
	}
		break;

	case 0x15: //RetAccessoryAuthenticationInfo
	{
		//const uint8_t RetAccessoryAuthenticationInfo = 0;

		//xprintf("accc provided auth info\n");

		if (msg.payload[0] == 0x02 && msg.payload[1] == 0x00) {
			//xprintf("Acc uses authentication 2.0\n");
			//iap 2 auth system
			uint8_t certCurSect = msg.payload[2];
			uint8_t certMaxSect = msg.payload[3];

			if (certCurSect < certMaxSect) {
				xprintf("got partical cert: %d/%d \n", certCurSect,
						certMaxSect);
				//ack for next run, else proceed
				const uint8_t cmdstatus = 0x00; //success
				const uint8_t origCmdID = 0x15; //origCmdID?
				const uint8_t pL[2] = { cmdstatus, origCmdID };

				initResponse(LingoGeneral, 0x02, transactionAuthstartId);
				addResponsePayload(&pL, sizeof(pL));
				transmitToAcc();

			} else { //AckAccessoryAuthenticationInfo

				xprintf("got last part: %d/%d \n", certCurSect, certMaxSect);
				//xprintf("all sections of cert are transmitted\n");

				xprintf("ack all info\n");

				//we pretend, we looked at the certificate, and reply all is ok:
				const uint8_t AckAccessoryAuthenticationInfo = 0x16;
				const uint8_t ok = 0x00;
				initResponse(LingoGeneral, AckAccessoryAuthenticationInfo,
						transactionAuthstartId);
				addResponsePayload(ok, 1);
				transmitToAcc();

				/*//get sample rate
				 xprintf("get samplerate\n");
				 podTransactionCounter++;
				 const uint8_t GetAccessorySampleRateCaps = 0x02;
				 initResponse(LingoGeneral, GetAccessorySampleRateCaps,
				 podTransactionCounter);

				 transmitToAcc();*/
				for (uint8_t x = 0; x < 254; x++) {
					for (uint8_t y = 0; y < 254; y++) {
						asm("nop");
					}
				}
				//send signature req:
				xprintf("ask challenge\n");

				//podTransactionCounter++;
				//DONT_ICNREMENT_TRANSACTION_COUNTER
				const uint8_t GetAccessoryAuthenticationSignature = 0x17;
				uint8_t signatureChallenge[21] = { 0 };
				signatureChallenge[20] = 0;

				initResponse(LingoGeneral, GetAccessoryAuthenticationSignature,
						transactionAuthstartId);
				addResponsePayload(signatureChallenge,
						sizeof(signatureChallenge));
				transmitToAcc();

			}
		} else {
			xprintf("Auth 1.0 not implemented!\n");
		}

	}
		break;

	case 0x18: //RetAccessoryAuthenticationSignature
	{
		//should contain the challenged signature
		xprintf("Challenge Returned :): cmd: %x\n", msg.commandId);
		//podTransactionCounter++;
		const uint8_t AckAuthStatus = 0x19;
		uint8_t authOk = 0x00;

		initResponse(LingoGeneral, AckAuthStatus, transactionChallengeId);
		addResponsePayload(authOk, sizeof(authOk));
		transmitToAcc();

	}
		break;

	default:
		xprintf("unknown General cmnd: %x", msg.commandId);
		;
	}

	return;

}

void taskInitializeAuthentication() {
	//triggerd by timed-event
	xprintf("taskInitializeAuthentication called\n");
	const uint8_t GetAccessoryAuthenicatationInfo = 0x14;

	transactionAuthstartId = podTransactionCounter + 1;
	initResponse(LingoGeneral, GetAccessoryAuthenicatationInfo,
			transactionAuthstartId);
	//no payload
	transmitToAcc();

}
