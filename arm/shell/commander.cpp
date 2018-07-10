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
//---------------------------------------------------------------------------------------
byte get_sel_attr(FRECORD &fr)
{
	byte result = 0117;

	if ((fr.attr & AM_DIR) == 0) {
		strlwr(fr.name);

		char *ext = fr.name + strlen(fr.name);
		while (ext > fr.name && *ext != '.')
			ext--;

		if (strcmp(ext, ".trd") == 0 || strcmp(ext, ".fdi") == 0 || strcmp(ext, ".scl") == 0)
			result = 0115;
		else if (strcmp(ext, ".tap") == 0 || strcmp(ext, ".tzx") == 0)
			result = 0114;
		else if (strcmp(ext, ".sna") == 0)
			result = 0113;
		else if (strcmp(ext, ".scr") == 0 || strcmp(ext, ".rom") == 0)
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
	display_path(get_current_dir(), 0, 22, 32);

	FRECORD fr;

	for (int i = 0; i < FILES_PER_ROW; i++) {
		for (int j = 0; j < 2; j++) {
			int col = j * 16;
			int row = i + 2;
			int pos = i + j * FILES_PER_ROW + files_table_start;

			Read(&fr, table_buffer, pos);

			char sname[15];
			make_short_name(sname, 15, fr.name);

			if (pos < files_size) {
				if (fr.sel) {
					WriteChar(col, row, 0xDD);
					WriteAttr(col, row, 0116);
				}
				else {
					WriteChar(col, row, 0xB3);
					WriteAttr(col, row, 0117);
				}

				WriteStrAttr(col + 1, row, sname, get_sel_attr(fr), 14);
			}
			else {
				WriteAttr(col, row, 0117, 15);
				WriteStr(col, row, "\xB3", 15);
			}
		}
	}

	if (too_many_files)
		WriteStrAttr(17, 17, "TOO MANY FILES", 0126, 14);
	else if (files_size == 0)
		WriteStrAttr(1, 2, ".. no files ..", 0112, 14);
}
//---------------------------------------------------------------------------------------
void hide_sel()
{
	if (files_size != 0) {
		FRECORD fr;
		Read(&fr, table_buffer, files_sel);

		int col = files_sel_pos_x * 16;
		int row = files_sel_pos_y + 2;

		if (fr.sel) {
			WriteChar(col, row, 0xDD);
			WriteAttr(col, row, 0116);
		}
		else {
			WriteChar(col, row, 0xB3);
			WriteAttr(col, row, 0117);
		}

		WriteAttr(col + 1, row, get_sel_attr(fr), 14);
		WriteChar(col + 15, row, 0xB3);
		WriteAttr(col + 15, row, 0117);
	}

	WriteStr(1, 19, "", 30);
	WriteStr(1, 19, "", 30);
}
//---------------------------------------------------------------------------------------
void show_sel(bool redraw = false)
{
	if (calc_sel() || redraw)
		show_table();

	if (files_size != 0) {
		FRECORD fr;
		Read(&fr, table_buffer, files_sel);

		int col = files_sel_pos_x * 16;
		int row = files_sel_pos_y + 2;
		byte borders = 0117, fill = 0171;

		if (fr.sel) {
			WriteChar(col, row, 0xDB);
			borders = 0116;
			fill = 0161;
		}
		else
			WriteChar(col, row, 0xDE);

		WriteAttr(col, row, borders);
		WriteAttr(col + 1, row, fill, 14);
		WriteChar(col + 15, row, 0xDD);
		WriteAttr(col + 15, row, borders);

		char sname[31];
		make_short_name(sname, sizeof(sname), fr.name);
		WriteStrAttr(1, 19, sname, get_sel_attr(fr), 30);
		WriteStrAttr(24, 18, "\xC4\xC4\xC4\xC4\xC4\xC4", 0117, 6);

		if (files_sel_number > 0) {
			sniprintf(sname, sizeof(sname), "(%u)", files_sel_number);
			size_t len = strlen(sname);
			WriteStrAttr(30 - len, 18, sname, 0116, len);
		}

		if ((fr.date & 0x1ff) == 0 || strcmp(sname, "..") == 0) {
			WriteStr(1, 20, "---------- -----", 16);
		}
		else {
			sniprintf(sname, sizeof(sname), "%04u-%02u-%02u %02u:%02u",
				(1980 + (fr.date >> 9)),
				(fr.date >> 5) & 0x0f,
				(fr.date & 0x1f),
				(fr.time >> 11) & 0x1f,
				(fr.time >> 5) & 0x3f);
			WriteStr(1, 20, sname, 16);
		}

		if ((fr.attr & AM_DIR) != 0)
			sniprintf(sname, sizeof(sname), "       <DIR>");
		else if (fr.size < 9999)
			sniprintf(sname, sizeof(sname), "%10u B", fr.size);
		else if (fr.size < 0x100000)
			sniprintf(sname, sizeof(sname), "%6u.%.2u kB", fr.size >> 10, ((fr.size & 0x3ff) * 100) >> 10);
		else
			sniprintf(sname, sizeof(sname), "%6u.%.2u MB", fr.size >> 20, ((fr.size & 0xfffff) * 100) >> 20);

		WriteStr(19, 20, sname, 12);
	}
}
//---------------------------------------------------------------------------------------
void init_screen()
{
	SystemBus_Write(0xc00022, 0x8000); // Enable shell border

	ClrScr(0006);
	DrawFrame(0, 18, 32,  4, 0117, "\xC3\xC4\xB4\xB3\xC0\xC4\xD9");
	DrawFrame(0,  1, 16, 18, 0117, "\xD1\xCD\xD1\xB3\xC3\xC4\xC1");
	DrawFrame(16, 1, 16, 18, 0117, "\xD1\xCD\xD1\xB3\xC1\xC4\xB4");

	WriteStrAttr(0, 0, " Speccy2010 v" VERSION " File Manager ", 0114, 32);
	WriteStrAttr(0, 23, "1hlp2cd3vw4trd5cp6mv7mkd8del9fmt", 0050, 32);

	const byte fnKeys[9] = { 0, 4, 7, 10, 14, 17, 20, 24, 28 };
	for (int i = 0; i < 9; i++)
		WriteAttr(fnKeys[i], 23, 0107);

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

	char shortName[20];
	make_short_name(shortName, sizeof(shortName), sname);

	char progressBar[25];
	for (int i = 0; i < 24; i++)
		progressBar[i] = 0xB0;
	progressBar[24] = 0;

	show_table();

	int res;

	do {
		if (FileExists(dstName)) {
			if (askForOverwrite != NULL && overwrite != NULL) {
				if (*askForOverwrite == true) {
					*askForOverwrite = false;
					*overwrite = Shell_MessageBox("Overwrite", "Overwrite all exist files?", NULL, NULL, MB_YESNO, 0050, 0115);
				}
				if (*overwrite == false)
					return false;
			}
			else if (!Shell_MessageBox("Overwrite", "Do you want to overwrite", dstName, NULL, MB_YESNO, 0050, 0115))
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

			Shell_MessageBox("Processing", shortName, "", progressBar, MB_NO, 0070);

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
					byte prog = (progress * 24);

					if (prog > 0)
						WriteStrAttr(4, 11, progressBar, 0062, prog);
					WDT_Kick();
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
	if (!Shell_InputBox(title, "Enter new name/path :", name))
		return false;

	bool newPath = false;

	char pname[PATH_SIZE];
	char pnameNew[PATH_SIZE];

	if (name.GetSymbol(0) == '/') {
		if (name.GetSymbol(name.Length() - 1) != '/')
			name += '/';
		newPath = true;
	}

	if (files_sel_number == 0 || !newPath) {
		sniprintf(pname, sizeof(pname), "%s%s", get_current_dir(), _name);

		if (newPath)
			sniprintf(pnameNew, sizeof(pnameNew), "%s%s", name.String() + 1, _name);
		else
			sniprintf(pnameNew, sizeof(pnameNew), "%s%s", get_current_dir(), name.String());

		if (Shell_CopyItem(pname, pnameNew, move) && !newPath && move) {
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
				sniprintf(pname, sizeof(pname), "%s%s", get_current_dir(), fr.name);
				sniprintf(pnameNew, sizeof(pnameNew), "%s%s", name.String() + 1, fr.name);

				if (!Shell_CopyItem(pname, pnameNew, move, &askForOverwrite, &overwrite))
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
	if (!Shell_InputBox("Create folder", "Enter name :", name))
		return false;

	char pname[PATH_SIZE];
	sniprintf(pname, sizeof(pname), "%s%s", get_current_dir(), name.String());

	int res = f_mkdir(pname);
	if (res == FR_OK) {
		sniprintf(files_lastName, sizeof(files_lastName), "%s", name.String());
		return true;
	}

	char shortName[20];
	make_short_name(shortName, sizeof(shortName), name);

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

	char shortName[20];
	make_short_name(shortName, sizeof(shortName), sname);

	Shell_MessageBox("Error", "Cannot delete the item", shortName, fatErrorMsg[res]);
	show_table();

	return false;
}
//---------------------------------------------------------------------------------------
bool Shell_Delete(const char *name)
{
	if (files_sel_number == 0 && strcmp(name, "..") == 0)
		return false;

	char sname[PATH_SIZE];

	if (files_sel_number > 0)
		sniprintf(sname, sizeof(sname), "selected %u item%s", files_sel_number, files_sel_number > 1 ? "s" : "");
	else
		make_short_name(sname, 21, name);

	strcat(sname, "?");

	if (!Shell_MessageBox("Delete", "Do you wish to delete", sname, "", MB_YESNO))
		return false;

	if (files_sel_number == 0) {
		sniprintf(sname, sizeof(sname), "%s%s", get_current_dir(), name);
		Shell_DeleteItem(sname);
	}
	else {
		FRECORD fr;

		for (int i = 0; i < files_size; i++) {
			Read(&fr, table_buffer, i);

			if (fr.sel) {
				sniprintf(sname, sizeof(sname), "%s%s", get_current_dir(), fr.name);
				if (!Shell_DeleteItem(sname))
					break;
			}
		}
	}

	return true;
}
//---------------------------------------------------------------------------------------
bool Shell_EmptyTrd(const char *_name)
{
	char name[PATH_SIZE];
	bool result = false;

	if (strlen(_name) == 0) {
		CString newName = "empty.trd";
		if (!Shell_InputBox("Format", "Enter name :", newName))
			return false;

		show_table();
		sniprintf(name, sizeof(name), "%s", newName.String());
		result = true;

		char *ext = name + strlen(name);
		while (ext > name && *ext != '.')
			ext--;
		strlwr(ext);

		if (strcmp(ext, ".trd") != 0)
			sniprintf(name, sizeof(name), "%s.trd", newName.String());
		else
			sniprintf(name, sizeof(name), "%s", newName.String());
	}
	else {
		sniprintf(name, sizeof(name), "%s", _name);

		char *ext = name + strlen(name);
		while (ext > name && *ext != '.')
			ext--;
		strlwr(ext);

		if (strcmp(ext, ".trd") != 0)
			return false;

		make_short_name(name, 21, _name);
		strcat(name, "?");

		if (!Shell_MessageBox("Format", "Do you wish to format", name, "", MB_YESNO, 0050, 0115))
			return false;
		show_table();

		sniprintf(name, sizeof(name), "%s", _name);
	}

	CString label = name;
	label.TrimRight(4);
	if (label.Length() > 8)
		label.TrimRight(label.Length() - 8);

	if (!Shell_InputBox("Format", "Enter disk label :", label))
		return false;
	show_table();

	char pname[PATH_SIZE];
	sniprintf(pname, sizeof(pname), "%s%s", get_current_dir(), name);

	const byte zero[0x10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	const byte sysArea[] = { 0x00, 0x00, 0x01, 0x16, 0x00, 0xf0, 0x09, 0x10 };
	char sysLabel[8];
	strncpy(sysLabel, label.String(), sizeof(sysLabel));

	int res;
	UINT r;
	FIL dst;

	{
		res = f_open(&dst, pname, FA_WRITE | FA_OPEN_EXISTING);
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
		Shell_MessageBox("Error", "Cannot format", name, fatErrorMsg[res]);
	}

	return result;
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
		}
		else
			break;

		char key = GetKey();
		if (key != ' ')
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

			if (strcmp(ext, ".scr") == 0) {
				sniprintf(fullName, PATH_SIZE, "%s%s", get_current_dir(), fr.name);
				break;
			}

			if (--i == 0)
				return;
		}
	}
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
			if (specConfig.specDiskIf == SpecDiskIf_Betadisk && (fr.attr & AM_DIR) == 0) {
				char fullName[PATH_SIZE];
				sniprintf(fullName, PATH_SIZE, "%s%s", get_current_dir(), fr.name);

				strlwr(fr.name);

				char *ext = fr.name + strlen(fr.name);
				while (ext > fr.name && *ext != '.')
					ext--;

				if (strcmp(ext, ".trd") == 0 || strcmp(ext, ".fdi") == 0 || strcmp(ext, ".scl") == 0) {
					int i = key - '1';

					if (fdc_open_image(i, fullName)) {
						floppy_disk_wp(i, &specConfig.specBdiImages[i].writeProtect);

						strcpy(specConfig.specBdiImages[key - '1'].name, fullName);
						SaveConfig();
					}
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
			Shell_Viewer("speccy2010.hlp");

			init_screen();
			show_table();
			show_sel(true);
		}
		else if (key == K_F2) {
			sniprintf(destinationPath, sizeof(destinationPath), "/%s", get_current_dir());
		}
		else if (key == K_F3) {
			if ((fr.attr & AM_DIR) == 0) {
				char fullName[PATH_SIZE];
				sniprintf(fullName, PATH_SIZE, "%s%s", get_current_dir(), fr.name);
				strlwr(fr.name);

				char *ext = fr.name + strlen(fr.name);
				while (ext > fr.name && *ext != '.')
					ext--;

				if (strcmp(ext, ".scr") == 0) {
					Shell_ScreenBrowser(fullName);
				}
				else {
					Shell_Viewer(fullName);
				}

				init_screen();
				show_table();
				show_sel(true);
			}
		}
		else if (key == K_F4) {
			hide_sel();
			if (Shell_EmptyTrd(""))
				read_dir();
			show_sel(true);
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
			if (Shell_EmptyTrd(fr.name))
				read_dir();

			show_sel(true);
		}
		else if (key == K_ESC || key == K_F10 || key == K_F12) {
			break;
		}
		else if (key == K_F11) {
			static byte testVideo = 0;
			SystemBus_Write(0xc00049, ++testVideo);
			testVideo &= 7;
		}
		else if (key == K_RETURN) {
			if ((fr.attr & AM_DIR) != 0) {
				hide_sel();
				enter_dir(fr.name);
				show_table();
				show_sel();
			}
			else {
				char fullName[PATH_SIZE];
				sniprintf(fullName, PATH_SIZE, "%s%s", get_current_dir(), fr.name);

				strlwr(fr.name);

				char *ext = fr.name + strlen(fr.name);
				while (ext > fr.name && *ext != '.')
					ext--;

				if (strcmp(ext, ".tap") == 0 || strcmp(ext, ".tzx") == 0) {
					Tape_SelectFile(fullName);
					break;
				}
				else if (strcmp(ext, ".sna") == 0) {
					sniprintf(specConfig.snaName, sizeof(specConfig.snaName), "/%s", fullName);

					if (LoadSnapshot(fullName)) {
						break;
					}
				}
				else if (strcmp(ext, ".trd") == 0 || strcmp(ext, ".fdi") == 0 || strcmp(ext, ".scl") == 0) {
					if (specConfig.specDiskIf == SpecDiskIf_Betadisk && fdc_open_image(0, fullName)) {
						sniprintf(specConfig.snaName, sizeof(specConfig.snaName), "/%s.00.sna", fullName);

						floppy_disk_wp(0, &specConfig.specBdiImages[0].writeProtect);

						strcpy(specConfig.specBdiImages[0].name, fullName);
						SaveConfig();

						CPU_Start();
						CPU_Reset(true);
						CPU_Reset(false);
						CPU_Stop();

						CPU_ModifyPC(0, 0);

						if (specConfig.specMachine == SpecRom_Classic48)
							SystemBus_Write(0xc00017, (1 << 4) | (1 << 5));
						else
							SystemBus_Write(0xc00017, (1 << 4));

						SystemBus_Write(0xc00018, 0x0001);

						break;
					}
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
