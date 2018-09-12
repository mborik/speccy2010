#include "system.h"
#include "specSerial.h"
#include "specConfig.h"

const int MAX_CMD_SIZE = 0x20;
char cmd[MAX_CMD_SIZE + 1];
int cmdSize = 0;
bool traceNewLine = false;

bool receivingFile = false;
dword receivedBytes = 0;

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

void Serial_InitReceiver(FIL *dest)
{
	receivingFile = true;
	receivedBytes = 0;

	f_lseek(dest, 0);
}

dword Serial_ReceiveBytes(FIL *dest, int msDelay)
{
	UINT r;
	byte temp;

	int cnt = 0;
	while (cnt < msDelay) {
		if (uart0.GetRxCntr() > 0) {
			temp = uart0.ReadByte();
			f_write(dest, &temp, 1, &r);
			receivedBytes++;
			break;
		}

		DelayUs(1);
		cnt++;
	}

	return receivedBytes;
}

dword Serial_CloseReceiver(FIL *dest)
{
	receivingFile = false;
	f_close(dest);

	return receivedBytes;
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
