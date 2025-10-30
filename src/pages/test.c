#include "test.h"

#define MAX_INPUT 64
#define RGB_TO_NCURSES(r, g, b)                                                \
  ((r) * 1000 / 255), ((g) * 1000 / 255), ((b) * 1000 / 255)

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
