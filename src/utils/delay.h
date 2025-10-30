#ifndef DELAY_H
#define DELAY_H

#ifdef _WIN32
#include <windows.h>
#elif __linux__
// remove compilation errors when testing memory leaks
// with valgrind on unix based systems
#include <unistd.h>
#endif

void delay(int millisecond);

#endif
