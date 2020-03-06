#ifndef UNIX_SHELL_PROMPTLINE_H
#define UNIX_SHELL_PROMPTLINE_H

#include <stdio.h>
#include <ctype.h>
#include <string.h>

int parse_line(char *, char *);
int prompt_line(char *, int);

#endif