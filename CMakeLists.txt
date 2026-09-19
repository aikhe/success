cmake_minimum_required(VERSION 3.20)
project(success C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED OFF)
set(CMAKE_C_EXTENSIONS ON)

# Source files for the full application
set(SUCCESS_SOURCES
    src/curses.c
    src/callbacks/write_callback.c
    src/features/ai_chat.c
    src/features/flashcard.c
    src/features/quiz.c
    src/features/social_hall.c
    src/features/study_timer.c
    src/features/todo.c
    src/gemini_api/gemini_request.c
    src/gemini_api/get_file_uri.c
    src/gemini_api/get_upload_url.c
    src/pages/introduction.c
    src/pages/menu.c
    src/pages/tools.c
    src/utils/compat.c
    src/utils/delay.c
    src/utils/gemini_loading.c
    src/utils/get_file_mime_type.c
    src/utils/grep_string.c
    src/utils/nfd_compat.c
    src/utils/paths.c
    src/utils/read_file.c
    src/utils/read_file_b64.c
    src/utils/replace_escaped_ansii.c
    src/vendor/cjson/cJSON.c
)

add_executable(success ${SUCCESS_SOURCES})

# Target Include Directories
target_include_directories(success PRIVATE
    include
    src
    src/vendor/cjson
)

# Common Definitions
target_compile_definitions(success PRIVATE _XOPEN_SOURCE_EXTENDED=1)

# Threading
find_package(Threads REQUIRED)
target_link_libraries(success PRIVATE Threads::Threads)

# Libcurl
find_package(CURL REQUIRED)
target_link_libraries(success PRIVATE CURL::libcurl)

# SQLite3
find_package(SQLite3 REQUIRED)
target_link_libraries(success PRIVATE SQLite::SQLite3)

# Curses Terminal Library
if(WIN32)
    # On Windows, link PDCurses
    find_library(PDCURSES_LIB NAMES pdcurses pdcursesw PATHS ${CMAKE_SOURCE_DIR}/lib)
    if(PDCURSES_LIB)
        target_link_libraries(success PRIVATE ${PDCURSES_LIB})
    else()
        target_link_libraries(success PRIVATE -lpdcursesw)
    endif()
    target_link_libraries(success PRIVATE comdlg32 ws2_32 ole32 uuid winmm gdi32 user32)
else()
    # On Linux / macOS / BSD, find native ncursesw
    set(CURSES_NEED_NCURSES TRUE)
    set(CURSES_NEED_WIDE TRUE)
    find_package(Curses REQUIRED)
    target_include_directories(success PRIVATE ${CURSES_INCLUDE_DIRS})
    target_link_libraries(success PRIVATE ${CURSES_LIBRARIES})
endif()

# Compiler Warning Flags
if(MSVC)
    target_compile_options(success PRIVATE /W4)
else()
    target_compile_options(success PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# Standalone CLI test binary (main.c)
set(SUCCESS_CLI_SOURCES
    src/main.c
    src/callbacks/write_callback.c
    src/gemini_api/gemini_request.c
    src/gemini_api/get_file_uri.c
    src/gemini_api/get_upload_url.c
    src/pages/introduction.c
    src/pages/menu.c
    src/pages/tools.c
    src/features/ai_chat.c
    src/features/flashcard.c
    src/features/quiz.c
    src/features/social_hall.c
    src/features/study_timer.c
    src/features/todo.c
    src/utils/compat.c
    src/utils/delay.c
    src/utils/gemini_loading.c
    src/utils/get_file_mime_type.c
    src/utils/grep_string.c
    src/utils/nfd_compat.c
    src/utils/paths.c
    src/utils/read_file.c
    src/utils/read_file_b64.c
    src/utils/replace_escaped_ansii.c
    src/vendor/cjson/cJSON.c
)

add_executable(success_cli ${SUCCESS_CLI_SOURCES})
target_include_directories(success_cli PRIVATE include src src/vendor/cjson)
target_compile_definitions(success_cli PRIVATE _XOPEN_SOURCE_EXTENDED=1)
target_link_libraries(success_cli PRIVATE Threads::Threads CURL::libcurl SQLite::SQLite3)

if(WIN32)
    if(PDCURSES_LIB)
        target_link_libraries(success_cli PRIVATE ${PDCURSES_LIB})
    else()
        target_link_libraries(success_cli PRIVATE -lpdcursesw)
    endif()
    target_link_libraries(success_cli PRIVATE comdlg32 ws2_32 ole32 uuid winmm gdi32 user32)
else()
    target_include_directories(success_cli PRIVATE ${CURSES_INCLUDE_DIRS})
    target_link_libraries(success_cli PRIVATE ${CURSES_LIBRARIES})
endif()

# Automated Test Suite (Headless)
set(TEST_SOURCES
    tests/test_suite.c
    src/utils/compat.c
    src/utils/paths.c
    src/utils/get_file_mime_type.c
    src/utils/read_file_b64.c
    src/utils/read_file.c
    src/utils/replace_escaped_ansii.c
    src/gemini_api/gemini_request.c
    src/callbacks/write_callback.c
    src/vendor/cjson/cJSON.c
)

add_executable(test_suite ${TEST_SOURCES})
target_include_directories(test_suite PRIVATE include src src/vendor/cjson)
target_link_libraries(test_suite PRIVATE Threads::Threads CURL::libcurl SQLite::SQLite3)
if(WIN32)
    target_link_libraries(test_suite PRIVATE ws2_32)
endif()

