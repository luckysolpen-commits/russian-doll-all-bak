#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Methods plugin - provides reusable behavioral methods
int method_process_turn(const char* piece_name) {
    printf("Processing turn for %s...\n", piece_name);
    return 1;
}

int method_update_stats(const char* piece_name) {
    printf("Updating stats for %s...\n", piece_name);
    return 1;
}

int method_increment_turn(const char* piece_name) {
    // First get the current turn value using piece manager
    FILE *fp = popen("./+x/piece_manager.+x get_piece_state player", "r");
    int current_turn = 0;
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "turn ", 5) == 0) {
                sscanf(line + 5, "%d", &current_turn);
                break;
            }
        }
        pclose(fp);
    }
    
    // Update piece state to increment turn counter
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./+x/piece_manager.+x update_piece_state player player turn %d", current_turn + 1);
    system(cmd);
    printf("Turn incremented for %s\n", piece_name);
    return 1;
}

int method_increment_hunger(const char* piece_name) {
    // First get the current hunger value using piece manager
    FILE *fp = popen("./+x/piece_manager.+x get_piece_state player", "r");
    int current_hunger = 0;
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "hunger ", 7) == 0) {
                sscanf(line + 7, "%d", &current_hunger);
                break;
            }
        }
        pclose(fp);
    }
    
    // Update piece state to increment hunger
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./+x/piece_manager.+x update_piece_state player player hunger %d", current_hunger + 1);
    system(cmd);
    printf("Hunger incremented for %s\n", piece_name);
    return 1;
}

int method_read_piece_state(const char* piece_name) {
    printf("Reading state for %s...\n", piece_name);
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "./+x/piece_manager.+x get_piece_state %s", piece_name);
    system(cmd);
    return 1;
}

int method_update_piece_state(const char* piece_name) {
    printf("Updating state for %s...\n", piece_name);
    return 1;
}

int method_validate_state(const char* piece_name) {
    printf("Validating state for %s...\n", piece_name);
    return 1;
}

int method_move_towards_player(const char* piece_name) {
    printf("Moving %s towards player...\n", piece_name);
    return 1;
}

int method_update_position(const char* piece_name) {
    printf("Updating position for %s...\n", piece_name);
    return 1;
}

int method_perform_attack(const char* piece_name) {
    printf("Performing attack for %s...\n", piece_name);
    return 1;
}

int method_take_damage(const char* piece_name) {
    printf("Applying damage to %s...\n", piece_name);
    return 1;
}

int method_heal(const char* piece_name) {
    printf("Healing %s...\n", piece_name);
    return 1;
}

int method_tick(const char* piece_name) {
    printf("Time tick for %s...\n", piece_name);
    return 1;
}

int method_get_time(const char* piece_name) {
    printf("Getting time for %s...\n", piece_name);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <method_name> <piece_name>\n", argv[0]);
        printf("Available methods: process_turn, update_stats, increment_turn, increment_hunger,\n");
        printf("                   read_piece_state, update_piece_state, validate_state,\n");
        printf("                   move_towards_player, update_position, perform_attack,\n");
        printf("                   take_damage, heal, tick, get_time\n");
        return 1;
    }
    
    char* method_name = argv[1];
    char* piece_name = argv[2];
    
    // Execute the requested method
    if (strcmp(method_name, "process_turn") == 0) {
        return method_process_turn(piece_name);
    } else if (strcmp(method_name, "update_stats") == 0) {
        return method_update_stats(piece_name);
    } else if (strcmp(method_name, "increment_turn") == 0) {
        return method_increment_turn(piece_name);
    } else if (strcmp(method_name, "increment_hunger") == 0) {
        return method_increment_hunger(piece_name);
    } else if (strcmp(method_name, "read_piece_state") == 0) {
        return method_read_piece_state(piece_name);
    } else if (strcmp(method_name, "update_piece_state") == 0) {
        return method_update_piece_state(piece_name);
    } else if (strcmp(method_name, "validate_state") == 0) {
        return method_validate_state(piece_name);
    } else if (strcmp(method_name, "move_towards_player") == 0) {
        return method_move_towards_player(piece_name);
    } else if (strcmp(method_name, "update_position") == 0) {
        return method_update_position(piece_name);
    } else if (strcmp(method_name, "perform_attack") == 0) {
        return method_perform_attack(piece_name);
    } else if (strcmp(method_name, "take_damage") == 0) {
        return method_take_damage(piece_name);
    } else if (strcmp(method_name, "heal") == 0) {
        return method_heal(piece_name);
    } else if (strcmp(method_name, "tick") == 0) {
        return method_tick(piece_name);
    } else if (strcmp(method_name, "get_time") == 0) {
        return method_get_time(piece_name);
    } else {
        printf("Unknown method: %s\n", method_name);
        return 1;
    }
    
    return 0;
}