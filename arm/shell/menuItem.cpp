#include "menuItem.h"
#include "screen.h"

//---------------------------------------------------------------------------------------
CMenuItem::CMenuItem(int _x, int _y, const char *_name, const char *_data)
{
	data = _data;
	int maxLen = 32 - (x + strlen(_name));
	if (data.Length() > maxLen) {
		data.Delete(0, 1 + data.Length() - maxLen);
		data.Insert(0, "~");
	}

	Init(_x, _y, _name, NULL);
}
//---------------------------------------------------------------------------------------
CMenuItem::CMenuItem(int _x, int _y, const char *_name, const CParameter *_param)
{
	Init(_x, _y, _name, _param);
}
//---------------------------------------------------------------------------------------
void CMenuItem::Init(int _x, int _y, const char *_name, const CParameter *_param)
{
	x = _x;
	y = origY = _y;

	param = _param;
	origName = _name;
	state = 0;

	colors[0] = 0107; /* inactive */
	colors[1] = 0071; /* hover */
	colors[2] = 0120; /* active */
	colors[3] = 0007; /* label */
}
//---------------------------------------------------------------------------------------
void CMenuItem::Redraw(bool justInit)
{
	char *name = (char *) origName;
	size = strlen(origName);

	y = origY;
	char *ptr = name;
	while (*ptr != 0) {
		if (*ptr++ == '\n') {
			int offset = (ptr - name);
			WriteStrAttr(x, y, name, colors[3], offset - 1);
			name += offset;
			size -= offset;
			y++;
		}
	}

	WriteStrAttr(x, y, name, colors[3], size);

	if (justInit) {
		maxLen = 31 - (x + size);
		return;
	}

	if (param->GetType() == PTYPE_STRING) {
		WriteStr(x + size, y, data, maxLen);
		WriteAttr(x + size, y, colors[state], maxLen);
	}
	else
		WriteStrAttr(x + size, y, data, colors[state]);
}
//---------------------------------------------------------------------------------------
void CMenuItem::UpdateData(bool force)
{
	Redraw(true);

	if (param != NULL) {
		int delNumber = data.Length();

		param->GetValueText(data);
		int diff = data.Length() - maxLen;
		if (diff == 1)
			maxLen++;
		else if (diff > 1) {
			data.Delete(0, ++diff);
			data.Insert(0, "~");
		}

		Redraw();

		delNumber -= data.Length();
		if (delNumber > 0) {
			int x2 = x + size + strlen(data);
			WriteStr(x2, y, "", delNumber);

			if (param->GetType() == PTYPE_STRING)
				delNumber = maxLen;
			WriteAttr(x2, y, colors[0], delNumber);
		}
	}
	else if (force)
		Redraw();
}
//---------------------------------------------------------------------------------------
void CMenuItem::UpdateData(const char *_data)
{
	if (strncmp(data, _data, strlen(_data)) != 0) {
		int delNumber = strlen(data) - strlen(_data);

		data = _data;
		Redraw();

		if (delNumber > 0) {
			int x2 = x + size + strlen(data);
			WriteStr(x2, y, "", delNumber);

			if (param->GetType() != PTYPE_STRING)
				WriteAttr(x2, y, colors[0], delNumber);
		}
	}
}
//---------------------------------------------------------------------------------------
void CMenuItem::UpdateState(int _state)
{
	if (state != _state) {
		state = _state;

		int x2 = x + size;
		int size = strlen(data);

		if (param->GetType() == PTYPE_STRING)
			size = maxLen;
		WriteAttr(x2, y, colors[state], size);
	}
}
//---------------------------------------------------------------------------------------
