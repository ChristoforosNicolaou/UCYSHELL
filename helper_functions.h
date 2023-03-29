#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Parses arguments
int parse_args(char **args, int argc, char **input, char **output, int *bg);

// Checks if "tokens" has a token = "value" and return index
int index_of(char **tokens, const char *value);

// Checks if "expression" contains character 'c' and return index 
int index_of_str(char *expression, char c);

// Checks if expression is a variable assignment
int is_variable_assignment(char *expression);

// Concatenates arguments to a single string and splits by delimiter
char *concat(char **args, char delimiter, int start, int end);

// Returns a substring in a given range
char *substr(char *string, int start, int end);

// Tokenize string "buf" into "tokens" based on "delimeters"
int tokenize(char *buf, const char *delimiter, char **tokens);

#endif
