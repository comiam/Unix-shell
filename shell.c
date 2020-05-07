#include "shell.h"

/* Initialize shell process. */
void init_shell(char *argv[]);

/* Prints invite_string to STDOUT.
   Return 1, if print was successful.
   Or 0, if print failed. After fail shell will be closed. */
int get_invite();

/* Launch and wait, if necessary, new job. */
void launch_job(int foreground);

/* Used for executing command in forked process. */
void launch_process(process *p, pid_t pgid, int infile_local, int outfile_local, int errfile_local, int foreground);

char hostname[HOST_NAME_MAX];  /* name of host */
char username[LOGIN_NAME_MAX]; /* user name */
char invite_string[HOST_NAME_MAX + LOGIN_NAME_MAX + MAX_DIRECTORY_SIZE + 5]; /* invite string */
char line[READ_LINE_SIZE]; /* line reading buffer */
char varline[READ_LINE_SIZE * 4]; /* buffer line for values of variables from parsing input string */
char dir[MAX_DIRECTORY_SIZE]; /* directory string buffer for printing to invite_string */

int main(int argc, char *argv[])
{
    /* INIT SHELL */
    init_shell(argv);

    int ncmds;

    /* Main cycle. First, shell makes an invitation, then waits for the line to be entered. */
    while (get_invite() && prompt_line(line, sizeof(line)) > 0)
    {
        /* End of waiting for input from the terminal. */
        invite_mode = 0;

        /* Create new job. */
        if (!(current_job = create_new_job(0, line)))
        {
            fprintf(stderr, "Can't create job!");
            fflush(stderr);
            shell_exit(EXIT_FAILURE);
        }

        /* Parse line to cmds[] array. */
        if ((ncmds = parse_line(line, varline)) <= 0)
            continue;

        /* Parse cmds[] data to current_job. */
        fill_job(&current_job, ncmds);

        /* If parsing was failed. */
        if(!current_job)
            continue;

        /* Add new current_job to job list. */
        add_job(current_job);

        /* Run current_job with set bkgrnd flag.
           The bkgrnd value was obtained when parsing the input line. */
        launch_job(!bkgrnd);

        /* Check state of job after waiting. */
        if(!bkgrnd && job_is_completed(current_job))
        {
            /* Notify all completed or stopped jobs after executing current_job. */
            do_job_notification(0);

            /* We no longer need this job. */
            remove_job(current_job->pgid);
        }else if(!bkgrnd && job_is_stopped(current_job))
        {
            /* Notify if current_job is stopped. */
            current_job->notified = 1;
            format_job_info(current_job, "stopped");
            fprintf(stdout, "\n");
            fflush(stdout);
        }else if(bkgrnd)
        {
            /* Show index of new background task. */
            current_job->notified = 1;
            format_job_info(current_job, NULL);
            fprintf(stdout, "\n");
            fflush(stdout);
        }
    }
}

/* Prints invite_string to STDOUT.
   Return 1, if print was successful.
   Or 0, if print failed. After fail shell will be closed. */
int get_invite()
{
    /* Begin to wait input.
       Now SIGCHLD may print notifications of stopped or terminated processes. */
    invite_mode = 1;
    int flag = write(STDOUT_FILENO, invite_string, strlen(invite_string)) != -1;

    return flag && (fflush(stdout) != EOF);
}

/* Initialize shell process. */
void init_shell(char *argv[])
{
    shell_terminal = STDIN_FILENO;

    /* See if we are running interactively. */
    int shell_is_interactive = isatty(shell_terminal);
    if (shell_is_interactive)
    {
        /* Loop until we are in the foreground. */
        //while(tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
        //     kill(-shell_pgid, SIGTTIN);

        /* Ignore interactive and job-control signals. */
        set_signal_handler(SIGINT, SIG_IGN);
        set_signal_handler(SIGQUIT, SIG_IGN);
        set_signal_handler(SIGTSTP, SIG_IGN);
        set_signal_handler(SIGTTIN, SIG_IGN);
        set_signal_handler(SIGTTOU, SIG_IGN);
        set_signal_handler(SIGTERM, SIG_IGN);
        set_signal_handler(SIGCHLD, notify_child);

        /* Put ourselves in our own process group. */
        shell_pgid = getpid();
        /*if (setpgid(shell_pgid, shell_pgid) < 0)
        {
            perror("Couldn't put the shell in its own process group");
            shell_exit(EXIT_FAILURE);
        }*/

        /* Grab control of the terminal. */
        tcsetpgrp(shell_terminal, shell_pgid);
        tcgetattr(shell_terminal, &shell_tmodes);

        /* Init home location of shell. */
        init_home(argv[0]);

        /* Init invite_string. */
        gethostname(hostname, HOST_NAME_MAX);
        getlogin_r(username, LOGIN_NAME_MAX);
        get_dir_prompt(dir);
        sprintf(invite_string, "%s@%s:%s$ ", username, hostname, dir);
    } else
    {
        fprintf(stderr, "Can't run shell: shell not in foreground!");
        fflush(stderr);
        shell_exit(EXIT_FAILURE);
    }
}

/* Used for executing command in forked process. */
void launch_process(process *p, pid_t pgid, int infile_local, int outfile_local, int errfile_local, int foreground)
{
    /* Put the process into the process group and give the process group
       the terminal, if appropriate.
       This has to be done both by the shell and in the individual
       child processes because of potential race conditions. */
    pid_t pid = getpid();
    if (pgid == 0)
        pgid = pid;
    setpgid(pid, pgid);

    if (foreground)
        tcsetpgrp(shell_terminal, pgid);

    /* Set the handling for job control signals back to the default. */
    set_signal_handler(SIGINT, SIG_DFL);
    set_signal_handler(SIGQUIT, SIG_DFL);
    set_signal_handler(SIGTSTP, SIG_DFL);
    set_signal_handler(SIGTTIN, SIG_DFL);
    set_signal_handler(SIGTTOU, SIG_DFL);
    set_signal_handler(SIGCHLD, SIG_DFL);
    set_signal_handler(SIGPIPE, SIG_DFL);
    set_signal_handler(SIGTERM, SIG_DFL);

    /* Set the standard input/output channels of the new process. */
    if (infile_local != STDIN_FILENO)
    {
        dup2(infile_local, STDIN_FILENO);
        close(infile_local);
    }
    if (outfile_local != STDOUT_FILENO)
    {
        dup2(outfile_local, STDOUT_FILENO);
        close(outfile_local);
    }
    if (errfile_local != STDERR_FILENO)
    {
        dup2(errfile_local, STDERR_FILENO);
        close(errfile_local);
    }

    /* Save only argv for child. So copy it. */
    unsigned size = 0;
    while (p->argv[size] != NULL)
        size++;

    char** argv = malloc((size + 1) * sizeof(char*));

    if(!argv)
    {
        perror("child malloc");
        clear_job_list(0);
        exit(EXIT_FAILURE);
    }

    for(unsigned j = 0; j < size; j++)
    {
        argv[j] = strdup(p->argv[j]);
        if(errno == ENOMEM)
        {
            perror("child malloc");
            for (unsigned k = 0; k < j; ++k)
                free(argv[k]);
            free(argv);
            clear_job_list(0);
            exit(EXIT_FAILURE);
        }
    }

    argv[size] = NULL;

    /* Clear extra memory in child process. */
    clear_job_list(0);

    /* Exec the new process. Make sure we exit. */
    execvp(argv[0], argv);
    fprintf(stderr, "\nexecvp: %s: ", argv[0]);
    perror("");
    exit(EXIT_FAILURE);
}

/* Launch and wait, if necessary, new job. */
void launch_job(int foreground)
{
    process *p, *p_next = NULL;
    pid_t pid;
    int mypipe[2], infile_local, outfile_local;

    /* Flag for checking the job for inner commands only. */
    int exec_only_inner = 1;

    infile_local = current_job->stdin_file;

    for (p = current_job->first_process;p;)
    {
        p_next = p->next;

        /* Set up pipes, if necessary. */
        if (p->next)
        {
            current_job->have_pipe = 1;
            if (pipe(mypipe) < 0)
            {
                perror("pipe");
                shell_exit(EXIT_FAILURE);
            }
            outfile_local = mypipe[1];
        } else
            outfile_local = current_job->stdout_file;

        /* Check for the internal implementation of the command. */
        if(command_is_inner(p->argv[0]))
        {
            int inner_cmd_stat;

            if ((inner_cmd_stat = exec_inner(p->argv[0], (const char **) p->argv)) == MAY_EXIT)
                /* We get MAY_EXIT code, so we can exit from shell. */
                shell_exit(EXIT_SUCCESS);
            else if (inner_cmd_stat != NOT_INNER_COMMAND)
            {
                /* Update invite_string, because of we can launch cd command. */
                get_dir_prompt(dir);
                sprintf(invite_string, "%s@%s:%s$ ", username, hostname, dir);

                /* Set flags that this inner process completed. */
                p->stopped = 0;
                p->completed = 1;
            }
        } else
        {
            /* We have non-internal command, so set exec_only_inner to 0. */
            exec_only_inner = 0;

            /* Fork the child processes. */
            pid = fork();
            if (pid == 0)
                /* This is the child process. */
                launch_process(p, current_job->pgid, infile_local, outfile_local, current_job->stderr_file, foreground);
            else if (pid < 0)
            {
                /* The fork failed. */
                perror("fork");
                shell_exit(EXIT_FAILURE);
            } else
            {
                /* This is the parent process. */
                p->pid = pid;
                if (!current_job->pgid)
                    current_job->pgid = pid;
                setpgid(pid, current_job->pgid);
            }
        }

        /* Clean up after pipes. */
        if (infile_local != current_job->stdin_file)
            close(infile_local);
        if (outfile_local != current_job->stdout_file)
            close(outfile_local);
        infile_local = mypipe[0];

        p = p_next;
    }

    /* Inner commands don't run in forked processes, so we don't have to wait for them. */
    if(!exec_only_inner)
    {
        if(foreground)
            put_job_in_foreground(current_job, 0);
        else
            put_job_in_background(current_job, 0);
    }
}

/* Exit from shell. */
void shell_exit(int stat)
{
    clear_job_list(1);

    exit(stat);
}
