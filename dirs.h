#ifndef UNIX_SHELL_DIRS_H
#define UNIX_SHELL_DIRS_H

#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <sys/stat.h>

#define MAX_DIRECTORY_SIZE 256 /* maximal size of directory string for invite string */
#define CANT_OPEN_DIR     -257 /* special codes for set_directory */
#define DIR_NOT_EXIST     -258
#define DIR_EXIST         -259
#define DIR_IS_FILE       -260

/* Init home directory of shell.
   begin must be non null. */
void  init_home(char *begin);

/* Set current working directory to path, with validation of dir. */
int   set_directory(const char* dir);

/* Get directory string for invite string.
   dirstr must be non null. */
void  get_dir_prompt(char *dirstr);

#endif
