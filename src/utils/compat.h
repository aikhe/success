#ifndef COMPAT_H
#define COMPAT_H

#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED 1
#endif

#include <stddef.h>
#include <stdbool.h>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <conio.h>
  #include <direct.h>
  #include <io.h>
  #undef MOUSE_MOVED
#else
  #include <unistd.h>
  #include <termios.h>
  #include <sys/stat.h>
  #include <sys/select.h>
  #include <sys/time.h>
#endif

#if defined(__has_include)
  #if __has_include(<ncursesw/curses.h>)
    #include <ncursesw/curses.h>
  #else
    #include <curses.h>
  #endif
#else
  #include <curses.h>
#endif

// Compatibility API
void compat_sleep_ms(int milliseconds);
int compat_kbhit(void);
int compat_getch(void);
int compat_mkdir(const char *path);
void compat_clear_screen(void);

#endif // COMPAT_H
