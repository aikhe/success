#include <curl/curl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
  size_t bytes = size * nmemb;
  int lineNum = 1;

  printf("total chunk bytes: %zu\n", bytes);
  printf("%d:\t", lineNum);
  for (int i = 0; i < bytes; i++) {
    printf("%c", ptr[i]);
    if (ptr[i] == '\n') {
      lineNum++;
      printf("%d\t", lineNum);
    }
  }
  printf("\n\n");
  // fwrite(ptr, size, nmemb, stdout);

  return bytes;
}

int main(void) {
  bool is_success = true;

  if (!is_success) {
    fprintf(stderr, "[ERROR] Operation failed. \n");
    return EXIT_FAILURE;
  }

  printf("[INFO] Success succeeded!\n\n");

  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();

  printf("curl handle address: %p\n\n", curl);
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "https://jacobsorber.com");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert-2025-09-09.pem");

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed. %s\n",
              curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
  }

  return EXIT_SUCCESS;
}
