// libraries used
#include <stdio.h>
#include <unistd.h>
#include <string.h>

// function prototypes for clear, ascii arts, countdown, and functions
void countdown(int seconds);
void study();
void Break();
void clear();
void Notes();
void Notes1();
void pomodoro();
void pomodoro1();
void methods();
void help();
void help1();

// main function
int main()
{

    // initialized variable where the input from the main prorgam prompt
    char options[20];

    // function calling for ascii art
    methods();

    // program prompt and user input
    printf("Type \033[1;34m'pomodoro'\033[0m to continue | Type \033[1;34m'notes'\033[0m to list your notes |Type \033[1;34m'help'\033[0m for further instructions| Type \033[1;34m'main'\033[0m to redirect on the main page: \033[0m: ");
    fgets(options, (sizeof options), stdin);
    options[strcspn(options, "\n")] = '\0';

    // nested main conditional statement
    if (strcmp(options, "pomodoro") == 0) // this will redirect to the pomodoro page
    {
        pomodoro1(); // if the condition is true pomodoro page user will be directed to this page
    }
    else if (strcmp(options, "notes") == 0)
    {
        Notes1(); // if the condition is true pomodoro page user will be directed to this page
    }
    else if (strcmp(options, "help") == 0)
    {
        help1(); // if the condition is true pomodoro page user will be directed to this page (prototype)
    }
    return 0;
}

void pomodoro1()
{

    // initializing and declaring variables
    int Studytime, Shortbreak, Longbreak, sessions;
    char notes[1000] = "\0";
    char options[20]; // main reciever of the user input for decision the decision variables (page redirection)

    clear(); // clears previous terminal output to make it neat
    pomodoro();

    printf("Type \033[1;31m'continue'\033[0m to enter \033[1;34mpomodoro mode\033[0m | Type \033[1;34m'notes'\033[0m to swtch to notes mode |Type \033[1;34m'help'\033[0m for further instructions| Type \033[1;34m'main'\033[0m to redirect on the main page: \033[0m: ");
    fgets(options, (sizeof options), stdin);
    options[strcspn(options, "\n")] = '\0';

    if (strcmp(options, "continue") == 0) // this will enable the user to finally use pomodoro
    {

        // program prompts that recieves user input to customize their pomodoro timer
        printf("\033[1;34mEnter your study time in minutes (MM):\033[0m "); // study time in minutes
        scanf("%d", &Studytime);

        printf("\033[1;34mEnter your short break time in minutes (MM):\033[0m "); // short break in minutes
        scanf("%d", &Shortbreak);

        printf("\033[1;34mEnter your long break time in minutes (MM):\033[0m "); // long break in minutes only happen once before the study session will end
        scanf("%d", &Longbreak);

        printf("\033[1;34mEnter the number of your study sessions:\033[0m "); // number of how many the pomodoro timer/session will repeat
        scanf("%d", &sessions);
        getchar();

        printf("\033[1;34mEnter your Objectives/To-Do's for the whole study session:\033[0m "); // goals to be achieved while in pomodoro study mode
        fgets(notes, sizeof(notes), stdin);

        // user inputs will be
        Studytime *= 60;
        Shortbreak *= 60;
        Longbreak *= 60;
        sessions;

        // int work_time = 1 * 60;   // 25 minutes
        // int short_break = 1 * 60; // 5 minutes
        // int long_break = 1 * 60;  // 15 minutes
        // int sessions = 3;

        for (int i = 1; i <= sessions; i++) // will repeat the interation (session) until it is greater than eqaul to the sessions of pomodoro
        {
            clear();                                                  // will clear the previous outputs
            study();                                                  // will print the ascii art for pomodoro/study session
            printf("\n### Pomodoro Session %d: Study Time! ##\n", i); // will display the no. of sessions and study time
            printf("\033[1;33mTip: Engage with Active recall and Space Repetitions to increase retention and study efficiency!\033[0m\n\n");
            printf("You're only focused in this whole study session for: \n");
            printf("\033[1;31m# %s\033[0m\n", notes); // this will display your objectives/to-do/notes
            countdown(Studytime);
            clear(); // after the countdown is over this function call will remove the previous output from the previous session

            if (i >= sessions) // if the iteration (study sessions) is greater than equal to the total sessions the program will grant the user a one time long break
            {
                Break();
                printf("\nHere's your long break! \n");
                printf("\a");
                printf("\033[1;33mTip: Stay disciplined and avoid cheap dopamine!\033[0m\n\n");
                countdown(Longbreak);
            }
            else // else the program will continuously grant the user short breaks
            {
                Break();
                printf("\nYour short break!\n");
                printf("\a");
                printf("\033[1;33mTip: Take a deep breath, drink water and rest for a while\033[0m\n\n");
                countdown(Shortbreak);
                clear();
            }
        }

        // this part will clear the previous output from the last session, will produce a sound on a terminal with kitty emulator only for linux users, and will congratualate the user
        clear();
        printf("\a");
        printf("\n\033[1;35mYour study session is complete! Good job!\033[0m\n\n");
    }

    // if the previous conditional statement were true this part will get skipped but otherwise if the previous condition is false and one of these is true the user will be redirected accordingly to the page selected
    else if (strcmp(options, "notes") == 0)
    {
        Notes1();
    }
    else if (strcmp(options, "help") == 0)
    {
        help1();
    }

    // after ending the previous conditional statement this will appear to let the user move around in the study page
    printf("Type \033[1;34m'pomodoro'\033[0m to continue | Type \033[1;34m'notes'\033[0m to list your notes |Type \033[1;34m'help'\033[0m for further instructions| Type \033[1;34m'main'\033[0m to redirect on the main page: \033[0m: ");
    fgets(options, (sizeof options), stdin);
    options[strcspn(options, "\n")] = '\0';

    if (strcmp(options, "pomodoro") == 0) // this will redirect to the pomodoro page
    {
        pomodoro1();
    }
    else if (strcmp(options, "notes") == 0)
    {
        Notes1();
    }
    else if (strcmp(options, "help") == 0)
    {
        help1();
    }
}

void Notes1() // the user will be redirected to this page if it is selected
{

    clear();
    char filename[100];
    char line[256];
    FILE *file;
    char options[20];

    Notes();

    printf("Type \033[1;31m'continue'\033[0mto enter \033[1;34mnotes mode\033[0m | Type \033[1;34m'pomodoro'\033[0m to redirect to pomodoro |Type \033[1;34m'help'\033[0m for further instructions| Type \033[1;34m'main'\033[0m to redirect on the main page: \033[0m: ");
    fgets(options, (sizeof options), stdin);
    options[strcspn(options, "\n")] = '\0';

    if (strcmp(options, "continue") == 0) // this will redirect to the pomodoro page
    {
        printf("\033[1;34mEnter your Notes/To-Do list file name (without file extension):\033[0m ");
        scanf("%99s", filename);
        strcat(filename, ".txt");

        getchar(); // clear leftover newline

        file = fopen(filename, "w");
        if (file == NULL)
        {
            printf("\033[1;31mError: Could not create file.\033[0m\n");
            return;
        }

        printf("\033[1;32mStart typing your content below.\033[0m\n");
        printf("\033[1;32mType 'exit' on a new line to save and exit.\033[0m\n\n");

        while (1)
        {
            fgets(line, sizeof(line), stdin);

            // Check if user typed 'exit' to save and quit
            if (strcmp(line, "exit\n") == 0)
            {

                clear();
                break;
            }

            fprintf(file, "%s", line);
        }

        fclose(file);
        printf("\n\033[1;35mYour Notes '%s' was saved successfully Keep up that good work!\033[0m\n\n", filename);
    }
    else if (strcmp(options, "pomodoro") == 0)
    {
        pomodoro1();
    }
    else if (strcmp(options, "help") == 0)
    {
        help1();
    }

    // program prompt and user input
    printf("Type \033[1;34m'pomodoro'\033[0m to continue | Type \033[1;34m'notes'\033[0m to list your notes |Type \033[1;34m'help'\033[0m for further instructions| Type \033[1;34m'main'\033[0m to redirect on the main page: \033[0m: ");
    fgets(options, (sizeof options), stdin);
    options[strcspn(options, "\n")] = '\0';

    if (strcmp(options, "pomodoro") == 0) // this will redirect to the pomodoro page
    {
        pomodoro1();
    }
    else if (strcmp(options, "notes") == 0)
    {
        Notes1();
    }
    else if (strcmp(options, "help") == 0)
    {
        help1();
    }
}

void help1()
{

    clear();
    help();
    char options[20];

    printf("Type \033[1;31m'continue'\033[0m to enter \033[1;34mhelp page of sucess\033[0m | Type \033[1;34m'notes'\033[0m to redirect to notes mode |Type \033[1;34m'pomodoro'\033[0m to redirect to pomodoro mode| Type \033[1;34m'main'\033[0m to redirect on the main page: \033[0m: ");
    fgets(options, (sizeof options), stdin);
    options[strcspn(options, "\n")] = '\0';

    if (strcmp(options, "continue") == 0) // this will redirect to the pomodoro page
    {
        printf("\n\n\033[1;31m## THIS PAGE IS STILL UNDERDEVELOPMENT ##\033[0m\n\n");
        printf("\033[1;31m-- the purpose of this section is to give guidance to its users on how to use it properly in order to avoid confusions --\033[0m\n\n");
        printf("\033[1;34m~By choosing which page of study methods you wanted to redirect with, always enter the available options presented by the program in small caps to avoid errors\033[0m\n\n");
        printf("\033[1;34m~The format for entering the alloted time for pomodoro is 'MM' there will be an error if you try to input seconds with it\033[0m\n\n");
        printf("\033[1;34m~Choose wisely on entering your Objectives/To-Do list on the section of pomodoro, you can only enter 999 characters in one line. \033[0m\n\n");
        printf("\033[1;34m~if you wish to enter multiple objectives and to-do list in one study session on pomodoro you can do it by entering all of your tasks in one line since the program will accept spaces and special characters\033[0m\n\n");
        printf("\033[1;34m~alternatively, you can redirect to the 'notes' section to enter as many tasks, notes, and objectives you wanted to achieve\033[0m\n\n");
        printf("\033[1;34m~After exiting to your notes (text file) you can locate it into this directory where this program is stored\033[0m\n\n");
        printf("\033[1;35mThat's all, Thank you for using SUCCESS!!\033[0m\n\n");
    }

    else if (strcmp(options, "notes") == 0)
    {
        Notes1();
    }
    else if (strcmp(options, "pomodoro") == 0)
    {
        pomodoro1();
    }

    // program prompt and user input
    printf("Type \033[1;34m'pomodoro'\033[0m to continue | Type \033[1;34m'notes'\033[0m to list your notes |Type \033[1;34m'help'\033[0m for further instructions| Type \033[1;34m'main'\033[0m to redirect on the main page: \033[0m: ");
    fgets(options, (sizeof options), stdin);
    options[strcspn(options, "\n")] = '\0';

    if (strcmp(options, "pomodoro") == 0) // this will redirect to the pomodoro page
    {
        pomodoro1();
    }
    else if (strcmp(options, "notes") == 0)
    {
        Notes1();
    }
    else if (strcmp(options, "help") == 0)
    {
        help1();
    }
}

// functions for various purposes
void countdown(int seconds) // function for pomodoro countdown
{
    for (int i = seconds; i > 0; i--)
    {
        printf("\rTime left: \033[1;32m%02d:%02d\033[0m ", i / 60, i % 60);
        fflush(stdout);
        sleep(1);
    }
    printf("\nTime’s up!\n");
}

void study() // ascii art for study timer in pomodoro section
{

    printf("   ░██████   ░██████████░██     ░██ ░███████   ░██     ░██ \n");
    printf("  ░██   ░██      ░██    ░██     ░██ ░██   ░██   ░██   ░██  \n");
    printf(" ░██             ░██    ░██     ░██ ░██    ░██   ░██ ░██   \n");
    printf("  ░████████      ░██    ░██     ░██ ░██    ░██    ░████    \n");
    printf("         ░██     ░██    ░██     ░██ ░██    ░██     ░██     \n");
    printf("  ░██   ░██      ░██     ░██   ░██  ░██   ░██      ░██     \n");
    printf("   ░██████       ░██      ░██████   ░███████       ░██     \n");
}

void Break() // ascii art for both short and long breaks on pomodoro section
{

    printf("░████████   ░█████████  ░██████████    ░███    ░██     ░██ \n");
    printf("░██    ░██  ░██     ░██ ░██           ░██░██   ░██    ░██  \n");
    printf("░██    ░██  ░██     ░██ ░██          ░██  ░██  ░██   ░██   \n");
    printf("░████████   ░█████████  ░█████████  ░█████████ ░███████    \n");
    printf("░██     ░██ ░██   ░██   ░██         ░██    ░██ ░██   ░██   \n");
    printf("░██     ░██ ░██    ░██  ░██         ░██    ░██ ░██    ░██  \n");
    printf("░█████████  ░██     ░██ ░██████████ ░██    ░██ ░██     ░██ \n");
}

void Notes()
{
    printf("░███    ░██   ░██████   ░██████████░██████████   ░██████   \n");
    printf("░████   ░██  ░██   ░██      ░██    ░██          ░██   ░██  \n");
    printf("░██░██  ░██ ░██     ░██     ░██    ░██         ░██         \n");
    printf("░██ ░██ ░██ ░██     ░██     ░██    ░█████████   ░████████  \n");
    printf("░██  ░██░██ ░██     ░██     ░██    ░██                 ░██ \n");
    printf("░██   ░████  ░██   ░██      ░██    ░██          ░██   ░██  \n");
    printf("░██    ░███   ░██████       ░██    ░██████████   ░██████   \n\n");
}

void pomodoro()
{
    printf("░█████████    ░██████   ░███     ░███   ░██████   ░███████     ░██████   ░█████████    ░██████   \n");
    printf("░██     ░██  ░██   ░██  ░████   ░████  ░██   ░██  ░██   ░██   ░██   ░██  ░██     ░██  ░██   ░██  \n");
    printf("░██     ░██ ░██     ░██ ░██░██ ░██░██ ░██     ░██ ░██    ░██ ░██     ░██ ░██     ░██ ░██     ░██ \n");
    printf("░█████████  ░██     ░██ ░██ ░████ ░██ ░██     ░██ ░██    ░██ ░██     ░██ ░█████████  ░██     ░██ \n");
    printf("░██         ░██     ░██ ░██  ░██  ░██ ░██     ░██ ░██    ░██ ░██     ░██ ░██   ░██   ░██     ░██ \n");
    printf("░██          ░██   ░██  ░██       ░██  ░██   ░██  ░██   ░██   ░██   ░██  ░██    ░██   ░██   ░██  \n");
    printf("░██           ░██████   ░██       ░██   ░██████   ░███████     ░██████   ░██     ░██   ░██████\n\n");
}

void methods()
{

    printf("  ░██████   ░██████████░██     ░██ ░███████   ░██     ░██    ░███     ░███ ░██████████ ░██████████░██     ░██   ░██████   ░███████     ░██████\n");
    printf(" ░██   ░██      ░██    ░██     ░██ ░██   ░██   ░██   ░██     ░████   ░████ ░██             ░██    ░██     ░██  ░██   ░██  ░██   ░██   ░██   ░██  \n");
    printf("░██             ░██    ░██     ░██ ░██    ░██   ░██ ░██      ░██░██ ░██░██ ░██             ░██    ░██     ░██ ░██     ░██ ░██    ░██ ░██         \n");
    printf(" ░████████      ░██    ░██     ░██ ░██    ░██    ░████       ░██ ░████ ░██ ░█████████      ░██    ░██████████ ░██     ░██ ░██    ░██  ░████████  \n");
    printf("        ░██     ░██    ░██     ░██ ░██    ░██     ░██        ░██  ░██  ░██ ░██             ░██    ░██     ░██ ░██     ░██ ░██    ░██         ░██ \n");
    printf(" ░██   ░██      ░██     ░██   ░██  ░██   ░██      ░██        ░██       ░██ ░██             ░██    ░██     ░██  ░██   ░██  ░██   ░██   ░██   ░██  \n");
    printf("  ░██████       ░██      ░██████   ░███████       ░██        ░██       ░██ ░██████████     ░██    ░██     ░██   ░██████   ░███████     ░██████   \n\n");
}

void help()
{
    printf("░██     ░██ ░██████████ ░██         ░█████████       ░██████   ░██████████   ░██████  ░██████████░██████  ░██████   ░███    ░██ \n");
    printf("░██     ░██ ░██         ░██         ░██     ░██     ░██   ░██  ░██          ░██   ░██     ░██      ░██   ░██   ░██  ░████   ░██ \n");
    printf("░██     ░██ ░██         ░██         ░██     ░██    ░██         ░██         ░██            ░██      ░██  ░██     ░██ ░██░██  ░██ \n");
    printf("░██████████ ░█████████  ░██         ░█████████      ░████████  ░█████████  ░██            ░██      ░██  ░██     ░██ ░██ ░██ ░██ \n");
    printf("░██     ░██ ░██         ░██         ░██                    ░██ ░██         ░██            ░██      ░██  ░██     ░██ ░██  ░██░██ \n");
    printf("░██     ░██ ░██         ░██         ░██             ░██   ░██  ░██          ░██   ░██     ░██      ░██   ░██   ░██  ░██   ░████ \n");
    printf("░██     ░██ ░██████████ ░██████████ ░██              ░██████   ░██████████   ░██████      ░██    ░██████  ░██████   ░██    ░███ \n\n");
}

void clear() // to clear previous output of terminal
{
    system("cls");   // for windows terminal
    system("clear"); // for linux & mac terminal
}
