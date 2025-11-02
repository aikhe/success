// libraries used
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#error "This study timer is Windows-only"
#endif

#include "study_timer.h"

// function prototypes for clear, ascii arts, countdown, and functions
static int countdown(int seconds);
static void study(void);
static void Break(void);
static void clear_screen(void);
static void Notes(void);
static void pomodoro_ascii(void);
static void methods_ascii(void);
static void help_ascii(void);
static void pomodoro1(void);

// Main entry point for study timer
void study_timer(void) {
  char options[20];

  // function calling for ascii art
  methods_ascii();

  // program prompt and user input
  printf("Type \033[1;34m'pomodoro'\033[0m to continue | Type "
         "\033[1;34m'notes'\033[0m to list your notes |Type "
         "\033[1;34m'help'\033[0m for further instructions| Type "
         "\033[1;34m'main'\033[0m to redirect on the main page: \033[0m: ");
  fgets(options, (sizeof options), stdin);
  options[strcspn(options, "\n")] = '\0';

  // nested main conditional statement
  if (strcmp(options, "pomodoro") == 0) {
    pomodoro1();
  } else if (strcmp(options, "notes") == 0) {
    Notes(); // Simplified - just show notes ASCII for now
    printf("\nPress Enter to return to menu...\n");
    getchar();
  } else if (strcmp(options, "help") == 0) {
    clear_screen();
    help_ascii();
    printf("\n\033[1;31m## THIS PAGE IS STILL UNDERDEVELOPMENT ##\033[0m\n\n");
    printf("\033[1;34m~By choosing which page of study methods you wanted to "
           "redirect with, always enter the available options presented by the "
           "program in small caps to avoid errors\033[0m\n\n");
    printf("\033[1;34m~The format for entering the alloted time for pomodoro "
           "is 'MM' there will be an error if you try to input seconds with "
           "it\033[0m\n\n");
    printf("\033[1;34m~Press Enter to return to menu...\033[0m\n");
    getchar();
  } else if (strcmp(options, "main") == 0) {
    // Return to menu
    return;
  }
}

static void pomodoro1(void) {
  // initializing and declaring variables
  int Studytime, Shortbreak, Longbreak, sessions;
  char notes[1000] = "\0";
  char options[20];

  clear_screen();
  pomodoro_ascii();

  printf("Type \033[1;31m'continue'\033[0m to enter \033[1;34mpomodoro "
         "mode\033[0m | Type \033[1;34m'main'\033[0m to return to main menu: \033[0m: ");
  fgets(options, (sizeof options), stdin);
  options[strcspn(options, "\n")] = '\0';

  if (strcmp(options, "continue") == 0) {
    // program prompts that recieves user input to customize their pomodoro timer
    printf("\033[1;34mEnter your study time in minutes (MM):\033[0m ");
    scanf("%d", &Studytime);

    printf("\033[1;34mEnter your short break time in minutes (MM):\033[0m ");
    scanf("%d", &Shortbreak);

    printf("\033[1;34mEnter your long break time in minutes (MM):\033[0m ");
    scanf("%d", &Longbreak);

    printf("\033[1;34mEnter the number of your study sessions:\033[0m ");
    scanf("%d", &sessions);
    getchar();

    printf("\033[1;34mEnter your Objectives/To-Do's for the whole study "
           "session:\033[0m ");
    fgets(notes, sizeof(notes), stdin);

    // user inputs will be converted to seconds
    Studytime *= 60;
    Shortbreak *= 60;
    Longbreak *= 60;

    for (int i = 1; i <= sessions; i++) {
      clear_screen();
      study();
      printf("\n### Pomodoro Session %d: Study Time! ##\n", i);
      printf("\033[1;33mTip: Engage with Active recall and Space Repetitions "
             "to increase retention and study efficiency!\033[0m\n\n");
      printf("You're only focused in this whole study session for: \n");
      printf("\033[1;31m# %s\033[0m\n", notes);
      printf("\033[1;36mPress 'q' at any time to exit the timer\033[0m\n\n");
      
      if (countdown(Studytime) != 0) {
        // User pressed 'q' to exit
        clear_screen();
        printf("\n\033[1;33mTimer stopped by user. Returning to menu...\033[0m\n");
        Sleep(2000); // Show message for 2 seconds
        return;
      }
      
      clear_screen();

      if (i >= sessions) {
        Break();
        printf("\nHere's your long break! \n");
        printf("\a");
        printf("\033[1;33mTip: Stay disciplined and avoid cheap "
               "dopamine!\033[0m\n\n");
        printf("\033[1;36mPress 'q' at any time to exit the timer\033[0m\n\n");
        
        if (countdown(Longbreak) != 0) {
          // User pressed 'q' to exit
          clear_screen();
          printf("\n\033[1;33mTimer stopped by user. Returning to menu...\033[0m\n");
          Sleep(2000);
          return;
        }
      } else {
        Break();
        printf("\nYour short break!\n");
        printf("\a");
        printf("\033[1;33mTip: Take a deep breath, drink water and rest for a "
               "while\033[0m\n\n");
        printf("\033[1;36mPress 'q' at any time to exit the timer\033[0m\n\n");
        
        if (countdown(Shortbreak) != 0) {
          // User pressed 'q' to exit
          clear_screen();
          printf("\n\033[1;33mTimer stopped by user. Returning to menu...\033[0m\n");
          Sleep(2000);
          return;
        }
        clear_screen();
      }
    }

    // this part will clear the previous output from the last session, will
    // produce a sound, and will congratulate the user
    clear_screen();
    printf("\a");
    printf("\n\033[1;35mYour study session is complete! Good job!\033[0m\n\n");
    printf("Press Enter to return to menu...\n");
    getchar();
  } else if (strcmp(options, "main") == 0) {
    return;
  }
}

// Interruptible countdown function - returns 0 if completed, 1 if interrupted
static int countdown(int seconds) {
  for (int i = seconds; i > 0; i--) {
    // Check for keypress without blocking (Windows only)
    if (_kbhit()) {
      char key = _getch();
      if (key == 'q' || key == 'Q') {
        return 1; // Interrupted by user
      }
    }
    
    printf("\rTime left: \033[1;32m%02d:%02d\033[0m ", i / 60, i % 60);
    fflush(stdout);
    Sleep(1000); // Windows Sleep (1000 milliseconds = 1 second)
  }
  printf("\nTime's up!\n");
  return 0; // Completed successfully
}

static void study(void) {
  printf("   ░██████   ░██████████░██     ░██ ░███████   ░██     ░██ \n");
  printf("  ░██   ░██      ░██    ░██     ░██ ░██   ░██   ░██   ░██  \n");
  printf(" ░██             ░██    ░██     ░██ ░██    ░██   ░██ ░██   \n");
  printf("  ░████████      ░██    ░██     ░██ ░██    ░██    ░████    \n");
  printf("         ░██     ░██    ░██     ░██ ░██    ░██     ░██     \n");
  printf("  ░██   ░██      ░██     ░██   ░██  ░██   ░██      ░██     \n");
  printf("   ░██████       ░██      ░██████   ░███████       ░██     \n");
}

static void Break(void) {
  printf("░████████   ░█████████  ░██████████    ░███    ░██     ░██ \n");
  printf("░██    ░██  ░██     ░██ ░██           ░██░██   ░██    ░██  \n");
  printf("░██    ░██  ░██     ░██ ░██          ░██  ░██  ░██   ░██   \n");
  printf("░████████   ░█████████  ░█████████  ░█████████ ░███████    \n");
  printf("░██     ░██ ░██   ░██   ░██         ░██    ░██ ░██   ░██   \n");
  printf("░██     ░██ ░██    ░██  ░██         ░██    ░██ ░██    ░██  \n");
  printf("░█████████  ░██     ░██ ░██████████ ░██    ░██ ░██     ░██ \n");
}

static void Notes(void) {
  printf("░███    ░██   ░██████   ░██████████░██████████   ░██████   \n");
  printf("░████   ░██  ░██   ░██      ░██    ░██          ░██   ░██  \n");
  printf("░██░██  ░██ ░██     ░██     ░██    ░██         ░██         \n");
  printf("░██ ░██ ░██ ░██     ░██     ░██    ░█████████   ░████████  \n");
  printf("░██  ░██░██ ░██     ░██     ░██    ░██                 ░██ \n");
  printf("░██   ░████  ░██   ░██      ░██    ░██          ░██   ░██  \n");
  printf("░██    ░███   ░██████       ░██    ░██████████   ░██████   \n\n");
}

static void pomodoro_ascii(void) {
  printf("░█████████    ░██████   ░███     ░███   ░██████   ░███████     "
         "░██████   ░█████████    ░██████   \n");
  printf("░██     ░██  ░██   ░██  ░████   ░████  ░██   ░██  ░██   ░██   ░██   "
         "░██  ░██     ░██  ░██   ░██  \n");
  printf("░██     ░██ ░██     ░██ ░██░██ ░██░██ ░██     ░██ ░██    ░██ ░██     "
         "░██ ░██     ░██ ░██     ░██ \n");
  printf("░█████████  ░██     ░██ ░██ ░████ ░██ ░██     ░██ ░██    ░██ ░██     "
         "░██ ░█████████  ░██     ░██ \n");
  printf("░██         ░██     ░██ ░██  ░██  ░██ ░██     ░██ ░██    ░██ ░██     "
         "░██ ░██   ░██   ░██     ░██ \n");
  printf("░██          ░██   ░██  ░██       ░██  ░██   ░██  ░██   ░██   ░██   "
         "░██  ░██    ░██   ░██   ░██  \n");
  printf("░██           ░██████   ░██       ░██   ░██████   ░███████     "
         "░██████   ░██     ░██   ░██████\n\n");
}

static void methods_ascii(void) {
  printf("  ░██████   ░██████████░██     ░██ ░███████   ░██     ░██    ░███    "
         " ░███ ░██████████ ░██████████░██     ░██   ░██████   ░███████     "
         "░██████\n");
  printf(" ░██   ░██      ░██    ░██     ░██ ░██   ░██   ░██   ░██     ░████   "
         "░████ ░██             ░██    ░██     ░██  ░██   ░██  ░██   ░██   ░██ "
         "  ░██  \n");
  printf("░██             ░██    ░██     ░██ ░██    ░██   ░██ ░██      ░██░██ "
         "░██░██ ░██             ░██    ░██     ░██ ░██     ░██ ░██    ░██ ░██ "
         "        \n");
  printf(" ░████████      ░██    ░██     ░██ ░██    ░██    ░████       ░██ "
         "░████ ░██ ░█████████      ░██    ░██████████ ░██     ░██ ░██    ░██  "
         "░████████  \n");
  printf("        ░██     ░██    ░██     ░██ ░██    ░██     ░██        ░██  "
         "░██  ░██ ░██             ░██    ░██     ░██ ░██     ░██ ░██    ░██   "
         "      ░██ \n");
  printf(" ░██   ░██      ░██     ░██   ░██  ░██   ░██      ░██        ░██     "
         "  ░██ ░██             ░██    ░██     ░██  ░██   ░██  ░██   ░██   ░██ "
         "  ░██  \n");
  printf("  ░██████       ░██      ░██████   ░███████       ░██        ░██     "
         "  ░██ ░██████████     ░██    ░██     ░██   ░██████   ░███████     "
         "░██████   \n\n");
}

static void help_ascii(void) {
  printf("░██     ░██ ░██████████ ░██         ░█████████       ░██████   "
         "░██████████   ░██████  ░██████████░██████  ░██████   ░███    ░██ \n");
  printf("░██     ░██ ░██         ░██         ░██     ░██     ░██   ░██  ░██   "
         "       ░██   ░██     ░██      ░██   ░██   ░██  ░████   ░██ \n");
  printf("░██     ░██ ░██         ░██         ░██     ░██    ░██         ░██   "
         "      ░██            ░██      ░██  ░██     ░██ ░██░██  ░██ \n");
  printf("░██████████ ░█████████  ░██         ░█████████      ░████████  "
         "░█████████  ░██            ░██      ░██  ░██     ░██ ░██ ░██ ░██ \n");
  printf("░██     ░██ ░██         ░██         ░██                    ░██ ░██   "
         "      ░██            ░██      ░██  ░██     ░██ ░██  ░██░██ \n");
  printf("░██     ░██ ░██         ░██         ░██             ░██   ░██  ░██   "
         "       ░██   ░██     ░██      ░██   ░██   ░██  ░██   ░████ \n");
  printf(
      "░██     ░██ ░██████████ ░██████████ ░██              ░██████   "
      "░██████████   ░██████      ░██    ░██████  ░██████   ░██    ░███ \n\n");
}

static void clear_screen(void) {
#ifdef _WIN32
  system("cls");
#else
  system("clear");
#endif
}
