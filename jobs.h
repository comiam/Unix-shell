#ifndef UNIX_SHELL_JOBS_H
#define UNIX_SHELL_JOBS_H

#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <wait.h>
#include "string.h"
#include "cmds.h"

#ifndef __USE_MISC
#    define WAIT_ANY -1
#endif

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

/* Create job from command array from parsed line. */
void fill_job(job* jobs, int ncmds);

/* Get head of job list. */
job *get_job_list_head();

/* Set head of job list. */
void set_job_list_head(job* job1);

/* Get job by index in job list. */
job *find_job_jid(int index);

/* Find the job with the indicated pgid. */
job *find_job_pid(pid_t pgid);

/* Create job for foreground process. */
job* create_new_job(pid_t pgid, char* name);

/* Return true if all processes in the job have stopped or completed. */
int job_is_stopped(job *jobs);

/* Return true if all processes in the job have completed. */
int job_is_completed(job *jobs);

/* Check job list on containing non inner commands. */
int job_list_is_inner();

/* Return index of job in list. */
int get_job_index(pid_t pgid);

/* Return last index of job in list. */
int get_last_job_index();

/* Add job to list. Return success, if added. */
int add_job(job* jobs);

/* Remove job from job list. Return success, if removed. */
int remove_job(pid_t pgid);

/* Free memory of job. */
void free_job(job* jobs);

/* Notify the user about stopped or terminated jobs.
   Delete terminated jobs from the active job list. */
void do_job_notification(int show_all);

/* Used only for SIGCHLD. */
void notify_child(int signum);

/* Format information about job status for the user to look at. */
void format_job_info(job *j, const char *status);

/* Check for processes that have status information available,
   without blocking. */
void update_job_status();

/* Store the status of the process pid that was returned by waitpid.
   Return 0 if all went well, nonzero otherwise. */
int mark_process_status(pid_t pid, int status);

/* Mark a stopped job as being running again. */
void mark_job_as_running(job *j);

/* Continue the job to work. Terminal will switch to this job, if foreground = 1. */
void continue_job(job *j, int foreground);

/* Put job in the foreground. If cont is nonzero,
   restore the saved terminal modes and send the process group a
   SIGCONT signal to wake it up before we block. */
void put_job_in_foreground(job *j, int cont);

/* Put a job in the background. If the cont argument is true, send
   the process group a SIGCONT signal to wake it up. */
void put_job_in_background(job *j, int cont);

/* Check for processes that have status information available,
   blocking until all processes in the given job have reported. */
void wait_for_job(job *j);

#endif
