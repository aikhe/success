#ifndef PATHS_H
#define PATHS_H

#include <stddef.h>
#include <cjson/cJSON.h>

// Returns the path to env.json (e.g. "env.json" or "../env.json")
const char *get_env_config_path(void);

// Loads configuration from environment variables or env.json file
cJSON *load_env_config(void);

// Resolves a database file path and ensures parent directory exists
void get_db_path(const char *dbname, char *out_path, size_t max_len);

#endif // PATHS_H
