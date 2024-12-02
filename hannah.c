#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_TOKENS 100

char **path = NULL;

pid_t fg_pid = 0;
pid_t bg_pid = 0;

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

void print_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void execute_cmd(char **args) {
    int i;
    char *outfile = NULL;
    for (i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
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
        int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) {
            print_error();
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
    }

    for (i = 0; path[i] != NULL; i++) {
        char cmd[1024];
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

void execute_fork(char **cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        execute_cmd(cmd);
    } 
    else if (pid > 0) {
        fg_pid = pid;
        waitpid(pid, NULL, 0);
        fg_pid = 0;
    } 
    else {
        print_error();
    }
}

void exit_cmd(char **tokens, int token_count) {
    if (token_count != 1) {
        print_error();
    } 
    else {
        exit(0);
    }
}

void cd_cmd(char **tokens, int token_count) {
    if (token_count != 2 || chdir(tokens[1]) != 0) {
        print_error();
    }
}

void path_cmd(char **tokens, int token_count) {
    for (int i = 0; path[i] != NULL; i++) {
        free(path[i]);
    }
    path = malloc(sizeof(char*) * (token_count));
    for (int i = 1; i < token_count; i++) {
        path[i-1] = strdup(tokens[i]);
    }
    path[token_count-1] = NULL;
}

void loop_cmd(char **tokens, int token_count) {
    if (token_count < 3 || atoi(tokens[1]) <= 0) {
        print_error();
        return;
    }

    int count = atoi(tokens[1]);
    char **cmd = &tokens[2];
    char loop_str[20];

    for (int i = 1; i <= count; i++) {
        sprintf(loop_str, "%d", i);
        for (int j = 0; cmd[j]; j++) {
            if (strcmp(cmd[j], "$loop") == 0) {
                cmd[j] = loop_str;
            }
        }
        execute_fork(cmd);
    }
}

void fg_cmd() {
    if (bg_pid > 0) {
        fg_pid = bg_pid;
        kill(bg_pid, SIGCONT);
        waitpid(bg_pid, NULL, 0);
        bg_pid = 0;
    }
}

void bg_cmd() {
    if (bg_pid > 0) {
        kill(bg_pid, SIGCONT);
        bg_pid = 0;
    }
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTSTP, signal_handler);

    path = malloc(2 * sizeof(char*));
    path[0] = strdup("/bin");
    path[1] = NULL;
    
    if (argc > 2) {
        print_error();
        exit(1);
    }

    FILE *input;
    if (argc == 2) {
        input = fopen(argv[1], "r");
    } 
    else if (!input) {
        print_error();
        exit(1);
    }
    else {
        input = stdin;
    }

    char *line = NULL;
    size_t len = 0;

    while (1) {
        if (argc == 1){
            printf("hannah> ");
        }
        if (getline(&line, &len, input) == -1){
            break;
        }
        
        line[strcspn(line, "\n")] = '\0';

        char *tokens[MAX_TOKENS];
        int token_count = 0;
        char *str = line;
        char *token;
        while ((token = strsep(&str, " \t")) && token_count < MAX_TOKENS - 1) {
            if (*token != '\0') {
                tokens[token_count++] = token;
            }
        }
        tokens[token_count] = NULL;

        if (token_count == 0){
            continue;
        }

        // Built-in commands
        if (strcmp(tokens[0], "exit") == 0) {
            exit_cmd(tokens, token_count);
        } 
        else if (strcmp(tokens[0], "cd") == 0) {
            cd_cmd(tokens, token_count);
        } 
        else if (strcmp(tokens[0], "path") == 0) {
            path_cmd(tokens, token_count);
        } 
        else if (strcmp(tokens[0], "loop") == 0) {
            loop_cmd(tokens, token_count);
        }
        else if (strcmp(tokens[0], "fg") == 0) {
            fg_cmd();
        }
        else if (strcmp(tokens[0], "bg") == 0) {
            bg_cmd();
        }
        else {
            execute_fork(tokens);
        }
    }

    free(line);
    if (input != stdin){
        fclose(input);
    }
    return 0;
}
