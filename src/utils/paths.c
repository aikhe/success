#include "paths.h"
#include "compat.h"
#include "read_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #define F_OK 0
  #define access _access
#else
  #include <unistd.h>
#endif

const char *get_env_config_path(void) {
  if (access("env.json", F_OK) == 0) {
    return "env.json";
  }
  if (access(".env.json", F_OK) == 0) {
    return ".env.json";
  }
  if (access("../env.json", F_OK) == 0) {
    return "../env.json";
  }
  if (access("../.env.json", F_OK) == 0) {
    return "../.env.json";
  }
  return "env.json";
}

cJSON *load_env_config(void) {
  const char *env_key = getenv("GEMINI_API_KEY");
  const char *env_url = getenv("GEMINI_API_URL");
  const char *env_file_url = getenv("GEMINI_FILE_URL");

  // If environment variables are set directly, use them
  if (env_key && strlen(env_key) > 0) {
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "GEMINI_API_KEY", env_key);
    cJSON_AddStringToObject(
        obj, "GEMINI_API_URL",
        (env_url && strlen(env_url) > 0)
            ? env_url
            : "https://generativelanguage.googleapis.com/v1beta/models/"
              "gemini-2.5-flash:generateContent");
    cJSON_AddStringToObject(
        obj, "GEMINI_FILE_URL",
        (env_file_url && strlen(env_file_url) > 0)
            ? env_file_url
            : "https://generativelanguage.googleapis.com/upload/v1beta/files");
    return obj;
  }

  // Otherwise, load from env.json
  const char *config_path = get_env_config_path();
  char *content = read_file(config_path);
  if (!content) {
    return NULL;
  }

  cJSON *parsed = cJSON_Parse(content);
  free(content);
  if (!parsed) {
    return NULL;
  }

  // Supply default endpoints if omitted from env.json
  if (!cJSON_GetObjectItemCaseSensitive(parsed, "GEMINI_API_URL")) {
    cJSON_AddStringToObject(
        parsed, "GEMINI_API_URL",
        "https://generativelanguage.googleapis.com/v1beta/models/"
        "gemini-2.5-flash:generateContent");
  }
  if (!cJSON_GetObjectItemCaseSensitive(parsed, "GEMINI_FILE_URL")) {
    cJSON_AddStringToObject(
        parsed, "GEMINI_FILE_URL",
        "https://generativelanguage.googleapis.com/upload/v1beta/files");
  }

  return parsed;
}

void get_db_path(const char *dbname, char *out_path, size_t max_len) {
  compat_mkdir("db");
  snprintf(out_path, max_len, "db/%s", dbname);
}
