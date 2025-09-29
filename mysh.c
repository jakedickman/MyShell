#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <fnmatch.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include "arraylist.h"
#include "tokenizer.h"

#define BUFSIZE 1024

struct command {
    arraylist args;
    char *path;         
    char *input_file;        
    char *output_file;       
    int is_pipeline;         
};

void wildcard(char *pattern, char **args_list, int *count);

void read_cmd_input(char **tokens, struct command *cmd) {
    initialize_list(&cmd->args, 10);
    char *word = *tokens;
    int run = 1;

    while (run) {
        enum tok_t token = next_token(&word, 0);

        if (token == WORD) { //Word token logic
            if (strchr(word, '*')) { //Wildcard signifier
                wildcard(word, cmd->args.content, &cmd->args.size);
                free(word);
            } else {
                append_list(&cmd->args, word);
            }

        } else if (token == LT) { //If token is <
            token = next_token(&word, 0);
            if (token == WORD) {
                cmd->input_file = word;
            } else {
                fprintf(stderr, "Error: Expected file after '<'\n");
            }

        } else if (token == GT) { //If token is >
            token = next_token(&word, 0);
            if (token == WORD) {
                cmd->output_file = word;
            } else {
                fprintf(stderr, "Error: Expected file after '>'\n");
            }

        } else if (token == BAR) { //Checks for | token
            if (cmd->args.size > 0) {
                cmd->path = cmd->args.content[0];
            }
            cmd->is_pipeline = 1;
            *tokens = word;
            return;

        } else if (token == NL) { //NL then return
            return;

        } else if (token == EOS) {
            if (cmd->args.size > 0) {
                cmd->path = cmd->args.content[0];
            }
            append_list(&cmd->args, NULL);
            next_token(&word, 1);  // consume NL if present
            return;

        } else {
            fprintf(stderr, "Token type not found\n");
            return;
        }
    }
}


int cd_cmd(char **args) { //Returns 1 if error occurs, 0 otheriwse.
    if (args[1] == NULL) {
        fprintf(stderr, "missing argument for cd command\n");
        return 1;
    }

    if (chdir(args[1]) == -1) {
        perror("chdir failed");
        return 1;
    }
    return 0;
}

int pwd_cmd() { //Returns 1 if error occurs, 0 otheriwse.
    char cwd[BUFSIZE];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("pwd failed");
        return 1;
    }
    printf("%s\n", cwd);
    return 0;
}

void *which_cmd(const char *command) { //Returns NULL if path not found
    static char full_path[BUFSIZE];
    char *env_paths = getenv("PATH");

    if (env_paths == NULL) {
        return NULL;
    }

    char *path_copy = strdup(env_paths);
    char *path_token = strtok(path_copy, ":");

    while (path_token != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", path_token, command);

        if (access(full_path, X_OK) == 0) {
            free(path_copy);
            return full_path;
        }
        path_token = strtok(NULL, ":");
    }

    free(path_copy);
    return NULL;
}


void exit_cmd(char **args) {
    int exit_code = 0;
    int count = 1;
    for (int i = 1; args[i] != NULL; i++) {//print args
        printf("%s", args[i]);
        if (args[i + 1] != NULL) {
            printf(" "); 
        }
        count++;
    }
    printf("\n"); 

    if (args[count] != NULL) {
        exit_code = atoi(args[count]);//converts last arg to int 
    }

    exit(exit_code);
}


void wildcard(char *pattern, char **args_list, int *count) {
    char *dir_path = ".";
    char *base_pattern = pattern;
    char *pattern_copy = strdup(pattern);
    
    char *slash_pos = strrchr(pattern_copy, '/');
    if (slash_pos) {
        *slash_pos = '\0';
        dir_path = pattern_copy;
        base_pattern = slash_pos + 1;
    }

    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        args_list[(*count)++] = strdup(pattern);
        free(pattern_copy);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.' && base_pattern[0] != '.') {
            continue;
        }

        if (fnmatch(base_pattern, entry->d_name, 0) == 0) {
            char full_path[BUFSIZE];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
            args_list[(*count)++] = strdup(full_path);
        }
    }

    closedir(dir);
    free(pattern_copy);
}


int run_shell_cmds(struct command *cmd) {
    if (strcmp(cmd->path, "cd") == 0) {
        return cd_cmd(cmd->args.content);
    }

    if (strcmp(cmd->path, "pwd") == 0) {
        return pwd_cmd();
    }

    if (strcmp(cmd->path, "which") == 0) {
        char *path = which_cmd(cmd->args.content[1]);
        if (path) {
            printf("%s\n", path);
        } 
    }

    if (strcmp(cmd->path, "exit") == 0) {
        exit_cmd(cmd->args.content);
    }

    return -1; //If it is not a basic shell command, return -1
}


void execute_cmds(struct command *cmd) {
    if (!cmd->path || cmd->args.size == 0) {
        fprintf(stderr, "No command to execute.\n");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        return;
    }

    if (pid == 0) {
        if (cmd->input_file) {
            int fd_in = open(cmd->input_file, O_RDONLY);
            if (fd_in < 0) {
                perror(cmd->input_file);
                exit(1);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }

        if (cmd->output_file) {
            int fd_out = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
            if (fd_out < 0) {
                perror(cmd->output_file);
                exit(1);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }

        char *exec_path = which_cmd(cmd->path);
        if (!exec_path) {
            fprintf(stderr, "Command not found: %s\n", cmd->path);
            exit(1);
        }

        execv(exec_path, cmd->args.content);
        perror("Command execution failed");
        free(exec_path);
        exit(1);
    } 

    int status;
    if (wait(&status) == -1) {
        perror("Wait failed");
    } else if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        if (exit_code != 0) {
            printf("Command failed with code %d\n", exit_code);
        }
    } else if (WIFSIGNALED(status)) {
        psignal(WTERMSIG(status), "Terminated by signal");
    }
}



void execute_cmd_in_pipe(struct command *cmd, int input_fd, int output_fd) {
    if (input_fd != STDIN_FILENO) {
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
    }

    if (output_fd != STDOUT_FILENO) {
        dup2(output_fd, STDOUT_FILENO);
        close(output_fd);
    }

    char *exec_path = which_cmd(cmd->path);
    if (!exec_path) {
        fprintf(stderr, "Command not found: %s\n", cmd->path);
        exit(127);
    }

    execv(exec_path, cmd->args.content);
    perror("Execution failed");
    exit(1);
}

void pipeline(struct command *cmd1, struct command *cmd2) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("Pipe failed");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("Fork failed for first command");
        return;
    }

    if (pid1 == 0) {
        close(pipefd[0]); // close read end in first child
        execute_cmd_in_pipe(cmd1, STDIN_FILENO, pipefd[1]);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("Fork failed for second command");
        return;
    }

    if (pid2 == 0) {
        close(pipefd[1]); // close write end in second child
        execute_cmd_in_pipe(cmd2, pipefd[0], STDOUT_FILENO);
    }

    // Parent closes both ends of the pipe
    close(pipefd[0]);
    close(pipefd[1]);

    // Wait for both children
    int status;
    for (int i = 0; i < 2; ++i) {
        pid_t pid = wait(&status);
        if (pid == -1) {
            perror("Wait failed");
            continue;
        }

        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            if (exit_code != 0) {
                printf("Command Failed: code %d\n", exit_code);
            }
        } else if (WIFSIGNALED(status)) {
            psignal(WTERMSIG(status), "Terminated by signal");
        }
    }
}


void free_cmd(struct command *cmd) {
    free_list(&cmd->args);
    if (cmd->input_file) free(cmd->input_file);
    if (cmd->output_file) free(cmd->output_file);
}

int process_line(char *line) {
    char *pipe_pos = strchr(line, '|'); //checks for position of the pipe

    if (pipe_pos) {
        *pipe_pos = '\0'; //Split the line into two strings
        char *cmd1_str = line;
        char *cmd2_str = pipe_pos + 1;

        while (isspace((unsigned char)*cmd2_str)) cmd2_str++; //Skip spaces

        struct command cmd1 = {0};
        struct command cmd2 = {0};

        char *cmd1_ptr = cmd1_str;
        char *cmd2_ptr = cmd2_str;

        read_cmd_input(&cmd1_ptr, &cmd1);
        read_cmd_input(&cmd2_ptr, &cmd2);

        if (run_shell_cmds(&cmd1) == 0) {
            free_cmd(&cmd1);
            free_cmd(&cmd2);
            return 0;
        }

        pipeline(&cmd1, &cmd2);

        free_cmd(&cmd1);
        free_cmd(&cmd2);
    } else {
        char *line_ptr = line;
        struct command cmd = {0};
        read_cmd_input(&line_ptr, &cmd);

        if (run_shell_cmds(&cmd) == 0) {
            free_cmd(&cmd);
            return 0;
        }
        execute_cmds(&cmd);
        free_cmd(&cmd);
    }
    return 0;
}


int main(int argc, char *argv[]) {
    FILE *input = stdin;

    if (argc > 2) {
        fprintf(stderr, "Usage: mysh [batch_file]\n");
        exit(EXIT_FAILURE);
    }

    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (!input) {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }
    }

    int is_interactive = isatty(fileno(input)); //checks if in interactive mode
    if (is_interactive) {
        printf("Welcome to my shell!\n");
    }

    char buffer[BUFSIZE];
    char line[BUFSIZE] = {0};
    int line_pos = 0;

    while (!feof(input)) {
        if (is_interactive) {
            printf("mysh> ");
            fflush(stdout);
        }

        ssize_t bytes_read = read(fileno(input), buffer, BUFSIZE - 1);
        if (bytes_read <= 0) break;

        for (int i = 0; i < bytes_read; ++i) {
            if (buffer[i] == '\n') {
                line[line_pos] = '\0';
                if (line_pos > 0) {
                    process_line(line);
                }
                line_pos = 0;
            } else if (line_pos < BUFSIZE - 1) {
                line[line_pos++] = buffer[i];
            } else {
                fprintf(stderr, "Input line too long\n");
                line_pos = 0;
                break;
            }
        }
    }

    if (line_pos > 0) {
        line[line_pos] = '\0';
        process_line(line);
    }

    if (is_interactive) {
        printf("Exiting my shell.\n");
    }

    if (input != stdin) {
        fclose(input);
    }

    return EXIT_SUCCESS;
}

