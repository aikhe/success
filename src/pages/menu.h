#ifndef MENU_H
#define MENU_H

// removes implicit definition warning when using heavy borders via curses
#define _XOPEN_SOURCE_EXTENDED 1
#define PDC_WIDE 1

#include "curses.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void menu(void);

#endif
