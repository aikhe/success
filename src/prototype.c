// define __declspc as empty for native linux build (0or MSVC)
#include <stddef.h>
#ifndef __declspec
#define __declspec(x)
#endif

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <nfd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "base64.h"
#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// remove compilation errors when testing memory leaks
// with valgrind on unix based systems
#ifdef __linux__
#include <string.h>
#include <unistd.h>
#endif

#define QUOTE(...) #__VA_ARGS__ // pre-processor to turn content into string

typedef struct Memory {
  char *response;
  size_t size;
} Memory;

#ifdef _WIN32
// enable ANSI support for windows cmd
void enableVirtualTerminal() {
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD dwMode = 0;
  GetConsoleMode(hOut, &dwMode);
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(hOut, dwMode);
}
#endif

static const int B64index[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  62, 63, 62, 62, 63, 52, 53, 54, 55, 56, 57,
    58, 59, 60, 61, 0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,
    7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 0,  0,  0,  0,  63, 0,  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
    37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

char *b64decode(const void *data, const size_t len) {
  unsigned char *p = (unsigned char *)data;
  int pad = len > 0 && (len % 4 || p[len - 1] == '=');
  const size_t L = ((len + 3) / 4 - pad) * 4;
  char *str = malloc(L / 4 * 3 + pad + 1);
  if (!str)
    return NULL;

  size_t j = 0;
  for (size_t i = 0; i < L; i += 4) {
    int n = B64index[p[i]] << 18 | B64index[p[i + 1]] << 12 |
            B64index[p[i + 2]] << 6 | B64index[p[i + 3]];
    str[j++] = n >> 16;
    str[j++] = n >> 8 & 0xFF;
    str[j++] = n & 0xFF;
  }
  if (pad) {
    int n = B64index[p[L]] << 18 | B64index[p[L + 1]] << 12;
    str[j++] = n >> 16;

    if (len > L + 2 && p[L + 2] != '=') {
      n |= B64index[p[L + 2]] << 6;
      str[j++] = (n >> 8) & 0xFF;
    }
  }

  str[j] = '\0';
  return str;
}

void delay(int millisecond) {
#ifdef _WIN32
  Sleep(millisecond);
#elif __linux__
  usleep(millisecond * 1000);
#endif
}

char *read_file(const char *filename) {
  FILE *fptr = fopen(filename, "rb");
  if (!fptr)
    return NULL;

  fseek(fptr, 0, SEEK_END);  // move position indicator to end
  long length = ftell(fptr); // get length
  rewind(fptr);              // move position indicator to start

  char *data = malloc(length + 1); // add 1 for null terminator
  fread(data, 1, length,
        fptr); // this is where data finally gets read and added
  data[length] =
      '\0'; // add null terminator (crucial for printf & other string functions)
  fclose(fptr);

  return data;
}

char *replace_escaped_ansi(char *input) {
  const char *pattern = "\\033";
  const char esc = '\x1b';
  size_t len = strlen(input);
  char *output = malloc(len + 1);
  if (!output)
    return NULL;

  size_t i = 0, j = 0;
  while (i < len) {
    if (strncmp(&input[i], pattern, 4) == 0) {
      output[j++] = esc;
      i += 4; // skip over \033
    } else {
      output[j++] = input[i++];
    }
  }
  output[j] = '\0';
  return output;
}

unsigned char *read_file_b64(const char *filename, size_t *out_len) {
  FILE *fptr = fopen(filename, "rb");
  if (!fptr)
    return NULL;

  fseek(fptr, 0, SEEK_END);  // move position indicator to end
  long length = ftell(fptr); // get length
  rewind(fptr);              // move position indicator to start

  unsigned char *data = malloc(length); // add 1 for null terminato-
  size_t read_len = fread(
      data, 1, length, fptr); // this is where data finally gets read and added
  fclose(fptr);

  *out_len = read_len; // updates length
  return data;
}

static size_t write_callback(char *ptr, size_t size, size_t nmemb,
                             void *userdata) {
  size_t total_bytes = size * nmemb;
  struct Memory *mem = (struct Memory *)userdata;
  int lineNum = 1;

  char *temp = realloc(mem->response, mem->size + total_bytes + 1);
  if (!temp) {
    fprintf(stderr, "[ERROR] Failed to realloc memory. \n");
    return EXIT_FAILURE;
  }

  mem->response = temp;
  memcpy(&(mem->response[mem->size]), ptr, total_bytes);
  mem->size += total_bytes;
  mem->response[mem->size] = '\0';

  // printf("total chunk total_bytes: %zu\n", total_bytes);
  // printf("%d:\t", lineNum);
  for (int i = 0; i < total_bytes; i++) {
    // printf("%c", ptr[i]);
    if (ptr[i] == '\n') {
      lineNum++;
      // printf("%d:\t", lineNum);
    }
  }
  // printf("\n\n");

  return total_bytes;
}

bool is_generating = false;

void *geminiLoading(void *arg) {
  printf("\033[92mThinking");

  while (is_generating) {
    printf(".");
    delay(500);
  }
  printf("\x1b[0m");

  return NULL;
}

int main(void) {
  // ensure unbuffered output for all printf calls
  setvbuf(stdout, NULL, _IONBF, 0);

#ifdef _WIN32
  enableVirtualTerminal();
#endif

  size_t encoded_len;
  unsigned char *file_data = read_file_b64("python.pdf", &encoded_len);
  char *sample_encoded = base64_encode(file_data, encoded_len);

  while (1) {
    char *env_json = read_file("env.json");
    // printf("[DEBUG] env.json:\n%s\n", env_json);
    cJSON *env = cJSON_Parse(env_json);
    if (!env) {
      const char *error_ptr = cJSON_GetErrorPtr();

      if (error_ptr) {
        fprintf(stderr, "[ERROR] Error parsing JSON at %s\n", error_ptr);
      }

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

    CURL *curl;
    CURLcode res;

    Memory mem = {malloc(1), 0};

    char userPrompt[256];
    printf("Enter your prompt [enter 0 to exit]: ");
    if (fgets(userPrompt, sizeof(userPrompt), stdin) != NULL) {
      userPrompt[strcspn(userPrompt, "\n")] = '\0';

      if (strcmp(userPrompt, "0") == 0) {
        printf("[INFO] Exited\n");

        free(mem.response);
        cJSON_Delete(env);
        free(env_json);

        break;
      }
    }
    char *systemPrompt =
        "This is running on a terminal so format it to look good, no markdown "
        "formatting like bolds with double asterisk cause im getting the "
        "response and viewing it via terminal and no markdown formatting will "
        "work, also format it with colors with ANSI format like this \\033[97m";
    char fullPrompt[512];
    snprintf(fullPrompt, 512, "System Prompt: %s\nrnUser Prompt: %s",
             systemPrompt, userPrompt);

    cJSON *req_body_json = cJSON_CreateObject();
    cJSON *contents = cJSON_CreateArray();
    cJSON_AddItemToObject(req_body_json, "contents", contents);

    cJSON *content = cJSON_CreateObject();
    cJSON_AddItemToArray(contents, content);
    cJSON *parts = cJSON_CreateArray();
    cJSON_AddItemToObject(content, "parts", parts);

    cJSON *part_file = cJSON_CreateObject();
    cJSON *inline_data = cJSON_CreateObject();
    cJSON_AddItemToObject(part_file, "inline_data", inline_data);
    cJSON_AddStringToObject(inline_data, "mime_type", "application/pdf");
    cJSON_AddStringToObject(inline_data, "data", sample_encoded);
    cJSON_AddItemToArray(parts, part_file);

    cJSON *part_text = cJSON_CreateObject();
    cJSON_AddStringToObject(part_text, "text", fullPrompt);
    cJSON_AddItemToArray(parts, part_text);

    char *req_body_json_str = cJSON_Print(req_body_json);
    // printf("Request body:\n");
    // printf("%s\n", req_body_json_str);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    pthread_t generate_thread;

    is_generating = true;
    pthread_create(&generate_thread, NULL, geminiLoading, NULL);

    curl = curl_easy_init();
    if (curl) {
      struct curl_slist *list = NULL;
      char auth_header[512];
      char *header_name = "x-goog-api-key:";
      char *content_type = "Content-Type: application/json";

      snprintf(auth_header, sizeof(auth_header), "%s %s", header_name,
               gemini_api_key->valuestring);

      list = curl_slist_append(list, auth_header);
      list = curl_slist_append(list, content_type);

      // reduce latency
      curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 5000L);
      curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
      curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);

      // verbose logging
      // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

      // curl_easy_setopt(curl, CURLOPT_URL, "https://jacobsorber.com");
      curl_easy_setopt(curl, CURLOPT_URL, gemini_api_url->valuestring);
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body_json_str);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                       (long)strlen(req_body_json_str));
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&mem);

      // verification
      curl_easy_setopt(curl, CURLOPT_CAINFO,
                       "cacert-2025-09-09.pem"); // set cacert for ssl
      // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // removes SSL

      res = curl_easy_perform(curl);

      cJSON *mem_res = cJSON_Parse(mem.response);
      cJSON *candidates =
          cJSON_GetObjectItemCaseSensitive(mem_res, "candidates");
      cJSON *first_candidate = cJSON_GetArrayItem(candidates, 0);
      cJSON *content =
          cJSON_GetObjectItemCaseSensitive(first_candidate, "content");
      cJSON *parts = cJSON_GetObjectItemCaseSensitive(content, "parts");
      cJSON *first_part = cJSON_GetArrayItem(parts, 0);
      cJSON *text = cJSON_GetObjectItemCaseSensitive(first_part, "text");

      char *cleaned_text = replace_escaped_ansi(text->valuestring);
      printf("\n\033[97mGemini response:\n%s\x1b[0m\n", cleaned_text);
      free(cleaned_text);
      cJSON_Delete(mem_res);

      if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed. %s\n",
                curl_easy_strerror(res));
      } else {
        // printf("\nmem->response address: %p\n", &mem);
        // printf("\nFrom mem->response:\n%s\n", mem.response);
      }

      free(req_body_json_str);
      cJSON_Delete(req_body_json);
      curl_slist_free_all(list);
      curl_easy_cleanup(curl);
    } else {
      fprintf(stderr, "Curl initialization failed.\n");
    }

    is_generating = false;
    pthread_cancel(generate_thread);
    pthread_join(generate_thread, NULL);

    free(env_json);
    cJSON_Delete(env);
    free(mem.response);
    curl_global_cleanup();
  }

  free(sample_encoded);
  free(file_data);

  return EXIT_SUCCESS;
}
