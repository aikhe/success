#ifndef GETFILEURI_H
#define GETFILEURI_H

#include "../callbacks/write_callback.h"
#include "../types/types.h"

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdlib.h>

char *get_file_uri(unsigned char *image_data, long int image_len,
                   char *image_path, char *upload_url, char *gemini_api_key,
                   char *file_mime_type);

#endif
