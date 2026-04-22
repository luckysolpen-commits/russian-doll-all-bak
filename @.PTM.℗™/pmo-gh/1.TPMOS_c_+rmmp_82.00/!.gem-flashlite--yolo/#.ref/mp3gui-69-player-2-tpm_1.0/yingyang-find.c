#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ftw.h>
#include <sys/stat.h>

FILE *playlist_file;

static int find_mp3(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    if (typeflag == FTW_F) {
        const char *filename = strrchr(fpath, '/');
        filename = (filename != NULL) ? filename + 1 : fpath;

        if (strncmp(filename, "._", 2) == 0) {
            return 0;
        }

        const char *ext = strrchr(fpath, '.');
        if (ext && strcasecmp(ext, ".mp3") == 0) {
            fprintf(playlist_file, "%s\n", fpath);
            printf("Found: %s\n", fpath);
        }
    }
    return 0;
}

char* get_path_from_config(const char* config_path) {
    FILE *f = fopen(config_path, "r");
    if (!f) return NULL;

    char line[4096];
    char *result = NULL;
    while (fgets(line, sizeof(line), f)) {
        // Strip newline
        line[strcspn(line, "\r\n")] = 0;
        
        // Skip empty lines or comments
        if (strlen(line) == 0 || line[0] == '#') continue;

        result = strdup(line);
        break;
    }
    fclose(f);
    return result;
}

int main(int argc, char *argv[]) {
    char *search_path = NULL;

    if (argc > 1) {
        search_path = strdup(argv[1]);
    } else {
        // Try local config.txt first, then the one in jukebox folder
        search_path = get_path_from_config("config.txt");
        if (!search_path) {
            search_path = get_path_from_config("../🎛️.juke-box=money-sink-NOW!/config.txt");
        }
    }

    if (!search_path) {
        printf("No search path found in config.txt or arguments. Using current directory.\n");
        search_path = strdup(".");
    }

    printf("Searching for .mp3 files in: %s\n", search_path);

    playlist_file = fopen("playlist.txt", "w");
    if (playlist_file == NULL) {
        perror("Failed to open playlist.txt for writing");
        free(search_path);
        return 1;
    }

    if (nftw(search_path, find_mp3, 20, FTW_PHYS) == -1) {
        perror("nftw failed");
        fclose(playlist_file);
        free(search_path);
        return 1;
    }

    printf("Search complete. Playlist saved to playlist.txt\n");
    fclose(playlist_file);
    free(search_path);
    return 0;
}
