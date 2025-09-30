#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <memoryapi.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

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
  size_t totalBytes = size * nmemb;
  struct Memory *mem = (struct Memory *)userdata;
  int lineNum = 1;

  char *temp = realloc(mem->response, mem->size + totalBytes + 1);
  if (!ptr) {
    fprintf(stderr, "[ERROR] Failed to realloc memory. \n");
    return 0;
  }

  mem->response = temp;
  memcpy(&(mem->response[mem->size]), ptr, totalBytes);
  mem->size += totalBytes;
  mem->response[mem->size] = '\0';

  printf("total chunk totalBytes: %zu\n", totalBytes);
  printf("%d:\t", lineNum);
  for (int i = 0; i < totalBytes; i++) {
    printf("%c", ptr[i]);
    if (ptr[i] == '\n') {
      lineNum++;
      printf("%d:\t", lineNum);
    }
  }
  printf("\n\n");
  // fwrite(ptr, size, nmemb, stdout);

  return totalBytes;
}

int main(void) {
  bool is_success = true;

  if (!is_success) {
    fprintf(stderr, "[ERROR] Operation failed. \n");
    return EXIT_FAILURE;
  }

  printf("[INFO] Success succeeded!\n\n");

  char *json_data = read_file("env.json");
  cJSON *root = cJSON_Parse(json_data);

  cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
  printf("Name: %s\n", name->valuestring);

  CURL *curl;
  CURLcode res;

  Memory mem = {malloc(1), 0};

  curl = curl_easy_init();

  printf("curl handle address: %p\n\n", curl);
  if (curl) {
    // curl_easy_setopt(curl, CURLOPT_URL, "https://jacobsorber.com");
    curl_easy_setopt(curl, CURLOPT_URL, "https://example.com");
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

    curl_easy_cleanup(curl);
  } else {
    fprintf(stderr, "Curl initialization failed.\n");
  }

  free(mem.response);
  return EXIT_SUCCESS;
}
