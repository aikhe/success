#include "compat.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef _WIN32
static struct termios orig_termios;
static bool raw_mode_active = false;
static int peek_char = -1;

static void disable_raw_mode(void) {
  if (raw_mode_active) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    raw_mode_active = false;
  }
}

static void enable_raw_mode(void) {
  if (!raw_mode_active) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    raw_mode_active = true;
  }
}
#endif

void compat_sleep_ms(int milliseconds) {
#ifdef _WIN32
  Sleep(milliseconds);
#else
  struct timespec ts;
  ts.tv_sec = milliseconds / 1000;
  ts.tv_nsec = (milliseconds % 1000) * 1000000L;
  nanosleep(&ts, NULL);
#endif
}

int compat_kbhit(void) {
#ifdef _WIN32
  return _kbhit();
#else
  if (peek_char != -1) {
    return 1;
  }
  enable_raw_mode();
  unsigned char ch;
  int nread = read(STDIN_FILENO, &ch, 1);
  disable_raw_mode();
  if (nread == 1) {
    peek_char = ch;
    return 1;
  }
  return 0;
#endif
}

int compat_getch(void) {
#ifdef _WIN32
  return _getch();
#else
  if (peek_char != -1) {
    int ch = peek_char;
    peek_char = -1;
    return ch;
  }
  enable_raw_mode();
  unsigned char ch = 0;
  while (read(STDIN_FILENO, &ch, 1) <= 0) {
    compat_sleep_ms(10);
  }
  disable_raw_mode();
  return ch;
#endif
}

int compat_mkdir(const char *path) {
#ifdef _WIN32
  int ret = _mkdir(path);
#else
  int ret = mkdir(path, 0755);
#endif
  if (ret != 0 && errno == EEXIST) {
    return 0;
  }
  return ret;
}

void compat_clear_screen(void) {
  // Clear screen and scrollback using standard ANSI VT100 escapes
  printf("\033[H\033[2J\033[3J");
  fflush(stdout);
}
