EPL421 Systems Programming
Programming Assignment 3: UCYSHELL

Author: Christoforos Nicolaou

This program is the implementation of a custom shell (UCYSHELL) that supports basic shell commands,
environmental and local variables, and built-in functions. 

To compile:
> make

To remove files:
> make clean

Notes:
> Maximum 10 running processes
> Multiple commands + piped commands supported (separated with ;)
> Multiple piped commands supported (separated with |)
> Each command (separated with ;) can be sent to the background using &
> Each non-piped command supports input/output redirection with <, >
> Commands in a pipe and piped sequences cannot be sent to the background
> Exit shell using exit/logout commands or with Ctrl-C
> Example given in assignment pdf runs perfectly fine

> Supported built-in commands:
- cd
- echo (Can also print variable values)
- env/printenv (Can be used in pipes)
- exec
- exit/logout
- export
- history (Can be used in pipes)
- read (multiple variables, print message with -p)

> Supported variables:
- All inherited environmental variables
- $HOSTNAME
- $RANDOM (Generates random value in range 0, 32767)
- Can add a new environmental variable declaration with "export var=value" (inherited to children)
- Can add a new local variable declaration with "var=value" (not inherited)
- Use echo $var to print a variable value
