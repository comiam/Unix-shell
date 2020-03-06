#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "shell.h"

int prompt_line( char *line, int sizeline)
{
    int n = 0;

    while (1)
    {
        n += read(0, (line + n), sizeline - n);
        *(line + n) = '\0';
         /*
          *  check to see if command line extends onto
          *  next line.  If so, append next line to command line
          */

        if (*(line + n - 2) == '\\' && *(line + n - 1) == '\n')
        {
            *(line + n) = ' ';
            *(line + n - 1) = ' ';
            *(line + n - 2) = ' ';
            continue;   /*  read next line  */
        }
        return (n);      /* all done */
    }
}

static char *blank_skip(register char *);

int parse_line(char *line, char *varline)
{
    int nargs, ncmds;
    register char *s;
    register char *envword;
    char aflg = 0;
    int rval;
    register int i;
    static char delim[] = " \t|&<>;$\n";

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

        /*  handle <, >, |, &, $ and ;  */
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
                    fprintf(stderr, "syntax error\n");
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
                    fprintf(stderr, "syntax error\n");
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
                    fprintf(stderr, "syntax error\n");
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
                if (*(s + 1) == '$')
                {
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
            default:
                if (nargs == 0)
                    rval = ncmds + 1;
                cmds[ncmds].cmdargs[nargs++] = s;
                cmds[ncmds].cmdargs[nargs] = (char *) NULL;
                s = strpbrk(s, delim);
                if (isspace(*s))
                    *s++ = '\0';
                break;
        }
    }
    if (cmds[ncmds - 1].cmdflag & OUTPIP)
    {
        if (nargs == 0)
        {
            fprintf(stderr, "syntax error\n");
            return (-1);
        }
    }

    return (rval);
}

static char *blank_skip(register char *s)
{
    while (isspace(*s) && *s) ++s;
    return (s);
}