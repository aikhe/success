#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define QUOTE(...) #__VA_ARGS__ // pre-processor to turn content into string

typedef struct Memory {
  char *response;
  size_t size;
} Memory;

char *read_file(const char *filename) {
  FILE *fptr = fopen(filename, "rb");
  if (!fptr)
    return NULL;

  fseek(fptr, 0, SEEK_END);  // move position indicator to end
  long length = ftell(fptr); // get length
  rewind(fptr);              // move position indicator to end

  char *data = malloc(length + 1); // add 1 for null terminator
  fread(data, 1, length, fptr);    // this is where data finally gets read
  data[length] =
      '\0'; // add null terminator (crucial for printf & other string functions)
  fclose(fptr);

  return data;
}

static size_t write_callback(char *ptr, size_t size, size_t nmemb,
                             void *userdata) {
  size_t total_bytes = size * nmemb;
  struct Memory *mem = (struct Memory *)userdata;
  int lineNum = 1;

  char *temp = realloc(mem->response, mem->size + total_bytes + 1);
  if (!ptr) {
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

int main(void) {
  bool is_success = true;

  if (!is_success) {
    fprintf(stderr, "[ERROR] Operation failed. \n");
    return EXIT_FAILURE;
  }
  printf("[INFO] Success succeeded!\n\n");

  char *env_json = read_file("env.json");
  printf("[DEBUG] env.json:\n%s\n", env_json);
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
  // printf("API: %s\n", gemini_api_key->valuestring);
  // printf("URL: %s\n", gemini_api_url->valuestring);

  CURL *curl;
  CURLcode res;

  Memory mem = {malloc(1), 0};

  char *req_body = read_file("gemini_req_body.json");
  char userPrompt[256];
  printf("Enter your prompt: ");
  if (fgets(userPrompt, sizeof(userPrompt), stdin) != NULL) {
    userPrompt[strcspn(userPrompt, "\n")] = '\0'; // remove newline

    printf("User prompt: %s\n\n", userPrompt);
  }
  cJSON *req_body_json = cJSON_Parse(req_body);
  cJSON *contents = cJSON_GetObjectItemCaseSensitive(req_body_json, "contents");
  cJSON *contents_arr = cJSON_GetArrayItem(contents, 0);
  cJSON *parts = cJSON_GetObjectItemCaseSensitive(contents_arr, "parts");
  cJSON *parts_arr = cJSON_GetArrayItem(parts, 0);
  cJSON *text = cJSON_GetObjectItemCaseSensitive(parts_arr, "text");
  cJSON_SetValuestring(text, userPrompt);

  char *req_body_json_str = cJSON_PrintUnformatted(req_body_json);
  printf("Request body:\n");
  printf("%s\n", req_body);
  printf("Updated request body:\n");
  printf("%s\n\n", req_body_json_str);

  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();

  // printf("\ncurl handle address: %p\n\n", curl);

  if (curl) {
    struct curl_slist *list = NULL;
    char auth_header[512];
    char *header_name = "x-goog-api-key:";
    char *content_type = "Content-Type: application/json";

    snprintf(auth_header, sizeof(auth_header), "%s %s", header_name,
             gemini_api_key->valuestring);
    // printf("%s\n", auth_header);

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
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body_json_str);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                     (long)strlen(req_body_json_str));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&mem);

    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // removes SSL
    // verification
    curl_easy_setopt(curl, CURLOPT_CAINFO,
                     "cacert-2025-09-09.pem"); // set cacert for ssl

    res = curl_easy_perform(curl);

    cJSON *mem_res = cJSON_Parse(mem.response);
    cJSON *candidates = cJSON_GetObjectItemCaseSensitive(mem_res, "candidates");
    cJSON *first_candidate = cJSON_GetArrayItem(candidates, 0);
    cJSON *content =
        cJSON_GetObjectItemCaseSensitive(first_candidate, "content");
    cJSON *parts = cJSON_GetObjectItemCaseSensitive(content, "parts");
    cJSON *first_part = cJSON_GetArrayItem(parts, 0);
    cJSON *text = cJSON_GetObjectItemCaseSensitive(first_part, "text");

    printf("Gemini response: %s\n", text->valuestring);
    cJSON_Delete(mem_res);

    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed. %s\n",
              curl_easy_strerror(res));
    } else {
      // printf("\nmem->response address: %p\n", &mem);
      // printf("\nFrom mem->response:\n%s\n", mem.response);
    }

    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
  } else {
    fprintf(stderr, "Curl initialization failed.\n");
  }

  free(env_json);
  cJSON_Delete(env);
  free(req_body);
  free(mem.response);
  curl_global_cleanup();

  printf("\n[INFO] Program finished. Press ENTER to exit...\n");
  getchar();

  return EXIT_SUCCESS;
}
