#include <malloc.h>
#include <new>
#include <stdio.h>
#include <string.h>

#include "system.h"
#include "ps2scancodes.h"

#include "specKeyboard.h"
#include "specKeyMap.h"
#include "specConfig.h"
#include "specSnapshot.h"
#include "specTape.h"

#include "shell/commander.h"
#include "shell/debugger.h"
#include "shell/menu.h"
#include "shell/snapshot.h"

int kbdInited = 0;
int kbdResetTimer = 0;

static byte keyPort[SPEC_KEYS_NUMBER];
static word pressedKeys[4] = { KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE };

word prevKey = KEY_NONE;
dword keyRepeatTime = 0;

byte keybordSend[4];
int keybordSendPos = 0;
int keybordSendSize = 0;

CFifo keyboard(0x10);
CFifo joystick(0x10);

//---------------------------------------------------------------------------------------
word keyFlags = 0;
word ReadKeyFlags() {
	return keyFlags;
}
//---------------------------------------------------------------------------------------
void UpdateKeyPort()
{
	byte *keyPortPtr = keyPort;
	word temp;

	for (int i = 0; i < 4; i++) {
		temp = 0xFFFF;

		if (*keyPortPtr++) temp &= ~0x0001;
		if (*keyPortPtr++) temp &= ~0x0002;
		if (*keyPortPtr++) temp &= ~0x0004;
		if (*keyPortPtr++) temp &= ~0x0008;
		if (*keyPortPtr++) temp &= ~0x0010;

		if (*keyPortPtr++) temp &= ~0x0100;
		if (*keyPortPtr++) temp &= ~0x0200;
		if (*keyPortPtr++) temp &= ~0x0400;
		if (*keyPortPtr++) temp &= ~0x0800;
		if (*keyPortPtr++) temp &= ~0x1000;

		SystemBus_Write(0xc00010 + i, temp);
	}

	temp = 0x0000;

	if (*keyPortPtr++) temp |= 0x0001;
	if (*keyPortPtr++) temp |= 0x0002;
	if (*keyPortPtr++) temp |= 0x0004;
	if (*keyPortPtr++) temp |= 0x0008;
	if (*keyPortPtr++) temp |= 0x0010;

	SystemBus_Write(0xc00014, temp);
}

void ResetKeyboard()
{
	for (int i = 0; i < SPEC_KEYS_NUMBER; i++)
		keyPort[i] = 0;
	UpdateKeyPort();
}

void SetSpecKey(byte code, bool set)
{
	if (code < SPEC_KEYS_NUMBER) {
		if (set && keyPort[code] != 0xff)
			keyPort[code]++;
		else if (!set && keyPort[code] != 0)
			keyPort[code]--;
	}

	UpdateKeyPort();
}
//---------------------------------------------------------------------------------------
void DecodeKeyMatrix(word keyCode, bool flagKeyRelease, const CMatrixRecord *matrix)
{
	for (word code; (code = matrix->keyCode) != KEY_NONE; matrix++) {
		if (code == keyCode) {
			if (!flagKeyRelease) {
				SetSpecKey(matrix->specKeyCode0, true);

				if (matrix->specKeyCode1 != SPEC_KEY_NONE) {
					DelayMs(3);
					SetSpecKey(matrix->specKeyCode1, true);
				}
			}
			else {
				if (matrix->specKeyCode1 != SPEC_KEY_NONE) {
					SetSpecKey(matrix->specKeyCode1, false);
					DelayMs(3);
				}

				SetSpecKey(matrix->specKeyCode0, false);
			}

			break;
		}
	}
}

bool DecodeSymbolShortcut(word keyCode, word keyFlags, const CSymbolShortcutKeyRecord *map)
{
	if (!(keyFlags & fKeyPCEmu))
		return false;

	bool flagKeyRelease = (keyFlags & fKeyRelease);
	word shifts = (keyFlags & (fKeyShift | fKeyCtrl));

	for (word code; (code = map->keyCode) != KEY_NONE; map++) {
		if (code == keyCode) {
			//__TRACE("# keyCode - 0x%04x, 0x%04x[m:0x%04x], ext: %d\n", keyCode, keyFlags, map->ctrlMask, map->extend);

			if ((map->ctrlMask && (shifts & map->ctrlMask)) || (map->ctrlMask == 0 && shifts == 0)) {
				if (map->extend) {
					if (!flagKeyRelease) {
						SetSpecKey(SPEC_KEY_CAPS_SHIFT, true);
						DelayMs(3);
						SetSpecKey(SPEC_KEY_SYMBOL_SHIFT, true);
					}
					else {
						SetSpecKey(SPEC_KEY_SYMBOL_SHIFT, false);
						DelayMs(3);
						SetSpecKey(SPEC_KEY_CAPS_SHIFT, false);
					}

					DelayMs(10);
				}

				if (!flagKeyRelease) {
					SetSpecKey(SPEC_KEY_SYMBOL_SHIFT, true);
					DelayMs(3);
					SetSpecKey(map->specKeyCode, true);
				}
				else {
					SetSpecKey(map->specKeyCode, false);
					DelayMs(3);
					SetSpecKey(SPEC_KEY_SYMBOL_SHIFT, false);
				}

				return true;
			}
		}
	}

	return false;
}

void DecodeKeySpec(word keyCode, word keyFlags)
{
	bool flagKeyRelease = (keyFlags & fKeyRelease) != 0;

	if (!DecodeSymbolShortcut(keyCode, keyFlags, keyMatrixSymbolShortcuts))
		DecodeKeyMatrix(keyCode, flagKeyRelease, keyMatrixMain);

	int joyMode = specConfig.specJoyModeEmulation;

	if ((keyFlags & fKeyJoy1) != 0)
		joyMode = specConfig.specJoyMode1;
	else if ((keyFlags & fKeyJoy2) != 0)
		joyMode = specConfig.specJoyMode2;

	switch (joyMode) {
		case SpecJoy_Kempston:
			DecodeKeyMatrix(keyCode, flagKeyRelease, keyMatrixKempston);
			break;
		case SpecJoy_Cursor:
			DecodeKeyMatrix(keyCode, flagKeyRelease, keyMatrixCursor);
			break;
		case SpecJoy_Sinclair1:
			DecodeKeyMatrix(keyCode, flagKeyRelease, keyMatrixSinclair1);
			break;
		case SpecJoy_Sinclair2:
			DecodeKeyMatrix(keyCode, flagKeyRelease, keyMatrixSinclair2);
			break;
		case SpecJoy_Qaopm:
			DecodeKeyMatrix(keyCode, flagKeyRelease, keyMatrixQaopm);
			break;
	}
}

void DecodeKey(word keyCode, word keyFlags)
{
	bool flagKeyRelease = (keyFlags & fKeyRelease) != 0;
	int videoMode = 0;
	if (!flagKeyRelease) {
		if ((keyFlags & fKeyAlt) != 0) {
			switch (keyCode) {
				case KEY_5:
					videoMode++;
				case KEY_4:
					videoMode++;
				case KEY_3:
					videoMode++;
				case KEY_2:
					videoMode++;
				case KEY_1:
					specConfig.specVideoMode = videoMode;
					Spectrum_UpdateConfig();
					SaveConfig();
					break;

				case KEY_F4:
					SystemBus_Write(0xc00000, 0x00004);
					break;

				case KEY_F5:
					CPU_Reset_Seq();
					break;

				case KEY_F7:
					Shell_SaveSnapshot();
					break;

				case KEY_F8: {
					CPU_Stop();

					byte specPort7ffd = SystemBus_Read(0xc00017);

					byte page = (specPort7ffd & (1 << 3)) != 0 ? 7 : 5;
					dword addr = 0x800000 | (page << 13);

					SystemBus_Write(0xc00020, 0); // use bank 0

					for (int i = 0x1800; i < 0x1b00; i += 2) {
						SystemBus_Write(addr + (i >> 1), 0x3838);
					}

					CPU_Start();
				}
				break;

				case KEY_F9:
					Shell_RomCfgMenu();
					break;

				case KEY_F12:
					Shell_Debugger();
					break;
			}
		}
		else if ((keyFlags & (fKeyAlt | fKeyCtrl)) != 0) {
			if (keyCode >= KEY_F1 && keyCode <= KEY_F10) {
				char snaName[0x10];
				sniprintf(snaName, sizeof(snaName), "!slot_%.1d.sna", keyCode - KEY_F1);
				SaveSnapshot(snaName);
			}
		}
		else {
			switch (keyCode) {
				case KEY_PAUSE:
					WaitKey();
					break;

				case KEY_F1:
					specConfig.specTurbo = specConfig.specTurbo ? 0 : 3;
					Spectrum_UpdateConfig();
					break;

				case KEY_F2:
					specConfig.specTurbo = 1;
					Spectrum_UpdateConfig();
					break;

				case KEY_F3:
					specConfig.specTurbo = 2;
					Spectrum_UpdateConfig();
					break;

				case KEY_F4:
					specConfig.specTurbo = 3;
					Spectrum_UpdateConfig();
					break;

				case KEY_F5:
					CPU_NMI();
					break;

				case KEY_F6:
					if (specConfig.specDiskIf != SpecDiskIf_DivMMC)
						Shell_DisksMenu();
					break;

				case KEY_F7:
					SaveSnapshot(UpdateSnaName());
					break;

				case KEY_F9:
					Shell_SettingsMenu();
					break;

				case KEY_F10:
				case KEY_KP_PLUS:
					if (!Tape_Started())
						Tape_Restart();
					break;

				case KEY_F11:
				case KEY_KP_MINUS:
					if (!Tape_Started())
						Tape_Start();
					else
						Tape_Stop();
					break;

				case KEY_F12:
					Shell_Commander();
					break;

				case KEY_POWER:
					CPU_Reset_Seq();
					break;
			}
		}
	}
}

char CodeToChar(word keyCode)
{
	bool shift = (keyFlags & fKeyShift) != 0;
	bool caps = (keyFlags & fKeyCaps) != 0;
	if (shift)
		caps = !caps;

	switch (keyCode) {
		case KEY_NONE:          return 0;

		case KEY_ESC:           return K_ESC;
		case KEY_RETURN:        return K_RETURN;
		case KEY_BACKSPACE:     return K_BACKSPACE;

		case KEY_UP:            return K_UP;
		case KEY_DOWN:          return K_DOWN;
		case KEY_LEFT:          return K_LEFT;
		case KEY_RIGHT:         return K_RIGHT;

		case KEY_PAGEUP:        return K_PAGEUP;
		case KEY_PAGEDOWN:      return K_PAGEDOWN;
		case KEY_HOME:          return K_HOME;
		case KEY_END:           return K_END;

		case KEY_DELETE:        return K_DELETE;
		case KEY_INSERT:        return K_INSERT;

		case KEY_F1:            return K_F1;
		case KEY_F2:            return K_F2;
		case KEY_F3:            return K_F3;
		case KEY_F4:            return K_F4;
		case KEY_F5:            return K_F5;
		case KEY_F6:            return K_F6;
		case KEY_F7:            return K_F7;
		case KEY_F8:            return K_F8;
		case KEY_F9:            return K_F9;
		case KEY_F10:           return K_F10;
		case KEY_F11:           return K_F11;
		case KEY_F12:           return K_F12;

		case KEY_SPACE:         return ' ';

		case KEY_1:             return !shift ? '1' : '!';
		case KEY_2:             return !shift ? '2' : '@';
		case KEY_3:             return !shift ? '3' : '#';
		case KEY_4:             return !shift ? '4' : '$';
		case KEY_5:             return !shift ? '5' : '%';
		case KEY_6:             return !shift ? '6' : '^';
		case KEY_7:             return !shift ? '7' : '&';
		case KEY_8:             return !shift ? '8' : '*';
		case KEY_9:             return !shift ? '9' : '(';
		case KEY_0:             return !shift ? '0' : ')';

		case KEY_MINUS:         return !shift ? '-' : '_';
		case KEY_EQUALS:        return !shift ? '=' : '+';

		case KEY_SLASH:         return !shift ? '/' : '?';
		case KEY_BACKSLASH:     return !shift ? '\\' : '|';

		case KEY_A:             return !caps ? 'a' : 'A';
		case KEY_B:             return !caps ? 'b' : 'B';
		case KEY_C:             return !caps ? 'c' : 'C';
		case KEY_D:             return !caps ? 'd' : 'D';
		case KEY_E:             return !caps ? 'e' : 'E';
		case KEY_F:             return !caps ? 'f' : 'F';
		case KEY_G:             return !caps ? 'g' : 'G';
		case KEY_H:             return !caps ? 'h' : 'H';
		case KEY_I:             return !caps ? 'i' : 'I';
		case KEY_J:             return !caps ? 'j' : 'J';
		case KEY_K:             return !caps ? 'k' : 'K';
		case KEY_L:             return !caps ? 'l' : 'L';
		case KEY_M:             return !caps ? 'm' : 'M';
		case KEY_N:             return !caps ? 'n' : 'N';
		case KEY_O:             return !caps ? 'o' : 'O';
		case KEY_P:             return !caps ? 'p' : 'P';
		case KEY_Q:             return !caps ? 'q' : 'Q';
		case KEY_R:             return !caps ? 'r' : 'R';
		case KEY_S:             return !caps ? 's' : 'S';
		case KEY_T:             return !caps ? 't' : 'T';
		case KEY_U:             return !caps ? 'u' : 'U';
		case KEY_V:             return !caps ? 'v' : 'V';
		case KEY_W:             return !caps ? 'w' : 'W';
		case KEY_X:             return !caps ? 'x' : 'X';
		case KEY_Y:             return !caps ? 'y' : 'Y';
		case KEY_Z:             return !caps ? 'z' : 'Z';

		case KEY_BACKQUOTE:     return !shift ? '`' : '~';
		case KEY_LEFTBRACKET:   return !shift ? '[' : '{';
		case KEY_RIGHTBRACKET:  return !shift ? ']' : '}';
		case KEY_COMMA:         return !shift ? ',' : '<';
		case KEY_PERIOD:        return !shift ? '.' : '>';
		case KEY_SEMICOLON:     return !shift ? ';' : ':';
		case KEY_QUOTE:         return !shift ? '\'' : '\"';
	}

	return 0;
}
//---------------------------------------------------------------------------------------
void Keyboard_Reset()
{
	// flush the keybuffer...
	word keyCode = SystemBus_Read(0xc00032);
	while ((keyCode & 0x8000) != 0)
		keyCode = SystemBus_Read(0xc00032);

	kbdInited = 0;
	SystemBus_Write(0xc00032, 0xff);

	kbdResetTimer = 0;
	keybordSendPos = 0;
	keybordSendSize = 0;

	keyboard.Clean();
	joystick.Clean();
}

void Keyboard_Send(const byte *data, int size)
{
	if (size <= 0 || size > (int)sizeof(keybordSend))
		return;
	if (kbdInited != KBD_OK)
		return;

	memcpy(keybordSend, data, size);
	keybordSendSize = size;
	keybordSendPos = 0;

	SystemBus_Write(0xc00032, keybordSend[keybordSendPos]);
}

void Keyboard_SendNext(byte code)
{
	if (code == 0xfa)
		keybordSendPos++;
	if (keybordSendPos >= keybordSendSize)
		return;

	SystemBus_Write(0xc00032, keybordSend[keybordSendPos]);
}
//---------------------------------------------------------------------------------------
bool ReadKey(word &_keyCode, word &_keyFlags)
{
	static byte keyPrefix = 0x00;
	static bool keyRelease = false;

	_keyCode = KEY_NONE;
	_keyFlags = keyFlags;

	while (_keyCode == KEY_NONE && keyboard.GetCntr() > 0) {
		word keyCode = keyboard.ReadByte();

		//__TRACE("keyCode - 0x%.2x\n", keyCode);

		if (kbdInited == 0 && keyCode == 0xfa) {
			kbdInited = 1;
		}
		else if (kbdInited == 1 && keyCode == 0xaa) {
			kbdInited = KBD_OK;
			keyFlags = 0;
			keyPrefix = 0x00;
			keyRelease = false;

			__TRACE("PS/2 keyboard init OK..\n");
		}
		else if (kbdInited == KBD_OK) {
			if (keyCode == 0xfa || keyCode == 0xfe)
				Keyboard_SendNext(keyCode);
			else if (keyCode == 0xf0)
				keyRelease = true;
			else if (keyCode == 0xe0 || keyCode == 0xe1) {
				keyPrefix = keyCode;
			}
			else {
				keyCode |= ((word)keyPrefix << 8);
				keyPrefix = 0x00;

				if (keyCode == KEY_PRNTSCR_SKIP || keyCode == KEY_PAUSE_SKIP) {
					keyCode = KEY_NONE;
					keyRelease = false;
					continue;
				}

				//__TRACE("# keyCode - 0x%.4x, 0x%.4x\n", keyCode, (word) keyRelease);

				int i;
				const int pressedKeysCount = sizeof(pressedKeys) / sizeof(word);
				bool result = false;

				for (i = 0; i < pressedKeysCount; i++) {
					if (pressedKeys[i] == keyCode) {
						if (keyRelease) {
							pressedKeys[i] = KEY_NONE;
							result = true;
						}

						break;
					}
				}

				if (i >= pressedKeysCount && !keyRelease) {
					for (i = 0; i < pressedKeysCount; i++) {
						if (pressedKeys[i] == KEY_NONE) {
							pressedKeys[i] = keyCode;
							result = true;
							break;
						}
					}
				}

				if (!result) {
					keyCode = KEY_NONE;
					keyRelease = false;
					continue;
				}

				word tempFlag = 0;
				if (keyCode == KEY_LSHIFT)
					tempFlag = fKeyShiftLeft;
				else if (keyCode == KEY_RSHIFT)
					tempFlag = fKeyShiftRight;
				else if (keyCode == KEY_LCTRL)
					tempFlag = fKeyCtrlLeft;
				else if (keyCode == KEY_RCTRL)
					tempFlag = fKeyCtrlRight;
				else if (keyCode == KEY_LALT)
					tempFlag = fKeyAltLeft;
				else if (keyCode == KEY_RALT)
					tempFlag = fKeyAltRight;
				else if (keyCode == KEY_LWIN)
					tempFlag = fKeyAltLeft;
				else if (keyCode == KEY_RWIN)
					tempFlag = fKeyAltRight;
				else if (keyCode == KEY_MENU)
					tempFlag = fKeyAltLeft;

				if (tempFlag != 0) {
					if (!keyRelease)
						keyFlags |= tempFlag;
					else
						keyFlags &= ~tempFlag;
				}

				if (!keyRelease) {
					if (keyCode == KEY_SCROLLOCK)
						keyFlags = keyFlags ^ fKeyPCEmu;

					if (!(keyFlags & fKeyPCEmu)) {
						if (keyCode == KEY_HOME)
							keyFlags = keyFlags ^ fKeyCaps;
					}
					else if (keyCode == KEY_BACKQUOTE)
						keyFlags = keyFlags ^ fKeyCaps;
				}

				_keyCode = keyCode;
				_keyFlags = keyFlags;

				if (keyRelease)
					_keyFlags |= fKeyRelease;
				keyRelease = false;
			}
		}
	}

	// joystick emulation
	while (_keyCode == KEY_NONE && joystick.GetCntr() > 0) {
		byte joyCode = joystick.ReadByte();

		//__TRACE("joyCode - 0x%.2x\n", joyCode);

		word keyCode = KEY_NONE;
		word tempFlag = 0;

		if ((joyCode & 0x80))
			tempFlag = fKeyRelease;
		joyCode &= 0x7f;

		if (joyCode < 16)
			tempFlag |= fKeyJoy1;
		else
			tempFlag |= fKeyJoy2;
		joyCode &= 0x0f;

		switch (joyCode) {
			case 0:
				keyCode = KEY_UP;
				break;
			case 1:
				keyCode = KEY_DOWN;
				break;
			case 2:
				keyCode = KEY_LEFT;
				break;
			case 3:
				keyCode = KEY_RIGHT;
				break;
			case 4:
				keyCode = KEY_RCTRL;
				break;
			case 5:
				keyCode = KEY_RCTRL;
				break;
		}

		if (keyCode != KEY_NONE) {
			_keyCode = keyCode;
			_keyFlags |= tempFlag;
		}
	}

	if (_keyCode != KEY_NONE) {
		//__TRACE("# keyCode - 0x%.4x, 0x%.4x\n", _keyCode, _keyFlags);

		DecodeKeySpec(_keyCode, _keyFlags);

		if ((_keyFlags & fKeyRelease) == 0) {
			prevKey = _keyCode;
			keyRepeatTime = Timer_GetTickCounter() + 50;
		}
		else {
			prevKey = KEY_NONE;
		}
	}

	return _keyCode != KEY_NONE;
}

word ReadKeySimple(bool norepeat)
{
	Timer_Routine();

	word keyCode;
	word keyFlags;

	while (ReadKey(keyCode, keyFlags)) {
		if ((keyFlags & fKeyRelease) == 0)
			return keyCode;
	}

	if (norepeat)
		return KEY_NONE;

	if (prevKey != KEY_NONE && (int)(Timer_GetTickCounter() - keyRepeatTime) >= 0) {
		keyRepeatTime = Timer_GetTickCounter() + 5;
		return prevKey;
	}

	return KEY_NONE;
}
//---------------------------------------------------------------------------------------
void Beep()
{
	byte specPortFe = SystemBus_Read(0xc00016);

	for (int i = 0; i < 20; i++) {
		//for (int j = 0; j < 2; j++);

		specPortFe ^= 0x10;
		SystemBus_Write(0xc00016, specPortFe);
	}
}

char GetKey(bool wait)
{
	do {
		word keyCode = ReadKeySimple();

		if (keyCode != KEY_NONE) {
			char c = CodeToChar(keyCode);
			if (c != 0)
				Beep();
			return c;
		}
	} while (wait);

	return 0;
}

void WaitKey()
{
	CPU_Stop();
	GetKey();
	CPU_Start();
}
//---------------------------------------------------------------------------------------
void Keyboard_Routine()
{
	word keyCode, keyFlags;

	while (ReadKey(keyCode, keyFlags)) {
		DecodeKey(keyCode, keyFlags);
		//__TRACE("% keyCode - 0x%.4x, 0x%.4x\n", keyCode, keyFlags);
	}
}
//---------------------------------------------------------------------------------------
