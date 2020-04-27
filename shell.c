#include "shell.h"

void init_shell(char *argv[]);
int get_invite();
void launch_job(int foreground);
void launch_process(process *p, pid_t pgid, int infile_local, int outfile_local, int errfile_local, int foreground);
void put_job_in_foreground(job *j, int cont);
void put_job_in_background(job *j, int cont);
void wait_for_job(job *j);

pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;

char hostname[HOST_NAME_MAX];
char username[LOGIN_NAME_MAX];
char prompt[HOST_NAME_MAX + LOGIN_NAME_MAX + MAX_DIRECTORY_SIZE + 5];
char line[1024];
char varline[4096];
char dir[MAX_DIRECTORY_SIZE];

int main(int argc, char *argv[])
{
    /* INIT SHELL */
    init_shell(argv);

    int ncmds;

    while (get_invite() && prompt_line(line, sizeof(line)) > 0)
    {
        invite_mode = 0;

        if (!(current_job = create_new_job(0, line)))
        {
            fprintf(stderr, "Can't create job!");
            fflush(stderr);
            exit(EXIT_FAILURE);
        }

        if ((ncmds = parse_line(line, varline)) <= 0)
            continue;

        fill_job(current_job, ncmds);
        add_job(current_job);

        launch_job(!bkgrnd);
        if(!bkgrnd && job_is_completed(current_job))
            remove_job(current_job->pgid);
        else if(!bkgrnd && job_is_stopped(current_job))
        {
            current_job->notified = 1;
            format_job_info(current_job, "stopped");
            fprintf(stdout, "\n");
            fflush(stdout);
        }
    }
}

int get_invite()
{
    invite_mode = 1;
    int flag = write(STDOUT_FILENO, prompt, strlen(prompt)) != -1;

    return flag && (fflush(stdout) != EOF);
}

void init_shell(char *argv[])
{
    shell_terminal = STDIN_FILENO;
    int shell_is_interactive = isatty(shell_terminal);

    if (shell_is_interactive)
    {
        /* Loop until we are in the foreground.  */
        //while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
        //    kill(-shell_pgid, SIGTTIN);

        /* Ignore interactive and job-control signals.  */
        set_signal_handler(SIGINT, SIG_IGN);
        set_signal_handler(SIGQUIT, SIG_IGN);
        set_signal_handler(SIGTSTP, SIG_IGN);
        set_signal_handler(SIGTTIN, SIG_IGN);
        set_signal_handler(SIGTTOU, SIG_IGN);
        set_signal_handler(SIGCHLD, notify_child);

        /* Put ourselves in our own process group.  */
        shell_pgid = getpid();
        /*if (setpgid(shell_pgid, shell_pgid) < 0)
        {
            perror("Couldn't put the shell in its own process group");
            exit(EXIT_FAILURE);
        }*/

        /* Grab control of the terminal.  */
        tcsetpgrp(shell_terminal, shell_pgid);
        tcgetattr(shell_terminal, &shell_tmodes);

        /* Init home location of shell */
        init_home(argv[0]);

        /* Init invite string */
        gethostname(hostname, HOST_NAME_MAX);
        getlogin_r(username, LOGIN_NAME_MAX);
        get_dir_prompt(dir);
        sprintf(prompt, "%s@%s:%s$ ", username, hostname, dir);
    } else
    {
        fprintf(stderr, "Can't run shell: shell not in foreground!");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }
}

void launch_process(process *p, pid_t pgid, int infile_local, int outfile_local, int errfile_local, int foreground)
{
    /* Put the process into the process group and give the process group
           the terminal, if appropriate.
           This has to be done both by the shell and in the individual
           child processes because of potential race conditions.  */
    pid_t pid = getpid();
    if (pgid == 0)
        pgid = pid;
    setpgid(pid, pgid);

    if (foreground)
        tcsetpgrp(shell_terminal, pgid);

    /* Set the handling for job control signals back to the default.  */
    set_signal_handler(SIGINT, SIG_DFL);
    set_signal_handler(SIGQUIT, SIG_DFL);
    set_signal_handler(SIGTSTP, SIG_DFL);
    set_signal_handler(SIGTTIN, SIG_DFL);
    set_signal_handler(SIGTTOU, SIG_DFL);
    set_signal_handler(SIGCHLD, SIG_DFL);

    /* Set the standard input/output channels of the new process.  */
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

    /* Exec the new process.  Make sure we exit.  */
    execvp(p->argv[0], p->argv);
    perror("execvp");
    exit(EXIT_FAILURE);
}

void launch_job(int foreground)
{
    process *p;
    pid_t pid;
    int mypipe[2], infile_local, outfile_local;
    int exec_only_inner = 1;

    infile_local = current_job->stdin_file;
    for (p = current_job->first_process; p; p = p->next)
    {
        /* Set up pipes, if necessary. */
        if (p->next)
        {
            if (pipe(mypipe) < 0)
            {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            outfile_local = mypipe[1];
        } else
            outfile_local = current_job->stdout_file;

        if(command_is_inner(p->argv[0]))
        {
            int inner_cmd_stat;

            if ((inner_cmd_stat = exec_inner(p->argv[0], (const char **) p->argv)) == MAY_EXIT)
                exit(EXIT_SUCCESS);
            else if (inner_cmd_stat != NOT_INNER_COMMAND)
            {
                get_dir_prompt(dir);
                sprintf(prompt, "%s@%s:%s$ ", username, hostname, dir);
                p->stopped = 0;
                p->completed = 1;
            }
        } else
        {
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
                exit(EXIT_FAILURE);
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
    }

    if(!exec_only_inner)
    {
        if(foreground)
            put_job_in_foreground(current_job, 0);
        else
            put_job_in_background(current_job, 0);
    }
}

/* Put job j in the foreground.  If cont is nonzero,
   restore the saved terminal modes and send the process group a
   SIGCONT signal to wake it up before we block.  */
void put_job_in_foreground(job *j, int cont)
{
    /* Put the job into the foreground.  */
    tcsetpgrp(shell_terminal, j->pgid);

    /* Send the job a continue signal, if necessary.  */
    if (cont)
    {
        tcsetattr(shell_terminal, TCSADRAIN, &j->tmodes);
        if (kill(-j->pgid, SIGCONT) < 0)
            perror("kill (SIGCONT)");
    }

    /* Wait for it to report.  */
    wait_for_job(j);

    /* Put the shell back in the foreground.  */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Restore the shell's terminal modes.  */
    tcgetattr(shell_terminal, &j->tmodes);
    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
}

/* Put a job in the background.  If the cont argument is true, send
   the process group a SIGCONT signal to wake it up.  */
void put_job_in_background(job *j, int cont)
{
    /* Send the job a continue signal, if necessary.  */
    if (cont)
        if (kill(-j->pgid, SIGCONT) < 0)
            perror("kill (SIGCONT)");

    current_job->notified = 1;
    format_job_info(j, NULL);
    fprintf(stdout, "\n");
    fflush(stdout);
}

/* Check for processes that have status information available,
   blocking until all processes in the given job have reported.  */
void wait_for_job(job *j)
{
    int status;
    pid_t pid;

    if(!get_job_list_head() || job_list_is_inner())
        return;

    do
        pid = waitpid(WAIT_ANY, &status, WUNTRACED);
    while (!mark_process_status(pid, status) && !job_is_stopped(j) && !job_is_completed(j));
}