#include <locale.h>

#include "utils/compat.h"
#include "pages/introduction.h"
#include "pages/menu.h"
#include "pages/tools.h"

void enableVirtualTerminal(void) {
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
#endif
}

int main(void) {
  // Set system default locale for wide-character UTF-8 support
  setlocale(LC_ALL, "");

#ifdef _WIN32
  if (!setlocale(LC_ALL, "en_US.UTF-8")) {
    setlocale(LC_ALL, "C.UTF-8");
  }
#endif

  enableVirtualTerminal();

  introduction_page();

  return 0;
}
