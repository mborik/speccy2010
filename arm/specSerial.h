#ifndef SPEC_SERIAL_H_INCLUDED
#define SPEC_SERIAL_H_INCLUDED

#ifdef __cplusplus
#include "types.h"
#include "fatfs/ff.h"

#define X_BLK_SIZE 128

class XModem {
private:
	FIL *dest;

	int  data; // holds readed byte
	byte blockNo; // expected block number
	int  retries; // retry counter for NACK
	byte buffer[X_BLK_SIZE]; // buffer
	bool repeatedBlock; // repeated block flag
	byte errorCode;

	int  recvChar(int msDelay);
	word crc16_ccitt(byte *buf, int size);
	bool dataAvail(int delay);
	int  dataRead(int delay);
	void dataWrite(byte symbol);
	bool receiveFrameNo(void);
	bool receiveData(void);
	bool checkCrc(void);
	bool checkChkSum(void);
	bool receiveFrames(bool crc);
	bool sendNack(void);
	byte generateChkSum(void);

public:
	XModem(FIL *file);
	void init(void);
	char receive();
};
#endif

void Serial_Routine();
void __TRACE(const char *str, ...);

#endif
