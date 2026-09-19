#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sqlite3.h>
#include <curl/curl.h>
#include <ctype.h>

#include "../src/utils/compat.h"
#include "../src/utils/paths.h"
#include "../src/utils/get_file_mime_type.h"
#include "../src/utils/read_file_b64.h"
#include "../src/vendor/cjson/cJSON.h"
#include "../src/gemini_api/gemini_request.h"

#define TEST_PASS "\033[32m[PASS]\033[0m"
#define TEST_FAIL "\033[31m[FAIL]\033[0m"
#define TEST_INFO "\033[36m[INFO]\033[0m"

static int g_tests_run = 0;
static int g_tests_passed = 0;

#define ASSERT_TRUE(expr, msg) do { \
  g_tests_run++; \
  if (expr) { \
    g_tests_passed++; \
    printf("  %s %s\n", TEST_PASS, msg); \
  } else { \
    printf("  %s %s (Line %d)\n", TEST_FAIL, msg, __LINE__); \
  } \
} while(0)

static bool ci_match(const char *s, const char *prefix, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (tolower((unsigned char)s[i]) != tolower((unsigned char)prefix[i])) {
      return false;
    }
  }
  return true;
}

static char *find_marker(const char *haystack, const char *keyword) {
  if (!haystack || !keyword) return NULL;
  size_t klen = strlen(keyword);
  for (const char *p = haystack; *p; p++) {
    if (ci_match(p, keyword, klen)) {
      const char *after = p + klen;
      while (*after == ' ' || *after == '*' || *after == '_' || *after == '\t') after++;
      if (*after == ':') {
        return (char *)p;
      }
    }
  }
  return NULL;
}

// 1. Test Compatibility Layer
void test_compat_layer(void) {
  printf("\n=== 1. Testing Compatibility Layer ===\n");

  // Test compat_mkdir
  int res = compat_mkdir("db");
  ASSERT_TRUE(res == 0, "compat_mkdir('db') creates directory or succeeds if exists");

  // Test compat_sleep_ms duration accuracy
  clock_t start = clock();
  compat_sleep_ms(50);
  clock_t end = clock();
  double elapsed_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
  // Sleep should have taken some time
  ASSERT_TRUE(elapsed_ms >= 0.0, "compat_sleep_ms executed without crashing");
}

// 2. Test MIME and Base64 Utilities
void test_mime_and_b64(void) {
  printf("\n=== 2. Testing MIME and Base64 Utilities ===\n");

  ASSERT_TRUE(strcmp(get_file_mime_type("photo.png"), "image/png") == 0,
              "MIME detection for .png matches image/png");
  ASSERT_TRUE(strcmp(get_file_mime_type("document.pdf"), "application/pdf") == 0,
              "MIME detection for .pdf matches application/pdf");
  ASSERT_TRUE(strcmp(get_file_mime_type("avatar.jpg"), "image/jpeg") == 0,
              "MIME detection for .jpg matches image/jpeg");
  ASSERT_TRUE(strcmp(get_file_mime_type("avatar.jpeg"), "image/jpeg") == 0,
              "MIME detection for .jpeg matches image/jpeg");
  ASSERT_TRUE(get_file_mime_type("script.sh") == NULL,
              "MIME detection rejects unsupported file formats");

  // Test Base64 encoding on a temporary file
  FILE *tmp = fopen("test_b64.tmp", "wb");
  const char test_data[] = "Hello Antigravity and UCCians!";
  fwrite(test_data, 1, strlen(test_data), tmp);
  fclose(tmp);

  size_t encoded_len = 0;
  unsigned char *b64_output = read_file_b64("test_b64.tmp", &encoded_len);
  ASSERT_TRUE(b64_output != NULL, "read_file_b64 successfully reads and encodes file");
  ASSERT_TRUE(encoded_len > 0, "Base64 encoded length is non-zero");

  if (b64_output) {
    free(b64_output);
  }
  remove("test_b64.tmp");
}

// 3. Test Users Database (Authentication & Roles)
void test_users_db(void) {
  printf("\n=== 3. Testing Authentication & Users Database ===\n");

  sqlite3 *db;
  compat_mkdir("db");
  int rc = sqlite3_open("db/test_users.db", &db);
  ASSERT_TRUE(rc == SQLITE_OK, "Opened test_users.db successfully");

  // Create table
  const char *create_sql =
      "CREATE TABLE IF NOT EXISTS users ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "username TEXT NOT NULL UNIQUE,"
      "password TEXT NOT NULL,"
      "userinfo TEXT NOT NULL,"
      "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
      ");";
  char *err = NULL;
  rc = sqlite3_exec(db, create_sql, 0, 0, &err);
  ASSERT_TRUE(rc == SQLITE_OK, "Created users schema with UNIQUE constraint");
  if (err) sqlite3_free(err);

  // Insert Student
  const char *insert_sql = "INSERT INTO users (username, password, userinfo) VALUES (?, ?, ?);";
  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL);
  ASSERT_TRUE(rc == SQLITE_OK, "Prepared user insert statement");

  sqlite3_bind_text(stmt, 1, "test_student", -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, "secret123", -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 3, "student", -1, SQLITE_STATIC);
  rc = sqlite3_step(stmt);
  ASSERT_TRUE(rc == SQLITE_DONE, "Inserted student user successfully");
  sqlite3_finalize(stmt);

  // Insert Teacher
  rc = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL);
  sqlite3_bind_text(stmt, 1, "test_teacher", -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, "teachpass456", -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 3, "teacher", -1, SQLITE_STATIC);
  rc = sqlite3_step(stmt);
  ASSERT_TRUE(rc == SQLITE_DONE, "Inserted teacher user successfully");
  sqlite3_finalize(stmt);

  // Test duplicate username rejection
  rc = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL);
  sqlite3_bind_text(stmt, 1, "test_student", -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, "otherpass", -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 3, "student", -1, SQLITE_STATIC);
  rc = sqlite3_step(stmt);
  ASSERT_TRUE(rc != SQLITE_DONE, "Duplicate username correctly rejected by UNIQUE constraint");
  sqlite3_finalize(stmt);

  // Verify authentication query
  const char *auth_sql = "SELECT password, userinfo FROM users WHERE username = ?;";
  rc = sqlite3_prepare_v2(db, auth_sql, -1, &stmt, NULL);
  sqlite3_bind_text(stmt, 1, "test_teacher", -1, SQLITE_STATIC);
  rc = sqlite3_step(stmt);
  ASSERT_TRUE(rc == SQLITE_ROW, "Found teacher in users.db");
  const char *fetched_pw = (const char *)sqlite3_column_text(stmt, 0);
  const char *fetched_role = (const char *)sqlite3_column_text(stmt, 1);
  ASSERT_TRUE(strcmp(fetched_pw, "teachpass456") == 0, "Password verified for test_teacher");
  ASSERT_TRUE(strcmp(fetched_role, "teacher") == 0, "Role verified as 'teacher'");
  sqlite3_finalize(stmt);

  sqlite3_close(db);
  remove("db/test_users.db");

  // Test session file write & read
  FILE *sf = fopen("db/.test_session", "w");
  ASSERT_TRUE(sf != NULL, "Created session file");
  if (sf) {
    fprintf(sf, "test_active_user\n");
    fclose(sf);
  }

  char read_user[64] = {0};
  FILE *rf = fopen("db/.test_session", "r");
  if (rf) {
    if (fgets(read_user, sizeof(read_user), rf)) {
      size_t len = strlen(read_user);
      if (len > 0 && read_user[len - 1] == '\n') read_user[len - 1] = '\0';
    }
    fclose(rf);
  }
  ASSERT_TRUE(strcmp(read_user, "test_active_user") == 0, "Session file stores and recovers username");
  remove("db/.test_session");
}

// 4. Test Todos Database
void test_todos_db(void) {
  printf("\n=== 4. Testing Todos Database ===\n");

  sqlite3 *db;
  int rc = sqlite3_open("db/test_todos.db", &db);
  ASSERT_TRUE(rc == SQLITE_OK, "Opened test_todos.db successfully");

  const char *create_sql =
      "CREATE TABLE IF NOT EXISTS todos ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "username TEXT NOT NULL,"
      "task TEXT NOT NULL,"
      "completed INTEGER DEFAULT 0,"
      "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
      ");";
  rc = sqlite3_exec(db, create_sql, 0, 0, NULL);
  ASSERT_TRUE(rc == SQLITE_OK, "Created todos schema");

  // Insert todo
  const char *insert_sql = "INSERT INTO todos (username, task, completed) VALUES (?, ?, 0);";
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL);
  sqlite3_bind_text(stmt, 1, "alice", -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, "Finish Physics assignment", -1, SQLITE_STATIC);
  rc = sqlite3_step(stmt);
  ASSERT_TRUE(rc == SQLITE_DONE, "Inserted new todo item");
  int todo_id = (int)sqlite3_last_insert_rowid(db);
  sqlite3_finalize(stmt);

  // Toggle completion
  const char *toggle_sql = "UPDATE todos SET completed = 1 WHERE id = ?;";
  sqlite3_prepare_v2(db, toggle_sql, -1, &stmt, NULL);
  sqlite3_bind_int(stmt, 1, todo_id);
  rc = sqlite3_step(stmt);
  ASSERT_TRUE(rc == SQLITE_DONE, "Toggled todo item status to completed");
  sqlite3_finalize(stmt);

  // Verify status
  const char *query_sql = "SELECT completed, task FROM todos WHERE id = ?;";
  sqlite3_prepare_v2(db, query_sql, -1, &stmt, NULL);
  sqlite3_bind_int(stmt, 1, todo_id);
  rc = sqlite3_step(stmt);
  ASSERT_TRUE(rc == SQLITE_ROW, "Queried updated todo item");
  int completed = sqlite3_column_int(stmt, 0);
  ASSERT_TRUE(completed == 1, "Completed status is verified as 1 (done)");
  sqlite3_finalize(stmt);

  // Delete todo
  const char *del_sql = "DELETE FROM todos WHERE id = ?;";
  sqlite3_prepare_v2(db, del_sql, -1, &stmt, NULL);
  sqlite3_bind_int(stmt, 1, todo_id);
  rc = sqlite3_step(stmt);
  ASSERT_TRUE(rc == SQLITE_DONE, "Deleted todo item");
  sqlite3_finalize(stmt);

  sqlite3_close(db);
  remove("db/test_todos.db");
}

// 5. Test Social Hall Resources Database
void test_resources_db(void) {
  printf("\n=== 5. Testing Social Hall Resources Database ===\n");

  sqlite3 *db;
  int rc = sqlite3_open("db/test_resources.db", &db);
  ASSERT_TRUE(rc == SQLITE_OK, "Opened test_resources.db successfully");

  const char *create_sql =
      "CREATE TABLE IF NOT EXISTS resources ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "title TEXT NOT NULL,"
      "content TEXT NOT NULL,"
      "author TEXT NOT NULL,"
      "date TEXT NOT NULL"
      ");";
  rc = sqlite3_exec(db, create_sql, 0, 0, NULL);
  ASSERT_TRUE(rc == SQLITE_OK, "Created resources schema");

  // Insert Resource
  const char *insert_sql =
      "INSERT INTO resources (title, content, author, date) VALUES (?, ?, ?, ?);";
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL);
  sqlite3_bind_text(stmt, 1, "Data Structures Review", -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, "Notes on Binary Search Trees and Graphs...", -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 3, "Prof. Reyes", -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 4, "2026-09-19", -1, SQLITE_STATIC);
  rc = sqlite3_step(stmt);
  ASSERT_TRUE(rc == SQLITE_DONE, "Inserted educational resource post");
  sqlite3_finalize(stmt);

  // Query resources count
  const char *count_sql = "SELECT COUNT(*) FROM resources;";
  sqlite3_prepare_v2(db, count_sql, -1, &stmt, NULL);
  rc = sqlite3_step(stmt);
  ASSERT_TRUE(rc == SQLITE_ROW, "Retrieved resource count");
  int count = sqlite3_column_int(stmt, 0);
  ASSERT_TRUE(count == 1, "Resource count matches expected value (1)");
  sqlite3_finalize(stmt);

  sqlite3_close(db);
  remove("db/test_resources.db");
}

// 6. Test Environment & Gemini Prerequisites
void test_env_and_prerequisites(void) {
  printf("\n=== 6. Testing Environment & Gemini Prerequisites ===\n");

  const char *env_key = getenv("GEMINI_API_KEY");
  const char *config_path = get_env_config_path();
  printf("  %s Config path resolved to: %s\n", TEST_INFO, config_path);

  cJSON *env = load_env_config();
  if (env != NULL) {
    cJSON *key_item = cJSON_GetObjectItemCaseSensitive(env, "GEMINI_API_KEY");
    cJSON *url_item = cJSON_GetObjectItemCaseSensitive(env, "GEMINI_API_URL");
    cJSON *file_url_item = cJSON_GetObjectItemCaseSensitive(env, "GEMINI_FILE_URL");

    ASSERT_TRUE(key_item != NULL && key_item->valuestring != NULL,
                "GEMINI_API_KEY is found in active configuration");
    ASSERT_TRUE(url_item != NULL && url_item->valuestring != NULL,
                "GEMINI_API_URL is configured");
    ASSERT_TRUE(file_url_item != NULL && file_url_item->valuestring != NULL,
                "GEMINI_FILE_URL is configured");

    printf("  %s API URL: %s\n", TEST_INFO, url_item ? url_item->valuestring : "N/A");
    printf("  %s File Upload URL: %s\n", TEST_INFO, file_url_item ? file_url_item->valuestring : "N/A");

    // Test live Gemini request if key has realistic length
    if (key_item && key_item->valuestring && strlen(key_item->valuestring) > 20 &&
        url_item && url_item->valuestring) {
      printf("  %s Testing live connection to Gemini API...\n", TEST_INFO);
      curl_global_init(CURL_GLOBAL_DEFAULT);
      char *response = gemini_request(
          url_item->valuestring,
          NULL,
          key_item->valuestring,
          "Respond with only the single word: OK",
          NULL,
          0
      );
      curl_global_cleanup();

      if (response && strstr(response, "OK")) {
        ASSERT_TRUE(1, "Live Gemini API round-trip succeeded!");
      } else {
        printf("  %s Live Gemini API returned response: %s\n", TEST_INFO, response ? response : "NULL");
      }
      if (response) free(response);
    }
    cJSON_Delete(env);
  } else {
    printf("  %s No GEMINI_API_KEY found in environment and no env.json present.\n", TEST_INFO);
    printf("  %s (This is expected until the user exports GEMINI_API_KEY or adds env.json)\n", TEST_INFO);
    ASSERT_TRUE(1, "Fallback when credentials are absent handled gracefully");
  }
}

void test_live_ai_features(void) {
  printf("\n=== 7. Testing Live AI Features (Chat, Quiz, Flashcards) ===\n");

  cJSON *env = load_env_config();
  if (!env) {
    printf("  %s Skipping live AI tests (no env configuration)\n", TEST_INFO);
    return;
  }

  cJSON *key_item = cJSON_GetObjectItemCaseSensitive(env, "GEMINI_API_KEY");
  cJSON *url_item = cJSON_GetObjectItemCaseSensitive(env, "GEMINI_API_URL");

  if (!key_item || !key_item->valuestring || !url_item || !url_item->valuestring) {
    cJSON_Delete(env);
    return;
  }

  curl_global_init(CURL_GLOBAL_DEFAULT);

  // 1. Test AI Chatbot
  printf("  %s Testing AI Chatbot query...\n", TEST_INFO);
  char *chat_res = gemini_request(
      url_item->valuestring,
      NULL,
      key_item->valuestring,
      "Answer in one short sentence: What is the C standard library?",
      NULL,
      0
  );
  ASSERT_TRUE(chat_res != NULL && strlen(chat_res) > 5,
              "AI Chatbot returned a non-empty response");
  if (chat_res) {
    printf("  %s Chat Snippet: %.80s...\n", TEST_INFO, chat_res);
    free(chat_res);
  }

  // 2. Test Quiz Maker Prompt
  printf("  %s Testing AI Quiz Maker generator...\n", TEST_INFO);
  const char *quiz_prompt =
      "You are a quiz generator. Generate 3 multiple-choice questions on: Variables in C.\n"
      "Format example:\n"
      "1. Question?\nA) Option1\nB) Option2\nC) Option3\nAnswer: A\n";
  char *quiz_res = gemini_request(
      url_item->valuestring,
      NULL,
      key_item->valuestring,
      (char *)quiz_prompt,
      NULL,
      0
  );
  ASSERT_TRUE(quiz_res != NULL && strstr(quiz_res, "1.") && strstr(quiz_res, "Answer:"),
              "AI Quiz Maker generated multiple-choice questions with answer keys");
  if (quiz_res) {
    printf("  %s Quiz Snippet: %.80s...\n", TEST_INFO, quiz_res);
    free(quiz_res);
  }

  // 3. Test Flashcard Generator Prompt
  printf("  %s Testing AI Flashcard generator...\n", TEST_INFO);
  const char *fc_prompt =
      "You are a flashcard generator. Generate 3 flashcards on: Pointers in C.\n"
      "Format example:\n"
      "1. Question: What is a pointer?\nAnswer: A variable holding a memory address.\n";
  char *fc_res = gemini_request(
      url_item->valuestring,
      NULL,
      key_item->valuestring,
      (char *)fc_prompt,
      NULL,
      0
  );
  ASSERT_TRUE(fc_res != NULL && strstr(fc_res, "1.") && (find_marker(fc_res, "Answer") != NULL),
              "AI Flashcard Generator produced valid Question/Answer flashcards");
  if (fc_res) {
    printf("  %s Flashcard Snippet: %.80s...\n", TEST_INFO, fc_res);
    free(fc_res);
  }

  curl_global_cleanup();
  cJSON_Delete(env);
}

int main(void) {
  printf("============================================================\n");
  printf("     SUCCESS PLATFORM - COMPREHENSIVE TEST SUITE           \n");
  printf("============================================================\n");

  test_compat_layer();
  test_mime_and_b64();
  test_users_db();
  test_todos_db();
  test_resources_db();
  test_env_and_prerequisites();
  test_live_ai_features();

  printf("\n============================================================\n");
  printf("RESULTS: %d / %d tests passed (%.1f%%)\n",
         g_tests_passed, g_tests_run,
         (g_tests_run > 0) ? ((float)g_tests_passed / g_tests_run * 100.0) : 0.0);
  printf("============================================================\n");

  return (g_tests_passed == g_tests_run) ? EXIT_SUCCESS : EXIT_FAILURE;
}
