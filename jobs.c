#include <fcntl.h>
#include "jobs.h"
#include "shell.h"

job *head_job_list = NULL;

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

/* Get job by index. */
job *find_job_jid(int index)
{
    job *j;
    int i = 0;

    for (j = get_job_list_head(); j; j = j->next, i++)
        if (i == index)
            return j;
    return NULL;
}

/* Return true if all processes in the job have stopped or completed.  */
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

/* Return true if all processes in the job have completed.  */
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

/* return index of job in list */
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

/* create job for foreground process */
job* create_new_job(pid_t pgid, char* name)
{
    job* new_job = malloc(sizeof(job));
    memset(new_job, 0, sizeof(job));
    new_job->command = malloc(sizeof(char) * (strlen(name) + 1));
    memcpy(new_job->command, name, strlen(name) + 1);

    for (size_t i = strlen(name); i >= 0; --i)
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

    return new_job;
}

/* add exist job to list */
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

/* remove job with closing streams */
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
                set_job_list_head(jlast);
            return 1;
        }
        jlast = j;
    }

    return 0;
}

/* free memory of job */
void free_job(job* jobs)
{
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
    free(jobs->command);
    free(jobs);
}

/* Store the status of the process pid that was returned by waitpid.
   Return 0 if all went well, nonzero otherwise.  */

int mark_process_status(pid_t pid, int status)
{
    job *j;
    process *p;

    if(pid > 0)
    {
        /* Update the record for the process.  */
        for(j = get_job_list_head(); j; j = j->next)
        {
            for(p = j->first_process; p; p = p->next)
                if(p->pid == pid)
                {
                    p->status = status;
                    if (WIFSTOPPED(status))
                        p->stopped = 1;
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
                    }
                    return 0;
                }
        }

        fprintf(stderr, "No child process %d.\n", pid);
        fflush(stderr);
        return -1;
    } else if (pid == 0 || errno == ECHILD)
        /* No processes ready to report.  */
        return -1;
    else
    {
        /* Other weird errors.  */
        perror("waitpid");
        return -1;
    }
}

/* check job contains only inner commands */
int job_is_inner(job* jobs)
{
    process *p;
    for (p = jobs->first_process; p; p = p->next)
        if(!command_is_inner(p->argv[0]))
            return 0;

    return 1;
}

/* check job list on containing non inner commands */
int job_list_is_inner()
{
    for(job *j = get_job_list_head(); j; j = j->next)
        if(!job_is_inner(j))
            return 0;

    return 1;
}

/* Check for processes that have status information available,
   without blocking.  */

void update_job_status()
{
    int status;
    pid_t pid;

    if(!get_job_list_head() || job_list_is_inner())
        return;
    do
        pid = waitpid(WAIT_ANY, &status, WUNTRACED | WNOHANG);
    while (!mark_process_status(pid, status));
}

/* Format information about job status for the user to look at.  */

void format_job_info(job *j, const char *status)
{
    if(status)
        fprintf(stdout, "[%d] (%s): %s", get_job_index(j->pgid), status, j->command);
    else
        fprintf(stdout, "[%d] : %s", get_job_index(j->pgid), j->command);
    fflush(stdout);
}

/* Notify the user about stopped or terminated jobs.
   Delete terminated jobs from the active job list.  */

void do_job_notification(int show_all)
{
    job *j;
    int printed = 0;

    /* Update status information for child processes.  */
    update_job_status();


    for (j = get_job_list_head(); j; j = j->next)
    {
        /* Dont show jobs */
        if(!strcmp(j->command, "jobs"))
            continue;

        /* If all processes have completed, tell the user the job has
           completed and delete it from the list of active jobs.  */
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
               marking them so that we won't do this more than once.  */
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
        /* Don't say anything about jobs that are still running.  */
    }
    if(printed)
    {
        fprintf(stdout, "\n");
        fflush(stdout);
    }
}

void fill_job(job* jobs, int ncmds)
{
    register int i, u;
    process *first_process = NULL;
    process *p = NULL;
    process *p_last = NULL;
    int size = 0;

    for (i = 0; i < ncmds; i++)
    {
        p_last = malloc(sizeof(process));
        memset(p_last, 0, sizeof(process));

        for (u = 0; u < MAXARGS; ++u)
        {
            size++;
            if (!cmds[i].cmdargs[u])
                break;
        }

        p_last->argv = malloc(size * sizeof(char *));
        for (int j = 0; j < size; ++j)
            if (cmds[i].cmdargs[j])
            {
                p_last->argv[j] = malloc((strlen(cmds[i].cmdargs[j]) + 1) * sizeof(char));
                memcpy(p_last->argv[j], cmds[i].cmdargs[j], strlen(cmds[i].cmdargs[j]) + 1);
            }
            else
            {
                p_last->argv[j] = NULL;
                break;
            }

        if (!first_process)
            first_process = p_last;

        if (p)
            p->next = p_last;
        p = p_last;

        if (!i && infile)
        {
            int input = open(infile, O_RDONLY);
            if (input != -1)
                jobs->stdin_file = input;
            else
            {
                perror("Couldn't open input file");
                exit(EXIT_FAILURE);
            }
        }
        if (i == (ncmds - 1) && (outfile || appfile))
        {
            int output;
            if (outfile)
                output = open(outfile, O_WRONLY | O_TRUNC | O_CREAT, (mode_t) 0644);//  READ-WRITE-NOT_EXECUTE
            else
                output = open(appfile, O_WRONLY | O_APPEND | O_CREAT, (mode_t) 0644);

            if (output != -1)
                jobs->stdout_file = output;
            else
            {
                perror("Couldn't open output file");
                exit(EXIT_FAILURE);
            }
        }
    }
    jobs->first_process = first_process;
}

/* used only for SIGCHLD*/
void notify_child(int signum)
{
    do_job_notification(0);
}