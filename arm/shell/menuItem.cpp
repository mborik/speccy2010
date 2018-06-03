#include "menuItem.h"
#include "screen.h"

//---------------------------------------------------------------------------------------
CMenuItem::CMenuItem(int _x, int _y, const char *_name, const char *_data)
{
	data = _data;
	int maxLen = 32 - (x + strlen(name));
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
	y = _y;

	param = _param;
	name = _name;
	state = 0;

	colors[0] = 0107; /* inactive */
	colors[1] = 0071; /* hover */
	colors[2] = 0120; /* active */
	colors[3] = 0007; /* label */
}
//---------------------------------------------------------------------------------------
void CMenuItem::Redraw()
{
	WriteStr(x, y, name);
	WriteAttr(x, y, colors[3], strlen(name));

	int x2 = x + strlen(name);
	int size = strlen(data);

	WriteStr(x2, y, data);
	WriteAttr(x2, y, colors[state], size);
}
//---------------------------------------------------------------------------------------
void CMenuItem::UpdateData()
{
	if (param != NULL) {
		int delNumber = data.Length();

		param->GetValueText(data);
		int maxLen = 32 - (x + strlen(name));
		if (data.Length() > maxLen) {
			data.Delete(0, 1 + data.Length() - maxLen);
			data.Insert(0, "~");
		}

		delNumber -= data.Length();

		Redraw();

		if (delNumber > 0) {
			int x2 = x + strlen(name) + strlen(data);
			WriteStr(x2, y, "", delNumber);
			WriteAttr(x2, y, colors[0], delNumber);
		}
	}
}
//---------------------------------------------------------------------------------------
void CMenuItem::UpdateData(const char *_data)
{
	if (strncmp(data, _data, strlen(_data)) != 0) {
		int delNumber = strlen(data) - strlen(_data);

		data = _data;
		Redraw();

		if (delNumber > 0) {
			int x2 = x + strlen(name) + strlen(data);
			WriteStr(x2, y, "", delNumber);
			WriteAttr(x2, y, colors[0], delNumber);
		}
	}
}
//---------------------------------------------------------------------------------------
void CMenuItem::UpdateState(int _state)
{
	if (state != _state) {
		state = _state;

		int x2 = x + strlen(name);
		int size = strlen(data);

		WriteAttr(x2, y, colors[state], size);
	}
}
//---------------------------------------------------------------------------------------
