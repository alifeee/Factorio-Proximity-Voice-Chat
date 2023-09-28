#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#include <BaseTsd.h>
#include "getline.c"
#else // linux / macos
#include <unistd.h>
#include <pwd.h>
#endif

#define POSITION_INFO_PATH "/Factorio/script-output/mumble_positional-audio_information.txt"

// for c_read_file func below
#define FILE_OK 0
#define FILE_NOT_EXIST 1
#define FILE_TOO_LARGE 2
#define FILE_READ_ERROR 3
// for parse_factorio_logfile func below
#define FILE_PARSE_NO_XYZ 4

/**
 * @brief Get the string for the home directory
 * This is OS dependent
 *
 * @return char* home directory string
 */
char *get_home_dir()
{
#ifdef _WIN32
    char *appdata = getenv("APPDATA");
    return appdata;
#else
    char *homeDir = getenv("HOME");
    if (!homeDir)
    {
        struct passwd *pwd = getpwuid(getuid());
        if (pwd)
            homeDir = pwd->pw_dir;
    }
    return homeDir;
#endif
}

/**
 * @brief Check if file exists
 *
 * @param fname filename
 * @return int 1 if it exists, 0 if it does not
 */
int file_exists(const char *fname)
{
    FILE *file;
    if ((file = fopen(fname, "r")))
    {
        fclose(file);
        return 1;
    }
    return 0;
}

/**
 * @brief Check if the factorio logfile is present
 *
 * @return int 1 if it exists, 0 if it does not
 */
int is_factorio_logfile_there()
{
    char *homeDir = get_home_dir();
    char *factorioLogfile = malloc(strlen(homeDir) + strlen(POSITION_INFO_PATH) + 1);
    strcpy(factorioLogfile, homeDir);
    strcat(factorioLogfile, POSITION_INFO_PATH);
    int does_exist = file_exists(factorioLogfile);
    free(factorioLogfile);
    return does_exist;
}

time_t get_file_modified_time(char *path)
{
    struct stat attr;
    stat(path, &attr);
    return attr.st_mtime;
}

time_t get_factorio_file_modified_time()
{
    char *homeDir = get_home_dir();
    char *factorioLogfile = malloc(strlen(homeDir) + strlen(POSITION_INFO_PATH) + 1);
    strcpy(factorioLogfile, homeDir);
    strcat(factorioLogfile, POSITION_INFO_PATH);
    time_t modified_time = get_file_modified_time(factorioLogfile);
    free(factorioLogfile);
    return modified_time;
}

int is_factorio_logfile_recent(int seconds)
{
    // default argument
    if (seconds == 0)
    {
        seconds = 2;
    }
    time_t modified_time = get_factorio_file_modified_time();
    time_t now = time(NULL);
    time_t diff = now - modified_time;
    if (diff > seconds)
    {
        return 0;
    }
    return 1;
}

/**
 * @brief Reads a file into a string (returns err on failure)
 * from https://stackoverflow.com/a/54057690
 *
 * @param f_name filename
 * @param err error code (pointer)
 * @param f_size size of file (pointer)
 * @return char* string of file contents
 */
char *c_read_file(const char *f_name, int *err, size_t *f_size)
{
    char *buffer;
    size_t length;
    FILE *f = fopen(f_name, "rb");
    size_t read_length;

    if (f)
    {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);

        // 1 GiB; best not to load a whole large file in one string
        if (length > 1073741824)
        {
            *err = FILE_TOO_LARGE;

            return NULL;
        }

        buffer = (char *)malloc(length + 1);

        if (length)
        {
            read_length = fread(buffer, 1, length, f);

            if (length != read_length)
            {
                free(buffer);
                *err = FILE_READ_ERROR;

                return NULL;
            }
        }

        fclose(f);

        *err = FILE_OK;
        buffer[length] = '\0';
        *f_size = length;
    }
    else
    {
        *err = FILE_NOT_EXIST;

        return NULL;
    }

    return buffer;
}

/*
 * public domain strtok_r() by charlie gordon
 *
 *   from comp.lang.c  9/14/2007
 *
 *      http://groups.google.com/group/comp.lang.c/msg/2ab1ecbb86646684
 *
 *     (declaration that it's public domain):
 *      http://groups.google.com/group/comp.lang.c/msg/7c7b39328fefab9c
 */

char *strtok_r(
    char *str,
    const char *delim,
    char **nextp)
{
    char *ret;

    if (str == NULL)
    {
        str = *nextp;
    }

    str += strspn(str, delim);

    if (*str == '\0')
    {
        return NULL;
    }

    ret = str;

    str += strcspn(str, delim);

    if (*str)
    {
        *str++ = '\0';
    }

    *nextp = str;

    return ret;
}

/**
 * @brief Loads Factorio log file and assigns values to given pointers
 *
 * @param x x coordinate
 * @param y y coordinate
 * @param z z coordinate
 * @param player player id (integer)
 * @param surface surface id (integer)
 * @param server server id (string) - username of host
 * @param server_len length of server string
 * @param error error code (pointer)
 * @return int 1 if successful, 0 if not
 */
int parse_factorio_logfile(float *x, float *y, float *z, int *player, int *surface, char **server, size_t *server_len, int *error)
{
    char *homeDir = get_home_dir();
    char *factorioLogfile = malloc(strlen(homeDir) + strlen(POSITION_INFO_PATH) + 1);
    strcpy(factorioLogfile, homeDir);
    strcat(factorioLogfile, POSITION_INFO_PATH);

    int err;
    size_t f_size;
    char *f_data;

    f_data = c_read_file(factorioLogfile, &err, &f_size);

    if (err)
    {
        // process error
        *error = err;
        return 0;
    }
    // process data

    // check XYZ is present
    // IMPORTANT: otherwise, the file is invalid somehow (thanks Lua)
    //  and Mumble will crash when trying to parse it
    if (strstr(f_data, "XYZ") == NULL)
    {
        *error = FILE_PARSE_NO_XYZ;
        return 0;
    }
    // printf("%s\n", f_data);
    // split by newline
    char *line;
    char *saveptr;
    line = strtok_r(f_data, "\n", &saveptr);
    while (line != NULL)
    {
        // skip first line
        if (strstr(line, "XYZ") != NULL)
        {
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }
        // printf("line!\n");
        // printf(" %s\n", line);
        // split by colon
        char *token2;
        char *value2;
        char *saveptr2;
        token2 = strtok_r(line, ":", &saveptr2);
        value2 = strtok_r(NULL, ":", &saveptr2);

        if (strcmp(token2, "x") == 0)
        {
            *x = atof(value2);
        }
        else if (strcmp(token2, "y") == 0)
        {
            *y = atof(value2);
        }
        else if (strcmp(token2, "z") == 0)
        {
            *z = atof(value2);
        }
        else if (strcmp(token2, "p") == 0)
        {
            *player = atoi(value2);
        }
        else if (strcmp(token2, "u") == 0)
        {
            *surface = atoi(value2);
        }
        else if (strcmp(token2, "s") == 0)
        {
            // strip newline if present
            if (value2[strlen(value2) - 1] == '\n')
            {
                value2[strlen(value2) - 1] = '\0';
            }
            *server_len = strlen(value2);
            *server = malloc(*server_len + 1);
            strcpy(*server, value2);
        }

        line = strtok_r(NULL, "\n", &saveptr);
    }
    free(f_data);
    free(factorioLogfile);
    return 1;
}
