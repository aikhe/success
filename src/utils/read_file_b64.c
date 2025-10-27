#include "read_file_b64.h"

unsigned char *read_file_b64(const char *filename, size_t *out_len) {
  FILE *fptr = fopen(filename, "rb");
  if (!fptr)
    return NULL;

  fseek(fptr, 0, SEEK_END);
  long length = ftell(fptr);
  rewind(fptr);

  unsigned char *data = malloc(length);
  size_t read_len = fread(data, 1, length, fptr);
  fclose(fptr);

  *out_len = read_len;
  return data;
}
