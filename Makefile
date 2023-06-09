###############################################
# Makefile for compiling the program skeleton
# 'make' build executable file 'PROJ'
# 'make clean' removes all .o and executable
###############################################
PROJ = ucysh # the name of the project
CC = gcc # name of compiler
# define any compile-time flags
CFLAGS = -Wall # there is a space at the end of this
###############################################
# You don't need to edit anything below this line
###############################################
# list of object files
# The following includes all of them!
C_FILES := $(wildcard *.c)
OBJS := $(patsubst %.c, %.o, $(C_FILES))
# To create the executable file we need the individual
# object files
$(PROJ): $(OBJS)
	$(CC) -o $(PROJ) $(OBJS)
# To create each individual object file we need to
# compile these files using the following general
# purpose macro
.c.o:
	$(CC) $(CFLAGS) -c $<
# there is a TAB for each identation.
# To clean .o files: "make clean"
clean:
	rm -rf *.o $(PROJ)
