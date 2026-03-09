#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <stdarg.h>

// clock_daemon.c - System Clock Daemon (v1.2 - DECLARATION FIX)
// Responsibility: Warning-free background time ticking.

#define MAX_PATH 16384

char project_root[MAX_PATH] = ".";

char* trim_str(char *str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

char* trim_pmo(char *str) { return trim_str(str); }

void resolve_root() {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[4096];
        while (fgets(line, sizeof(line), kvp)) {
            if (strncmp(line, "project_root=", 13) == 0) {
                char *v = line + 13;
                strncpy(project_root, trim_pmo(v), MAX_PATH - 1); break;
            }
        }
        fclose(kvp);
    }
}

int main() {
    resolve_root();
    int turn = 0;
    
    while (1) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char *new_time = NULL;
        asprintf(&new_time, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
        
        char *path = NULL;
        asprintf(&path, "%s/pieces/system/clock_daemon/state.txt", project_root);
        FILE *f = fopen(path, "w");
        if (f) {
            fprintf(f, "time=%s\nturn=%d\nmode=auto\n", new_time, turn);
            fclose(f);
        }
        
        char *frame_ch = NULL;
        asprintf(&frame_ch, "%s/pieces/master_ledger/frame_changed.txt", project_root);
        FILE *marker = fopen(frame_ch, "a");
        if (marker) { fprintf(marker, "C\n"); fclose(marker); }
        
        free(new_time); free(path); free(frame_ch);
        sleep(1);
        turn++;
    }
    return 0;
}
