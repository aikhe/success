#include "delay.h"

void delay(int millisecond) {
  compat_sleep_ms(millisecond);
}
