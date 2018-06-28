#ifndef SHELL_MENU_H_INCLUDED
#define SHELL_MENU_H_INCLUDED

#include "menuItem.h"

void Shell_SettingsMenu();
void Shell_DisksMenu();
void Shell_RomCfgMenu();

void Shell_Menu(const char *title, CMenuItem *menu, int menuSize);

#endif
