.PHONY: all run sanitize sanitize-run clean 

ifeq ($(OS),Windows_NT)
all:
	chcp 65001
	gcc -Wall -g -D__USE_MINGW_ANSI_STDIO=1 main.c -I"../include" -L"../lib" -lcurl -lcjson -lpthread ../lib/nfd.lib -lole32 -luuid -lpdcurses -o main.exe

run: all
	.\main.exe 2>&1
else
	# gcc prototype.c -o prototype -pthread -lcurl -lcjson ./lib/libnfd.a $(pkg-config --cflags --libs gtk+-3.0) ./base64.c
all:
	gcc function_calling.c -o function_calling -pthread -lcurl -lcjson ./lib/libnfd.a $$(pkg-config --cflags --libs gtk+-3.0) ./base64.c
run: all
	./function_calling
endif

sanitize:
	gcc -g -fsanitize=address -fno-omit-frame-pointer prototype.c -lcurl -lcjson -pthread -o prototype

sanitize-run: sanitize
	make run

clean:
	rm -f prototype prototype.exe
