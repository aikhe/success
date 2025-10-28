#include "base64.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/";

static int is_base64_char(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

char *base64_encode(const BYTE *buf, size_t len) {
  char *ret;
  size_t ret_len = 4 * ((len + 2) / 3);
  ret = (char *)malloc(ret_len + 1);
  if (!ret)
    return NULL;

  size_t i = 0, j = 0;
  BYTE char_array_3[3];
  BYTE char_array_4[4];
  size_t pos = 0;

  while (len--) {
    char_array_3[i++] = *(buf++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] =
          ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] =
          ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (i = 0; i < 4; i++)
        ret[pos++] = base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] =
        ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] =
        ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; j < i + 1; j++)
      ret[pos++] = base64_chars[char_array_4[j]];

    while (i++ < 3)
      ret[pos++] = '=';
  }

  ret[pos] = '\0';
  return ret;
}

BYTE *base64_decode(const char *str, size_t *out_len) {
  size_t len = strlen(str);
  size_t i = 0, j = 0, in_ = 0;
  BYTE char_array_4[4], char_array_3[3];
  BYTE *ret = (BYTE *)malloc(len * 3 / 4 + 1);
  if (!ret)
    return NULL;
  size_t pos = 0;

  while (len-- && (str[in_] != '=') && is_base64_char(str[in_])) {
    char_array_4[i++] = str[in_];
    in_++;
    if (i == 4) {
      for (i = 0; i < 4; i++)
        char_array_4[i] =
            (BYTE)(strchr(base64_chars, char_array_4[i]) - base64_chars);

      char_array_3[0] =
          (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] =
          ((char_array_4[1] & 0x0f) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x03) << 6) + char_array_4[3];

      for (i = 0; i < 3; i++)
        ret[pos++] = char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 4; j++)
      char_array_4[j] = 0;

    for (j = 0; j < 4; j++)
      char_array_4[j] =
          (BYTE)(strchr(base64_chars, char_array_4[j]) - base64_chars);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] =
        ((char_array_4[1] & 0x0f) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x03) << 6) + char_array_4[3];

    for (j = 0; j < i - 1; j++)
      ret[pos++] = char_array_3[j];
  }

  *out_len = pos;
  return ret;
}
