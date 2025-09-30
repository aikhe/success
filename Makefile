all:
	gcc -Wall -g -D__USE_MINGW_ANSI_STDIO=1 prototype.c -I"./include" -L"./lib" -lcurl -lcjson -o prototype

run: all
	.\prototype

clean:
	del /Q prototype.exe
