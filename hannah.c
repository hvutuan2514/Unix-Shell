#define _GNU_SOURCE // For getline() and strdup()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h> // For kill()

#define MAX_ARGS 100 // Maximum number of arguments

char **path = NULL; // global path variable

// PIDs for fg and bg commands
pid_t fg_pid = 0;
pid_t bg_pid = 0;

// Signal handling for Ctrl-C and Ctrl-Z
void signal_handler(int signo) {
    if (signo == SIGINT && fg_pid > 0) {
        kill(fg_pid, SIGINT);
        printf("\n");
    } 
    else if (signo == SIGTSTP && fg_pid > 0) {
        kill(fg_pid, SIGTSTP);
        bg_pid = fg_pid;
        printf("\n");
    }
}

// Prints error message
void print_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

// Executes command with redirection
void execute_cmd(char **args) {
    int i;
    char *outfile = NULL;

    // Checks for '>' character
    for (i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            // Verifies redirection syntax
            if (args[i+1] == NULL || args[i+2] != NULL) {
                print_error();
                exit(1);
            }
            outfile = args[i+1];
            args[i] = NULL;
            break;
        }
    }
    
    if (outfile != NULL) {
        // Opens or creates output file
        int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) {
            print_error();
            exit(1);
        }
        // Reroutes stdout and stderr to the output file
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
    }

    // Searches for command in path
    for (i = 0; path[i] != NULL; i++) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "%s/%s", path[i], args[0]);

        // Checks if command exists and is executable
        if (access(cmd, X_OK) == 0) {
            execv(cmd, args); // Runs command
            print_error();
            exit(1);
        }
    }
    print_error();
    exit(1);
}

// Forks and execute command in child process
void execute_fork(char **cmd) {
    pid_t pid = fork();
    if (pid == 0) { // Child process
        execute_cmd(cmd);
    } 
    else if (pid > 0) { // Parent process
        fg_pid = pid;
        waitpid(pid, NULL, WUNTRACED);
        fg_pid = 0;
    } 
    else {
        print_error();
    }
}

// Exits shell
void exit_cmd(char **args, int arg_count) {
    if (arg_count != 1) {
        print_error();
    } 
    else {
        exit(0);
    }
}

// Changes directories
void cd_cmd(char **args, int arg_count) {
    if (arg_count != 2 || chdir(args[1]) != 0) {
        print_error();
    }
}

// Updates old path with newly specified path
void path_cmd(char **args, int arg_count) {
    for (int i = 0; path[i] != NULL; i++) {
        free(path[i]);
    }
    path = malloc(sizeof(char*) * (arg_count));
    for (int i = 1; i < arg_count; i++) {
        path[i-1] = strdup(args[i]);
    }
    path[arg_count-1] = NULL;
}

// Runs a command multiple times
void loop_cmd(char **args, int arg_count) {
    if (arg_count < 3 || atoi(args[1]) <= 0) {
        print_error();
        return;
    }

    int count = atoi(args[1]); // Gets number of iterations
    char **cmd = &args[2];
    char loop_str[20];

    // Executes loop command 'count' times
    for (int i = 1; i <= count; i++) {
        sprintf(loop_str, "%d", i);
        for (int j = 0; cmd[j]; j++) {
            // Adds a loop variable that is replaced with counter
            if (strcmp(cmd[j], "$loop") == 0) {
                cmd[j] = loop_str;
            }
        }
        execute_fork(cmd);
    }
}

// Resumes process in foreground
void fg_cmd() {
    if (bg_pid > 0) {
        fg_pid = bg_pid;
        kill(bg_pid, SIGCONT);
        waitpid(bg_pid, NULL, 0);
        bg_pid = 0;
    }
}

// Resumes process in background
void bg_cmd() {
    if (bg_pid > 0) {
        kill(bg_pid, SIGCONT);
        bg_pid = 0;
    }
}

int main(int argc, char *argv[]) {
    // Sets up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTSTP, signal_handler);

    // Initializes shell path
    path = malloc(2 * sizeof(char*));
    path[0] = strdup("/bin");
    path[1] = NULL;

    // Opens input file or uses stdin
    FILE *input = (argc == 2) ? fopen(argv[1], "r") : stdin;
    if (!input || argc > 2) {
        print_error();
        exit(1);
    }

    char *line = NULL;
    size_t len = 0;

    while (1) {
        if (argc == 1){
            printf("hannah> "); // Prompt 
        }

        // Reads lines of input
        ssize_t read = getline(&line, &len, input);
        if (read == -1) {
            if (feof(input)) {
                // Clean exit at end of file
                break;
            }
            print_error();
            exit(1);
        }
        
        // Removes newline character
        line[strcspn(line, "\n")] = '\0';

        // Parses input into arguments
        char *args[MAX_ARGS];
        char *str = line;
        char *arg;
        int arg_count = 0;
        
        // Splits input
        while ((arg = strsep(&str, " \t")) && arg_count < MAX_ARGS - 1) {
            if (*arg != '\0') {
                args[arg_count++] = arg;
            }
        }

        args[arg_count] = NULL;

        if (arg_count == 0){ // Skips empty lines
            continue;
        }

        // Built-in commands
        if (strcmp(args[0], "exit") == 0) {
            exit_cmd(args, arg_count);
        } 
        else if (strcmp(args[0], "cd") == 0) {
            cd_cmd(args, arg_count);
        } 
        else if (strcmp(args[0], "path") == 0) {
            path_cmd(args, arg_count);
        } 
        else if (strcmp(args[0], "loop") == 0) {
            loop_cmd(args, arg_count);
        }
        else if (strcmp(args[0], "fg") == 0) {
            fg_cmd();
        }
        else if (strcmp(args[0], "bg") == 0) {
            bg_cmd();
        }
        else {
            execute_fork(args);
        }
    }
    free(line);
    if (input != stdin){ // Closes input file
        fclose(input);
    }
    return 0;
}
