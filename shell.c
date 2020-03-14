#include <stdlib.h>
#include <termio.h>
#include <signal.h>
#include "shell.h"

int get_invite();
void set_signal_handler();
void set_bkgr_sig_handl();

char *infile, *outfile, *appfile;
struct command cmds[MAXCMDS];
char bkgrnd;
char hostname[HOST_NAME_MAX];
char username[LOGIN_NAME_MAX];
char prompt[HOST_NAME_MAX + LOGIN_NAME_MAX + MAX_DIRECTORY_SIZE + 5];
struct termios term_strct;

int main(int argc, char *argv[])
{
    register int i;
    char line[1024];
    char varline[4096];
    char dir[MAX_DIRECTORY_SIZE];
    int ncmds;
    int inner_cmd_stat = 0;

    /* SAVE TERMINAL ATTRIBUTES */
    tcgetattr(0, &term_strct);

    /* PLACE sigset CODE HERE */
    set_signal_handler();

    /* INIT HOME LOCATION IN SHELL */
    init_home(argv[0]);

    /* INIT NAME AND HOSTNAME */
    gethostname(hostname, HOST_NAME_MAX);
    getlogin_r(username, LOGIN_NAME_MAX);

    get_dir_prompt(dir);
    sprintf(prompt, "%s@%s:%s$ ", username, hostname, dir);

    while (get_invite() && prompt_line(line, sizeof(line)) > 0)
    {
        if ((ncmds = parse_line(line, varline)) <= 0)
            continue;

        #ifdef DEBUG
        {
            int i, j;
            for (i = 0; i < ncmds; i++)
            {
                for (j = 0; cmds[i].cmdargs[j] != (char *) NULL; j++)
                    fprintf(stderr, "cmd[%d].cmdargs[%d] = %s\n", i, j, cmds[i].cmdargs[j]);
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
                    set_bkgr_sig_handl();
                if(!i && infile)
                {
                    int input = open(infile, O_RDONLY);
                    if(input != -1)
                    {
                        if(dup2(input, STDIN_FILENO) == -1)
                        {
                            perror("Couldn't redirect input\n");
                            close(input);
                            return EXIT_FAILURE;
                        }
                        close(input);
                    }
                    else
                    {
                        perror("Couldn't open input file\n");
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
                            perror("Couldn't redirect output\n");
                            close(output);
                            return EXIT_FAILURE;
                        }
                        close(output);
                    }
                    else
                    {
                        perror("Couldn't open output file\n");
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
                    wait_process(pid);
                else
                    printf("Background pid: %ld\n", (long)pid);
            }
            else
                perror("Couldn't create process\n");
        }
    }
    return EXIT_SUCCESS;
}

int get_invite()
{
    write(STDOUT_FILENO, prompt, strlen(prompt));
    fflush(stdout);

    return 1;
}

/* PLACE sigset CODE HERE */
void close_zombies(int signum)
{
    waitpid(-1, NULL, WNOHANG);
}

/* USED FOR CTRL-Z COMMAND(PROCESS GO TO BACKGROUND)*/
void goto_back(int signum)
{
    
}

/* ALLOW SHELL TO GET SIGCHLD */
void set_signal_handler()
{
    sigset_t mask;

    sigfillset(&mask);
    sigdelset(&mask, SIGCHLD);

    struct sigaction act0 = {SIG_IGN,0};
    sigaction(SIGINT, &act0, NULL);
    sigaction(SIGTSTP, &act0, NULL);

    struct sigaction act1 = {close_zombies,0};
    sigaction(SIGCHLD, &act1, NULL);
}

/* USED FOR BACKGROUND PROCESSES */
void set_bkgr_sig_handl()
{
    struct sigaction act0 = {SIG_IGN,0};

    sigaction(SIGINT, &act0, NULL);
    sigaction(SIGQUIT, &act0, NULL);
    sigaction(SIGHUP, &act0, NULL);
}

int wait_process(pid_t pid)
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

        tcsetattr(0, TCSANOW, &term_strct);
    }
    return status;
}