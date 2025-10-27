#ifndef GEMINILOADING_H
#define GEMINILOADING_H

#include <stdbool.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
// remove compilation errors when testing memory leaks
// with valgrind on unix based systems
#include <unistd.h>
#endif

void delay(int millisecond);

extern bool is_generating;

void *gemini_loading(void *arg);

#endif
