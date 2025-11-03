#include "todo.h"
#include "../pages/menu.h"
#include "curses.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for functions from menu.c
extern void render_input(WINDOW *win, int w, int h, int y, int x,
                         char *fieldname);
extern void define_colors(void);

// Get current username from session file
static void get_current_username(char *username, size_t len) {
  username[0] = '\0';
  FILE *f = fopen("db/.session", "r");
  if (f) {
    if (fgets(username, len, f)) {
      // Remove newline if present
      size_t slen = strlen(username);
      if (slen > 0 && username[slen - 1] == '\n') {
        username[slen - 1] = '\0';
      }
    }
    fclose(f);
  }
  // Fallback to default if file doesn't exist
  if (strlen(username) == 0) {
    strncpy(username, "default_user", len - 1);
    username[len - 1] = '\0';
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

// Todo item structure
typedef struct {
  int id;
  char *task;
  bool done;
} TodoItem;

static TodoItem *todos = NULL;
static int todo_count = 0;
static sqlite3 *db = NULL;

// Initialize database and create table
static int init_db(void) {
  // Ensure db directory exists
#ifdef _WIN32
  system("if not exist db mkdir db");
#else
  system("mkdir -p db");
#endif

  int rc = sqlite3_open("db/todos.db", &db);
  if (rc != SQLITE_OK) {
    return 1;
  }

  const char *sql = "CREATE TABLE IF NOT EXISTS todos ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "username TEXT NOT NULL,"
                    "task TEXT NOT NULL,"
                    "done INTEGER DEFAULT 0,"
                    "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
                    ");";

  char *err_msg = 0;
  rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
  if (rc != SQLITE_OK) {
    sqlite3_free(err_msg);
    sqlite3_close(db);
    db = NULL;
    return 1;
  }

  return 0;
}

// Load todos for a specific username
static void load_todos(const char *username) {
  // Free existing todos
  if (todos) {
    for (int i = 0; i < todo_count; i++) {
      free(todos[i].task);
    }
    free(todos);
    todos = NULL;
  }
  todo_count = 0;

  if (!db || !username) {
    return;
  }

  sqlite3_stmt *stmt;
  const char *sql = "SELECT id, task, done FROM todos WHERE username = ? "
                    "ORDER BY id ASC;";

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    return;
  }

  sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    todos = realloc(todos, (todo_count + 1) * sizeof(TodoItem));
    if (!todos) {
      break;
    }

    todos[todo_count].id = sqlite3_column_int(stmt, 0);
    const unsigned char *task_text = sqlite3_column_text(stmt, 1);
    todos[todo_count].task = strdup((const char *)task_text);
    todos[todo_count].done = (sqlite3_column_int(stmt, 2) == 1);
    todo_count++;
  }

  sqlite3_finalize(stmt);
}

// Add a new todo
static int add_todo(const char *username, const char *task) {
  if (!db || !username || !task || strlen(task) == 0) {
    return 1;
  }

  sqlite3_stmt *stmt;
  const char *sql = "INSERT INTO todos (username, task) VALUES (?, ?);";

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    return 1;
  }

  sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, task, -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  return (rc == SQLITE_DONE) ? 0 : 1;
}

// Delete a todo
static int delete_todo(int id) {
  if (!db) {
    return 1;
  }

  sqlite3_stmt *stmt;
  const char *sql = "DELETE FROM todos WHERE id = ?;";

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    return 1;
  }

  sqlite3_bind_int(stmt, 1, id);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  return (rc == SQLITE_DONE) ? 0 : 1;
}

// Toggle todo done status
static int toggle_todo(int id, bool done) {
  if (!db) {
    return 1;
  }

  sqlite3_stmt *stmt;
  const char *sql = "UPDATE todos SET done = ? WHERE id = ?;";

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    return 1;
  }

  sqlite3_bind_int(stmt, 1, done ? 1 : 0);
  sqlite3_bind_int(stmt, 2, id);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  return (rc == SQLITE_DONE) ? 0 : 1;
}

// Helper structure for colored text
typedef struct {
  const char *word;
  int color_pair;
} ColoredWord;

// Calculate height needed for wrapped text (with 2 padding each side)
static int calculate_text_height_local(const char *text, int width) {
  // Strip leading/trailing whitespace and normalize
  int len = (int)strlen(text);
  char *clean = malloc(len + 1);
  int clean_idx = 0;
  int in_word = 0;

  for (int i = 0; i < len; i++) {
    if (text[i] == '\n' || text[i] == '\r') {
      if (in_word) {
        clean[clean_idx++] = ' ';
        in_word = 0;
      }
    } else if (text[i] == ' ' || text[i] == '\t') {
      if (in_word) {
        clean[clean_idx++] = ' ';
        in_word = 0;
      }
    } else {
      clean[clean_idx++] = text[i];
      in_word = 1;
    }
  }
  clean[clean_idx] = '\0';

  // Calculate wrapped lines
  int lines = 1;
  int current_len = 0;
  const char *p = clean;

  while (*p) {
    if (*p == ' ') {
      p++;
      if (current_len > 0) {
        // Find next word length
        int next_word_len = 0;
        const char *next = p;
        while (*next && *next != ' ') {
          next_word_len++;
          next++;
        }
        if (current_len + 1 + next_word_len > width) {
          lines++;
          current_len = 0;
        } else {
          current_len++;
        }
      }
      continue;
    }
    current_len++;
    p++;
  }

  free(clean);
  return lines > 0 ? lines : 1;
}

// Render text with wrapping, padding, and colored words (with sidebar color
// support)
static WINDOW *render_text_with_colors(WINDOW *win, int w, int y, int x,
                                       const char *text, int bg_color,
                                       int bar_color, bool has_bg,
                                       ColoredWord *colored_words,
                                       int num_colors, int sidebar_color) {
  // Apply 2 padding left and right
  int padding = 2;
  int text_width = w - (padding * 2);

  // Clean and normalize text
  int len = (int)strlen(text);
  char *clean = malloc(len + 1);
  int clean_idx = 0;
  int in_word = 0;

  for (int i = 0; i < len; i++) {
    if (text[i] == '\n' || text[i] == '\r') {
      if (in_word) {
        clean[clean_idx++] = ' ';
        in_word = 0;
      }
    } else if (text[i] == ' ' || text[i] == '\t') {
      if (in_word) {
        clean[clean_idx++] = ' ';
        in_word = 0;
      }
    } else {
      clean[clean_idx++] = text[i];
      in_word = 1;
    }
  }
  clean[clean_idx] = '\0';

  // Calculate height based on wrapping
  int text_h = calculate_text_height_local(clean, text_width);
  int window_h = text_h + 2; // Add padding top/bottom

  WINDOW *win_bars = newwin(window_h, w + 2, y, x - 1);
  WINDOW *text_win = newwin(window_h, w, y, x);
  wbkgd(win_bars, COLOR_PAIR(bar_color));
  wbkgd(text_win, COLOR_PAIR(bg_color));

  if (has_bg) {
    cchar_t vbar;
    setcchar(&vbar, L"┃", 0, 0, NULL);
    // Left sidebar with color
    if (sidebar_color > 0) {
      wattron(win_bars, COLOR_PAIR(sidebar_color));
      for (int i = 0; i < window_h; i++) {
        mvwadd_wch(win_bars, i, 0, &vbar);
      }
      wattroff(win_bars, COLOR_PAIR(sidebar_color));
    } else {
      for (int i = 0; i < window_h; i++) {
        mvwadd_wch(win_bars, i, 0, &vbar);
      }
    }
    // Right bar
    for (int i = 0; i < window_h; i++) {
      mvwadd_wch(win_bars, i, w + 2 - 1, &vbar);
    }
  }

  // Render text with wrapping and coloring
  int line = 1;
  int col = padding;
  const char *p = clean;

  while (*p) {
    if (*p == ' ') {
      // Check if next word fits on this line
      const char *next = p + 1;
      while (*next == ' ')
        next++;
      int next_word_len = 0;
      const char *word_end = next;
      while (*word_end && *word_end != ' ') {
        next_word_len++;
        word_end++;
      }

      if (col > padding && col + 1 + next_word_len > text_width + padding) {
        line++;
        col = padding;
      } else if (col > padding) {
        mvwaddch(text_win, line, col, ' ');
        col++;
      }
      p++;
      continue;
    }

    // Find word
    const char *word_start = p;
    int word_len = 0;
    while (*p && *p != ' ') {
      word_len++;
      p++;
    }

    // Check if word should be colored
    char *word = malloc(word_len + 1);
    strncpy(word, word_start, word_len);
    word[word_len] = '\0';

    int found_color = -1;
    for (int i = 0; i < num_colors; i++) {
      if (strcmp(word, colored_words[i].word) == 0) {
        found_color = colored_words[i].color_pair;
        break;
      }
    }

    // Wrap if needed
    if (col + word_len > text_width + padding) {
      line++;
      col = padding;
    }

    // Print word
    if (found_color != -1) {
      wattron(text_win, COLOR_PAIR(found_color));
      for (int i = 0; i < word_len; i++) {
        mvwaddch(text_win, line, col + i, word[i]);
      }
      wattroff(text_win, COLOR_PAIR(found_color));
    } else {
      for (int i = 0; i < word_len; i++) {
        mvwaddch(text_win, line, col + i, word[i]);
      }
    }
    col += word_len;

    free(word);
  }

  free(clean);
  wrefresh(win_bars);
  wrefresh(text_win);
  refresh();

  return text_win;
}

// Helper function to render a guide line with gray text
static void render_guide_line(int y, int x, const char *text) {
  wattron(stdscr, COLOR_PAIR(2)); // Gray for guide text
  mvaddstr(y, x, text);
  wattroff(stdscr, COLOR_PAIR(2));
}

void todo(const char *username_param) {
  char username[256];

  // Get username from parameter or session file
  if (username_param && strlen(username_param) > 0) {
    strncpy(username, username_param, sizeof(username) - 1);
    username[sizeof(username) - 1] = '\0';
  } else {
    get_current_username(username, sizeof(username));
  }

  // Initialize database
  if (init_db() != 0) {
    return;
  }

  // Initialize ncurses
  WINDOW *scr = initscr();
  if (scr == NULL) {
    if (db) {
      sqlite3_close(db);
    }
    return;
  }

  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0); // Hide cursor initially

  define_colors();

  wbkgd(stdscr, COLOR_PAIR(5));
  clear();

  int h, w;
  getmaxyx(stdscr, h, w);

  // Load todos for this user
  load_todos(username);

  int selected_index = (todo_count > 0) ? 0 : -1;
  int input_mode = 0; // 0 = navigation, 1 = adding new todo
  char new_todo_input[256] = {0};
  int input_cursor_pos = 0;

  // Calculate display area
  int input_w = w - 10;
  int input_y = h - 7;
  int input_x = (w - input_w) / 2;

  flushinp();

  int ch;
  while (1) {
    clear();

    getmaxyx(stdscr, h, w);

    // Status bar
    draw_status_bar(h, w, " Success v0.1.10 ", " in Todo List ",
                    " Made with <3 ");

    // Title
    const char *title = "TODO LIST";
    int title_x = (w - (int)strlen(title)) / 2;
    wattron(stdscr, COLOR_PAIR(3));
    mvaddstr(2, title_x, title);
    wattroff(stdscr, COLOR_PAIR(3));

    // Description window
    const char *desc_text = R"(
    An academic and task tracking tool that helps you organize assignments, monitor deadlines, and stay on top of your study goals keeping your academic life structured and stress-free.
    )";

    int desc_input_w = w - 10;
    int desc_input_x = (w - desc_input_w) / 2;
    int desc_padding = 2;
    int desc_text_width = desc_input_w - (desc_padding * 2);
    int desc_height =
        calculate_text_height_local(desc_text, desc_text_width) + 2;
    int desc_y = 4;                        // Position below title
    ColoredWord colored_words_desc[] = {}; // No colored words for description
    render_text_with_colors(stdscr, desc_input_w, desc_y, desc_input_x,
                            desc_text, 11, 12, true, colored_words_desc, 0, 0);

    // Render todos list (start after description)
    int list_start_y = desc_y + desc_height + 1;
    int available_height =
        h - list_start_y -
        (input_mode
             ? 8
             : 6); // Space for title, desc, input (if shown), guide (3 spaces)
    int max_visible =
        available_height /
        2; // Divide by 2 since we use 2 lines per task (1 space between)
    int scroll_offset = 0;

    if (selected_index >= max_visible) {
      scroll_offset = selected_index - max_visible + 1;
    }

    for (int i = 0; i < todo_count; i++) {
      int display_y = list_start_y +
                      (i - scroll_offset) * 2; // *2 for 1 space between tasks
      if (display_y >= list_start_y + max_visible * 2 ||
          display_y < list_start_y) {
        continue;
      }

      // Checkbox and task
      char checkbox = todos[i].done ? 'X' : ' ';
      char display_text[512];
      snprintf(display_text, sizeof(display_text), "[%c] %s", checkbox,
               todos[i].task);

      // Color based on selection and done status
      if (i == selected_index && input_mode == 0) {
        // Selected - orange text
        wattron(stdscr, COLOR_PAIR(4));
      } else if (todos[i].done) {
        // Finished - green
        wattron(stdscr, COLOR_PAIR(16));
      } else {
        // Unfinished - gray
        wattron(stdscr, COLOR_PAIR(2));
      }

      mvaddstr(display_y, 5, display_text);

      if (i == selected_index && input_mode == 0) {
        wattroff(stdscr, COLOR_PAIR(4));
      } else if (todos[i].done) {
        wattroff(stdscr, COLOR_PAIR(16));
      } else {
        wattroff(stdscr, COLOR_PAIR(2));
      }
    }

    // Input field - only show when adding
    if (input_mode == 1) {
      // Format input with "> " prefix and space after for cursor consistency
      char input_display[256];
      snprintf(input_display, sizeof(input_display), "> %s ", new_todo_input);
      render_input(stdscr, input_w, 3, input_y, input_x, input_display);
      curs_set(1); // Show cursor for input
      move(input_y + 1,
           input_x + 3 + input_cursor_pos + 2); // +2 for "> " prefix
    } else {
      // Hide input bar when not adding
      curs_set(0); // Hide cursor when not adding
    }

    // Guide - 3 spaces from status bar (h - 1 is status bar, so h - 4 is 3
    // spaces above)
    int guide_y = input_mode == 1 ? input_y - 3 : h - 4;
    const char *guide_text;
    if (input_mode == 1) {
      guide_text = "[Enter] Add Todo   [ESC] Cancel";
    } else {
      guide_text = "[a] Add   [d] Delete   [↑↓] Navigate   [Enter] Toggle   "
                   "[x] Exit";
    }
    int guide_x = (w - (int)strlen(guide_text)) / 2;
    render_guide_line(guide_y, guide_x, guide_text);

    refresh();

    ch = getch();

    if (input_mode == 1) {
      // Adding new todo mode
      if (ch == 10 || ch == KEY_ENTER) {
        if (strlen(new_todo_input) > 0) {
          add_todo(username, new_todo_input);
          load_todos(username);
          new_todo_input[0] = '\0';
          input_cursor_pos = 0;
          input_mode = 0;
          selected_index = (todo_count > 0) ? todo_count - 1 : -1;
          curs_set(0); // Hide cursor
        }
      } else if (ch == 27) { // ESC
        new_todo_input[0] = '\0';
        input_cursor_pos = 0;
        input_mode = 0;
        curs_set(0); // Hide cursor
      } else if ((ch == KEY_BACKSPACE || ch == 127 || ch == 8) &&
                 input_cursor_pos > 0) {
        input_cursor_pos--;
        new_todo_input[input_cursor_pos] = '\0';
      } else if (ch >= 32 && ch <= 126 && input_cursor_pos < 254) {
        new_todo_input[input_cursor_pos++] = (char)ch;
        new_todo_input[input_cursor_pos] = '\0';
      }
    } else {
      // Navigation mode
      if (ch == 'a' || ch == 'A') {
        input_mode = 1;
        new_todo_input[0] = '\0';
        input_cursor_pos = 0;
        curs_set(1); // Show cursor
      } else if ((ch == 'd' || ch == 'D') && selected_index >= 0 &&
                 selected_index < todo_count) {
        delete_todo(todos[selected_index].id);
        load_todos(username);
        if (selected_index >= todo_count && todo_count > 0) {
          selected_index = todo_count - 1;
        } else if (todo_count == 0) {
          selected_index = -1;
        }
      } else if (ch == KEY_UP && selected_index > 0) {
        selected_index--;
      } else if (ch == KEY_DOWN && selected_index < todo_count - 1) {
        selected_index++;
      } else if ((ch == 10 || ch == KEY_ENTER) && selected_index >= 0 &&
                 selected_index < todo_count) {
        toggle_todo(todos[selected_index].id, !todos[selected_index].done);
        load_todos(username);
      } else if (ch == 'x' || ch == 'X') {
        break;
      }
    }

    if (ch == KEY_RESIZE) {
      // Will redraw on next iteration
      continue;
    }
  }

  // Cleanup
  if (todos) {
    for (int i = 0; i < todo_count; i++) {
      free(todos[i].task);
    }
    free(todos);
    todos = NULL;
  }

  if (db) {
    sqlite3_close(db);
    db = NULL;
  }

  endwin();
}
