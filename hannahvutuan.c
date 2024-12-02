#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 100

char **path = NULL;

void print_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void execute_command(char **args) {
    for (int i = 0; path[i] != NULL; i++) {
        char cmd[MAX_INPUT_SIZE];
        snprintf(cmd, sizeof(cmd), "%s/%s", path[i], args[0]);
        if (access(cmd, X_OK) == 0) {
            execv(cmd, args);
            print_error();
            exit(1);
        }
    }
    print_error();
    exit(1);
}

int main(int argc, char *argv[]) {
    path = malloc(2 * sizeof(char*));
    path[0] = strdup("/bin");
    path[1] = NULL;
    
    if (argc > 2) {
        print_error();
        exit(1);
    }

    FILE *input = (argc == 2) ? fopen(argv[1], "r") : stdin;
    if (!input) {
        print_error();
        exit(1);
    }

    char *line = NULL;
    size_t len = 0;

    while (1) {
        if (argc == 1){
            printf("hannah> ");
        }
        if (getline(&line, &len, input) == -1) break;

        // Remove newline
        line[strcspn(line, "\n")] = '\0';

        char *tokens[MAX_TOKENS];
        int token_count = 0;
        char *token = strtok(line, " ");
        while (token && token_count < MAX_TOKENS - 1) {
            tokens[token_count++] = token;
            token = strtok(NULL, " ");
        }
        tokens[token_count] = NULL;

        if (token_count == 0) continue;

        // Built-in commands
        if (strcmp(tokens[0], "exit") == 0) {
            if (token_count != 1){
                print_error();
            }
            else{
                exit(0);
            }
        } 
        else if (strcmp(tokens[0], "cd") == 0) {
            if (token_count != 2 || chdir(tokens[1]) != 0){
                print_error();
            }
        } 
        else if (strcmp(tokens[0], "path") == 0) {
            for (int i = 0; path[i] != NULL; i++) {
                free(path[i]);
            }   
            path = malloc(sizeof(char*) * (token_count));
            for (int i = 1; i < token_count; i++) {
                path[i-1] = strdup(tokens[i]);
            }
            path[token_count-1] = NULL;
        } 
        else if (strcmp(tokens[0], "loop") == 0) {
            if (token_count < 3) {
                print_error();
                continue;
            }
            
            int count = atoi(tokens[1]);
            if (count <= 0) {
                print_error();
                continue;
            }
            char **loop_cmd = &tokens[2];
            
            for (int i = 0; i < count; i++) {
                pid_t pid = fork();
                if (pid == 0) {
                    execute_command(loop_cmd);
                } else if (pid > 0) {
                    waitpid(pid, NULL, 0);
                } else {
                    print_error();
                }
            }
        }
        else {
            pid_t pid = fork();
            if (pid == 0) {
                execute_command(tokens);
            } else if (pid > 0) {
                waitpid(pid, NULL, 0);
            } else {
                print_error();
            }
        }
    }

    free(line);
    if (input != stdin){
        fclose(input);
    }
    return 0;
}
