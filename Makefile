RM = powershell -Command "Remove-Item -Force -ErrorAction SilentlyContinue"

all:
	gcc -Wall -g prototype.c -I"./include" -L"./lib" -lcurl -o prototype

run: all
	.\prototype

clean:
	del /Q prototype.exe
