/*
project: 01
author: Jahir Bakari
email: jahirb1@umbc.edu
student id: BE41579
description: a simple linux shell designed to perform basic linux commands
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include "utils.h"  // Ensure the utils.h header is present for unescape().

/*
Function Name: main
Arguments: int argc, char** argv
Description: The entry point of the program. Verifies that the shell is executed
             without any arguments. If valid, starts the user prompt loop. If
             invalid, prints an error message and exits with code 1.
*/
int main(int argc, char** argv) {
    if (argc > 1) {
        fprintf(stderr, "Error: Too many arguments (%d)\n", argc);
        return 1;
    } else {
        user_prompt_loop();
        return 0;
    }
}

/*
Function Name: free_2d_array
Arguments: char** arr
Description: Frees a dynamically allocated 2D array of strings. Iterates over the
             array and releases each string, then releases the memory for the array itself.
*/
void free_2d_array(char** arr) {
    if (arr == NULL) return;
    for (int i = 0; arr[i] != NULL; i++) {
        free(arr[i]);
    }
    free(arr);
}

/*
Function Name: handle_proc
Arguments: char** parsed, char* input
Description: Processes commands related to the /proc filesystem. Checks if the user 
             requested valid files like /cpuinfo, /loadavg, or /uptime and reads them. 
             If the requested file is unsupported, prints an error message.
*/
void handle_proc(char** parsed, char* input) {
    char* str = slash(parsed[1]);

    if (strcmp(str, parsed[1]) != 0) {
        char* temp = strdup(str);
        free(parsed[1]);
        free(str);
        parsed[1] = temp;
    }

    if (strcmp(parsed[1], "/cpuinfo") == 0) {
        read_status("/proc/cpuinfo");
    } else if (strcmp(parsed[1], "/loadavg") == 0) {
        read_status("/proc/loadavg");
    } else if (strcmp(parsed[1], "/uptime") == 0) {
        read_status("/proc/uptime");
    } else {
        fprintf(stderr, "Error: Unsupported /proc file\n");
    }
    free(str);
}

/*
Function Name: slash
Arguments: char* input
Description: Ensures that the given input path starts with '/'. If the path already starts
             with '/', returns a duplicate. Otherwise, allocates memory and prepends '/'.
*/
char* slash(char* input) {
    const char slash = '/';
    if (input[0] == slash) {
        return strdup(input);
    } else {
        char* str = malloc(strlen(input) + 2);
        if (str == NULL) {
            perror("Error: Memory Allocation Failed");
            exit(EXIT_FAILURE);
        }
        sprintf(str, "%c%s", slash, input);
        return str;
    }
}

/*
Function Name: read_status
Arguments: char* file_path
Description: Opens the specified file in /proc, reads its contents line by line, 
             and prints them to the console. If the file cannot be opened, prints an error.
*/
void read_status(char* file_path) {
    FILE* open_file = fopen(file_path, "r");
    if (open_file == NULL) {
        perror("Error: Failed to open file");
        return;
    }

    char buffer[300];
    while (fgets(buffer, sizeof(buffer), open_file)) {
        printf("%s", buffer);
    }
    fclose(open_file);
}

/*
Function Name: save_cmd_history
Arguments: char* last_cmd, char* file_path
Description: Appends the given command to the history file at the specified path. 
             If the file does not exist, creates it. Ensures the command is saved line by line.
*/
void save_cmd_history(char* last_cmd, char* file_path) {
    FILE* history_file = fopen(file_path, "a");
    if (history_file == NULL) {
        history_file = fopen(file_path, "w");
        fclose(history_file);
        history_file = fopen(file_path, "a");
    }
    fprintf(history_file, "%s\n", last_cmd);
    fclose(history_file);
}

/*
Function Name: user_prompt_loop
Arguments: None
Description: The main loop of the shell. Continuously prompts the user for input,
             saves commands to history, parses them, and executes them accordingly. 
             Supports built-in commands like 'exit', '/proc', and '$history'. If the user 
             enters a valid command, forks a process to execute it.
*/
void user_prompt_loop() {
    char* home_dir = getenv("HOME");
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/.421sh", home_dir);

    while (1) {
        printf(">> ");
        char* usr_input = get_user_command();
        if (usr_input == NULL) continue;

        save_cmd_history(usr_input, file_path);
        char* raw_input = strdup(usr_input);
        char** parsed_input = parse_command(usr_input);

        if (parsed_input == NULL) {
            free(usr_input);
            free(raw_input);
            continue;
        }

        if (strcmp(parsed_input[0], "exit") == 0) {
            int exit_code = (parsed_input[1] != NULL) ? atoi(parsed_input[1]) : 0;
            free_2d_array(parsed_input);
            free(usr_input);
            free(raw_input);
            exit(exit_code);
        } else if (strcmp(parsed_input[0], "/proc") == 0) {
            if (parsed_input[1] == NULL) {
                fprintf(stderr, "Error: Missing /proc argument\n");
            } else {
                handle_proc(parsed_input, raw_input);
            }
        } else if (strcmp(parsed_input[0], "$history") == 0) {
            FILE* history_file = fopen(file_path, "r");
            if (history_file != NULL) {
                char buffer[256];
                while (fgets(buffer, sizeof(buffer), history_file)) {
                    printf("%s", buffer);
                }
                fclose(history_file);
            }
        } else {
            execute_command(parsed_input, raw_input, usr_input);
        }

        free(usr_input);
        free(raw_input);
        free_2d_array(parsed_input);
    }
}

/*
Function Name: get_user_command
Arguments: None
Description: Reads a line of input from the user. If the input is valid, returns it. 
             Removes any trailing newline. If an error occurs, returns NULL.
*/
char* get_user_command() {
    char* input = NULL;
    size_t len = 0;
    if (getline(&input, &len, stdin) == -1) {
        perror("Error: Failed to read input");
        free(input);
        return NULL;
    }
    input[strcspn(input, "\n")] = '\0';
    return input;
}

/*
Function Name: parse_command
Arguments: char* input
Description: Parses the user input into an array of tokens (strings). Allocates memory 
             for the tokens and stores them in a dynamically allocated array. If input is empty, 
             returns NULL.
*/
char** parse_command(char* input) {
    if (strlen(input) == 0) return NULL;

    char* escape = unescape(input, stdin);
    char* token = strtok(escape, " ");
    size_t token_count = 0;

    char** list = malloc(sizeof(char*) * (strlen(input) / 2 + 2));
    if (list == NULL) {
        perror("Error: Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    while (token != NULL) {
        list[token_count++] = strdup(token);
        token = strtok(NULL, " ");
    }
    list[token_count] = NULL;
    free(escape);

    return list;
}

/*
Function Name: execute_command
Arguments: char** cmd_list, char* raw, char* usr
Description: Forks a child process to execute the given command using execvp(). 
             If the fork or exec fails, prints an error. The parent process waits for 
             the child process to finish before returning.
*/
void execute_command(char** cmd_list, char* raw, char* usr) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Error: Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        execvp(cmd_list[0], cmd_list);
        perror("Error: execvp failed");
        free_2d_array(cmd_list);
        free(raw);
        free(usr);
        _exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}
