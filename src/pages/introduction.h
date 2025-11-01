#ifndef INTRODUCTION_H
#define INTRODUCTION_H

#include <windows.h>

// remove redefinition errors from wincon.h macro
#undef MOUSE_MOVED

// removes implicit definition warning when using heavy borders via curses
#define _XOPEN_SOURCE_EXTENDED 1
#define PDC_WIDE 1

#include <curses.h>

#include "../utils/delay.h"

void introduction_page(void);
void signup_page(void);
void login_page(void);
void success_menu_page(void);
void student_page(void);
void teacher_page(void);

#endif
