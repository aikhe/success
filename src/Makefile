CC = gcc
CFLAGS = -Wall -g
PROGRAM ?= main
SRC = main.c utils/get_file_mime_type.c utils/read_file.c utils/replace_escaped_ansii.c utils/read_file_b64.c utils/gemini_loading.c callbacks/write_callback.c

# OBJ = $(SRC:.c=.o)
BUILD_DIR = builds

ifeq ($(OS),Windows_NT)
TARGET := $(BUILD_DIR)\\$(PROGRAM)
LIB = -lcurl -lcjson -lpthread ../lib/nfd.lib -lole32 -luuid -lpdcurses
RM = del /F /Q
RMDIR = rmdir /S /Q
MKDIR = mkdir
else
TARGET := $(BUILD_DIR)/$(PROGRAM)
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
