#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include "helper_functions.h"
#include "built_in_functions.h"

#define MAX_COMMANDS 512
#define MAX_PIPED_COMMANDS 512
#define MAX_PIPES 9

#define EXIT_CHILD -2

#define READ 0
#define WRITE 1


// Process handling funuctions

// Signal handler
void signal_handler(int sig);

// Add a process id to the running processes
int add_running_process(int pid, int *pids);

// Remove a process id from the running processes
int remove_running_process(int pid, int *pids);

// Execute a command that is in a pipe sequence
int execute_piped(char **argv, int index_r, int index_w, int (*pipes)[2], int num_pipes, int bg);

// Execute a command
int execute(char **argv, int fd_r, int fd_w, int bg);


int main(int argc, char **argv, char **environ)
{
	// Get a reference to inherited environment variables names
	while (total_env < MAX_ENVIRONMENT_VARIABLES && environ[total_env] != NULL)
	{
		int index_eq = index_of_str(environ[total_env], '=');
		char *name = substr(environ[total_env], 0, index_eq);
		environment_variables[total_env] = (char *) malloc(strlen(name) + 1);
		strcpy(environment_variables[total_env], name);
		total_env++;
	}
	
	// Init rng
	srand(time(NULL));
	
	int i_hist = 0;
	
	int num_commands = 0;
	int num_piped_commands = 0;
	int num_args = 0;
	
	int pipes[MAX_PIPES][2];
	
	char input_buf[INPUT_BUF_SIZE];
	char *args[MAX_ARGS];
	char *piped_commands[MAX_PIPED_COMMANDS];
	char *commands[MAX_COMMANDS];
	
	// Set signal handler
	signal(SIGCHLD, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGUSR1, signal_handler);
	
	while (1)
	{
		printf("%d-ucysh> ", num_forked_processes);
		
		// Get user input
		if (fgets(input_buf, sizeof(input_buf), stdin) == NULL)
		{
			perror("fgets"); exit(1);
		}
		
		// Add command to history
		history_commands[i_hist] = (char *) malloc(strlen(input_buf) + 1);
		strcpy(history_commands[i_hist++], input_buf); 
		
		// Tokenize commands based on ';'
		if ((num_commands = tokenize(input_buf, ";\n", commands)) < 0)
		{
			fprintf(stderr, "Unable to tokenize commands\n");
			exit(1);
		}
		
		int i_comm = 0;
		//printf("Total %d commands\n", num_commands);
		
		// For each command
		while (commands[i_comm] != NULL)
		{
			//printf("Command: %s\n", commands[i_comm]);
			
			// Tokenize commands based on '|'
			if ((num_piped_commands = tokenize(commands[i_comm], "|", piped_commands)) < 0)
			{
				fprintf(stderr, "Unable to tokenize piped commands\n");
				exit(1);
			}
			
			int i_piped = 0;
			//printf("Total %d piped commands\n", num_piped_commands);
			
			// If no pipe -> 1 command
			if (num_piped_commands == 1)
			{
				//printf("\tOnly one sub-command: %s\n", piped_commands[i_piped]);
				
				// Tokenize commands based on whitespaces
				if ((num_args = tokenize(piped_commands[i_piped], " \t\n", args)) < 0)
				{
					fprintf(stderr, "Unable to tokenize arguments\n");
					exit(1);
				}
				
				char *input_file = NULL, *output_file = NULL;
				int bg = 0;
				if ((num_args = parse_args(args, num_args, &input_file, &output_file, &bg)) < 0)
				{
					fprintf(stderr, "Invalid arguments\n");
					//exit(1);
				}
				else
				{
					int fd_r = -1, fd_w = -1;
					int exit_code, input_ok = 1;
					// printf("Num args after parsing: %d\n", num_args);
					// if (input_file != NULL) printf("Input file: %s\n", input_file);
					// if (output_file != NULL) printf("Output file: %s\n", output_file);
					// printf("%s\n", (bg) ? "Background" : "Foreground");
					
					// Redirect input
					if (input_file != NULL)
					{
						if ((fd_r = open(input_file, O_RDONLY)) < 0)
						{
							perror("open");
							input_ok = 0;
						}
					}
					
					// Redirect output
					if (output_file != NULL)
					{
						if ((fd_w = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0600)) < 0)
						{
							perror("open");
							input_ok = 0;
						}
					}
					
					// Execute command
					if ((exit_code = execute(args, fd_r, fd_w, bg)) < 0)
					{
						fprintf(stderr, "Unable to execute command\n");
						if (exit_code == EXIT_CHILD)
						{
							exit(1); // Child
						}
					}
					else if (input_ok)
					{
						num_forked_processes++;
					}
				}
				
				// Free resources 
				int i_args = 0;
				while (args[i_args] != NULL)
				{
					free(args[i_args++]);
				}
				 
				free(input_file);
				free(output_file);
				free(piped_commands[i_piped]);
			}
			else // Not allowed input/output redirection from/to file
			{
				// Open pipe file descriptors
				int i_fd, pipe_ok = 1;
				int pipes_needed = (num_piped_commands - 1 < MAX_PIPES) ? num_piped_commands - 1 : MAX_PIPES;
				for (i_fd = 0; i_fd < pipes_needed; i_fd++)
				{
					if (pipe(pipes[i_fd]) < 0)
					{
						perror("pipe");
						pipe_ok = 0;
						break;
					}
				}
				
				// Check if it is able to spawn processes
				if (num_running_processes + num_piped_commands > MAX_RUNNING_PROCESSES)
				{
					fprintf(stderr, "Insufficient Resources\n");
				}
				else if (pipe_ok)
				{
					int pid, index_r, index_w, bg, current_running = 0;
					
					// Initialize running piped commands to -1 so kill does not crash
					for (current_running = 0; current_running < MAX_RUNNING_PROCESSES; current_running++)
					{
						running_piped_commands[current_running] = -1;
					}
					current_running = 0;
					
					pipe_failure = 0;
					
					for (i_piped = 0; i_piped < num_piped_commands && !pipe_failure; i_piped++)
					{
						// Tokenize commands based on whitespaces
						if ((num_args = tokenize(piped_commands[i_piped], " \t\n", args)) < 0)
						{
							fprintf(stderr, "Unable to tokenize arguments\n");
							exit(1);
						}
						
						// First command
						if (i_piped == 0)
						{
							index_r = -1; // Default (STDIN)
							index_w = i_piped;
							bg = 1;
						} 
						// Last command
						else if (i_piped == num_piped_commands - 1)
						{
							index_r = i_piped-1;
							index_w = -1; // Default (STDOUT)
							bg = 0; // Wait only for last command
						} 
						// In between commands
						else
						{
							index_r = i_piped-1;
							index_w = i_piped;
							bg = 1;
						}
						
						// Execute command
						if ((pid = execute_piped(args, index_r, index_w, pipes, pipes_needed, bg)) < 0)
						{
							// Child
							fprintf(stderr, "Unable to execute command\n");
							
							// Free resources 
							int i_args = 0;
							while (args[i_args] != NULL)
							{
								free(args[i_args++]);
							}
							
							if (pid == EXIT_CHILD)
							{
								kill(getppid(), SIGUSR1);
								exit(1); // Child
							} 
						}
						else
						{
							num_forked_processes++;
							running_piped_commands[current_running++] = pid;
						}
						
						// Free resources 
						int i_args = 0;
						while (args[i_args] != NULL)
						{
							free(args[i_args++]);
						}
					}
				}
			
				// Close any remaining open file descriptors
				for (i_fd = 0; i_fd < pipes_needed; i_fd++)
				{
					close(pipes[i_fd][READ]);
					close(pipes[i_fd][WRITE]);
				}
				
				// Free resources
				for (i_piped = 0; i_piped < num_piped_commands; i_piped++)
				{
					free(piped_commands[i_piped]);
				}
			}
			
			// Free resources
			free(commands[i_comm]);
			i_comm++;
		}
		
		// Print history
		//history(history_commands);
	}
	return 0;
}

void signal_handler(int sig)
{
	if (sig == SIGCHLD)
	{
		//printf("SIGCHLD\n");
		int pid, status;
		/* // HAD PROBLEMS WITH ZOMBIE CHILDREN
		if ((pid = wait(&status)) > 0)
		{
			//printf("Process %d terminated with exit code %d\n", pid, status >> 8);
			printf("Received SIGCHLD %d - ", pid);
			remove_running_process(pid, running_processes);
		}
		*/
		while ((pid = waitpid(-1, &status, WNOHANG)) > 0) 
		{
			if (WIFEXITED(status)) 
			{
			    
				//printf("Process %d terminated with exit code %d\n", pid, status >> 8);
				//printf("Received SIGCHLD %d - ", pid);
				remove_running_process(pid, running_processes);
			}
		}
		
		return;
	}
	
	if (sig == SIGUSR1) // Pipe failure
	{
		kill_processes(running_piped_commands);
		pipe_failure = 1;
	}

	if (sig == SIGINT)
	{
		char *args[2] = {NULL, NULL};
		exit_shell(args);
	}
}

int add_running_process(int pid, int *pids)
{
	int i;
	for (i = 0; i < MAX_RUNNING_PROCESSES; i++)
	{
		// Find empty space
		if (pids[i] < 0)
		{
			pids[i] = pid;
			
			num_running_processes++;
			
			return i;
		}
	}
	
	return -1; // Full
}

int remove_running_process(int pid, int *pids)
{
	int i;
	for (i = 0; i < MAX_RUNNING_PROCESSES; i++)
	{
		// Find process pid
		if (pids[i] == pid)
		{
			pids[i] = -1;
			//printf("Removed %d\n", pid);
			num_running_processes--;
			
			return i;
		}
	}
	//printf("Couldn't find %d\n", pid);
	return -1; // Not found
}

int execute_piped(char **argv, int index_r, int index_w, int (*pipes)[2], int num_pipes, int bg)
{
	if (is_variable_assignment(argv[0]))
	{
		return variable_assignment(argv[0]);
	}
	
	int built_in_index = is_built_in(argv[0]);
	if ((built_in_index == 7 && argv[1] != NULL) /*export*/ || (built_in_index >=0 && !built_in_spawn_child[built_in_index])) 
	{
		return execute_built_in(argv, built_in_index);
	}

	int pid; //, status;
	if ((pid = fork()) < 0)
	{
		perror("fork"); return -1;
	}	
	else if (pid == 0) // Child process
	{
		// Redirect input
		if (index_r != -1)
		{
			close(pipes[index_r][WRITE]); // Close write end
			close(STDIN_FILENO);
			if (dup2(pipes[index_r][READ], STDIN_FILENO) < 0)
			{
				perror("dup2");	
			}
			close(pipes[index_r][READ]);
		}
		
		// Redirect output
		if (index_w != -1)
		{
			close(pipes[index_w][READ]); // Close read end
			close(STDOUT_FILENO);
			if (dup2(pipes[index_w][WRITE], STDOUT_FILENO) < 0)
			{
				perror("dup2");	
			}			
			close(pipes[index_w][WRITE]);
		}
		
		if (built_in_index >= 0) // If command is built-in
		{
			exit(execute_built_in(argv, built_in_index));
		}

		// Else normal command
		if (execvp(argv[0], argv) < 0)
		{
			perror(argv[0]); 
			return EXIT_CHILD;
		}
	}
	else // Parent process
	{	
		int index = add_running_process(pid, running_processes);
		
		if (index_r != -1)
		{
			close(pipes[index_r][READ]);
			close(pipes[index_r][WRITE]);
		}
		// Close open pipe file descriptors when at last command
		if (!bg)
		{
			// Busy waiting -> stop when remove_running_process called
			while (running_processes[index] == pid);
		}
	}
	
	return pid;
}

int execute(char **argv, int fd_r, int fd_w, int bg)
{
	if (is_variable_assignment(argv[0]))
	{
		return variable_assignment(argv[0]);
	}

	if (num_running_processes >= MAX_RUNNING_PROCESSES)
	{
		fprintf(stderr, "Insufficient Resources\n");
		return -1;
	}
	
	int built_in_index = is_built_in(argv[0]);
	if (built_in_index >=0 && !built_in_spawn_child[built_in_index]) 
	{
		return execute_built_in(argv, built_in_index);
	}

	int pid; //, status;
	if ((pid = fork()) < 0)
	{
		perror("fork"); return -1;
	}	
	else if (pid == 0) // Child process
	{
		// Redirect input
		if (fd_r != -1)
		{
			close(STDIN_FILENO);
			if (dup2(fd_r, STDIN_FILENO) < 0)
			{
				perror("dup2");	
			}
			close(fd_r);
		}
		
		// Redirect output
		if (fd_w != -1)
		{
			close(STDOUT_FILENO);
			if (dup2(fd_w, STDOUT_FILENO) < 0)
			{
				perror("dup2");	
			}			
			close(fd_w);
		}
		
		if (built_in_index >= 0) // If command is built-in
		{
			exit(execute_built_in(argv, built_in_index));
		}

		// Else normal command
		if (execvp(argv[0], argv) < 0)
		{
			perror(argv[0]); 
			return EXIT_CHILD;
		}
	}
	else // Parent process
	{
		int index = add_running_process(pid, running_processes);
		if (bg) // Background -> don't wait
		{
			//printf("%d\n", pid);
		}
		else // Foreground -> no need to add to running processes since parent will wait
		{
			// Busy waiting -> stop when remove_running_process called
			while (running_processes[index] == pid);
		}
	}
	
	return pid;
}



