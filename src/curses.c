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
  setlocale(LC_ALL, "");

  enableVirtualTerminal();

  introduction_page();

  return 0;
}
