#include "grep_string.h"

char *grep_string(const char *data) {
  if (*data == '\0')
    return NULL;

  const char *line_start = data;
  const char *new_line;
  char line[1024];

  while ((new_line = strchr(line_start, '\n')) != NULL) {
    size_t line_len = new_line - line_start;

    if (line_len >= sizeof(line)) {
      line_len = sizeof(line) - 1;
    }

    memcpy(line, line_start, line_len);
    line[line_len] = '\0';

    const char *fo = strstr(line, "X-Goog-Upload-URL:");
    if (fo != NULL) {
      const char *url_start = strstr(fo, "https://");
      if (url_start) {
        const char *url_end = url_start;
        while (*url_end != '\r') {
          url_end++;
        }

        size_t url_len = url_end - url_start;
        char *url = malloc(url_len + 1);
        if (url) {
          memcpy(url, url_start, url_len);
          url[url_len] = '\0';
        }
        return url;
      }
      return NULL;
    }

    line_start = new_line + 1;
  }

  return NULL;
}
