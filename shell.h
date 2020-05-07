#ifndef UNIX_SHELL_SHELL_H
#define UNIX_SHELL_SHELL_H

#include <fcntl.h>
#include <limits.h>
#include "jobs.h"
#include "signals.h"
#include "promptline.h"

#define HOST_NAME_MAX 64
#define READ_LINE_SIZE 1024

#ifndef LOGIN_NAME_MAX
#    define LOGIN_NAME_MAX 256
#endif

command cmds[MAXCMDS]; /* current set of parsed commands from line */
char *infile, *outfile, *appfile; /* files for redirect streams of new job */
job* current_job; /* current foreground working job */
char bkgrnd;      /* flag for the process in the background */
int invite_mode;  /* flag for waiting for input from the terminal.
                     Used in SIGCHLD handler and do_job_notification() */

pid_t shell_pgid; /* shell process group ID */
struct termios shell_tmodes; /* saved attributes of shell terminal */
int shell_terminal;          /* descriptor of shell STDIN */

/* Exit from shell. */
void shell_exit(int stat);

#endif
