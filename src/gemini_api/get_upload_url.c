#include "get_upload_url.h"

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

  // verbose logging
  // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

  curl_easy_setopt(curl, CURLOPT_URL, gemini_file_url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_json);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(req_json));
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&mem);

  curl_easy_setopt(curl, CURLOPT_CAINFO, "../cacert-2025-09-09.pem");

  curl_easy_perform(curl);

  char *res_url = grep_string(mem.response);

  // printf("res_url: %s\n", res_url);

  curl_slist_free_all(list);
  free(mem.response);
  curl_easy_cleanup(curl);

  return res_url;
}
