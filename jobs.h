#ifndef UNIX_SHELL_JOBS_H
#define UNIX_SHELL_JOBS_H

#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <wait.h>
#include "string.h"
#include "cmds.h"

typedef struct process
{
    struct process *next;       /* next process in pipeline */
    char **argv;                /* for exec */
    pid_t pid;                  /* process ID */
    char completed;             /* true if process has completed */
    char stopped;               /* true if process has stopped */
    int status;                 /* reported status value */
} process;

/* A job is a pipeline of processes.  */
typedef struct job
{
    struct job *next;
    char *command;              /* command line, used for messages */
    process *first_process;     /* list of processes in this job */
    pid_t pgid;                 /* process group ID */
    char notified;              /* true if user told about stopped job */
    struct termios tmodes;      /* saved terminal modes */
    int stdin_file, stdout_file, stderr_file;  /* standart i/o channels */
} job;

/* create job from command array from parsed line */
void fill_job(job* jobs, int ncmds);

/* Get head of job list. */
job *get_job_list_head();

/* Set head of job list. */
void set_job_list_head(job* job1);

/* Get job by index. */
job *find_job_jid(int index);

/* create job for foreground process */
job* create_new_job(pid_t pgid, char* name);

/* Return true if all processes in the job have stopped or completed.  */
int job_is_stopped(job *jobs);

/* Return true if all processes in the job have completed.  */
int job_is_completed(job *jobs);

/* check job list on containing non inner commands */
int job_list_is_inner();

/* return index of job in list */
int get_job_index(pid_t pgid);

/* add exist job to list */
int add_job(job* jobs);

/* remove job with closing streams */
int remove_job(pid_t pgid);

/* free memory of job */
void free_job(job* jobs);

/* Notify the user about stopped or terminated jobs.
   Delete terminated jobs from the active job list.  */
void do_job_notification(int show_all);

/* used only for SIGCHLD*/
void notify_child(int signum);

/* Format information about job status for the user to look at.  */
void format_job_info(job *j, const char *status);

/* Check for processes that have status information available,
   without blocking.  */
void update_job_status();

/* Store the status of the process pid that was returned by waitpid.
   Return 0 if all went well, nonzero otherwise.  */
int mark_process_status(pid_t pid, int status);

#endif //UNIX_SHELL_JOBS_H
