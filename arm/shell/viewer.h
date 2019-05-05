#ifndef SHELL_VIEWER_H_INCLUDED
#define SHELL_VIEWER_H_INCLUDED

void Shell_HelpViewer(bool running = false);
void Shell_TextViewer(const char *fullName, bool simpleMode = false, const char *description = 0);
bool Shell_HexViewer(const char *fullName, bool editMode = false, bool fromTextView = false);

#endif
