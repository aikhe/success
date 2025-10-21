// define __declspc as empty for native linux build (0or MSVC)
#include <stddef.h>
#ifndef __declspec
#define __declspec(x)
#endif

#include "base64.h"
#include "include/nfd.h"
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

const char *get_file_mime_type(const char *filename) {
  const char *dot = strrchr(filename, '.');
  if (!dot || dot == filename)
    return NULL;

  char *ext = (char *)dot + 1;

  if (strcmp(ext, "png") == 0)
    return "image/png";
  else if (strcmp(ext, "jpeg") == 0 || strcmp(ext, "jpg") == 0)
    return "image/jpeg";
  else if (strcmp(ext, "pdf") == 0)
    return "application/pdf";
  else
    printf("[Error] Invalid file formats.");

  return NULL;
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

  fseek(fptr, 0, SEEK_END);
  long length = ftell(fptr);
  rewind(fptr);

  char *data = malloc(length + 1);
  fread(data, 1, length, fptr);
  data[length] = '\0';
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
      i += 4;
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

  fseek(fptr, 0, SEEK_END);
  long length = ftell(fptr);
  rewind(fptr);

  unsigned char *data = malloc(length);
  size_t read_len = fread(data, 1, length, fptr);
  fclose(fptr);

  *out_len = read_len;
  return data;
}

static size_t write_callback(char *ptr, size_t size, size_t nmemb,
                             void *userdata) {
  size_t total_bytes = size * nmemb;
  struct Memory *mem = (struct Memory *)userdata;

  char *temp = realloc(mem->response, mem->size + total_bytes + 1);
  if (!temp) {
    fprintf(stderr, "[ERROR] Failed to realloc memory. \n");
    return 0;
  }

  mem->response = temp;
  memcpy(&(mem->response[mem->size]), ptr, total_bytes);
  mem->size += total_bytes;
  mem->response[mem->size] = '\0';

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

typedef struct CallType {
  char *call_type;
} CallType;

CallType set_call_type(char *call_type) {
  CallType ct = {.call_type = call_type};
  return ct;
}

char *grep_string(const char *data) {
  if (*data == '\0')
    return NULL;

  const char *line_start = data;
  const char *new_line;
  char line[1024];

  while ((new_line = strchr(line_start, '\n')) != NULL) {
    size_t line_len = new_line - line_start;

    if (line_len >= sizeof(line)) {
      line_len = sizeof(line) - 1;
    }

    memcpy(line, line_start, line_len);
    line[line_len] = '\0';

    const char *fo = strstr(line, "x-goog-upload-url:");
    if (fo != NULL) {
      const char *url_start = strstr(fo, "https://");
      if (url_start) {
        const char *url_end = url_start;
        while (*url_end != '\r') {
          url_end++;
        }

        size_t url_len = url_end - url_start;
        char *url = malloc(url_len + 1);
        if (url) {
          memcpy(url, url_start, url_len);
          url[url_len] = '\0';
        }
        return url;
      }
      return NULL;
    }

    line_start = new_line + 1;
  }

  return NULL;
}

char *get_upload_url(long int image_len, char *gemini_file_url,
                     char *gemini_api_key, char *file_mime_type) {
  Memory mem = {malloc(1), 0};

  CURL *curl = curl_easy_init();

  struct curl_slist *list = NULL;
  char auth_header[512];
  char *api_key = "x-goog-api-key:";
  char *upload_protocol = "X-Goog-Upload-Protocol: resumable";
  char *upload_command = "X-Goog-Upload-Command: start";
  char length[512];
  char *upload_header_content_length = "X-Goog-Upload-Header-Content-Length:";
  char type[512];
  char *upload_header_content_type = "X-Goog-Upload-Header-Content-Type:";
  char *content_type = "Content-Type: application/json";

  snprintf(auth_header, sizeof(auth_header), "%s %s", api_key, gemini_api_key);
  snprintf(length, sizeof(length), "%s %ld", upload_header_content_length,
           image_len);
  snprintf(type, sizeof(type), "%s %s", upload_header_content_type,
           file_mime_type);

  list = curl_slist_append(list, auth_header);
  list = curl_slist_append(list, upload_protocol);
  list = curl_slist_append(list, upload_command);
  list = curl_slist_append(list, length);
  list = curl_slist_append(list, type);
  list = curl_slist_append(list, content_type);

  const char *req_json = "{'file': {'display_name': 'IMAGE'}}";

  curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 5000L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
  curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);

  curl_easy_setopt(curl, CURLOPT_URL, gemini_file_url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_json);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(req_json));
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&mem);

  curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert-2025-09-09.pem");

  curl_easy_perform(curl);

  char *res_url = grep_string(mem.response);

  curl_slist_free_all(list);
  free(mem.response);
  curl_easy_cleanup(curl);

  return res_url;
}

char *get_file_uri(unsigned char *image_data, long int image_len,
                   char *image_path, char *upload_url, char *gemini_api_key) {
  Memory mem = {malloc(1), 0};

  CURL *curl = curl_easy_init();

  struct curl_slist *list = NULL;
  char auth_header[512];
  char *api_key = "x-goog-api-key:";
  char *content_length = "Content-Length:";
  char *upload_offset = "X-Goog-Upload-Offset: 0";
  char *upload_command = "X-Goog-Upload-Command: upload, finalize";
  char *content_type = "Content-Type: image/png";
  char length[512];

  snprintf(auth_header, sizeof(auth_header), "%s %s", api_key, gemini_api_key);
  snprintf(length, sizeof(length), "%s %ld", content_length, image_len);

  list = curl_slist_append(list, auth_header);
  list = curl_slist_append(list, length);
  list = curl_slist_append(list, upload_offset);
  list = curl_slist_append(list, upload_command);
  list = curl_slist_append(list, content_type);

  curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 5000L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
  curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);

  curl_easy_setopt(curl, CURLOPT_URL, upload_url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, image_data);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)image_len);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&mem);

  curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert-2025-09-09.pem");

  curl_easy_perform(curl);

  char *result_uri = NULL;
  cJSON *parsed_json = cJSON_Parse(mem.response);
  cJSON *file = cJSON_GetObjectItemCaseSensitive(parsed_json, "file");
  cJSON *uri = cJSON_GetObjectItemCaseSensitive(file, "uri");
  result_uri = strdup(uri->valuestring);

  cJSON_Delete(parsed_json);

  curl_slist_free_all(list);
  curl_easy_cleanup(curl);
  free(mem.response);

  return result_uri;
}

char *gemini_request(char *gemini_url, char **file_uris, char *gemini_api_key,
                     char *fullPrompt, char **file_mime_types, int file_count) {
  Memory mem = {malloc(1), 0};

  cJSON *req_body_json = cJSON_CreateObject();
  cJSON *contents = cJSON_CreateArray();
  cJSON_AddItemToObject(req_body_json, "contents", contents);

  cJSON *content = cJSON_CreateObject();
  cJSON_AddItemToArray(contents, content);
  cJSON *parts = cJSON_CreateArray();
  cJSON_AddItemToObject(content, "parts", parts);

  for (size_t i = 0; i < file_count; i++) {
    cJSON *part_file = cJSON_CreateObject();
    cJSON *file_data = cJSON_CreateObject();
    cJSON_AddItemToObject(part_file, "file_data", file_data);
    cJSON_AddStringToObject(file_data, "mime_type", file_mime_types[i]);
    cJSON_AddStringToObject(file_data, "file_uri", file_uris[i]);
    cJSON_AddItemToArray(parts, part_file);
  }

  cJSON *part_text = cJSON_CreateObject();
  cJSON_AddStringToObject(part_text, "text", fullPrompt);
  cJSON_AddItemToArray(parts, part_text);

  char *req_body_json_str = cJSON_Print(req_body_json);

  CURL *curl = curl_easy_init();
  if (curl) {
    struct curl_slist *list = NULL;
    char api_key[512];
    char *header_name = "x-goog-api-key:";
    char *content_type = "Content-Type: application/json";

    snprintf(api_key, sizeof(api_key), "%s %s", header_name, gemini_api_key);

    list = curl_slist_append(list, api_key);
    list = curl_slist_append(list, content_type);

    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 5000L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);

    curl_easy_setopt(curl, CURLOPT_URL, gemini_url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body_json_str);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                     (long)strlen(req_body_json_str));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&mem);

    curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert-2025-09-09.pem");

    curl_easy_perform(curl);

    cJSON *mem_res = cJSON_Parse(mem.response);
    cJSON *candidates = cJSON_GetObjectItemCaseSensitive(mem_res, "candidates");
    cJSON *first_candidate = cJSON_GetArrayItem(candidates, 0);
    cJSON *content_obj =
        cJSON_GetObjectItemCaseSensitive(first_candidate, "content");
    cJSON *parts_obj = cJSON_GetObjectItemCaseSensitive(content_obj, "parts");
    cJSON *first_part = cJSON_GetArrayItem(parts_obj, 0);
    cJSON *text = cJSON_GetObjectItemCaseSensitive(first_part, "text");

    char *gemini_response = NULL;
    char *cleaned_text = replace_escaped_ansi(text->valuestring);
    gemini_response = strdup(cleaned_text);

    cJSON_Delete(mem_res);

    free(req_body_json_str);
    cJSON_Delete(req_body_json);
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    free(mem.response);
    free(cleaned_text);

    return gemini_response;
  }

  free(req_body_json_str);
  cJSON_Delete(req_body_json);
  free(mem.response);
  curl_easy_cleanup(curl);

  return NULL;
}

int main(void) {
  puts(R"(
        ::::::::  :::    :::  ::::::::   ::::::::  :::::::::: ::::::::   :::::::: 
      :+:    :+: :+:    :+: :+:    :+: :+:    :+: :+:       :+:    :+: :+:    :+: 
     +:+        +:+    +:+ +:+        +:+        +:+       +:+        +:+         
    +#++:++#++ +#+    +:+ +#+        +#+        +#++:++#  +#++:++#++ +#++:++#++   
          +#+ +#+    +#+ +#+        +#+        +#+              +#+        +#+    
  #+#    #+# #+#    #+# #+#    #+# #+#    #+# #+#       #+#    #+# #+#    #+#     
  ########   ########   ########   ########  ########## ########   ########       
  )");

  setvbuf(stdout, NULL, _IONBF, 0);

#ifdef _WIN32
  enableVirtualTerminal();
#endif

  char *env_json = read_file("env.json");

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
  cJSON *gemini_file_url =
      cJSON_GetObjectItemCaseSensitive(env, "GEMINI_FILE_URL");
  if (!gemini_file_url->valuestring) {
    fprintf(stderr, "GEMINI_FILE_URL environment variable not set.\n");
  }

  char *systemPrompt =
      "This is running on a terminal so format it to look good, no "
      "markdown formatting like bolds with double asterisk cause im getting "
      "the "
      "response and viewing it via terminal and no markdown formatting "
      "will work, also format it with colors with ANSI format like this "
      "\\033[97m ascii utlize the background and foreground colors, bolds, "
      "italics, etc.";
  char userPrompt[256];
  char fullPrompt[512];

  nfdresult_t nfd_res = NFD_CANCEL;
  nfdpathset_t pathSet = {0};

  while (1) {
    int total_file_num = 0;
    char **exts = NULL;
    char **file_uris = NULL;
    char *res_gemini_req = NULL;

    printf("Enter your prompt [1 to attach files, enter 0 to exit]: ");
    if (fgets(userPrompt, sizeof(userPrompt), stdin) != NULL) {
      userPrompt[strcspn(userPrompt, "\n")] = '\0';

      if (strcmp(userPrompt, "0") == 0) {
        printf("[INFO] Exited\n");
        break;
      } else if (strcmp(userPrompt, "1") == 0) {
        nfd_res = NFD_OpenDialogMultiple("png,jpeg,jpg,pdf", NULL, &pathSet);

        for (size_t i = 0; i < NFD_PathSet_GetCount(&pathSet); ++i) {
          nfdchar_t *path = NFD_PathSet_GetPath(&pathSet, i);
          printf("Path %i: %s\n", (int)i, path);
        }

        continue;
      }
    }
    snprintf(fullPrompt, 512, "System Prompt: %s\nUser Prompt: %s",
             systemPrompt, userPrompt);

    pthread_t generate_thread = {0};

    curl_global_init(CURL_GLOBAL_DEFAULT);

    if (nfd_res == NFD_OKAY) {
      is_generating = true;
      pthread_create(&generate_thread, NULL, geminiLoading, NULL);

      total_file_num = NFD_PathSet_GetCount(&pathSet);

      int capacity = 0;

      for (size_t i = 0; i < total_file_num; ++i) {
        nfdchar_t *path = NFD_PathSet_GetPath(&pathSet, i);

        size_t encoded_len;
        unsigned char *file_data = read_file_b64(path, &encoded_len);
        const char *ext = get_file_mime_type(path);

        char *res_upload_url =
            get_upload_url(encoded_len, gemini_file_url->valuestring,
                           gemini_api_key->valuestring, (char *)ext);
        char *res_file_uri =
            get_file_uri(file_data, encoded_len, path, res_upload_url,
                         gemini_api_key->valuestring);

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

      is_generating = false;
      pthread_cancel(generate_thread);
      pthread_join(generate_thread, NULL);

      for (size_t j = 0; j < total_file_num; j++) {
        printf("ext %zu: %s\n", j + 1, exts[j]);
        printf("file_uri %zu: %s\n", j + 1, file_uris[j]);
      }
    } else if (nfd_res == NFD_CANCEL) {
      puts("User pressed cancel.");
    } else {
      printf("Error: %s\n", NFD_GetError());
    }

    bool query_with_file = total_file_num > 0;

    res_gemini_req = gemini_request(
        gemini_api_url->valuestring, query_with_file ? file_uris : NULL,
        gemini_api_key->valuestring, fullPrompt, query_with_file ? exts : NULL,
        total_file_num);

    printf("\033[97mGemini response:\n%s\n", res_gemini_req);

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
