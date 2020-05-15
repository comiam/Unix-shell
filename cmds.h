#ifndef UNIX_SHELL_CMDS_H
#define UNIX_SHELL_CMDS_H

#include "dirs.h"

/* Flags for command structure. */
#define OUTPIP  01
#define INPIP   02

#define MAXARGS            256 /* maximal number of arguments for one command */
#define MAXCMDS            50  /* maximal number of commands */
#define MAY_EXIT          -356 /* special codes for exec_inner */
#define EXEC_SUCCESS      -357
#define EXEC_FAILED       -358
#define NOT_INNER_COMMAND -359

typedef struct command_str
{
    char *cmdargs[MAXARGS];  /* arguments for command */
    char cmdflag;            /* used for syntax checking in line parser */
} command;

/* Check command name is inner realized command. */
int exec_inner(const char *name, const char *argv[], int input_file, int output_file);

/* Exec inner command.
   Notify, this commands not executed in forked process.
   Fall, if name == NULL. */
int command_is_inner(const char* name);

#endif
