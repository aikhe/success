#include "social_hall.h"
#include "../pages/menu.h"
#include "curses.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef __declspec
#define __declspec(x)
#endif

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <locale.h>
#include <nfd.h>
#include <pthread.h>

#include "../gemini_api/gemini_request.h"
#include "../gemini_api/get_file_uri.h"
#include "../gemini_api/get_upload_url.h"

#include "../utils/gemini_loading.h"
#include "../utils/get_file_mime_type.h"
#include "../utils/read_file.h"
#include "../utils/read_file_b64.h"

extern bool is_generating;

// Forward declarations for functions from menu.c
extern void render_input(WINDOW *win, int w, int h, int y, int x,
                         char *fieldname);
extern WINDOW *render_win(WINDOW *win, int w, int h, int y, int x,
                          const char *content, int bg_color, int bar_color,
                          bool has_bg);
extern void define_colors(void);

// Helper structure for colored text
typedef struct {
  const char *word;
  int color_pair;
} ColoredWord;

// Resource structure
typedef struct {
  int id;
  char *title;
  char *content;      // Full content with pages
  char *author;
  char *date;
  int total_pages;
} Resource;

static Resource *resources = NULL;
static int resource_count = 0;
static sqlite3 *db = NULL;

// Get current username from session file
static void get_current_username(char *username, size_t len) {
  username[0] = '\0';
  FILE *f = fopen("db/.session", "r");
  if (f) {
    if (fgets(username, len, f)) {
      size_t slen = strlen(username);
      if (slen > 0 && username[slen - 1] == '\n') {
        username[slen - 1] = '\0';
      }
    }
    fclose(f);
  }
  if (strlen(username) == 0) {
    strncpy(username, "default_user", len - 1);
    username[len - 1] = '\0';
  }
}

// Get user role (teacher or student) from database
static int get_user_role(const char *username, char *role, size_t len) {
  role[0] = '\0';
  sqlite3 *user_db;
  int rc = sqlite3_open("db/users.db", &user_db);
  if (rc != SQLITE_OK) {
    return 0;
  }

  const char *sql = "SELECT userinfo FROM users WHERE username = ?;";
  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(user_db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    sqlite3_close(user_db);
    return 0;
  }

  sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    const unsigned char *userinfo = sqlite3_column_text(stmt, 0);
    if (userinfo) {
      strncpy(role, (const char *)userinfo, len - 1);
      role[len - 1] = '\0';
    }
    sqlite3_finalize(stmt);
    sqlite3_close(user_db);
    return 1;
  }

  sqlite3_finalize(stmt);
  sqlite3_close(user_db);
  return 0;
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

// Calculate height needed for wrapped text
static int calculate_text_height_local(const char *text, int width) {
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

  int lines = 1;
  int current_len = 0;
  const char *p = clean;

  while (*p) {
    if (*p == ' ') {
      p++;
      if (current_len > 0) {
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

// Render text with wrapping, padding, and colored words
static WINDOW *render_text_with_colors(WINDOW *win, int w, int y, int x,
                                       const char *text, int bg_color,
                                       int bar_color, bool has_bg,
                                       ColoredWord *colored_words,
                                       int num_colors, int sidebar_color) {
  int padding = 2;
  int text_width = w - (padding * 2);

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

  int text_h = calculate_text_height_local(clean, text_width);
  int window_h = text_h + 2;

  WINDOW *win_bars = newwin(window_h, w + 2, y, x - 1);
  WINDOW *text_win = newwin(window_h, w, y, x);
  wbkgd(win_bars, COLOR_PAIR(bar_color));
  wbkgd(text_win, COLOR_PAIR(bg_color));

  if (has_bg) {
    cchar_t vbar;
    setcchar(&vbar, L"┃", 0, 0, NULL);
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
    for (int i = 0; i < window_h; i++) {
      mvwadd_wch(win_bars, i, w + 2 - 1, &vbar);
    }
  }

  int line = 1;
  int col = padding;
  const char *p = clean;

  while (*p) {
    if (*p == ' ') {
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

    const char *word_start = p;
    int word_len = 0;
    while (*p && *p != ' ') {
      word_len++;
      p++;
    }

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

    if (col + word_len > text_width + padding) {
      line++;
      col = padding;
    }

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

// Initialize database and create table
static int init_db(void) {
#ifdef _WIN32
  system("if not exist db mkdir db");
#else
  system("mkdir -p db");
#endif

  int rc = sqlite3_open("db/resources.db", &db);
  if (rc != SQLITE_OK) {
    return 1;
  }

  const char *sql = "CREATE TABLE IF NOT EXISTS resources ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "title TEXT NOT NULL,"
                    "content TEXT NOT NULL,"
                    "author TEXT NOT NULL,"
                    "date TEXT NOT NULL,"
                    "total_pages INTEGER DEFAULT 1,"
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

// Load all resources
static void load_resources(void) {
  if (resources) {
    for (int i = 0; i < resource_count; i++) {
      free(resources[i].title);
      free(resources[i].content);
      free(resources[i].author);
      free(resources[i].date);
    }
    free(resources);
    resources = NULL;
  }
  resource_count = 0;

  if (!db) {
    return;
  }

  sqlite3_stmt *stmt;
  const char *sql = "SELECT id, title, content, author, date, total_pages FROM resources ORDER BY created_at DESC;";

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    return;
  }

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    resources = realloc(resources, (resource_count + 1) * sizeof(Resource));
    if (!resources) {
      break;
    }

    resources[resource_count].id = sqlite3_column_int(stmt, 0);
    const unsigned char *title_text = sqlite3_column_text(stmt, 1);
    const unsigned char *content_text = sqlite3_column_text(stmt, 2);
    const unsigned char *author_text = sqlite3_column_text(stmt, 3);
    const unsigned char *date_text = sqlite3_column_text(stmt, 4);

    resources[resource_count].title = strdup((const char *)title_text);
    resources[resource_count].content = strdup((const char *)content_text);
    resources[resource_count].author = strdup((const char *)author_text);
    resources[resource_count].date = strdup((const char *)date_text);
    resources[resource_count].total_pages = sqlite3_column_int(stmt, 5);
    resource_count++;
  }

  sqlite3_finalize(stmt);
}

// Add a new resource
static int add_resource(const char *title, const char *content, const char *author, int total_pages) {
  if (!db || !title || !content || !author || strlen(title) == 0 || strlen(content) == 0) {
    return 1;
  }

  time_t now = time(NULL);
  struct tm *timeinfo = localtime(&now);
  char date_str[64];
  strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", timeinfo);

  sqlite3_stmt *stmt;
  const char *sql = "INSERT INTO resources (title, content, author, date, total_pages) VALUES (?, ?, ?, ?, ?);";

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    return 1;
  }

  sqlite3_bind_text(stmt, 1, title, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, content, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, author, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 4, date_str, -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 5, total_pages);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  return (rc == SQLITE_DONE) ? 0 : 1;
}

// Split content into pages (200 words per page minimum)
static int split_into_pages(const char *content, char ***pages, int *page_count) {
  *page_count = 0;
  *pages = NULL;

  if (!content || strlen(content) == 0) {
    return 1;
  }

  // Count words in content
  int word_count = 0;
  const char *p = content;
  int in_word = 0;
  while (*p) {
    if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
      if (in_word) {
        word_count++;
        in_word = 0;
      }
    } else {
      in_word = 1;
    }
    p++;
  }
  if (in_word) {
    word_count++;
  }

  // Calculate number of pages (200 words minimum per page)
  int total_pages = (word_count + 199) / 200;
  if (total_pages == 0) {
    total_pages = 1;
  }

  *pages = malloc(total_pages * sizeof(char *));
  if (!*pages) {
    return 1;
  }

  // Split content into pages
  int current_page = 0;
  int words_in_page = 0;
  const char *page_start = content;
  p = content;
  in_word = 0;
  while (*p) {
    if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
      if (in_word) {
        words_in_page++;
        in_word = 0;
      }
    } else {
      if (!in_word) {
        in_word = 1;
      }
    }

    // If we've reached 200 words and this is a word boundary, start new page
    if (words_in_page >= 200 && (in_word == 0 || *p == '\n' || *p == '\r')) {
      int page_len = p - page_start;
      (*pages)[current_page] = malloc(page_len + 1);
      if ((*pages)[current_page]) {
        strncpy((*pages)[current_page], page_start, page_len);
        (*pages)[current_page][page_len] = '\0';
      }
      current_page++;
      page_start = p;
      words_in_page = 0;
    }

    p++;
  }

  // Add remaining content as last page
  if (current_page < total_pages) {
    int page_len = strlen(page_start);
    (*pages)[current_page] = malloc(page_len + 1);
    if ((*pages)[current_page]) {
      strncpy((*pages)[current_page], page_start, page_len);
      (*pages)[current_page][page_len] = '\0';
    }
    current_page++;
  }

  *page_count = total_pages;
  return 0;
}

// Render markdown headers (# ## ### ####) with different colors
static void render_markdown_content(WINDOW *win, int w, int y, int x, const char *content, int bg_color, int bar_color, bool has_bg) {
  int padding_horizontal = 2; // Left and right padding
  int padding_vertical = 1;   // Top and bottom padding
  int text_width = w - (padding_horizontal * 2);

  // Calculate total height needed
  int total_lines = 0;
  const char *p = content;
  while (*p) {
    if (*p == '\n') {
      total_lines++;
    }
    p++;
  }
  total_lines++; // Last line

  // Add vertical padding (top and bottom)
  int window_h = total_lines + (padding_vertical * 2);

  WINDOW *win_bars = newwin(window_h, w + 2, y, x - 1);
  WINDOW *text_win = newwin(window_h, w, y, x);
  wbkgd(win_bars, COLOR_PAIR(bar_color));
  wbkgd(text_win, COLOR_PAIR(bg_color));

  if (has_bg) {
    cchar_t vbar;
    setcchar(&vbar, L"┃", 0, 0, NULL);
    for (int i = 0; i < window_h; i++) {
      mvwadd_wch(win_bars, i, 0, &vbar);
      mvwadd_wch(win_bars, i, w + 2 - 1, &vbar);
    }
  }

  // Render content line by line, handling markdown headers
  int line = 1 + padding_vertical; // Start after top padding
  p = content;
  const char *line_start = content;
  int col = padding_horizontal;

  while (*p) {
    if (*p == '\n' || *p == '\0') {
      int line_len = p - line_start;
      if (line_len > 0) {
        char *line_buf = malloc(line_len + 1);
        strncpy(line_buf, line_start, line_len);
        line_buf[line_len] = '\0';

        // Check for markdown headers
        int header_level = 0;
        const char *h_start = line_buf;
        while (*h_start == '#') {
          header_level++;
          h_start++;
        }
        if (header_level > 0 && (*h_start == ' ' || *h_start == '\t')) {
          // It's a header
          h_start++;
          while (*h_start == ' ' || *h_start == '\t') {
            h_start++;
          }

          int color_pair = 3; // Default foreground
          if (header_level == 1) {
            color_pair = 4; // Orange for # Header One
          } else if (header_level == 2) {
            color_pair = 15; // Purple for ## Header Two
          } else if (header_level == 3) {
            color_pair = 16; // Green for ### Header Three
          } else if (header_level == 4) {
            color_pair = 17; // Cyan for #### Header Four
          }

          // Clear line with background color first
          wattron(text_win, COLOR_PAIR(bg_color));
          int clear_len = w - padding_horizontal * 2;
          for (int i = 0; i < clear_len; i++) {
            mvwaddch(text_win, line, padding_horizontal + i, ' ');
          }
          // Now render header text
          wattron(text_win, COLOR_PAIR(color_pair));
          // Skip the #'s when rendering
          mvwaddstr(text_win, line, col, h_start);
          wattroff(text_win, COLOR_PAIR(color_pair));
        } else {
          // Regular content - use bg_color for background
          // Clear the line first to ensure background color
          wattron(text_win, COLOR_PAIR(bg_color)); // Use window background color
          int clear_len = w - padding_horizontal * 2;
          for (int i = 0; i < clear_len; i++) {
            mvwaddch(text_win, line, padding_horizontal + i, ' ');
          }
          // Now add text with proper color (use bg_color which has correct foreground and background)
          wattroff(text_win, COLOR_PAIR(bg_color));
          wattron(text_win, COLOR_PAIR(bg_color)); // Use bg_color pair (has correct fg/bg)
          mvwaddstr(text_win, line, col, line_buf);
          wattroff(text_win, COLOR_PAIR(bg_color));
        }

        free(line_buf);
      }

      line++;
      col = padding_horizontal;
      if (*p == '\n') {
        p++;
      }
      line_start = p;
      continue;
    }

    // Handle word wrapping
    if (col >= text_width + padding_horizontal - 1) {
      // Find last space before this position
      const char *space_pos = p;
      while (space_pos > line_start && *space_pos != ' ') {
        space_pos--;
      }
      if (*space_pos == ' ') {
        // Wrap at space
        int wrap_len = space_pos - line_start;
        char *wrap_buf = malloc(wrap_len + 1);
        strncpy(wrap_buf, line_start, wrap_len);
        wrap_buf[wrap_len] = '\0';

        // Clear line with background color
        wattron(text_win, COLOR_PAIR(bg_color));
        int clear_len = w - padding_horizontal * 2;
        for (int i = 0; i < clear_len; i++) {
          mvwaddch(text_win, line, padding_horizontal + i, ' ');
        }
        // Render text with background color
        wattroff(text_win, COLOR_PAIR(bg_color));
        wattron(text_win, COLOR_PAIR(bg_color)); // Use bg_color pair (has correct fg/bg)
        mvwaddstr(text_win, line, col, wrap_buf);
        wattroff(text_win, COLOR_PAIR(bg_color));

        line++;
        col = padding_horizontal;
        line_start = space_pos + 1;
        p = space_pos + 1;
        free(wrap_buf);
        continue;
      }
    }

    p++;
  }

  wrefresh(win_bars);
  wrefresh(text_win);
  refresh();
}

// View a resource
static void view_resource(int resource_id) {
  Resource *resource = NULL;
  for (int i = 0; i < resource_count; i++) {
    if (resources[i].id == resource_id) {
      resource = &resources[i];
      break;
    }
  }

  if (!resource) {
    return;
  }

  // Split content into pages
  char **pages = NULL;
  int page_count = 0;
  if (split_into_pages(resource->content, &pages, &page_count) != 0 || page_count == 0) {
    return;
  }

  int current_page = 0;

  while (1) {
    clear();
    int h, w;
    getmaxyx(stdscr, h, w);

    draw_status_bar(h, w, " Success v0.1.10 ", " Viewing Resource ", " Made with <3 ");

    // Top section: date left, title center, author right
    int title_x = (w - (int)strlen(resource->title)) / 2;
    wattron(stdscr, COLOR_PAIR(2)); // Gray for date
    mvaddstr(1, 2, resource->date);
    wattroff(stdscr, COLOR_PAIR(2));

    wattron(stdscr, COLOR_PAIR(3)); // Foreground for title
    mvaddstr(1, title_x, resource->title);
    wattroff(stdscr, COLOR_PAIR(3));

    char author_text[256];
    snprintf(author_text, sizeof(author_text), "By: %s", resource->author);
    int author_x = w - (int)strlen(author_text) - 2;
    wattron(stdscr, COLOR_PAIR(2)); // Gray for author
    mvaddstr(1, author_x, author_text);
    wattroff(stdscr, COLOR_PAIR(2));

    // Middle section: content window
    int input_w = w - 10;
    int input_y = h - 7;
    int input_x = (w - input_w) / 2;
    int content_y = 4;

    render_markdown_content(stdscr, input_w, content_y, input_x, pages[current_page], 11, 12, true);

    // Bottom section: page num/total pages
    char page_text[64];
    snprintf(page_text, sizeof(page_text), "Page %d/%d", current_page + 1, page_count);
    int page_x = (w - (int)strlen(page_text)) / 2;
    int page_y = h - 5; // Position above guide with spacing
    wattron(stdscr, COLOR_PAIR(2));
    mvaddstr(page_y, page_x, page_text);
    wattroff(stdscr, COLOR_PAIR(2));

    // Guide - one space below page number
    const char *guide_text = "[←] Previous Page   [→] Next Page   [x] Exit";
    int guide_x = (w - (int)strlen(guide_text)) / 2;
    wattron(stdscr, COLOR_PAIR(2));
    mvaddstr(page_y + 1, guide_x, guide_text); // One space below page number
    wattroff(stdscr, COLOR_PAIR(2));

    refresh();

    int ch = getch();
    if (ch == KEY_LEFT && current_page > 0) {
      current_page--;
    } else if (ch == KEY_RIGHT && current_page < page_count - 1) {
      current_page++;
    } else if (ch == 'x' || ch == 'X') {
      break;
    }
  }

  // Free pages
  for (int i = 0; i < page_count; i++) {
    if (pages[i]) {
      free(pages[i]);
    }
  }
  free(pages);
}

// Upload resource page (teachers only)
static void upload_resource(const char *username) {
  clear();
  int h, w;
  getmaxyx(stdscr, h, w);

  draw_status_bar(h, w, " Success v0.1.10 ", " Upload Resource ", " Made with <3 ");

  const char *title = "UPLOAD RESOURCE";
  int title_x = (w - (int)strlen(title)) / 2;
  wattron(stdscr, COLOR_PAIR(3));
  mvaddstr(2, title_x, title);
  wattroff(stdscr, COLOR_PAIR(3));

  // Description window
  const char *desc_text = R"(
    Upload academic lessons or resources. Input a topic or attach images and PDF files (press 'f' to attach files). The content will be converted to text by Gemini AI and stored for students to view. Format: Use markdown headers (# Header One, ## Header Two, ### Header Three, #### Header Four). Each page should have at least 200 words. There's no limit to pages.
  )";

  int input_w = w - 10;
  int input_x = (w - input_w) / 2;
  int desc_padding = 2;
  int desc_text_width = input_w - (desc_padding * 2);
  int desc_height = calculate_text_height_local(desc_text, desc_text_width) + 2;
  int desc_y = 4;

  ColoredWord colored_words_desc[] = {};
  render_text_with_colors(stdscr, input_w, desc_y, input_x, desc_text, 11, 12, true, colored_words_desc, 0, 0);

    // Input field - positioned at bottom (2 spaces above status bar)
    // Status bar is at h-1, need 2 spaces, then input (3 lines tall)
    // So: status bar at h-1, space at h-2, space at h-3, input at h-6 to h-4
    int input_y = h - 6; // 2 spaces above status bar (status bar is at h-1)
  char *input = malloc(1024);
  int cursor_pos = 0;
  int capacity = 1024;
  input[0] = '\0';

  char **file_uris = NULL;
  char **exts = NULL;
  char **file_names = NULL;
  int total_file_num = 0;
  nfdresult_t nfd_res = NFD_CANCEL;
  nfdpathset_t pathSet = {0};

  // Load env.json for Gemini API
  char *env_json = read_file("../env.json");
  cJSON *env = cJSON_Parse(env_json);
  if (!env) {
    free(env_json);
    endwin();
    return;
  }

  cJSON *gemini_api_key = cJSON_GetObjectItemCaseSensitive(env, "GEMINI_API_KEY");
  cJSON *gemini_api_url = cJSON_GetObjectItemCaseSensitive(env, "GEMINI_API_URL");
  cJSON *gemini_file_url = cJSON_GetObjectItemCaseSensitive(env, "GEMINI_FILE_URL");

  while (1) {
    clear();
    getmaxyx(stdscr, h, w);

    draw_status_bar(h, w, " Success v0.1.10 ", " Upload Resource ", " Made with <3 ");

    wattron(stdscr, COLOR_PAIR(3));
    mvaddstr(2, title_x, title);
    wattroff(stdscr, COLOR_PAIR(3));

    render_text_with_colors(stdscr, input_w, desc_y, input_x, desc_text, 11, 12, true, colored_words_desc, 0, 0);

    // Show attached files (between description and guide)
    int files_y = desc_y + desc_height + 1;
    if (total_file_num > 0) {
      wattron(stdscr, COLOR_PAIR(2));
      mvprintw(files_y, input_x, "Attached files: %d", total_file_num);
      wattroff(stdscr, COLOR_PAIR(2));
    }

    // Guide - one space above input bar
    const char *guide_text = "[Enter] Submit   [f] Attach Files   [x] Cancel";
    int guide_x = (w - (int)strlen(guide_text)) / 2;
    int guide_y = input_y - 1; // One space above input bar
    wattron(stdscr, COLOR_PAIR(2));
    mvaddstr(guide_y, guide_x, guide_text);
    wattroff(stdscr, COLOR_PAIR(2));

    // Input field
    char input_display[1024];
    snprintf(input_display, sizeof(input_display), "Topic/Prompt: %s", input);
    render_input(stdscr, input_w, 3, input_y, input_x, input_display);

    int input_cursor_y = input_y + 1;
    int input_cursor_x = input_x + 15; // After "Topic/Prompt: "
    move(input_cursor_y, input_cursor_x + cursor_pos);
    curs_set(1);

    refresh();

    int ch = getch();

    if (ch == 10 || ch == KEY_ENTER) {
      // Validate: must have topic or file attached
      if (strlen(input) == 0 && total_file_num == 0) {
        continue; // Can't submit empty
      }

      // Clear screen and show loading animation
      clear();
      refresh();
      endwin(); // End ncurses to show printf output

      // Submit - convert with Gemini
      char systemPrompt[2048] = "You are an educational content converter. Convert the provided academic lesson, image, or PDF into a well-structured educational text format. The response must follow this EXACT format:\n\n"
                                  "- Use markdown headers: # Header One, ## Header Two, ### Header Three, #### Header Four\n"
                                  "- Include regular content paragraphs\n"
                                  "- Each page should have at least 200 words\n"
                                  "- There's no limit to pages\n"
                                  "- Format: # Header One\n## Header Two\n### Header Three\n#### Header Four\nContent\nPage num\nTotal pages\n\n"
                                  "Make the content educational, informative, and well-structured.";

      char fullPrompt[4096];
      snprintf(fullPrompt, sizeof(fullPrompt), "System Prompt: %s\nUser Prompt: %s", systemPrompt, strlen(input) > 0 ? input : "Convert the attached files into educational content.");

      // Show loading animation
      is_generating = true;
      pthread_t generate_thread = {0};
      pthread_create(&generate_thread, NULL, gemini_loading, NULL);

      curl_global_init(CURL_GLOBAL_DEFAULT);

      // Validate that if we have files, we have valid URIs
      if (total_file_num > 0 && (!file_uris || !exts)) {
        // Cleanup and return - something went wrong with file processing
        is_generating = false;
        pthread_cancel(generate_thread);
        pthread_join(generate_thread, NULL);
        printf("\r\033[K");
        fflush(stdout);
        curl_global_cleanup();
        
        // Reinitialize ncurses
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        define_colors();
        wbkgd(stdscr, COLOR_PAIR(5));
        
        // Continue loop to allow user to try again
        continue;
      }

      char *res_gemini_req = gemini_request(
          gemini_api_url->valuestring,
          total_file_num > 0 ? file_uris : NULL,
          gemini_api_key->valuestring,
          fullPrompt,
          total_file_num > 0 ? exts : NULL,
          total_file_num);

      is_generating = false;
      pthread_cancel(generate_thread);
      pthread_join(generate_thread, NULL);
      
      // Clear the "Thinking....." output
      printf("\r\033[K"); // Clear the line
      fflush(stdout);
      
      curl_global_cleanup();
      
      // Reinitialize ncurses after loading
      initscr();
      cbreak();
      noecho();
      keypad(stdscr, TRUE);
      curs_set(0);
      define_colors();
      wbkgd(stdscr, COLOR_PAIR(5));

      if (res_gemini_req) {
        // Extract title from input or use default
        char *title = strlen(input) > 0 ? strdup(input) : strdup("Resource");
        if (strlen(title) > 100) {
          title[100] = '\0';
        }

        // Count pages (split by "Page num" markers or calculate from word count)
        char **pages = NULL;
        int page_count = 0;
        split_into_pages(res_gemini_req, &pages, &page_count);

        // Save to database
        add_resource(title, res_gemini_req, username, page_count);

        // Free pages
        for (int i = 0; i < page_count; i++) {
          if (pages[i]) {
            free(pages[i]);
          }
        }
        free(pages);

        free(title);
        free(res_gemini_req);
      }

      // Cleanup before returning to social hall
      free(input);
      if (file_uris) {
        for (int i = 0; i < total_file_num; i++) {
          if (file_uris[i]) {
            free(file_uris[i]);
          }
        }
        free(file_uris);
      }
      if (exts) {
        free(exts);
      }
      if (file_names) {
        for (int i = 0; i < total_file_num; i++) {
          if (file_names[i]) {
            free(file_names[i]);
          }
        }
        free(file_names);
      }
      NFD_PathSet_Free(&pathSet);
      cJSON_Delete(env);
      free(env_json);
      
      // Return to social hall (will reload resources)
      return;
    } else if (ch == 'f' || ch == 'F') {
      // Attach files
      endwin();
      nfd_res = NFD_OpenDialogMultiple("png,jpeg,jpg,pdf", NULL, &pathSet);

      initscr();
      cbreak();
      noecho();
      keypad(stdscr, TRUE);
      curs_set(1);
      define_colors();
      wbkgd(stdscr, COLOR_PAIR(5));

      if (nfd_res == NFD_OKAY) {
        int selected_count = NFD_PathSet_GetCount(&pathSet);

        if (file_names) {
          for (int i = 0; i < total_file_num; i++) {
            if (file_names[i]) {
              free(file_names[i]);
            }
          }
          free(file_names);
        }
        file_names = malloc(selected_count * sizeof(char *));
        if (file_names) {
          for (int i = 0; i < selected_count; i++) {
            file_names[i] = NULL;
          }
        }

        for (size_t i = 0; i < selected_count; ++i) {
          nfdchar_t *path = NFD_PathSet_GetPath(&pathSet, i);

          const char *filename = strrchr(path, '/');
          if (!filename) {
            filename = strrchr(path, '\\');
          }
          if (!filename) {
            filename = path;
          } else {
            filename++;
          }

          if (file_names && (int)i < selected_count) {
            file_names[i] = strdup(filename);
          }

          size_t encoded_len;
          unsigned char *file_data = read_file_b64(path, &encoded_len);
          if (!file_data) {
            // Failed to read file - skip this file
            continue;
          }
          
          const char *ext = get_file_mime_type(path);
          if (!ext) {
            // Failed to get mime type - skip this file
            free(file_data);
            continue;
          }

          char *res_upload_url = NULL;
          if (gemini_file_url && gemini_file_url->valuestring &&
              gemini_api_key && gemini_api_key->valuestring) {
            res_upload_url =
                get_upload_url(encoded_len, gemini_file_url->valuestring,
                               gemini_api_key->valuestring, (char *)ext);
          }

          char *res_file_uri = NULL;
          if (res_upload_url && gemini_api_key && gemini_api_key->valuestring) {
            res_file_uri =
                get_file_uri(file_data, encoded_len, path, res_upload_url,
                             gemini_api_key->valuestring, (char *)ext);
          }

          if (res_upload_url) {
            free(res_upload_url);
            res_upload_url = NULL;
          }

          if (res_file_uri) {
            int new_capacity = (total_file_num == 0) ? 1 : (total_file_num + 1);
            char **new_ext = realloc(exts, new_capacity * sizeof(char *));
            char **new_file_uris =
                realloc(file_uris, new_capacity * sizeof(char *));
            
            // Check if realloc succeeded
            if (!new_ext || !new_file_uris) {
              // Realloc failed - free what we have and abort this file
              if (res_file_uri) {
                free(res_file_uri);
              }
              // Don't free new_ext or new_file_uris if they're NULL (realloc failed)
              // If one succeeded but the other failed, we'd need to free the successful one,
              // but that's complex. For now, just skip this file.
              free(file_data);
              continue; // Skip this file and try next one
            }
            
            exts = new_ext;
            file_uris = new_file_uris;

            exts[total_file_num] = (char *)ext;
            file_uris[total_file_num] = res_file_uri;
            total_file_num++;
          }

          free(file_data);
        }
      }
      NFD_PathSet_Free(&pathSet);
    } else if (ch == 'x' || ch == 'X') {
      break;
    } else if ((ch == KEY_BACKSPACE || ch == 127 || ch == 8) && cursor_pos > 0) {
      cursor_pos--;
      input[cursor_pos] = '\0';
    } else if (ch >= 32 && ch <= 126 && cursor_pos < capacity - 1) {
      input[cursor_pos++] = (char)ch;
      input[cursor_pos] = '\0';
    }
  }

  // Cleanup
  free(input);
  if (file_uris) {
    for (int i = 0; i < total_file_num; i++) {
      if (file_uris[i]) {
        free(file_uris[i]);
      }
    }
    free(file_uris);
  }
  if (exts) {
    free(exts);
  }
  if (file_names) {
    for (int i = 0; i < total_file_num; i++) {
      if (file_names[i]) {
        free(file_names[i]);
      }
    }
    free(file_names);
  }
  cJSON_Delete(env);
  free(env_json);
  NFD_PathSet_Free(&pathSet);
}

// Main Social Hall page
void social_hall(void) {
  char username[256];
  get_current_username(username, sizeof(username));

  char user_role[16] = {0};
  int role_found = get_user_role(username, user_role, sizeof(user_role));
  int is_teacher = (strcmp(user_role, "teacher") == 0);
  
  // Debug: If role not found, default to student
  if (!role_found || strlen(user_role) == 0) {
    is_teacher = 0;
  }

  if (init_db() != 0) {
    return;
  }

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
  curs_set(0);

  define_colors();

  wbkgd(stdscr, COLOR_PAIR(5));
  clear();

  load_resources();

  int selected_index = (resource_count > 0) ? 0 : -1;
  int current_page = 0;
  int resources_per_page = 8;

  while (1) {
    clear();
    int h, w;
    getmaxyx(stdscr, h, w);

    draw_status_bar(h, w, " Success v0.1.10 ", " in Social Hall ", " Made with <3 ");

    // Title
    const char *title = "SOCIAL HALL";
    int title_x = (w - (int)strlen(title)) / 2;
    wattron(stdscr, COLOR_PAIR(3));
    mvaddstr(2, title_x, title);
    wattroff(stdscr, COLOR_PAIR(3));
    
    // Show user role (especially helpful for teachers to see they can upload)
    int desc_y;
    if (is_teacher) {
      // 1 space after title (line 3)
      const char *role_text = "Teacher Mode - Press 'u' to upload resources";
      int role_x = (w - (int)strlen(role_text)) / 2;
      wattron(stdscr, COLOR_PAIR(4)); // Orange color
      mvaddstr(4, role_x, role_text); // Line 4 (1 space after title at line 2)
      wattroff(stdscr, COLOR_PAIR(4));
      desc_y = 6; // 1 space after teacher status (line 5)
    } else {
      desc_y = 4; // No teacher status, so 2 spaces after title
    }

    // Description window
    const char *desc_text = R"(
    Access learning materials and updates directly from your teachers. View resources, lessons, and academic content shared by educators.
    )";

    int input_w = w - 10;
    int input_x = (w - input_w) / 2;
    int desc_padding = 2;
    int desc_text_width = input_w - (desc_padding * 2);
    int desc_height = calculate_text_height_local(desc_text, desc_text_width) + 2;

    ColoredWord colored_words_desc[] = {{"Social", 16}, {"Hall", 16}};
    render_text_with_colors(stdscr, input_w, desc_y, input_x, desc_text, 11, 12, true, colored_words_desc, 2, 16);

    // Resources list
    int list_start_y = desc_y + desc_height + 3;
    int list_end_y = h - 8; // Leave space for page info, guide, and status bar
    int available_height = list_end_y - list_start_y;
    int max_visible = (available_height - 1) / 2; // 2 lines per resource (1 space between)
    if (max_visible > resources_per_page) {
      max_visible = resources_per_page;
    }

    int start_idx = current_page * resources_per_page;
    int end_idx = start_idx + resources_per_page;
    if (end_idx > resource_count) {
      end_idx = resource_count;
    }

    int visible_start = start_idx;
    int visible_end = start_idx + max_visible;
    if (visible_end > resource_count) {
      visible_end = resource_count;
    }

    for (int i = visible_start; i < visible_end; i++) {
      int display_y = list_start_y + (i - visible_start) * 2;
      if (display_y >= list_end_y) {
        break;
      }

      Resource *res = &resources[i];
      char display_text[512];
      snprintf(display_text, sizeof(display_text), "%s - By: %s (%s)", res->title, res->author, res->date);

      // Truncate if too long
      int max_display_len = input_w - 4;
      if ((int)strlen(display_text) > max_display_len) {
        display_text[max_display_len - 3] = '\0';
        strcat(display_text, "...");
      }

      // Color based on selection
      if (i == selected_index) {
        wattron(stdscr, COLOR_PAIR(4)); // Orange when selected
      } else {
        wattron(stdscr, COLOR_PAIR(2)); // Gray otherwise
      }

      mvaddstr(display_y, input_x + 2, display_text);

      if (i == selected_index) {
        wattroff(stdscr, COLOR_PAIR(4));
      } else {
        wattroff(stdscr, COLOR_PAIR(2));
      }
    }

    // Page info
    int total_pages = (resource_count + resources_per_page - 1) / resources_per_page;
    if (total_pages == 0) {
      total_pages = 1;
    }
    char page_text[64];
    snprintf(page_text, sizeof(page_text), "Page %d/%d", current_page + 1, total_pages);
    int page_x = (w - (int)strlen(page_text)) / 2;
    int page_y = h - 5; // Position for page number
    wattron(stdscr, COLOR_PAIR(2));
    mvaddstr(page_y, page_x, page_text);
    wattroff(stdscr, COLOR_PAIR(2));

    // Guide - one space below page number
    const char *guide_text;
    if (is_teacher) {
      guide_text = "[↑↓] Navigate   [Enter] View   [u] Upload Resource   [x] Exit";
    } else {
      guide_text = "[↑↓] Navigate   [Enter] View   [x] Exit";
    }
    int guide_x = (w - (int)strlen(guide_text)) / 2;
    if (is_teacher) {
      // Make upload option more visible for teachers
      wattron(stdscr, COLOR_PAIR(4)); // Orange color for teacher guide
    } else {
      wattron(stdscr, COLOR_PAIR(2)); // Gray for student guide
    }
    mvaddstr(page_y + 1, guide_x, guide_text); // One space below page number
    if (is_teacher) {
      wattroff(stdscr, COLOR_PAIR(4));
    } else {
      wattroff(stdscr, COLOR_PAIR(2));
    }

    refresh();

    int ch = getch();

    if (ch == KEY_UP && selected_index > 0) {
      selected_index--;
      // If we moved to previous page, adjust
      if (selected_index < current_page * resources_per_page) {
        current_page--;
        if (current_page < 0) {
          current_page = 0;
        }
      }
    } else if (ch == KEY_DOWN && selected_index < resource_count - 1) {
      selected_index++;
      // If we moved to next page, adjust
      if (selected_index >= (current_page + 1) * resources_per_page) {
        current_page++;
        int max_page = (resource_count + resources_per_page - 1) / resources_per_page - 1;
        if (current_page > max_page) {
          current_page = max_page;
        }
      }
    } else if ((ch == 10 || ch == KEY_ENTER) && selected_index >= 0 && selected_index < resource_count) {
      view_resource(resources[selected_index].id);
      load_resources(); // Reload in case anything changed
    } else if ((ch == 'u' || ch == 'U') && is_teacher) {
      upload_resource(username);
      load_resources(); // Reload after upload
      selected_index = 0; // Reset selection
      current_page = 0;
    } else if (ch == 'x' || ch == 'X') {
      break;
    } else if (ch == KEY_RESIZE) {
      continue; // Will redraw on next iteration
    }
  }

  // Cleanup
  if (resources) {
    for (int i = 0; i < resource_count; i++) {
      free(resources[i].title);
      free(resources[i].content);
      free(resources[i].author);
      free(resources[i].date);
    }
    free(resources);
    resources = NULL;
  }

  if (db) {
    sqlite3_close(db);
    db = NULL;
  }

  endwin();
}
