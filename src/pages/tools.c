#include "tools.h"
#include "../features/flashcard.h"
#include "../features/quiz.h"
#include "../features/todo.h"
#include "curses.h"
#include "menu.h"
#include <stdlib.h>
#include <string.h>

// Forward declarations for functions from menu.c
extern void render_input(WINDOW *win, int w, int h, int y, int x,
                         char *fieldname);
extern WINDOW *render_win(WINDOW *win, int w, int h, int y, int x,
                          const char *content, int bg_color, int bar_color,
                          bool has_bg);
extern void define_colors(void);

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

// Helper function to get color pair for a guide character
static int get_guide_char_color(char ch) {
  switch (ch) {
  case 'f':
    return 15; // Purple for Flashcard
  case 'q':
    return 16; // Green for Quiz
  case 't':
    return 17; // Cyan for Todo
  case 'x':
    return 4; // Orange for Exit
  default:
    return 5; // White default
  }
}

// Helper function to render a guide line with colored brackets
static void render_guide_line(int y, int x, const char *text) {
  int pos = 0;
  const char *p = text;
  while (*p) {
    if (*p == '[') {
      p++;                // Skip '['
      int color_pair = 5; // Default white
      char guide_char = 0;

      if (*p && *p != ']') {
        guide_char = *p;
        color_pair = get_guide_char_color(*p);
      }

      // Draw opening bracket
      wattron(stdscr, COLOR_PAIR(color_pair));
      mvwaddch(stdscr, y, x + pos, '[');
      pos++;

      // Draw letter if present
      if (guide_char) {
        mvwaddch(stdscr, y, x + pos, guide_char);
        pos++;
        p++;
      }

      // Draw closing bracket
      if (*p == ']') {
        mvwaddch(stdscr, y, x + pos, ']');
        pos++;
        p++;
      }

      wattroff(stdscr, COLOR_PAIR(color_pair));
    } else {
      wattron(stdscr, COLOR_PAIR(2)); // Gray for rest
      mvwaddch(stdscr, y, x + pos, *p);
      wattroff(stdscr, COLOR_PAIR(2));
      pos++;
      p++;
    }
  }
}

void tools(void) {
  // Use a loop to return to tools page after quiz/flashcard
  while (1) {
    // Initialize ncurses - check for errors
    WINDOW *scr = initscr();
    if (scr == NULL) {
      // If initscr fails, we can't continue
      return;
    }

    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);

    define_colors();

    wbkgd(stdscr, COLOR_PAIR(5));

    clear();

    int h, w;
    getmaxyx(stdscr, h, w);

    const char *tools_title = R"(
  ▀██▀ █▀▀█ █▀▀█ ██   █▀▀▀ █
   ██  █░░█ █░░█ ██   ▀▀▀█ ▀
   ▀▀  ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀
              Powered by A.I
  )";

    // Text content for each section
    const char *desc = R"(
  Create powerful study materials with AI-powered tools. Generate flashcards, quizzes, and manage your todo list all in one place.
  )";

    const char *flashcard_text = R"(
  Flashcard Generator - Powered by AI. Create custom flashcards from any topic or text to boost your memorization.
  )";

    const char *quiz_text = R"(
  Quiz Maker - Powered by AI. Generate comprehensive quizzes with multiple question types to test your knowledge.
  )";

    const char *todo_text = R"(
  Todo List - Organize and track your study tasks and assignments with a simple, efficient todo list manager.
  )";

    // Initial display
    draw_status_bar(h, w, " Success v0.1.10 ", " in Tools ", " Made with <3 ");
    int input_w = w - 10;
    int input_y = h - 7;
    int input_x = (w - input_w) / 2;
    render_input(stdscr, input_w, 3, input_y, input_x, ">");

    // Render tools title at the top
    WINDOW *tools_title_win =
        render_win(stdscr, 38, 6, 1, (w - 36) / 2, tools_title, 2, 13, false);
    // Default gray color (no highlight)
    wrefresh(tools_title_win);

    // Calculate positions for text windows (start after title)
    int padding = 2;
    int text_width = input_w - (padding * 2);
    int current_y = 7; // Removed 1 space

    // Render description
    ColoredWord colored_words_desc[] = {{"AI", 15}, {"powered", 15}};
    int desc_height = calculate_text_height_local(desc, text_width) + 2;
    render_text_with_colors(stdscr, input_w, current_y, input_x, desc, 11, 12,
                            true, colored_words_desc, 2, 0);

    current_y += desc_height + 1;

    // Render Flashcard window (Purple sidebar)
    ColoredWord colored_words_flashcard[] = {
        {"Flashcard", 15}, {"Generator", 15}, {"AI", 15}};
    int flashcard_height =
        calculate_text_height_local(flashcard_text, text_width) + 2;
    render_text_with_colors(stdscr, input_w, current_y, input_x, flashcard_text,
                            14, 12, true, colored_words_flashcard, 3,
                            15); // Purple

    current_y += flashcard_height + 1;

    // Render Quiz window (Green sidebar)
    ColoredWord colored_words_quiz[] = {{"Quiz", 16}, {"Maker", 16}, {"AI", 16}};
    int quiz_height = calculate_text_height_local(quiz_text, text_width) + 2;
    render_text_with_colors(stdscr, input_w, current_y, input_x, quiz_text, 14,
                            12, true, colored_words_quiz, 3, 16); // Green

    current_y += quiz_height + 1;

    // Render Todo window (Cyan sidebar)
    ColoredWord colored_words_todo[] = {{"Todo", 17}, {"List", 17}};
    render_text_with_colors(stdscr, input_w, current_y, input_x, todo_text, 14,
                            12, true, colored_words_todo, 2, 17); // Cyan

    // Selection guide 2 lines up from input bar (wraps on small screens)
    int guide_y = input_y - 3;
    const char *guide_single =
        "[f] Flashcard Generator   [q] Quiz Maker   [t] Todo List   [x] Exit";
    const char *guide_line1 =
        "[f] Flashcard Generator            [t] Todo List       ";
    const char *guide_line2 =
        "[q] Quiz Maker                     [x] Exit            ";

    int guide_single_len = (int)strlen(guide_single);
    int guide_line1_len = (int)strlen(guide_line1);
    int guide_line2_len = (int)strlen(guide_line2);
    int max_wrapped_len =
        guide_line1_len > guide_line2_len ? guide_line1_len : guide_line2_len;

    // Use single line if screen is wide enough, otherwise wrap to 2 lines
    if (w >= guide_single_len + 4) {
      // Single line (default)
      int guide_x = (w - guide_single_len) / 2;
      render_guide_line(guide_y, guide_x, guide_single);
    } else {
      // Two lines (wrapped)
      int guide_x = (w - max_wrapped_len) / 2;
      render_guide_line(guide_y - 1, guide_x, guide_line1);
      render_guide_line(guide_y, guide_x, guide_line2);
    }

    int input_cursor_y = input_y + 1;
    int input_cursor_x = input_x + 3;
    move(input_cursor_y, input_cursor_x);

    // Force a complete screen refresh to ensure everything is displayed
    refresh();
    doupdate();

    int capacity = 16;
    int cursor_pos = 0;
    char *input = malloc(capacity);
    if (input == NULL) {
      endwin();
      return;
    }
    input[0] = '\0'; // Initialize to empty string

    // Flush any buffered input before starting
    flushinp();

    int ch;
    while (1) {
      ch = getch();

      if (ch == 10 || ch == KEY_ENTER) {
        if (cursor_pos == 1 && input[0] == 'f') {
          // Flashcard generator
          endwin();
          flashcard();
          free(input);
          break; // Break inner loop, will restart tools
        } else if (cursor_pos == 1 && input[0] == 'q') {
          // Quiz maker
          endwin();
          quiz();
          free(input);
          break; // Break inner loop, will restart tools
        } else if (cursor_pos == 1 && input[0] == 't') {
          // Todo list - pass NULL to read from session file
          endwin();
          todo(NULL); // NULL will make it read from session file
          free(input);
          break; // Break inner loop, will restart tools
        } else if (cursor_pos == 1 && input[0] == 'x') {
          // Exit and return to menu
          free(input);
          endwin();
          return;
        }
      // Clear input on Enter if not a recognized command or command not yet
      // implemented
      cursor_pos = 0;
      input[0] = '\0';
      // Clear the input line visually
      int clear_width = input_w - 4; // Account for prompt "> "
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

      int h, w;
      getmaxyx(stdscr, h, w);

      draw_status_bar(h, w, " Success v0.1.10 ", " in Tools ",
                      " Made with <3 ");
      int input_w = w - 10;
      int input_y = h - 7;
      int input_x = (w - input_w) / 2;
      render_input(stdscr, input_w, 3, input_y, input_x, ">");

      // Render tools title at the top
      WINDOW *tools_title_win =
          render_win(stdscr, 38, 6, 1, (w - 36) / 2, tools_title, 2, 13, false);
      // Default gray color (no highlight)
      wrefresh(tools_title_win);

      // Recalculate positions (start after title)
      int padding = 2;
      int text_width = input_w - (padding * 2);
      int current_y = 7; // Removed 1 space

      // Render description
      ColoredWord colored_words_desc[] = {{"AI", 15}, {"powered", 15}};
      int desc_height = calculate_text_height_local(desc, text_width) + 2;
      render_text_with_colors(stdscr, input_w, current_y, input_x, desc, 11, 12,
                              true, colored_words_desc, 2, 0);

      current_y += desc_height + 1;

      // Render Flashcard window
      ColoredWord colored_words_flashcard[] = {
          {"Flashcard", 15}, {"Generator", 15}, {"AI", 15}};
      int flashcard_height =
          calculate_text_height_local(flashcard_text, text_width) + 2;
      render_text_with_colors(stdscr, input_w, current_y, input_x,
                              flashcard_text, 14, 12, true,
                              colored_words_flashcard, 3, 15);

      current_y += flashcard_height + 1;

      // Render Quiz window
      ColoredWord colored_words_quiz[] = {
          {"Quiz", 16}, {"Maker", 16}, {"AI", 16}};
      int quiz_height = calculate_text_height_local(quiz_text, text_width) + 2;
      render_text_with_colors(stdscr, input_w, current_y, input_x, quiz_text,
                              14, 12, true, colored_words_quiz, 3, 16);

      current_y += quiz_height + 1;

      // Render Todo window
      ColoredWord colored_words_todo[] = {{"Todo", 17}, {"List", 17}};
      render_text_with_colors(stdscr, input_w, current_y, input_x, todo_text,
                              14, 12, true, colored_words_todo, 2, 17);

      // Selection guide
      int guide_y = input_y - 3;
      const char *guide_single =
          "[f] Flashcard Generator   [q] Quiz Maker   [t] Todo List   [x] Exit";
      const char *guide_line1 =
          "[f] Flashcard Generator            [t] Todo List       ";
      const char *guide_line2 =
          "[q] Quiz Maker                     [x] Exit            ";

      int guide_single_len = (int)strlen(guide_single);
      int guide_line1_len = (int)strlen(guide_line1);
      int guide_line2_len = (int)strlen(guide_line2);
      int max_wrapped_len =
          guide_line1_len > guide_line2_len ? guide_line1_len : guide_line2_len;

      if (w >= guide_single_len + 4) {
        int guide_x = (w - guide_single_len) / 2;
        render_guide_line(guide_y, guide_x, guide_single);
      } else {
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
    } else if ((ch == KEY_BACKSPACE || ch == 127 || ch == 8) &&
               cursor_pos > 0) {
      cursor_pos--;
      input[cursor_pos] = '\0'; // Null-terminate after backspace
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
      input[cursor_pos] = '\0'; // Null-terminate

      wattron(stdscr, COLOR_PAIR(1));
      mvwaddch(stdscr, input_cursor_y, input_cursor_x + cursor_pos - 1, ch);
      wattroff(stdscr, COLOR_PAIR(1));

      refresh();
    }
    } // Close inner while loop
    
    free(input);
    // Continue outer loop to return to tools (will restart with new input)
    continue;
  }
}
