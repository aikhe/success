#ifndef GETUPLOADURL_H
#define GETUPLOADURL_H

#include "../callbacks/write_callback.h"
#include "../types/types.h"
#include "../utils/grep_string.h"

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdlib.h>

char *get_upload_url(long int image_len, char *gemini_file_url,
                     char *gemini_api_key, char *file_mime_type);

#endif
