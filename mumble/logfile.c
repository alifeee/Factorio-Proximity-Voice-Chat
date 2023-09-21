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
typedef SSIZE_T ssize_t;
#else // linux / macos
#include <unistd.h>
#include <pwd.h>
#endif

#define POSITION_INFO_PATH "/Factorio/script-output/player_position.txt"

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
    return file_exists(factorioLogfile);
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
    return get_file_modified_time(factorioLogfile);
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
 * @brief Loads Factorio log file and assigns values to given pointers
 *
 * @param x x coordinate
 * @param y y coordinate
 * @param z z coordinate
 * @param player player id (integer)
 * @param surface surface id (integer)
 * @param server server id (string) - username of host
 * @param server_len length of server string
 */
void parse_factorio_logfile(float *x, float *y, float *z, int *player, int *surface, char **server, size_t *server_len)
{
    char *homeDir = get_home_dir();
    char *factorioLogfile = malloc(strlen(homeDir) + strlen(POSITION_INFO_PATH) + 1);
    strcpy(factorioLogfile, homeDir);
    strcat(factorioLogfile, POSITION_INFO_PATH);

    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(factorioLogfile, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1)
    {
        // skip first line
        if (strstr(line, "XYZ") != NULL)
        {
            continue;
        }
        // printf("Retrieved line of length %zu:\n", read);
        // printf("%s", line);

        char *token = strtok(line, ":");
        char *value = strtok(NULL, ":");

        // printf("token: %s\n", token);
        // printf("value: %s\n", value);

        if (strcmp(token, "x") == 0)
        {
            *x = atof(value);
        }
        else if (strcmp(token, "y") == 0)
        {
            *y = atof(value);
        }
        else if (strcmp(token, "z") == 0)
        {
            *z = atof(value);
        }
        else if (strcmp(token, "p") == 0)
        {
            *player = atoi(value);
        }
        else if (strcmp(token, "u") == 0)
        {
            *surface = atoi(value);
        }
        else if (strcmp(token, "s") == 0)
        {
            // strip newline if present
            if (value[strlen(value) - 1] == '\n')
            {
                value[strlen(value) - 1] = '\0';
            }
            *server_len = strlen(value);
            *server = malloc(*server_len + 1);
            strcpy(*server, value);
        }
    }

    fclose(fp);
    if (line)
        free(line);
}
