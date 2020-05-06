#ifndef UNIX_SHELL_PROMPTLINE_H
#define UNIX_SHELL_PROMPTLINE_H

#include <stdio.h>
#include <ctype.h>
#include <string.h>

/* Parse input line. Return -1, if was syntax error.
   Or 1, if parsing was successful.
   All args must be non null. */
int parse_line(char *line, char *varline);

/* Read line from input. Return count of read symbols. */
ssize_t prompt_line(char *line, int sizeline);

#endif
