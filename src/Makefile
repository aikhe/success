CC = gcc
CFLAGS = -Wall -g -DPDC_WIDE
PROGRAM ?= curses
SRC = $(PROGRAM).c utils/get_file_mime_type.c utils/read_file.c utils/replace_escaped_ansii.c utils/read_file_b64.c utils/gemini_loading.c callbacks/write_callback.c utils/grep_string.c gemini_api/get_upload_url.c gemini_api/get_file_uri.c gemini_api/gemini_request.c utils/delay.c pages/introduction.c pages/menu.c features/ai_chat.c features/study_timer.c pages/tools.c features/todo.c features/quiz.c features/flashcard.c features/social_hall.c
# SRC = $(PROGRAM).c pages/introduction.c pages/menu.c utils/delay.c 
# OBJ = $(SRC:.c=.o)
BUILD_DIR = builds

ifeq ($(OS),Windows_NT)
TARGET := $(PROGRAM)
LIB = -lcurl -lcjson -lpthread ../lib/nfd.lib -lole32 -luuid -lpdcursesw -lwinmm -lgdi32 -luser32 -lsqlite3
RM = del /F /Q
RMDIR = rmdir /S /Q
MKDIR = mkdir
else
TARGET := $(PROGRAM)
RM = rm
RMDIR = rm -rf
MKDIR = mkdir -pa
endif

.PHONY: all run sanitize sanitize-run clean 

ifeq ($(OS),Windows_NT)
all:
	chcp 65001
	$(CC) $(CFLAGS) -D__USE_MINGW_ANSI_STDIO=1 $(SRC) -I"../include" -I"./utils" -L"../lib" $(LIB) -o $(TARGET)

run: all
	$(TARGET)

clean:
	if exist $(TARGET).exe $(RM) $(TARGET).exe
else
all:
	gcc main.c -o main -pthread -lcurl -lcjson ./lib/libnfd.a $$(pkg-config --cflags --libs gtk+-3.0) ./base64.c
run: all
	$(TARGET)

clean:
	if exist $(TARGET) $(RM) $(TARGET)
endif

sanitize:
	gcc -g -fsanitize=address -fno-omit-frame-pointer prototype.c -lcurl -lcjson -pthread -o prototype

sanitize-run: sanitize
	make run
