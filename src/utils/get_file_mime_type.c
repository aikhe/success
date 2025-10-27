#include "get_file_mime_type.h"

const char *get_file_mime_type(const char *filename) {
  const char *dot = strrchr(filename, '.');
  if (!dot || dot == filename)
    return NULL;

  char *ext = (char *)dot + 1;

  // only supports 3 file types via gamini file api (png, jpeg/jpg, and pdf)
  if (strcmp(ext, "png") == 0)
    return "image/png";
  else if (strcmp(ext, "jpeg") == 0 || strcmp(ext, "jpg") == 0)
    return "image/jpeg";
  else if (strcmp(ext, "pdf") == 0)
    return "application/pdf";
  else
    printf("[Error] Invalid file formats.");

  return NULL;
}
