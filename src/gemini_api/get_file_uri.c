#include "get_file_uri.h"

char *get_file_uri(unsigned char *image_data, long int image_len,
                   char *image_path, char *upload_url, char *gemini_api_key,
                   char *file_mime_type) {
  Memory mem = {malloc(1), 0};

  CURL *curl = curl_easy_init();

  struct curl_slist *list = NULL;
  char auth_header[512];
  char *api_key = "x-goog-api-key:";
  char *content_length = "Content-Length:";
  char *upload_offset = "X-Goog-Upload-Offset: 0";
  char *upload_command = "X-Goog-Upload-Command: upload, finalize";
  char content_type[512];
  char length[512];

  snprintf(auth_header, sizeof(auth_header), "%s %s", api_key, gemini_api_key);
  snprintf(length, sizeof(length), "%s %ld", content_length, image_len);
  snprintf(content_type, sizeof(length), "%s %s",
           "Content-Type:", file_mime_type);

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

  curl_easy_setopt(curl, CURLOPT_CAINFO, "../cacert-2025-09-09.pem");

  curl_easy_perform(curl);

  // printf("GET FILE URI:\n%s\n", mem.response);

  char *result_uri = NULL;
  cJSON *parsed_json = cJSON_Parse(mem.response);
  cJSON *file = cJSON_GetObjectItemCaseSensitive(parsed_json, "file");
  // char *req_body_json_str = cJSON_Print(file);
  // printf("im here uri: %s\n", req_body_json_str);
  cJSON *uri = cJSON_GetObjectItemCaseSensitive(file, "uri");
  result_uri = strdup(uri->valuestring);

  // printf("im here uri: %s\n", uri->valuestring);

  cJSON_Delete(parsed_json);

  curl_slist_free_all(list);
  curl_easy_cleanup(curl);
  free(mem.response);

  return result_uri;
}
