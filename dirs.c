#include <stdio.h>
#include <assert.h>
#include "dirs.h"

static char buffer[PATH_MAX]; /* buffer for operations in dirs.c */
static char *tmp_dir = NULL;  /* saved string of current directory */
static char *home_dir = NULL; /* initial home directory for shell */
static int found_home_var = 0; /* used for free home_dir at exit */

/* Concat strings.
   Strings must be non null. */
static char *concat_str(const char *s0, const char *s1);

/* Check path is valid directory. */
static int  is_dir(const char *path);

/* Replace first word1 in str to word2. */
static void replace_first(char *str, char *word1, char *word2);

/* Clear tmp_dir. */
static void clear_tmp();

/* Final cd operation.
   path must be non null. */
static void cd(const char *path);

/* Return to home directory. */
static void go_to_parent();

/* Set current working directory to path, with validation of dir. */
int set_directory(const char *dirstr)
{
    if (!dirstr || !strlen(dirstr))
    {
        /* If arguments is invalid. */
        cd(home_dir);
        return DIR_EXIST;
    }

    /* Go back to parent directory. */
    if(!strcmp(dirstr, ".."))
    {
        go_to_parent();
        return DIR_EXIST;
    }

    /* Try to open current directory. */
    DIR *dir = opendir(dirstr);
    if (dir)
    {
        /* If success. */
        closedir(dir);
        cd(dirstr);
        return DIR_EXIST;
    } else
        /* No such file or directory. */
    if (errno == ENOENT)
    {
        /* Try to find this directory in current working directory.
           For example:
            maxim@pc:~/unix_shell/$ cd folder1
            maxim@pc:~/unix_shell/folder1$
            */
        getcwd(buffer, sizeof(buffer));
        char *tmp = concat_str(buffer, "/");

        if(!tmp)
            return CANT_OPEN_DIR;

        char *newdir = concat_str(tmp, dirstr);

        free(tmp);

        if(!newdir)
            return CANT_OPEN_DIR;

        dir = opendir(newdir);
        if (dir)
        {
            /* If success. */
            cd(dirstr);
            return DIR_EXIST;
        } else
        {
            /* If we can't open directory. */
            if (errno == ENOENT)
                return DIR_NOT_EXIST;
            else
            {
                /* Check directory is file. Not folder. */
                if(!is_dir(dirstr))
                    return DIR_IS_FILE;
                else
                    /* We don't have permissions to open the folder, or this is an unexpected error. */
                    return CANT_OPEN_DIR;
            }
        }
    } else
    {
        /* Check directory is file. Not folder. */
        if(!is_dir(dirstr))
            return DIR_IS_FILE;
        else
            /* We don't have permissions to open the folder, or this is an unexpected error. */
            return CANT_OPEN_DIR;
    }
}

/* Get directory string for invite string.
   dirstr must be non null. */
void get_dir_prompt(char *dirstr)
{
    assert(dirstr != NULL);

    /* Check if we already have a string made. */
    if(tmp_dir)
    {
        strcpy(dirstr, tmp_dir);
        return;
    }
    /* Make string again. Now we get current working directory. */
    getcwd(buffer, sizeof(buffer));

    /* Format directory string. */
    char *newdir = malloc(sizeof(char) * strlen(buffer) + 1);

    if(!newdir)
    {
        perror("malloc");
        return;
    }

    strcpy(newdir, buffer);
    /* Replace our home directory to ~ . */
    replace_first(newdir, home_dir, "~");

    if (!strcmp(newdir, home_dir))
    {
        strcpy(dirstr, "~");
        return;
    }

    char *newdir_copy = newdir;

    /* If size of directory string is greater than needed. */
    if (strlen(newdir) > strlen(dirstr))
        while (newdir && strlen(newdir) + 3 > MAX_DIRECTORY_SIZE)
            newdir = strpbrk(newdir, "/\\");

    memset(dirstr, ' ', strlen(dirstr));
    if (!newdir)
    {
        dirstr[0] = '.';
        dirstr[1] = '.';
        dirstr[2] = '.';
        dirstr[3] = '\0';
    } else
    {
        /* If we cropped directory string. */
        if (!(newdir - newdir_copy))
            strcpy(dirstr, newdir);
        else
        {
            dirstr[0] = '.';
            dirstr[1] = '.';
            dirstr[2] = '/';
            strcpy(dirstr + 3, newdir);
        }
    }
    tmp_dir = newdir;
}

/* Clear memory of string dir buffers at exit. */
void free_dir()
{
    if(tmp_dir)
        free(tmp_dir);

    /* We can't free home_dir, if he takes a pointer from getenv("HOME"). */
    if(!found_home_var)
        free(home_dir);
}

/* Final cd operation.
   path must be non null. */
static void cd(const char *path)
{
    assert(path != NULL);

    /* We clear tmp_dir because of now we move to new directory. */
    if(tmp_dir)
        clear_tmp();
    chdir(path);
}

/* Init home directory of shell.
   begin must be non null. */
void init_home(char *begin)
{
    assert(begin != NULL);

    /* Set home to $HOME environment variable. */
    if ((home_dir = getenv("HOME")))
    {
        found_home_var = 1;
        cd(home_dir);
    }
    else
    {
        /* Set working directory to directory, where shell was executed. */
        char *newdir = malloc(sizeof(char) * strlen(begin));

        if(!newdir)
        {
            perror("malloc");
            return;
        }

        strcpy(newdir, begin);
        for (int i = (int)strlen(newdir) - 1; i >= 0; --i)
            if (newdir[i] == '/' || newdir[i] == '\\')
            {
                newdir[i] = '\0';
                break;
            }
        home_dir = malloc(sizeof(char) * strlen(newdir));

        if(!home_dir)
        {
            perror("malloc");
            free(newdir);
            return;
        }

        strcpy(home_dir, newdir);

        cd(newdir);

        free(newdir);
    }
}

/* Concat strings.
   Strings must be non null. */
static char *concat_str(const char *s0, const char *s1)
{
    assert(s0 != NULL);
    assert(s1 != NULL);

    char *str = (char *) malloc(sizeof(char) * (1 + strlen(s0) + strlen(s1)));
    if(!str)
    {
        perror("malloc");
        return NULL;
    }
    strcpy(str, s0);
    strcat(str, s1);
    return str;
}

/* Check path is valid directory. */
static int is_dir(const char *path)
{
    assert(path != NULL);

    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

/* Return to home directory. */
static void go_to_parent()
{
    /* Check, if we are in root directory. */
    getcwd(buffer, sizeof(buffer));
    if(!strcmp(buffer, "/"))
        return;


    for (int i = (int)strlen(buffer) - 1; i >= 0; --i)
        if ((buffer[i] == '/' || buffer[i] == '\\'))
        {
            /* Move to root, in we haven't any parents. */
            if (i == 0)
            {
                cd("/");
                return;
            }

            /* Cut string on position, where ends parent directory path. */
            char del = buffer[i];
            buffer[i] = '\0';
            cd(buffer);
            buffer[i] = del;
            break;
        }
}

/* Clear tmp_dir. */
static void clear_tmp()
{
    free(tmp_dir);
    tmp_dir = NULL;
}

/* Replace first word1 in str to word2. */
static void replace_first(char *str, char *word1, char *word2)
{
    assert(str != NULL);
    assert(word1 != NULL);
    assert(word2 != NULL);

    char tmp[1024 * 5];
    char *search;

    search = strstr(str, word1);
    if (search)
    {
        ptrdiff_t head_length = search - str;
        size_t sentence_length = strlen(str);
        size_t word1_length = strlen(word1);
        size_t word2_length = strlen(word2);

        if (sentence_length + word2_length - word1_length < 1024 * 5)
        {
            strncpy(tmp, str, (size_t)head_length);
            strcpy(tmp + head_length, word2);
            strcpy(tmp + head_length + word2_length, search + word1_length);
            strcpy(str, tmp);
        }
    }
}
