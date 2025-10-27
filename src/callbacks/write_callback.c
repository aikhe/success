#include "write_callback.h"
#include "../types/types.h"

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
  size_t total_bytes = size * nmemb;
  struct Memory *mem = (struct Memory *)userdata;

  char *temp = realloc(mem->response, mem->size + total_bytes + 1);
  if (!temp) {
    fprintf(stderr, "[ERROR] Failed to realloc memory. \n");
    return 0;
  }

  mem->response = temp;
  memcpy(&(mem->response[mem->size]), ptr, total_bytes);
  mem->size += total_bytes;
  mem->response[mem->size] = '\0';

  return total_bytes;
}
