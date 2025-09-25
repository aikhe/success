#include <curl/curl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  bool is_success = true;

  if (!is_success) {
    fprintf(stderr, "[ERROR] Operation failed. \n");
    return EXIT_FAILURE;
  }

  printf("[INFO] Success succeeded!");

  return EXIT_SUCCESS;
}
