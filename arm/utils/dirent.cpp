#include "dirent.h"
#include "../system/sdram.h"

dword table_buffer = 0;
int files_size = 0;
int files_table_start;
int files_sel;
int files_sel_number = 0;
char files_lastName[_MAX_LFN + 1] = "";
bool too_many_files = false;
byte files_sel_pos_x = 0, files_sel_pos_y = 0;

char path[PATH_SIZE] = "";
//---------------------------------------------------------------------------------------
const char *get_current_dir()
{
	return path;
}
//---------------------------------------------------------------------------------------
byte comp_name(int a, int b)
{
	FRECORD ra, rb;

	Read(&ra, table_buffer, a);
	Read(&rb, table_buffer, b);

	strlwr(ra.name);
	strlwr(rb.name);

	if ((ra.attr & AM_DIR) && !(rb.attr & AM_DIR))
		return true;
	else if (!(ra.attr & AM_DIR) && (rb.attr & AM_DIR))
		return false;
	else
		return strcmp(ra.name, rb.name) <= 0;
}
//---------------------------------------------------------------------------------------
void swap_name(int a, int b)
{
	FRECORD ra, rb;

	Read(&ra, table_buffer, a);
	Read(&rb, table_buffer, b);

	Write(&ra, table_buffer, b);
	Write(&rb, table_buffer, a);
}
//---------------------------------------------------------------------------------------
void qsort(int l, int h)
{
	int i = l;
	int j = h;

	int k = (l + h) / 2;

	while (true) {
		while (i < k && comp_name(i, k))
			i++;
		while (j > k && comp_name(k, j))
			j--;

		if (i == j)
			break;
		swap_name(i, j);

		if (i == k)
			k = j;
		else if (j == k)
			k = i;

		WDT_Kick();
	}

	if (l < k - 1)
		qsort(l, k - 1);
	if (k + 1 < h)
		qsort(k + 1, h);

	WDT_Kick();
}
//---------------------------------------------------------------------------------------
void read_dir()
{
	if (table_buffer == 0)
		table_buffer = Malloc(FILES_SIZE, sizeof(FRECORD));

	files_size = 0;
	too_many_files = false;
	files_table_start = 0;
	files_sel = 0;
	files_sel_number = 0;

	FRECORD fr;

	if (strlen(path) != 0) {
		fr.attr = AM_DIR;
		fr.sel = 0;
		fr.size = 0;
		strcpy(fr.name, "..");
		Write(&fr, table_buffer, files_size++);
	}

	DIR dir;
	FRESULT r;

	int pathSize = strlen(path);
	if (pathSize > 0)
		path[pathSize - 1] = 0;

	r = f_opendir(&dir, path);
	if (pathSize > 0)
		path[pathSize - 1] = '/';

	while (r == FR_OK) {
		FILINFO fi;
		char lfn[_MAX_LFN + 1];
		fi.lfname = lfn;
		fi.lfsize = sizeof(lfn);

		r = f_readdir(&dir, &fi);

		if (r != FR_OK || fi.fname[0] == 0)
			break;
		if (fi.fattrib & (AM_HID | AM_SYS))
			continue;

		if (lfn[0] == 0)
			strcpy(lfn, fi.fname);

		if (files_size >= FILES_SIZE) {
			too_many_files = true;

			if (strlen(path) != 0)
				files_size = 1;
			else
				files_size = 0;

			break;
		}

		fr.sel = 0;
		fr.attr = fi.fattrib;
		fr.size = fi.fsize;
		fr.date = fi.fdate;
		fr.time = fi.ftime;

		strcpy(fr.name, lfn);
		Write(&fr, table_buffer, files_size);

		files_size++;
		WDT_Kick();

/* TODO progress indicator
		if ((files_size & 0x3f) == 0)
			CycleMark(0, FILES_PER_ROW + 4);
*/
	}

	if (files_size > 0 && files_size < 0x100)
		qsort(0, files_size - 1);

	if (strlen(files_lastName) != 0) {
		FRECORD fr;

		for (int i = 0; i < files_size; i++) {
			Read(&fr, table_buffer, i);
			if (strcmp(fr.name, files_lastName) == 0) {
				files_sel = i;
				break;
			}
		}

		WDT_Kick();
	}
}
//---------------------------------------------------------------------------------------
void leave_dir()
{
	FRECORD fr;

	byte i = strlen(path);
	char dir_name[_MAX_LFN + 1];

	if (i != 0) {
		i--;
		path[i] = 0;

		while (i != 0 && path[i - 1] != '/')
			i--;
		strcpy(dir_name, &path[i]);

		path[i] = 0;
		read_dir();

		for (files_sel = 0; files_sel < files_size; files_sel++) {
			Read(&fr, table_buffer, files_sel);
			if (strcmp(fr.name, dir_name) == 0)
				break;
		}

		if ((files_table_start + FILES_PER_ROW * 2 - 1) < files_sel) {
			files_table_start = files_sel;
		}

		files_sel_pos_y = ((files_sel - files_table_start) % FILES_PER_ROW);
		files_sel_pos_x = ((files_sel - files_table_start) / FILES_PER_ROW);
	}
}
//---------------------------------------------------------------------------------------
void enter_dir(const char *name)
{
	if (strcmp(name, "..") == 0)
		leave_dir();

	else if (strlen(path) + strlen(name) + 1 < PATH_SIZE) {
		strcpy(files_lastName, "");

		strcat(path, name);
		strcat(path, "/");
		read_dir();

		files_sel_pos_y = ((files_sel - files_table_start) % FILES_PER_ROW);
		files_sel_pos_x = ((files_sel - files_table_start) / FILES_PER_ROW);
	}
}
//---------------------------------------------------------------------------------------
bool calc_sel()
{
	if (files_size == 0)
		return false;

	if (files_sel >= files_size)
		files_sel = files_size - 1;
	else if (files_sel < 0)
		files_sel = 0;

	bool redraw = false;

	while (files_sel < files_table_start) {
		files_table_start -= FILES_PER_ROW;
		redraw = true;
	}

	while (files_sel >= files_table_start + FILES_PER_ROW * 2) {
		files_table_start += FILES_PER_ROW;
		redraw = true;
	}

	files_sel_pos_x = ((files_sel - files_table_start) / FILES_PER_ROW);
	files_sel_pos_y = ((files_sel - files_table_start) % FILES_PER_ROW);

	return redraw;
}
//---------------------------------------------------------------------------------------
