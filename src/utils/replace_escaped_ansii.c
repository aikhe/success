#include "replace_escaped_ansii.h"

char *replace_escaped_ansi(char *input) {
  const char *pattern = "\\033";
  const char esc = '\x1b';
  size_t len = strlen(input);
  char *output = malloc(len + 1);
  if (!output)
    return NULL;

  size_t i = 0, j = 0;
  while (i < len) {
    if (strncmp(&input[i], pattern, 4) == 0) {
      output[j++] = esc;
      i += 4;
    } else {
      output[j++] = input[i++];
    }
  }
  output[j] = '\0';
  return output;
}
