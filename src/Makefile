CC ?= gcc
PROGRAM ?= curses
TARGET := $(PROGRAM)

SRC = $(PROGRAM).c \
      utils/get_file_mime_type.c \
      utils/read_file.c \
      utils/replace_escaped_ansii.c \
      utils/read_file_b64.c \
      utils/gemini_loading.c \
      callbacks/write_callback.c \
      utils/grep_string.c \
      gemini_api/get_upload_url.c \
      gemini_api/get_file_uri.c \
      gemini_api/gemini_request.c \
      utils/delay.c \
      pages/introduction.c \
      pages/menu.c \
      features/ai_chat.c \
      features/study_timer.c \
      pages/tools.c \
      features/todo.c \
      features/quiz.c \
      features/flashcard.c \
      features/social_hall.c \
      utils/compat.c \
      utils/paths.c \
      utils/nfd_compat.c \
      vendor/cjson/cJSON.c

.PHONY: all run clean

ifeq ($(OS),Windows_NT)
CFLAGS = -Wall -g -D_XOPEN_SOURCE_EXTENDED=1 -D__USE_MINGW_ANSI_STDIO=1
INCLUDES = -I"../include" -I"../include/win32" -I"." -I"./utils" -I"./vendor/cjson"
LIB = -lcurl -lpthread -lole32 -luuid -lpdcursesw -lwinmm -lgdi32 -luser32 -lcomdlg32 -lsqlite3 -L"../lib"
RM = del /F /Q

all:
	chcp 65001
	$(CC) $(CFLAGS) $(SRC) $(INCLUDES) $(LIB) -o $(TARGET).exe

run: all
	$(TARGET).exe

clean:
	if exist $(TARGET).exe $(RM) $(TARGET).exe
else
CFLAGS = -Wall -g -D_XOPEN_SOURCE_EXTENDED=1 -std=gnu23
INCLUDES = -I"../include" -I"." -I"./utils" -I"./vendor/cjson"
LIB = -lncursesw -lcurl -lsqlite3 -pthread
RM = rm -f

all:
	$(CC) $(CFLAGS) $(SRC) $(INCLUDES) $(LIB) -o $(TARGET)

run: all
	./$(TARGET)

clean:
	$(RM) $(TARGET)
endif
