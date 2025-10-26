#include "introduction.h"
#include <stdio.h>
#include <string.h>

// Helper function to count lines in a string
int count_lines(const char *text) {
  // Skip leading newlines
  while (*text == '\n')
    text++;

  int lines = 0;
  for (int i = 0; text[i] != '\0'; i++) {
    if (text[i] == '\n')
      lines++;
  }

  printf("%d\n", lines);

  return lines;
}

// Count the number of UTF-8 characters in a string of given byte length
size_t utf8_strlen(const char *s, size_t byte_len) {
  size_t char_count = 0;
  for (size_t i = 0; i < byte_len; i++) {
    unsigned char c = (unsigned char)s[i];
    // Count bytes that are not continuation bytes (10xxxxxx)
    if ((c & 0xC0) != 0x80) {
      char_count++;
    }
  }
  return char_count;
}

// Helper function to get the longest line width (trimmed)
int max_line_width(const char *text) {
  int max_width = 0;
  const char *start = text;

  while (*start) {
    const char *end = strchr(start, '\n');
    if (!end)
      end = start + strlen(start);

    size_t byte_len = end - start;
    int char_len = utf8_strlen(start, byte_len);

    if (char_len > max_width) {
      max_width = char_len;
    }

    start = end;
    if (*start == '\n')
      start++;
  }

  printf("%d\n", max_width);

  return max_width;
}

void draw_centered(WINDOW *win, const char *text, int y) {
  // int h, w;
  // getmaxyx(win, h, w);

  int w = getmaxx(win);

  wrefresh(win);

  while (*text == '\n')
    text++;

  int num_lines = count_lines(text);
  int max_width = max_line_width(text);

  // No borders, so dimensions are just content size
  int box_w = max_width - 2;
  int box_h = num_lines;
  // int start_y = (h - box_h) / 2;
  int start_x = (w - box_w) / 2;

  WINDOW *boxwin = newwin(box_h, box_w, y, start_x);

  // cchar_t vline, hline, ul, ur, ll, lr;
  // setcchar(&vline, L"┃", 0, 0, NULL);
  // setcchar(&hline, L"━", 0, 0, NULL);
  // setcchar(&ul, L"┏", 0, 0, NULL);
  // setcchar(&ur, L"┓", 0, 0, NULL);
  // setcchar(&ll, L"┗", 0, 0, NULL);
  // setcchar(&lr, L"┛", 0, 0, NULL);
  //
  // wborder_set(boxwin, &vline, &vline, &hline, &hline, &ul, &ur, &ll, &lr);

  // Print multi-line text line by line
  int line_num = 0;
  const char *line_start = text;

  for (int i = 0; text[i] != '\0' && line_num < box_h; i++) {
    if (text[i] == '\n') {
      // Extract and print the current line
      int line_len = &text[i] - line_start;
      char line_buf[256];
      strncpy(line_buf, line_start, line_len);
      line_buf[line_len] = '\0';

      // Trim leading whitespace
      char *trimmed = line_buf;
      while (*trimmed == ' ' || *trimmed == '\t')
        trimmed++;

      // Trim trailing whitespace
      int trimmed_len = strlen(trimmed);
      while (trimmed_len > 0 && (trimmed[trimmed_len - 1] == ' ' ||
                                 trimmed[trimmed_len - 1] == '\t' ||
                                 trimmed[trimmed_len - 1] == '\r')) {
        trimmed_len--;
      }
      trimmed[trimmed_len] = '\0';

      if (trimmed_len > 0) {
        wattron(boxwin, COLOR_PAIR(1));
        mvwaddstr(boxwin, line_num, 0, trimmed);
        wattroff(boxwin, COLOR_PAIR(1));
      }

      line_num++;
      line_start = &text[i + 1];
    }
  }

  wrefresh(boxwin);
}

void introduction_page(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  start_color();
  init_pair(1, COLOR_CYAN, COLOR_BLACK);

  const char *ascii_art = R"(
  █▀▀▀ █  █ █▀▀▀ █▀▀▀ █▀▀█ █▀▀▀ █▀▀▀
  ▀▀▀█ █░░█ █░░░ █░░░ █▀▀▀ ▀▀▀█ ▀▀▀█
  ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀▀▀
                             v0.1.10

  hello this is me!
  )";

  // const char *s = R"(
  // █▀▀▀
  // ▀▀▀█
  // ▀▀▀▀
  // )";

  const char *new_text = R"(
  Lorem Ipsum is simply dummy text of the printing and typesetting industry.
  Lorem Ipsum has been the industry's standard dummy text ever since the
  1500s.
  )";

  draw_centered(stdscr, ascii_art, 1);
  draw_centered(stdscr, new_text, 8);

  int ch;
  while ((ch = getch()) != 'q') {
    if (ch == KEY_RESIZE) {
      resize_term(0, 0);
      clear();
      draw_centered(stdscr, ascii_art, 1);
      draw_centered(stdscr, new_text, 12);
    }
  }

  clear();
  refresh();
  endwin();
}
