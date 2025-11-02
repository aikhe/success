#include "test.h"

#define RGB_TO_NCURSES(r, g, b)                                                \
  ((r) * 1000 / 255), ((g) * 1000 / 255), ((b) * 1000 / 255)

void define_colors(void) {
  start_color();
  if (can_change_color() && COLORS > 16) {
    short DARK_GRAY = 16;
    short GRAY_2 = 17;
    short FOREGROUND = 18;
    short ORANGE = 19;
    short BLACK = 20;
    short BLUE = 21;
    short GRAY_3 = 22;
    short GRAY_4 = 23;
    short GRAY_5 = 24;
    short GRAY_6 = 25;
    short PURPLE = 26;
    short GREEN = 27;
    short CYAN = 28;
    short PINK = 29;

    init_color(DARK_GRAY, RGB_TO_NCURSES(30, 30, 30));
    init_color(GRAY_2, RGB_TO_NCURSES(128, 128, 128));
    init_color(FOREGROUND, RGB_TO_NCURSES(238, 238, 238));
    init_color(ORANGE, RGB_TO_NCURSES(243, 173, 128));
    init_color(BLACK, RGB_TO_NCURSES(10, 10, 10));
    init_color(BLUE, RGB_TO_NCURSES(92, 156, 245));
    init_color(GRAY_3, RGB_TO_NCURSES(53, 53, 53));
    init_color(GRAY_4, RGB_TO_NCURSES(16, 16, 16));
    init_color(GRAY_5, RGB_TO_NCURSES(18, 18, 18));
    init_color(GRAY_6, RGB_TO_NCURSES(34, 34, 34));
    init_color(PURPLE, RGB_TO_NCURSES(198, 210, 255));
    init_color(GREEN, RGB_TO_NCURSES(185, 248, 207));
    init_color(CYAN, RGB_TO_NCURSES(116, 212, 255));
    init_color(PINK, RGB_TO_NCURSES(255, 201, 201));

    init_pair(1, COLOR_WHITE, DARK_GRAY);
    init_pair(2, GRAY_2, BLACK);
    init_pair(3, FOREGROUND, BLACK);
    init_pair(4, ORANGE, BLACK);
    init_pair(5, COLOR_WHITE, BLACK);
    init_pair(6, ORANGE, DARK_GRAY);
    init_pair(7, BLACK, BLUE);
    init_pair(8, COLOR_WHITE, GRAY_3);
    init_pair(9, GRAY_2, GRAY_4);
    init_pair(10, COLOR_WHITE, GRAY_4);
    init_pair(11, GRAY_2, GRAY_5);
    init_pair(12, GRAY_6, BLACK);
    init_pair(13, BLACK, BLACK);
    init_pair(14, COLOR_WHITE, GRAY_5);
    init_pair(15, PURPLE, BLACK);
    init_pair(16, GREEN, BLACK);
    init_pair(17, CYAN, BLACK);
    init_pair(18, PINK, BLACK);
  }
}

void define_colors(void) {
  start_color();
  if (can_change_color() && COLORS > 16) {
    short DARK_GRAY = 16;
    short GRAY_2 = 17;
    short FOREGROUND = 18;
    short ORANGE = 19;
    short BLACK = 20;
    short BLUE = 21;
    short GRAY_3 = 22;
    short GRAY_4 = 23;
    short GRAY_5 = 24;
    short GRAY_6 = 25;
    short PURPLE = 26;
    short GREEN = 27;
    short CYAN = 28;
    short PINK = 29;

    init_color(DARK_GRAY, RGB_TO_NCURSES(30, 30, 30));
    init_color(GRAY_2, RGB_TO_NCURSES(128, 128, 128));
    init_color(FOREGROUND, RGB_TO_NCURSES(238, 238, 238));
    init_color(ORANGE, RGB_TO_NCURSES(243, 173, 128));
    init_color(BLACK, RGB_TO_NCURSES(10, 10, 10));
    init_color(BLUE, RGB_TO_NCURSES(92, 156, 245));
    init_color(GRAY_3, RGB_TO_NCURSES(53, 53, 53));
    init_color(GRAY_4, RGB_TO_NCURSES(16, 16, 16));
    init_color(GRAY_5, RGB_TO_NCURSES(18, 18, 18));
    init_color(GRAY_6, RGB_TO_NCURSES(34, 34, 34));
    init_color(PURPLE, RGB_TO_NCURSES(198, 210, 255));
    init_color(GREEN, RGB_TO_NCURSES(185, 248, 207));
    init_color(CYAN, RGB_TO_NCURSES(116, 212, 255));
    init_color(PINK, RGB_TO_NCURSES(255, 201, 201));

    init_pair(1, COLOR_WHITE, DARK_GRAY);
    init_pair(2, GRAY_2, BLACK);
    init_pair(3, FOREGROUND, BLACK);
    init_pair(4, ORANGE, BLACK);
    init_pair(5, COLOR_WHITE, BLACK);
    init_pair(6, ORANGE, DARK_GRAY);
    init_pair(7, BLACK, BLUE);
    init_pair(8, COLOR_WHITE, GRAY_3);
    init_pair(9, GRAY_2, GRAY_4);
    init_pair(10, COLOR_WHITE, GRAY_4);
    init_pair(11, GRAY_2, GRAY_5);
    init_pair(12, GRAY_6, BLACK);
    init_pair(13, BLACK, BLACK);
    init_pair(14, COLOR_WHITE, GRAY_5);
    init_pair(15, PURPLE, BLACK);
    init_pair(16, GREEN, BLACK);
    init_pair(17, CYAN, BLACK);
    init_pair(18, PINK, BLACK);
  }
}

static void draw_status_bar(int h, int w, char *left, char *mid, char *right) {
  wattron(stdscr, COLOR_PAIR(9));
  mvhline(h - 1, 0, ' ', w);
  wattroff(stdscr, COLOR_PAIR(9));

  int left_x = (int)strlen(left);
  wattron(stdscr, COLOR_PAIR(9));
  mvaddstr(h - 1, left_x, mid);
  wattroff(stdscr, COLOR_PAIR(9));

  wattron(stdscr, COLOR_PAIR(8));
  mvaddnstr(h - 1, 0, left, w - 2);
  wattroff(stdscr, COLOR_PAIR(8));

  int right_x = w - (int)strlen(right);
  wattron(stdscr, COLOR_PAIR(7));
  mvaddstr(h - 1, right_x, right);
  wattroff(stdscr, COLOR_PAIR(7));

  refresh();
}

void render_input(WINDOW *win, int w, int h, int y, int x, char *fieldname) {
  // int max_win_w = getmaxx(stdscr);

  WINDOW *input_win_bars = newwin(h, w + 2, y, x - 1);
  WINDOW *input_win = newwin(h, w, y, x);
  wbkgd(input_win_bars, COLOR_PAIR(2));
  wbkgd(input_win, COLOR_PAIR(1));

  cchar_t vbar;
  setcchar(&vbar, L"┃", 0, 0, NULL);

  for (int i = 0; i < h; i++) {
    mvwadd_wch(input_win_bars, i, 0, &vbar);
    mvwadd_wch(input_win_bars, i, w + 2 - 1, &vbar);
  }

  wattron(input_win, COLOR_PAIR(6));
  mvwaddstr(input_win, 1, 1, fieldname);
  wattroff(input_win, COLOR_PAIR(6));

  wrefresh(input_win_bars);
  wrefresh(input_win);

  // left text relative to the input bar
  mvaddstr(y + h, x, "enter");
  wattron(stdscr, COLOR_PAIR(2));
  mvaddstr(y + h, x + 6, "send");
  wattroff(stdscr, COLOR_PAIR(2));

  // right
  const char *right_hint = "Success Platform";
  int right_x = x + w - (int)strlen(right_hint);
  mvaddstr(y + h, right_x, "Success");
  wattron(stdscr, COLOR_PAIR(2));
  mvaddstr(y + h, right_x + 8, "Platform");
  wattroff(stdscr, COLOR_PAIR(2));

  refresh();
}

WINDOW *render_win(WINDOW *win, int w, int h, int y, int x, const char *content,
                   int bg_color, int bar_color, bool has_bg) {
  WINDOW *input_win_bars = newwin(h, w + 2, y, x - 1);
  WINDOW *input_win = newwin(h, w, y, x);
  wbkgd(input_win_bars, COLOR_PAIR(bar_color));
  wbkgd(input_win, COLOR_PAIR(bg_color));

  if (has_bg) {
    cchar_t vbar;
    setcchar(&vbar, L"┃", 0, 0, NULL);

    for (int i = 0; i < h; i++) {
      mvwadd_wch(input_win_bars, i, 0, &vbar);
      mvwadd_wch(input_win_bars, i, w + 2 - 1, &vbar);
    }
  }

  mvwaddstr(input_win, 0, 0, content);

  wrefresh(input_win_bars);
  wrefresh(input_win);

  refresh();

  return input_win;
}

void test(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(1);

  start_color();
  if (can_change_color() && COLORS > 16) {
    short DARK_GRAY = 16;
    short BLACK = 17;
    short WHITE = 18;

    init_color(DARK_GRAY, RGB_TO_NCURSES(30, 30, 30));
    init_color(BLACK, RGB_TO_NCURSES(0, 0, 0));
    init_color(WHITE, RGB_TO_NCURSES(255, 255, 255));

    init_pair(1, WHITE, DARK_GRAY);
    init_pair(2, BLACK, WHITE);
  }

  wbkgd(stdscr, COLOR_PAIR(1));
  clear();

  int row, col;
  getmaxyx(stdscr, row, col);

  const char *label = "Enter your name:";
  mvprintw(row / 2 - 2, (col - (int)strlen(label)) / 2, "%s", label);

  int box_width = 30;
  int box_y = row / 2;
  int box_x = (col - box_width) / 2;

  WINDOW *input_win = newwin(3, box_width, box_y, box_x);
  box(input_win, 0, 0);
  wbkgd(input_win, COLOR_PAIR(2));
  wrefresh(input_win);

  char input[MAX_INPUT] = "";
  int pos = 0;
  int ch;

  wmove(input_win, 1, 1);
  wrefresh(input_win);

  while ((ch = wgetch(input_win)) != '\n' && ch != 'x') {
    if ((ch == KEY_BACKSPACE || ch == 127) && pos > 0) {
      pos--;
      input[pos] = '\0';
      mvwaddch(input_win, 1, pos + 1, ' ');
      wmove(input_win, 1, pos + 1);
    } else if (pos < MAX_INPUT - 1 && ch >= 32 && ch <= 126) {
      input[pos++] = (char)ch;
      input[pos] = '\0';
      mvwaddch(input_win, 1, pos, ch);
    }
    wrefresh(input_win);
  }

  clear();
  mvprintw(row / 2, (col - (int)strlen(input)) / 2, "Hello, %s!", input);
  mvprintw(row - 2, 0, "Press any key to exit...");
  refresh();
  getch();

  delwin(input_win);
  endwin();
}
