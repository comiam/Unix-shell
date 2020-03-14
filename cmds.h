#ifndef UNIX_SHELL_CMDS_H
#define UNIX_SHELL_CMDS_H

#include "dirs.h"

#define MAXARGS            256
#define MAXCMDS            50
#define MAY_EXIT          -356
#define EXEC_SUCCESS      -357
#define EXEC_FAILED       -358
#define NOT_INNER_COMMAND -359

struct command
{
    char *cmdargs[MAXARGS];
    char cmdflag;
};

int wait_process(pid_t pid);
int exec_inner(struct command *cmd);
#endif
