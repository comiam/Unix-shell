#include <sys/types.h>
#include <stdlib.h>
#include <termio.h>
#include "shell.h"

char *infile, *outfile, *appfile;
struct command cmds[MAXCMDS];
char bkgrnd;
char hostname[HOST_NAME_MAX];
char username[LOGIN_NAME_MAX];
struct termios __termios_p;

int main(int argc, char *argv[])
{
    register int i;
    char line[1024];
    char varline[4096];
    char dir[MAX_DIRECTORY_SIZE];
    int ncmds;
    char prompt[HOST_NAME_MAX + LOGIN_NAME_MAX + MAX_DIRECTORY_SIZE + 5];
    int inner_cmd_stat = 0;

    tcgetattr(0, &__termios_p);

    /* PLACE SIGNAL CODE HERE */
    signal(SIGINT, SIG_IGN);

    init_home(argv[0]);
    gethostname(hostname, HOST_NAME_MAX);
    getlogin_r(username, LOGIN_NAME_MAX);

    get_dir_prompt(dir);
    sprintf(prompt, "%s@%s:%s$ ", username, hostname, dir);

    write(1, prompt, strlen(prompt));
    char start = 1;
    while (prompt_line(line, sizeof(line)) > 0)
    {
        if(!start)
            write(1, prompt, strlen(prompt));
        else
            start = 0;

        if ((ncmds = parse_line(line, varline)) <= 0)
            continue;

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
            if((inner_cmd_stat = exec_inner(&cmds[i])) == MAY_EXIT)
                return EXIT_SUCCESS;
            else if(inner_cmd_stat != NOT_INNER_COMMAND)
            {
                get_dir_prompt(dir);
                sprintf(prompt, "%s@%s:%s$ ", username, hostname, dir);
                continue;
            }

            pid_t pid = fork();
            if(!pid)
            {
                if(bkgrnd)
                {
                    signal(SIGINT, SIG_IGN);
                    signal(SIGQUIT, SIG_IGN);
                    signal(SIGHUP, SIG_IGN);
                }
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
                    int status;

                    if(waitpid(pid, &status, 0) == -1)
                        perror("Couldn't wait for child process termination\n");

                    if(!WIFEXITED(status))
                    {
                        if(WIFSIGNALED(status))
                            fprintf(stderr, "Process %u stopped due to an unprocessed signal %u\n", pid, WTERMSIG(status));
                        else
                            fprintf(stderr, "The process %u failed with a signal %u\n", pid, WEXITSTATUS(status));

                        tcsetattr(0, TCSANOW, &__termios_p);
                    }
                }
                else
                    printf("Background pid: %ld\n", (long)pid);
            }
            else
                perror("Couldn't create process");
        }
    }
    return EXIT_SUCCESS;
}
/* PLACE SIGNAL CODE HERE */