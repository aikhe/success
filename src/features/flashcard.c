#include "flashcard.h"
#include "../pages/menu.h"
#include "curses.h"
#include <ctype.h>
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

// Flashcard structure
typedef struct {
  int number;
  char *question;
  char *correct_answer;
  char *explanation;
  char *user_answer; // User's input answer
  bool answered;
} Flashcard;

static Flashcard *flashcards = NULL;
static int flashcard_count = 0;
static char *flashcard_title = NULL;

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

// Parse flashcard response from Gemini
static int parse_flashcard_response(const char *response) {
  // Free existing flashcard data
  if (flashcards) {
    for (int i = 0; i < flashcard_count; i++) {
      free(flashcards[i].question);
      free(flashcards[i].correct_answer);
      free(flashcards[i].explanation);
      if (flashcards[i].user_answer) {
        free(flashcards[i].user_answer);
      }
    }
    free(flashcards);
    flashcards = NULL;
  }
  if (flashcard_title) {
    free(flashcard_title);
    flashcard_title = NULL;
  }
  flashcard_count = 0;

  // Try to extract title
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
      flashcard_title = strdup(title_buf);
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
      flashcard_title = strdup(title_buf);
    } else {
      flashcard_title = strdup("Flashcard Session");
    }
  }

  // Parse flashcards - look for numbered questions
  char *response_copy = strdup(response);
  char *line = strtok(response_copy, "\n");
  int current_fc = -1;

  while (line != NULL) {
    bool can_create_new = flashcard_count < 10;
    // Skip empty lines
    while (*line == ' ' || *line == '\t')
      line++;
    if (*line == '\0') {
      line = strtok(NULL, "\n");
      continue;
    }

    // Check if this is a flashcard number (e.g., "1.", "2.", etc.)
    char *num_start = line;
    while (*num_start == ' ' || *num_start == '\t')
      num_start++;

    if (*num_start >= '0' && *num_start <= '9' && can_create_new) {
      // New flashcard
      current_fc = flashcard_count;
      flashcards =
          realloc(flashcards, (flashcard_count + 1) * sizeof(Flashcard));
      flashcards[current_fc].number = current_fc + 1;
      flashcards[current_fc].question = NULL;
      flashcards[current_fc].correct_answer = NULL;
      flashcards[current_fc].explanation = NULL;
      flashcards[current_fc].user_answer = NULL;
      flashcards[current_fc].answered = false;

      // Extract question text (after number and period)
      char *q_start = num_start;
      while (*q_start && *q_start != '.' && *q_start != ' ')
        q_start++;
      if (*q_start == '.')
        q_start++;
      while (*q_start == ' ' || *q_start == '\t')
        q_start++;

      // Read until we find "Answer:" or "Correct answer:" or end of line
      char q_text[512] = {0};
      char *answer_marker = strstr(q_start, "Answer:");
      if (!answer_marker)
        answer_marker = strstr(q_start, "Correct answer:");
      if (!answer_marker)
        answer_marker = strstr(q_start, "Correct Answer:");
      if (!answer_marker)
        answer_marker = strstr(q_start, "answer:");

      if (answer_marker) {
        int q_len = answer_marker - q_start;
        while (q_len > 0 &&
               (q_start[q_len - 1] == ' ' || q_start[q_len - 1] == '\t'))
          q_len--;
        if (q_len > 0 && q_len < 511) {
          strncpy(q_text, q_start, q_len);
          q_text[q_len] = '\0';
          flashcards[current_fc].question = strdup(q_text);
        }
      } else {
        // No answer marker found, use rest of line
        int q_len = strlen(q_start);
        if (q_len > 0 && q_len < 511) {
          strncpy(q_text, q_start, q_len);
          flashcards[current_fc].question = strdup(q_text);
        }
      }

      flashcard_count++;
    } else if (current_fc >= 0 && flashcards[current_fc].question != NULL) {
      // Look for answer or explanation
      char *answer_marker = strstr(line, "Answer:");
      if (!answer_marker)
        answer_marker = strstr(line, "Correct answer:");
      if (!answer_marker)
        answer_marker = strstr(line, "Correct Answer:");
      if (!answer_marker)
        answer_marker = strstr(line, "answer:");

      char *explanation_marker = strstr(line, "Explanation:");
      if (!explanation_marker)
        explanation_marker = strstr(line, "explanation:");

      // Extract correct answer
      if (answer_marker && !flashcards[current_fc].correct_answer) {
        char *ans_start = answer_marker;
        // Find the colon after "Answer:" or "Correct answer:"
        while (*ans_start && *ans_start != ':')
          ans_start++;
        if (*ans_start == ':')
          ans_start++;
        while (*ans_start == ' ' || *ans_start == '\t')
          ans_start++;

        char *ans_end = ans_start;
        if (explanation_marker && explanation_marker > ans_start) {
          ans_end = explanation_marker;
        } else {
          ans_end = line + strlen(line);
        }
        while (ans_end > ans_start &&
               (ans_end[-1] == ' ' || ans_end[-1] == '\t'))
          ans_end--;

        int ans_len = ans_end - ans_start;
        if (ans_len > 0) {
          char ans_buf[256];
          strncpy(ans_buf, ans_start, ans_len);
          ans_buf[ans_len] = '\0';
          flashcards[current_fc].correct_answer = strdup(ans_buf);
        }
      }

      // Extract explanation
      if (explanation_marker && !flashcards[current_fc].explanation) {
        char *exp_start = explanation_marker;
        while (*exp_start && *exp_start != ':')
          exp_start++;
        if (*exp_start == ':')
          exp_start++;
        while (*exp_start == ' ' || *exp_start == '\t')
          exp_start++;

        char *exp_end = line + strlen(line);
        while (exp_end > exp_start &&
               (exp_end[-1] == ' ' || exp_end[-1] == '\t'))
          exp_end--;

        int exp_len = exp_end - exp_start;
        if (exp_len > 0) {
          char exp_buf[512];
          strncpy(exp_buf, exp_start, exp_len);
          exp_buf[exp_len] = '\0';
          flashcards[current_fc].explanation = strdup(exp_buf);
        }
      }
    }

    line = strtok(NULL, "\n");
  }

  free(response_copy);

  // Set default values if not found
  for (int i = 0; i < flashcard_count; i++) {
    if (!flashcards[i].question) {
      flashcards[i].question = strdup("Question not found");
    }
    if (!flashcards[i].correct_answer) {
      flashcards[i].correct_answer = strdup("Answer not found");
    }
    if (!flashcards[i].explanation) {
      flashcards[i].explanation = strdup("Explanation not found");
    }
  }

  return flashcard_count;
}

// Display flashcard generation input screen
static bool get_flashcard_input(char *topic, size_t topic_len,
                                 char ***file_uris_out, char ***exts_out,
                                 int *file_count_out, cJSON *gemini_api_key,
                                 cJSON *gemini_file_url) {
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

  draw_status_bar(h, w, " Success v0.1.10 ", " in Flashcard Generator ",
                  " Made with <3 ");

  const char *title_text = "FLASHCARD GENERATOR";
  int title_x = (w - (int)strlen(title_text)) / 2;
  wattron(stdscr, COLOR_PAIR(3));
  mvaddstr(2, title_x, title_text);
  wattroff(stdscr, COLOR_PAIR(3));

  const char *desc_text = R"(
  Generate comprehensive flashcards with AI. Enter a topic or attach files to create custom flashcard questions.
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
  char **file_names = NULL;

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

    draw_status_bar(h, w, " Success v0.1.10 ", " in Flashcard Generator ",
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

// Display flashcard screen
static void display_flashcard(int current_index, int hearts, bool show_answer,
                              const char *user_input) {
  int h, w;
  getmaxyx(stdscr, h, w);

  clear();

  draw_status_bar(h, w, " Success v0.1.10 ", " in Flashcard ",
                  " Made with <3 ");

  // Display title
  if (flashcard_title) {
    int title_x = (w - (int)strlen(flashcard_title)) / 2;
    wattron(stdscr, COLOR_PAIR(3));
    mvaddstr(2, title_x, flashcard_title);
    wattroff(stdscr, COLOR_PAIR(3));
  }

  if (current_index >= 0 && current_index < flashcard_count) {
    Flashcard *fc = &flashcards[current_index];

    // Calculate display area
    int input_w = w - 10;
    int input_x = (w - input_w) / 2;
    int padding_h = 1;
    int padding_w = 2;
    int text_width = input_w - (padding_w * 2);

    // Display question (2 spaces below title, using description window style)
    int question_y = 5; // 2 spaces below title (title at 2, so 2+2+1 = 5)
    int question_height = calculate_text_height_local(fc->question, text_width) + 2;

    // Render question with white text - need to manually render with white color
    // Create window first
    WINDOW *win_bars_q = newwin(question_height, input_w + 2, question_y, input_x - 1);
    WINDOW *question_win = newwin(question_height, input_w, question_y, input_x);
    wbkgd(win_bars_q, COLOR_PAIR(12));
    wbkgd(question_win, COLOR_PAIR(11));

    // Draw borders
    cchar_t vbar;
    setcchar(&vbar, L"┃", 0, 0, NULL);
    for (int i = 0; i < question_height; i++) {
      mvwadd_wch(win_bars_q, i, 0, &vbar);
      mvwadd_wch(win_bars_q, i, input_w + 2 - 1, &vbar);
    }

    // Render question text with white color (COLOR_PAIR(14) = white on GRAY_5)
    int padding = 2;
    int text_width_q = input_w - (padding * 2);
    wattron(question_win, COLOR_PAIR(14)); // White on GRAY_5
    
    // Wrap and render text
    int line = 1;
    int col = padding;
    const char *p = fc->question;
    
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

        if (col > padding && col + 1 + next_word_len > text_width_q + padding) {
          line++;
          col = padding;
        } else if (col > padding) {
          mvwaddch(question_win, line, col, ' ');
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

      if (col + word_len > text_width_q + padding) {
        line++;
        col = padding;
      }

      for (int i = 0; i < word_len; i++) {
        mvwaddch(question_win, line, col + i, word_start[i]);
      }
      col += word_len;
    }
    
    wattroff(question_win, COLOR_PAIR(14));
    wrefresh(win_bars_q);
    wrefresh(question_win);

    // Display correct answer window (below question)
    int answer_y = question_y + question_height + 1;
    int answer_height;
    
    if (show_answer && fc->correct_answer) {
      // Render answer with label in gray and answer text in white
      // Create window
      int answer_text_height = calculate_text_height_local(fc->correct_answer, text_width);
      answer_height = answer_text_height + 2;
      
      WINDOW *win_bars_ans = newwin(answer_height, input_w + 2, answer_y, input_x - 1);
      WINDOW *answer_win = newwin(answer_height, input_w, answer_y, input_x);
      wbkgd(win_bars_ans, COLOR_PAIR(12));
      wbkgd(answer_win, COLOR_PAIR(11));
      
      // Draw borders
      cchar_t vbar;
      setcchar(&vbar, L"┃", 0, 0, NULL);
      for (int i = 0; i < answer_height; i++) {
        mvwadd_wch(win_bars_ans, i, 0, &vbar);
        mvwadd_wch(win_bars_ans, i, input_w + 2 - 1, &vbar);
      }
      
      // Render "Correct answer: " label in gray
      wattron(answer_win, COLOR_PAIR(11)); // Gray on GRAY_5
      mvwaddstr(answer_win, 1, padding, "Correct answer: ");
      wattroff(answer_win, COLOR_PAIR(11));
      
      // Render answer text in white
      wattron(answer_win, COLOR_PAIR(14)); // White on GRAY_5
      int answer_start_x = padding + 17; // After "Correct answer: "
      int line = 1;
      int col = answer_start_x;
      const char *ans_p = fc->correct_answer;
      
      while (*ans_p) {
        if (*ans_p == ' ') {
          const char *next = ans_p + 1;
          while (*next == ' ')
            next++;
          int next_word_len = 0;
          const char *word_end = next;
          while (*word_end && *word_end != ' ') {
            next_word_len++;
            word_end++;
          }
          
          if (col > answer_start_x && col + 1 + next_word_len > input_w - padding) {
            line++;
            col = answer_start_x;
          } else if (col > answer_start_x) {
            mvwaddch(answer_win, line, col, ' ');
            col++;
          }
          ans_p++;
          continue;
        }
        
        const char *word_start = ans_p;
        int word_len = 0;
        while (*ans_p && *ans_p != ' ') {
          word_len++;
          ans_p++;
        }
        
        if (col + word_len > input_w - padding) {
          line++;
          col = answer_start_x;
        }
        
        for (int i = 0; i < word_len; i++) {
          mvwaddch(answer_win, line, col + i, word_start[i]);
        }
        col += word_len;
      }
      
      wattroff(answer_win, COLOR_PAIR(14));
      wrefresh(win_bars_ans);
      wrefresh(answer_win);
    } else {
      // Just show label
      char answer_display[512];
      strncpy(answer_display, "Correct answer: ", sizeof(answer_display) - 1);
      answer_display[sizeof(answer_display) - 1] = '\0';
      answer_height = calculate_text_height_local(answer_display, text_width) + 2;
      ColoredWord answer_colors[] = {};
      render_text_with_colors(stdscr, input_w, answer_y, input_x, answer_display,
                              11, 12, true, answer_colors, 0, 0);
    }

    // Bottom section
    int input_y = h - 7;
    int input_cursor_y = input_y + 1;
    int input_cursor_x = input_x + 3;

    // Hearts and current/total on same line
    int hearts_y = input_y - 4;
    char hearts_text[64];
    snprintf(hearts_text, sizeof(hearts_text), "Hearts: %d   %d/%d", hearts,
             current_index + 1, flashcard_count);
    int hearts_x = (w - (int)strlen(hearts_text)) / 2;
    wattron(stdscr, COLOR_PAIR(2));
    mvaddstr(hearts_y, hearts_x, hearts_text);
    wattroff(stdscr, COLOR_PAIR(2));

    // Guide (2 spaces above input bar when answering)
    int guide_y = show_answer ? input_y - 3 : input_y - 2;
    const char *guide_text;
    if (show_answer) {
      guide_text = "[Any Key] Continue   [F1] Stop";
    } else {
      guide_text = "[Enter] Submit   [F1] Stop";
    }
    int guide_x = (w - (int)strlen(guide_text)) / 2;
    render_guide_line(guide_y, guide_x, guide_text);

    // Input bar (only show input text if not showing answer)
    render_input(stdscr, input_w, 3, input_y, input_x, ">");

    // Show user input if any and not showing answer
    if (!show_answer && user_input && strlen(user_input) > 0) {
      wattron(stdscr, COLOR_PAIR(1));
      mvwaddstr(stdscr, input_cursor_y, input_cursor_x, user_input);
      wattroff(stdscr, COLOR_PAIR(1));
    }

    if (!show_answer) {
      move(input_cursor_y, input_cursor_x + (user_input ? strlen(user_input) : 0));
    } else {
      move(input_cursor_y, input_cursor_x);
    }
  }

  refresh();
}

// Display results screen
static void display_results(int hearts, int correct_count) {
  int h, w;
  getmaxyx(stdscr, h, w);

  clear();
  draw_status_bar(h, w, " Success v0.1.10 ", " Flashcard Results ",
                  " Made with <3 ");

  const char *results_title = "FLASHCARD RESULTS";
  int title_x = (w - (int)strlen(results_title)) / 2;
  wattron(stdscr, COLOR_PAIR(3));
  mvaddstr(2, title_x, results_title);
  wattroff(stdscr, COLOR_PAIR(3));

  char score_text[256];
  snprintf(score_text, sizeof(score_text),
           "You got %d out of %d flashcards correct!", correct_count,
           flashcard_count);
  int score_x = (w - (int)strlen(score_text)) / 2;
  wattron(stdscr, COLOR_PAIR(4));
  mvaddstr(5, score_x, score_text);
  wattroff(stdscr, COLOR_PAIR(4));

  char hearts_text[64];
  snprintf(hearts_text, sizeof(hearts_text), "Hearts remaining: %d", hearts);
  int hearts_x = (w - (int)strlen(hearts_text)) / 2;
  wattron(stdscr, COLOR_PAIR(3));
  mvaddstr(7, hearts_x, hearts_text);
  wattroff(stdscr, COLOR_PAIR(3));

  // Display each flashcard result
  int list_start_y = 10;
  int max_display = flashcard_count < 10 ? flashcard_count : 10;
  for (int i = 0; i < max_display; i++) {
    int display_y = list_start_y + i * 3;

    char result_line[512];
    bool is_correct = false;
    if (flashcards[i].user_answer && flashcards[i].correct_answer) {
      // Simple string comparison (case-insensitive)
      char user_lower[256];
      char correct_lower[256];
      int j = 0;
      for (j = 0; j < 255 && flashcards[i].user_answer[j]; j++) {
        user_lower[j] = tolower(flashcards[i].user_answer[j]);
      }
      user_lower[j] = '\0';
      for (j = 0; j < 255 && flashcards[i].correct_answer[j]; j++) {
        correct_lower[j] = tolower(flashcards[i].correct_answer[j]);
      }
      correct_lower[j] = '\0';

      // Trim whitespace
      while (strlen(user_lower) > 0 && user_lower[strlen(user_lower) - 1] == ' ') {
        user_lower[strlen(user_lower) - 1] = '\0';
      }
      while (strlen(correct_lower) > 0 && correct_lower[strlen(correct_lower) - 1] == ' ') {
        correct_lower[strlen(correct_lower) - 1] = '\0';
      }

      is_correct = (strcmp(user_lower, correct_lower) == 0);
    }

    if (is_correct) {
      snprintf(result_line, sizeof(result_line),
               "FC%d: Correct (Your answer: %s, Correct answer: %s)", i + 1,
               flashcards[i].user_answer ? flashcards[i].user_answer : "?",
               flashcards[i].correct_answer ? flashcards[i].correct_answer : "?");
      wattron(stdscr, COLOR_PAIR(16)); // Green
    } else {
      snprintf(result_line, sizeof(result_line),
               "FC%d: Incorrect (Your answer: %s, Correct answer: %s)", i + 1,
               flashcards[i].user_answer ? flashcards[i].user_answer : "?",
               flashcards[i].correct_answer ? flashcards[i].correct_answer : "?");
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
      draw_status_bar(h, w, " Success v0.1.10 ", " Flashcard Results ",
                      " Made with <3 ");
      title_x = (w - (int)strlen(results_title)) / 2;
      wattron(stdscr, COLOR_PAIR(3));
      mvaddstr(2, title_x, results_title);
      wattroff(stdscr, COLOR_PAIR(3));
      score_x = (w - (int)strlen(score_text)) / 2;
      wattron(stdscr, COLOR_PAIR(4));
      mvaddstr(5, score_x, score_text);
      wattroff(stdscr, COLOR_PAIR(4));
      hearts_x = (w - (int)strlen(hearts_text)) / 2;
      wattron(stdscr, COLOR_PAIR(3));
      mvaddstr(7, hearts_x, hearts_text);
      wattroff(stdscr, COLOR_PAIR(3));

      list_start_y = 10;
      max_display = flashcard_count < 10 ? flashcard_count : 10;
      for (int i = 0; i < max_display; i++) {
        int display_y = list_start_y + i * 3;
        char result_line[512];
        bool is_correct = false;
        if (flashcards[i].user_answer && flashcards[i].correct_answer) {
          char user_lower[256];
          char correct_lower[256];
          int j = 0;
          for (j = 0; j < 255 && flashcards[i].user_answer[j]; j++) {
            user_lower[j] = tolower(flashcards[i].user_answer[j]);
          }
          user_lower[j] = '\0';
          for (j = 0; j < 255 && flashcards[i].correct_answer[j]; j++) {
            correct_lower[j] = tolower(flashcards[i].correct_answer[j]);
          }
          correct_lower[j] = '\0';

          while (strlen(user_lower) > 0 && user_lower[strlen(user_lower) - 1] == ' ') {
            user_lower[strlen(user_lower) - 1] = '\0';
          }
          while (strlen(correct_lower) > 0 && correct_lower[strlen(correct_lower) - 1] == ' ') {
            correct_lower[strlen(correct_lower) - 1] = '\0';
          }

          is_correct = (strcmp(user_lower, correct_lower) == 0);
        }

        if (is_correct) {
          snprintf(result_line, sizeof(result_line),
                   "FC%d: Correct (Your answer: %s, Correct answer: %s)", i + 1,
                   flashcards[i].user_answer ? flashcards[i].user_answer : "?",
                   flashcards[i].correct_answer ? flashcards[i].correct_answer : "?");
          wattron(stdscr, COLOR_PAIR(16));
        } else {
          snprintf(result_line, sizeof(result_line),
                   "FC%d: Incorrect (Your answer: %s, Correct answer: %s)", i + 1,
                   flashcards[i].user_answer ? flashcards[i].user_answer : "?",
                   flashcards[i].correct_answer ? flashcards[i].correct_answer : "?");
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

void flashcard(void) {
  // Get flashcard input
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

  // Get user input for flashcard generation
  if (!get_flashcard_input(topic, sizeof(topic), &file_uris, &exts, &file_count,
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

  // Generate flashcards using Gemini API
  char *systemPrompt =
      "You are a flashcard generator. Generate exactly 10 flashcards "
      "based on the user's topic or prompt. "
      "For each flashcard, provide:\n"
      "1. A flashcard number (1, 2, 3, etc.)\n"
      "2. A question text\n"
      "3. The correct answer\n"
      "4. An explanation of the correct answer\n\n"
      "Format example:\n"
      "Title: [Flashcard Session Title]\n\n"
      "1. What is the capital of France?\n"
      "   Answer: Paris\n"
      "   Explanation: Paris is the capital and largest city of France.\n\n"
      "2. What is 2 + 2?\n"
      "   Answer: 4\n"
      "   Explanation: Two plus two equals four.\n\n"
      "Continue this format for all 10 flashcards. Make sure each flashcard has a question, answer, and explanation.";

  char fullPrompt[4000];
  snprintf(fullPrompt, sizeof(fullPrompt), "System Prompt: %s\nUser Prompt: %s",
           systemPrompt,
           strlen(topic) > 0 ? topic : "Generate a general knowledge flashcard set");

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

  // Parse flashcard response
  if (parse_flashcard_response(res_gemini_req) == 0) {
    free(res_gemini_req);
    cJSON_Delete(env);
    free(env_json);
    return;
  }

  free(res_gemini_req);

  // Initialize ncurses for flashcard display
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(1);

  define_colors();
  wbkgd(stdscr, COLOR_PAIR(5));

  int current_index = 0;
  int hearts = 5;
  bool show_answer = false;
  char user_input[256] = {0};
  int input_cursor_pos = 0;

  // Main flashcard loop
  int ch;
  bool input_mode = true;
  while (1) {
    if (current_index >= flashcard_count || current_index < 0) {
      // Finished all flashcards
      break;
    }

    // Check if hearts reached 0 - game over
    if (hearts <= 0) {
      break;
    }

    display_flashcard(current_index, hearts, show_answer, user_input);

    ch = getch();

    if (input_mode) {
      if (ch == 10 || ch == KEY_ENTER) {
        // Submit answer
        if (strlen(user_input) > 0) {
          if (!flashcards[current_index].user_answer) {
            flashcards[current_index].user_answer = strdup(user_input);
          } else {
            free(flashcards[current_index].user_answer);
            flashcards[current_index].user_answer = strdup(user_input);
          }
          flashcards[current_index].answered = true;

          // Check if answer is correct
          if (flashcards[current_index].correct_answer) {
            char user_lower[256];
            char correct_lower[256];
            int j = 0;
            for (j = 0; j < 255 && user_input[j]; j++) {
              user_lower[j] = tolower(user_input[j]);
            }
            user_lower[j] = '\0';
            for (j = 0; j < 255 && flashcards[current_index].correct_answer[j]; j++) {
              correct_lower[j] = tolower(flashcards[current_index].correct_answer[j]);
            }
            correct_lower[j] = '\0';

            // Trim whitespace
            while (strlen(user_lower) > 0 && user_lower[strlen(user_lower) - 1] == ' ') {
              user_lower[strlen(user_lower) - 1] = '\0';
            }
            while (strlen(correct_lower) > 0 && correct_lower[strlen(correct_lower) - 1] == ' ') {
              correct_lower[strlen(correct_lower) - 1] = '\0';
            }

            if (strcmp(user_lower, correct_lower) != 0) {
              // Wrong answer - lose a heart
              if (hearts > 0) {
                hearts--;
              }
            }
          }

          // Show answer
          show_answer = true;
          input_mode = false;
          // Clear user input from display (answer will show)
          user_input[0] = '\0';
          input_cursor_pos = 0;
          // Redisplay with answer shown
          display_flashcard(current_index, hearts, show_answer, user_input);
          
          // If hearts reached 0, end the game after showing the answer
          if (hearts <= 0) {
            // Wait for any key to see results
            ch = getch();
            break;
          }
        }
      } else if (ch == KEY_F(1)) {
        // Stop and finish
        break;
      } else if ((ch == KEY_BACKSPACE || ch == 127 || ch == 8) &&
                 input_cursor_pos > 0) {
        input_cursor_pos--;
        user_input[input_cursor_pos] = '\0';
      } else if (ch >= 32 && ch <= 126 && input_cursor_pos < 255) {
        user_input[input_cursor_pos++] = (char)ch;
        user_input[input_cursor_pos] = '\0';
      }
    } else {
      // Answer is shown, wait for any key to continue
      if (ch == KEY_F(1)) {
        // Stop and finish
        break;
      } else {
        // Move to next flashcard
        current_index++;
        show_answer = false;
        input_mode = true;
        user_input[0] = '\0';
        input_cursor_pos = 0;
      }
    }

    if (ch == KEY_RESIZE) {
      // Redraw on resize
      continue;
    }
  }

  // Calculate results
  int correct_count = 0;
  for (int i = 0; i < flashcard_count; i++) {
    if (flashcards[i].user_answer && flashcards[i].correct_answer) {
      char user_lower[256];
      char correct_lower[256];
      int j = 0;
      for (j = 0; j < 255 && flashcards[i].user_answer[j]; j++) {
        user_lower[j] = tolower(flashcards[i].user_answer[j]);
      }
      user_lower[j] = '\0';
      for (j = 0; j < 255 && flashcards[i].correct_answer[j]; j++) {
        correct_lower[j] = tolower(flashcards[i].correct_answer[j]);
      }
      correct_lower[j] = '\0';

      while (strlen(user_lower) > 0 && user_lower[strlen(user_lower) - 1] == ' ') {
        user_lower[strlen(user_lower) - 1] = '\0';
      }
      while (strlen(correct_lower) > 0 && correct_lower[strlen(correct_lower) - 1] == ' ') {
        correct_lower[strlen(correct_lower) - 1] = '\0';
      }

      if (strcmp(user_lower, correct_lower) == 0) {
        correct_count++;
      }
    }
  }

  // Display results
  display_results(hearts, correct_count);

  // Cleanup
  if (flashcards) {
    for (int i = 0; i < flashcard_count; i++) {
      free(flashcards[i].question);
      free(flashcards[i].correct_answer);
      free(flashcards[i].explanation);
      if (flashcards[i].user_answer) {
        free(flashcards[i].user_answer);
      }
    }
    free(flashcards);
    flashcards = NULL;
  }
  if (flashcard_title) {
    free(flashcard_title);
    flashcard_title = NULL;
  }

  cJSON_Delete(env);
  free(env_json);
  endwin();

  // Redirect to tools page
  tools();
}

