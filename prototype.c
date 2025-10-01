#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define QUOTE(...) #__VA_ARGS__

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

  printf("total chunk total_bytes: %zu\n", total_bytes);
  printf("%d:\t", lineNum);
  for (int i = 0; i < total_bytes; i++) {
    printf("%c", ptr[i]);
    if (ptr[i] == '\n') {
      lineNum++;
      printf("%d:\t", lineNum);
    }
  }
  printf("\n\n");
  // fwrite(ptr, size, nmemb, stdout);

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
  printf("%s\n", env_json);
  cJSON *env = cJSON_Parse(env_json);
  if (!env) {
    const char *error_ptr = cJSON_GetErrorPtr();

    if (error_ptr) {
      fprintf(stderr, "[ERROR] Error parsing JSON at %s\n", error_ptr);
    }

    free(env);
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
  printf("API: %s\n", gemini_api_key->valuestring);
  printf("URL: %s\n", gemini_api_url->valuestring);

  CURL *curl;
  CURLcode res;

  Memory mem = {malloc(1), 0};

  char *gemini_header_json = read_file("gemini_header.json");
  printf("%s\n", gemini_header_json);

  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();

  printf("\ncurl handle address: %p\n\n", curl);

  if (curl) {
    struct curl_slist *list = NULL;
    char auth_header[512];
    char *header_name = "x-goog-api-key:";
    char *content_type = "Content-Type: application/json";

    snprintf(auth_header, sizeof(auth_header), "%s %s", header_name,
             gemini_api_key->valuestring);
    printf("%s\n", auth_header);

    list = curl_slist_append(list, auth_header);
    list = curl_slist_append(list, content_type);

    /* Optional: reduce internal buffering to get data to you faster */
    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 10240L); /* 10KB buffer */

    /* Useful timeouts to avoid hanging indefinitely */
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L); /* connection timeout */
    /* Do NOT set an overall CURLOPT_TIMEOUT if you expect the stream to be open
       for long. If you want an overall limit, set CURLOPT_TIMEOUT to a large
       value. */

    /* TCP_NODELAY can reduce latency for small data */
    curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);

    // curl_easy_setopt(curl, CURLOPT_URL, "https://jacobsorber.com");
    curl_easy_setopt(curl, CURLOPT_URL, gemini_api_url->valuestring);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, gemini_header_json);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                     (long)strlen(gemini_header_json));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&mem);

    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert-2025-09-09.pem");

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed. %s\n",
              curl_easy_strerror(res));
    } else {
      printf("\nFrom mem->response:\n%s\n", mem.response);
    }

    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
  } else {
    fprintf(stderr, "Curl initialization failed.\n");
  }

  cJSON_Delete(env);
  free(gemini_header_json);
  free(env_json);
  free(mem.response);
  curl_global_cleanup();

  return EXIT_SUCCESS;
}
