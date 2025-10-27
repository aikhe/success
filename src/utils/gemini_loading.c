#include "gemini_loading.h"

bool is_generating = false;

void delay(int millisecond) {
#ifdef _WIN32
  Sleep(millisecond);
#elif __linux__
  usleep(millisecond * 1000);
#endif
}

void *gemini_loading(void *arg) {
  printf("\033[92mThinking");

  while (is_generating) {
    printf(".");
    delay(500);
  }
  printf("\033[0m");

  return NULL;
}
