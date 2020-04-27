#include <stdio.h>
#include "cmds.h"
#include "jobs.h"
#include "shell.h"

int command_is_inner(const char* name)
{
    return !strcmp(name, "cd") || !strcmp(name, "exit") || !strcmp(name, "jobs") || !strcmp(name, "bg") || !strcmp(name, "fg");
}

int exec_inner(const char *name, const char *argv[])
{
    if(!name || !argv)
        return EXEC_FAILED;

    if(!strcmp(name, "exit"))
        return MAY_EXIT;
    else if(!strcmp(name, "cd"))
    {
        int size = 0;
        int i = 1;
        while(argv[i++])
            size++;

        if(size > 1)
        {
            fprintf(stderr, "%s: Too many args!\n", argv[1]);
            fflush(stderr);
            return EXEC_FAILED;
        }

        int status = set_directory(argv[1]);

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
        do_job_notification(1);

        return EXEC_SUCCESS;
    }else if(!strcmp(name, "bg"))
    {
        int size = 0;
        int i = 1;
        while(argv[i++])
            size++;

        if(size > 1)
        {
            fprintf(stderr, "%s: Too many args!\n", argv[1]);
            fflush(stderr);
            return EXEC_FAILED;
        }

        pid_t pid = (pid_t)strtol(argv[1], NULL, 10);

        if(!pid || errno == ERANGE)
        {
            fprintf(stderr, "Invalid pid!\n");
            fflush(stderr);
            return EXEC_FAILED;
        }

        job* jobs = find_job_pid(-pid);
        if(!jobs)
        {
            fprintf(stderr, "%d - no such job!\n", pid);
            fflush(stderr);
            return EXEC_FAILED;
        }

        remove_job(current_job->pgid);
        continue_job(jobs, 0);

        return EXEC_SUCCESS;
    }else if(!strcmp(name, "fg"))
    {
        int size = 0;
        int i = 1;
        while(argv[i++])
            size++;

        if(size > 1)
        {
            fprintf(stderr, "%s: Too many args!\n", argv[1]);
            fflush(stderr);
            return EXEC_FAILED;
        }

        pid_t pid = (pid_t)strtol(argv[1], NULL, 10);

        if(!pid || errno == ERANGE)
        {
            fprintf(stderr, "Invalid pid!\n");
            fflush(stderr);
            return EXEC_FAILED;
        }

        job* jobs = find_job_pid(-pid);
        if(!jobs)
        {
            fprintf(stderr, "%d - no such job!\n", pid);
            fflush(stderr);
            return EXEC_FAILED;
        }

        remove_job(current_job->pgid);
        continue_job(jobs, 1);

        return EXEC_SUCCESS;
    }else
        return NOT_INNER_COMMAND;
}