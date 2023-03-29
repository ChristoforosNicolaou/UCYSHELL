#include "helper_functions.h"

// Parses arguments
int parse_args(char **args, int argc, char **input, char **output, int *bg)
{
	int i_index = index_of(args, "<");
	int o_index = index_of(args, ">");
	int bg_index = index_of(args, "&");
	
	// Check for bg/fg option
	if (bg_index < 0) 
	{
		*bg = 0; // Run in foreground
	}
	else 
	{
		if (bg_index != argc - 1)
		{
			// Error: & must be the last argument
			return -1;
		}
		
		*bg = 1; // Run in background
		free(args[bg_index]); args[bg_index] = NULL; // Free data
	}
	
	// Check for input redirection
	if (i_index < 0)
	{
		*input = NULL;
	}
	else
	{
		if (i_index == 0 || i_index == argc - 1)
		{
			// Error
			return -1;
		}
		
		*input = (char *) malloc(strlen(args[i_index + 1]) + 1);
		strcpy(*input, args[i_index + 1]);
		
		free(args[i_index]); args[i_index] = NULL; // Free data
		free(args[i_index + 1]); args[i_index + 1] = NULL; // Free data
	}
	
	// Check for output redirection
	if (o_index < 0)
	{
		*output = NULL;
	}
	else
	{
		if (o_index == 0 || o_index == argc - 1)
		{
			// Error
			return -1;
		}
		
		*output = (char *) malloc(strlen(args[o_index + 1]) + 1);
		strcpy(*output, args[o_index + 1]);
		
		free(args[o_index]); args[o_index] = NULL; // Free data
		free(args[o_index + 1]); args[o_index + 1] = NULL; // Free data
	}
	
	// Rearrange args so that all non NULL arguments are in the beginning
	int i, non_null_pos = 0;
	for (i = 0; i < argc; i++)
	{
		if (args[i] != NULL)
		{
			if (i != non_null_pos)
			{
				char *tmp = args[i];
				args[i] = args[non_null_pos];
				args[non_null_pos] = tmp; 
			}
			non_null_pos++;
		}
	}
	
	return non_null_pos; // Number of remaining arguments
}

// Checks if "tokens" has a token = "value" and return index
int index_of(char **tokens, const char *value)
{
	int i = 0;
	while (tokens[i] != NULL)
	{
		if (strcmp(tokens[i], value) == 0)
		{
			return i;
		}
		i++;
	}
	
	return -1;
}

// Checks if "expression" contains character 'c' and return index 
int index_of_str(char *expression, char c)
{
	int i;
	for (i = 0; i < strlen(expression); i++)
	{
		if (expression[i] == c)
		{
			return i;
		}
	}
	
	return -1;
}

// Checks if expression is a variable assignment
int is_variable_assignment(char *expression)
{
	if (index_of_str(expression, '=') >= 0)
	{
		return 1;
	}
	return 0;
}

// Concatenates arguments from start to end to a single string and splits by delimiter
char *concat(char **args, char delimiter, int start, int end)
{
	int i = start;
	int total_size = 0;
	
	while (i < end)
	{
		total_size += (strlen(args[i])) + 1; // +1 for delimiter 
		i++;
	}
	
	char *result = (char *) malloc(total_size);
	if (result == NULL)
	{
		perror("malloc");
		return NULL;
	}

	i = start;
	int c = 0;
	while (i < end)
	{
		int arg_c = 0;
		while (args[i][arg_c] != '\0')
		{
			result[c++] = args[i][arg_c++];
		}
		
		result[c++] = delimiter;
		i++;
	}
	result[total_size-1] = '\0';
	
	return result; 
}

// Returns a substring in a given range
char *substr(char *string, int start, int end)
{
	char *str = (char *) malloc(end - start + 1);
	if (str == NULL)
	{
		perror("malloc");
		return NULL;
	}
	
	int i = 0;
	while (i < end - start && start < strlen(string))
	{
		str[i] = string[start + i];
		i++;
	}
	str[i] = '\0';
	return str;
}

// Tokenize string "buf" into "tokens" based on "delimeters"
int tokenize(char *buf, const char *delimiters, char **tokens)
{
	char *token = strtok(buf, delimiters);
	
	int i = 0;
	while (token != NULL)
	{
		// Store token
		tokens[i] = (char *) malloc(strlen(token) + 1);
		if (tokens[i] == NULL)
		{
			perror("malloc");
			return -1;
		}
		
		strcpy(tokens[i++], token);
		
		// Get next token
		token = strtok(NULL, delimiters);
	}
	
	tokens[i] = NULL;
	return i; // Return number of tokens
}
