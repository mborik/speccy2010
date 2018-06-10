#include "debugger.h"
#include "screen.h"
#include "../system.h"
#include "../system/sdram.h"
#include "../specKeyboard.h"
#include "../specSnapshot.h"

//---------------------------------------------------------------------------------------
dword CalculateAddrAtCursor(word addr, byte CurX, byte CurY)
{
	byte specPort7ffd = SystemBus_Read(0xC00017);
	byte specTrdosFlag = SystemBus_Read(0xC00018);

	byte ROMpage = 0;
	if ((specPort7ffd & 0x10) != 0)
		ROMpage |= 0x01;
	if ((specTrdosFlag & 0x01) == 0)
		ROMpage |= 0x02;

	byte RAMpage = specPort7ffd & 0b00000111;

	dword lineAddr = ((addr & 0xFFF8) + (CurY * 8) + CurX);
	int page;

	if (lineAddr < 0x4000)
		page = 0x100 + ROMpage;
	else if (lineAddr < 0x8000)
		page = 0x05;
	else if (lineAddr < 0xc000)
		page = 0x02;
	else
		page = RAMpage;

	return page * 0x4000 + (lineAddr & 0x3fff);
}
//---------------------------------------------------------------------------------------
byte ReadByteAtCursor(word addr, byte CurX, byte CurY)
{
	return (byte) SystemBus_Read(CalculateAddrAtCursor(addr, CurX, CurY));
}
//---------------------------------------------------------------------------------------
void WriteByteAtCursor(word addr, byte data, byte CurX, byte CurY)
{
	SystemBus_Write(CalculateAddrAtCursor(addr, CurX, CurY), data);
}
//---------------------------------------------------------------------------------------
/*- disassembler window WIP -------------------------------------------------------------
void Debugger_Screen0()
{
	ClrScr(0x07);

	char str[0x20];

	word pc = SystemBus_Read(0xc00001);
	sniprintf(str, sizeof(str), "pc #%.4x", pc);
	WriteStr(0, 1, str);
}
*/
//---------------------------------------------------------------------------------------
//- HEX editor --------------------------------------------------------------------------
void Debugger_Screen1(word addr, byte CurX, byte CurY) //hex editor
{
	char str[0x20];
	for (int i = 0; i < 23; i++) {
		sniprintf(str, sizeof(str), "%.4X", (addr & 0xFFF8) + (i << 3));
		WriteStr(0, i, str);
		for (int j = 0; j < 8; j++) {
			sniprintf(str, sizeof(str), "%.2X ", ReadByteAtCursor(addr, j, i));
			WriteStr(5 + j * 3, i, str);
		}
	}
}
//---------------------------------------------------------------------------------------
void Shell_Debugger()
{
	CPU_Stop();
	SystemBus_Write(0xc00020, 0); // use bank 0

	ClrScr(0007);
	SystemBus_Write(0xc00021, 0x8000 | VIDEO_PAGE); // Enable shell videopage
	SystemBus_Write(0xc00022, 0x8000 | 0); // Enable shell border

	char str[0x20];
	byte CurX = 0, CurY = 0, CurXold, CurYold, half = 0, editAddr = 0, editAddrPos = 3;

	word addr = SystemBus_Read(0xC00001);
	CurX = addr & 0x0007;

	Debugger_Screen1(addr, CurX, CurY);
	WriteAttr(5 + CurX * 3, CurY, 0x17, 2);

	while (true) {
		byte key = GetKey(true);

		if (key == K_ESC)
			break;

		CurXold = CurX;
		CurYold = CurY;

		if (!editAddr) {
			if (key == K_UP) //KEY_UP
			{
				half = 0;
				if (CurY == 0)
					if (addr > 8)
						addr -= 8;
					else
						addr = 0;
				else
					CurY--;
			}
			if (key == K_DOWN) //KEY_DOWN
			{
				half = 0;
				if (CurY == 22)
					if (addr < 0xFF48)
						addr += 8;
					else
						addr = 0xFF48;
				else
					CurY++;
			}
			if (key == K_LEFT) //KEY_LEFT
			{
				half = 0;
				if (CurX > 0)
					CurX--;
			}
			if (key == K_RIGHT) //KEY_RIGHT
			{
				half = 0;
				if (CurX < 7)
					CurX++;
			}
		}
		if ((key >= '0') && (key <= '9')) {
			if (!editAddr) {
				if (!half) {
					WriteByteAtCursor(addr, (ReadByteAtCursor(addr, CurX, CurY) & 0x0F) | ((key - '0') << 4), CurX, CurY);
					half = 1;
				}
				else {
					WriteByteAtCursor(addr, (ReadByteAtCursor(addr, CurX, CurY) & 0xF0) | (key - '0'), CurX, CurY);
					half = 0;
					if (CurX < 7)
						CurX++;
					else {
						CurX = 0;
						if (CurY == 22)
							if (addr < 0xFF48)
								addr += 8;
							else
								addr = 0xFF48;
						else
							CurY++;
					}
				}
			}
			else {
				addr = (addr & ~(0x0F << (editAddrPos * 4))) | ((key - '0') << (editAddrPos * 4));
				editAddrPos--;
				if (editAddrPos == 0xFF) {
					if (addr > 0xFF48)
						addr = 0xFF48;
					editAddr = 0;
					editAddrPos = 3;
					WriteAttr(0, 0, 0x07, 4);
					WriteAttr(5 + CurX * 3, CurY, 0x17, 2);
				}
			}
		}
		if ((key >= 'a') && (key <= 'f')) {
			if (!editAddr) {
				if (!half) {
					WriteByteAtCursor(addr, (ReadByteAtCursor(addr, CurX, CurY) & 0x0F) | ((key - 'a' + 0x0A) << 4), CurX, CurY);
					half = 1;
				}
				else {
					WriteByteAtCursor(addr, (ReadByteAtCursor(addr, CurX, CurY) & 0xF0) | (key - 'a' + 0x0A), CurX, CurY);
					half = 0;
					if (CurX < 7)
						CurX++;
					else {
						CurX = 0;
						if (CurY == 22)
							if (addr < 0xFF48)
								addr += 8;
							else
								addr = 0xFF48;
						else
							CurY++;
					}
				}
			}
			else {
				addr = (addr & ~(0x0F << (editAddrPos * 4))) | ((key - 'a' + 0x0A) << (editAddrPos * 4));
				editAddrPos--;
				if (editAddrPos == 0xFF) {
					if (addr > 0xFF48)
						addr = 0xFF48;
					editAddr = 0;
					editAddrPos = 3;
					WriteAttr(0, 0, 0x07, 4);
					WriteAttr(5 + CurX * 3, CurY, 0x17, 2);
				}
			}
		}
		if (key == 'z') //one step
		{
			SystemBus_Write(0xc00008, 0x01);
			DelayMs(1);
			DiskIF_Routine();

			addr = SystemBus_Read(0xC00001);
			CurX = addr & 0x0007;
			CurY = 0;
		}
		if (key == 's')
			SaveSnapshot(UpdateSnaName());
		if (key == 'm')
			editAddr = 1;
		if (key != 0) {
			if (editAddr) {
				sniprintf(str, sizeof(str), "%.4X", addr & 0xFFF8);
				WriteStr(0, 0, str);
				WriteAttr(0, 0, 0x17, 4);
				WriteAttr(3 - editAddrPos, 0, 0x57, 1);
			}
			else {
				Debugger_Screen1(addr, CurX, CurY);
				WriteAttr(5 + CurXold * 3, CurYold, 0x07, 2);
				WriteAttr(5 + CurX * 3, CurY, 0x17, 2);
			}
		}
	}

	DelayMs(100);

	CPU_Start();

	SystemBus_Write(0xc00021, 0); //armVideoPage
	SystemBus_Write(0xc00022, 0); //armBorder
}
//---------------------------------------------------------------------------------------
