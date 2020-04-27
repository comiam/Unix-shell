#ifndef UNIX_SHELL_SHELL_H
#define UNIX_SHELL_SHELL_H

#include <fcntl.h>
#include <limits.h>
#include "jobs.h"
#include "signals.h"
#include "promptline.h"

/*  cmdflag's  */
#define OUTPIP  01
#define INPIP   02
#define HOST_NAME_MAX 64

command cmds[MAXCMDS];
char *infile, *outfile, *appfile;
job* current_job;
char bkgrnd;
int invite_mode;

#endif