#include "read_file.h"

char *read_file(const char *filename) {
  FILE *fptr = fopen(filename, "rb");
  if (!fptr)
    return NULL;

  fseek(fptr, 0, SEEK_END);
  long length = ftell(fptr);
  rewind(fptr);

  char *data = malloc(length + 1);
  fread(data, 1, length, fptr);
  data[length] = '\0';
  fclose(fptr);

  return data;
}
