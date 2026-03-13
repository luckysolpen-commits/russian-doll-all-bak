/*
 * Op: create_piece
 * Usage: ./create_piece.+x <type> <x> <y> [project_id]
 * 
 * Creates a new piece directory at projects/{id}/pieces/{type}_{x}_{y}/
 * with state.txt and piece.pdl files.
 * 
 * Type mapping:
 *   @ or player → player piece
 *   & or npc    → npc piece
 *   Z or zombie → zombie piece
 *   T or chest  → chest piece
 *   * or item   → item piece
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_PATH 4096
#define MAX_CMD 16384
#define MAX_LINE 256

char project_root[MAX_PATH] = ".";
char current_project[MAX_LINE] = "template";

char* trim_str(char *str) {
    char *end;
    if(!str) return str;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void resolve_paths() {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), kvp)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(line);
                char *v = trim_str(eq + 1);
                if (strcmp(k, "project_root") == 0) snprintf(project_root, sizeof(project_root), "%s", v);
            }
        }
        fclose(kvp);
    }
    
    // Load from editor state
    FILE *state = fopen("pieces/apps/editor/state.txt", "r");
    if (state) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), state)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(line);
                char *v = trim_str(eq + 1);
                if (strcmp(k, "project_id") == 0) snprintf(current_project, sizeof(current_project), "%s", v);
            }
        }
        fclose(state);
    }
}

const char* get_piece_id_from_type(const char *type) {
    if (strcmp(type, "@") == 0 || strcmp(type, "player") == 0) return "player";
    if (strcmp(type, "&") == 0 || strcmp(type, "npc") == 0) return "npc";
    if (strcmp(type, "Z") == 0 || strcmp(type, "zombie") == 0) return "zombie";
    if (strcmp(type, "T") == 0 || strcmp(type, "chest") == 0) return "chest";
    if (strcmp(type, "*") == 0 || strcmp(type, "item") == 0) return "item";
    if (strcmp(type, "X") == 0 || strcmp(type, "selector") == 0) return "selector";
    return "entity";  // Generic fallback
}

const char* get_icon_from_type(const char *type) {
    if (strcmp(type, "@") == 0 || strcmp(type, "player") == 0) return "@";
    if (strcmp(type, "&") == 0 || strcmp(type, "npc") == 0) return "&";
    if (strcmp(type, "Z") == 0 || strcmp(type, "zombie") == 0) return "Z";
    if (strcmp(type, "T") == 0 || strcmp(type, "chest") == 0) return "T";
    if (strcmp(type, "*") == 0 || strcmp(type, "item") == 0) return "*";
    if (strcmp(type, "X") == 0 || strcmp(type, "selector") == 0) return "X";
    return "?";
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <type> <x> <y> [project_id]\n", argv[0]);
        fprintf(stderr, "  type: @, &, Z, T, *, X or player, npc, zombie, chest, item, selector\n");
        return 1;
    }
    
    resolve_paths();
    
    const char *type = argv[1];
    int x = atoi(argv[2]);
    int y = atoi(argv[3]);
    
    if (argc > 4) {
        snprintf(current_project, sizeof(current_project), "%s", argv[4]);
    }
    
    // Determine piece_id and icon
    const char *piece_id = get_piece_id_from_type(type);
    const char *icon = get_icon_from_type(type);
    
    // Create directory: projects/{id}/pieces/{type}_{x}_{y}/
    char *piece_dir = NULL;
    if (asprintf(&piece_dir, "%s/projects/%s/pieces/%s_%d_%d", project_root, current_project, piece_id, x, y) == -1) return 1;
    
    if (mkdir(piece_dir, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "Error: Cannot create directory %s\n", piece_dir);
        free(piece_dir);
        return 1;
    }
    
    // Create state.txt
    char *state_path = NULL;
    if (asprintf(&state_path, "%s/state.txt", piece_dir) == -1) { free(piece_dir); return 1; }
    FILE *f = fopen(state_path, "w");
    if (!f) {
        fprintf(stderr, "Error: Cannot create %s\n", state_path);
        free(piece_dir); free(state_path);
        return 1;
    }
    
    fprintf(f, "name=%s_%d_%d\n", piece_id, x, y);
    fprintf(f, "type=%s\n", piece_id);
    fprintf(f, "pos_x=%d\n", x);
    fprintf(f, "pos_y=%d\n", y);
    fprintf(f, "pos_z=0\n");
    fprintf(f, "on_map=1\n");
    fprintf(f, "icon=%s\n", icon);
    
    // Add type-specific defaults
    if (strcmp(piece_id, "npc") == 0) {
        fprintf(f, "behavior=passive\n");
        fprintf(f, "dialogue=Hello, traveler!\n");
    } else if (strcmp(piece_id, "zombie") == 0) {
        fprintf(f, "behavior=aggressive\n");
        fprintf(f, "speed=1\n");
    } else if (strcmp(piece_id, "chest") == 0) {
        fprintf(f, "contents=gold\n");
        fprintf(f, "opened=0\n");
    } else if (strcmp(piece_id, "player") == 0) {
        fprintf(f, "health=100\n");
        fprintf(f, "inventory=\n");
    }
    fclose(f);

    // Create piece.pdl (minimal DNA)
    char *pdl_path = NULL;
    if (asprintf(&pdl_path, "%s/piece.pdl", piece_dir) == -1) { free(piece_dir); free(state_path); return 1; }
    f = fopen(pdl_path, "w");
    if (!f) {
        fprintf(stderr, "Error: Cannot create %s\n", pdl_path);
        free(piece_dir); free(state_path); free(pdl_path);
        return 1;
    }

    fprintf(f, "# Piece: %s_%d_%d\n", piece_id, x, y);
    fprintf(f, "META         | piece_id           | %s_%d_%d\n", piece_id, x, y);
    fprintf(f, "STATE        | pos_x              | %d\n", x);
    fprintf(f, "STATE        | pos_y              | %d\n", y);
    fprintf(f, "STATE        | pos_z              | 0\n");
    fprintf(f, "STATE        | on_map             | 1\n");
    fprintf(f, "STATE        | type               | %s\n", piece_id);

    fclose(f);

    free(piece_dir); free(state_path); free(pdl_path);

    // Add methods based on type
    if (strcmp(piece_id, "npc") == 0) {
        fprintf(f, "METHOD       | on_interact        | ./pieces/apps/playrm/ops/+x/interact.+x %s_%d_%d\n", piece_id, x, y);
    } else if (strcmp(piece_id, "zombie") == 0) {
        fprintf(f, "METHOD       | on_turn            | ./pieces/apps/playrm/ops/+x/zombie_ai.+x %s_%d_%d\n", piece_id, x, y);
    } else if (strcmp(piece_id, "chest") == 0) {
        fprintf(f, "METHOD       | on_interact        | ./pieces/apps/playrm/ops/+x/open_chest.+x %s_%d_%d\n", piece_id, x, y);
    }
    
    fclose(f);
    
    // Log to ledger (for undo)
    char ledger_path[MAX_CMD];
    snprintf(ledger_path, sizeof(ledger_path),
             "%s/projects/%s/config/editor_history.ledger",
             project_root, current_project);
    FILE *ledger = fopen(ledger_path, "a");
    if (ledger) {
        fprintf(ledger, "CREATE_PIECE|%s|%d|%d\n", piece_id, x, y);
        fclose(ledger);
    }
    
    // Hit frame marker
    char path[MAX_CMD];
    snprintf(path, sizeof(path), "%s/pieces/display/frame_changed.txt", project_root);
    FILE *fm = fopen(path, "a");
    if (fm) { fprintf(fm, "X\n"); fclose(fm); }
    
    snprintf(path, sizeof(path), "%s/pieces/apps/editor/view_changed.txt", project_root);
    fm = fopen(path, "a");
    if (fm) { fprintf(fm, "X\n"); fclose(fm); }
    
    printf("Created piece: %s_%d_%d at %s\n", piece_id, x, y, piece_dir);
    return 0;
}
