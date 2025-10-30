#include "gemini_loading.h"

bool is_generating = false;

void *gemini_loading(void *arg) {
  printf("\033[92mThinking");

  while (is_generating) {
    printf(".");
    delay(500);
  }
  printf("\033[0m");

  return NULL;
}
