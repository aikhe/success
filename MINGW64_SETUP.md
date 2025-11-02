# MinGW64 Setup Guide

This guide explains how to build and run this project using MinGW64 on Windows.

## Prerequisites

1. **MinGW64** - Install MinGW-w64 (MSYS2 recommended)

   - Download from: https://www.msys2.org/
   - Or use a standalone MinGW-w64 installation

2. **Make** - Usually included with MinGW64/MSYS2
   - Verify with: `make --version`

## Required Dependencies

The project requires the following libraries. Most are already included in the `lib/` directory, but you may need to install some via MSYS2 package manager.

### Libraries Already Included

These libraries are present in the `lib/` directory:

- **libcurl** - HTTP client library (libcurl.a, libcurl.dll.a)
- **cJSON** - JSON parsing library (libcjson.a)
- **PDCurses** - Terminal UI library (libpdcurses.a, libpdcursesw.a)
- **SQLite3** - Database library (libsqlite3.a)
- **pthread** - POSIX threads for Windows (libpthreadGC1.a)
- **NFD** - Native File Dialog (libnfd.a)

### Installing Missing Dependencies via MSYS2

If you're using MSYS2, you can install dependencies using pacman:

```bash
# Update package database
pacman -Syu

# Install development tools
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-pkg-config

# Install libraries (if needed for development or if rebuilding)
pacman -S mingw-w64-x86_64-curl
pacman -S mingw-w64-x86_64-sqlite3
```

**Note:** The pre-compiled libraries in `lib/` should work, but if you encounter linking issues, you may need to rebuild some libraries for MinGW64.

### DLL Dependencies

- **libcurl.dll** - May be required if using dynamic linking for curl
  - If present, copy to the same directory as your executable

**Note:** The project now uses MinGW64's built-in winpthread library, so external pthread DLLs are not needed. If you see errors about pthreadGC1 being incompatible, ensure the Makefile uses `-pthread` flag (which it does now).

## Building the Project

### Step 1: Open MinGW64/MSYS2 Terminal

- If using MSYS2: Open "MSYS2 MinGW 64-bit" terminal
- If using standalone MinGW64: Open the terminal where MinGW64 is in your PATH

### Step 2: Navigate to Project Directory

```bash
cd /c/Users/aikhe/Desktop/ike/local/success/src
```

Or use Windows path format if using Windows CMD/PowerShell with MinGW64:

```bash
cd C:\Users\aikhe\Desktop\ike\local\success\src
```

### Step 3: Build the Project

```bash
make
```

Or build and run:

```bash
make run
```

### Step 4: Copy Required DLLs (if needed)

After building, if you get DLL errors when running (e.g., for libcurl.dll):

```bash
# Copy required DLLs to executable location if needed
# Note: pthread is now handled by MinGW64's winpthread, so no pthread DLL is needed
```

## Modified Makefile Changes

The Makefile has been updated for MinGW64 compatibility:

1. **Library Linking**: Changed from `../lib/nfd.lib` to `-lnfd` (uses libnfd.a automatically)
2. **pthread Library**: Uses MinGW64's built-in winpthread via `-pthread` flag (no external pthread library needed)
3. **Compiler Flags**: Already includes `-D__USE_MINGW_ANSI_STDIO=1` for proper MinGW stdio support

## Troubleshooting

### Error: "cannot find -lcurl" or similar

**Solution**: Ensure you're linking from the correct directory. The Makefile uses `-L"../lib"` which should point to the `lib/` directory. Verify the library files exist:

```bash
ls ../lib/*.a
```

### Error: "undefined reference to..."

**Possible causes:**

1. Missing library - check if the `.a` file exists in `lib/`
2. Wrong library order - MinGW64 may require specific library ordering
3. 32-bit vs 64-bit mismatch - ensure all libraries are compiled for the same architecture

### Error: "DLL not found" at runtime

**Solution**: Copy required DLLs (like libcurl.dll) to the executable directory or add their directory to PATH. Note: pthread is now handled by MinGW64's winpthread, so no pthread DLL is needed.

### Error: Linking issues with nfd.lib

**Solution**: The Makefile now uses `-lnfd` which will automatically find `libnfd.a`. If you still have issues, you can try:

```makefile
LIB = ... -L"../lib" -lnfd ...
```

Or explicitly:

```makefile
LIB = ... ../lib/libnfd.a ...
```

### Error: "skipping incompatible libpthreadGC1.a" or "cannot find -lpthreadGC1"

**Solution**: This error occurs when trying to use the external pthreadGC1 library which is incompatible with MinGW64. The Makefile now uses MinGW64's built-in winpthread library via the `-pthread` flag. This should resolve the issue automatically. Ensure your Makefile has `-pthread` in the compilation command (not `-lpthreadGC1`).

## Running the Program

After successful build:

```bash
./curses.exe
```

Or:

```bash
make run
```

## Clean Build

To clean build artifacts:

```bash
make clean
```

## Alternative: Using CMake

The project also has a CMakeLists.txt. To build with CMake:

```bash
# From project root
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
```

However, the CMakeLists.txt appears to be for a different target (clay_sample.c), not the main curses program.

## Notes

- The project uses PDCurses (libpdcursesw.a) for terminal UI, which is Windows-specific
- pthread is provided via MinGW64's built-in winpthread library (no external DLL needed)
- All Windows-specific libraries (ole32, uuid, winmm, gdi32, user32) are system libraries and don't need installation
