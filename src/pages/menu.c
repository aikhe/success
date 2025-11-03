#include "menu.h"
#include "curses.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../features/ai_chat.h"
#include "../features/study_timer.h"
#include "../features/social_hall.h"
#include "tools.h"

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
    init_pair(19, ORANGE, GRAY_5); // Orange text on GRAY_5 background (for quiz selection)
    init_pair(20, GREEN, GRAY_5); // Green text on GRAY_5 background (for quiz confirmed answers)
  }
}

// Renamed to avoid conflict with introduction.c's static draw_status_bar
void draw_status_bar_menu(int h, int w, char *left, char *mid, char *right) {
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

// Wrapper function for render_input to avoid conflict with introduction.c's static render_input
void render_input_field(WINDOW *win, int w, int h, int y, int x, char *fieldname) {
  render_input(win, w, h, y, x, fieldname);
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

// Helper structure for colored text
typedef struct {
  const char *word;
  int color_pair;
} ColoredWord;

// Calculate height needed for wrapped text (with 2 padding each side)
int calculate_text_height(const char *text, int width) {
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

// Render text with wrapping, padding, and colored words
WINDOW *render_text_with_colors(WINDOW *win, int w, int y, int x,
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
  int text_h = calculate_text_height(clean, text_width);
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
  case 'c':
    return 15; // Purple for Chat
  case 's':
    return 16; // Green for Social Hall
  case 't':
    return 17; // Cyan for Tools
  case 'p':
    return 18; // Pink for Study Timer
  case 'x':
    return 4; // Orange for Exit
  default:
    return 5; // White default
  }
}

// Helper function to render a guide line with colored brackets
void render_guide_line(int y, int x, const char *text) {
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

void menu(void) {
  // Use a loop instead of recursion to avoid infinite recursion warning
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
  Welcome to SUCCESS! This is a TUI (Terminal User Interface) application where you can accelerate your learning, automate your study sessions, and improve your education journey. Enjoy and goodluck!
  )";

    const char *ai_chat_text = R"(
  SUCCESS AI Chatbot - Get instant answers, explanations, and study guidance powered by AI.
  )";

    const char *social_hall_text = R"(
  Social Hall - Access learning materials and updates directly from your teachers.
  )";

    const char *tools_text = R"(
  Tools - Instantly create quizzes and flashcards to boost your review sessions.
  )";

    const char *study_timer_text = R"(
  Smart Study Timer - Focus better with Pomodoro, Active Recall, and Spaced Repetition all in one timer.
  )";

    // initial display
    draw_status_bar_menu(h, w, " Success v0.1.10 ", " in Student Menu ",
                    " Made with <3 ");
    int input_w = w - 10;
    int input_y = h - 7;
    int input_x = (w - input_w) / 2;
    render_input(stdscr, input_w, 3, input_y, input_x, ">");

    WINDOW *success_woodmark_win = render_win(stdscr, 40, 10, 1, (w - 38) / 2,
                                              success_woodmark, 2, 13, false);
    // add highlight
    wattron(success_woodmark_win, COLOR_PAIR(5));
    mvwaddstr(success_woodmark_win, 1, 7, "█  █ ▄▀▀▀ ▄▀▀▀");
    mvwaddstr(success_woodmark_win, 2, 7, "█░░█ █░░░ █░░░");
    mvwaddstr(success_woodmark_win, 3, 8, "▀▀▀ ▀▀▀▀ ▀▀▀▀");
    wattroff(success_woodmark_win, COLOR_PAIR(5));
    wrefresh(success_woodmark_win);

    ColoredWord colored_words[] = {{"SUCCESS!", 14}};
    int padding = 2;
    int text_width = input_w - (padding * 2);
    int desc_height = calculate_text_height(desc, text_width) + 2;
    render_text_with_colors(stdscr, input_w, 7, input_x, desc, 11, 12, true,
                            colored_words, 1, 0);

    ColoredWord colored_words_ai_chat[] = {
        {"SUCCESS", 15}, {"AI", 15}, {"Chatbot", 15}};
    int current_y = 7 + desc_height + 1; // 1 space between windows
    int ai_chat_height = calculate_text_height(ai_chat_text, text_width) + 2;
    render_text_with_colors(stdscr, input_w, current_y, input_x, ai_chat_text,
                            14, 12, true, colored_words_ai_chat, 3,
                            15); // Purple

    current_y += ai_chat_height + 1;
    ColoredWord colored_words_social[] = {{"Social", 16}, {"Hall", 16}};
    int social_height = calculate_text_height(social_hall_text, text_width) + 2;
    render_text_with_colors(stdscr, input_w, current_y, input_x, social_hall_text,
                            14, 12, true, colored_words_social, 2, 16); // Green

    current_y += social_height + 1;
    ColoredWord colored_words_tools[] = {{"Tools", 17}};
    int tools_height = calculate_text_height(tools_text, text_width) + 2;
    render_text_with_colors(stdscr, input_w, current_y, input_x, tools_text, 14, 12,
                            true, colored_words_tools, 1, 17); // Cyan

    current_y += tools_height + 1;
    ColoredWord colored_words_timer[] = {
        {"Smart", 18}, {"Study", 18}, {"Timer", 18}};
    render_text_with_colors(stdscr, input_w, current_y, input_x,
                            study_timer_text, 14, 12, true, colored_words_timer,
                            3, 18); // Pink

    // Selection guide 2 lines up from input bar (wraps to 3 lines on small
    // screens)
    int guide_y = input_y - 3;
    const char *guide_single =
        "[c] Chat with SUCCESS AI   [s] Access Social "
        "Hall   [t] Open Tools   [p] Start Study Timer   [x] Exit";
    const char *guide_line1 =
        "[c] Chat with SUCCESS AI            [t] Open Tools       ";
    const char *guide_line2 =
        "[s] Access Social Hall              [p] Start Study Timer";
    const char *guide_line3 =
        "                       [x] Exit                          ";

    int guide_single_len = (int)strlen(guide_single);
    int guide_line1_len = (int)strlen(guide_line1);
    int guide_line2_len = (int)strlen(guide_line2);
    int guide_line3_len = (int)strlen(guide_line3);
    int max_wrapped_len =
        guide_line1_len > guide_line2_len ? guide_line1_len : guide_line2_len;
    if (guide_line3_len > max_wrapped_len) {
      max_wrapped_len = guide_line3_len;
    }

    // Use single line if screen is wide enough, otherwise wrap to 3 lines
    if (w >= guide_single_len + 4) {
      // Single line (default)
      int guide_x = (w - guide_single_len) / 2;
      render_guide_line(guide_y, guide_x, guide_single);
    } else {
      // Three lines (wrapped)
      int guide_x = (w - max_wrapped_len) / 2;
      render_guide_line(guide_y - 1, guide_x, guide_line1);
      render_guide_line(guide_y, guide_x, guide_line2);
      render_guide_line(guide_y + 1, guide_x, guide_line3);
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
        } else if (cursor_pos == 1 && input[0] == 'p') {
          endwin();

#ifdef _WIN32
          system("cls");
#else
          system("clear");
#endif

          study_timer();

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
        } else if (cursor_pos == 1 && input[0] == 't') {
          endwin();
          // Note: No need to clear screen - tools() will call initscr() which takes over
          tools();

          break;
          // } else if (cursor_pos == 1 && input[0] == 't') {
        } else if (cursor_pos == 1 && input[0] == 'x') {
          // User typed 'x' and pressed Enter - exit program completely
          endwin();
          free(input);
          exit(0);
        }
        // Clear input on Enter if not a recognized command
        cursor_pos = 0;
        input[0] = '\0';
        // Clear the input line visually (use input_w width)
        int input_w = getmaxx(stdscr) - 10;
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

        draw_status_bar_menu(h, w, " Success v0.1.10 ", " in Student Menu ",
                        " Made with <3 ");
        int input_w = w - 10;
        int input_y = h - 7;
        int input_x = (w - input_w) / 2;
        render_input(stdscr, input_w, 3, input_y, input_x, ">");

        WINDOW *success_woodmark_win = render_win(
            stdscr, 40, 10, 1, (w - 38) / 2, success_woodmark, 2, 13, false);
        // add highlight
        wattron(success_woodmark_win, COLOR_PAIR(5));
        mvwaddstr(success_woodmark_win, 1, 7, "█  █ ▄▀▀▀ ▄▀▀▀");
        mvwaddstr(success_woodmark_win, 2, 7, "█░░█ █░░░ █░░░");
        mvwaddstr(success_woodmark_win, 3, 8, "▀▀▀ ▀▀▀▀ ▀▀▀▀");
        wattroff(success_woodmark_win, COLOR_PAIR(5));
        wrefresh(success_woodmark_win);

        ColoredWord colored_words[] = {{"SUCCESS!", 14}};
        int padding = 2;
        int text_width = input_w - (padding * 2);
        int desc_height = calculate_text_height(desc, text_width) + 2;
        render_text_with_colors(stdscr, input_w, 7, input_x, desc, 11, 12, true,
                                colored_words, 1, 0);

        ColoredWord colored_words_ai_chat[] = {
            {"SUCCESS", 15}, {"AI", 15}, {"Chatbot", 15}};
        int current_y = 7 + desc_height + 1; // 1 space between windows
        int ai_chat_height =
            calculate_text_height(ai_chat_text, text_width) + 2;
        render_text_with_colors(stdscr, input_w, current_y, input_x,
                                ai_chat_text, 14, 12, true,
                                colored_words_ai_chat, 3,
                                15); // Purple

        current_y += ai_chat_height + 1;
        ColoredWord colored_words_social[] = {{"Social", 16}, {"Hall", 16}};
        int social_height = calculate_text_height(social_hall_text, text_width) + 2;
        render_text_with_colors(stdscr, input_w, current_y, input_x,
                                social_hall_text, 14, 12, true, colored_words_social,
                                2,
                                16); // Green

        current_y += social_height + 1;
        ColoredWord colored_words_tools[] = {{"Tools", 17}};
        int tools_height = calculate_text_height(tools_text, text_width) + 2;
        render_text_with_colors(stdscr, input_w, current_y, input_x, tools_text, 14,
                                12, true, colored_words_tools, 1, 17); // Cyan

        current_y += tools_height + 1;
        ColoredWord colored_words_timer[] = {
            {"Smart", 18}, {"Study", 18}, {"Timer", 18}};
        render_text_with_colors(stdscr, input_w, current_y, input_x,
                                study_timer_text, 14, 12, true,
                                colored_words_timer, 3, 18); // Pink

        // Selection guide 2 lines up from input bar (wraps to 3 lines on small
        // screens)
        int guide_y = input_y - 3;
        const char *guide_single =
            "[c] Chat with SUCCESS AI   [s] Access Social "
            "Hall   [t] Open Tools   [p] Start Study Timer   [x] Exit";
        const char *guide_line1 =
            "[c] Chat with SUCCESS AI            [t] Open Tools       ";
        const char *guide_line2 =
            "[s] Access Social Hall              [p] Start Study Timer";
        const char *guide_line3 =
            "                       [x] Exit                          ";

        int guide_single_len = (int)strlen(guide_single);
        int guide_line1_len = (int)strlen(guide_line1);
        int guide_line2_len = (int)strlen(guide_line2);
        int guide_line3_len = (int)strlen(guide_line3);
        int max_wrapped_len = guide_line1_len > guide_line2_len
                                  ? guide_line1_len
                                  : guide_line2_len;
        if (guide_line3_len > max_wrapped_len) {
          max_wrapped_len = guide_line3_len;
        }

        // Use single line if screen is wide enough, otherwise wrap to 3 lines
        if (w >= guide_single_len + 4) {
          // Single line (default)
          int guide_x = (w - guide_single_len) / 2;
          render_guide_line(guide_y, guide_x, guide_single);
        } else {
          // Three lines (wrapped)
          int guide_x = (w - max_wrapped_len) / 2;
          render_guide_line(guide_y - 1, guide_x, guide_line1);
          render_guide_line(guide_y, guide_x, guide_line2);
          render_guide_line(guide_y + 1, guide_x, guide_line3);
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

        // mvprintw(10, 10, "%d", cursor_pos);
        refresh();
      }
    }

    // If we break from the inner loop, it means ai_chat() returned
    // Free input and continue to redraw menu (outer loop will restart)
    free(input);

    // Continue loop to return to menu (instead of recursion)
    continue;
  }
}
