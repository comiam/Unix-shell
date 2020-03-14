#include <stdio.h>
#include <signal.h>
#include "cmds.h"

int exec_inner(struct command *cmd)
{
    if(!cmd)
        return EXEC_FAILED;

    if(!strcmp((*cmd).cmdargs[0], "exit"))
        return MAY_EXIT;
    else if(!strcmp((*cmd).cmdargs[0], "cd"))
    {
        int size = 0;
        int i = 1;
        while((*cmd).cmdargs[i++])
            size++;

        if(size > 1)
        {
            fprintf(stderr, "%s: Too many args!\n", (*cmd).cmdargs[1]);
            fflush(stderr);
            return EXEC_FAILED;
        }

        int status = set_directory((*cmd).cmdargs[1]);

        switch(status)
        {
            case DIR_EXIST:
                return EXEC_SUCCESS;
            case DIR_NOT_EXIST:
                fprintf(stderr, "%s: No such file or directory!\n", (*cmd).cmdargs[1]);
                fflush(stderr);
                return EXEC_FAILED;
            case CANT_OPEN_DIR:
                fprintf(stderr, "%s: Can't open dir!\n", (*cmd).cmdargs[1]);
                fflush(stderr);
                return EXEC_FAILED;
            case DIR_IS_FILE:
                fprintf(stderr, "%s: Not a directory!\n", (*cmd).cmdargs[1]);
                fflush(stderr);
                return EXEC_FAILED;
            default:
                return EXEC_FAILED;
        }
    }else if(!strcmp((*cmd).cmdargs[0], "bg"))
    {
        int size = 0;
        int i = 1;
        while((*cmd).cmdargs[i++])
            size++;

        if(size > 1)
        {
            fprintf(stderr, "%s: Too many args!\n", (*cmd).cmdargs[1]);
            fflush(stderr);
            return EXEC_FAILED;
        }
        int res;
        if(!(res = kill((pid_t)atoi((*cmd).cmdargs[1]), SIGCONT)))
            return EXEC_SUCCESS;
        else
        {
            switch(errno)
            {
                case ESRCH:
                    fprintf(stderr, "No such process!");
                    fflush(stderr);
                    break;
                case EPERM:
                    fprintf(stderr, "Permission denied!");
                    fflush(stderr);
                    break;
                default:
                    fprintf(stderr, "%d error!", res);
                    fflush(stderr);
                    break;
            }
            return EXEC_FAILED;
        }
    }else if(!strcmp((*cmd).cmdargs[0], "fg"))
    {
        int size = 0;
        int i = 1;
        while((*cmd).cmdargs[i++])
            size++;

        if(size > 1)
        {
            fprintf(stderr, "%s: Too many args!\n", (*cmd).cmdargs[1]);
            fflush(stderr);
            return EXEC_FAILED;
        }

        if(wait_process((pid_t)atoi((*cmd).cmdargs[1])) == -1)
            switch(errno)
            {
                case ECHILD:
                    fprintf(stderr, "This process not exist!");
                    fflush(stderr);
                    break;
                case EPERM:
                    fprintf(stderr, "Permission denied!");
                    fflush(stderr);
                    break;
                default:
                    fprintf(stderr, "%d error!", errno);
                    fflush(stderr);
                    break;
            }

        return EXEC_SUCCESS;
    }else
        return NOT_INNER_COMMAND;
}