<div align="center">

<img width="417" height="107" alt="Image" src="https://github.com/user-attachments/assets/5322d41c-b35a-4c65-976e-a194d6ca0958" />

<sub style="font-weight: bold;"> A Modern Cross-Platform Collaborative Learning & Study Environment powered by A.I  built for UCCians
1st year 1st semester finals project in ComProg I </sub>
</div>
<br><br>


An offline first terminal study suite ***made in pure C*** integrating proven learning methodologies (**Pomodoro**, **Active Recall**, **Spaced Repetition**) with local SQLite storage and Google Gemini generative intelligence.

---

## Tech Stack

[![Language: C23 / C99](https://img.shields.io/badge/C_23_%2F_C99-00599C?style=for-the-badge&logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C_(programming_language)) [![TUI: NCurses Wide](https://img.shields.io/badge/NCurses_Wide-00599C?style=for-the-badge&logo=gnu&logoColor=white)](https://invisible-island.net/ncurses/) [![TUI Windows: PDCursesMod](https://img.shields.io/badge/PDCursesMod-0078D4?style=for-the-badge&logo=windows&logoColor=white)](https://github.com/wmcbrine/PDCursesMod) [![Threading: POSIX](https://img.shields.io/badge/POSIX_Threads-2B2D31?style=for-the-badge&logo=linux&logoColor=white)](https://en.wikipedia.org/wiki/Pthreads) [![API: Google Gemini](https://img.shields.io/badge/Google_Gemini_2.5_Flash-8E75B2?style=for-the-badge&logo=googlegemini&logoColor=white)](https://ai.google.dev/) [![Network: libcurl](https://img.shields.io/badge/libcurl-073551?style=for-the-badge&logo=curl&logoColor=white)](https://curl.se/) [![JSON: cJSON v1.7.19](https://img.shields.io/badge/cJSON_v1.7.19-000000?style=for-the-badge&logo=json&logoColor=white)](https://github.com/DaveGamble/cJSON) [![DB: SQLite3](https://img.shields.io/badge/SQLite_3-003B57?style=for-the-badge&logo=sqlite&logoColor=white)](https://www.sqlite.org/) [![Dialogs: NativeFileDialog](https://img.shields.io/badge/NFD_Abstraction-059669?style=for-the-badge&logo=gnome&logoColor=white)](https://github.com/mlabbe/nativefiledialog) [![Build: CMake](https://img.shields.io/badge/CMake-064F8C?style=for-the-badge&logo=cmake&logoColor=white)](https://cmake.org/) [![Build: GNU Make](https://img.shields.io/badge/GNU_Make-427819?style=for-the-badge&logo=gnu&logoColor=white)](https://www.gnu.org/software/make/) [![Git VERSION CONTROL](https://img.shields.io/badge/Git-F05032?style=for-the-badge&logo=git&logoColor=white)](https://git-scm.com/) [![GitHub](https://img.shields.io/badge/GitHub-181717?style=for-the-badge&logo=github&logoColor=white)](https://github.com/) [![Neovim](https://img.shields.io/badge/Neovim-57A143?style=for-the-badge&logo=neovim&logoColor=white)](https://neovim.io/) [![VSCode](https://img.shields.io/badge/VS_Code-0078D4?style=for-the-badge&logo=visualstudiocode&logoColor=white)](https://code.visualstudio.com/) [![Debug: GDB](https://img.shields.io/badge/GDB-A42E2B?style=for-the-badge&logo=gnu&logoColor=white)](https://www.gnu.org/software/gdb/) [![Memory: Valgrind](https://img.shields.io/badge/Valgrind-2C3E50?style=for-the-badge&logo=valgrind&logoColor=white)](https://valgrind.org/)

---

## Architecture Overview

SUCCESS is engineered around a **Multi-Tier Decoupled Terminal Architecture (N-Tier Pattern)**. By separating display rendering from application domain logic, operating system syscalls, and network I/O, the codebase achieves complete cross-platform portability without compromising performance or code clarity.

```mermaid
flowchart TD
    %% Theme-Agnostic High-Contrast Node Styles (WCAG AA Compliant)
    classDef client fill:#5865F2,stroke:#3C45A5,stroke-width:2px,color:#FFFFFF;
    classDef tui fill:#00599C,stroke:#003B57,stroke-width:2px,color:#FFFFFF;
    classDef feature fill:#D97706,stroke:#92400E,stroke-width:2px,color:#FFFFFF;
    classDef compat fill:#059669,stroke:#065F46,stroke-width:2px,color:#FFFFFF;
    classDef vault fill:#7C3AED,stroke:#5B21B6,stroke-width:2px,color:#FFFFFF;
    classDef external fill:#8E75B2,stroke:#5B21B6,stroke-width:2px,color:#FFFFFF;

    %% Ingress Terminal Viewport
    Terminal["<b>Terminal Viewport</b><br/>(Alacritty • Kitty • Windows Terminal • WezTerm)"]:::client

    %% Core Application Boundary
    subgraph Engine ["<b>SUCCESS Platform Runtime Engine</b>"]
        direction TB

        %% Row 1: Presentation Layer
        UI["<b>NCurses / PDCurses Windowing Layer</b><br/>• Double-buffered frame blitting<br/>• UTF-8 unicode border glyphs"]:::tui

        %% Row 2: Business & Study Modules
        subgraph Features ["<b>Core Study Modules</b>"]
            direction LR
            Timer["<b>Pomodoro Timer</b><br/>Hardware Sleep"]:::feature
            Todo["<b>Todo Manager</b><br/>Task Tracking"]:::feature
            Social["<b>Social Hall</b><br/>Resource Hub"]:::feature
            AI["<b>AI Suite</b><br/>Chat • Quiz • Cards"]:::feature
        end

        %% Row 3: Cross-Platform Hardware & OS Abstraction
        subgraph Abstraction ["<b>OS & Hardware Abstraction Layer</b>"]
            direction LR
            Compat["<b>compat.c</b><br/>• Non-blocking I/O<br/>• Sleep & mkdir"]:::compat
            Paths["<b>paths.c</b><br/>• Dynamic env.json<br/>• Endpoint fallback"]:::compat
            NFD["<b>nfd_compat.c</b><br/>• Zenity / Win32<br/>• Terminal fallback"]:::compat
        end

        UI --> Features
        Features --> Abstraction
    end

    %% Transparent background adapts cleanly to GitHub Light, Dark, and Mobile
    style Engine fill:none,stroke:#5865F2,stroke-width:2px,stroke-dasharray: 5 5

    %% Target Backends
    DB[("<b>Local SQLite3 Storage</b><br/>• users.db (Auth)<br/>• todos.db (Tasks)<br/>• resources.db (Posts)")]:::vault
    Gemini["<b>Google Gemini Cloud</b><br/>• gemini-2.5-flash<br/>• Resumable File Protocol"]:::external

    %% Ingress & Egress Connections
    Terminal <-->|ANSI VT100 / Raw Termios| UI
    Features -->|ACID SQL Transactions| DB
    AI -->|HTTPS REST API via libcurl| Gemini
```
---

## Project Structure

```
success/
├── CMakeLists.txt                                   # Root cross-platform CMake build configuration
├── env.json.example                                 # Template for Gemini API credential setup
├── .gitignore                                       # Version control ignore definitions
├── db/                                              # Local SQLite database directory (auto-created)
│   ├── users.db                                     # User accounts, credentials, and roles
│   ├── todos.db                                     # User task records and completion states
│   ├── resources.db                                 # Social Hall academic materials and notes
│   └── .session                                     # Active authentication session cache
├── include/                                         # Project headers
│   └── win32/                                       # Isolated Windows-specific SDK headers (PDCurses/pthreads)
├── src/                                             # Application source code
│   ├── Makefile                                     # GNU Makefile with automatic OS detection
│   ├── curses.c                                     # Main TUI application entry point
│   ├── main.c                                       # CLI fallback entry point
│   ├── callbacks/
│   │   └── write_callback.c                         # Memory buffer handler for libcurl streams
│   ├── features/
│   │   ├── ai_chat.c                                # Interactive AI academic chatbot
│   │   ├── flashcard.c                              # AI active recall flashcard generator
│   │   ├── quiz.c                                   # AI 20-question multiple choice exam maker
│   │   ├── social_hall.c                            # Student/Teacher resource sharing hub
│   │   ├── study_timer.c                            # Low-power Pomodoro countdown timer
│   │   └── todo.c                                   # SQLite todo and assignment tracker
│   ├── gemini_api/
│   │   ├── gemini_request.c                         # JSON payload builder & response parser
│   │   ├── get_file_uri.c                           # Multimodal attachment handler
│   │   └── get_upload_url.c                         # Google Resumable Media Upload protocol
│   ├── pages/
│   │   ├── introduction.c                           # Welcome splash, authentication, role routing
│   │   ├── menu.c                                   # Student / Teacher dashboard navigation
│   │   └── tools.c                                  # Academic tools selection sub-menu
│   ├── utils/
│   │   ├── compat.c / .h                            # Cross-platform primitives (sleep, kbhit, mkdir)
│   │   ├── paths.c / .h                             # Unified env.json & credential resolver
│   │   ├── nfd_compat.c                             # Native file dialog abstraction & CLI fallback
│   │   ├── get_file_mime_type.c                     # Document & image MIME detection
│   │   ├── read_file_b64.c                          # Base64 file encoder for attachments
│   │   └── grep_string.c                            # Safe HTTP header parser
│   └── vendor/
│       └── cjson/                                   # Vendored official MIT cJSON v1.7.19 parser
└── tests/
    └── test_suite.c                                 # Automated 39-case headless regression test harness
```

---

## Installing Tools and Dependencies

To build and run SUCCESS, you need a **C compiler (GCC or Clang)**, **CMake (or Make)**, **SQLite3**, **libcurl**, and **NCurses** (or **PDCurses** on Windows).

### Arch-based Distros (CachyOS / Arch / Manjaro)
```bash
sudo pacman -S base-devel cmake ncurses curl sqlite
```

### Debian / Ubuntu-based Distros
```bash
sudo apt update && sudo apt install -y build-essential cmake libncurses-dev libcurl4-openssl-dev libsqlite3-dev
```

### Fedora-based Distros
```bash
sudo dnf install -y gcc gcc-c++ cmake ncurses-devel libcurl-devel sqlite-devel
```

### macOS (Homebrew)
```bash
brew install cmake ncurses curl sqlite3
```

### Windows 10/11

The recommended approach for Windows is using **MSYS2** or modern package managers.

#### Option A: MSYS2 (MinGW-w64) — Recommended
Launch the **MSYS2 UCRT64** or **MINGW64** shell and run:
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-pdcurses mingw-w64-x86_64-curl mingw-w64-x86_64-sqlite3
```

#### Option B: Winget
Install tools from PowerShell / CMD:
```powershell
winget install Kitware.CMake -e --accept-source-agreements
winget install LLVM.LLVM
winget install Git.Git
```

#### Option C: Scoop
```powershell
scoop install git cmake gcc curl sqlite
```

---

## Configuration (AI Features)

##### Prerequisites
To use the AI Chatbot, Quiz Generator, and Flashcard Generator, obtain a free Google Gemini API Key from **[Google AI Studio](https://aistudio.google.com/api-keys)**.

> [!IMPORTANT]
> Never commit or publicly share your API key. Keep `env.json` listed in `.gitignore`.

#### Method 1: Environment Variable (Recommended for quick testing)
```bash
export GEMINI_API_KEY="paste-your-gemini-api-key-here"
```

#### Method 2: Configuration File
Create an `env.json` file in the root project directory:
```json
{
  "GEMINI_API_KEY": "paste-your-gemini-api-key-here"
}
```

---

## Build and Run

### Building with CMake (Recommended)

1. Configure the build directory:
```bash
cmake -B build
```

2. Compile all targets:
```bash
cmake --build build
```

This compiles three executables into `build/`:
* `build/success`: Full interactive NCurses TUI application.
* `build/success_cli`: Command-line fallback interface.
* `build/test_suite`: Automated headless test runner.

### Alternative: Building with GNU Make
```bash
make -C src
```

---

## Running the Application

Launch the full interactive TUI:
```bash
./build/success
```

Execute the headless test harness to verify your installation:
```bash
./build/test_suite
```

> [!NOTE]
> For the best visual experience, ensure your terminal emulator supports UTF-8 and ANSI colors with a minimum resolution of **80 columns × 24 rows** (recommended: **100 × 30**).

---

## PROJECT PREVIEW

<p align="center">
<h3> video demo (v0.1.10) </h3>

https://github.com/user-attachments/assets/9222df74-09c8-4f65-a5b1-84c32ea21009

<h3> success terminal preview linux environment kitty terminal emulator (v0.1.10) </h3>

<img width="1920" height="1080" alt="Image" src="https://github.com/user-attachments/assets/95e4cf74-e15d-4d5b-aa03-3bb6f44b04aa" />

<img width="1920" height="1080" alt="Image" src="https://github.com/user-attachments/assets/0ac99a09-5c56-427b-a2e1-b0dd58ac2d24" />

<img width="1920" height="1080" alt="Image" src="https://github.com/user-attachments/assets/e16abfc8-0f08-481f-a2cb-c3da25b69010" />

<img width="1920" height="1080" alt="Image" src="https://github.com/user-attachments/assets/e309cb6c-801c-47e1-9182-f97b8e4aba59" />

<img width="1920" height="1080" alt="Image" src="https://github.com/user-attachments/assets/e6eeaccb-6e4c-44af-af3b-ff43efa7851f" />

<img width="1920" height="1080" alt="Image" src="https://github.com/user-attachments/assets/5d07b755-abdd-4d35-a5cf-cfef518a134c" />

<img width="1920" height="1080" alt="Image" src="https://github.com/user-attachments/assets/62f68c88-04a6-4b04-a61b-6f21a51b3cf6" />

</p>
<!--
## PROJECT FLOWCHART

<img width="3435" height="2440" alt="Image" src="https://github.com/user-attachments/assets/0bc03bb0-4c81-442c-8847-5b08d44c5b56" />

<img width="1239" height="2745" alt="Image" src="https://github.com/user-attachments/assets/955cca57-981b-4382-8a98-7c6b62eff0d8" />
-->

---

## License
This project is licensed under the terms of the [GNU General Public License v3.0](LICENSE).
