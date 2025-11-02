#include "ai_chat.h"

// define __declspc as empty for native linux build (0or MSVC)
#include <stddef.h>
#ifndef __declspec
#define __declspec(x)
#endif

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <locale.h>
#include <nfd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <windows.h>
#undef MOUSE_MOVED // remove redefinition errors from wincon.h macro
#include <curses.h>

// #include "pages/introduction.h"

#include "../gemini_api/gemini_request.h"
#include "../gemini_api/get_file_uri.h"
#include "../gemini_api/get_upload_url.h"

#include "../utils/gemini_loading.h"
#include "../utils/get_file_mime_type.h"
#include "../utils/read_file.h"
#include "../utils/read_file_b64.h"

#define QUOTE(...) #__VA_ARGS__ // pre-processor to turn content into string

// void enableVirtualTerminal() {
// #ifdef _WIN32
//   // enable ANSI support for windows cmd
//   HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
//   DWORD dwMode = 0;
//   GetConsoleMode(hOut, &dwMode);
//   dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
//   SetConsoleMode(hOut, dwMode);
//
//   // set both input and output to UTF-8
//   SetConsoleOutputCP(CP_UTF8);
//   SetConsoleCP(CP_UTF8);
//
//   // support ACS symbols (e.g. pwdcurses box)
//   // SetConsoleOutputCP(437);
//   // SetConsoleCP(437);
// #endif
// }

int ai_chat(void) {
  //   // Set locale BEFORE calling any curses functions
  //   setlocale(LC_ALL, "en_US.UTF-8");
  //
  // // On Windows, try UTF-8 locale if the above fails
  // #ifdef _WIN32
  //   if (!setlocale(LC_ALL, "en_US.UTF-8")) {
  //     setlocale(LC_ALL, "C.UTF-8");
  //   }
  // #endif

  // enableVirtualTerminal();

  // setvbuf(stdout, NULL, _IONBF, 0);

  char *env_json = read_file("../env.json");

  cJSON *env = cJSON_Parse(env_json);
  if (!env) {
    printf("no env\n");
    const char *error_ptr = cJSON_GetErrorPtr();

    if (error_ptr) {
      fprintf(stderr, "[ERROR] Error parsing JSON at %s\n", error_ptr);
    }
    printf("no env\n");

    free(env_json);
    return EXIT_FAILURE;
  }

  cJSON *gemini_api_key =
      cJSON_GetObjectItemCaseSensitive(env, "GEMINI_API_KEY");
  if (!gemini_api_key->valuestring) {
    fprintf(stderr, "GEMINI_API_KEY environment variable not set.\n");
  }
  cJSON *gemini_api_url =
      cJSON_GetObjectItemCaseSensitive(env, "GEMINI_API_URL");
  if (!gemini_api_url->valuestring) {
    fprintf(stderr, "GEMINI_API_URL environment variable not set.\n");
  }
  cJSON *gemini_file_url =
      cJSON_GetObjectItemCaseSensitive(env, "GEMINI_FILE_URL");
  if (!gemini_file_url->valuestring) {
    fprintf(stderr, "GEMINI_FILE_URL environment variable not set.\n");
  }

  char *systemPrompt =
      // "CRITICAL RESPONSE RULES: "
      // "- Answer ONLY what is asked - nothing more, nothing less "
      // "- NO greetings, NO asking questions back, NO offering help "
      // "- When asked 'what is SUCCESS', describe the platform directly "
      // "- When asked 'who are you', say you are SUCCESS AI "
      // "- For any other question, answer it directly without introduction "
      // "- Never say 'I understand', 'Hello', or 'How can I help' "
      // "- Be concise, informative, and get straight to the point "
      "CRITICAL RESPONSE RULES: "
      "- Don't reply with \"Okay, I understand.\""

      "SUCCESS is a collaboration platform designed for UCCians to connect, "
      "learn, "
      "and grow together. It features built-in tools such as quizzes, "
      "flashcards, "
      "summarizers, streak tracking, and leaderboards to make learning "
      "engaging, "
      "productive, and exam-ready. Most importantly, this platform integrates "
      "proven "
      "study methods (e.g., Pomodoro, active recall, spaced repetition, etc.), "
      "personalized to fit each student's learning style, making study "
      "sessions "
      "more effective and easier to manage. "
      "To have a lively feel to this platform, both students and teachers can "
      "collaborate, share resources, and ask or answer questions. This system "
      "also "
      "allows students to track their academic progress, mark completed "
      "subjects, "
      "and prepare for upcoming lessons. By storing data locally, the program "
      "ensures that resources remain accessible offline, making it useful for "
      "students who don't always have access to the internet. "

      "TERMINAL FORMATTING:"
      "- This is running on a terminal, so make it visually clean and readable."
      "- Do NOT use markdown formatting (like **bold** or ```code``` blocks)."
      "- Use ANSI escape codes for styling and color to make it colorful "
      "(e.g., \\033[97m for "
      "white text, other colors like orange, green, blue, red, etc.)."
      "- Utilize foreground colors for contrast."
      "- Use \\033[1m for bold and \\033[3m for italics when needed."
      "- Keep alignment consistent with clear indentation and spacing."
      "- Use simple ASCII characters or line symbols for structure like (e.g., "
      "─, "
      "│, •)."
      "- Output should look like a well-formatted CLI report — no markdown "
      "- Don't use line seperators"
      "syntax.";

  char userPrompt[512];
  char fullPrompt[4000];

  nfdresult_t nfd_res = NFD_CANCEL;
  nfdpathset_t pathSet = {0};

  while (1) {
    int total_file_num = 0;
    char **exts = NULL;
    char **file_uris = NULL;
    char *res_gemini_req = NULL;

    printf("\033[97mEnter your prompt \033[34m[f to "
           "attach files, enter x to "
           "exit]: "
           "\033[0m");

    if (fgets(userPrompt, sizeof(userPrompt), stdin) != NULL) {
      userPrompt[strcspn(userPrompt, "\n")] = '\0';

      if (strcmp(userPrompt, "x") == 0) {
        printf("[INFO] Exited\n");
        break;
      } else if (strcmp(userPrompt, "f") == 0) {
        nfd_res = NFD_OpenDialogMultiple("png,jpeg,jpg,pdf", NULL, &pathSet);

        for (size_t i = 0; i < NFD_PathSet_GetCount(&pathSet); ++i) {
          nfdchar_t *path = NFD_PathSet_GetPath(&pathSet, i);
          printf("Path %i: %s\n", (int)i, path);
        }

        continue;
      }
    }
    snprintf(fullPrompt, sizeof(fullPrompt),
             "System Prompt: %s\nUser Prompt: %s", systemPrompt, userPrompt);

    // printf("Full prompt:\n%s\n", fullPrompt);

    pthread_t generate_thread = {0};

    curl_global_init(CURL_GLOBAL_DEFAULT);

    is_generating = true;
    pthread_create(&generate_thread, NULL, gemini_loading, NULL);

    if (nfd_res == NFD_OKAY) {
      total_file_num = NFD_PathSet_GetCount(&pathSet);

      int capacity = 0;

      printf("im here loop\n");

      for (size_t i = 0; i < total_file_num; ++i) {
        nfdchar_t *path = NFD_PathSet_GetPath(&pathSet, i);

        size_t encoded_len;
        unsigned char *file_data = read_file_b64(path, &encoded_len);
        const char *ext = get_file_mime_type(path);

        printf("im here loop 2\n");

        char *res_upload_url =
            get_upload_url(encoded_len, gemini_file_url->valuestring,
                           gemini_api_key->valuestring, (char *)ext);

        printf("im here loop 3\n");

        char *res_file_uri =
            get_file_uri(file_data, encoded_len, path, res_upload_url,
                         gemini_api_key->valuestring, (char *)ext);

        printf("im here loop 4\n");

        capacity = (capacity == 0) ? 1 : (capacity + 1);
        char **new_ext = realloc(exts, (capacity) * sizeof(char *));
        char **new_file_uris = realloc(file_uris, (capacity) * sizeof(char *));
        exts = new_ext;
        file_uris = new_file_uris;

        exts[i] = (char *)ext;
        file_uris[i] = res_file_uri;

        free(file_data);
        free(res_upload_url);
      }

      // for (size_t j = 0; j < total_file_num; j++) {
      //   printf("ext %zu: %s\n", j + 1, exts[j]);
      //   printf("file_uri %zu: %s\n", j + 1, file_uris[j]);
      // }
    } else if (nfd_res == NFD_CANCEL) {
      // puts("User pressed cancel.");
    } else {
      printf("Error: %s\n", NFD_GetError());
    }

    bool query_with_file = total_file_num > 0;

    // printf("im here hi\n");

    res_gemini_req = gemini_request(
        gemini_api_url->valuestring, query_with_file ? file_uris : NULL,
        gemini_api_key->valuestring, fullPrompt, query_with_file ? exts : NULL,
        total_file_num);

    // printf("im here wow\n");

    is_generating = false;
    pthread_cancel(generate_thread);
    pthread_join(generate_thread, NULL);

    printf("✓\n\033[97mGemini response:\n%s\n", res_gemini_req);

    if (query_with_file) {
      for (size_t i = 0; i < total_file_num; i++) {
        free(file_uris[i]);
      }
      free(file_uris);
      free(exts);
    }

    if (res_gemini_req) {
      free(res_gemini_req);
    }

    curl_global_cleanup();

    nfd_res = NFD_CANCEL;
    NFD_PathSet_Free(&pathSet);
    memset(&pathSet, 0, sizeof(pathSet));
  }

  NFD_PathSet_Free(&pathSet);
  free(env_json);
  cJSON_Delete(env);

  return EXIT_SUCCESS;
}
