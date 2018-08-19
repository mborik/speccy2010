#include "specRtc.h"

static byte rtcData[0x30] = {
	// GLUK RTC
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00,
	0x80, 0xaa, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	// MB-02 RTC
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x04
};

const byte RTC_ADDRESS = 0xd0;
const int RTC_TIMEOUT = 100;
const byte RTC_BUSY = 0x02;
const byte RTC_IRQ = 0x04;

tm currentTime;
byte data[8];

bool rtcInited = false;
bool firstTimeInit = true;

void RTC_Init()
{
	currentTime.tm_sec = 0;
	currentTime.tm_min = 0;
	currentTime.tm_hour = 0;

	currentTime.tm_wday = 0;
	currentTime.tm_mday = 0;
	currentTime.tm_mon = 0;
	currentTime.tm_year = 118;

	if (RTC_GetTime(NULL))
		__TRACE("RTC init OK..\n");
	else
		__TRACE("RTC init error..\n");
}

int RTC_FetchData()
{
	int rtc_timer = 0;

	I2C_GenerateSTART(ENABLE);
	while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT) && ++rtc_timer < RTC_TIMEOUT)
		DelayUs(100);

	I2C_Send7bitAddress(RTC_ADDRESS, I2C_MODE_TRANSMITTER);
	while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECTED) && ++rtc_timer < RTC_TIMEOUT)
		DelayUs(100);

	I2C_Cmd(ENABLE);
	I2C_SendData(0);
	while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) && ++rtc_timer < RTC_TIMEOUT)
		DelayUs(100);

	I2C_GenerateSTART(ENABLE);
	while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT) && ++rtc_timer < RTC_TIMEOUT)
		DelayUs(100);

	I2C_Send7bitAddress(RTC_ADDRESS, I2C_MODE_RECEIVER);
	while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECTED) && ++rtc_timer < RTC_TIMEOUT)
		DelayUs(100);

	I2C_Cmd(ENABLE);

	byte address = 0;
	while (address < 7) {
		while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_RECEIVED) && ++rtc_timer < RTC_TIMEOUT)
			DelayUs(100);

		if (address == 5)
			I2C_AcknowledgeConfig(DISABLE);
		else if (address == 6)
			I2C_GenerateSTOP(ENABLE);

		data[address++] = I2C_ReceiveData();
	}

	I2C_AcknowledgeConfig(ENABLE);
	//__TRACE( "RTC read time %d..\n", *rtc_timer );

	return rtc_timer;
}

void RTC_FillCurrentTime()
{
	currentTime.tm_sec = (data[0] >> 4) * 10 + (data[0] & 0x0f);
	currentTime.tm_min = (data[1] >> 4) * 10 + (data[1] & 0x0f);
	currentTime.tm_hour = (data[2] >> 4) * 10 + (data[2] & 0x0f);

	currentTime.tm_wday = (data[3] & 0x0f);
	currentTime.tm_mday = (data[4] >> 4) * 10 + (data[4] & 0x0f);
	currentTime.tm_mon = (data[5] >> 4) * 10 + (data[5] & 0x0f) - 1;
	currentTime.tm_year = 100 + (data[6] >> 4) * 10 + (data[6] & 0x0f);
}

bool RTC_GetTime(tm *time)
{
	if (RTC_FetchData() < RTC_TIMEOUT) {
		RTC_FillCurrentTime();
		rtcInited = true;
	}
	else {
		rtcInited = false;
	}

	if (time)
		memcpy(time, &currentTime, sizeof(tm));
	return rtcInited;
}

bool RTC_SetTime(tm *newTime)
{
	tm time;
	memcpy(&time, newTime, sizeof(tm));

	time.tm_sec = (60 + time.tm_sec) % 60;
	time.tm_min = (60 + time.tm_min) % 60;
	time.tm_hour = (24 + time.tm_hour) % 24;

	time.tm_mon = (12 + time.tm_mon) % 12;

	int mdayMax = 31;
	if (time.tm_mon == 1 && (time.tm_year % 4) != 0)
		mdayMax = 28;
	else if (time.tm_mon == 1)
		mdayMax = 29;
	else if (time.tm_mon == 3 || time.tm_mon == 5)
		mdayMax = 30;
	else if (time.tm_mon == 8 || time.tm_mon == 10)
		mdayMax = 30;

	if (time.tm_mon != currentTime.tm_mon) {
		if (time.tm_mday < 1)
			time.tm_mday = 1;
		else if (time.tm_mday > mdayMax)
			time.tm_mday = mdayMax;
	}
	else {
		time.tm_mday = 1 + (mdayMax + time.tm_mday - 1) % mdayMax;
	}

	mktime(&time);
	memcpy(&currentTime, &time, sizeof(tm));

	if (rtcInited) {
		time.tm_mon += 1;
		time.tm_year %= 100;

		data[0] = ((time.tm_sec / 10) << 4) | (time.tm_sec % 10);
		data[1] = ((time.tm_min / 10) << 4) | (time.tm_min % 10);
		data[2] = ((time.tm_hour / 10) << 4) | (time.tm_hour % 10);
		data[3] = time.tm_wday;
		data[4] = ((time.tm_mday / 10) << 4) | (time.tm_mday % 10);
		data[5] = ((time.tm_mon / 10) << 4) | (time.tm_mon % 10);
		data[6] = ((time.tm_year / 10) << 4) | (time.tm_year % 10);

		byte address = 0;

		I2C_GenerateSTART(ENABLE);
		while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));

		I2C_Send7bitAddress(RTC_ADDRESS, I2C_MODE_TRANSMITTER);
		while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECTED));

		I2C_Cmd(ENABLE);
		I2C_SendData(address);
		while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));

		I2C_Cmd(ENABLE);
		while (address < 7) {
			I2C_SendData(data[address++]);
			while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));
		}

		I2C_GenerateSTOP(ENABLE);
	}

	return rtcInited;
}

byte RTC_ToDec(byte dat)
{
	return ((dat / 10) << 4) | (dat % 10);
}

void RTC_Update()
{
	rtcData[0x0a] |= 0x80;
	SystemBus_Write(0xc0010a, rtcData[0x0a]);

	tm time;
	if (RTC_FetchData() < RTC_TIMEOUT) {
		RTC_FillCurrentTime();
		memcpy(&time, &currentTime, sizeof(tm));
		rtcInited = true;
	}
	else {
		rtcInited = false;
		return;
	}

	rtcData[0] = RTC_ToDec(time.tm_sec);
	rtcData[2] = RTC_ToDec(time.tm_min);
	rtcData[4] = RTC_ToDec(time.tm_hour);
	rtcData[6] = RTC_ToDec(time.tm_wday);
	rtcData[7] = RTC_ToDec(time.tm_mday);
	rtcData[8] = RTC_ToDec(time.tm_mon + 1);
	rtcData[9] = RTC_ToDec(time.tm_year % 100);

	rtcData[0x20] = (data[0] & 0x0f);
	rtcData[0x21] = (data[0] >> 4) & 0x07;
	rtcData[0x22] = (data[1] & 0x0f);
	rtcData[0x23] = (data[1] >> 4) & 0x07;
	rtcData[0x24] = (data[2] & 0x0f);
	rtcData[0x25] = (data[2] >> 4) & 0x07;
	rtcData[0x26] = (data[4] & 0x0f);
	rtcData[0x27] = (data[4] >> 4) & 0x03;
	rtcData[0x28] = (data[5] & 0x0f);
	rtcData[0x29] = (data[5] >> 4) & 0x01;
	rtcData[0x2a] = (data[6] & 0x0f);
	rtcData[0x2b] = (data[6] >> 4);
	rtcData[0x2c] = (data[3] & 0x0f);

	int i;
	if (firstTimeInit) {
		for (i = 0; i < 0x30; i++)
			SystemBus_Write(0xc00100 + i, rtcData[i]);

		firstTimeInit = false;
	}
	else {
		for (i = 0x00; i < 0x0a; i++)
			SystemBus_Write(0xc00100 + i, rtcData[i]);

		rtcData[0x0a] &= ~0x80;
		rtcData[0x2d] = SystemBus_Read(0xc0012d);
		SystemBus_Write(0xc0010a, rtcData[0x0a]);

		int busyWait = 0;
		while (busyWait < RTC_TIMEOUT && (rtcData[0x2d] & RTC_BUSY) != 0) {
			rtcData[0x2d] = SystemBus_Read(0xc0012d);
			busyWait += 5;
			DelayUs(5);
		}

		rtcData[0x2d] |= RTC_IRQ;

		for (i = 0x20; i <= 0x2d; i++)
			SystemBus_Write(0xc00100 + i, rtcData[i]);
	}
}
