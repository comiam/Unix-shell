#include <stdio.h>
#include "cmds.h"
#include "jobs.h"

int command_is_inner(const char* name)
{
    return !strcmp(name, "cd") || !strcmp(name, "exit") || !strcmp(name, "jobs") /*|| !strcmp(name, "bg") || !strcmp(name, "fg") */;
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
    }else /*if(!strcmp(name, "bg"))
    {
        int size = 0;
        int i = 1;
        while(argv[i++])
            size++;

        if(size > 1)
        {
            fprintf(stderr_file, "%s: Too many args!\n", argv[1]);
            fflush(stderr_file);
            return EXEC_FAILED;
        }

        if(!(pid_t)strtol(argv[1], NULL, 10) || errno == ERANGE)
        {
            fprintf(stderr_file, "Invalid pid!");
            fflush(stderr_file);
            return EXEC_FAILED;
        }

        if(kill((pid_t)strtol(argv[1], NULL, 10), SIGCONT))
            return EXEC_SUCCESS;
        else
        {
            perror("Cant execute bg!");
            return EXEC_FAILED;
        }
    }else if(!strcmp(name, "fg"))
    {
        int size = 0;
        int i = 1;
        while(argv[i++])
            size++;

        if(size > 1)
        {
            fprintf(stderr_file, "%s: Too many args!\n", argv[1]);
            fflush(stderr_file);
            return EXEC_FAILED;
        }

        if(!(pid_t)strtol(argv[1], NULL, 10) || errno == ERANGE)
        {
            fprintf(stderr_file, "Invalid pid!");
            fflush(stderr_file);
            return EXEC_FAILED;
        }

        kill((pid_t)strtol(argv[1], NULL, 10), SIGUSR1);

        if(wait_process((pid_t)strtol(argv[1], NULL, 10)) == -1)
            perror("Cant execute fg!");

        return EXEC_SUCCESS;
    }else*/
        return NOT_INNER_COMMAND;
}