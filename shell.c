#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>
#include "promptline.h"

char *infile, *outfile, *appfile;
struct command cmds[MAXCMDS];
char bkgrnd;

int main(int argc, char *argv[])
{
    register int i;
    char line[1024];      /*  allow large command lines  */
    char varline[4096];   /*  used for replaces in command line */
    int ncmds;
    char prompt[50];      /* shell prompt */

    /* PLACE SIGNAL CODE HERE */

    sprintf(prompt, "[%s] ", argv[0]);

    while (promptline(prompt, line, sizeof(line)) > 0)
    {    /* until eof */
        if ((ncmds = parseline(line, varline)) <= 0)
            continue;   /* read next line */
        #ifdef DEBUG
        {
           int i, j;     for (i = 0; i < ncmds; i++) {
               for (j = 0; cmds[i].cmdargs[j] != (char *) NULL; j++)
                   fprintf(stderr, "cmd[%d].cmdargs[%d] = %s\n",
                    i, j, cmds[i].cmdargs[j]);
                fprintf(stderr, "cmds[%d].cmdflag = %o\n", i, cmds[i].cmdflag);
            }
        }
        #endif

        for (i = 0; i < ncmds; i++)
        {
            /*  FORK AND EXECUTE  */

            if(!cmds[i].cmdargs[1] && !strcmp(cmds[i].cmdargs[0], "exit"))
                return EXIT_SUCCESS;

            pid_t pid = fork();
            if(!pid)
            {
                if(!i && infile)
                {
                    int input = open(infile, O_RDONLY);
                    if(input != -1)
                    {
                        if(dup2(input, STDIN_FILENO) == -1)
                        {
                            perror("Couldn't redirect input");
                            close(input);
                            return EXIT_FAILURE;
                        }
                        close(input);
                    }
                    else
                    {
                        perror("Couldn't open input file");
                        return EXIT_FAILURE;
                    }
                }
                if(i == (ncmds - 1) && (outfile || appfile))
                {
                    int output;
                    if(outfile)
                        output = open(outfile, O_WRONLY | O_TRUNC  | O_CREAT, (mode_t)0644);//  READ-WRITE-NOT_EXECUTE
                    else
                        output = open(appfile, O_WRONLY | O_APPEND | O_CREAT, (mode_t)0644);

                    if(output != -1)
                    {
                        if(dup2(output, STDOUT_FILENO) == -1)// SET STDOUT_FILENO = OUTPUT
                         {
                            perror("Couldn't redirect output");
                            close(output);
                            return EXIT_FAILURE;
                        }
                        close(output);
                    }
                    else
                    {
                        perror("Couldn't open output file");
                        return EXIT_FAILURE;
                    }
                }
                execvp(cmds[i].cmdargs[0], cmds[i].cmdargs);
                perror("Couldn't execute command");
                return EXIT_FAILURE;
            }
            else if(pid != -1)
            {
                if(!bkgrnd)
                {
                    if(waitpid(pid, NULL, 0) == -1)
                        perror("Couldn't wait for child process termination");
                }
                else
                    printf("Background pid: %ld\n", (long)pid);
            }
            else
                perror("Couldn't create process");
        }

    }  /* close while */
    return EXIT_SUCCESS;
}
/* PLACE SIGNAL CODE HERE */