#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "shell.h"

/* Read line from input. Return count of read symbols. */
ssize_t prompt_line(char *line, int sizeline)
{
    ssize_t n = 0;

    while (1)
    {
        n += read(0, (line + n), (size_t) (sizeline - n));
        *(line + n) = '\0';
         /* Check to see if command line extends on to next line.
            If so, append next line to command line. */

        if (*(line + n - 2) == '\\' && *(line + n - 1) == '\n' )
        {
            *(line + n) = ' ';
            *(line + n - 1) = '\n';
            *(line + n - 2) = ' ';
            continue;   /* Read next line. */
        }
        return (n);      /* All done. */
    }
}

/* Skip all spaces from begin of string s. */
static char *blank_skip(register char *s);

/* Parse input line. Return -1, if was syntax error.
   Or 1, if parsing was successful.
   All args must be non null. */
int parse_line(char *line, char *varline)
{
    assert(line != NULL);
    assert(varline != NULL);

    int nargs, ncmds;
    register char *s;
    register char *envword;
    char aflg = 0;
    int rval;
    register int i;
    static char delim[] = " \t|&<>;$\n\"\\";

    bkgrnd = nargs = ncmds = rval = 0;
    s = line;
    infile = outfile = appfile = (char *) NULL;
    cmds[0].cmdargs[0] = (char *) NULL;
    for (i = 0; i < MAXCMDS; i++)
        cmds[i].cmdflag = 0;

    while (*s)
    {
        s = blank_skip(s);
        if (!*s)
            break;

        /* Handle <, >, |, &, $, " and ; */
        switch (*s)
        {
            case '&':
                ++bkgrnd;
                *s++ = '\0';
                break;
            case '>':
                if (*(s + 1) == '>')
                {
                    ++aflg;
                    *s++ = '\0';
                }
                *s++ = '\0';
                s = blank_skip(s);
                if (!*s)
                {
                    fprintf(stderr, "Syntax error!\n");
                    return (-1);
                }

                if (aflg)
                    appfile = s;
                else
                    outfile = s;
                s = strpbrk(s, delim);
                if (isspace(*s))
                    *s++ = '\0';
                break;
            case '<':
                *s++ = '\0';
                s = blank_skip(s);
                if (!*s)
                {
                    fprintf(stderr, "Syntax error!\n");
                    return (-1);
                }
                infile = s;
                s = strpbrk(s, delim);
                if (isspace(*s))
                    *s++ = '\0';
                break;
            case '|':
                if (nargs == 0)
                {
                    fprintf(stderr, "Syntax error!\n");
                    return (-1);
                }
                cmds[ncmds++].cmdflag |= OUTPIP;
                cmds[ncmds].cmdflag |= INPIP;
                *s++ = '\0';
                nargs = 0;
                break;
            case ';':
                *s++ = '\0';
                ++ncmds;
                nargs = 0;
                break;
            case '$':
                /* Replace $($) to value of variable. */
                if (*(s + 1) == '$')
                {
                    /* If on this position $$. We must get PID of shell process. */
                    *s++ = '\0';
                    *s++ = '\0';
                    s = strpbrk(s, delim);
                    if (isspace(*s))
                        *s++ = '\0';

                    cmds[ncmds].cmdargs[nargs++] = varline;
                    cmds[ncmds].cmdargs[nargs] = (char *) NULL;

                    char str[10];
                    sprintf(str, "%d", getpid());

                    int index = 0;
                    while(!isspace(str[index]) && str[index] != '\0')
                        *varline++ = str[index++];

                    *varline++ = '\0';
                } else
                {
                    /* If on this position $<VAR_NAME>. We must get value of VAR_NAME. */
                    *s++ = '\0';

                    envword = s;
                    while(!isspace(*s) && *s != '\0') ++s;

                    s = strpbrk(s, delim);
                    if (isspace(*s))
                        *s++ = '\0';

                    cmds[ncmds].cmdargs[nargs++] = getenv(envword);
                    cmds[ncmds].cmdargs[nargs] = (char *) NULL;
                }
                break;
            case '%':
                /* Symbol of job list index. */
                *s++ = '\0';
                envword = s;
                while(!isspace(*s) && *s != '\0') ++s;

                s = strpbrk(s, delim);
                if (isspace(*s))
                    *s++ = '\0';

                cmds[ncmds].cmdargs[nargs++] = varline;
                cmds[ncmds].cmdargs[nargs] = (char *) NULL;

                char str[10];
                int a = (pid_t)strtol(envword, NULL, 10);

                /* Check is the parsed PID correct. */
                if(errno == ERANGE)
                {
                    fprintf(stderr, "%%%s: Invalid job index!\n", envword);
                    fflush(stderr);
                    return EXEC_FAILED;
                }

                /* Try to find job by parsed index. */
                if (!find_job_jid(a))
                {
                    fprintf(stderr, "%%%s: no such job!\n", envword);
                    return (-1);
                }

                sprintf(str, "%d", -find_job_jid(a)->pgid);

                int index = 0;
                while(!isspace(str[index]) && str[index] != '\0')
                    *varline++ = str[index++];

                *varline++ = '\0';
                break;
            case '\"':
                *s++ = '\0';

                /* Try to find second bracket. */
                char second_bracket[1] = "\"";
                char *entry = s;

                while((entry = strpbrk(entry, second_bracket)) && entry > line && *(entry - 1) == '\\')
                    if(entry > line && *(entry - 1) == '\\')
                        strncpy(entry - 1, entry, strlen(entry));

                if (!entry)
                {
                    fprintf(stderr, "Syntax error!\n");
                    return (-1);
                } else
                    *entry++ = '\0';

                cmds[ncmds].cmdargs[nargs++] = s;
                cmds[ncmds].cmdargs[nargs] = (char *) NULL;

                s = strpbrk(entry, delim);
                break;
            default:
                /* If we get a word, not a symbol. */
                if (nargs == 0)
                    rval = ncmds + 1;
                cmds[ncmds].cmdargs[nargs++] = s;
                cmds[ncmds].cmdargs[nargs] = (char *) NULL;

                s = strpbrk(s, delim);

                if(!s)
                    return (-1);

                if (isspace(*s))
                    *s++ = '\0';
                break;
        }
    }
    if ((cmds[ncmds - 1].cmdflag & OUTPIP) && nargs == 0)
    {
        fprintf(stderr, "Syntax error!\n");
        return (-1);
    }

    return (rval);
}

/* Skip all spaces from begin of string s. */
static char *blank_skip(register char *s)
{
    while (isspace(*s) && *s) ++s;
    return (s);
}
