#include <algorithm>
#include <stdio.h>
#include <string.h>

typedef unsigned long dword;
typedef unsigned short word;
typedef unsigned char byte;

#define AppendCrc CrcSum

//---------------------------------------------------------------------------------------
#ifdef WIN32

#include <windows.h>

HANDLE OpenPort(const char *name)
{
	HANDLE handle = CreateFile(name, FILE_ALL_ACCESS, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (handle == INVALID_HANDLE_VALUE) {
		handle = NULL;
		return handle;
	}

	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = 100;

	timeouts.ReadTotalTimeoutMultiplier = 20;
	timeouts.ReadTotalTimeoutConstant = 20;

	timeouts.WriteTotalTimeoutMultiplier = 20;
	timeouts.WriteTotalTimeoutConstant = 20;

	SetCommTimeouts(handle, &timeouts);

	DWORD read_bytes = 1;
	BYTE buff[0x10];
	while (read_bytes)
		ReadFile(handle, &buff, 0x10, &read_bytes, 0);

	return handle;
}

bool SetPortBaudrate(HANDLE port_handle, dword baudRate)
{
	DCB dcb;
	FillMemory(&dcb, sizeof(dcb), 0);

	if (!GetCommState(port_handle, &dcb)) {
		return false;
	}

	dcb.BaudRate = baudRate;
	dcb.ByteSize = 8;
	dcb.StopBits = ONESTOPBIT;
	dcb.Parity = NOPARITY;

	if (!SetCommState(port_handle, &dcb)) {
		return false;
	}

	return true;
}

void ClosePort(HANDLE port_handle)
{
	if (port_handle != NULL) {
		CloseHandle(port_handle);
	}
}

//---------------------------------------------------------------------------------------
#else

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

typedef int HANDLE;

void Sleep(dword n)
{
	struct timespec t;
	t.tv_sec = n / 1000;
	t.tv_nsec = (n % 1000) * 1000000;
	nanosleep(&t, 0);
	//millisleep( i );
}

HANDLE OpenPort(const char *name)
{
	int fd;

	fd = open(name, O_RDWR | O_NONBLOCK, 0);

	if (fd > 0) {
		Sleep(10);
		fcntl(fd, F_SETFL, 0);
	}

	if (fd < 0)
		fd = 0;
	return fd;
}

bool SetPortBaudrate(HANDLE port_handle, dword baudRate)
{
	termios tio;
	int baud = B9600;

	if (baudRate == 115200)
		baud = B115200;
	else if (baudRate == 921600)
		baud = B921600;

	tcgetattr(port_handle, &tio);
	tio.c_iflag = IGNBRK | IGNPAR | IXOFF | 0 | 0;
	tio.c_cflag = CREAD | HUPCL | CLOCAL | CS8 | 0;
	tio.c_oflag = 0;
	tio.c_lflag = 0;
	cfsetispeed(&tio, baud);
	cfsetospeed(&tio, baud);

	if (tcsetattr(port_handle, TCSAFLUSH, &tio) == -1)
		return false;
	else
		return true;
}

void ClosePort(HANDLE port_handle)
{
	close(port_handle);
}

void ReadFile(HANDLE port_handle, void *buff, dword size, dword *read_bytes, dword size2)
{
	*read_bytes = 0;
	byte *buffer = (byte *) buff;

	while (size > 0) {
		int res = read(port_handle, buffer, size);
		if (res <= 0)
			break;

		buffer += res;
		size -= res;
		*read_bytes += res;
	}
}

void WriteFile(HANDLE port_handle, const void *buff, dword size, dword *read_bytes, dword size2)
{
	write(port_handle, buff, size);
}

#endif
//---------------------------------------------------------------------------------------

dword StrToInt(char *str)
{
	int len = strlen(str);
	int base = 10;
	dword result = 0;

	if (len > 2 && str[0] == '0' && str[1] == 'x') {
		base = 16;
		str += 2;
	}

	while (*str != 0) {
		result *= base;

		if (*str >= '0' && *str <= '9')
			result += *str - '0';
		else if (base == 16 && *str >= 'a' && *str <= 'f')
			result += 10 + (*str - 'a');

		str++;
	}

	return result;
}

bool GetAnswer(HANDLE port_handle, int delay = 10)
{
	for (int i = 0; i < delay; i++) {
		dword res;

		byte answer;
		ReadFile(port_handle, &answer, 1, &res, 0);

		if (res == 1) {
			if (answer == 0x75)
				return true;
			else
				return false;
		}

		Sleep(100);
	}

	return false;
}

bool InitDevice(HANDLE port_handle)
{
	dword res;

	printf("initializing device..");

	byte initByte = 0x7f;
	WriteFile(port_handle, &initByte, 1, &res, 0);
	if (!GetAnswer(port_handle)) {
		puts("\b\b - error");
		return false;
	}

	puts("\b\b - OK  ");
	return true;
}

bool EraseSector(HANDLE port_handle, int sectorNumber)
{
	dword res;
	byte header[0x10];

	printf("erasing sector 0x%.2x..", sectorNumber);

	header[0] = 0x71;
	WriteFile(port_handle, header, 1, &res, 0);
	if (!GetAnswer(port_handle))
		goto eraseError;
	if (!GetAnswer(port_handle))
		goto eraseError;

	//------------------------------------------------------------

	header[0] = 0x43;
	WriteFile(port_handle, header, 1, &res, 0);
	if (!GetAnswer(port_handle))
		goto eraseError;

	header[0] = 0;
	header[1] = (byte) sectorNumber;
	WriteFile(port_handle, header, 2, &res, 0);
	if (!GetAnswer(port_handle, 100))
		goto eraseError;

	puts("\b\b - OK  ");
	return true;

eraseError:
	puts("\b\b - error");
	return false;
}

const int SECTORS_DESCRIPTORS = 10;
dword sectorsDescriptors[SECTORS_DESCRIPTORS][3] = {
	{ 0x00, 0x20000000, 0x2000 },
	{ 0x01, 0x20002000, 0x2000 },
	{ 0x02, 0x20004000, 0x2000 },
	{ 0x03, 0x20006000, 0x2000 },
	{ 0x04, 0x20008000, 0x8000 },
	{ 0x05, 0x20010000, 0x10000 },
	{ 0x06, 0x20020000, 0x10000 },
	{ 0x07, 0x20030000, 0x10000 },
	{ 0x10, 0x200c0000, 0x2000 },
	{ 0x11, 0x200c2000, 0x2000 }
};

bool EraseAffectedSectors(HANDLE port_handle, dword addr, dword size)
{
	bool result = true;

	dword programBegin = addr;
	dword programEnd = addr + size - 1;

	for (int i = 0; i < SECTORS_DESCRIPTORS && result; i++) {
		dword sectorBegin = sectorsDescriptors[i][1];
		dword sectorEnd = sectorBegin + sectorsDescriptors[i][2] - 1;

		if ((sectorBegin >= programBegin && sectorBegin <= programEnd) || (sectorEnd >= programBegin && sectorEnd <= programEnd)) {
			result = EraseSector(port_handle, (int) sectorsDescriptors[i][0]);
		}
	}

	return result;
}

bool EraseDevice(HANDLE port_handle)
{
	dword res;
	byte header[0x10];

	printf("erasing device..");

	header[0] = 0x71;
	WriteFile(port_handle, header, 1, &res, 0);
	if (!GetAnswer(port_handle))
		goto eraseError;
	if (!GetAnswer(port_handle))
		goto eraseError;

	//------------------------------------------------------------

	header[0] = 0x43;
	WriteFile(port_handle, header, 1, &res, 0);
	if (!GetAnswer(port_handle))
		goto eraseError;

	header[0] = 8 - 1;
	for (byte code = 0; code < 0x08; code++)
		header[1 + code] = code;
	WriteFile(port_handle, header, 9, &res, 0);
	if (!GetAnswer(port_handle, 100))
		goto eraseError;

	puts("\b\b - OK  ");
	return true;

eraseError:
	puts("\b\b - error");
	return false;
}

bool WriteBlock(HANDLE port_handle, dword addr, const byte *buff, dword size)
{
	dword res;
	byte header[0x10];
	dword fullSize = size;

	printf("writting");
	printf(" %lu bytes..  0%%", size);

	while (size > 0) {
		header[0] = 0x31;
		WriteFile(port_handle, header, 1, &res, 0);
		if (!GetAnswer(port_handle))
			goto writeError;

		header[0] = (addr >> 24) & 0xff;
		header[1] = (addr >> 16) & 0xff;
		header[2] = (addr >> 8) & 0xff;
		header[3] = (addr >> 0) & 0xff;
		WriteFile(port_handle, header, 4, &res, 0);
		if (!GetAnswer(port_handle))
			goto writeError;

		dword tSize = size < 256 ? size : 256;

		header[0] = (byte)(tSize - 1);
		WriteFile(port_handle, header, 1, &res, 0);
		WriteFile(port_handle, buff, tSize, &res, 0);
		if (!GetAnswer(port_handle))
			goto writeError;

		addr += tSize;
		buff += tSize;
		size -= tSize;

		printf("\b\b\b\b");
		printf("%3d%%", (int) ((fullSize - size) * 100 / fullSize));
		fflush(stdout);
	}

	printf("\b\b\b\b");
	puts("\b\b - OK  ");
	return true;

writeError:
	printf("\b\b\b\b");
	puts("\b\b - error");
	return false;
}

bool ReadBlock(HANDLE port_handle, dword addr, byte *buff, dword size)
{
	dword res;
	byte header[0x10];

	dword fullSize = size;

	printf("reading");
	printf(" %lu bytes..  0%%", size);

	while (size > 0) {
		header[0] = 0x11;
		WriteFile(port_handle, header, 1, &res, 0);
		if (!GetAnswer(port_handle))
			goto readError;

		header[0] = (addr >> 24) & 0xff;
		header[1] = (addr >> 16) & 0xff;
		header[2] = (addr >> 8) & 0xff;
		header[3] = (addr >> 0) & 0xff;
		WriteFile(port_handle, header, 4, &res, 0);
		if (!GetAnswer(port_handle))
			goto readError;

		dword tSize = size < 256 ? size : 256;

		header[0] = (byte)(tSize - 1);
		WriteFile(port_handle, header, 1, &res, 0);
		ReadFile(port_handle, buff, tSize, &res, 0);
		if (res != tSize)
			goto readError;

		addr += tSize;
		buff += tSize;
		size -= tSize;

		printf("\b\b\b\b");
		printf("%3d%%", (int) ((fullSize - size) * 100 / fullSize));
		fflush(stdout);
	}

	printf("\b\b\b\b");
	puts("\b\b - OK  ");
	return true;

readError:
	printf("\b\b\b\b");
	puts("\b\b - error");
	return false;
}

int main(int argc, char *argv[])
{
	char port_name[0x100] = "COM1";
	int port_baudrate = 115200;

	char file_name[0x100] = "default.bin";
	int errors_numder = 0;

	puts("str750 prog v1.2\n");

	bool cmd_write = false;
	bool cmd_verify = false;
	dword address = 0x20000000;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--port") == 0)
			strcpy(port_name, argv[++i]);
		else if (strcmp(argv[i], "--baudrate") == 0)
			port_baudrate = atoi(argv[++i]);
		else if (strcmp(argv[i], "--write") == 0)
			cmd_write = true;
		else if (strcmp(argv[i], "--verify") == 0)
			cmd_verify = true;
		else if (strcmp(argv[i], "--address") == 0 && i < argc - 1)
			address = StrToInt(argv[++i]);
		else
			snprintf(file_name, sizeof(file_name), argv[i]);
	}

	FILE *file = NULL;
	dword file_size;
	byte *buff = NULL;

	HANDLE port_handle = OpenPort(port_name);
	if (!port_handle) {
		printf("error: cannot open port %s\n", port_name);
		errors_numder++;
		goto exit_prog;
	}

	SetPortBaudrate(port_handle, port_baudrate);
	if (errors_numder)
		goto exit_prog;

	file = fopen(file_name, "rb");
	if (file == NULL) {
		printf("cannot open file %s..\n", file_name);
		errors_numder++;
		goto exit_prog;
	}

	fseek(file, 0, SEEK_END);
	file_size = ftell(file);

	buff = new byte[file_size];
	fseek(file, 0, SEEK_SET);
	fread(buff, file_size, 1, file);

	if (!InitDevice(port_handle))
		errors_numder++;

	if (cmd_write && errors_numder == 0) {
		if (EraseAffectedSectors(port_handle, address, file_size)) {
			if (!WriteBlock(port_handle, address, buff, file_size))
				errors_numder++;
		}
		else
			errors_numder++;
	}

	if (cmd_verify && errors_numder == 0) {
		byte *buffTemp = new byte[file_size];

		if (ReadBlock(port_handle, address, buffTemp, file_size)) {
			if (memcmp(buff, buffTemp, file_size) != 0)
				errors_numder++;
		}
		else
			errors_numder++;

		delete buffTemp;
	}

exit_prog:
	if (port_handle)
		ClosePort(port_handle);
	if (file)
		fclose(file);
	if (buff)
		delete buff;

	if (!errors_numder)
		puts("done without errors..\n");
	else {
		printf("done with %d error(s)..\n\n", errors_numder);
		getchar();
	}

	return 0;
}
