#include "delay.h"

void delay(int millisecond) {
#ifdef _WIN32
  Sleep(millisecond);
#elif __linux__
  usleep(millisecond * 1000);
#endif
}
