#include <stdio.h>
#include "cmds.h"
#include "jobs.h"
#include "shell.h"

/* Check command name is inner realized command. */
int command_is_inner(const char* name)
{
    return !strcmp(name, "cd") || !strcmp(name, "exit") || !strcmp(name, "jobs") || !strcmp(name, "bg") || !strcmp(name, "fg");
}

/* Exec inner command. Notify, this commands not executed in forked process. */
int exec_inner(const char *name, const char *argv[])
{
    if(!name || !argv)
        return EXEC_FAILED;

    if(!strcmp(name, "exit"))
        /* Return code for exit from shell. */
        return MAY_EXIT;
    else if(!strcmp(name, "cd"))
    {
        int size = 0;
        int i = 1;

        /* Check args. */
        while(argv[i++])
            size++;

        if(size > 1)
        {
            fprintf(stderr, "%s: Too many args!\n", argv[1]);
            fflush(stderr);
            return EXEC_FAILED;
        }

        int status = set_directory(argv[1]);

        /* Check status of last command. */
        switch(status)
        {
            case DIR_EXIST:
                return EXEC_SUCCESS;
            case DIR_NOT_EXIST:
                fprintf(stderr, "%s: No such file or directory!\n", argv[1]);
                fflush(stderr);
                return EXEC_FAILED;
            case CANT_OPEN_DIR:
                fprintf(stderr, "%s: Can't open dir!\n", argv[1]);
                fflush(stderr);
                return EXEC_FAILED;
            case DIR_IS_FILE:
                fprintf(stderr, "%s: Not a directory!\n", argv[1]);
                fflush(stderr);
                return EXEC_FAILED;
            default:
                return EXEC_FAILED;
        }
    }else if(!strcmp(name, "jobs"))
    {
        /* Show all executed jobs, but not terminated, jobs. */
        do_job_notification(1);

        return EXEC_SUCCESS;
    }else if(!strcmp(name, "bg"))
    {
        int size = 0;
        int i = 1;

        /* Check args. */
        while(argv[i++])
            size++;

        if(size > 1)
        {
            fprintf(stderr, "%s: Too many args!\n", argv[1]);
            fflush(stderr);
            return EXEC_FAILED;
        }

        pid_t pid = (pid_t)strtol(argv[1], NULL, 10);

        /* Check is the parsed PID correct. */
        if(!pid || errno == ERANGE)
        {
            fprintf(stderr, "Invalid pid!\n");
            fflush(stderr);
            return EXEC_FAILED;
        }

        /* Try to find job by pid. */
        job* jobs = find_job_pid(-pid);
        if(!jobs)
        {
            fprintf(stderr, "%d - no such job!\n", pid);
            fflush(stderr);
            return EXEC_FAILED;
        }

        /* Remove bg job from list. We no longer need this job. */
        remove_job(current_job->pgid);
        continue_job(jobs, 0);

        return EXEC_SUCCESS;
    }else if(!strcmp(name, "fg"))
    {
        int size = 0;
        int i = 1;

        /* Check args. */
        while(argv[i++])
            size++;

        if(size > 1)
        {
            fprintf(stderr, "%s: Too many args!\n", argv[1]);
            fflush(stderr);
            return EXEC_FAILED;
        }

        pid_t pid = (pid_t)strtol(argv[1], NULL, 10);

        /* Check is the parsed PID correct. */
        if(!pid || errno == ERANGE)
        {
            fprintf(stderr, "Invalid pid!\n");
            fflush(stderr);
            return EXEC_FAILED;
        }

        /* Try to find job by pid. */
        job* jobs = find_job_pid(-pid);
        if(!jobs)
        {
            fprintf(stderr, "%d - no such job!\n", pid);
            fflush(stderr);
            return EXEC_FAILED;
        }

        /* Remove fg job from list. We no longer need this job. */
        remove_job(current_job->pgid);
        continue_job(jobs, 1);

        return EXEC_SUCCESS;
    }else
        return NOT_INNER_COMMAND;
}