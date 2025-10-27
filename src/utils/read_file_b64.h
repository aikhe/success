#ifndef READFILEB64_H
#define READFILEB64_H

#include <stdio.h>
#include <stdlib.h>

unsigned char *read_file_b64(const char *filename, size_t *out_len);

#endif
