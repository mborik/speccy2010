#include "commander.h"
#include "screen.h"
#include "dialog.h"
#include "viewer.h"
#include "../system.h"
#include "../system/sdram.h"
#include "../utils/cstring.h"
#include "../utils/dirent.h"
#include "../betadisk/fdc.h"
#include "../betadisk/floppy.h"
#include "../specMB02.h"
#include "../specConfig.h"
#include "../specKeyboard.h"
#include "../specSnapshot.h"
#include "../specTape.h"


const char *fatErrorMsg[] = {
	"FR_OK",
	"FR_DISK_ERR",
	"FR_INT_ERR",
	"FR_NOT_READY",
	"FR_NO_FILE",
	"FR_NO_PATH",
	"FR_INVALID_NAME",
	"FR_DENIED",
	"FR_EXIST",
	"FR_INVALID_OBJECT",
	"FR_WRITE_PROTECTED",
	"FR_INVALID_DRIVE",
	"FR_NOT_ENABLED",
	"FR_NO_FILESYSTEM",
	"FR_MKFS_ABORTED",
	"FR_TIMEOUT",
	"FR_LOCKED",
	"FR_NOT_ENOUGH_CORE",
};
//---------------------------------------------------------------------------------------
char destinationPath[PATH_SIZE] = "/";
char srcName[PATH_SIZE], dstName[PATH_SIZE], shortName[45];
//---------------------------------------------------------------------------------------
void dynamic_bytes_text(dword size, char *buffer)
{
	if (size < 9999)
		sprintf(buffer, "%10u B", size);
	else if (size < 0x100000)
		sprintf(buffer, "%6u.%.2u kB", size >> 10, ((size & 0x3ff) * 100) >> 10);
	else
		sprintf(buffer, "%6u.%.2u MB", size >> 20, ((size & 0xfffff) * 100) >> 20);
}
//---------------------------------------------------------------------------------------
byte get_sel_attr(FRECORD &fr)
{
	byte result = 0117;

	if ((fr.attr & AM_DIR) == 0) {
		strlwr(fr.name);

		char *ext = fr.name + strlen(fr.name);
		while (ext > fr.name && *ext != '.')
			ext--;

		if (strstr(".trd.fdi.scl.mbd.mb2", ext))
			result = 0115;
		else if (strstr(".tap.tzx", ext))
			result = 0114;
		else if (strstr(".sna.scr.rom", ext))
			result = 0113;
		else if (strstr(".txt.hlp.ini.cfg.lst.nfo.asm.a80.inc", ext))
			result = 0112;
		else
			result = 0110;
	}

	if (fr.sel)
		result = 0116;

	return result;
}
//---------------------------------------------------------------------------------------
void show_table()
{
	display_path(get_current_dir(), 0, 22, 42);

	FRECORD fr;
	char sname[20];

	word i, j, x, col, row, pos;
	for (i = 0; i < FILES_PER_ROW; i++) {
		for (j = 0; j < 2; j++) {
			x = (j * 128) + 2;
			col = (x + 6) >> 3;
			row = i + 2;
			pos = i + j * FILES_PER_ROW + files_table_start;

			Read(&fr, table_buffer, pos);
			make_short_name(sname, 19, fr.name);

			if (pos < files_size) {
				if (fr.sel) {
					DrawChar(x, row, 0xDD);
					DrawAttr(x, row, 0116);
				}
				else {
					DrawChar(x, row, 0xB3);
					DrawAttr(x, row, 0117);
				}

				DrawStr(x + 6, row, sname, 19);
				DrawAttr8(col, row, get_sel_attr(fr), 14);
			}
			else {
				DrawStr(x, row, "\xB3", 19);
				DrawAttr8(col, row, 0117, 14);
			}
		}
	}

	if (too_many_files)
		DrawStrAttr(144, 18, " TOO MANY FILES ", 0126);
	else if (files_size == 0)
		DrawStrAttr(8, 2, "\x1C no files in dir \x1C", 0112, 19);
}
//---------------------------------------------------------------------------------------
void hide_sel()
{
	if (!too_many_files && files_size != 0) {
		FRECORD fr;
		Read(&fr, table_buffer, files_sel);

		word x = (files_sel_pos_x * 128) + 2;
		word col = (x + 6) >> 3;
		word row = files_sel_pos_y + 2;

		if (fr.sel) {
			DrawChar(x, row, 0xDD);
			DrawAttr(x, row, 0116);
		}
		else {
			DrawChar(x, row, 0xB3);
			DrawAttr(x, row, 0117);
		}

		DrawAttr8(col, row, get_sel_attr(fr), 14);
		DrawStr(x + 6, row, "", 20, true, true);
		DrawChar(x + 120, row, 0xB3);
		DrawAttr(x + 120, row, 0117);
	}

	DrawStr(8, 19, "", 40);
}
//---------------------------------------------------------------------------------------
void show_sel(bool redraw = false)
{
	if (calc_sel() || redraw)
		show_table();

	if (!too_many_files && files_size != 0) {
		FRECORD fr;
		Read(&fr, table_buffer, files_sel);

		word x = (files_sel_pos_x * 128) + 2;
		word col = files_sel_pos_x * 16;
		word row = files_sel_pos_y + 2;
		byte attr = 0117;

		if (fr.sel) {
			DrawChar(x, row, 0xDB);
			attr = 0116;
		}
		else
			DrawChar(x, row, 0xDE);

		DrawAttr8(col, row, attr, 16);
		DrawStr(x + 6, row, "", 20, true, true);
		DrawChar(x + 120, row, 0xDD);

		char sname[41];
		make_short_name(sname, sizeof(sname), fr.name);
		DrawStrAttr(8, 19, sname, get_sel_attr(fr), 40);
		DrawStrAttr(206, 18, "\xC4\xC4\xC4\xC4\xC4\xC4", 0117, 6);

		if (files_sel_number > 0) {
			sniprintf(sname, sizeof(sname), " (%u)", files_sel_number);
			size_t len = strlen(sname);
			DrawStr(242 - (len * 6), 18, sname, len);
			switch (len) {
				case 4:  len = 2; break;
				case 5:  len = 3; break;
				case 6:
				case 7:  len = 4; break;
				default: len = 0; break;
			}
			DrawAttr8(30 - len, 18, 0116, len);
		}

		if ((fr.date & 0x1ff) == 0 || strcmp(sname, "..") == 0) {
			DrawStr(8, 20, "---------- -----", 16);
		}
		else {
			sniprintf(sname, sizeof(sname), "%04u-%02u-%02u %02u:%02u",
				(1980 + (fr.date >> 9)),
				(fr.date >> 5) & 0x0f,
				(fr.date & 0x1f),
				(fr.time >> 11) & 0x1f,
				(fr.time >> 5) & 0x3f);
			DrawStr(8, 20, sname, 16);
		}

		if ((fr.attr & AM_DIR) != 0)
			sprintf(sname, "       <DIR>");
		else
			dynamic_bytes_text(fr.size, sname);

		DrawStr(176, 20, sname, 12);
	}
}
//---------------------------------------------------------------------------------------
void init_screen()
{
	SystemBus_Write(0xc00022, 0x8000); // Enable shell border

	ClrScr(0006);
	DrawFrame(  2, 18, 254,  4, 0117, "\xC3\xC4\xB4\xB3\xC0\xC4\xD9");
	DrawFrame(  2,  1, 126, 18, 0117, "\xD1\xCD\xD1\xB3\xC3\xC4\xC1");
	DrawFrame(130,  1, 126, 18, 0117, "\xD1\xCD\xD1\xB3\xC1\xC4\xB4");

	DrawLine(1, 1);
	DrawLine(1, 3);

	DrawStr(2, 0, "Speccy2010 Commander v" VERSION " \7 File Manager");
	DrawFnKeys(1, 23, "1help2cwd3view4hex5copy6mov7mkdir8del9img", 41);

	DrawAttr8(0,  0, 0114, 32);
	DrawAttr8(0, 23, 0005, 32);

	SystemBus_Write(0xc00021, 0x8000 | VIDEO_PAGE); // Enable shell videopage
}
//---------------------------------------------------------------------------------------
//- COMMANDER ACTION HANDLERS -----------------------------------------------------------
//---------------------------------------------------------------------------------------
bool Shell_CopyItem(const char *srcName, const char *dstName, bool move, bool *askForOverwrite = NULL, bool *overwrite = NULL)
{
	const char *sname = &dstName[strlen(dstName)];
	while (sname != dstName && sname[-1] != '/')
		sname--;

	int res;
	make_short_name(shortName, 32, sname);

	show_table();

	do {
		if (FileExists(dstName)) {
			if (askForOverwrite != NULL && overwrite != NULL) {
				if (*askForOverwrite == true) {
					*askForOverwrite = false;
					*overwrite = Shell_MessageBox("Overwrite", "Overwrite all exist files?", "", "", MB_YESNO, 0050, 0115);
				}
				if (*overwrite == false)
					return false;
			}
			else if (!Shell_MessageBox("Overwrite", "Do you want to overwrite", shortName, "", MB_YESNO, 0050, 0115))
				return false;

			res = f_unlink(dstName);
			if (res != FR_OK) {
				res = FR_EXIST;
				break;
			}
		}

		if (move) {
			res = f_rename(srcName, dstName);
		}
		else {
			FIL src, dst;

			res = f_open(&src, srcName, FA_READ);
			if (res != FR_OK) {
				break;
			}

			res = f_open(&dst, dstName, FA_WRITE | FA_CREATE_ALWAYS);
			if (res != FR_OK) {
				f_close(&src);
				break;
			}

			Shell_ProgressBar("Processing", shortName);

			byte buff[0x100];
			UINT size = src.fsize;
			while (size > 0) {
				UINT currentSize = min(size, sizeof(buff));
				UINT r;

				res = f_read(&src, buff, currentSize, &r);
				if (res != FR_OK)
					break;

				res = f_write(&dst, buff, currentSize, &r);
				if (res != FR_OK)
					break;

				size -= currentSize;
				if ((src.fptr & 0x300) == 0) {
					float progress = src.fsize - size;
					progress /= src.fsize;
					Shell_UpdateProgress(progress);
				}

				if (GetKey(false) == K_ESC) {
					res = FR_DISK_ERR;
					break;
				}
			}

			ScreenPop();

			f_close(&dst);
			if (res != FR_OK)
				f_unlink(dstName);

			f_close(&src);
		}
	} while (false);

	if (res == FR_OK)
		return true;
	else {
		Shell_MessageBox("Error",
			(move ? "Cannot move/rename to" : "Cannot copy to"),
				shortName, fatErrorMsg[res]);
	}

	show_table();

	return false;
}
//---------------------------------------------------------------------------------------
bool Shell_Copy(const char *_name, bool move)
{
	CString name;

	if ((ReadKeyFlags() & fKeyShift) != 0)
		name = _name;
	else
		name = destinationPath;

	const char *title = move ? "Move/Rename" : "Copy";
	if (!Shell_InputBox(title, "Enter new name/path:", name))
		return false;

	bool newPath = false;

	if (name.GetSymbol(0) == '/') {
		if (name.GetSymbol(name.Length() - 1) != '/')
			name += '/';
		newPath = true;
	}

	if (files_sel_number == 0 || !newPath) {
		sniprintf(srcName, PATH_SIZE, "%s%s", get_current_dir(), _name);

		if (newPath)
			sniprintf(dstName, PATH_SIZE, "%s%s", name.String() + 1, _name);
		else
			sniprintf(dstName, PATH_SIZE, "%s%s", get_current_dir(), name.String());

		if (Shell_CopyItem(srcName, dstName, move) && !newPath && move) {
			sniprintf(files_lastName, sizeof(files_lastName), "%s", name.String());
		}
	}
	else {
		FRECORD fr;
		bool overwrite = true;
		bool askForOverwrite = true;

		for (int i = 0; i < files_size; i++) {
			Read(&fr, table_buffer, i);

			if (fr.sel) {
				sniprintf(srcName, PATH_SIZE, "%s%s", get_current_dir(), fr.name);
				sniprintf(dstName, PATH_SIZE, "%s%s", name.String() + 1, fr.name);

				if (!Shell_CopyItem(srcName, dstName, move, &askForOverwrite, &overwrite))
					break;

				fr.sel = false;
				files_sel_number--;

				Write(&fr, table_buffer, i);
			}

			if (GetKey(false) == K_ESC)
				break;
		}
	}

	return !newPath || move;
}
//---------------------------------------------------------------------------------------
bool Shell_CreateFolder()
{
	CString name = "";
	if (!Shell_InputBox("Create folder", "Enter name:", name))
		return false;

	sniprintf(dstName, PATH_SIZE, "%s%s", get_current_dir(), name.String());

	int res = f_mkdir(dstName);
	if (res == FR_OK) {
		sniprintf(files_lastName, sizeof(files_lastName), "%s", name.String());
		return true;
	}

	make_short_name(shortName, 32, name);

	Shell_MessageBox("Error", "Cannot create the folder", shortName, fatErrorMsg[res]);
	show_table();

	return false;
}
//---------------------------------------------------------------------------------------
bool Shell_DeleteItem(const char *name)
{
	int res = f_unlink(name);
	if (res == FR_OK)
		return true;

	const char *sname = &name[strlen(name)];
	while (sname != name && sname[-1] != '/')
		sname--;

	make_short_name(shortName, 32, sname);

	Shell_MessageBox("Error", "Cannot delete the item", shortName, fatErrorMsg[res]);
	show_table();

	return false;
}
//---------------------------------------------------------------------------------------
bool Shell_Delete(const char *name)
{
	if (files_sel_number == 0 && strcmp(name, "..") == 0)
		return false;

	if (files_sel_number > 0)
		sniprintf(shortName, 24, "selected %u item%s", files_sel_number, files_sel_number > 1 ? "s" : "");
	else
		make_short_name(shortName, 32, name);
	strcat(shortName, "?");

	if (!Shell_MessageBox("Delete", "Do you wish to delete", shortName, "", MB_YESNO))
		return false;

	if (files_sel_number == 0) {
		sniprintf(dstName, PATH_SIZE, "%s%s", get_current_dir(), name);
		Shell_DeleteItem(dstName);
	}
	else {
		FRECORD fr;

		for (int i = 0; i < files_size; i++) {
			Read(&fr, table_buffer, i);

			if (fr.sel) {
				sniprintf(dstName, PATH_SIZE, "%s%s", get_current_dir(), fr.name);
				if (!Shell_DeleteItem(dstName))
					break;
			}
		}
	}

	return true;
}
//---------------------------------------------------------------------------------------
bool Shell_EmptyTrd(const char *_name, bool format = true)
{
	bool result = false;
	make_short_name(shortName, 32, _name);

	if (format) {
		char *ext = (char *) (_name + strlen(_name));
		while (ext > _name && *ext != '.')
			ext--;

		if (strcasecmp(ext, ".trd") != 0)
			return false;

		strcat(shortName, "?");
		if (!Shell_MessageBox("Format", "Do you wish to format", shortName, "", MB_YESNO, 0050, 0115))
			return false;
	}

	CString label = _name;
	label.TrimRight(4);
	if (label.Length() > 8)
		label.TrimRight(label.Length() - 8);

	if (!Shell_InputBox("Format", "Enter disk label:", label))
		return false;

	sniprintf(dstName, PATH_SIZE, "%s%s", get_current_dir(), _name);

	const byte zero[0x10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	const byte sysArea[] = { 0x00, 0x00, 0x01, 0x16, 0x00, 0xf0, 0x09, 0x10 };
	char sysLabel[8];
	strncpy(sysLabel, label.String(), sizeof(sysLabel));

	int res;
	UINT r;
	FIL dst;
	BYTE flags = FA_WRITE | (format ? FA_OPEN_EXISTING : FA_CREATE_ALWAYS);

	{
		res = f_open(&dst, dstName, flags);
		if (res != FR_OK)
			goto formatExit1;

		for (int i = 0; i < 256 * 16; i += sizeof(zero)) {
			res = f_write(&dst, zero, sizeof(zero), &r);
			if (res != FR_OK)
				break;
		}
		if (res != FR_OK)
			goto formatExit2;

		res = f_lseek(&dst, 0x8e0);
		if (res != FR_OK)
			goto formatExit2;

		res = f_write(&dst, sysArea, sizeof(sysArea), &r);
		if (res != FR_OK)
			goto formatExit2;

		res = f_lseek(&dst, 0x8f5);
		if (res != FR_OK)
			goto formatExit2;

		res = f_write(&dst, sysLabel, sizeof(sysLabel), &r);
		if (res != FR_OK)
			goto formatExit2;

		res = f_lseek(&dst, 256 * 16 * 2 * 80 - sizeof(zero));
		if (res != FR_OK)
			goto formatExit2;

		res = f_write(&dst, zero, sizeof(zero), &r);
		if (res != FR_OK)
			goto formatExit2;

	formatExit2:
		f_close(&dst);

	formatExit1:;
	}

	if (res != FR_OK) {
		make_short_name(shortName, 32, _name);
		Shell_MessageBox("Error", "Cannot format", shortName, fatErrorMsg[res]);
	}

	return result;
}
//---------------------------------------------------------------------------------------
bool Shell_EmptyMbd(const char *_name, bool format = true)
{
	const char *title = "MBD Disk Image";

	int disk_size;
	int numtrk = 0;
	int numsec = 0;
	bool askForGeometry = true;

	sniprintf(dstName, PATH_SIZE, "%s%s", get_current_dir(), _name);

	if (format) {
		make_short_name(shortName, 32, _name);

		if (mb02_checkfmt(dstName, &numtrk, &numsec)) {
			strcat(shortName, "?");
			if (!Shell_MessageBox(title, "Do you wish to format", shortName, "", MB_YESNO, 0050, 0115))
				return false;
			askForGeometry = false;
		}
		else {
			strcat(shortName, ".");
			if (!Shell_MessageBox(title, "Invalid image format of", shortName, "Recreate image anyway?", MB_YESNO))
				return false;
		}
	}

	if (askForGeometry) {
		numtrk = 82; numsec = 11;
		CString value = "82/11";

		while (true) {
			if (!Shell_InputBox(title, "Tracks/Sectors:", value))
				return false;

			sscanf(value.String(), "%d/%d", &numtrk, &numsec);

			if (numtrk < 1 || numtrk > 255) {
				Shell_MessageBox(title, "Invalid number of tracks", "(must be 1..255)");
				continue;
			}

			if (numsec < 1 || numsec > 127) {
				Shell_MessageBox(title, "Invalid number of sectors", "(must be 1..127)");
				continue;
			}

			disk_size = (numtrk * numsec) << 1;
			sniprintf(shortName, 30, "Invalid disk size %u kB", disk_size);

			if (disk_size < 5 || disk_size >= 2047) {
				Shell_MessageBox(title, shortName, "(must be 6..2046 kB)");
				continue;
			}

			break;
		}
	}

	CString label = _name;
	label.TrimRight(4);
	if (label.Length() > 8)
		label.TrimRight(label.Length() - 8);
	if (!Shell_InputBox(title, "Enter disk label:", label))
		label = _name;
	if (label.Length() > 26)
		label.TrimRight(label.Length() - 26);

	Shell_ProgressBar(title, "Formatting...");
	bool result = mb02_formatdisk(dstName, numtrk, numsec, label.String());
	ScreenPop();

	return result;
}
//---------------------------------------------------------------------------------------
bool Shell_CreateDiskImage()
{
	const char *title = "Create disk image";

	bool mbd = Shell_MessageBox(title,
		"Which type of virtual disk",
		"image you wish to create?", "", MB_DISK, 0050, 0115);

	const char *baseExt = &".mbd\0.trd"[mbd ? 0 : 5];

	CString newName = "empty";
	newName += baseExt;

	if (!Shell_InputBox(title, "Enter image name:", newName))
		return false;

	char *ext = ((char *) newName.String()) + newName.Length();
	while (ext > newName.String() && *ext != '.')
		ext--;

	if (strcasecmp(ext, baseExt) != 0)
		newName += baseExt;

	return mbd ?
		Shell_EmptyMbd(newName.String(), false) :
		Shell_EmptyTrd(newName.String(), false);
}
//---------------------------------------------------------------------------------------
void Shell_ScreenBrowser(char *fullName)
{
	while (true) {
		dword pos = 0x1800, addr = VIDEO_PAGE_PTR;
		for (; pos < 0x1b00; pos++)
			SystemBus_Write(addr + pos, 0);

		FIL image;
		if (f_open(&image, fullName, FA_READ) == FR_OK) {
			byte data;
			UINT res;

			for (pos = 0; pos < 0x1b00; pos++) {
				if (f_read(&image, &data, 1, &res) != FR_OK)
					break;
				if (res == 0)
					break;

				SystemBus_Write(addr++, data);
			}

			f_close(&image);

			// missing attributes filled with base color...
			for (; pos < 0x1b00; pos++)
				SystemBus_Write(addr++, 0070);
		}
		else
			break;

		char key = GetKey();
		if (key != ' ' && key != K_RETURN)
			break;

		int i = files_size;
		while (true) {
			files_sel++;
			if (files_sel >= files_size)
				files_sel = 0;

			FRECORD fr;
			Read(&fr, table_buffer, files_sel);
			strlwr(fr.name);

			char *ext = fr.name + strlen(fr.name);
			while (ext > fr.name && *ext != '.')
				ext--;

			if (strcmp(ext, ".scr") == 0 || fr.size == 6912 || fr.size == 6144) {
				sniprintf(fullName, PATH_SIZE, "%s%s", get_current_dir(), fr.name);
				break;
			}

			if (--i == 0)
				return;
		}
	}
}
//---------------------------------------------------------------------------------------
bool Shell_Viewer(char *fullName)
{
	FIL file;
	if (f_open(&file, fullName, FA_READ) == FR_OK) {
		byte data;
		UINT res, total, ascii;

		for (total = 0, ascii = 0; total < 1024; total++) {
			if (f_read(&file, &data, 1, &res) != FR_OK)
				break;
			if (res == 0)
				break;

			if ((data > 7 && data <= 13) || (data >= 32 && data < 127))
				ascii++;
		}

		f_close(&file);

		if ((100.0f / ((float) total / ascii)) > 80.0f) {
			Shell_TextViewer(fullName);
			return true;
		}
		else {
			Shell_MessageBox("Binary file",
				"Hex view of binary file", "not yet implemented", "", MB_OK, 0137);
		}
	}

	return false;
}
//---------------------------------------------------------------------------------------
bool Shell_Receiver(const char *inputName)
{
	const char *title = "XMODEM File Transfer";
	CString name = inputName;
	if (!Shell_InputBox(title, "Enter output name:", name) || name.Length() == 0)
		return false;

	make_short_name(shortName, 32, name.String());
	sniprintf(dstName, PATH_SIZE, "%s%s", get_current_dir(), name.String());

	if (FileExists(dstName) && !Shell_MessageBox("Overwrite", "Do you want to overwrite", shortName, "", MB_YESNO, 0050, 0115))
		return false;

	sniprintf(srcName, PATH_SIZE, "%s%lo.tmp", get_current_dir(), get_fattime());

	FIL file;
	if (f_open(&file, srcName, FA_READ | FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
		Shell_MessageBox(title, "Receiving transmission...", "", "", MB_PROGRESS, 0070);

		char status = 0;
		XModem modem(&file);

		modem.init();

		while (true) {
			status = modem.receive();
			if (status < 0) {
				if (GetKey(false) == K_ESC)
					break;
			}
			else break;
		}

		ScreenPop();

		if (status > 0) {
			DWORD dc = file.fsize;
			byte c;
			UINT r;

			// try to find proper ending
			for (int i = 0; dc > 1 && i < 127; i++) {
				f_lseek(&file, --dc);
				f_read(&file, &c, 1, &r);

				if (c != 0x1A)
					break;
			}

			CString value;
			value.Format("%lu", ++dc);
			if (Shell_InputBox(title, "Confirm file size:", value))
				sscanf(value.String(), "%lu", &dc);

			if (dc > 0 && dc < file.fsize) {
				f_lseek(&file, dc);
				f_truncate(&file);
			}

			f_close(&file);

			if (FileExists(dstName))
				f_unlink(dstName);
			f_rename(srcName, dstName);
		}
		else {
			f_close(&file);
			f_unlink(srcName);

			Shell_MessageBox("Error", "Transmission failed!", shortName);
		}
	}

	show_table();
	return true;
}
//---------------------------------------------------------------------------------------
// COMMANDER CORE -----------------------------------------------------------------------
//---------------------------------------------------------------------------------------
void Shell_Commander()
{
	CPU_Stop();

	SystemBus_Write(0xc00020, 0); // use bank 0

	init_screen();

	read_dir();
	show_sel(true);

	while (true) {
		byte key = GetKey();
		char fullName[PATH_SIZE];

		FRECORD fr;
		Read(&fr, table_buffer, files_sel);

		sniprintf(files_lastName, sizeof(files_lastName), "%s", fr.name);

		if ((key == K_UP || key == K_DOWN || key == K_LEFT || key == K_RIGHT) && files_size > 0) {
			hide_sel();

			if (key == K_LEFT)
				files_sel -= FILES_PER_ROW;
			else if (key == K_RIGHT)
				files_sel += FILES_PER_ROW;
			else if (key == K_UP)
				files_sel--;
			else if (key == K_DOWN)
				files_sel++;

			show_sel();
		}
		else if (key == ' ' && files_size > 0) {
			if (strcmp(fr.name, "..") != 0) {
				if (fr.sel)
					files_sel_number--;
				else
					files_sel_number++;

				fr.sel = (fr.sel + 1) % 2;
				Write(&fr, table_buffer, files_sel);
			}

			hide_sel();
			files_sel++;
			show_sel();
		}
		else if ((key == '+' || key == '=' || key == '-' || key == '/' || key == '\\') && files_size > 0) {
			hide_sel();
			files_sel_number = 0;

			for (int i = 0; i < files_size; i++) {
				Read(&fr, table_buffer, i);

				if (strcmp(fr.name, "..") != 0) {
					if (key == '+' || key == '=')
						fr.sel = 1;
					else if (key == '-')
						fr.sel = 0;
					else if (key == '/' || key == '\\')
						fr.sel = (fr.sel + 1) % 2;

					if (fr.sel)
						files_sel_number++;
					Write(&fr, table_buffer, i);
				}

				if ((i & 0x3f) == 0)
					CycleMark();
			}

			show_sel(true);
		}
		else if (key >= '1' && key <= '4') {
			if (specConfig.specDiskIf != SpecDiskIf_DivMMC && (fr.attr & AM_DIR) == 0) {
				sniprintf(fullName, PATH_SIZE, "%s%s", get_current_dir(), fr.name);

				char *ext = fr.name + strlen(fr.name);
				while (ext > fr.name && *ext != '.')
					ext--;

				int i = key - '1';
				const char *mountPoint;
				bool extMatch = false, difValid = false, imgValid = false;

				if (strcasestr(".trd.fdi.scl", ext)) {
					extMatch = true;

					if (specConfig.specDiskIf == SpecDiskIf_Betadisk) {
						difValid = true;

						if (fdc_open_image(i, fullName)) {
							floppy_disk_wp(i, &specConfig.specBdiImages[i].writeProtect);

							strcpy(specConfig.specBdiImages[i].name, fullName);
							SaveConfig();

							mountPoint = &"A:\0B:\0C:\0D:"[i * 3]; // TR-DOS drive letter
							imgValid = true;
						}
					}
				}
				else if (strcasestr(".mbd.mb2", ext)) {
					extMatch = true;

					if (specConfig.specDiskIf == SpecDiskIf_MB02) {
						difValid = true;

						if (mb02_open_image(i, fullName) == 0) {
							specConfig.specMB2Images[i].writeProtect |= mb02_is_disk_wp(i);
							mb02_set_disk_wp(i, (specConfig.specMB2Images[i].writeProtect != 0));

							strcpy(specConfig.specMB2Images[i].name, fullName);
							SaveConfig();

							mountPoint = &"@1\0@2\0@3\0@4"[i * 3]; // BS-DOS drive number
							imgValid = true;
						}
					}
				}

				if (extMatch) {
					if (imgValid) {
						char successMsg[2][36];

						make_short_name(successMsg[0], 36, fr.name);
						sniprintf(successMsg[1], 36, "mounted into %s drive...", mountPoint);

						Shell_Toast(successMsg[0], successMsg[1]);
					}
					else if (difValid)
						Shell_MessageBox("Mount disk error", "Invalid disk image");
					else
						Shell_MessageBox("Mount disk error", "Cannot mount disk image", "to the current Disk IF");
				}
			}
		}
		else if (key == K_BACKSPACE) {
			hide_sel();
			leave_dir();
			show_table();
			show_sel();
		}
		else if (key == K_F1) {
			Shell_TextViewer("speccy2010.hlp", true);

			init_screen();
			show_table();
			show_sel(true);
		}
		else if (key == K_F2) {
			sniprintf(destinationPath, sizeof(destinationPath), "/%s", get_current_dir());
			make_short_name(shortName, 36, destinationPath);
			Shell_Toast("Current working directory:", shortName, 0070);
		}
		else if (key == K_F3) {
			if ((fr.attr & AM_DIR) == 0) {
				sniprintf(fullName, PATH_SIZE, "%s%s", get_current_dir(), fr.name);

				char *ext = fr.name + strlen(fr.name);
				while (ext > fr.name && *ext != '.')
					ext--;

				strlwr(ext);
				if (strcmp(ext, ".scr") == 0 || fr.size == 6912 || fr.size == 6144) {
					Shell_ScreenBrowser(fullName);
					init_screen();
					show_table();
				}
				else if (Shell_Viewer(fullName)) {
					init_screen();
					show_table();
				}

				show_sel(true);
			}
		}
		else if (key == K_F4) {
			if ((fr.attr & AM_DIR) == 0)
				Shell_Toast("Hex editor", "not yet implemented");
		}
		else if (key == K_F5) {
			hide_sel();
			if (Shell_Copy(fr.name, false))
				read_dir();
			show_sel(true);
		}
		else if (key == K_F6) {
			hide_sel();
			if (Shell_Copy(fr.name, true))
				read_dir();
			show_sel(true);
		}
		else if (key == K_F7) {
			hide_sel();
			if (Shell_CreateFolder())
				read_dir();
			show_sel(true);
		}
		else if (key == K_F8) {
			hide_sel();

			if (Shell_Delete(fr.name)) {
				int last_files_sel = files_sel;
				read_dir();
				files_sel = last_files_sel;
			}

			show_sel(true);
		}
		else if (key == K_F9) {
			hide_sel();
			bool createNew = ((fr.attr & AM_DIR) != 0), success = false;

			if (!createNew) {
				char *ext = fr.name + strlen(fr.name);
				while (ext > fr.name && *ext != '.')
					ext--;
				strlwr(ext);

				if (strcmp(ext, ".trd") == 0) {
					if (Shell_EmptyTrd(fr.name))
						success = true;
				}
				else if (strstr(".mbd.mb2", ext)) {
					if (Shell_EmptyMbd(fr.name))
						success = true;
				}
				else
					createNew = true;
			}

			if (createNew)
				success = Shell_CreateDiskImage();
			if (success)
				read_dir();

			show_table();
			show_sel(true);
		}
		else if (key == K_F11) {
			hide_sel();

			if (Shell_Receiver((fr.attr & AM_DIR) ? "" : fr.name)) {
				int last_files_sel = files_sel;
				read_dir();
				files_sel = last_files_sel;
			}

			show_sel(true);
		}
		else if (key == K_ESC || key == K_F10 || key == K_F12) {
			break;
		}
		else if (key == K_RETURN) {
			if ((fr.attr & AM_DIR) != 0) {
				hide_sel();
				enter_dir(fr.name);
				show_table();
				show_sel();
			}
			else {
				sniprintf(fullName, PATH_SIZE, "%s%s", get_current_dir(), fr.name);
				strlwr(fr.name);

				char *ext = fr.name + strlen(fr.name);
				while (ext > fr.name && *ext != '.')
					ext--;

				if (strstr(".tap.tzx", ext)) {
					Tape_SelectFile(fullName);
					break;
				}
				else if (strcmp(ext, ".sna") == 0) {
					sniprintf(specConfig.snaName, sizeof(specConfig.snaName), "/%s", fullName);

					if (LoadSnapshot(fullName))
						break;
				}
				else if (strstr(".trd.fdi.scl", ext)) {
					if (specConfig.specDiskIf != SpecDiskIf_Betadisk)
						Shell_MessageBox("Mount disk error", "Cannot mount disk image", "to the current Disk IF");

					else if (fdc_open_image(0, fullName)) {
						floppy_disk_wp(0, &specConfig.specBdiImages[0].writeProtect);

						strcpy(specConfig.specBdiImages[0].name, fullName);
						SaveConfig();

						CPU_Start();
						CPU_Reset(true);
						CPU_Reset(false);
						CPU_Stop();

						CPU_ModifyPC(0, 0);

						SystemBus_Write(0xc00017, (1 << 4) |
							(specConfig.specMachine == SpecRom_Classic48 ? (1 << 5) : 0));

						SystemBus_Write(0xc00018, 0x0001);
						break;
					}
					else
						Shell_MessageBox("Mount disk error", "Invalid disk image");
				}
				else if (strstr(".mbd.mb2", ext)) {
					if (specConfig.specDiskIf != SpecDiskIf_MB02)
						Shell_MessageBox("Mount disk error", "Cannot mount disk image", "to the current Disk IF");

					else if (mb02_open_image(0, fullName) == 0) {
						specConfig.specMB2Images[0].writeProtect |= mb02_is_disk_wp(0);
						mb02_set_disk_wp(0, (specConfig.specMB2Images[0].writeProtect != 0));

						strcpy(specConfig.specMB2Images[0].name, fullName);
						SaveConfig();
						break;
					}
					else
						Shell_MessageBox("Mount disk error", "Invalid disk image");
				}
				else if (strcmp(ext, ".scr") == 0) {
					Shell_ScreenBrowser(fullName);

					init_screen();
					show_table();
					show_sel();
				}
			}
		}
	}

	SystemBus_Write(0xc00021, 0); // Enable Video
	SystemBus_Write(0xc00022, 0);

	CPU_Start();
}
//---------------------------------------------------------------------------------------
