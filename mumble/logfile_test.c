#include "logfile.c"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    char *homeDir = get_home_dir();
    printf("Home dir: %s\n\n", homeDir);

    if (is_factorio_logfile_there())
    {
        printf("Factorio logfile is present\n\n");
    }
    else
    {
        printf("Factorio logfile is not present. Terminating...\n");
        return 0;
    }

    float x, y, z;
    int player, surface;
    char *server;
    size_t server_len;

    parse_factorio_logfile(&x, &y, &z, &player, &surface, &server, &server_len);

    // log variables
    printf("x: %f\n", x);
    printf("y: %f\n", y);
    printf("z: %f\n", z);
    printf("player: %d\n", player);
    printf("surface: %d\n", surface);
    printf("server: %s\n", server);

    // combine surface and server
    char *context_str = malloc(server_len + 1 + 1 + 1);
    strcpy(context_str, server);
    strcat(context_str, "/");
    char surface_str[12];
    sprintf(surface_str, "%d", surface);
    strcat(context_str, surface_str);
    char *context = context_str;

    printf("context: %s\n", context_str);

    // get_file_modified_time
    time_t modified_time = get_factorio_file_modified_time();
    printf("modified time: %ld\n", modified_time);
    // compare to time now
    time_t now = time(NULL);
    printf("now: %ld\n", now);
    // difference
    time_t diff = now - modified_time;
    printf("diff: %ld\n", diff);
    if (diff > 20)
    {
        printf("More than 20 seconds ago!\n");
    }

    // is recent?
    if (is_factorio_logfile_recent(2))
    {
        printf("Factorio logfile is recent\n");
    }
    else
    {
        printf("Factorio logfile is not recent.\n");
    }

    return 0;
}