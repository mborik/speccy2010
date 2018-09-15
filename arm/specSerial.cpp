#include "system.h"
#include "specSerial.h"
#include "specConfig.h"

const int MAX_CMD_SIZE = 0x20;
char cmd[MAX_CMD_SIZE + 1];
int cmdSize = 0;
bool traceNewLine = false;

enum { X_SOH = 1, X_EOT = 4, X_ACK = 6, X_NACK = 21, X_CAN = 24 };
enum { NOERR = 0, ERR_MAXRETRY, ERR_TIMEOUT, ERR_CANCELLED, ERR_UNKNOWN };

// delay when receive bytes in frame
#define X_RECEIVEDELAY 7000
// retry limit when receiving
#define X_RCVRETRYLIMIT  10

XModem::XModem(FIL *file)
{
	dest = file;
}
void XModem::init(void)
{
	data = -1;
	retries = 0;
	blockNo = 0;
	errorCode = 0;

	f_lseek(dest, 0);
}
int XModem::recvChar(int msDelay)
{
	int cnt = 0;
	while (cnt < msDelay) {
		if (uart0.GetRxCntr() > 0)
			return uart0.ReadByte();

		DelayMs(1);
		cnt++;
	}
	return -1;
}
bool XModem::dataAvail(int delay)
{
	if (data != -1)
		return true;
	return ((data = recvChar(delay)) != -1);
}
int XModem::dataRead(int delay)
{
	int b;
	if (data != -1) {
		b = data;
		data = -1;
		return b;
	}
	return recvChar(delay);
}
void XModem::dataWrite(byte symbol)
{
	uart0.WriteFile((byte *) &symbol, 1);
}
bool XModem::receiveFrameNo()
{
	byte num = (byte) dataRead(X_RECEIVEDELAY);
	byte invnum = (byte) dataRead(X_RECEIVEDELAY);

	repeatedBlock = false;
	// check for repeated block
	if (invnum == (255 - num) && num == blockNo - 1) {
		repeatedBlock = true;
		return true;
	}

	return (num == blockNo && invnum == (255 - num));
}
bool XModem::receiveData()
{
	for (int i = 0; i < X_BLK_SIZE; i++) {
		int byte = dataRead(X_RECEIVEDELAY);
		if (byte != -1)
			buffer[i] = byte;
		else
			return false;
	}
	return true;
}
bool XModem::checkCrc()
{
	word frame_crc = ((byte) dataRead(X_RECEIVEDELAY)) << 8;

	frame_crc |= (byte) dataRead(X_RECEIVEDELAY);
	// now calculate crc on data
	word crc = crc16_ccitt(buffer, X_BLK_SIZE);

	return (frame_crc == crc);
}
bool XModem::checkChkSum()
{
	byte frame_chksum = (byte) dataRead(X_RECEIVEDELAY);

	// calculate chksum
	byte chksum = 0;
	for (int i = 0; i < X_BLK_SIZE; i++) {
		chksum += buffer[i];
	}
	return (frame_chksum == chksum);
}
bool XModem::sendNack()
{
	dataWrite(X_NACK);
	retries++;
	return (retries < X_RCVRETRYLIMIT);
}
bool XModem::receiveFrames(bool crc)
{
	UINT r;

	blockNo = 1;
	retries = 0;
	errorCode = 0;

	while (true) {
		char cmd = dataRead(1000);
		switch (cmd) {
			case X_SOH:
				if (!receiveFrameNo()) {
					if (sendNack())
						break;
					else {
						errorCode = ERR_MAXRETRY;
						return false;
					}
				}
				if (!receiveData()) {
					if (sendNack())
						break;
					else {
						errorCode = ERR_MAXRETRY;
						return false;
					}
				};
				if (crc) {
					if (!checkCrc()) {
						if (sendNack())
							break;
						else {
							errorCode = ERR_MAXRETRY;
							return false;
						}
					}
				}
				else {
					if (!checkChkSum()) {
						if (sendNack())
							break;
						else {
							errorCode = ERR_MAXRETRY;
							return false;
						}
					}
				}
				//callback
				if (!repeatedBlock)
					f_write(dest, buffer, X_BLK_SIZE, &r);
				//ack
				dataWrite(X_ACK);
				if (!repeatedBlock)
					blockNo++;
				break;

			case X_EOT:
				dataWrite(X_ACK);
				return true;

			case X_CAN:
				// wait second CAN
				if (dataRead(X_RECEIVEDELAY) == X_CAN) {
					dataWrite(X_ACK);
					errorCode = ERR_CANCELLED;
					return false;
				}
				// something wrong
				errorCode = ERR_UNKNOWN;
				dataWrite(X_CAN);
				dataWrite(X_CAN);
				dataWrite(X_CAN);
				return false;

			default:
				retries++;
				if (retries < X_RCVRETRYLIMIT) {
					// something wrong
					errorCode = ERR_MAXRETRY;
					dataWrite(X_CAN);
					dataWrite(X_CAN);
					dataWrite(X_CAN);
					return false;
				}
		}
	}
}
char XModem::receive()
{
	for (int i = 0; i < 16; i++) {
		dataWrite('C');
		if (dataAvail(1000)) {
			if (receiveFrames(true))
				return 1;
			else {
				__TRACE("\nXMODEM error code: %d", errorCode);
				return 0;
			}
		}
	}
	for (int i = 0; i < 16; i++) {
		dataWrite(X_NACK);
		if (dataAvail(1000)) {
			if (receiveFrames(false))
				return 1;
			else {
				__TRACE("\nXMODEM error code: %d", errorCode);
				return 0;
			}
		}
	}
	return -1;
}
word XModem::crc16_ccitt(byte *buf, int size)
{
	word crc = 0;
	while (--size >= 0) {
		int i;
		crc ^= (word) *buf++ << 8;
		for (i = 0; i < 8; i++)
			if (crc & 0x8000)
				crc = crc << 1 ^ 0x1021;
			else
				crc <<= 1;
	}
	return crc;
}
byte XModem::generateChkSum(void)
{
	// calculate chksum
	byte chksum = 0;
	for (int i = 0; i < X_BLK_SIZE; i++) {
		chksum += buffer[i];
	}
	return chksum;
}

//---------------------------------------------------------------------------------------
void Serial_Routine()
{
	while (uart0.GetRxCntr() > 0) {
		char temp = uart0.ReadByte();

		if (temp == 0x0a) {
			cmd[cmdSize] = 0;
			cmdSize = 0;

			if (strcmp(cmd, "stack") == 0)
				TestStack();
			else if (strcmp(cmd, "update rom") == 0)
				Spectrum_UpdateConfig(true);
			else if (strcmp(cmd, "reset") == 0)
				while (true); // dihalt
			else if (strncmp(cmd, "video ", 6) == 0 && cmd[6] >= '1' && cmd[6] <= '5') {
				specConfig.specVideoMode = cmd[6] - '1';
				Spectrum_UpdateConfig();
			}
			else
				__TRACE("cmd: %s\n", cmd);
		}
		else if (temp == 0x08) {
			if (cmdSize > 0) {
				cmdSize--;

				const char delStr[2] = { 0x20, 0x08 };
				uart0.WriteFile((byte *) delStr, 2);
			}
		}
		else if (temp != 0x0d && cmdSize < MAX_CMD_SIZE) {
			cmd[cmdSize++] = temp;
		}
	}
}

void __TRACE(const char *str, ...)
{
	static char fullStr[0x80];

	va_list ap;
	va_start(ap, str);
	vsniprintf(fullStr, sizeof(fullStr), str, ap);
	va_end(ap);

	if (traceNewLine) {
		Serial_Routine();
		const char delChar = 0x08;
		for (int i = 0; i <= cmdSize; i++)
			uart0.WriteFile((byte *) &delChar, 1);
	}

	char lastChar = 0;
	char *strPos = fullStr;

	while (*strPos != 0) {
		lastChar = *strPos++;

		if (lastChar == '\n')
			uart0.WriteFile((byte *) "\r\n", 2);
		else
			uart0.WriteFile((byte *) &lastChar, 1);

		WDT_Kick();
	}

	traceNewLine = (lastChar == '\n');

	if (traceNewLine) {
		uart0.WriteFile((byte *) ">", 1);

		WDT_Kick();
		Serial_Routine();

		if (cmdSize > 0)
			uart0.WriteFile((byte *) cmd, cmdSize);
	}
}
