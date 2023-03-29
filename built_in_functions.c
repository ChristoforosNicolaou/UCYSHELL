#include "built_in_functions.h"

// Globals

char *history_commands[MAX_HISTORY_RECORDS] = {0};
char *environment_variables[MAX_ENVIRONMENT_VARIABLES] = {0};
int total_env = 0;
char *local_variables[MAX_LOCAL_VARIABLES] = {0};
char *local_variable_values[MAX_LOCAL_VARIABLES] = {0};
int total_loc = 0;

const char *built_in_commands[BUILT_IN_COMMANDS] = {"cd", "echo", "env", "printenv", "exec", "exit", "export", "history", "logout", "read", "unset"}; // Other built-in commands are already implemented
int built_in_spawn_child[BUILT_IN_COMMANDS] = {0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1};
int (*built_in_functions[BUILT_IN_COMMANDS])(char **args) = {cd, echo, env, env, exec, exit_shell, export, history, exit_shell, read_input, export};

int num_running_processes = 0;
int num_forked_processes = 0;
int pipe_failure = 0;
int running_processes[MAX_RUNNING_PROCESSES] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
int running_piped_commands[MAX_RUNNING_PROCESSES] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

// Functions

// Adds a local variable definition
int variable_assignment(char *expression)
{
	int index_eq = index_of_str(expression, '=');
	char *var_name = substr(expression, 0, index_eq);
	if (var_name == NULL)
	{
		return -1;
	}
	
	if (index_eq == strlen(expression) - 1) // Clear variable
	{
		int index;
		if ((index = index_of(local_variables, var_name)) >= 0)
		{
			free(local_variable_values[index]);
			local_variable_values[index] = (char *) malloc(1);
			local_variable_values[index][0] = '\0';
		}
		return 0;
	}
	
	char *var_value;
	if (expression[index_eq + 1] == '\"' || expression[index_eq + 1] == '\"') // If value contains quotes remove them
	{
		if (expression[strlen(expression) - 1] == '\"' || expression[strlen(expression) - 1] == '\"')
		{
			var_value = substr(expression, index_eq + 2, strlen(expression) - 1);
		}
		else
		{
			var_value = substr(expression, index_eq + 2, strlen(expression));
		}
	}
	else
	{
		var_value = substr(expression, index_eq + 1, strlen(expression));
	}
	
	if (var_value == NULL)
	{
		return -1;
	}
	
	int index;
	if ((index = index_of(local_variables, var_name)) >= 0) // If local variabe already declared
	{
		// Replace value
		free(local_variable_values[index]);
		local_variable_values[index] = (char *) malloc(strlen(var_value) + 1);
		strcpy(local_variable_values[index], var_value);
	}
	else // Add variable definition
	{
		local_variables[total_loc] = (char *) malloc(strlen(var_name) + 1);
		strcpy(local_variables[total_loc], var_name);
		local_variable_values[total_loc] = (char *) malloc(strlen(var_value) + 1);
		strcpy(local_variable_values[total_loc], var_value);
		total_loc++;
	}
	
	return 0;
}

// Check if command is built in
int is_built_in(char *command)
{
	int i;
	for (i = 0; i < BUILT_IN_COMMANDS; i++)
	{
		if (strcmp(built_in_commands[i], command) == 0)
		{
			return i;
		}
	}
	
	return -1;
}

// Executes built-in command
int execute_built_in(char **args, int index)
{
	if (index < 0 || index >= BUILT_IN_COMMANDS)
	{
		return -1;
	}
	
	return built_in_functions[index](args);
}

// Kills processes with given pids
void kill_processes(int *pids)
{
	int i;
	for (i = 0; i < MAX_RUNNING_PROCESSES; i++)
	{
		if (pids[i] > 0)
		{
			kill(pids[i], SIGKILL);
		}
	}
}

// Built in commands

// Built-in cd command
int cd(char **args)
{
	if (args[1] == NULL)
	{
		return -1;
	}
	
	if (chdir(args[1]) < 0)
	{
		perror("cd");
		return -1;
	}
	
	return 0;
}

// Built-in echo command
int echo(char **args)
{
	int i = 1; // Skip echo arg
	int ignore_variables = 0;
	while (args[i] != NULL)
	{
		char *arg;
		// Remove first quote
		if (i == 1 && args[i][0] == '\'') // Ignore variables
		{
			arg = substr(args[i], 1, strlen(args[i]));
			ignore_variables = 1;
		}
		else if (i == 1 && args[i][0] == '\"')
		{	
			arg = substr(args[i], 1, strlen(args[i]));
		}
		else
		{
			arg = args[i];
		}
		
		if (args[i + 1] == NULL) // Remove last quote 
		{
			int len = strlen(arg) - 1;
			if (arg[len] == '\"' || arg[len] == '\'')
			{
				arg = substr(arg, 0, len);
			}
		}
		
		int printed = 1;
		if (arg[0] == '$' && !ignore_variables) // If arg is variable -> print value
		{
			char *var_name = substr(arg, 1, strlen(arg));
			int index;
		
			if (strcmp(var_name, "RANDOM") == 0)
			{
				int rand_int = rand() % 32768;
				char rand_str[5];
				sprintf(rand_str, "%d", rand_int);
				write(STDOUT_FILENO, rand_str, strlen(rand_str));
			}	
			else if (strcmp(var_name, "HOSTNAME") == 0)
			{
				char hostname[HOST_NAME_MAX + 1]; 
				gethostname(hostname, HOST_NAME_MAX + 1);
				write(STDOUT_FILENO, hostname, strlen(hostname));
			}
			else if ((index = index_of(environment_variables, var_name)) >= 0) // Environmental variable
			{
				char *value = getenv(var_name);
				write(STDOUT_FILENO, value, strlen(value));
				printed = 1;
			}
			else if ((index = index_of(local_variables, var_name)) >= 0) // Local variable
			{
				write(STDOUT_FILENO, local_variable_values[index], strlen(local_variable_values[index]));
			}
			else
			{
				printed = 0;
			}
		}
		else // Print normal string
		{
			write(STDOUT_FILENO, arg, strlen(arg));
		}
		i++;
		
		if (args[i] != NULL && printed)
		{
			write(STDOUT_FILENO, " ", 1);
		}
	}
	write(STDOUT_FILENO, "\n", 1);
	
	return 0;
}

// Built-in env command
int env(char **args)
{
	// print environment variables == export
	int i = 0;
	while (environment_variables[i] != NULL)
	{
		char *env = getenv(environment_variables[i]);
		if (env != NULL)
		{
			char *name_val = (char *) malloc(strlen(environment_variables[i]) + strlen(env) + 3);
			sprintf(name_val, "%s=%s\n", environment_variables[i], env);
			write(STDOUT_FILENO, name_val, strlen(name_val));
		}
		i++;
	}
	
	return 0;
}

// Built-in exec command
int exec(char **args)
{
	if (execvp(args[1], args + 1) < 0)
	{
		perror("execvp");
		return -1;
	}
	
	return 0;
}

// Built-in exit command
int exit_shell(char **args)
{	
	int exit_code;
	if (args[1] == NULL)
	{
		exit_code = 0;
	}
	else
	{
		exit_code = atoi(args[1]);
	}
	
	// Free resources 
	int i_args = 0;
	while (args[i_args] != NULL)
	{
		free(args[i_args++]);
	}
	char **tmp = history_commands;
	while(*tmp != NULL)
	{
		free(*tmp++);
	}

	tmp = environment_variables;
	while(*tmp != NULL)
	{
		free(*tmp++);
	}
	
	tmp = local_variables;
	while(*tmp != NULL)
	{
		free(*tmp++);
	}
	
	tmp = local_variable_values;
	while(*tmp != NULL)
	{
		free(*tmp++);
	}

	// Kill running processes
	kill_processes(running_processes);
	exit(exit_code);
	return 0;
}

// Built-in export command
int export(char **args)
{
	if (args[1] == NULL)
	{
		return env(args);
	}
	else
	{
		int index_eq;
		if ((index_eq = index_of_str(args[1], '=')) >= 0)
		{
			char *name = substr(args[1], 0, index_eq);
			
			// Copy env variable so it is not lost after free
			char *env_var = (char *) malloc(strlen(args[1]) + 1);
			strcpy(env_var, args[1]);
			
			if (getenv(name) == NULL) // If variable does not exist
			{
				// Add the variable
				putenv(env_var);
				
				// Add to global also
				environment_variables[total_env++] = name;
			}
			else
			{
				putenv(env_var);
			}
		}
		else if (strcmp(args[0], "unset") == 0)
		{
			return putenv(args[1]); // Delete variable
		}
	}
	
	return 0;
}

// Built-in history command
int history(char **args) // no args
{
	char **current_command = history_commands;
	int line = 0;
	char tmp[INPUT_BUF_SIZE];
	while (*current_command != NULL)
	{
		sprintf(tmp, "%d\t%s", line++, *current_command++);
		write(STDOUT_FILENO, tmp, strlen(tmp));
	}
	
	return 0;
}

// Built-in read command
int read_input(char **args)
{
	if (args[1] == NULL)
	{
		fprintf(stderr, "Unable to execute command\n");
		return -1;
	}

	int index;
	if (strcmp(args[1], "-p") == 0) // Contains also ouput message
	{
		if (args[2] == NULL)
		{
			fprintf(stderr, "Unable to execute command\n");
			return -1;
		}
		
		char *message;
		char quotes = args[2][0];
		int start, end = 0;
		if (quotes == '\"' || quotes == '\'') // If message contains quotes join arguments within and remove the quotes
		{
			if (args[2][strlen(args[2]) - 1] == quotes)
			{
				message = substr(args[2], 1, strlen(args[2]) - 1);
				end = 3;
			}
			else // If spaces between quotes
			{
				start = 2;
				int i = start + 1;
				while (args[i] != NULL)
				{
					if (args[i][strlen(args[i])-1] == quotes)
					{
						end = i + 1;
						break;
					}
					i++;
				}
				
				if (end == 0) // No end quote
				{
					fprintf(stderr, "Unable to execute command\n");
					return -1;
				}
				
				char *cat = concat(args, ' ', start, end);
				
				message = substr(cat, 1, strlen(cat) - 1);
			}
		}
		else
		{
			message = substr(args[2], 0, strlen(args[2]));
			end = 3;
		}
		
		printf("%s", message);
		
		index = end;
	}
	else
	{
		index = 1;
	}
	
	char input_buf[INPUT_BUF_SIZE];
	
	// Get user input
	if (fgets(input_buf, sizeof(input_buf), stdin) == NULL)
	{
		perror("fgets");
		return -1;
	}
	
	char *tokens[MAX_ARGS];
	int num_tokens;
	if ((num_tokens = tokenize(input_buf, " \n", tokens)) < 0)
	{
		fprintf(stderr, "Unable to tokenize variables\n");
		return -1;
	}
	
	char expression[INPUT_BUF_SIZE];
	int i_token = 0;
	
	while (args[index] != NULL)
	{
		if (args[index + 1] == NULL) // If last variable
		{
			char *cat = concat(tokens, ' ', i_token, num_tokens); // Concat remaining tokens
			sprintf(expression, "%s=%s", args[index], cat);
			variable_assignment(expression);
			
			break;
		}
		
		if (i_token < num_tokens) // Next variable = next token
		{
			sprintf(expression, "%s=%s", args[index], tokens[i_token]);
			variable_assignment(expression);
		}
		else // Out of tokens
		{
			sprintf(expression, "%s=", args[index]);
			variable_assignment(expression);
		}
		
		i_token++;
		index++;
	}
	
	return 0;
}
