#include "menuItem.h"
#include "screen.h"

//---------------------------------------------------------------------------------------
CMenuItem::CMenuItem(int _x, int _y, const char *_name, const char *_data)
{
	data = _data;
	int maxLen = (256 - (_x + strlen(_name) * 6)) / 6;
	if (data.Length() > maxLen) {
		data.Delete(0, 1 + data.Length() - maxLen);
		data.Insert(0, "\34");
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
			byte offset = (ptr - name);
			DrawStrAttr(x, y, name, colors[3], offset - 1);
			name += offset;
			size -= offset;
			y++;
		}
	}

	DrawStrAttr(x, y, name, colors[3], size);

	x2 = x + (size * 6);
	byte mod = x2 & 0x07;
	if (mod > 0)
		x2 += (8 - mod);

	if (justInit) {
		maxLen = (256 - x2) / 6;
		return;
	}

	if (param->GetType() == PTYPE_STRING) {
		DrawStr(x2 + 1, y, data, maxLen);
		DrawAttr(x2, y, colors[state], 252 - x2);
	}
	else
		DrawStrAttr(x2 + 1, y, data, colors[state]);
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
			data.Insert(0, "\34");
		}

		Redraw();

		diff = data.Length();
		delNumber -= diff;
		if (delNumber > 0) {
			int xx = x2 + (diff * 6);
			DrawStr(xx + 1, y, "", delNumber);

			if (param->GetType() == PTYPE_STRING)
				delNumber = maxLen - delNumber;

			delNumber *= 6;
			byte mod = xx & 0x07;
			if (mod > 0)
				xx += (8 - mod);

			DrawAttr(xx, y, colors[0], delNumber);
		}
	}
	else if (force)
		Redraw();
}
//---------------------------------------------------------------------------------------
void CMenuItem::UpdateData(const char *_data)
{
	if (strncmp(data, _data, strlen(_data)) != 0) {
		int delNumber = data.Length() - strlen(_data);

		data = _data;
		Redraw();

		if (delNumber > 0) {
			int xx = x2 + (data.Length() * 6);
			DrawStr(xx + 1, y, "", delNumber);

			if (param->GetType() != PTYPE_STRING) {
				delNumber *= 6;
				byte mod = xx & 0x07;
				if (mod > 0)
					xx += (8 - mod);

				DrawAttr(xx, y, colors[0], delNumber);
			}
		}
	}
}
//---------------------------------------------------------------------------------------
void CMenuItem::UpdateState(int _state)
{
	if (state != _state) {
		state = _state;

		int size = strlen(data);
		if (param->GetType() == PTYPE_STRING)
			size = maxLen;
		DrawAttr(x2, y, colors[state], size * 6);
	}
}
//---------------------------------------------------------------------------------------
