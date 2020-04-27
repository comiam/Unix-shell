#include "dirs.h"

static char buffer[PATH_MAX];
static char *tmp_dir = NULL;
static char *home_dir = NULL;

static char *concat_str(const char *s0, const char *s1);
static int  is_dir(const char *path);
static void replace_first(char *str, char *word1, char *word2);
static void clear_tmp();
static void cd(const char *path);
static void go_to_parent();

int set_directory(const char *dirstr)
{
    if (!dirstr || !strlen(dirstr))
    {
        cd(home_dir);
        return DIR_EXIST;
    }

    if(!strcmp(dirstr, ".."))
    {
        go_to_parent();
        return DIR_EXIST;
    }

    DIR *dir = opendir(dirstr);
    if (dir)
    {
        closedir(dir);
        cd(dirstr);
        return DIR_EXIST;
    } else if (errno == ENOENT)
    {
        getcwd(buffer, sizeof(buffer));
        char *tmp = concat_str(buffer, "/");
        char *newdir = concat_str(tmp, dirstr);
        free(tmp);

        dir = opendir(newdir);
        if (dir)
        {
            cd(dirstr);
            return DIR_EXIST;
        } else
        {
            if (errno == ENOENT)
                return DIR_NOT_EXIST;
            else
            {
                if(!is_dir(dirstr))
                    return DIR_IS_FILE;
                else
                    return CANT_OPEN_DIR;
            }
        }
    } else
    {
        if(!is_dir(dirstr))
            return DIR_IS_FILE;
        else
            return CANT_OPEN_DIR;
    }
}

/* make path string for prompt */
void get_dir_prompt(char *dirstr)
{
    if(tmp_dir)
    {
        strcpy(dirstr, tmp_dir);
        return;
    }
    getcwd(buffer, sizeof(buffer));

    char *newdir = malloc(sizeof(char) * strlen(buffer));
    strcpy(newdir, buffer);
    replace_first(newdir, home_dir, "~");

    if (!strcmp(newdir, home_dir))
    {
        strcpy(dirstr, "~");
        return;
    }

    char *newdir_copy = newdir;

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

/* final cd operation */
static void cd(const char *path)
{
    if(tmp_dir)
        clear_tmp();
    chdir(path);
}

void init_home(char *begin)
{
    if ((home_dir = getenv("HOME")))
        cd(home_dir);
    else
    {
        char *newdir = malloc(sizeof(char) * strlen(begin));
        strcpy(newdir, begin);
        for (size_t i = strlen(newdir) - 1; i >= 0; --i)
            if (newdir[i] == '/' || newdir[i] == '\\')
            {
                newdir[i] = '\0';
                break;
            }
        home_dir = malloc(sizeof(char) * strlen(newdir));
        strcpy(home_dir, newdir);

        cd(newdir);
    }
}

static char *concat_str(const char *s0, const char *s1)
{
    char *str = (char *) malloc(sizeof(char) * (1 + strlen(s0) + strlen(s1)));
    strcpy(str, s0);
    strcat(str, s1);
    return str;
}

static int is_dir(const char *path)
{
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

static void go_to_parent()
{
    getcwd(buffer, sizeof(buffer));

    if(!strcmp(buffer, "/"))
        return;

    for (size_t i = strlen(buffer) - 1; i >= 0; --i)
        if ((buffer[i] == '/' || buffer[i] == '\\'))
        {
            if (i == 0)
            {
                cd("/");
                return;
            }

            char del = buffer[i];
            buffer[i] = '\0';
            cd(buffer);
            buffer[i] = del;
            break;
        }
}

static void clear_tmp()
{
    free(tmp_dir);
    tmp_dir = NULL;
}

/* replace word1 to word2 */
static void replace_first(char *str, char *word1, char *word2)
{
    char tempString[1024 * 5];
    char *search;

    search = strstr(str, word1);
    if (search)
    {
        ptrdiff_t head_length = search - str;
        size_t sentence_length = strlen(str);
        size_t word1_length = strlen(word1);
        size_t word2_length = strlen(word2);

        if (sentence_length + word2_length - word1_length < 1000)
        {
            strncpy(tempString, str, head_length);
            strcpy(tempString + head_length, word2);
            strcpy(tempString + head_length + word2_length, search + word1_length);
            strcpy(str, tempString);
        }
    }
}