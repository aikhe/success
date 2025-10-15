.PHONY: all run sanitize sanitize-run clean 

ifeq ($(OS),Windows_NT)
all:
	gcc -Wall -g -D__USE_MINGW_ANSI_STDIO=1 prototype.c -I"./include" -L"./lib" -lcurl -lcjson -lpthread -o prototype

run: all
	.\prototype
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
