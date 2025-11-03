// NOTE: Sign up and Login is heavily vibe coded since I dont really intend to
// add that feature but it's on our proposal so it had to be done, I'm really
// proud of the introduction page tho i did it myself :>

#include "introduction.h"

#include "curses.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winuser.h>

#include "menu.h"
#include "../features/ai_chat.h"
#include "../features/social_hall.h"

#ifdef _WIN32
#include <direct.h>
#define mkdir(dir, mode) _mkdir(dir)
#else
#include <sys/stat.h>
#endif

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
  wbkgd(ibox_bars, COLOR_PAIR(2));
  werase(ibox);

  // Draw vertical bars on left and right edges
  cchar_t vbar;
  setcchar(&vbar, L"┃", 0, 0, NULL);
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

  mvaddstr(y + bar_h, x, "enter");
  wattron(stdscr, COLOR_PAIR(2));
  mvaddstr(y + bar_h, x + 6, "send");
  wattroff(stdscr, COLOR_PAIR(2));

  const char *right_hint = "Success Platform";
  int right_x = x + bar_w - (int)strlen(right_hint);
  mvaddstr(y + bar_h, right_x, "Success");
  wattron(stdscr, COLOR_PAIR(2));
  mvaddstr(y + bar_h, right_x + 8, "Platform");
  wattroff(stdscr, COLOR_PAIR(2));

  refresh();

  return ibox;
}

static WINDOW *draw_labeled_input_bar(const char *label, int y, int *content_y,
                                      int *content_x, int *content_w) {
  int w = getmaxx(stdscr);

  int bar_h = 3;
  int bar_w = 50;
  int bar_wb = 52;

  int x = (w - bar_w) / 2;
  int xb = (w - bar_wb) / 2;

  WINDOW *ibox = newwin(bar_h, bar_w, y, x);
  WINDOW *ibox_bars = newwin(bar_h, bar_wb, y, xb);
  wbkgd(ibox, COLOR_PAIR(1));
  wbkgd(ibox_bars, COLOR_PAIR(2));
  werase(ibox);

  // Draw vertical bars on left and right edges
  cchar_t vbar;
  setcchar(&vbar, L"┃", 0, 0, NULL);
  for (int i = 0; i < bar_h; i++) {
    mvwadd_wch(ibox_bars, i, 0, &vbar);
    mvwadd_wch(ibox_bars, i, bar_wb - 1, &vbar);
  }

  // Label and prompt
  wattron(ibox, COLOR_PAIR(6) | A_DIM);
  char prompt[256];
  snprintf(prompt, sizeof(prompt), "%s: ", label);
  mvwaddstr(ibox, 1, 1, prompt);
  wattroff(ibox, COLOR_PAIR(6) | A_DIM);

  // Content area geometry for caller
  // Add one space after the colon for cursor positioning
  int label_len = (int)strlen(prompt) + 1;
  int inner_x = label_len;
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
  wattron(ibox, COLOR_PAIR(6) | A_DIM);
  mvwhline(ibox, rel_y, rel_x, ' ', content_w);
  wattroff(ibox, COLOR_PAIR(6) | A_DIM);

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

  wattron(stdscr, COLOR_PAIR(9));
  mvhline(h - 1, 0, ' ', w);
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

// Helper function to draw guide text centered at bottom of content area
static void draw_content_guide(int content_bottom_y, const char *guide_text) {
  int w = getmaxx(stdscr);
  int guide_x = (w - (int)strlen(guide_text)) / 2;
  wattron(stdscr, COLOR_PAIR(2));
  mvaddstr(content_bottom_y, guide_x, guide_text);
  wattroff(stdscr, COLOR_PAIR(2));
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
    short BLACK = 20;
    short BLUE = 21;
    short GRAY_3 = 22;
    short GRAY_4 = 23;

    init_color(DARK_GRAY, RGB_TO_NCURSES(30, 30, 30));
    init_color(GRAY_2, RGB_TO_NCURSES(128, 128, 128));
    init_color(FOREGROUND, RGB_TO_NCURSES(238, 238, 238));
    init_color(ORANGE, RGB_TO_NCURSES(243, 173, 128));
    init_color(BLACK, RGB_TO_NCURSES(10, 10, 10));
    init_color(BLUE, RGB_TO_NCURSES(92, 156, 245));
    init_color(GRAY_3, RGB_TO_NCURSES(53, 53, 53));
    init_color(GRAY_4, RGB_TO_NCURSES(16, 16, 16));

    init_pair(1, COLOR_WHITE, DARK_GRAY);
    init_pair(2, GRAY_2, BLACK);
    init_pair(3, FOREGROUND, BLACK);
    init_pair(4, ORANGE, BLACK);
    init_pair(5, COLOR_WHITE, BLACK);
    init_pair(6, ORANGE, DARK_GRAY);
    init_pair(7, BLACK, BLUE);
    init_pair(8, COLOR_WHITE, GRAY_3);
    init_pair(9, COLOR_WHITE, GRAY_4);
    // Extra pairs for dialogs on dark gray background
    init_pair(10, ORANGE, DARK_GRAY);
  }

  wbkgd(stdscr, COLOR_PAIR(5));

  // Keep stdscr from moving the hardware cursor; we'll position it via input
  // bar
  leaveok(stdscr, TRUE);

  const char *ascii_art = R"(
  █▀▀▀ █  █ ▄▀▀▀ ▄▀▀▀ █▀▀█ █▀▀▀ █▀▀▀ █
  ▀▀▀█ █░░█ █░░░ █░░░ █▀▀▀ ▀▀▀█ ▀▀▀█ ▀
  ▀▀▀▀  ▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀
                               v0.1.10
  )";

  const char *ucc = R"(
       █  █ ▄▀▀▀ ▄▀▀▀
       █░░█ █░░░ █░░░
        ▀▀▀ ▀▀▀▀ ▀▀▀▀
  )";

  // const char *input_bar = R"(
  // ┃                                                ┃
  // ┃ >                                              ┃
  // ┃                                                ┃
  // )";

  const char *intro = R"(
  SUCCESS is a learning platform for UCCians designed to make
  studying  engaging  and  effective.  It   offers tools like 
  quizzes,   flashcards,   streaks,   and   leaderboards plus
  personalized  study methods like Pomodoro and active recall
  to    help   students  learn   smarter   and   achieve more.
  )";

  const char *success = R"(
  SUCCESS
  )";

  const char *auth = R"(
  [l] login          To start using Success
  [s] signup                Make an account
  [x] exit                    Stop and quit
  )";

  const char *auth_coms = R"(
  [l] login 
  [s] signup
  [x] exit  
  )";

  const char *auth_guide = R"(
                     To start using Success
                            Make an account
                              Stop and quit
  )";

  const char *exit_dim = R"(


                              Stop and quit
  )";

  // Calculate content heights
  int ascii_h = count_lines(ascii_art);
  int intro_h = count_lines(intro);
  int auth_h = count_lines(auth);
  int input_bar_h = 3; // from draw_input_bar
  int status_bar_h = 1;

  // Calculate spacing between sections
  int spacing = 1;
  int total_content_h =
      ascii_h + spacing + intro_h + spacing + auth_h + spacing + input_bar_h;

  // Get screen height and calculate centered starting position
  // Leave space for status bar at bottom
  int screen_h = getmaxy(stdscr);
  int start_y = (screen_h - total_content_h - status_bar_h) / 2;
  if (start_y < 0)
    start_y = 0;

  // Position each section relative to the centered start
  int y_ascii = start_y - 1;
  int y_intro = y_ascii + ascii_h + spacing;
  int y_auth = y_intro + intro_h + spacing + 1;
  int y_input = y_auth + auth_h + spacing + 1;

  WINDOW *boxwin = draw_centered_win(stdscr, ascii_art, y_ascii, 0, 0);
  draw_sub_win(boxwin, ascii_art, 0, 0, 2);
  draw_sub_win(boxwin, ucc, 0, 0, 3);
  WINDOW *intro_win = draw_centered_win(stdscr, intro, y_intro, 0, 0);
  draw_sub_win(intro_win, success, 0, 0, 2);
  WINDOW *authwin = draw_centered_win(stdscr, auth, y_auth, 0, 1);
  draw_sub_win(authwin, auth_coms, 0, 0, 4);
  draw_sub_win(authwin, auth_guide, 0, 0, 2);
  draw_sub_win(authwin, exit_dim, 0, 0, 2);

  // Draw middle input bar and bottom status bar
  int input_y = y_input, input_x = 0, input_w = 0;
  WINDOW *input_bar = draw_input_bar(&input_y, &input_x, &input_w);
  draw_status_bar(" Success v0.1.10 ", " Made with love<3 ");

  char input_buf[512] = {0};
  int cursor_pos = 0;
  render_input(input_bar, input_y, input_x, input_w, input_buf, cursor_pos);

  int ch;
  int should_exit = 0;
  while (!should_exit && (ch = getch()) != ERR) {
    if (ch == KEY_RESIZE) {
      resize_term(0, 0);
      clear();

      // Recalculate content heights and positions
      int ascii_h = count_lines(ascii_art);
      int intro_h = count_lines(intro);
      int auth_h = count_lines(auth);
      int input_bar_h = 3;
      int status_bar_h = 1;

      int spacing = 1;
      int total_content_h = ascii_h + spacing + intro_h + spacing + auth_h +
                            spacing + input_bar_h;

      int screen_h = getmaxy(stdscr);
      int start_y = (screen_h - total_content_h - status_bar_h) / 2;
      if (start_y < 0)
        start_y = 0;

      int y_ascii = start_y - 1;
      int y_intro = y_ascii + ascii_h + spacing;
      int y_auth = y_intro + intro_h + spacing + 1;
      int y_input = y_auth + auth_h + spacing + 1;

      WINDOW *boxwin = draw_centered_win(stdscr, ascii_art, y_ascii, 0, 0);
      draw_sub_win(boxwin, ascii_art, 0, 0, 2);
      draw_sub_win(boxwin, ucc, 0, 0, 3);
      WINDOW *intro_win = draw_centered_win(stdscr, intro, y_intro, 0, 0);
      draw_sub_win(intro_win, success, 0, 0, 2);
      WINDOW *authwin = draw_centered_win(stdscr, auth, y_auth, 0, 1);
      draw_sub_win(authwin, auth_coms, 0, 0, 4);
      draw_sub_win(authwin, auth_guide, 0, 0, 2);
      draw_sub_win(authwin, exit_dim, 0, 0, 2);

      // Update outer scope variables (don't shadow them)
      input_y = y_input;
      input_x = 0;
      input_w = 0;
      input_bar = draw_input_bar(&input_y, &input_x, &input_w);
      draw_status_bar(" Success v0.1.10 ", " Made with love<3 ");
      render_input(input_bar, input_y, input_x, input_w, input_buf, cursor_pos);
    } else if (ch == 10 || ch == KEY_ENTER) {
      // Process commands when Enter is pressed
      if (cursor_pos > 0 && strlen(input_buf) > 0) {
        // Trim whitespace from input
        char *trimmed = input_buf;
        while (*trimmed == ' ' || *trimmed == '\t')
          trimmed++;

        if (strlen(trimmed) > 0) {
          char cmd = trimmed[0];
          if (cmd == 'l' || cmd == 's' || cmd == 'x') {
            // Handle commands: 'l' = login, 's' = signup, 'x' = exit
            if (cmd == 'x') {
              should_exit = 1; // Exit
              break;
            } else if (cmd == 'l') {
              // Login - call login page
              clear();
              refresh();
              endwin();
              login_page();
              // After login page exits, return to introduction page
              introduction_page();
              should_exit = 1; // Exit after returning from login
              break;
            } else if (cmd == 's') {
              // Signup - call signup page
              clear();
              refresh();
              endwin();
              signup_page();
              // After signup page exits, return to introduction page
              introduction_page();
              should_exit = 1; // Exit after returning from signup
              break;
            }
          }
        }

        // Clear input buffer after processing
        cursor_pos = 0;
        input_buf[0] = '\0';
        render_input(input_bar, input_y, input_x, input_w, input_buf,
                     cursor_pos);
      }
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

static void draw_user_type_selection(WINDOW *win, int selected_option) {
  werase(win);
  wattron(win, COLOR_PAIR(3));
  mvwaddstr(win, 1, 5, "Account Type:");
  wattroff(win, COLOR_PAIR(3));

  // Student option
  if (selected_option == 0) {
    wattron(win, COLOR_PAIR(4));
    mvwaddstr(win, 1, 25, "[Student]");
    wattroff(win, COLOR_PAIR(4));
    wattron(win, COLOR_PAIR(2));
    mvwaddstr(win, 1, 38, "Teacher");
    wattroff(win, COLOR_PAIR(2));
  } else {
    wattron(win, COLOR_PAIR(2));
    mvwaddstr(win, 1, 26, "Student");
    wattroff(win, COLOR_PAIR(2));
    wattron(win, COLOR_PAIR(4));
    mvwaddstr(win, 1, 37, "[Teacher]");
    wattroff(win, COLOR_PAIR(4));
  }
  wrefresh(win);
}

// Function to save user data to SQLite database
static int save_user_to_db(const char *username, const char *password,
                           const char *userinfo) {
  sqlite3 *db;
  int rc;
  char *err_msg = 0;

  // Ensure db directory exists (ignore error if it already exists)
  mkdir("db", 0755);

  // Open database connection (creates file if it doesn't exist)
  rc = sqlite3_open("db/users.db", &db);
  if (rc != SQLITE_OK) {
    // Database file doesn't exist or can't be created
    return rc;
  }

  // Create users table if it doesn't exist
  const char *create_table_sql = "CREATE TABLE IF NOT EXISTS users ("
                                 "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                 "username TEXT NOT NULL UNIQUE,"
                                 "password TEXT NOT NULL,"
                                 "userinfo TEXT NOT NULL,"
                                 "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
                                 ");";

  rc = sqlite3_exec(db, create_table_sql, 0, 0, &err_msg);
  if (rc != SQLITE_OK) {
    sqlite3_free(err_msg);
    sqlite3_close(db);
    return rc;
  }

  // Insert user data using parameterized query
  sqlite3_stmt *stmt;
  const char *insert_sql =
      "INSERT INTO users (username, password, userinfo) VALUES (?, ?, ?);";

  rc = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return rc;
  }

  // Bind parameters (SQLITE_TRANSIENT makes SQLite copy the strings)
  sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, password, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, userinfo, -1, SQLITE_TRANSIENT);

  // Execute the statement
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return rc;
  }

  // Finalize the statement
  sqlite3_finalize(stmt);

  // Close database connection
  sqlite3_close(db);
  return SQLITE_OK;
}

// Function to check user credentials and return userinfo
// Returns 1 if user found and credentials match, 0 if not found or wrong
// password If found, userinfo is copied into the provided buffer (must be at
// least 16 bytes)
static int check_user_credentials(const char *username, const char *password,
                                  char *userinfo) {
  sqlite3 *db;
  int rc;
  sqlite3_stmt *stmt;

  // Open database connection
  rc = sqlite3_open("db/users.db", &db);
  if (rc != SQLITE_OK) {
    return 0; // Database doesn't exist or can't be opened
  }

  // Query for user with matching username and password
  const char *query_sql =
      "SELECT userinfo FROM users WHERE username = ? AND password = ?;";

  rc = sqlite3_prepare_v2(db, query_sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return 0;
  }

  // Bind parameters
  sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, password, -1, SQLITE_TRANSIENT);

  // Execute the query
  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    // User found - get userinfo
    const unsigned char *userinfo_val = sqlite3_column_text(stmt, 0);
    if (userinfo_val && userinfo) {
      strncpy(userinfo, (const char *)userinfo_val, 15);
      userinfo[15] = '\0';
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 1; // User found
  }

  // User not found or wrong password
  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return 0;
}

void signup_page(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  start_color();
  // Use same color scheme as introduction page
  if (can_change_color() && COLORS > 16) {
    short DARK_GRAY = 16;
    short GRAY_2 = 17;
    short FOREGROUND = 18;
    short ORANGE = 19;
    short BLACK = 20;
    short BLUE = 21;
    short GRAY_3 = 22;
    short GRAY_4 = 23;

    init_color(DARK_GRAY, RGB_TO_NCURSES(30, 30, 30));
    init_color(GRAY_2, RGB_TO_NCURSES(128, 128, 128));
    init_color(FOREGROUND, RGB_TO_NCURSES(238, 238, 238));
    init_color(ORANGE, RGB_TO_NCURSES(243, 173, 128));
    init_color(BLACK, RGB_TO_NCURSES(10, 10, 10));
    init_color(BLUE, RGB_TO_NCURSES(92, 156, 245));
    init_color(GRAY_3, RGB_TO_NCURSES(53, 53, 53));
    init_color(GRAY_4, RGB_TO_NCURSES(16, 16, 16));

    init_pair(1, COLOR_WHITE, DARK_GRAY);
    init_pair(2, GRAY_2, BLACK);
    init_pair(3, FOREGROUND, BLACK);
    init_pair(4, ORANGE, BLACK);
    init_pair(5, COLOR_WHITE, BLACK);
    init_pair(6, ORANGE, DARK_GRAY);
    init_pair(7, BLACK, BLUE);
    init_pair(8, COLOR_WHITE, GRAY_3);
    init_pair(9, COLOR_WHITE, GRAY_4);
  }

  wbkgd(stdscr, COLOR_PAIR(5));
  leaveok(stdscr, TRUE);

  const char *signup_title = R"(
  Sign Up to SUCCESS
  )";

  // Storage for user data (all as strings for SQLite)
  char username[256] = {0};
  char password[256] = {0};
  char userinfo[16] = {0}; // "teacher" or "student"

  // Calculate content heights
  int title_h = count_lines(signup_title);
  int input_spacing = 1;
  int input_bar_h = 3;
  int selection_h = 3;
  int title_to_input_spacing = 2;

  // Calculate vertical positions
  int screen_h = getmaxy(stdscr);
  int y_title = (screen_h - (title_h + title_to_input_spacing + input_bar_h +
                             input_spacing + input_bar_h + 1 + selection_h)) /
                2;
  if (y_title < 0)
    y_title = 0;

  int y_username = y_title + title_h + title_to_input_spacing - 2;
  int y_password = y_username + input_bar_h + input_spacing;
  int y_selection = y_password + input_bar_h + 1;

  // Draw title
  WINDOW *title_win =
      draw_centered_win(stdscr, signup_title, y_title - 2, 0, 0);
  draw_sub_win(title_win, signup_title, 0, 0, 3);

  // Draw username input
  int username_y = 0, username_x = 0, username_w = 0;
  WINDOW *username_bar = draw_labeled_input_bar(
      "Username", y_username, &username_y, &username_x, &username_w);
  char username_buf[256] = {0};
  int username_cursor = 0;

  // Draw password input
  int password_y = 0, password_x = 0, password_w = 0;
  WINDOW *password_bar = draw_labeled_input_bar(
      "Password", y_password, &password_y, &password_x, &password_w);
  char password_buf[256] = {0};
  int password_cursor = 0;

  // Draw selection area for teacher/student (create before using)
  int w = getmaxx(stdscr);
  int sel_w = 50;
  int sel_x = (w - sel_w) / 2;
  WINDOW *selection_win = newwin(selection_h, sel_w, y_selection, sel_x);
  wbkgd(selection_win, COLOR_PAIR(5));

  int selected_option = 0; // 0 = student, 1 = teacher

  // Set initial focus on username field
  curs_set(1);
  leaveok(username_bar, FALSE);
  leaveok(password_bar, TRUE);
  leaveok(selection_win, TRUE);
  render_input(username_bar, username_y, username_x, username_w, username_buf,
               username_cursor);

  // Draw guide at bottom of content
  int content_bottom = y_selection + selection_h + 4;
  draw_content_guide(content_bottom,
                     " [ESC] Back   [TAB] Next   [ENTER] Submit ");

  // Draw status bar
  draw_status_bar(" Success v0.1.10 ", " Made with love<3 ");

  // Draw initial selection
  draw_user_type_selection(selection_win, selected_option);

  // Input state: 0 = username, 1 = password, 2 = selection
  int input_state = 0;
  int should_exit = 0;

  int ch;
  while (!should_exit && (ch = getch()) != ERR) {
    if (ch == KEY_RESIZE) {
      resize_term(0, 0);
      clear();

      // Recalculate screen dimensions
      screen_h = getmaxy(stdscr);
      w = getmaxx(stdscr);

      // Recalculate positions
      int title_to_input_spacing = 2;
      y_title = (screen_h - (title_h + title_to_input_spacing + input_bar_h +
                             input_spacing + input_bar_h + 1 + selection_h)) /
                2;
      if (y_title < 0)
        y_title = 0;
      y_username = y_title + title_h + title_to_input_spacing - 2;
      y_password = y_username + input_bar_h + input_spacing;
      y_selection = y_password + input_bar_h + 1;

      // Redraw everything
      title_win = draw_centered_win(stdscr, signup_title, y_title - 2, 0, 0);
      draw_sub_win(title_win, signup_title, 0, 0, 3);

      username_bar = draw_labeled_input_bar("Username", y_username, &username_y,
                                            &username_x, &username_w);
      render_input(username_bar, username_y, username_x, username_w,
                   username_buf, username_cursor);

      password_bar = draw_labeled_input_bar("Password", y_password, &password_y,
                                            &password_x, &password_w);
      render_input(password_bar, password_y, password_x, password_w,
                   password_buf, password_cursor);

      sel_x = (w - sel_w) / 2;
      delwin(selection_win);
      selection_win = newwin(selection_h, sel_w, y_selection, sel_x);
      wbkgd(selection_win, COLOR_PAIR(5));
      draw_user_type_selection(selection_win, selected_option);

      int content_bottom = y_selection + selection_h + 4;
      draw_content_guide(content_bottom,
                         " [ESC] Back   [TAB] Next   [ENTER] Submit ");

      // Draw status bar
      draw_status_bar(" Success v0.1.10 ", " Made with love<3 ");
    } else if (ch == 27) { // ESC
      should_exit = 1;
      break;
    } else if (ch == '\t' || ch == KEY_DOWN) { // TAB or DOWN
      input_state = (input_state + 1) % 3;

      // Update cursor visibility
      if (input_state == 0) {
        curs_set(1);
        leaveok(username_bar, FALSE);
        leaveok(password_bar, TRUE);
        leaveok(selection_win, TRUE);
        render_input(username_bar, username_y, username_x, username_w,
                     username_buf, username_cursor);
      } else if (input_state == 1) {
        curs_set(1);
        leaveok(username_bar, TRUE);
        leaveok(password_bar, FALSE);
        leaveok(selection_win, TRUE);
        render_input(password_bar, password_y, password_x, password_w,
                     password_buf, password_cursor);
      } else {
        curs_set(0);
        leaveok(username_bar, TRUE);
        leaveok(password_bar, TRUE);
        leaveok(selection_win, FALSE);
        draw_user_type_selection(selection_win, selected_option);
      }
    } else if (ch == KEY_UP) {
      input_state = (input_state + 2) % 3; // Go back one (add 2 then mod 3)

      if (input_state == 0) {
        curs_set(1);
        leaveok(username_bar, FALSE);
        leaveok(password_bar, TRUE);
        leaveok(selection_win, TRUE);
        render_input(username_bar, username_y, username_x, username_w,
                     username_buf, username_cursor);
      } else if (input_state == 1) {
        curs_set(1);
        leaveok(username_bar, TRUE);
        leaveok(password_bar, FALSE);
        leaveok(selection_win, TRUE);
        render_input(password_bar, password_y, password_x, password_w,
                     password_buf, password_cursor);
      } else {
        curs_set(0);
        leaveok(username_bar, TRUE);
        leaveok(password_bar, TRUE);
        leaveok(selection_win, FALSE);
        draw_user_type_selection(selection_win, selected_option);
      }
    } else if (ch == 10 || ch == KEY_ENTER) {
      if (input_state == 2) {
        // Show confirmation dialog
        int confirm = 1; // default to YES (0 = no, 1 = yes)
        int confirm_should_exit = 0;

        // Create confirmation dialog
        int confirm_h = 7;
        const char *confirm_title = "Are you sure?";
        const char *no_label = "No";
        const char *yes_label = "Yes";
        int max_text_len = (int)strlen(confirm_title);
        int no_yes_len = (int)strlen(no_label) + 6 + (int)strlen(yes_label);
        if (no_yes_len > max_text_len)
          max_text_len = no_yes_len;
        int confirm_w = max_text_len + 22; // 4 padding on each side
        int confirm_y = screen_h / 2 - confirm_h / 2;
        int confirm_x = (w - confirm_w) / 2;
        // WINDOW *confirm_win_border =
        //     newwin(confirm_h + 2, confirm_w + 2, confirm_y, confirm_x);
        WINDOW *confirm_win =
            newwin(confirm_h, confirm_w, confirm_y, confirm_x + 1);
        wbkgd(confirm_win, COLOR_PAIR(1));

        while (!confirm_should_exit) {
          werase(confirm_win);
          // Draw heavy border
          cchar_t vline, hline, ul, ur, ll, lr;
          setcchar(&vline, L"┃", 0, 0, NULL);
          setcchar(&hline, L"━", 0, 0, NULL);
          setcchar(&ul, L"┏", 0, 0, NULL);
          setcchar(&ur, L"┓", 0, 0, NULL);
          setcchar(&ll, L"┗", 0, 0, NULL);
          setcchar(&lr, L"┛", 0, 0, NULL);
          wborder_set(confirm_win, &vline, &vline, &hline, &hline, &ul, &ur,
                      &ll, &lr);

          // Title using default foreground on dark background (4 padding from
          // left)
          // int title_x = strlen(confirm_title) - confirm_w;
          int title_x = 11;
          wattron(confirm_win, A_BOLD);
          mvwaddstr(confirm_win, 2, title_x, confirm_title);
          wattroff(confirm_win, A_BOLD);

          int opts_y = confirm_h - 3;
          const char *no_label_sel = "[No]";
          const char *yes_label_sel = "[Yes]";
          int spacing = 6;
          const char *left = (confirm == 0) ? no_label_sel : no_label;
          const char *right = (confirm == 1) ? yes_label_sel : yes_label;
          // int total_w = (int)strlen(left) + spacing + (int)strlen(right);
          int start_x = 10; // 4 padding from left

          if (confirm == 0) {
            // NO selected
            wattron(confirm_win, COLOR_PAIR(6));
            mvwaddstr(confirm_win, opts_y, start_x - 1, left);
            wattroff(confirm_win, COLOR_PAIR(6));
            wattron(confirm_win, COLOR_PAIR(1) | A_DIM);
            mvwaddstr(confirm_win, opts_y,
                      start_x + (int)strlen(left) + spacing + 1, right);
            wattroff(confirm_win, COLOR_PAIR(1) | A_DIM);
          } else {
            // YES selected
            wattron(confirm_win, COLOR_PAIR(1) | A_DIM);
            mvwaddstr(confirm_win, opts_y, start_x, left);
            wattroff(confirm_win, COLOR_PAIR(1) | A_DIM);
            wattron(confirm_win, COLOR_PAIR(6));
            mvwaddstr(confirm_win, opts_y,
                      start_x + (int)strlen(left) + spacing + 2, right);
            wattroff(confirm_win, COLOR_PAIR(6));
          }

          wrefresh(confirm_win);

          int confirm_ch = getch();
          if (confirm_ch == KEY_LEFT || confirm_ch == KEY_RIGHT) {
            confirm = 1 - confirm;
          } else if (confirm_ch == 10 || confirm_ch == KEY_ENTER) {
            if (confirm == 1) {
              // Yes - store data and proceed
              strncpy(username, username_buf, sizeof(username) - 1);
              username[sizeof(username) - 1] = '\0';

              strncpy(password, password_buf, sizeof(password) - 1);
              password[sizeof(password) - 1] = '\0';

              strncpy(userinfo, selected_option == 0 ? "student" : "teacher",
                      sizeof(userinfo) - 1);
              userinfo[sizeof(userinfo) - 1] = '\0';

              // Store in SQLite database
              int db_result = save_user_to_db(username, password, userinfo);
              if (db_result != SQLITE_OK) {
                // Database save failed - could show error message here
                // For now, continue anyway
              }

              // Save username to session file
              mkdir("db", 0755);
              FILE *session_file = fopen("db/.session", "w");
              if (session_file) {
                fprintf(session_file, "%s\n", username);
                fclose(session_file);
              }

              delwin(confirm_win);
              clear();
              refresh();
              endwin();

              // Redirect to appropriate page based on account type
              if (strcmp(userinfo, "student") == 0) {
                menu();
              } else if (strcmp(userinfo, "teacher") == 0) {
                teacher_page();
              } else {
                // Fallback to success menu if userinfo is unexpected
                success_menu_page();
              }
              should_exit = 1;
              break;
            } else {
              // No - clear all data and restart
              memset(username_buf, 0, sizeof(username_buf));
              memset(password_buf, 0, sizeof(password_buf));
              username_cursor = 0;
              password_cursor = 0;
              selected_option = 0;
              input_state = 0;

              delwin(confirm_win);

              // Redraw everything
              clear();
              title_win =
                  draw_centered_win(stdscr, signup_title, y_title - 2, 0, 0);
              draw_sub_win(title_win, signup_title, 0, 0, 3);

              delwin(username_bar);
              delwin(password_bar);
              username_bar =
                  draw_labeled_input_bar("Username", y_username, &username_y,
                                         &username_x, &username_w);
              password_bar =
                  draw_labeled_input_bar("Password", y_password, &password_y,
                                         &password_x, &password_w);

              curs_set(1);
              leaveok(username_bar, FALSE);
              leaveok(password_bar, TRUE);
              leaveok(selection_win, TRUE);
              render_input(username_bar, username_y, username_x, username_w,
                           username_buf, username_cursor);

              draw_user_type_selection(selection_win, selected_option);
              int content_bottom = y_selection + selection_h + 4;
              draw_content_guide(content_bottom,
                                 " [ESC] Back   [TAB] Next   [ENTER] Submit ");

              // Draw status bar
              draw_status_bar(" Success v0.1.10 ", " Made with love<3 ");

              confirm_should_exit = 1;
            }
          } else if (confirm_ch == 27) {
            // ESC - cancel confirmation, stay on selection
            delwin(confirm_win);
            confirm_should_exit = 1;
          }
        }
      } else if (input_state == 0) {
        // Username field - move to password
        input_state = 1;
        curs_set(1);
        leaveok(username_bar, TRUE);
        leaveok(password_bar, FALSE);
        leaveok(selection_win, TRUE);
        render_input(password_bar, password_y, password_x, password_w,
                     password_buf, password_cursor);
      } else if (input_state == 1) {
        // Password field - move to selection
        input_state = 2;
        curs_set(0);
        leaveok(username_bar, TRUE);
        leaveok(password_bar, TRUE);
        leaveok(selection_win, FALSE);
        draw_user_type_selection(selection_win, selected_option);
      }
    } else if (input_state == 2) {
      // Selection mode - handle left/right arrows
      if (ch == KEY_LEFT || ch == KEY_RIGHT) {
        selected_option = 1 - selected_option;
        draw_user_type_selection(selection_win, selected_option);
      }
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
      // Handle backspace
      if (input_state == 0) {
        if (username_cursor > 0) {
          username_cursor--;
          username_buf[username_cursor] = '\0';
          render_input(username_bar, username_y, username_x, username_w,
                       username_buf, username_cursor);
        }
      } else if (input_state == 1) {
        if (password_cursor > 0) {
          password_cursor--;
          password_buf[password_cursor] = '\0';
          render_input(password_bar, password_y, password_x, password_w,
                       password_buf, password_cursor);
        }
      }
    } else if (ch >= 32 && ch <= 126) {
      // Handle printable characters
      if (input_state == 0) {
        if (username_cursor < (int)sizeof(username_buf) - 1) {
          username_buf[username_cursor++] = (char)ch;
          username_buf[username_cursor] = '\0';
          render_input(username_bar, username_y, username_x, username_w,
                       username_buf, username_cursor);
        }
      } else if (input_state == 1) {
        if (password_cursor < (int)sizeof(password_buf) - 1) {
          password_buf[password_cursor++] = (char)ch;
          password_buf[password_cursor] = '\0';
          // Show asterisks for password
          char display_buf[256];
          memset(display_buf, '*', password_cursor);
          display_buf[password_cursor] = '\0';
          render_input(password_bar, password_y, password_x, password_w,
                       display_buf, password_cursor);
        }
      }
    }
  }

  // Clean up windows
  delwin(selection_win);

  clear();
  refresh();
  endwin();
}

void success_menu_page(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  start_color();
  // Use same color scheme as introduction page
  if (can_change_color() && COLORS > 16) {
    short DARK_GRAY = 16;
    short GRAY_2 = 17;
    short FOREGROUND = 18;
    short ORANGE = 19;
    short BLACK = 20;
    short BLUE = 21;
    short GRAY_3 = 22;
    short GRAY_4 = 23;

    init_color(DARK_GRAY, RGB_TO_NCURSES(30, 30, 30));
    init_color(GRAY_2, RGB_TO_NCURSES(128, 128, 128));
    init_color(FOREGROUND, RGB_TO_NCURSES(238, 238, 238));
    init_color(ORANGE, RGB_TO_NCURSES(243, 173, 128));
    init_color(BLACK, RGB_TO_NCURSES(10, 10, 10));
    init_color(BLUE, RGB_TO_NCURSES(92, 156, 245));
    init_color(GRAY_3, RGB_TO_NCURSES(53, 53, 53));
    init_color(GRAY_4, RGB_TO_NCURSES(16, 16, 16));

    init_pair(1, COLOR_WHITE, DARK_GRAY);
    init_pair(2, GRAY_2, BLACK);
    init_pair(3, FOREGROUND, BLACK);
    init_pair(4, ORANGE, BLACK);
    init_pair(5, COLOR_WHITE, BLACK);
    init_pair(6, ORANGE, DARK_GRAY);
    init_pair(7, BLACK, BLUE);
    init_pair(8, COLOR_WHITE, GRAY_3);
    init_pair(9, COLOR_WHITE, GRAY_4);
  }

  wbkgd(stdscr, COLOR_PAIR(5));
  leaveok(stdscr, TRUE);

  const char *welcome_text = R"(
  Welcome to SUCCESS!
  
  Account created successfully.
  )";

  // Calculate content heights
  int welcome_h = count_lines(welcome_text);
  int screen_h = getmaxy(stdscr);
  int y_welcome = (screen_h - welcome_h) / 2;
  if (y_welcome < 0)
    y_welcome = 0;

  // Draw welcome message
  WINDOW *welcome_win =
      draw_centered_win(stdscr, welcome_text, y_welcome, 0, 0);
  draw_sub_win(welcome_win, welcome_text, 0, 0, 4);

  // Draw status bar
  draw_status_bar(" Success Menu ", " [ESC] Exit ");

  // Wait for ESC to exit
  int ch;
  while ((ch = getch()) != 27 && ch != KEY_RESIZE) {
    if (ch == KEY_RESIZE) {
      resize_term(0, 0);
      clear();

      screen_h = getmaxy(stdscr);
      y_welcome = (screen_h - welcome_h) / 2;
      if (y_welcome < 0)
        y_welcome = 0;

      welcome_win = draw_centered_win(stdscr, welcome_text, y_welcome, 0, 0);
      draw_sub_win(welcome_win, welcome_text, 0, 0, 4);
      draw_status_bar(" Success Menu ", " [ESC] Exit ");
    }
  }

  clear();
  refresh();
  endwin();
}

// Helper function to show error popup and wait for Enter
static void show_error_popup(const char *message) {
  int h = getmaxy(stdscr);
  int w = getmaxx(stdscr);

  int popup_h = 7; // Increased height for bottom padding
  int message_len = (int)strlen(message);
  int hint_len = (int)strlen("Press Enter to continue");
  int max_text_len = message_len > hint_len ? message_len : hint_len;
  int popup_w = max_text_len + 16; // 4 padding on each side
  int popup_y = h / 2 - popup_h / 2;
  int popup_x = (w - popup_w) / 2;

  WINDOW *popup_win = newwin(popup_h, popup_w, popup_y, popup_x);
  wbkgd(popup_win, COLOR_PAIR(1));

  // Draw border
  cchar_t vline, hline, ul, ur, ll, lr;
  setcchar(&vline, L"┃", 0, 0, NULL);
  setcchar(&hline, L"━", 0, 0, NULL);
  setcchar(&ul, L"┏", 0, 0, NULL);
  setcchar(&ur, L"┓", 0, 0, NULL);
  setcchar(&ll, L"┗", 0, 0, NULL);
  setcchar(&lr, L"┛", 0, 0, NULL);
  wborder_set(popup_win, &vline, &vline, &hline, &hline, &ul, &ur, &ll, &lr);

  // Show message (4 padding from left)
  int msg_y = popup_h / 2 - 1;
  int msg_x = 4;
  wattron(popup_win, A_BOLD);
  mvwaddstr(popup_win, msg_y, msg_x + 8, message);
  wattroff(popup_win, A_BOLD);

  // Show "Press Enter" hint (4 padding from left, 1 padding from bottom)
  const char *hint = "Press Enter to continue";
  int hint_x = 4;
  wattron(popup_win, COLOR_PAIR(6));
  mvwaddstr(popup_win, popup_h - 3, hint_x + 4, hint);
  wattroff(popup_win, COLOR_PAIR(6));

  wrefresh(popup_win);

  // Wait for Enter
  int ch;
  while ((ch = getch()) != 10 && ch != KEY_ENTER) {
    if (ch == KEY_RESIZE) {
      resize_term(0, 0);
      clear();
      refresh();
      // Redraw popup
      h = getmaxy(stdscr);
      w = getmaxx(stdscr);
      int message_len = (int)strlen(message);
      int hint_len = (int)strlen("Press Enter to continue");
      int max_text_len = message_len > hint_len ? message_len : hint_len;
      popup_w = max_text_len + 8; // 4 padding on each side
      popup_y = h / 2 - popup_h / 2;
      popup_x = (w - popup_w) / 2;
      delwin(popup_win);
      popup_win = newwin(popup_h, popup_w, popup_y, popup_x);
      wbkgd(popup_win, COLOR_PAIR(1));
      wborder_set(popup_win, &vline, &vline, &hline, &hline, &ul, &ur, &ll,
                  &lr);
      wattron(popup_win, A_BOLD);
      mvwaddstr(popup_win, msg_y, msg_x, message);
      wattroff(popup_win, A_BOLD);
      wattron(popup_win, COLOR_PAIR(6));
      mvwaddstr(popup_win, popup_h - 2, hint_x, hint);
      wattroff(popup_win, COLOR_PAIR(6));
      wrefresh(popup_win);
    }
  }

  delwin(popup_win);
}

void login_page(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  start_color();
  // Use same color scheme as introduction page
  if (can_change_color() && COLORS > 16) {
    short DARK_GRAY = 16;
    short GRAY_2 = 17;
    short FOREGROUND = 18;
    short ORANGE = 19;
    short BLACK = 20;
    short BLUE = 21;
    short GRAY_3 = 22;
    short GRAY_4 = 23;

    init_color(DARK_GRAY, RGB_TO_NCURSES(30, 30, 30));
    init_color(GRAY_2, RGB_TO_NCURSES(128, 128, 128));
    init_color(FOREGROUND, RGB_TO_NCURSES(238, 238, 238));
    init_color(ORANGE, RGB_TO_NCURSES(243, 173, 128));
    init_color(BLACK, RGB_TO_NCURSES(10, 10, 10));
    init_color(BLUE, RGB_TO_NCURSES(92, 156, 245));
    init_color(GRAY_3, RGB_TO_NCURSES(53, 53, 53));
    init_color(GRAY_4, RGB_TO_NCURSES(16, 16, 16));

    init_pair(1, COLOR_WHITE, DARK_GRAY);
    init_pair(2, GRAY_2, BLACK);
    init_pair(3, FOREGROUND, BLACK);
    init_pair(4, ORANGE, BLACK);
    init_pair(5, COLOR_WHITE, BLACK);
    init_pair(6, ORANGE, DARK_GRAY);
    init_pair(7, BLACK, BLUE);
    init_pair(8, COLOR_WHITE, GRAY_3);
    init_pair(9, COLOR_WHITE, GRAY_4);
  }

  wbkgd(stdscr, COLOR_PAIR(5));
  leaveok(stdscr, TRUE);

  const char *login_title = R"(
  Login to SUCCESS
  )";

  // Calculate content heights
  int title_h = count_lines(login_title);
  int input_spacing = 1;
  int input_bar_h = 3;
  int title_to_input_spacing = 2;

  // Calculate vertical positions
  int screen_h = getmaxy(stdscr);
  int y_title = (screen_h - (title_h + title_to_input_spacing + input_bar_h +
                             input_spacing + input_bar_h)) /
                2;
  if (y_title < 0)
    y_title = 0;

  int y_username = y_title + title_h + title_to_input_spacing - 2;
  int y_password = y_username + input_bar_h + input_spacing;

  // Draw title
  WINDOW *title_win = draw_centered_win(stdscr, login_title, y_title - 2, 0, 0);
  draw_sub_win(title_win, login_title, 0, 0, 3);

  // Draw username input
  int username_y = 0, username_x = 0, username_w = 0;
  WINDOW *username_bar = draw_labeled_input_bar(
      "Username", y_username, &username_y, &username_x, &username_w);
  char username_buf[256] = {0};
  int username_cursor = 0;

  // Draw password input
  int password_y = 0, password_x = 0, password_w = 0;
  WINDOW *password_bar = draw_labeled_input_bar(
      "Password", y_password, &password_y, &password_x, &password_w);
  char password_buf[256] = {0};
  int password_cursor = 0;

  // Set initial focus on username field
  curs_set(1);
  leaveok(username_bar, FALSE);
  leaveok(password_bar, TRUE);
  render_input(username_bar, username_y, username_x, username_w, username_buf,
               username_cursor);

  // Draw guide at bottom of content
  int content_bottom = y_password + input_bar_h + 6;
  draw_content_guide(content_bottom,
                     " [ESC] Back   [TAB] Next   [ENTER] Submit ");

  // Draw status bar
  draw_status_bar(" Success v0.1.10 ", " Made with love<3 ");

  // Input state: 0 = username, 1 = password
  int input_state = 0;
  int should_exit = 0;

  int ch;
  while (!should_exit && (ch = getch()) != ERR) {
    if (ch == KEY_RESIZE) {
      resize_term(0, 0);
      clear();

      // Recalculate screen dimensions
      screen_h = getmaxy(stdscr);
      // int w = getmaxx(stdscr);

      // Recalculate positions
      int title_to_input_spacing = 2;
      y_title = (screen_h - (title_h + title_to_input_spacing + input_bar_h +
                             input_spacing + input_bar_h)) /
                2;
      if (y_title < 0)
        y_title = 0;
      y_username = y_title + title_h + title_to_input_spacing - 2;
      y_password = y_username + input_bar_h + input_spacing;

      // Redraw everything
      title_win = draw_centered_win(stdscr, login_title, y_title - 2, 0, 0);
      draw_sub_win(title_win, login_title, 0, 0, 3);

      username_bar = draw_labeled_input_bar("Username", y_username, &username_y,
                                            &username_x, &username_w);
      render_input(username_bar, username_y, username_x, username_w,
                   username_buf, username_cursor);

      password_bar = draw_labeled_input_bar("Password", y_password, &password_y,
                                            &password_x, &password_w);
      render_input(password_bar, password_y, password_x, password_w,
                   password_buf, password_cursor);

      int content_bottom = y_password + input_bar_h + 4;
      draw_content_guide(content_bottom,
                         " [ESC] Back   [TAB] Next   [ENTER] Submit ");

      // Draw status bar
      draw_status_bar(" Success v0.1.10 ", " Made with love<3 ");
    } else if (ch == 27) { // ESC
      should_exit = 1;
      break;
    } else if (ch == '\t' || ch == KEY_DOWN) { // TAB or DOWN
      input_state = (input_state + 1) % 2;

      // Update cursor visibility
      if (input_state == 0) {
        curs_set(1);
        leaveok(username_bar, FALSE);
        leaveok(password_bar, TRUE);
        render_input(username_bar, username_y, username_x, username_w,
                     username_buf, username_cursor);
      } else {
        curs_set(1);
        leaveok(username_bar, TRUE);
        leaveok(password_bar, FALSE);
        render_input(password_bar, password_y, password_x, password_w,
                     password_buf, password_cursor);
      }
    } else if (ch == KEY_UP) {
      input_state = (input_state + 1) % 2; // Go back one

      if (input_state == 0) {
        curs_set(1);
        leaveok(username_bar, FALSE);
        leaveok(password_bar, TRUE);
        render_input(username_bar, username_y, username_x, username_w,
                     username_buf, username_cursor);
      } else {
        curs_set(1);
        leaveok(username_bar, TRUE);
        leaveok(password_bar, FALSE);
        render_input(password_bar, password_y, password_x, password_w,
                     password_buf, password_cursor);
      }
    } else if (ch == 10 || ch == KEY_ENTER) {
      // Submit login
      if (strlen(username_buf) > 0 && strlen(password_buf) > 0) {
        char userinfo[16] = {0};
        int found =
            check_user_credentials(username_buf, password_buf, userinfo);

        if (!found) {
          // User not found - show popup
          show_error_popup("User not found");
          // Return to introduction page
          clear();
          refresh();
          endwin();
          introduction_page();
          should_exit = 1;
          break;
        } else {
          // Save username to session file
          mkdir("db", 0755);
          FILE *session_file = fopen("db/.session", "w");
          if (session_file) {
            fprintf(session_file, "%s\n", username_buf);
            fclose(session_file);
          }

          // User found - navigate to appropriate page
          clear();
          refresh();
          endwin();

          if (strcmp(userinfo, "student") == 0) {
            menu();
          } else if (strcmp(userinfo, "teacher") == 0) {
            teacher_page();
          } else {
            // Unexpected userinfo value - show error and return
            show_error_popup("Invalid user type");
            clear();
            refresh();
            endwin();
            introduction_page();
          }
          should_exit = 1;
          break;
        }
      } else if (input_state == 0) {
        // Username field - move to password
        input_state = 1;
        curs_set(1);
        leaveok(username_bar, TRUE);
        leaveok(password_bar, FALSE);
        render_input(password_bar, password_y, password_x, password_w,
                     password_buf, password_cursor);
      }
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
      // Handle backspace
      if (input_state == 0) {
        if (username_cursor > 0) {
          username_cursor--;
          username_buf[username_cursor] = '\0';
          render_input(username_bar, username_y, username_x, username_w,
                       username_buf, username_cursor);
        }
      } else {
        if (password_cursor > 0) {
          password_cursor--;
          password_buf[password_cursor] = '\0';
          char display_buf[256];
          memset(display_buf, '*', password_cursor);
          display_buf[password_cursor] = '\0';
          render_input(password_bar, password_y, password_x, password_w,
                       display_buf, password_cursor);
        }
      }
    } else if (ch >= 32 && ch <= 126) {
      // Handle printable characters
      if (input_state == 0) {
        if (username_cursor < (int)sizeof(username_buf) - 1) {
          username_buf[username_cursor++] = (char)ch;
          username_buf[username_cursor] = '\0';
          render_input(username_bar, username_y, username_x, username_w,
                       username_buf, username_cursor);
        }
      } else {
        if (password_cursor < (int)sizeof(password_buf) - 1) {
          password_buf[password_cursor++] = (char)ch;
          password_buf[password_cursor] = '\0';
          // Show asterisks for password
          char display_buf[256];
          memset(display_buf, '*', password_cursor);
          display_buf[password_cursor] = '\0';
          render_input(password_bar, password_y, password_x, password_w,
                       display_buf, password_cursor);
        }
      }
    }
  }

  clear();
  refresh();
  endwin();
}

// Forward declare functions from menu.c that we'll use
// Note: draw_status_bar conflicts - menu.c has 5 params, introduction.c has static version with 2 params
// Use a wrapper function to avoid conflict
static void draw_status_bar_menu_wrapper(int h, int w, char *left, char *mid, char *right) {
  extern void draw_status_bar_menu(int h, int w, char *left, char *mid, char *right);
  draw_status_bar_menu(h, w, left, mid, right);
}
extern void render_input_field(WINDOW *win, int w, int h, int y, int x, char *fieldname);
extern WINDOW *render_win(WINDOW *win, int w, int h, int y, int x, const char *content, int bg_color, int bar_color, bool has_bg);
extern int calculate_text_height(const char *text, int width);
extern WINDOW *render_text_with_colors(WINDOW *win, int w, int y, int x, const char *text, int bg_color, int bar_color, bool has_bg, void *colored_words, int num_colors, int sidebar_color);
extern void render_guide_line(int y, int x, const char *text);
extern void define_colors(void);
extern int ai_chat(void);
extern void social_hall(void);

void teacher_page(void) {
  // ColoredWord type matching menu.c
  typedef struct {
    const char *word;
    int color_pair;
  } ColoredWordMenu;

  while (1) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);

    define_colors();

    wbkgd(stdscr, COLOR_PAIR(5));

    clear();

    int h, w;
    getmaxyx(stdscr, h, w);

    const char *success_woodmark = R"(
  █▀▀▀ █  █ ▄▀▀▀ ▄▀▀▀ █▀▀█ █▀▀▀ █▀▀▀ █
  ▀▀▀█ █░░█ █░░░ █░░░ █▀▀▀ ▀▀▀█ ▀▀▀█ ▀
  ▀▀▀▀  ▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀
                               v0.1.10
  )";

    const char *desc = R"(
  Welcome Teacher! Access the SUCCESS AI Chatbot for instant answers and explanations, or the Social Hall to share learning materials with your students.
  )";

    const char *ai_chat_text = R"(
  SUCCESS AI Chatbot - Get instant answers, explanations, and study guidance powered by AI.
  )";

    const char *social_hall_text = R"(
  Social Hall - Upload and share learning materials and lessons with your students.
  )";
    
    draw_status_bar_menu_wrapper(h, w, " Success v0.1.10 ", " in Teacher Menu ", " Made with <3 ");
    int input_w = w - 10;
    int input_y = h - 7;
    int input_x = (w - input_w) / 2;
    // Use menu.c's render_input_field wrapper to avoid conflict with static render_input
    extern void render_input_field(WINDOW *win, int w, int h, int y, int x, char *fieldname);
    render_input_field(stdscr, input_w, 3, input_y, input_x, ">");

    WINDOW *success_woodmark_win = render_win(stdscr, 40, 10, 1, (w - 38) / 2, success_woodmark, 2, 13, false);
    wattron(success_woodmark_win, COLOR_PAIR(5));
    mvwaddstr(success_woodmark_win, 1, 7, "█  █ ▄▀▀▀ ▄▀▀▀");
    mvwaddstr(success_woodmark_win, 2, 7, "█░░█ █░░░ █░░░");
    mvwaddstr(success_woodmark_win, 3, 8, "▀▀▀ ▀▀▀▀ ▀▀▀▀");
    wattroff(success_woodmark_win, COLOR_PAIR(5));
    wrefresh(success_woodmark_win);

    ColoredWordMenu colored_words[] = {{"Teacher", 4}};
    int padding = 2;
    int text_width = input_w - (padding * 2);
    int desc_height = calculate_text_height(desc, text_width) + 2;
    render_text_with_colors(stdscr, input_w, 7, input_x, desc, 11, 12, true, (void *)colored_words, 1, 0);

    ColoredWordMenu colored_words_ai_chat[] = {{"SUCCESS", 15}, {"AI", 15}, {"Chatbot", 15}};
    int current_y = 7 + desc_height + 1;
    int ai_chat_height = calculate_text_height(ai_chat_text, text_width) + 2;
    render_text_with_colors(stdscr, input_w, current_y, input_x, ai_chat_text, 14, 12, true, (void *)colored_words_ai_chat, 3, 15);

    current_y += ai_chat_height + 1;
    ColoredWordMenu colored_words_social[] = {{"Social", 16}, {"Hall", 16}};
    render_text_with_colors(stdscr, input_w, current_y, input_x, social_hall_text, 14, 12, true, (void *)colored_words_social, 2, 16);

    int guide_y = input_y - 3;
    const char *guide_single = "[c] Chat with SUCCESS AI   [s] Access Social Hall   [x] Exit";
    const char *guide_line1 = "[c] Chat with SUCCESS AI   [s] Access Social Hall";
    const char *guide_line2 = "                      [x] Exit                      ";

    int guide_single_len = (int)strlen(guide_single);
    int guide_line1_len = (int)strlen(guide_line1);
    int guide_line2_len = (int)strlen(guide_line2);

    if (w >= guide_single_len + 4) {
      int guide_x = (w - guide_single_len) / 2;
      render_guide_line(guide_y, guide_x, guide_single);
    } else {
      int max_wrapped_len = guide_line1_len > guide_line2_len ? guide_line1_len : guide_line2_len;
      int guide_x = (w - max_wrapped_len) / 2;
      render_guide_line(guide_y - 1, guide_x, guide_line1);
      render_guide_line(guide_y, guide_x, guide_line2);
    }

    int input_cursor_y = input_y + 1;
    int input_cursor_x = input_x + 3;
    move(input_cursor_y, input_cursor_x);
    refresh();

    int capacity = 16;
    int cursor_pos = 0;
    char *input = malloc(capacity);
    input[0] = '\0';

    int ch;
    while (1) {
      ch = getch();

      if (ch == 10 || ch == KEY_ENTER) {
        if (cursor_pos == 1 && input[0] == 'c') {
          endwin();
#ifdef _WIN32
          system("cls");
#else
          system("clear");
#endif
          ai_chat();
          break;
        } else if (cursor_pos == 1 && input[0] == 's') {
          endwin();
#ifdef _WIN32
          system("cls");
#else
          system("clear");
#endif
          social_hall();
          break;
        } else if (cursor_pos == 1 && input[0] == 'x') {
          endwin();
          free(input);
          exit(0);
        }
        cursor_pos = 0;
        input[0] = '\0';
        int input_w = getmaxx(stdscr) - 10;
        int clear_width = input_w - 4;
        if (clear_width > 0) {
          wattron(stdscr, COLOR_PAIR(1));
          mvwhline(stdscr, input_cursor_y, input_cursor_x, ' ', clear_width);
          wattroff(stdscr, COLOR_PAIR(1));
        }
        move(input_cursor_y, input_cursor_x);
        refresh();
        continue;
      }

      if (ch == KEY_RESIZE) {
        curs_set(0);
        clear();
        getmaxyx(stdscr, h, w);
        draw_status_bar_menu_wrapper(h, w, " Success v0.1.10 ", " in Teacher Menu ", " Made with <3 ");
        int input_w = w - 10;
        int input_y = h - 7;
        int input_x = (w - input_w) / 2;
        // Use menu.c's render_input_field wrapper to avoid conflict with static render_input
        extern void render_input_field(WINDOW *win, int w, int h, int y, int x, char *fieldname);
        render_input_field(stdscr, input_w, 3, input_y, input_x, ">");

        WINDOW *success_woodmark_win = render_win(stdscr, 40, 10, 1, (w - 38) / 2, success_woodmark, 2, 13, false);
        wattron(success_woodmark_win, COLOR_PAIR(5));
        mvwaddstr(success_woodmark_win, 1, 7, "█  █ ▄▀▀▀ ▄▀▀▀");
        mvwaddstr(success_woodmark_win, 2, 7, "█░░█ █░░░ █░░░");
        mvwaddstr(success_woodmark_win, 3, 8, "▀▀▀ ▀▀▀▀ ▀▀▀▀");
        wattroff(success_woodmark_win, COLOR_PAIR(5));
        wrefresh(success_woodmark_win);

        ColoredWordMenu colored_words[] = {{"Teacher", 4}};
        int padding = 2;
        int text_width = input_w - (padding * 2);
        int desc_height = calculate_text_height(desc, text_width) + 2;
        render_text_with_colors(stdscr, input_w, 7, input_x, desc, 11, 12, true, (void *)colored_words, 1, 0);

        ColoredWordMenu colored_words_ai_chat[] = {{"SUCCESS", 15}, {"AI", 15}, {"Chatbot", 15}};
        int current_y = 7 + desc_height + 1;
        int ai_chat_height = calculate_text_height(ai_chat_text, text_width) + 2;
        render_text_with_colors(stdscr, input_w, current_y, input_x, ai_chat_text, 14, 12, true, (void *)colored_words_ai_chat, 3, 15);

        current_y += ai_chat_height + 1;
        ColoredWordMenu colored_words_social[] = {{"Social", 16}, {"Hall", 16}};
        render_text_with_colors(stdscr, input_w, current_y, input_x, social_hall_text, 14, 12, true, (void *)colored_words_social, 2, 16);

        int guide_y = input_y - 3;
        if (w >= guide_single_len + 4) {
          int guide_x = (w - guide_single_len) / 2;
          render_guide_line(guide_y, guide_x, guide_single);
        } else {
          int max_wrapped_len = guide_line1_len > guide_line2_len ? guide_line1_len : guide_line2_len;
          int guide_x = (w - max_wrapped_len) / 2;
          render_guide_line(guide_y - 1, guide_x, guide_line1);
          render_guide_line(guide_y, guide_x, guide_line2);
        }

        input_cursor_y = input_y + 1;
        input_cursor_x = input_x + 3;
        if (cursor_pos > 0) {
          wattron(stdscr, COLOR_PAIR(1));
          for (int i = 0; i < cursor_pos; i++) {
            mvwaddch(stdscr, input_cursor_y, input_cursor_x + i, input[i]);
          }
          wattroff(stdscr, COLOR_PAIR(1));
        }
        move(input_cursor_y, input_cursor_x + cursor_pos);
        curs_set(1);
        refresh();
      } else if ((ch == KEY_BACKSPACE || ch == 127 || ch == 8) && cursor_pos > 0) {
        cursor_pos--;
        input[cursor_pos] = '\0';
        wattron(stdscr, COLOR_PAIR(1));
        mvwaddch(stdscr, input_cursor_y, input_cursor_x + cursor_pos, ' ');
        wattroff(stdscr, COLOR_PAIR(1));
        wmove(stdscr, input_cursor_y, input_cursor_x + cursor_pos);
      } else if (ch >= 32 && ch <= 126) {
        if (cursor_pos + 1 >= capacity) {
          capacity *= 2;
          char *new_buf = realloc(input, capacity);
          input = new_buf;
        }
        input[cursor_pos++] = (char)ch;
        input[cursor_pos] = '\0';
        wattron(stdscr, COLOR_PAIR(1));
        mvwaddch(stdscr, input_cursor_y, input_cursor_x + cursor_pos - 1, ch);
        wattroff(stdscr, COLOR_PAIR(1));
        refresh();
      }
    }

    free(input);
    continue;
  }
}
