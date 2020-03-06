#ifndef UNIX_SHELL_DIRECTORIES_H
#define UNIX_SHELL_DIRECTORIES_H

#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/stat.h>

#define MAX_DIRECTORY_SIZE 256
#define CANT_OPEN_DIR     -257
#define DIR_NOT_EXIST     -258
#define DIR_EXIST         -259
#define DIR_IS_FILE       -260

void  init_home(char *begin);
int   set_directory(char* dir);
void  get_dir_prompt(char *dirstr);

#endif
