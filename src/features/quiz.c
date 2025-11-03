#include "quiz.h"
#include "../pages/menu.h"
#include "curses.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __declspec
#define __declspec(x)
#endif

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <locale.h>
#include <nfd.h>
#include <time.h>

#include "../gemini_api/gemini_request.h"
#include "../gemini_api/get_file_uri.h"
#include "../gemini_api/get_upload_url.h"

#include "../pages/tools.h"
#include "../utils/gemini_loading.h"
#include "../utils/get_file_mime_type.h"
#include "../utils/read_file.h"
#include "../utils/read_file_b64.h"

// Forward declarations
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

// Quiz question structure
typedef struct {
  int number;
  char *question;
  char *option_a;
  char *option_b;
  char *option_c;
  char correct_answer; // 'A', 'B', or 'C'
  char user_answer;    // 'A', 'B', 'C', or '\0' if not answered
} QuizQuestion;

static QuizQuestion *quiz_questions = NULL;
static int quiz_count = 0;
static char *quiz_title = NULL;

// Helper structure for colored text
typedef struct {
  const char *word;
  int color_pair;
} ColoredWord;

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

// Helper function to render a guide line
static void render_guide_line(int y, int x, const char *text) {
  wattron(stdscr, COLOR_PAIR(2));
  mvaddstr(y, x, text);
  wattroff(stdscr, COLOR_PAIR(2));
}

// Parse quiz response from Gemini
static int parse_quiz_response(const char *response) {
  // Free existing quiz data
  if (quiz_questions) {
    for (int i = 0; i < quiz_count; i++) {
      free(quiz_questions[i].question);
      free(quiz_questions[i].option_a);
      free(quiz_questions[i].option_b);
      free(quiz_questions[i].option_c);
    }
    free(quiz_questions);
    quiz_questions = NULL;
  }
  if (quiz_title) {
    free(quiz_title);
    quiz_title = NULL;
  }
  quiz_count = 0;

  // Simple parsing - look for patterns like:
  // Question number, question text, options A/B/C, and correct answer indicator

  // Try to extract title (first line or look for "Title:" pattern)
  const char *p = response;
  char title_buf[512] = {0};
  const char *title_marker = strstr(response, "Title:");
  if (title_marker) {
    const char *title_start = title_marker + 6;
    while (*title_start == ' ' || *title_start == '\t')
      title_start++;
    const char *title_end = title_start;
    while (*title_end && *title_end != '\n')
      title_end++;
    int title_len = title_end - title_start;
    if (title_len > 0 && title_len < 511) {
      strncpy(title_buf, title_start, title_len);
      title_buf[title_len] = '\0';
      quiz_title = strdup(title_buf);
    }
  } else {
    // Use first line as title if no title marker
    const char *first_line = response;
    while (*first_line == ' ' || *first_line == '\t' || *first_line == '\n' ||
           *first_line == '\r')
      first_line++;
    const char *first_line_end = first_line;
    while (*first_line_end && *first_line_end != '\n' &&
           *first_line_end != '\r')
      first_line_end++;
    int first_line_len = first_line_end - first_line;
    if (first_line_len > 0 && first_line_len < 511) {
      strncpy(title_buf, first_line, first_line_len);
      title_buf[first_line_len] = '\0';
      quiz_title = strdup(title_buf);
    } else {
      quiz_title = strdup("Quiz");
    }
  }

  // Parse questions - look for numbered questions
  char *response_copy = strdup(response);
  char *line = strtok(response_copy, "\n");
  int current_q = -1;

  while (line != NULL) {
    // Only stop creating new questions after 20, but continue parsing options
    // for the last question
    bool can_create_new = quiz_count < 20;
    // Skip empty lines
    while (*line == ' ' || *line == '\t')
      line++;
    if (*line == '\0') {
      line = strtok(NULL, "\n");
      continue;
    }

    // Check if this is a question number (e.g., "1.", "2.", etc.)
    char *num_start = line;
    while (*num_start == ' ' || *num_start == '\t')
      num_start++;

    if (*num_start >= '0' && *num_start <= '9' && can_create_new) {
      // New question
      current_q = quiz_count;
      quiz_questions =
          realloc(quiz_questions, (quiz_count + 1) * sizeof(QuizQuestion));
      quiz_questions[current_q].number = current_q + 1;
      quiz_questions[current_q].question = NULL;
      quiz_questions[current_q].option_a = NULL;
      quiz_questions[current_q].option_b = NULL;
      quiz_questions[current_q].option_c = NULL;
      quiz_questions[current_q].correct_answer = '\0';
      quiz_questions[current_q].user_answer = '\0';

      // Extract question text (after number and period)
      char *q_start = num_start;
      while (*q_start && *q_start != '.' && *q_start != ' ')
        q_start++;
      if (*q_start == '.')
        q_start++;
      while (*q_start == ' ' || *q_start == '\t')
        q_start++;

      // Read until we find "A." or similar
      char q_text[512] = {0};
      char *q_end = strstr(q_start, "A.");
      if (!q_end)
        q_end = strstr(q_start, "a.");
      if (q_end) {
        int q_len = q_end - q_start;
        while (q_len > 0 &&
               (q_start[q_len - 1] == ' ' || q_start[q_len - 1] == '\t'))
          q_len--;
        if (q_len > 0 && q_len < 511) {
          strncpy(q_text, q_start, q_len);
          q_text[q_len] = '\0';
          quiz_questions[current_q].question = strdup(q_text);
        }
      } else {
        // No A. found, use rest of line
        int q_len = strlen(q_start);
        if (q_len > 0 && q_len < 511) {
          strncpy(q_text, q_start, q_len);
          quiz_questions[current_q].question = strdup(q_text);
        }
      }

      quiz_count++;
    } else if (current_q >= 0 && quiz_questions[current_q].question != NULL) {
      // Look for options A, B, C
      char *a_marker = strstr(line, "A.");
      char *b_marker = strstr(line, "B.");
      char *c_marker = strstr(line, "C.");
      char *a_marker_lower = strstr(line, "a.");
      char *b_marker_lower = strstr(line, "b.");
      char *c_marker_lower = strstr(line, "c.");

      if (!a_marker)
        a_marker = a_marker_lower;
      if (!b_marker)
        b_marker = b_marker_lower;
      if (!c_marker)
        c_marker = c_marker_lower;

      // Extract option A
      if (a_marker && !quiz_questions[current_q].option_a) {
        char *opt_start = a_marker + 2;
        while (*opt_start == ' ' || *opt_start == '\t')
          opt_start++;
        // Skip opening bracket if present
        if (*opt_start == '[')
          opt_start++;
        while (*opt_start == ' ' || *opt_start == '\t')
          opt_start++;

        char *opt_end =
            b_marker ? b_marker : (c_marker ? c_marker : line + strlen(line));
        while (
            opt_end > opt_start &&
            (opt_end[-1] == ' ' || opt_end[-1] == '\t' || opt_end[-1] == ']'))
          opt_end--;

        int opt_len = opt_end - opt_start;
        if (opt_len > 0) {
          char opt_buf[256];
          strncpy(opt_buf, opt_start, opt_len);
          opt_buf[opt_len] = '\0';
          // Remove any trailing brackets
          int len = strlen(opt_buf);
          while (len > 0 && opt_buf[len - 1] == ']')
            opt_buf[--len] = '\0';
          quiz_questions[current_q].option_a = strdup(opt_buf);
        }
      }

      // Extract option B
      if (b_marker && !quiz_questions[current_q].option_b) {
        char *opt_start = b_marker + 2;
        while (*opt_start == ' ' || *opt_start == '\t')
          opt_start++;
        // Skip opening bracket if present
        if (*opt_start == '[')
          opt_start++;
        while (*opt_start == ' ' || *opt_start == '\t')
          opt_start++;

        char *opt_end = c_marker ? c_marker : line + strlen(line);
        while (
            opt_end > opt_start &&
            (opt_end[-1] == ' ' || opt_end[-1] == '\t' || opt_end[-1] == ']'))
          opt_end--;

        int opt_len = opt_end - opt_start;
        if (opt_len > 0) {
          char opt_buf[256];
          strncpy(opt_buf, opt_start, opt_len);
          opt_buf[opt_len] = '\0';
          // Remove any trailing brackets
          int len = strlen(opt_buf);
          while (len > 0 && opt_buf[len - 1] == ']')
            opt_buf[--len] = '\0';
          quiz_questions[current_q].option_b = strdup(opt_buf);
        }
      }

      // Extract option C
      if (c_marker && !quiz_questions[current_q].option_c) {
        char *opt_start = c_marker + 2;
        while (*opt_start == ' ' || *opt_start == '\t')
          opt_start++;
        // Skip opening bracket if present
        if (*opt_start == '[')
          opt_start++;
        while (*opt_start == ' ' || *opt_start == '\t')
          opt_start++;

        char *opt_end = line + strlen(line);
        while (
            opt_end > opt_start &&
            (opt_end[-1] == ' ' || opt_end[-1] == '\t' || opt_end[-1] == ']'))
          opt_end--;

        int opt_len = opt_end - opt_start;
        if (opt_len > 0) {
          char opt_buf[256];
          strncpy(opt_buf, opt_start, opt_len);
          opt_buf[opt_len] = '\0';
          // Remove any trailing brackets
          int len = strlen(opt_buf);
          while (len > 0 && opt_buf[len - 1] == ']')
            opt_buf[--len] = '\0';
          quiz_questions[current_q].option_c = strdup(opt_buf);
        }
      }

      // Look for correct answer indicator
      // Check for full format like "[ A. text ]" or "[B. text ]"
      char *bracket_a_full = strstr(line, "[ A.");
      if (!bracket_a_full)
        bracket_a_full = strstr(line, "[A.");
      if (!bracket_a_full)
        bracket_a_full = strstr(line, "[ a.");
      if (!bracket_a_full)
        bracket_a_full = strstr(line, "[a.");

      char *bracket_b_full = strstr(line, "[ B.");
      if (!bracket_b_full)
        bracket_b_full = strstr(line, "[B.");
      if (!bracket_b_full)
        bracket_b_full = strstr(line, "[ b.");
      if (!bracket_b_full)
        bracket_b_full = strstr(line, "[b.");

      char *bracket_c_full = strstr(line, "[ C.");
      if (!bracket_c_full)
        bracket_c_full = strstr(line, "[C.");
      if (!bracket_c_full)
        bracket_c_full = strstr(line, "[ c.");
      if (!bracket_c_full)
        bracket_c_full = strstr(line, "[c.");

      // Also check for simple [A], [B], [C]
      char *bracket_a = strstr(line, "[A]");
      char *bracket_b = strstr(line, "[B]");
      char *bracket_c = strstr(line, "[C]");

      char *correct_marker = strstr(line, "correct:");
      char *correct_marker_upper = strstr(line, "Correct:");

      if (!correct_marker)
        correct_marker = correct_marker_upper;

      if (bracket_a_full && !quiz_questions[current_q].correct_answer)
        quiz_questions[current_q].correct_answer = 'A';
      else if (bracket_b_full && !quiz_questions[current_q].correct_answer)
        quiz_questions[current_q].correct_answer = 'B';
      else if (bracket_c_full && !quiz_questions[current_q].correct_answer)
        quiz_questions[current_q].correct_answer = 'C';
      else if (bracket_a && !quiz_questions[current_q].correct_answer)
        quiz_questions[current_q].correct_answer = 'A';
      else if (bracket_b && !quiz_questions[current_q].correct_answer)
        quiz_questions[current_q].correct_answer = 'B';
      else if (bracket_c && !quiz_questions[current_q].correct_answer)
        quiz_questions[current_q].correct_answer = 'C';
      else if (correct_marker && !quiz_questions[current_q].correct_answer) {
        char *ans_start = correct_marker + 8;
        while (*ans_start == ' ' || *ans_start == '\t')
          ans_start++;
        if (*ans_start == 'A' || *ans_start == 'a')
          quiz_questions[current_q].correct_answer = 'A';
        else if (*ans_start == 'B' || *ans_start == 'b')
          quiz_questions[current_q].correct_answer = 'B';
        else if (*ans_start == 'C' || *ans_start == 'c')
          quiz_questions[current_q].correct_answer = 'C';
      }
    }

    line = strtok(NULL, "\n");
  }

  free(response_copy);

  // Set default correct answers if not found
  for (int i = 0; i < quiz_count; i++) {
    if (quiz_questions[i].correct_answer == '\0') {
      quiz_questions[i].correct_answer = 'A'; // Default
    }
  }

  // Scramble correct answers (shuffle options A, B, C)
  // IMPORTANT: Preserve the correct answer text by storing it before shuffling
  srand((unsigned int)time(NULL));
  for (int i = 0; i < quiz_count; i++) {
    char correct_orig = quiz_questions[i].correct_answer;

    // Store the TEXT of the correct answer (not just pointer)
    char *correct_text_str = NULL;
    if (correct_orig == 'A' && quiz_questions[i].option_a) {
      correct_text_str = strdup(quiz_questions[i].option_a);
    } else if (correct_orig == 'B' && quiz_questions[i].option_b) {
      correct_text_str = strdup(quiz_questions[i].option_b);
    } else if (correct_orig == 'C' && quiz_questions[i].option_c) {
      correct_text_str = strdup(quiz_questions[i].option_c);
    }

    // Store all option texts (make copies)
    char *opt_a_text =
        quiz_questions[i].option_a ? strdup(quiz_questions[i].option_a) : NULL;
    char *opt_b_text =
        quiz_questions[i].option_b ? strdup(quiz_questions[i].option_b) : NULL;
    char *opt_c_text =
        quiz_questions[i].option_c ? strdup(quiz_questions[i].option_c) : NULL;

    // Free original pointers if they exist
    if (quiz_questions[i].option_a)
      free(quiz_questions[i].option_a);
    if (quiz_questions[i].option_b)
      free(quiz_questions[i].option_b);
    if (quiz_questions[i].option_c)
      free(quiz_questions[i].option_c);

    // Create array of option texts for shuffling
    char *options[3] = {opt_a_text, opt_b_text, opt_c_text};

    // Shuffle the options using Fisher-Yates
    for (int j = 2; j > 0; j--) {
      int k = rand() % (j + 1);
      char *temp = options[j];
      options[j] = options[k];
      options[k] = temp;
    }

    // Find where the correct answer text is now (compare strings, not pointers)
    char new_correct = 'A';
    if (correct_text_str) {
      for (int j = 0; j < 3; j++) {
        if (options[j] && strcmp(options[j], correct_text_str) == 0) {
          new_correct = 'A' + j;
          break;
        }
      }
      free(correct_text_str);
    }

    // Update question with shuffled options
    quiz_questions[i].option_a = options[0];
    quiz_questions[i].option_b = options[1];
    quiz_questions[i].option_c = options[2];
    quiz_questions[i].correct_answer = new_correct;
  }

  return quiz_count;
}

// Display quiz generation input screen
static bool get_quiz_input(char *topic, size_t topic_len, char ***file_uris_out,
                           char ***exts_out, int *file_count_out,
                           cJSON *gemini_api_key, cJSON *gemini_file_url) {
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

  draw_status_bar(h, w, " Success v0.1.10 ", " in Quiz Maker ",
                  " Made with <3 ");

  const char *title_text = "QUIZ MAKER";
  int title_x = (w - (int)strlen(title_text)) / 2;
  wattron(stdscr, COLOR_PAIR(3));
  mvaddstr(2, title_x, title_text);
  wattroff(stdscr, COLOR_PAIR(3));

  const char *desc_text = R"(
  Generate comprehensive quizzes with AI. Enter a topic or attach files to create custom quiz questions.
  )";

  int input_w = w - 10;
  int input_x = (w - input_w) / 2;
  int padding = 2;
  int text_width = input_w - (padding * 2);
  int desc_y = 4; // 2 spaces below title (title at 2, so 2+2=4)
  int desc_height = calculate_text_height_local(desc_text, text_width) + 2;

  ColoredWord colored_words[] = {{"AI", 15}};
  render_text_with_colors(stdscr, input_w, desc_y, input_x, desc_text, 11, 12,
                          true, colored_words, 1, 12);

  // Input at bottom like menu page
  int input_y = h - 7;
  int input_cursor_y = input_y + 1;
  int input_cursor_x = input_x + 15; // After "Topic/Prompt: "

  // Guide above input
  const char *guide_text = "[f] Attach Files   [Enter] Generate   [F2] Exit";
  int guide_y = input_y - 3;
  int guide_x = (w - (int)strlen(guide_text)) / 2;

  int capacity = 256;
  int cursor_pos = 0;
  char *input = malloc(capacity);
  input[0] = '\0';

  nfdresult_t nfd_res = NFD_CANCEL;
  nfdpathset_t pathSet = {0};
  char **file_uris = NULL;
  char **exts = NULL;
  int total_file_num = 0;
  char **file_names = NULL; // Store file names to display

  // Initial render
  render_input(stdscr, input_w, 3, input_y, input_x, "Topic/Prompt: ");
  render_guide_line(guide_y, guide_x, guide_text);
  move(input_cursor_y, input_cursor_x);
  refresh();

  int ch;
  while (1) {
    // Re-render everything on each loop
    clear();
    getmaxyx(stdscr, h, w);

    draw_status_bar(h, w, " Success v0.1.10 ", " in Quiz Maker ",
                    " Made with <3 ");

    title_x = (w - (int)strlen(title_text)) / 2;
    wattron(stdscr, COLOR_PAIR(3));
    mvaddstr(2, title_x, title_text);
    wattroff(stdscr, COLOR_PAIR(3));

    input_w = w - 10;
    input_x = (w - input_w) / 2;
    text_width = input_w - (padding * 2);
    desc_y = 4;
    desc_height = calculate_text_height_local(desc_text, text_width) + 2;

    render_text_with_colors(stdscr, input_w, desc_y, input_x, desc_text, 11, 12,
                            true, colored_words, 1, 12);

    input_y = h - 7;
    input_cursor_y = input_y + 1;
    input_cursor_x = input_x + 15; // After "Topic/Prompt: "
    guide_y = input_y - 3;
    guide_x = (w - (int)strlen(guide_text)) / 2;

    // Don't show filename
    render_input(stdscr, input_w, 3, input_y, input_x, "Topic/Prompt: ");

    render_guide_line(guide_y, guide_x, guide_text);

    // Show input text
    if (cursor_pos > 0) {
      wattron(stdscr, COLOR_PAIR(1));
      for (int i = 0; i < cursor_pos; i++) {
        mvwaddch(stdscr, input_cursor_y, input_cursor_x + i, input[i]);
      }
      wattroff(stdscr, COLOR_PAIR(1));
    }
    move(input_cursor_y, input_cursor_x + cursor_pos);

    refresh();

    ch = getch();

    if (ch == KEY_RESIZE) {
      resize_term(0, 0);
      continue;
    }

    if (ch == 10 || ch == KEY_ENTER) {
      if (strlen(input) > 0 || total_file_num > 0) {
        // Valid input - copy topic and return
        strncpy(topic, input, topic_len - 1);
        topic[topic_len - 1] = '\0';
        *file_uris_out = file_uris;
        *exts_out = exts;
        *file_count_out = total_file_num;
        if (file_names) {
          for (int i = 0; i < total_file_num; i++) {
            if (file_names[i])
              free(file_names[i]);
          }
          free(file_names);
        }
        free(input);
        endwin();
        return true;
      }
    } else if (ch == KEY_F(2)) {
      // Exit without generating
      free(input);
      if (file_uris) {
        for (int i = 0; i < total_file_num; i++) {
          free(file_uris[i]);
        }
        free(file_uris);
      }
      if (exts)
        free(exts);
      if (file_names) {
        for (int i = 0; i < total_file_num; i++) {
          if (file_names[i])
            free(file_names[i]);
        }
        free(file_names);
      }
      NFD_PathSet_Free(&pathSet);
      endwin();
      return false;
    } else if (ch == 'f' || ch == 'F') {
      // Attach files
      endwin();
      nfd_res = NFD_OpenDialogMultiple("png,jpeg,jpg,pdf", NULL, &pathSet);

      // Reinitialize ncurses after file dialog
      initscr();
      cbreak();
      noecho();
      keypad(stdscr, TRUE);
      curs_set(1);
      define_colors();
      wbkgd(stdscr, COLOR_PAIR(5));

      if (nfd_res == NFD_OKAY) {
        int selected_count = NFD_PathSet_GetCount(&pathSet);
        int file_index = 0;

        // Allocate space for file names
        if (file_names) {
          for (int i = 0; i < total_file_num; i++) {
            if (file_names[i])
              free(file_names[i]);
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

          // Extract filename from path
          const char *filename = strrchr(path, '/');
          if (!filename)
            filename = strrchr(path, '\\');
          if (!filename)
            filename = path;
          else
            filename++;

          if (file_names && file_index < selected_count) {
            file_names[file_index] = strdup(filename);
          }

          size_t encoded_len;
          unsigned char *file_data = read_file_b64(path, &encoded_len);
          const char *ext = get_file_mime_type(path);

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

          if (res_upload_url)
            free(res_upload_url);

          if (res_file_uri) {
            int new_capacity = (total_file_num == 0) ? 1 : (total_file_num + 1);
            char **new_ext = realloc(exts, new_capacity * sizeof(char *));
            char **new_file_uris =
                realloc(file_uris, new_capacity * sizeof(char *));
            exts = new_ext;
            file_uris = new_file_uris;

            exts[total_file_num] = (char *)ext;
            file_uris[total_file_num] = res_file_uri;
            total_file_num++;
            file_index++;
          }

          free(file_data);
        }
      }
    } else if ((ch == KEY_BACKSPACE || ch == 127 || ch == 8) &&
               cursor_pos > 0) {
      cursor_pos--;
      input[cursor_pos] = '\0';
    } else if (ch >= 32 && ch <= 126) {
      if (cursor_pos + 1 >= capacity) {
        capacity *= 2;
        char *new_buf = realloc(input, capacity);
        input = new_buf;
      }
      input[cursor_pos++] = (char)ch;
      input[cursor_pos] = '\0';
    }
  }
}

// Display quiz questions screen
static void display_quiz(int current_page, int selected_q, int answering_q,
                         char selected_option) {
  int h, w;
  getmaxyx(stdscr, h, w);

  clear();

  draw_status_bar(h, w, " Success v0.1.10 ", " in Quiz ", " Made with <3 ");

  // Display title
  if (quiz_title) {
    int title_x = (w - (int)strlen(quiz_title)) / 2;
    wattron(stdscr, COLOR_PAIR(3));
    mvaddstr(2, title_x, quiz_title);
    wattroff(stdscr, COLOR_PAIR(3));
  }

  // Calculate display area - centered and responsive
  int input_w = w - 10;
  int input_x = (w - input_w) / 2;
  int padding_h = 1; // 1 padding top/bottom
  int padding_w = 2; // 2 padding left/right
  int text_width = input_w - (padding_w * 2);

  int start_q = current_page * 5;
  int end_q = start_q + 5;
  if (end_q > quiz_count)
    end_q = quiz_count;

  int current_y = 4;

  // Display questions for current page using description window style
  for (int i = start_q; i < end_q; i++) {
    // Determine which option is currently selected
    char current_selected = '\0';
    if (i == answering_q) {
      current_selected = selected_option; // Orange when selecting
    } else if (quiz_questions[i].user_answer != '\0') {
      current_selected = quiz_questions[i].user_answer; // Blue when confirmed
    }

    // Build question text (just the question, options will be rendered
    // separately)
    char q_text[512];
    snprintf(q_text, sizeof(q_text), "%d. %s", quiz_questions[i].number,
             quiz_questions[i].question ? quiz_questions[i].question : "");

    // Calculate height: question text + 1 space + 3 options
    int q_text_height = calculate_text_height_local(q_text, text_width);
    int total_q_height =
        padding_h * 2 + q_text_height + 1 +
        3; // 1 top/bottom padding + question + 1 space + 3 options

    // Create window manually with description window style
    WINDOW *win_bars =
        newwin(total_q_height, input_w + 2, current_y, input_x - 1);
    WINDOW *q_win = newwin(total_q_height, input_w, current_y, input_x);
    wbkgd(win_bars, COLOR_PAIR(12));
    wbkgd(q_win, COLOR_PAIR(11));

    // Draw borders
    cchar_t vbar;
    setcchar(&vbar, L"┃", 0, 0, NULL);
    for (int j = 0; j < total_q_height; j++) {
      mvwadd_wch(win_bars, j, 0, &vbar);
      mvwadd_wch(win_bars, j, input_w + 2 - 1, &vbar);
    }

    wrefresh(win_bars);

    // Color the question number and text (with GRAY_5 background like window)
    int q_color =
        (i == selected_q)
            ? 14
            : 11; // White on GRAY_5 if selected, gray on GRAY_5 otherwise
    wattron(q_win, COLOR_PAIR(q_color));
    mvwaddstr(q_win, padding_h, padding_w, q_text);
    wattroff(q_win, COLOR_PAIR(q_color));

    // Render options on separate lines (1 space after question, then options)
    int opt_y = padding_h + q_text_height + 1; // After question text + 1 space

    // Option A
    char opt_a_line[256];
    bool is_selected_a = (current_selected == 'A');
    if (is_selected_a && i == answering_q) {
      // When selecting (orange), don't add [ bracket at beginning
      snprintf(opt_a_line, sizeof(opt_a_line), "A. %s ]",
               quiz_questions[i].option_a ? quiz_questions[i].option_a : "");
    } else if (is_selected_a) {
      // When confirmed (green), remove left bracket
      snprintf(opt_a_line, sizeof(opt_a_line), "A. %s ]",
               quiz_questions[i].option_a ? quiz_questions[i].option_a : "");
    } else {
      snprintf(opt_a_line, sizeof(opt_a_line), "A. %s",
               quiz_questions[i].option_a ? quiz_questions[i].option_a : "");
    }
    // Trim any trailing brackets from option text before displaying
    char opt_a_clean[256];
    strncpy(opt_a_clean, opt_a_line, sizeof(opt_a_clean) - 1);
    opt_a_clean[sizeof(opt_a_clean) - 1] = '\0';
    int len_a = strlen(opt_a_clean);
    while (len_a > 0 && opt_a_clean[len_a - 1] == ']' &&
           opt_a_clean[len_a - 2] != '[') {
      opt_a_clean[--len_a] = '\0';
    }

    if (is_selected_a && i == answering_q) {
      // Orange text only, use window's background (GRAY_5)
      wattron(q_win, COLOR_PAIR(19)); // Orange on GRAY_5 background
      mvwaddstr(q_win, opt_y, padding_w, opt_a_clean);
      wattroff(q_win, COLOR_PAIR(19));
    } else if (is_selected_a && quiz_questions[i].user_answer == 'A') {
      wattron(q_win,
              COLOR_PAIR(20)); // Green when confirmed (on GRAY_5 background)
      mvwaddstr(q_win, opt_y, padding_w, opt_a_clean);
      wattroff(q_win, COLOR_PAIR(20));
    } else {
      mvwaddstr(q_win, opt_y, padding_w, opt_a_clean);
    }

    opt_y += 1; // No spacing between options

    // Option B
    char opt_b_line[256];
    bool is_selected_b = (current_selected == 'B');
    if (is_selected_b && i == answering_q) {
      // When selecting (orange), don't add [ bracket at beginning
      snprintf(opt_b_line, sizeof(opt_b_line), "B. %s ]",
               quiz_questions[i].option_b ? quiz_questions[i].option_b : "");
    } else if (is_selected_b) {
      // When confirmed (green), remove left bracket
      snprintf(opt_b_line, sizeof(opt_b_line), "B. %s ]",
               quiz_questions[i].option_b ? quiz_questions[i].option_b : "");
    } else {
      snprintf(opt_b_line, sizeof(opt_b_line), "B. %s",
               quiz_questions[i].option_b ? quiz_questions[i].option_b : "");
    }

    // Trim any trailing brackets
    char opt_b_clean[256];
    strncpy(opt_b_clean, opt_b_line, sizeof(opt_b_clean) - 1);
    opt_b_clean[sizeof(opt_b_clean) - 1] = '\0';
    int len_b = strlen(opt_b_clean);
    while (len_b > 0 && opt_b_clean[len_b - 1] == ']' &&
           opt_b_clean[len_b - 2] != '[') {
      opt_b_clean[--len_b] = '\0';
    }

    if (is_selected_b && i == answering_q) {
      wattron(q_win,
              COLOR_PAIR(19)); // Orange when selecting (on GRAY_5 background)
      mvwaddstr(q_win, opt_y, padding_w, opt_b_clean);
      wattroff(q_win, COLOR_PAIR(19));
    } else if (is_selected_b && quiz_questions[i].user_answer == 'B') {
      wattron(q_win,
              COLOR_PAIR(20)); // Green when confirmed (on GRAY_5 background)
      mvwaddstr(q_win, opt_y, padding_w, opt_b_clean);
      wattroff(q_win, COLOR_PAIR(20));
    } else {
      mvwaddstr(q_win, opt_y, padding_w, opt_b_clean);
    }

    opt_y += 1; // No spacing between options

    // Option C
    char opt_c_line[256];
    bool is_selected_c = (current_selected == 'C');
    if (is_selected_c && i == answering_q) {
      // When selecting (orange), don't add [ bracket at beginning
      snprintf(opt_c_line, sizeof(opt_c_line), "C. %s ]",
               quiz_questions[i].option_c ? quiz_questions[i].option_c : "");
    } else if (is_selected_c) {
      // When confirmed (green), remove left bracket
      snprintf(opt_c_line, sizeof(opt_c_line), "C. %s ]",
               quiz_questions[i].option_c ? quiz_questions[i].option_c : "");
    } else {
      snprintf(opt_c_line, sizeof(opt_c_line), "C. %s",
               quiz_questions[i].option_c ? quiz_questions[i].option_c : "");
    }

    // Trim any trailing brackets
    char opt_c_clean[256];
    strncpy(opt_c_clean, opt_c_line, sizeof(opt_c_clean) - 1);
    opt_c_clean[sizeof(opt_c_clean) - 1] = '\0';
    int len_c = strlen(opt_c_clean);
    while (len_c > 0 && opt_c_clean[len_c - 1] == ']' &&
           opt_c_clean[len_c - 2] != '[') {
      opt_c_clean[--len_c] = '\0';
    }

    if (is_selected_c && i == answering_q) {
      wattron(q_win,
              COLOR_PAIR(19)); // Orange when selecting (on GRAY_5 background)
      mvwaddstr(q_win, opt_y, padding_w, opt_c_clean);
      wattroff(q_win, COLOR_PAIR(19));
    } else if (is_selected_c && quiz_questions[i].user_answer == 'C') {
      wattron(q_win,
              COLOR_PAIR(20)); // Green when confirmed (on GRAY_5 background)
      mvwaddstr(q_win, opt_y, padding_w, opt_c_clean);
      wattroff(q_win, COLOR_PAIR(20));
    } else {
      mvwaddstr(q_win, opt_y, padding_w, opt_c_clean);
    }

    wrefresh(q_win);

    current_y += total_q_height + 1; // One space after each question
  }

  // Page indicator (2 lines above guides)
  int page_y = h - 6;
  char page_text[64];
  snprintf(page_text, sizeof(page_text), "Page %d/%d", current_page + 1,
           (quiz_count + 4) / 5);
  int page_x = (w - (int)strlen(page_text)) / 2;
  wattron(stdscr, COLOR_PAIR(2));
  mvaddstr(page_y, page_x, page_text);
  wattroff(stdscr, COLOR_PAIR(2));

  // Guide
  const char *guide_text;
  if (answering_q >= 0) {
    guide_text = "[↑↓] Select Option   [Enter] Confirm   [ESC] Cancel";
  } else {
    guide_text = "[↑↓] Navigate   [←→] Change Page   [Enter] Answer   [s] "
                 "Submit   [x] Exit";
  }
  int guide_y = h - 4;
  int guide_x = (w - (int)strlen(guide_text)) / 2;
  render_guide_line(guide_y, guide_x, guide_text);

  refresh();
}

// Display results screen
static void display_results(void) {
  int correct = 0;
  for (int i = 0; i < quiz_count; i++) {
    if (quiz_questions[i].user_answer == quiz_questions[i].correct_answer) {
      correct++;
    }
  }

  int h, w;
  getmaxyx(stdscr, h, w);

  clear();
  draw_status_bar(h, w, " Success v0.1.10 ", " Quiz Results ",
                  " Made with <3 ");

  const char *results_title = "QUIZ RESULTS";
  int title_x = (w - (int)strlen(results_title)) / 2;
  wattron(stdscr, COLOR_PAIR(3));
  mvaddstr(2, title_x, results_title);
  wattroff(stdscr, COLOR_PAIR(3));

  char score_text[256];
  snprintf(score_text, sizeof(score_text),
           "You got %d out of %d questions correct!", correct, quiz_count);
  int score_x = (w - (int)strlen(score_text)) / 2;
  wattron(stdscr, COLOR_PAIR(4));
  mvaddstr(5, score_x, score_text);
  wattroff(stdscr, COLOR_PAIR(4));

  char percentage_text[64];
  float percentage = quiz_count > 0 ? (correct * 100.0f) / quiz_count : 0;
  snprintf(percentage_text, sizeof(percentage_text), "Score: %.1f%%",
           percentage);
  int perc_x = (w - (int)strlen(percentage_text)) / 2;
  wattron(stdscr, COLOR_PAIR(3));
  mvaddstr(7, perc_x, percentage_text);
  wattroff(stdscr, COLOR_PAIR(3));

  // Display each question result (all 20 questions)
  int list_start_y = 10;
  int max_display = quiz_count < 20 ? quiz_count : 20;
  for (int i = 0; i < max_display; i++) {
    int display_y = list_start_y + i * 2;

    char result_line[512];
    bool is_correct =
        quiz_questions[i].user_answer == quiz_questions[i].correct_answer;
    if (is_correct) {
      snprintf(result_line, sizeof(result_line),
               "Q%d: Correct (Your answer: %c, Correct answer: %c)", i + 1,
               quiz_questions[i].user_answer ? quiz_questions[i].user_answer
                                             : '?',
               quiz_questions[i].correct_answer);
      wattron(stdscr, COLOR_PAIR(16)); // Green
    } else {
      snprintf(result_line, sizeof(result_line),
               "Q%d: Incorrect (Your answer: %c, Correct answer: %c)", i + 1,
               quiz_questions[i].user_answer ? quiz_questions[i].user_answer
                                             : '?',
               quiz_questions[i].correct_answer);
      wattron(stdscr, COLOR_PAIR(18)); // Pink/Red
    }

    mvaddstr(display_y, 5, result_line);
    if (is_correct)
      wattroff(stdscr, COLOR_PAIR(16));
    else
      wattroff(stdscr, COLOR_PAIR(18));
  }

  const char *guide_text = "[x] Exit";
  int guide_y = h - 4;
  int guide_x = (w - (int)strlen(guide_text)) / 2;
  render_guide_line(guide_y, guide_x, guide_text);

  refresh();

  // Wait for 'x' to exit
  int ch;
  while ((ch = getch()) != 'x' && ch != 'X') {
    if (ch == KEY_RESIZE) {
      getmaxyx(stdscr, h, w);
      clear();
      draw_status_bar(h, w, " Success v0.1.10 ", " Quiz Results ",
                      " Made with <3 ");
      title_x = (w - (int)strlen(results_title)) / 2;
      wattron(stdscr, COLOR_PAIR(3));
      mvaddstr(2, title_x, results_title);
      wattroff(stdscr, COLOR_PAIR(3));
      score_x = (w - (int)strlen(score_text)) / 2;
      wattron(stdscr, COLOR_PAIR(4));
      mvaddstr(5, score_x, score_text);
      wattroff(stdscr, COLOR_PAIR(4));
      perc_x = (w - (int)strlen(percentage_text)) / 2;
      wattron(stdscr, COLOR_PAIR(3));
      mvaddstr(7, perc_x, percentage_text);
      wattroff(stdscr, COLOR_PAIR(3));

      list_start_y = 10;
      max_display = quiz_count < 20 ? quiz_count : 20;
      for (int i = 0; i < max_display; i++) {
        int display_y = list_start_y + i * 2;
        char result_line[512];
        bool is_correct =
            quiz_questions[i].user_answer == quiz_questions[i].correct_answer;
        if (is_correct) {
          snprintf(result_line, sizeof(result_line),
                   "Q%d: Correct (Your answer: %c, Correct answer: %c)", i + 1,
                   quiz_questions[i].user_answer ? quiz_questions[i].user_answer
                                                 : '?',
                   quiz_questions[i].correct_answer);
          wattron(stdscr, COLOR_PAIR(16));
        } else {
          snprintf(result_line, sizeof(result_line),
                   "Q%d: Incorrect (Your answer: %c, Correct answer: %c)",
                   i + 1,
                   quiz_questions[i].user_answer ? quiz_questions[i].user_answer
                                                 : '?',
                   quiz_questions[i].correct_answer);
          wattron(stdscr, COLOR_PAIR(18));
        }
        mvaddstr(display_y, 5, result_line);
        if (is_correct)
          wattroff(stdscr, COLOR_PAIR(16));
        else
          wattroff(stdscr, COLOR_PAIR(18));
      }
      guide_x = (w - (int)strlen(guide_text)) / 2;
      render_guide_line(guide_y, guide_x, guide_text);
      refresh();
    }
  }
}

void quiz(void) {
  // Get quiz input
  char topic[512] = {0};
  char **file_uris = NULL;
  char **exts = NULL;
  int file_count = 0;
  cJSON *env = NULL;
  cJSON *gemini_api_key = NULL;
  cJSON *gemini_api_url = NULL;
  cJSON *gemini_file_url = NULL;

  // Load environment
  char *env_json = read_file("../env.json");
  if (!env_json) {
    return;
  }

  env = cJSON_Parse(env_json);
  if (!env) {
    free(env_json);
    return;
  }

  gemini_api_key = cJSON_GetObjectItemCaseSensitive(env, "GEMINI_API_KEY");
  gemini_api_url = cJSON_GetObjectItemCaseSensitive(env, "GEMINI_API_URL");
  gemini_file_url = cJSON_GetObjectItemCaseSensitive(env, "GEMINI_FILE_URL");

  if (!gemini_api_key || !gemini_api_key->valuestring || !gemini_api_url ||
      !gemini_api_url->valuestring || !gemini_file_url ||
      !gemini_file_url->valuestring) {
    cJSON_Delete(env);
    free(env_json);
    return;
  }

  // Get user input for quiz generation
  if (!get_quiz_input(topic, sizeof(topic), &file_uris, &exts, &file_count,
                      gemini_api_key, gemini_file_url)) {
    // User cancelled
    cJSON_Delete(env);
    free(env_json);
    if (file_uris) {
      for (int i = 0; i < file_count; i++) {
        free(file_uris[i]);
      }
      free(file_uris);
    }
    if (exts)
      free(exts);
    return;
  }

  // Generate quiz using Gemini API
  char *systemPrompt =
      "You are a quiz generator. Generate exactly 20 multiple-choice questions "
      "based on the user's topic or prompt. "
      "For each question, provide:\n"
      "1. A question number (1, 2, 3, etc.)\n"
      "2. The question text\n"
      "3. Three options labeled A, B, and C\n"
      "4. The correct answer marked with brackets [A], [B], or [C]\n\n"
      "Format example:\n"
      "Title: [Quiz Title]\n\n"
      "1. Question text here?\n"
      "   A. Option A text\n"
      "   [ B. Option B text ]\n"
      "   C. Option C text\n\n"
      "2. Next question?\n"
      "   A. Option A\n"
      "   B. Option B\n"
      "   [ C. Option C ]\n\n"
      "Continue this format for all 20 questions. Use brackets [] around the "
      "correct answer option.";

  char fullPrompt[4000];
  snprintf(fullPrompt, sizeof(fullPrompt), "System Prompt: %s\nUser Prompt: %s",
           systemPrompt,
           strlen(topic) > 0 ? topic : "Generate a general knowledge quiz");

  pthread_t generate_thread = {0};
  curl_global_init(CURL_GLOBAL_DEFAULT);

  is_generating = true;
  pthread_create(&generate_thread, NULL, gemini_loading, NULL);

  char *res_gemini_req = gemini_request(
      gemini_api_url->valuestring, file_count > 0 ? file_uris : NULL,
      gemini_api_key->valuestring, fullPrompt, file_count > 0 ? exts : NULL,
      file_count);

  is_generating = false;
  pthread_cancel(generate_thread);
  pthread_join(generate_thread, NULL);

  curl_global_cleanup();

  // Cleanup file URIs
  if (file_uris) {
    for (int i = 0; i < file_count; i++) {
      free(file_uris[i]);
    }
    free(file_uris);
  }
  if (exts)
    free(exts);

  if (!res_gemini_req) {
    cJSON_Delete(env);
    free(env_json);
    return;
  }

  // Parse quiz response
  if (parse_quiz_response(res_gemini_req) == 0) {
    free(res_gemini_req);
    cJSON_Delete(env);
    free(env_json);
    return;
  }

  free(res_gemini_req);

  // Initialize ncurses for quiz display
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  define_colors();
  wbkgd(stdscr, COLOR_PAIR(5));

  int current_page = 0;
  int selected_q = 0;
  int answering_q = -1;
  char selected_option = '\0';

  // Main quiz loop
  int ch;
  while (1) {
    display_quiz(current_page, selected_q, answering_q, selected_option);

    ch = getch();

    if (answering_q >= 0) {
      // Answering mode
      if (ch == KEY_UP || ch == KEY_DOWN) {
        // Cycle through A, B, C
        if (selected_option == '\0' || selected_option == 'A')
          selected_option = 'B';
        else if (selected_option == 'B')
          selected_option = 'C';
        else
          selected_option = 'A';
      } else if (ch == 10 || ch == KEY_ENTER) {
        // Confirm answer
        if (selected_option != '\0') {
          quiz_questions[answering_q].user_answer = selected_option;
          answering_q = -1;
          selected_option = '\0';
        }
      } else if (ch == 27) { // ESC
        answering_q = -1;
        selected_option = '\0';
      }
    } else {
      // Navigation mode
      if (ch == KEY_UP && selected_q > current_page * 5) {
        selected_q--;
      } else if (ch == KEY_DOWN && selected_q < (current_page * 5 + 4) &&
                 selected_q < quiz_count - 1) {
        selected_q++;
      } else if (ch == KEY_LEFT && current_page > 0) {
        current_page--;
        selected_q = current_page * 5;
      } else if (ch == KEY_RIGHT && current_page < (quiz_count + 4) / 5 - 1) {
        current_page++;
        selected_q = current_page * 5;
        if (selected_q >= quiz_count)
          selected_q = quiz_count - 1;
      } else if (ch == 10 || ch == KEY_ENTER) {
        // Start answering
        answering_q = selected_q;
        selected_option = 'A'; // Start with A
      } else if (ch == 's' || ch == 'S') {
        // Submit quiz
        display_results();
        break;
      } else if (ch == 'x' || ch == 'X') {
        // Exit
        break;
      }
    }

    if (ch == KEY_RESIZE) {
      // Redraw on resize
      continue;
    }
  }

  // Cleanup
  if (quiz_questions) {
    for (int i = 0; i < quiz_count; i++) {
      free(quiz_questions[i].question);
      free(quiz_questions[i].option_a);
      free(quiz_questions[i].option_b);
      free(quiz_questions[i].option_c);
    }
    free(quiz_questions);
    quiz_questions = NULL;
  }
  if (quiz_title) {
    free(quiz_title);
    quiz_title = NULL;
  }

  cJSON_Delete(env);
  free(env_json);
  endwin();

  // Redirect to tools page
  tools();
}
