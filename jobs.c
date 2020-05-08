#include <fcntl.h>
#include <assert.h>
#include "jobs.h"
#include "shell.h"

/* Head of job list. */
job *head_job_list = NULL;

/* Check job contains only inner commands. */
int job_is_inner(job* jobs)
{
    if(!jobs)
        return 0;

    process *p;
    for (p = jobs->first_process; p; p = p->next)
        if(!command_is_inner(p->argv[0]))
            return 0;

    return 1;
}

/* Clear job list. */
void clear_job_list(int kill_jobs)
{
    job *j;
    for (int i = get_last_job_index(); i >= 0; --i)
    {
        j = find_job_jid(i);
        if(kill_jobs && !job_is_inner(j))
            kill(-j->pgid, SIGTERM);

        remove_job(j->pgid);
    }
}

/* Find the job with the indicated pgid. */
job *find_job_pgid(pid_t pgid)
{
    job *j;

    for (j = get_job_list_head(); j; j = j->next)
        if (j->pgid == pgid)
            return j;
    return NULL;
}

/* Return last index of job in list. */
int get_last_job_index()
{
    int i = 0;
    job *j;

    for (j = get_job_list_head(); j; j = j->next, ++i);

    return i - 1;
}

/* Get head of job list. */
job *get_job_list_head()
{
    return head_job_list;
}

/* Set head of job list. */
void set_job_list_head(job* job1)
{
    head_job_list = job1;
}

/* Get job by index in job list. */
job *find_job_jid(int index)
{
    job *j;
    int i = 0;

    for (j = get_job_list_head(); j; j = j->next, i++)
        if (i == index)
            return j;
    return NULL;
}

/* Return true if all processes in the job have stopped or completed. */
int job_is_stopped(job *jobs)
{
    if(!jobs)
        return 0;
    process *p;

    for (p = jobs->first_process; p; p = p->next)
        if (!p->completed && !p->stopped)
            return 0;

    return 1;
}

/* Return true if all processes in the job have completed. */
int job_is_completed(job *jobs)
{
    if(!jobs)
        return 0;
    process *p;

    for (p = jobs->first_process; p; p = p->next)
        if (!p->completed)
            return 0;
    return 1;
}

/* Return index of job in list. */
int get_job_index(pid_t pgid)
{
    if(pgid < 0)
        return -1;

    int i = 0;
    job *j;

    for (j = get_job_list_head(); j; j = j->next, ++i)
        if (j->pgid == pgid)
            return i;

    return -1;
}

/* Create job for foreground process. */
job* create_new_job(pid_t pgid, char* name)
{
    job* new_job = malloc(sizeof(job));

    if(!new_job)
    {
        perror("malloc");
        return NULL;
    }

    memset(new_job, 0, sizeof(job));

    new_job->command = malloc(sizeof(char) * (strlen(name) + 1));

    if(!new_job->command)
    {
        perror("malloc");
        free(new_job);
        return NULL;
    }

    memcpy(new_job->command, name, strlen(name) + 1);

    /* Remove all \n in the end of name. */
    for (int i = (int)strlen(name); i >= 0; --i)
        if(new_job->command[i] == '\n')
        {
            new_job->command[i] = '\0';
            if(i == 0 || new_job->command[i - 1] != '\n')
                break;
        }

    new_job->next = NULL;
    new_job->pgid = pgid;
    new_job->stderr_file = -1;
    new_job->stdout_file = -1;
    new_job->stdin_file = -1;
    new_job->tmodes = shell_tmodes;

    return new_job;
}

/* Add job to list. Return success, if added. */
int add_job(job* jobs)
{
    if(!jobs)
        return 0;

    job *j;

    for (j = get_job_list_head(); j && j->next; j = j->next);

    if(j)
        j->next = jobs;
    else
        set_job_list_head(jobs);

    return 1;
}

/* Remove job from job list. Return success, if removed. */
int remove_job(pid_t pgid)
{
    if(pgid < 0 || !get_job_list_head())
        return 0;

    job *j, *jlast = NULL;

    for (j = get_job_list_head(); j; j = j->next)
    {
        if(j->pgid == pgid)
        {
            job* next = j->next;
            free_job(j);
            if(jlast)
                jlast->next = next;
            else
                set_job_list_head(next);
            return 1;
        }
        jlast = j;
    }

    return 0;
}

/* Free memory of job. */
void free_job(job* jobs)
{
    if(!jobs)
        return;

    if(jobs->first_process)
    {
        process *p, *pnext;
        for (p = jobs->first_process; p; p = pnext)
        {
            for (int i = 0; i < MAXARGS; ++i)
                if(!p->argv[i])
                    break;
                else
                    free(p->argv[i]);
            pnext = p->next;
            free(p->argv);
            free(p);
        }
    }

    if(jobs->command)
        free(jobs->command);

    free(jobs);
}

/* Store the status of the process pid that was returned by waitpid.
   Return 0 if all went well, nonzero otherwise. */
int mark_process_status(pid_t pid, int status)
{
    job *j;
    process *p, *p2;

    if(pid > 0)
    {
        /* Update the record for the process. */
        for(j = get_job_list_head(); j; j = j->next)
        {
            for(p = j->first_process; p; p = p->next)
                if(p->pid == pid)
                {
                    p->status = status;
                    if (WIFSTOPPED(status))
                    {
                        p->stopped = 1;
                        if(j->have_pipe)
                        {
                            kill(-j->pgid, SIGSTOP);
                            for(p2 = j->first_process; p2; p2 = p2->next)
                                p2->stopped = 1;
                        }
                    }
                    else
                    {
                        p->completed = 1;
                        if(WIFSIGNALED(status))
                        {
                            if(invite_mode)
                            {
                                fprintf(stdout, "\n");
                                invite_mode = 0;
                            }
                            fprintf(stdout, "[%d] - %s: Terminated by signal %d.\n",
                                    get_job_index(j->pgid), p->argv[0], WTERMSIG (p->status));
                            fflush(stdout);
                        }
                        if(j->have_pipe)
                        {
                            kill(-j->pgid, SIGKILL);
                            for(p2 = j->first_process; p2; p2 = p2->next)
                                p2->completed = 1;
                        }
                    }
                    return 0;
                }
        }

        fprintf(stderr, "No child process %d.\n", pid);
        fflush(stderr);
        return -1;
    } else if (pid == 0 || errno == ECHILD)
        /* No processes ready to report. */
        return -1;
    else
    {
        /* Other weird errors. */
        perror("waitpid");
        return -1;
    }
}

/* Check job list on containing non inner commands. */
int job_list_is_inner()
{
    for(job *j = get_job_list_head(); j; j = j->next)
        if(!job_is_inner(j))
            return 0;

    return 1;
}

/* Check for processes that have status information available,
   without blocking. */
void update_job_status()
{
    int status;
    pid_t pid;

    /* Return, because of we haven't child processes, if condition is true. */
    if(!get_job_list_head() || job_list_is_inner())
        return;
    
    /* Wait all child, also we close zombie processes. */
    do
        pid = waitpid(WAIT_ANY, &status, WUNTRACED | WNOHANG);
    while (!mark_process_status(pid, status));
}

/* Format information about job status for the user to look at. */
void format_job_info(job *jobs, const char *status)
{
    if(status)
        fprintf(stdout, "[%d] (%s): %s", get_job_index(jobs->pgid), status, jobs->command);
    else
        /* Used for, if this job put in background. */
        fprintf(stdout, "[%d] : pgid -%u" , get_job_index(jobs->pgid), jobs->pgid);
    fflush(stdout);
}

/* Notify the user about stopped or terminated jobs.
   Delete terminated jobs from the active job list. */
void do_job_notification(int show_all)
{
    job *j;
    int printed = 0;

    /* Update status information for child processes. */
    update_job_status();

    for (j = get_job_list_head(); j; j = j->next)
    {
        /* Don't show jobs. */
        if(!strcmp(j->command, "jobs"))
            continue;

        /* If all processes have completed, tell the user the job has
           completed and delete it from the list of active jobs. */
        if (job_is_completed(j) && j != current_job)
        {
            if(invite_mode || printed)
                fprintf(stdout, "\n");

            format_job_info(j, "completed");
            remove_job(j->pgid);
            printed = 1;
            invite_mode = 0;
        }
            /* Notify the user about stopped jobs,
               marking them so that we won't do this more than once. */
        else if (job_is_stopped(j) && (!j->notified || show_all) && j != current_job)
        {
            if(invite_mode || printed)
                fprintf(stdout, "\n");

            format_job_info(j, "stopped");
            if(!show_all)
                j->notified = 1;
            printed = 1;
            invite_mode = 0;
        }
        /* Don't say anything about jobs that are still running. */
    }
    
    /* Make new line, if we printed something. */
    if(printed)
    {
        fprintf(stdout, "\n");
        fflush(stdout);
    }
}

/* Create job from command array from parsed line. */
void fill_job(job **jobs, int ncmds)
{
    if(!jobs || !*jobs)
        return;

    register int i, u;
    process *first_process = NULL;
    process *p = NULL;
    process *p_last = NULL;
    unsigned long size = 0;

    for (i = 0; i < ncmds + 1; i++)
    {
        size = 0;
        for (u = 0; u < MAXARGS; ++u)
        {
            size++;
            if (!cmds[i].cmdargs[u])
                break;
        }

        /* Avoid empty commands. */
        if(size == 1)
            continue;

        /* Create new process. */
        p_last = malloc(sizeof(process));

        if(!p_last)
        {
            perror("malloc");
            free_job(*jobs);
            (*jobs) = NULL;
            return;
        }

        /* Set all fields of p_last to 0 or NULL. */
        memset(p_last, 0, sizeof(process));

        /* Fill argv for p_last. */
        p_last->argv = malloc(size * sizeof(char *));

        if(!p_last->argv)
        {
            perror("malloc");
            free_job(*jobs);
            (*jobs) = NULL;
            return;
        }

        for (unsigned long j = 0; j < size; ++j)
            if (cmds[i].cmdargs[j])
            {
                /* copy arg string to p_last. */
                p_last->argv[j] = malloc((strlen(cmds[i].cmdargs[j]) + 1) * sizeof(char));

                if(!p_last->argv[j])
                {
                    perror("malloc");
                    free_job(*jobs);
                    (*jobs) = NULL;
                    return;
                }

                memcpy(p_last->argv[j], cmds[i].cmdargs[j], strlen(cmds[i].cmdargs[j]) + 1);
            }
            else
            {
                /* The end of args in p_last. */
                p_last->argv[j] = NULL;
                break;
            }

        if (!first_process)
            first_process = p_last;

        if (p)
            p->next = p_last;
        p = p_last;

        /* Open input file, if user redirect STDIN. */
        if (!i && infile)
        {
            int input = open(infile, O_RDONLY);
            if (input != -1)
                (*jobs)->stdin_file = input;
            else
            {
                perror("Couldn't open input file");
                free_job(*jobs);
                (*jobs) = NULL;
                return;
            }
        }

        /* Open input file, if user redirect STDOUT. */
        if (i == (ncmds - 1) && (outfile || appfile))
        {
            int output;
            if (outfile)
                /* Rewrite outfile. */
                output = open(outfile, O_WRONLY | O_TRUNC | O_CREAT, (mode_t) 0644);/* READ-WRITE-NOT_EXECUTE */
            else
                /* Append to end of outfile. */
                output = open(appfile, O_WRONLY | O_APPEND | O_CREAT, (mode_t) 0644);

            if (output != -1)
                (*jobs)->stdout_file = output;
            else
            {
                perror("Couldn't open output file");
                free_job(*jobs);
                (*jobs) = NULL;
                return;
            }
        }
    }
    /* Finally write created process chain to jobs. */
    (*jobs)->first_process = first_process;
}

/* Used only for SIGCHLD. */
void notify_child(__attribute__((unused)) int signum)
{
    if(invite_mode)
        /* Handler prints something, when shell waits input line. */
        do_job_notification(0);
}

/* Mark a stopped job as being running again. */
void mark_job_as_running(job *jobs)
{
    if(!jobs)
        return;
    process *p;

    for (p = jobs->first_process; p; p = p->next)
        p->stopped = 0;
    jobs->notified = 0;
}

/* Put job in the foreground. If cont is nonzero,
   restore the saved terminal modes and send the process group a
   SIGCONT signal to wake it up before we block.
   Job must be non null. */
void put_job_in_foreground(job *jobs, int cont)
{
    assert(jobs != NULL);
    /* Put the job into the foreground. */
    tcsetpgrp(shell_terminal, jobs->pgid);

    /* Send the job a continue signal, if necessary. */
    if (cont)
    {
        tcsetattr(shell_terminal, TCSADRAIN, &jobs->tmodes);
        if (kill(-jobs->pgid, SIGCONT) < 0)
            perror("kill(SIGCONT)");
    }

    /* Wait for it to report. */
    wait_for_job(jobs);

    /* Put the shell back in the foreground. */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Restore the shell's terminal modes. */
    tcgetattr(shell_terminal, &jobs->tmodes);
    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
    tcflush(shell_terminal, TCIFLUSH);
}

/* Put a job in the background. If the cont argument is true, send
   the process group a SIGCONT signal to wake it up.
   Job must be non null. */
void put_job_in_background(job *jobs, int cont)
{
    assert(jobs != NULL);

    /* Send the job a continue signal, if necessary. */
    if (cont)
        if (kill(-jobs->pgid, SIGCONT) < 0)
            perror("kill (SIGCONT)");
}

/* Check for processes that have status information available,
   blocking until all processes in the given job have reported.
   Job must be non null. */
void wait_for_job(job *jobs)
{
    assert(jobs != NULL);

    int status;
    pid_t pid;

    /* Return, because of we haven't child processes, if condition is true. */
    if(!get_job_list_head() || job_list_is_inner())
        return;

    /* Wait all child, also we close zombie processes. */
    do
        pid = waitpid(WAIT_ANY, &status, WUNTRACED);
    while (!mark_process_status(pid, status) && !job_is_stopped(jobs) && !job_is_completed(jobs));
}

/* Continue the job to work. Terminal will switch to this job, if foreground = 1. */
void continue_job(job *jobs, int foreground)
{
    assert(jobs != NULL);

    mark_job_as_running(jobs);
    if (foreground)
    {
        put_job_in_foreground(jobs, 1);
        /* Switch current_process. */
        current_job = jobs;
    }
    else
        put_job_in_background(jobs, 1);
}
