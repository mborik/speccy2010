#include "commander.h"
#include "screen.h"
#include "snapshot.h"
#include "dialog.h"
#include "viewer.h"
#include "../system.h"
#include "../system/sdram.h"
#include "../utils/cstring.h"
#include "../utils/dirent.h"
#include "../specBetadisk.h"
#include "../specMB02.h"
#include "../specConfig.h"
#include "../specKeyboard.h"
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
CString cstrbuf(PATH_SIZE);
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

	word i, j, x, col, row, pos;
	for (i = 0; i < FILES_PER_ROW; i++) {
		for (j = 0; j < 2; j++) {
			x = (j * 128) + 2;
			col = (x + 6) >> 3;
			row = i + 2;
			pos = i + j * FILES_PER_ROW + files_table_start;

			Read(&mem.ra, table_buffer, pos);
			make_short_name(mem.sname, 19, mem.ra.name);

			if (pos < files_size) {
				if (mem.ra.sel) {
					DrawChar(x, row, 0xDD);
					DrawAttr(x, row, 0116);
				}
				else {
					DrawChar(x, row, 0xB3);
					DrawAttr(x, row, 0117);
				}

				DrawStr(x + 6, row, mem.sname, 19);
				DrawAttr8(col, row, get_sel_attr(mem.ra), 14);
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
		Read(&mem.ra, table_buffer, files_sel);

		word x = (files_sel_pos_x * 128) + 2;
		word col = (x + 6) >> 3;
		word row = files_sel_pos_y + 2;

		if (mem.ra.sel) {
			DrawChar(x, row, 0xDD);
			DrawAttr(x, row, 0116);
		}
		else {
			DrawChar(x, row, 0xB3);
			DrawAttr(x, row, 0117);
		}

		DrawAttr8(col, row, get_sel_attr(mem.ra), 14);
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
		Read(&mem.ra, table_buffer, files_sel);

		word x = (files_sel_pos_x * 128) + 2;
		word col = files_sel_pos_x * 16;
		word row = files_sel_pos_y + 2;
		byte attr = 0117;

		if (mem.ra.sel) {
			DrawChar(x, row, 0xDB);
			attr = 0116;
		}
		else
			DrawChar(x, row, 0xDE);

		DrawAttr8(col, row, attr, 16);
		DrawStr(x + 6, row, "", 20, true, true);
		DrawChar(x + 120, row, 0xDD);

		make_short_name(mem.sname, 41, mem.ra.name);
		DrawStrAttr(8, 19, mem.sname, get_sel_attr(mem.ra), 40);
		DrawStrAttr(206, 18, "\xC4\xC4\xC4\xC4\xC4\xC4", 0117, 6);

		if (files_sel_number > 0) {
			sniprintf(mem.sname, 41, " (%u)", files_sel_number);
			size_t len = strlen(mem.sname);
			DrawStr(242 - (len * 6), 18, mem.sname, len);
			switch (len) {
				case 4:  len = 2; break;
				case 5:  len = 3; break;
				case 6:
				case 7:  len = 4; break;
				default: len = 0; break;
			}
			DrawAttr8(30 - len, 18, 0116, len);
		}

		if ((mem.ra.date & 0x1ff) == 0 || strcmp(mem.sname, "..") == 0) {
			DrawStr(8, 20, "---------- -----", 16);
		}
		else {
			sniprintf(mem.sname, 41, "%04u-%02u-%02u %02u:%02u",
				(1980 + (mem.ra.date >> 9)),
				(mem.ra.date >> 5) & 0x0f,
				(mem.ra.date & 0x1f),
				(mem.ra.time >> 11) & 0x1f,
				(mem.ra.time >> 5) & 0x3f);
			DrawStr(8, 20, mem.sname, 16);
		}

		if ((mem.ra.attr & AM_DIR) != 0)
			sprintf(mem.sname, "       <DIR>");
		else
			dynamic_bytes_text(mem.ra.size, mem.sname);

		DrawStr(176, 20, mem.sname, 12);
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

	DrawLine(1, 2);
	DrawLine(1, 4);

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
	const char *ptr = (const char *) &dstName[strlen(dstName)];
	while (ptr != dstName && ptr[-1] != '/')
		ptr--;

	int res;
	make_short_name(mem.shortName, 32, ptr);

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
			else if (!Shell_MessageBox("Overwrite", "Do you want to overwrite", mem.shortName, "", MB_YESNO, 0050, 0115))
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
			res = f_open(&mem.fsrc, srcName, FA_READ);
			if (res != FR_OK) {
				break;
			}

			res = f_open(&mem.fdst, dstName, FA_WRITE | FA_CREATE_ALWAYS);
			if (res != FR_OK) {
				f_close(&mem.fsrc);
				break;
			}

			Shell_ProgressBar("Processing", mem.shortName);

			byte buff[0x100];
			UINT size = mem.fsrc.fsize;
			while (size > 0) {
				UINT currentSize = min(size, sizeof(buff));
				UINT r;

				res = f_read(&mem.fsrc, buff, currentSize, &r);
				if (res != FR_OK)
					break;

				res = f_write(&mem.fdst, buff, currentSize, &r);
				if (res != FR_OK)
					break;

				size -= currentSize;
				if ((mem.fsrc.fptr & 0x300) == 0) {
					float progress = mem.fsrc.fsize - size;
					progress /= mem.fsrc.fsize;
					Shell_UpdateProgress(progress);
				}

				if (GetKey(false) == K_ESC) {
					res = FR_DISK_ERR;
					break;
				}
			}

			ScreenPop();

			f_close(&mem.fdst);
			if (res != FR_OK)
				f_unlink(dstName);

			f_close(&mem.fsrc);
		}
	} while (false);

	if (res == FR_OK)
		return true;
	else {
		Shell_MessageBox("Error",
			(move ? "Cannot move/rename to" : "Cannot copy to"),
				mem.shortName, fatErrorMsg[res]);
	}

	show_table();

	return false;
}
//---------------------------------------------------------------------------------------
bool Shell_Copy(const char *_name, bool move)
{
	if (ModComb(MOD_SHIFT_1))
		cstrbuf.Set(_name);
	else
		cstrbuf = destinationPath;

	const char *title = move ? "Move/Rename" : "Copy";
	if (!Shell_InputBox(title, "Enter new name/path:", cstrbuf))
		return false;

	bool newPath = false;

	if (cstrbuf.GetSymbol(0) == '/') {
		if (cstrbuf.GetSymbol(cstrbuf.Length() - 1) != '/')
			cstrbuf += '/';
		newPath = true;
	}

	if (files_sel_number == 0 || !newPath) {
		sniprintf(mem.srcName, PATH_SIZE, "%s%s", get_current_dir(), _name);

		if (newPath)
			sniprintf(mem.dstName, PATH_SIZE, "%s%s", cstrbuf.String() + 1, _name);
		else
			sniprintf(mem.dstName, PATH_SIZE, "%s%s", get_current_dir(), cstrbuf.String());

		if (Shell_CopyItem(mem.srcName, mem.dstName, move) && !newPath && move) {
			sniprintf(files_lastName, sizeof(files_lastName), "%s", cstrbuf.String());
		}
	}
	else {
		bool overwrite = true;
		bool askForOverwrite = true;

		for (int i = 0; i < files_size; i++) {
			Read(&mem.ra, table_buffer, i);

			if (mem.ra.sel) {
				sniprintf(mem.srcName, PATH_SIZE, "%s%s", get_current_dir(), mem.ra.name);
				sniprintf(mem.dstName, PATH_SIZE, "%s%s", cstrbuf.String() + 1, mem.ra.name);

				if (!Shell_CopyItem(mem.srcName, mem.dstName, move, &askForOverwrite, &overwrite))
					break;

				mem.ra.sel = false;
				files_sel_number--;

				Write(&mem.ra, table_buffer, i);
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
	cstrbuf = "";
	if (!Shell_InputBox("Create folder", "Enter name:", cstrbuf))
		return false;

	sniprintf(mem.dstName, PATH_SIZE, "%s%s", get_current_dir(), cstrbuf.String());

	int res = f_mkdir(mem.dstName);
	if (res == FR_OK) {
		sniprintf(files_lastName, sizeof(files_lastName), "%s", cstrbuf.String());
		return true;
	}

	make_short_name(mem.shortName, 32, cstrbuf);

	Shell_MessageBox("Error", "Cannot create the folder", mem.shortName, fatErrorMsg[res]);
	show_table();

	return false;
}
//---------------------------------------------------------------------------------------
bool Shell_DeleteItem(const char *name)
{
	int res = f_unlink(name);
	if (res == FR_OK)
		return true;

	const char *ptr = (const char *) &name[strlen(name)];
	while (ptr != name && ptr[-1] != '/')
		ptr--;

	make_short_name(mem.shortName, 32, ptr);

	Shell_MessageBox("Error", "Cannot delete the item", mem.shortName, fatErrorMsg[res]);
	show_table();

	return false;
}
//---------------------------------------------------------------------------------------
bool Shell_Delete(const char *name)
{
	if (files_sel_number == 0 && strcmp(name, "..") == 0)
		return false;

	if (files_sel_number > 0)
		sniprintf(mem.shortName, 24, "selected %u item%s", files_sel_number, files_sel_number > 1 ? "s" : "");
	else
		make_short_name(mem.shortName, 32, name);
	strcat(mem.shortName, "?");

	if (!Shell_MessageBox("Delete", "Do you wish to delete", mem.shortName, "", MB_YESNO))
		return false;

	if (files_sel_number == 0) {
		sniprintf(mem.dstName, PATH_SIZE, "%s%s", get_current_dir(), name);
		Shell_DeleteItem(mem.dstName);
	}
	else {
		for (int i = 0; i < files_size; i++) {
			Read(&mem.ra, table_buffer, i);

			if (mem.ra.sel) {
				sniprintf(mem.dstName, PATH_SIZE, "%s%s", get_current_dir(), mem.ra.name);
				if (!Shell_DeleteItem(mem.dstName))
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
	make_short_name(mem.shortName, 32, _name);

	if (format) {
		char *ext = (char *) (_name + strlen(_name));
		while (ext > _name && *ext != '.')
			ext--;

		if (strcasecmp(ext, ".trd") != 0)
			return false;

		strcat(mem.shortName, "?");
		if (!Shell_MessageBox("Format", "Do you wish to format", mem.shortName, "", MB_YESNO, 0050, 0115))
			return false;
	}

	cstrbuf.Set(_name);
	cstrbuf.TrimRight(4);
	if (cstrbuf.Length() > 8)
		cstrbuf.TrimRight(cstrbuf.Length() - 8);

	if (!Shell_InputBox("Format", "Enter disk label:", cstrbuf))
		return false;

	sniprintf(mem.dstName, PATH_SIZE, "%s%s", get_current_dir(), _name);

	const byte zero[0x10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	const byte sysArea[] = { 0x00, 0x00, 0x01, 0x16, 0x00, 0xf0, 0x09, 0x10 };
	char *sysLabel = mem.sname + 32;
	strncpy(sysLabel, cstrbuf.String(), 8);

	int res;
	UINT r;
	BYTE flags = FA_WRITE | (format ? FA_OPEN_EXISTING : FA_CREATE_ALWAYS);

	{
		res = f_open(&mem.fdst, mem.dstName, flags);
		if (res != FR_OK)
			goto formatExit1;

		for (int i = 0; i < 256 * 16; i += sizeof(zero)) {
			res = f_write(&mem.fdst, zero, sizeof(zero), &r);
			if (res != FR_OK)
				break;
		}
		if (res != FR_OK)
			goto formatExit2;

		res = f_lseek(&mem.fdst, 0x8e0);
		if (res != FR_OK)
			goto formatExit2;

		res = f_write(&mem.fdst, sysArea, sizeof(sysArea), &r);
		if (res != FR_OK)
			goto formatExit2;

		res = f_lseek(&mem.fdst, 0x8f5);
		if (res != FR_OK)
			goto formatExit2;

		res = f_write(&mem.fdst, sysLabel, 8, &r);
		if (res != FR_OK)
			goto formatExit2;

		res = f_lseek(&mem.fdst, 256 * 16 * 2 * 80 - sizeof(zero));
		if (res != FR_OK)
			goto formatExit2;

		res = f_write(&mem.fdst, zero, sizeof(zero), &r);
		if (res != FR_OK)
			goto formatExit2;

	formatExit2:
		f_close(&mem.fdst);

	formatExit1:;
	}

	if (res != FR_OK) {
		make_short_name(mem.shortName, 32, _name);
		Shell_MessageBox("Error", "Cannot format", mem.shortName, fatErrorMsg[res]);
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

	sniprintf(mem.dstName, PATH_SIZE, "%s%s", get_current_dir(), _name);

	if (format) {
		make_short_name(mem.shortName, 32, _name);

		if (mb02_checkfmt(mem.dstName, &numtrk, &numsec)) {
			strcat(mem.shortName, "?");
			if (!Shell_MessageBox(title, "Do you wish to format", mem.shortName, "", MB_YESNO, 0050, 0115))
				return false;
			askForGeometry = false;
		}
		else {
			strcat(mem.shortName, ".");
			if (!Shell_MessageBox(title, "Invalid image format of", mem.shortName, "Recreate image anyway?", MB_YESNO))
				return false;
		}
	}

	if (askForGeometry) {
		numtrk = 82; numsec = 11;
		cstrbuf = "82/11";

		while (true) {
			if (!Shell_InputBox(title, "Tracks/Sectors:", cstrbuf))
				return false;

			sscanf(cstrbuf.String(), "%d/%d", &numtrk, &numsec);

			if (numtrk < 1 || numtrk > 255) {
				Shell_MessageBox(title, "Invalid number of tracks", "(must be 1..255)");
				continue;
			}

			if (numsec < 1 || numsec > 127) {
				Shell_MessageBox(title, "Invalid number of sectors", "(must be 1..127)");
				continue;
			}

			disk_size = (numtrk * numsec) << 1;
			sniprintf(mem.shortName, 30, "Invalid disk size %u kB", disk_size);

			if (disk_size < 5 || disk_size >= 2047) {
				Shell_MessageBox(title, mem.shortName, "(must be 6..2046 kB)");
				continue;
			}

			break;
		}
	}

	cstrbuf.Set(_name);
	cstrbuf.TrimRight(4);
	if (cstrbuf.Length() > 8)
		cstrbuf.TrimRight(cstrbuf.Length() - 8);
	if (!Shell_InputBox(title, "Enter disk label:", cstrbuf))
		cstrbuf.Set(_name);
	if (cstrbuf.Length() > 26)
		cstrbuf.TrimRight(cstrbuf.Length() - 26);

	Shell_ProgressBar(title, "Formatting...");
	bool result = mb02_formatdisk(mem.dstName, numtrk, numsec, cstrbuf.String());
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

	cstrbuf = "empty";
	cstrbuf += baseExt;

	if (!Shell_InputBox(title, "Enter image name:", cstrbuf))
		return false;

	char *ext = strcpy(mem.srcName, cstrbuf.String()) + cstrbuf.Length();
	while (ext > mem.srcName && *ext != '.')
		ext--;

	if (strncasecmp(ext, baseExt, 4) != 0)
		strcat(mem.srcName, baseExt);

	return mbd ?
		Shell_EmptyMbd(mem.srcName, false) :
		Shell_EmptyTrd(mem.srcName, false);
}
//---------------------------------------------------------------------------------------
void Shell_AutoloadESXDOS(char *fullName, bool disk = false)
{
	char *ptr = mem.dstName;
	*ptr = '\0';

	for (char *token = strchr(fullName, '/'); token;
			*token = '/', token = strchr(++token, '/')) {

		strcpy(ptr++, "/");
		*token = '\0';
		f_stat(fullName, &mem.finfo);
		ptr += sprintf(ptr, mem.finfo.fname);
	}

	f_stat(fullName, &mem.finfo);
	sprintf(ptr, "/%s", mem.finfo.fname);
	strlwr(mem.dstName);

	__TRACE("Autoloading \"%s\" file in ESXDOS...\n", mem.dstName);

	CPU_Quick_Reset();

	SystemBus_Write(0xc00020, 0); // use bank 0
	SystemBus_Write(0xC00017, 0x10); //specPort7ffd

	static const byte esxAutoloader[] = {
		// tapein
		0x00, 0x00, 0xf3, 0x31, 0x80, 0x5b, 0x06, 0x01,
		0xcf, 0x8b, 0xdd, 0x21, 0x98, 0x5b, 0x3e, 0x24,
		0x06, 0x00, 0xcf, 0x8b, 0xd8, 0xaf, 0xcf, 0x90,

		// trd
		0x00, 0x00, 0xf3, 0x31, 0x80, 0x5b, 0x3e, 0x60,
		0xcf, 0x85, 0xdd, 0x21, 0x9a, 0x5b, 0x3e, 0x60,
		0x01, 0x2a, 0x00, 0xcf, 0x80, 0xd8, 0x3e, 0xfd,
		0xcf, 0x90
	};

	int i = disk ? 0x18 : 0,
		l = disk ? 0x32 : 0x18;
	dword addr = 0x800000 | (5 << 13) | (0x1B80 >> 1);
	for (; i < l; i += 2)
		SystemBus_Write(addr++, ((word) esxAutoloader[i]) | (esxAutoloader[i + 1] << 8));

	ptr = mem.dstName;
	l = strlen(ptr) + 1;
	for (i = 0; i < l; i += 2)
		SystemBus_Write(addr++, ((word) ptr[i]) | (ptr[i + 1] << 8));

	CPU_ModifyPC(0x5b82, 1);
}
//---------------------------------------------------------------------------------------
void Shell_ScreenBrowser(char *fullName)
{
	while (true) {
		dword pos = 0x1800, addr = VIDEO_PAGE_PTR;
		for (; pos < 0x1b00; pos++)
			SystemBus_Write(addr + pos, 0);

		if (f_open(&mem.fdst, fullName, FA_READ) == FR_OK) {
			byte data;
			UINT res;

			for (pos = 0; pos < 0x1b00; pos++) {
				if (f_read(&mem.fdst, &data, 1, &res) != FR_OK)
					break;
				if (res == 0)
					break;

				SystemBus_Write(addr++, data);
			}

			f_close(&mem.fdst);

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

			Read(&mem.ra, table_buffer, files_sel);
			strlwr(mem.ra.name);

			char *ext = mem.ra.name + strlen(mem.ra.name);
			while (ext > mem.ra.name && *ext != '.')
				ext--;

			if (strcmp(ext, ".scr") == 0 || mem.ra.size == 6912 || mem.ra.size == 6144) {
				sniprintf(fullName, PATH_SIZE, "%s%s", get_current_dir(), mem.ra.name);
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
	if (f_open(&mem.fdst, fullName, FA_READ) == FR_OK) {
		byte data;
		UINT res, total, ascii;

		for (total = 0, ascii = 0; total < 1024; total++) {
			if (f_read(&mem.fdst, &data, 1, &res) != FR_OK)
				break;
			if (res == 0)
				break;

			if ((data > 7 && data <= 13) || (data >= 32 && data < 127))
				ascii++;
		}

		f_close(&mem.fdst);

		if ((100.0f / ((float) total / ascii)) > 80.0f) {
			Shell_TextViewer(fullName);
			return true;
		}
		else
			return Shell_HexViewer(fullName);
	}

	return false;
}
//---------------------------------------------------------------------------------------
bool Shell_Receiver(const char *inputName)
{
	const char *title = "XMODEM File Transfer";
	cstrbuf = inputName;
	if (!Shell_InputBox(title, "Enter output name:", cstrbuf) || cstrbuf.Length() == 0)
		return false;

	make_short_name(mem.shortName, 32, cstrbuf.String());
	sniprintf(mem.dstName, PATH_SIZE, "%s%s", get_current_dir(), cstrbuf.String());

	if (FileExists(mem.dstName) && !Shell_MessageBox("Overwrite", "Do you want to overwrite", mem.shortName, "", MB_YESNO, 0050, 0115))
		return false;

	sniprintf(mem.srcName, PATH_SIZE, "%s%lo.tmp", get_current_dir(), get_fattime());

	if (f_open(&mem.fdst, mem.srcName, FA_READ | FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
		Shell_MessageBox(title, "Receiving transmission...", "", "", MB_PROGRESS, 0070);

		char status = 0;
		XModem modem(&mem.fdst);

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
			DWORD dc = mem.fdst.fsize;
			byte c;
			UINT r;

			// try to find proper ending
			for (int i = 0; dc > 1 && i < 127; i++) {
				f_lseek(&mem.fdst, --dc);
				f_read(&mem.fdst, &c, 1, &r);

				if (c != 0x1A)
					break;
			}

			cstrbuf.Format("%lu", ++dc);
			if (Shell_InputBox(title, "Confirm file size:", cstrbuf))
				sscanf(cstrbuf.String(), "%lu", &dc);

			if (dc > 0 && dc < mem.fdst.fsize) {
				f_lseek(&mem.fdst, dc);
				f_truncate(&mem.fdst);
			}

			f_close(&mem.fdst);

			if (FileExists(mem.dstName))
				f_unlink(mem.dstName);
			f_rename(mem.srcName, mem.dstName);
		}
		else {
			f_close(&mem.fdst);
			f_unlink(mem.srcName);

			Shell_MessageBox("Error", "Transmission failed!", mem.shortName);
		}
	}

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
		if (!ModComb(MOD_ALT_0 | MOD_CTRL_0))
			continue;

		Read(&mem.ra, table_buffer, files_sel);

		sniprintf(files_lastName, sizeof(files_lastName), "%s", mem.ra.name);

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
			if (strcmp(mem.ra.name, "..") != 0) {
				if (mem.ra.sel)
					files_sel_number--;
				else
					files_sel_number++;

				mem.ra.sel = (mem.ra.sel + 1) % 2;
				Write(&mem.ra, table_buffer, files_sel);
			}

			hide_sel();
			files_sel++;
			show_sel();
		}
		else if ((key == '+' || key == '=' || key == '-' || key == '/' || key == '\\') && files_size > 0) {
			hide_sel();
			files_sel_number = 0;

			for (int i = 0; i < files_size; i++) {
				Read(&mem.ra, table_buffer, i);

				if (strcmp(mem.ra.name, "..") != 0) {
					if (key == '+' || key == '=')
						mem.ra.sel = 1;
					else if (key == '-')
						mem.ra.sel = 0;
					else if (key == '/' || key == '\\')
						mem.ra.sel = (mem.ra.sel + 1) % 2;

					if (mem.ra.sel)
						files_sel_number++;
					Write(&mem.ra, table_buffer, i);
				}

				if ((i & 0x3f) == 0)
					CycleMark();
			}

			show_sel(true);
		}
		else if (key >= '1' && key <= '4') {
			if (specConfig.specDiskIf != SpecDiskIf_DivMMC && (mem.ra.attr & AM_DIR) == 0) {
				sniprintf(mem.fullName, PATH_SIZE, "%s%s", get_current_dir(), mem.ra.name);

				char *ext = mem.ra.name + strlen(mem.ra.name);
				while (ext > mem.ra.name && *ext != '.')
					ext--;

				int i = key - '1';
				const char *mountPoint;
				bool extMatch = false, difValid = false, imgValid = false;

				if (strcasestr(".trd.fdi.scl", ext)) {
					extMatch = true;

					if (specConfig.specDiskIf == SpecDiskIf_Betadisk) {
						difValid = true;

						if (open_dsk_image(i, mem.fullName) == 0) {
							beta_set_disk_wp(i, specConfig.specBdiImages[i].writeProtect);

							strcpy(specConfig.specBdiImages[i].name, mem.fullName);
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

						if (mb02_open_image(i, mem.fullName) == 0) {
							specConfig.specMB2Images[i].writeProtect |= mb02_is_disk_wp(i);
							mb02_set_disk_wp(i, (specConfig.specMB2Images[i].writeProtect != 0));

							strcpy(specConfig.specMB2Images[i].name, mem.fullName);
							SaveConfig();

							mountPoint = &"@1\0@2\0@3\0@4"[i * 3]; // BS-DOS drive number
							imgValid = true;
						}
					}
				}

				if (extMatch) {
					if (imgValid) {
						make_short_name(mem.shortName, 35, mem.ra.name);
						sniprintf(mem.sname, 35, "mounted into %s drive...", mountPoint);

						Shell_Toast(mem.shortName, mem.sname);
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
			Shell_HelpViewer();

			init_screen();
			show_table();
			show_sel(true);
		}
		else if (key == K_F2) {
			sniprintf(destinationPath, sizeof(destinationPath), "/%s", get_current_dir());
			make_short_name(mem.shortName, 36, destinationPath);
			Shell_Toast("Current working directory:", mem.shortName, 0070);
		}
		else if (key == K_F3) {
			if ((mem.ra.attr & AM_DIR) == 0) {
				sniprintf(mem.fullName, PATH_SIZE, "%s%s", get_current_dir(), mem.ra.name);

				char *ext = mem.ra.name + strlen(mem.ra.name);
				while (ext > mem.ra.name && *ext != '.')
					ext--;

				strlwr(ext);
				if (strcmp(ext, ".scr") == 0 || mem.ra.size == 6912 || mem.ra.size == 6144)
					Shell_ScreenBrowser(mem.fullName);

				else if (!Shell_Viewer(mem.fullName)) {
					show_sel(true);
					continue;
				}

				init_screen();
				show_table();
				show_sel(true);
			}
		}
		else if (key == K_F4) {
			if ((mem.ra.attr & AM_DIR) == 0) {
				sniprintf(mem.fullName, PATH_SIZE, "%s%s", get_current_dir(), mem.ra.name);

				if (Shell_HexViewer(mem.fullName, true)) {
					init_screen();
					show_table();
					show_sel(true);
				}
			}
		}
		else if (key == K_F5) {
			hide_sel();
			if (Shell_Copy(mem.ra.name, false))
				read_dir();
			show_sel(true);
		}
		else if (key == K_F6) {
			hide_sel();
			if (Shell_Copy(mem.ra.name, true))
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

			if (Shell_Delete(mem.ra.name)) {
				int last_files_sel = files_sel;
				read_dir();
				files_sel = last_files_sel;
			}

			show_sel(true);
		}
		else if (key == K_F9) {
			hide_sel();
			bool createNew = ((mem.ra.attr & AM_DIR) != 0), success = false;

			if (!createNew) {
				char *ext = mem.ra.name + strlen(mem.ra.name);
				while (ext > mem.ra.name && *ext != '.')
					ext--;
				strlwr(ext);

				if (strcmp(ext, ".trd") == 0) {
					if (Shell_EmptyTrd(mem.ra.name))
						success = true;
				}
				else if (strstr(".mbd.mb2", ext)) {
					if (Shell_EmptyMbd(mem.ra.name))
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

			if (Shell_Receiver((mem.ra.attr & AM_DIR) ? "" : mem.ra.name)) {
				int last_files_sel = files_sel;
				read_dir();
				files_sel = last_files_sel;
			}

			show_table();
			show_sel(true);
		}
		else if (key == K_ESC || key == K_F10 || key == K_F12) {
			break;
		}
		else if (key == K_RETURN) {
			if ((mem.ra.attr & AM_DIR) != 0) {
				hide_sel();
				enter_dir(mem.ra.name);
				show_table();
				show_sel();
			}
			else {
				sniprintf(mem.fullName, PATH_SIZE, "%s%s", get_current_dir(), mem.ra.name);
				strlwr(mem.ra.name);

				char *ext = mem.ra.name + strlen(mem.ra.name);
				while (ext > mem.ra.name && *ext != '.')
					ext--;

				if (strstr(".tap.tzx", ext)) {
					if (specConfig.specDiskIf == SpecDiskIf_DivMMC && strcmp(ext, ".tap") == 0)
						Shell_AutoloadESXDOS(mem.fullName);
					else {
						Tape_SelectFile(mem.fullName);

						make_short_name(mem.shortName, 35, mem.ra.name);
						Shell_Toast("Tape file mounted:", mem.shortName);
					}
					break;
				}
				else if (strcmp(ext, ".sna") == 0) {
					sniprintf(specConfig.snaName, sizeof(specConfig.snaName), "/%s", mem.fullName);

					if (LoadSnapshot(mem.fullName, mem.ra.name))
						break;
				}
				else if (strstr(".trd.fdi.scl", ext)) {
					if (specConfig.specDiskIf == SpecDiskIf_Betadisk) {
						if (open_dsk_image(0, mem.fullName) == 0) {
							beta_set_disk_wp(0, specConfig.specBdiImages[0].writeProtect);

							strcpy(specConfig.specBdiImages[0].name, mem.fullName);
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
					else if (specConfig.specDiskIf == SpecDiskIf_DivMMC && strcmp(ext, ".trd") == 0) {
						Shell_AutoloadESXDOS(mem.fullName, true);
						break;
					}
					else
						Shell_MessageBox("Mount disk error", "Cannot mount disk image", "to the current Disk IF");
				}
				else if (strstr(".mbd.mb2", ext)) {
					if (specConfig.specDiskIf != SpecDiskIf_MB02)
						Shell_MessageBox("Mount disk error", "Cannot mount disk image", "to the current Disk IF");

					else if (mb02_open_image(0, mem.fullName) == 0) {
						specConfig.specMB2Images[0].writeProtect |= mb02_is_disk_wp(0);
						mb02_set_disk_wp(0, (specConfig.specMB2Images[0].writeProtect != 0));

						strcpy(specConfig.specMB2Images[0].name, mem.fullName);
						SaveConfig();
						break;
					}
					else
						Shell_MessageBox("Mount disk error", "Invalid disk image");
				}
				else if (strcmp(ext, ".scr") == 0) {
					Shell_ScreenBrowser(mem.fullName);

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
