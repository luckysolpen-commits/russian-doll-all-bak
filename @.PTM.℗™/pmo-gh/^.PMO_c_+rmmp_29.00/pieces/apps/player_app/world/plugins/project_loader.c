#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

// project_loader.c - Engine Operation (v1.0)
// Responsibility: Scan projects/ dir, generate a PDL menu, and return selection.

#define MAX_PATH 1024

int main() {
    DIR *dir = opendir("./projects");
    if (!dir) {
        printf("0\n");
        return 1;
    }

    FILE *f = fopen("./pieces/apps/player_app/PAL/tmp_projects.pdl", "w");
    if (!f) {
        closedir(dir);
        printf("0\n");
        return 1;
    }

    fprintf(f, "META | piece_id | tmp_projects\n");
    fprintf(f, "META | title    | Select Project\n\n");

    struct dirent *entry;
    char proj_list[16][256];
    int count = 0;
    while ((entry = readdir(dir)) != NULL && count < 16) {
        if (entry->d_name[0] == '.') continue;
        
        char path[MAX_PATH];
        snprintf(path, MAX_PATH, "./projects/%s", entry->d_name);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            strcpy(proj_list[count], entry->d_name);
            fprintf(f, "METHOD | %s | void\n", entry->d_name);
            count++;
        }
    }
    fclose(f);
    closedir(dir);

    if (count == 0) {
        printf("0\n");
        return 0;
    }

    // Now call menu_op on the generated PDL
    char cmd[MAX_PATH * 2];
    snprintf(cmd, sizeof(cmd), "./pieces/apps/player_app/world/plugins/+x/menu_op.+x ./pieces/apps/player_app/PAL/tmp_projects.pdl");
    
    FILE *mf = popen(cmd, "r");
    if (mf) {
        char buf[16];
        if (fgets(buf, sizeof(buf), mf)) {
            int sel = atoi(buf);
            if (sel > 0 && sel <= count) {
                // Success! Write selected project to engine state
                FILE *sf = fopen("./pieces/apps/player_app/manager/state.txt", "a");
                if (sf) {
                    fprintf(sf, "project_id=%s\n", proj_list[sel-1]);
                    fclose(sf);
                }
                printf("%d\n", sel); // Return success index to VM
            } else {
                printf("0\n");
            }
        }
        pclose(mf);
    }

    return 0;
}
