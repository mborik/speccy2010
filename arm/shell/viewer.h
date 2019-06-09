#ifndef SHELL_VIEWER_H_INCLUDED
#define SHELL_VIEWER_H_INCLUDED

#define SCREEN_WIDTH 42
#define VIEWER_LINES 21
#define VIEWER_BYTES (VIEWER_LINES * 8)

void Shell_HelpViewer(bool running = false);
void Shell_TextViewer(const char *fullName, bool simpleMode = false, const char *description = 0);
bool Shell_HexViewer(const char *fullName, bool editMode = false, bool fromTextView = false);

#endif
