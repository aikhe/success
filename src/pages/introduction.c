#include "introduction.h"
#include "curses.h"
#include <stdio.h>
#include <string.h>
#include <winuser.h>

#define RGB_TO_NCURSES(r, g, b)                                                \
  ((r) * 1000 / 255), ((g) * 1000 / 255), ((b) * 1000 / 255)

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

WINDOW *draw_centered_win(WINDOW *win, const char *text, int y, int text_pos_x,
                          int with_border) {
  // int h, w;
  // getmaxyx(win, h, w);

  // wbkgd(win, COLOR_PAIR(5));

  int w = getmaxx(win);

  wrefresh(win);

  while (*text == '\n')
    text++;

  int num_lines = count_lines(text);
  int max_width = max_line_width(text);

  // No borders, so dimensions are just content size
  int box_w = max_width;
  int box_h = num_lines;
  // int start_y = (h - box_h) / 2;
  int start_x = (w - box_w) / 2;

  WINDOW *boxwin = newwin(box_h, box_w, y, start_x);
  wbkgd(boxwin, COLOR_PAIR(5));

  if (with_border) {
    cchar_t vline, hline, ul, ur, ll, lr;
    setcchar(&vline, L"┃", 0, 0, NULL);
    setcchar(&hline, L"━", 0, 0, NULL);
    setcchar(&ul, L"┏", 0, 0, NULL);
    setcchar(&ur, L"┓", 0, 0, NULL);
    setcchar(&ll, L"┗", 0, 0, NULL);
    setcchar(&lr, L"┛", 0, 0, NULL);

    wborder_set(boxwin, &vline, &vline, &hline, &hline, &ul, &ur, &ll, &lr);
  }

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

      // Trim trailing whitespace
      int trimmed_len = strlen(line_buf);
      while (trimmed_len > 0 && (line_buf[trimmed_len - 1] == ' ' ||
                                 line_buf[trimmed_len - 1] == '\t' ||
                                 line_buf[trimmed_len - 1] == '\r')) {
        trimmed_len--;
      }
      line_buf[trimmed_len] = '\0';

      if (trimmed_len > 0) {
        mvwaddstr(boxwin, line_num, text_pos_x, line_buf);
      }

      line_num++;
      line_start = &text[i + 1];
    }
  }

  wrefresh(boxwin);
  return boxwin;
}

static WINDOW *draw_input_bar(int *content_y, int *content_x, int *content_w) {
  int w = getmaxx(stdscr);

  int bar_h = 3;
  int bar_w = 70;
  int bar_wb = 72;

  // int y = h / 2;
  int y = *content_y;
  int x = (w - bar_w) / 2;
  int xb = (w - bar_wb) / 2;

  WINDOW *ibox = newwin(bar_h, bar_w, y, x);
  WINDOW *ibox_bars = newwin(bar_h, bar_wb, y, xb);
  wbkgd(ibox, COLOR_PAIR(1));
  wbkgd(ibox_bars, COLOR_PAIR(5));
  werase(ibox);

  // Draw vertical bars on left and right edges
  cchar_t vbar;
  setcchar(&vbar, L"│", 0, 0, NULL);
  for (int i = 0; i < bar_h; i++) {
    mvwadd_wch(ibox_bars, i, 0, &vbar);
    mvwadd_wch(ibox_bars, i, bar_wb - 1, &vbar);
  }

  // Prompt on the left (adjusted for the left bar)
  wattron(ibox, COLOR_PAIR(6) | A_DIM);
  mvwaddstr(ibox, 1, 1, "> ");
  wattroff(ibox, COLOR_PAIR(6) | A_DIM);

  // Content area geometry for caller
  int inner_x = 3;                   // inside ibox, after prompt
  int inner_w = bar_w - inner_x - 2; // leave right padding
  if (inner_w < 1)
    inner_w = 1;
  if (content_y)
    *content_y = y + 1;
  if (content_x)
    *content_x = x + inner_x;
  if (content_w)
    *content_w = inner_w;

  wrefresh(ibox_bars);
  wrefresh(ibox);

  mvaddstr(y + bar_h, x, "enter send");
  const char *right_hint = "Success Platform";
  int right_x = x + bar_w - (int)strlen(right_hint);
  if (right_x > x) {
    mvaddstr(y + bar_h, right_x, right_hint);
  }

  refresh();

  return ibox;
}

static void render_input(WINDOW *ibox, int content_y, int content_x,
                         int content_w, const char *buf, int cursor_pos) {
  if (!ibox)
    return;

  // Convert absolute to window-relative
  int wy, wx;
  getbegyx(ibox, wy, wx);
  int rel_y = content_y - wy;
  int rel_x = content_x - wx;

  // Clear input area with the bar's background attributes
  wattron(ibox, COLOR_PAIR(1) | A_DIM);
  mvwhline(ibox, rel_y, rel_x, ' ', content_w);
  wattroff(ibox, COLOR_PAIR(1) | A_DIM);

  // Draw text inside the ibox
  int len = (int)strlen(buf);
  if (len > content_w)
    len = content_w;
  mvwaddnstr(ibox, rel_y, rel_x, buf, len);

  // Move visible cursor
  int cx = rel_x + (cursor_pos > content_w ? content_w : cursor_pos);
  wmove(ibox, rel_y, cx);
  leaveok(ibox, FALSE);
  curs_set(1);
  wrefresh(ibox);
}

void draw_sub_win(WINDOW *win, const char *text, int text_pos_y, int text_pos_x,
                  int color) {
  // int w = getmaxx(win);

  wbkgd(win, COLOR_PAIR(5));

  wrefresh(win);

  int num_lines = count_lines(text);

  // No borders, so dimensions are just content size
  int box_h = num_lines + 1;

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

      // Trim trailing whitespace
      int trimmed_len = strlen(line_buf);
      while (trimmed_len > 0 && (line_buf[trimmed_len - 1] == ' ' ||
                                 line_buf[trimmed_len - 1] == '\t' ||
                                 line_buf[trimmed_len - 1] == '\r')) {
        trimmed_len--;
      }
      line_buf[trimmed_len] = '\0';

      if (trimmed_len > 0) {
        wattron(win, COLOR_PAIR(color));
        int lead = 0;
        while (line_buf[lead] == ' ' || line_buf[lead] == '\t') {
          lead++;
        }
        mvwaddstr(win, line_num - 1, text_pos_x + lead, line_buf + lead);
        wattroff(win, COLOR_PAIR(color));
      }

      line_num++;
      line_start = &text[i + 1];
    }
  }

  wrefresh(win);
}

static void draw_status_bar(const char *left, const char *right) {
  int h = getmaxy(stdscr);
  int w = getmaxx(stdscr);

  // Avoid affecting cursor placement when drawing the status bar
  leaveok(stdscr, TRUE);
  attron(A_REVERSE);
  mvhline(h - 1, 0, ' ', w);
  mvaddnstr(h - 1, 1, left, w - 2);
  int right_x = w - (int)strlen(right) - 2;
  if (right_x > 1) {
    mvaddstr(h - 1, right_x, right);
  }
  attroff(A_REVERSE);
  refresh();
}

void introduction_page(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  start_color();
  // Input bar colors (pair 3): dark gray background if possible
  if (can_change_color() && COLORS > 16) {
    short DARK_GRAY = 16;
    short GRAY_2 = 17;
    short FOREGROUND = 18;
    short ORANGE = 19;
    short BACKGROUND = 20;

    init_color(DARK_GRAY, RGB_TO_NCURSES(30, 30, 30));
    init_color(GRAY_2, RGB_TO_NCURSES(128, 128, 128));
    init_color(FOREGROUND, RGB_TO_NCURSES(238, 238, 238));
    init_color(ORANGE, RGB_TO_NCURSES(243, 173, 128));
    init_color(BACKGROUND, RGB_TO_NCURSES(10, 10, 10));

    init_pair(1, COLOR_WHITE, DARK_GRAY);
    init_pair(2, GRAY_2, BACKGROUND);
    init_pair(3, FOREGROUND, BACKGROUND);
    init_pair(4, ORANGE, BACKGROUND);

    init_pair(5, COLOR_WHITE, BACKGROUND);
    init_pair(6, ORANGE, DARK_GRAY);
  }

  wbkgd(stdscr, COLOR_PAIR(5));

  // Keep stdscr from moving the hardware cursor; we'll position it via input
  // bar
  leaveok(stdscr, TRUE);

  const char *ascii_art = R"(
  █▀▀▀ █  █ █▀▀▀ █▀▀▀ █▀▀█ █▀▀▀ █▀▀▀
  ▀▀▀█ █░░█ █░░░ █░░░ █▀▀▀ ▀▀▀█ ▀▀▀█
  ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀▀▀
                             v0.1.10
  )";

  const char *ucc = R"(
       █  █ █▀▀▀ █▀▀▀
       █░░█ █░░░ █░░░
       ▀▀▀▀ ▀▀▀▀ ▀▀▀▀
                             v0.1.10
  )";

  // const char *input_bar = R"(
  // ┃                                                ┃
  // ┃ >                                              ┃
  // ┃                                                ┃
  // )";

  const char *intro = R"(
    SUCCESS is a learning platform for UCCians designed to make
   studying engaging and effective. It offers tools like quizzes,
   flashcards, streaks, and leaderboards—plus personalized study
   methods like Pomodoro and active recall—to help students learn
                     smarter and achieve more.
  )";

  const char *success = R"(
    SUCCESS
  )";

  const char *auth = R"(
  [l] login          To start using success ✦
  [s] signup                Make an account ✔
  [x] exit                    Stop and quit ⟡
  )";

  const char *auth_coms = R"(
  [l] login 
  [s] signup
  [x] exit  
  )";

  WINDOW *boxwin = draw_centered_win(stdscr, ascii_art, 1, 0, 0);
  draw_sub_win(boxwin, ascii_art, 0, 0, 2);
  draw_sub_win(boxwin, ucc, 0, 0, 3);
  WINDOW *intro_win = draw_centered_win(stdscr, intro, 6, 0, 0);
  draw_sub_win(intro_win, success, 0, 0, 2);
  WINDOW *authwin = draw_centered_win(stdscr, auth, 13, 0, 1);
  draw_sub_win(authwin, auth_coms, 0, 0, 4);

  // Draw middle input bar and bottom status bar
  int input_y = 18, input_x = 0, input_w = 0;
  WINDOW *input_bar = draw_input_bar(&input_y, &input_x, &input_w);
  draw_status_bar("success  v0.1.10", "tab | HOME");

  char input_buf[512] = {0};
  int cursor_pos = 0;
  render_input(input_bar, input_y, input_x, input_w, input_buf, cursor_pos);

  int ch;
  while ((ch = getch()) != 'q') {
    if (ch == KEY_RESIZE) {
      resize_term(0, 0);
      clear();
      WINDOW *boxwin = draw_centered_win(stdscr, ascii_art, 1, 0, 0);
      draw_sub_win(boxwin, ucc, 0, 0, 2);
      WINDOW *intro_win = draw_centered_win(stdscr, intro, 6, 0, 0);
      draw_sub_win(intro_win, success, 0, 0, 2);
      WINDOW *authwin = draw_centered_win(stdscr, auth, 13, 0, 1);
      draw_sub_win(authwin, auth_coms, 0, 0, 2);

      int input_y = 18, input_x = 0, input_w = 0;
      WINDOW *input_bar = draw_input_bar(&input_y, &input_x, &input_w);
      draw_status_bar("success  v0.1.10", "tab | HOME");
      render_input(input_bar, input_y, input_x, input_w, input_buf, cursor_pos);
    } else if (ch == 10 || ch == KEY_ENTER) {
      // Submit (no-op for now)
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
      if (cursor_pos > 0) {
        cursor_pos--;
        input_buf[cursor_pos] = '\0';
        render_input(input_bar, input_y, input_x, input_w, input_buf,
                     cursor_pos);
      }
    } else if (ch >= 32 && ch <= 126) {
      if (cursor_pos < (int)sizeof(input_buf) - 1) {
        input_buf[cursor_pos++] = (char)ch;
        input_buf[cursor_pos] = '\0';
        render_input(input_bar, input_y, input_x, input_w, input_buf,
                     cursor_pos);
      }
    }
  }

  clear();
  refresh();
  endwin();
}
