#include "gemini_request.h"

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

    // verbose logging
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(curl, CURLOPT_URL, gemini_url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body_json_str);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                     (long)strlen(req_body_json_str));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&mem);

    curl_easy_setopt(curl, CURLOPT_CAINFO, "../cacert-2025-09-09.pem");

    curl_easy_perform(curl);

    // printf("%s\n", mem.response);

    cJSON *mem_res = cJSON_Parse(mem.response);
    cJSON *candidates = cJSON_GetObjectItemCaseSensitive(mem_res, "candidates");
    cJSON *first_candidate = cJSON_GetArrayItem(candidates, 0);
    cJSON *content_obj =
        cJSON_GetObjectItemCaseSensitive(first_candidate, "content");
    cJSON *parts_obj = cJSON_GetObjectItemCaseSensitive(content_obj, "parts");
    cJSON *first_part = cJSON_GetArrayItem(parts_obj, 0);
    cJSON *text = cJSON_GetObjectItemCaseSensitive(first_part, "text");

    // printf("im here\n");

    char *gemini_response = NULL;
    char *cleaned_text = replace_escaped_ansi(text->valuestring);
    gemini_response = strdup(cleaned_text);

    // printf("gemini_res: %s\n", gemini_response);

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
