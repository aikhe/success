#include "introduction.h"

void draw_centered(WINDOW *win, const char *text) {
  int h, w;
  getmaxyx(win, h, w);
  werase(win);

  // int y = h / 2;
  // int x = (w - strlen(text)) / 2;

  // box(win, 0, 0);
  wrefresh(win);

  int box_w = strlen(text) + 4; // 2 spaces + 2 borders
  int box_h = 3;                // top + text + bottom
  int start_y = (h - box_h) / 2;
  int start_x = (w - box_w) / 2;

  WINDOW *boxwin = newwin(box_h, box_w, start_y, start_x);
  // wborder_set(boxwin, WACS_D_VLINE, WACS_D_VLINE, WACS_D_HLINE, WACS_D_HLINE,
  //             WACS_D_ULCORNER, WACS_D_URCORNER, WACS_D_LLCORNER,
  //             WACS_D_LRCORNER);
  cchar_t vline, hline, ul, ur, ll, lr;
  setcchar(&vline, L"┃", 0, 0, NULL);
  setcchar(&hline, L"━", 0, 0, NULL);
  setcchar(&ul, L"┏", 0, 0, NULL);
  setcchar(&ur, L"┓", 0, 0, NULL);
  setcchar(&ll, L"┗", 0, 0, NULL);
  setcchar(&lr, L"┛", 0, 0, NULL);
  wborder_set(boxwin, &vline, &vline, &hline, &hline, &ul, &ur, &ll, &lr);
  mvwprintw(boxwin, 1, 2, "%s", text);
  wrefresh(boxwin);
}

void introduction_page(void) {
  initscr();            // initialize the screen
  cbreak();             // disable line buffering, get input immediately
  noecho();             // don't print typed characters
  keypad(stdscr, TRUE); // enable arrow keys and function keys
  curs_set(0);          // hide curso

  // int ch = getch();
  // switch (ch) {
  // case KEY_UP:
  //   printw("You pressed UP!\n");
  //   break;
  // case KEY_DOWN:
  //   printw("You pressed DOWN!\n");
  //   break;
  // default:
  //   printw("You pressed: %c\n", ch);
  // }
  // refresh(); // actually show it on the terminal
  //
  // delay(2000);

  const char *msg = "Resize the window or press 'q' to quit";

  draw_centered(stdscr, msg);

  int ch;
  while ((ch = getch()) != 'q') {
    if (ch == KEY_RESIZE) {
      // Important step — update PDCurses internal size
      resize_term(0, 0);

      // Optional: also call clear() to avoid leftover text
      clear();

      draw_centered(stdscr, msg);
    }
  }

  clear();
  refresh();

  if (has_colors()) {
    start_color();                        // enable colors
    init_pair(1, COLOR_RED, COLOR_BLACK); // foreground red, background black
    attron(COLOR_PAIR(1));                // enable color pair
    draw_centered(stdscr, "this is a red text");
    attroff(COLOR_PAIR(1)); // disable color pair
    refresh();
  }

  delay(5000);
  endwin();
}
