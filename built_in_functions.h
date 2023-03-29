#ifndef BUILT_IN_FUNCTIONS_H
#define BUILT_IN_FUNCTIONS_H

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <limits.h>
#include "helper_functions.h"

#define INPUT_BUF_SIZE 1024
#define BUILT_IN_COMMANDS 11
#define MAX_HISTORY_RECORDS 1024
#define MAX_ENVIRONMENT_VARIABLES 128
#define MAX_LOCAL_VARIABLES 128
#define MAX_ARGS 64
#define MAX_RUNNING_PROCESSES 10

// Functions

// Adds a local variable definition
int variable_assignment(char *expression);

// Check if command is built in
int is_built_in(char *command);

// Executes built-in command
int execute_built_in(char **args, int index);

// Kills processes with given pids
void kill_processes(int *pids);

// Built-in cd command
int cd(char **args);

// Built-in echo command
int echo(char **args);

// Built-in env command
int env(char **args);

// Built-in exec command
int exec(char **args);

// Built-in exit command
int exit_shell(char **args);

// Built-in export command
int export(char **args);

// Built-in history command
int history(char **args);

// Built-in read command
int read_input(char **args);


// Globals
extern char *history_commands[MAX_HISTORY_RECORDS]; // Stores current session history commands
extern char *environment_variables[MAX_ENVIRONMENT_VARIABLES]; // Stores process environment variable names
extern int total_env; // Total number of environment variables
extern char *local_variables[MAX_LOCAL_VARIABLES]; // Stores process local variable names
extern char *local_variable_values[MAX_LOCAL_VARIABLES]; // Stores process local variable values
extern int total_loc; // Total number of local variables

extern const char *built_in_commands[BUILT_IN_COMMANDS]; // Names of implemented built in commands
extern int built_in_spawn_child[BUILT_IN_COMMANDS]; // 1 if built in command needs to fork()
extern int (*built_in_functions[BUILT_IN_COMMANDS])(char **args); // Matching of built in command name to function

extern int num_running_processes; // Total number of running processes
extern int num_forked_processes; // Total number of forked processes in session
extern int pipe_failure; // 1 if current pipe failed
extern int running_processes[MAX_RUNNING_PROCESSES]; // Stores current running processes ids
extern int running_piped_commands[MAX_RUNNING_PROCESSES]; // Stores current running piped commands ids

#endif
