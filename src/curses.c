#include <locale.h>

#include "pages/introduction.h"

void enableVirtualTerminal() {
#ifdef _WIN32
  // enable ANSI support for windows cmd
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD dwMode = 0;
  GetConsoleMode(hOut, &dwMode);
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(hOut, dwMode);

  // set both input and output to UTF-8
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);

  // support ACS symbols (e.g. pwdcurses box)
  // SetConsoleOutputCP(437);
  // SetConsoleCP(437);
#endif
}

int main(void) {
  // Set locale BEFORE calling any curses functions
  setlocale(LC_ALL, "en_US.UTF-8");

// On Windows, try UTF-8 locale if the above fails
#ifdef _WIN32
  if (!setlocale(LC_ALL, "en_US.UTF-8")) {
    setlocale(LC_ALL, "C.UTF-8");
  }
#endif

  enableVirtualTerminal();

  introduction_page();

  return 0;
}
