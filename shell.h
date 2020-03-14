#ifndef UNIX_SHELL_SHELL_H
#define UNIX_SHELL_SHELL_H

#include <unistd.h>
#include <fcntl.h>
#include <wait.h>
#include <limits.h>
#include "promptline.h"
#include "cmds.h"

/*  cmdflag's  */
#define OUTPIP  01
#define INPIP   02

extern struct command cmds[];
extern char *infile, *outfile, *appfile;
extern char bkgrnd;

#endif