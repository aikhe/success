#ifndef GEMINIREQUEST_H
#define GEMINIREQUEST_H

#include "../callbacks/write_callback.h"
#include "../types/types.h"
#include "../utils/replace_escaped_ansii.h"

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdlib.h>

char *gemini_request(char *gemini_url, char **file_uris, char *gemini_api_key,
                     char *fullPrompt, char **file_mime_types, int file_count);

#endif
