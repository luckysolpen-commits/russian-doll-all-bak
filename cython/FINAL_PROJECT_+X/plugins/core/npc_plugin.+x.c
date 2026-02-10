#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[]) {
    printf("NPC plugin initialized\n");
    
    // If called with respond_to_turn_end argument, process NPC movement
    if (argc >= 3 && strcmp(argv[1], "respond_to_turn_end") == 0) {
        char* npc_name = argv[2];
        
        // Get current NPC state
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "./+x/piece_manager.+x get_piece_state %s", npc_name);
        FILE* fp = popen(cmd, "r");
        
        if (fp) {
            char line[256];
            int x = 0, y = 0;
            int has_position = 0;
            
            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "x ", 2) == 0) {
                    sscanf(line + 2, "%d", &x);
                    has_position = 1;
                } else if (strncmp(line, "y ", 2) == 0) {
                    sscanf(line + 2, "%d", &y);
                    has_position = 1;
                }
            }
            pclose(fp);
            
            if (has_position) {
                // Simple AI: move randomly
                srand(time(NULL)); // For random movement
                
                // Random direction
                int dx = (rand() % 3) - 1;  // -1, 0, or 1
                int dy = (rand() % 3) - 1;  // -1, 0, or 1
                
                // Check world collision using world plugin
                char world_check_cmd[512];
                snprintf(world_check_cmd, sizeof(world_check_cmd), 
                         "echo \"Checking move from (%d, %d) to (%d, %d)\" 2>/dev/null", 
                         x, y, x+dx, y+dy);
                system(world_check_cmd);
                
                // Update NPC position
                char update_cmd[512];
                snprintf(update_cmd, sizeof(update_cmd), 
                         "./+x/piece_manager.+x update_piece_state %s %s x %d", 
                         npc_name, npc_name, x + dx);
                system(update_cmd);
                
                snprintf(update_cmd, sizeof(update_cmd), 
                         "./+x/piece_manager.+x update_piece_state %s %s y %d", 
                         npc_name, npc_name, y + dy);
                system(update_cmd);
                
                printf("NPC %s moved to position (%d, %d)\n", npc_name, x + dx, y + dy);
                
                // Update the last move time
                snprintf(update_cmd, sizeof(update_cmd),
                         "./+x/piece_manager.+x update_piece_state %s %s last_move_time %d", 
                         npc_name, npc_name, (int)time(NULL));
                system(update_cmd);
            } else {
                printf("NPC %s has no position to update\n", argv[2]);
            }
        } else {
            printf("Could not get state for NPC %s\n", argv[2]);
        }
    } else if (argc >= 2 && strcmp(argv[1], "initialize_npcs") == 0) {
        // Initialize NPC pieces if they don't exist
        printf("Initializing NPCs...\n");
        
        // Check if zombie piece exists, create if not
        system("mkdir -p peices/zombie");
        if (access("peices/zombie/zombie.txt", F_OK) != 0) {
            FILE* fp = fopen("peices/zombie/zombie.txt", "w");
            if (fp) {
                fprintf(fp, "x 20\n");
                fprintf(fp, "y 12\n");
                fprintf(fp, "symbol Z\n");
                fprintf(fp, "type enemy\n");
                fprintf(fp, "health 100\n");
                fprintf(fp, "speed 0.5\n");
                fprintf(fp, "name Zombie\n");
                fprintf(fp, "description \"Zombie enemy\"\n");
                fprintf(fp, "event_listeners [turn_end]\n");
                fprintf(fp, "methods [move_towards_player, update_position, attack]\n");
                fprintf(fp, "status active\n");
                fclose(fp);
            }
        }
        
        // Check if sheep piece exists, create if not
        system("mkdir -p peices/sheep");
        if (access("peices/sheep/sheep.txt", F_OK) != 0) {
            FILE* fp = fopen("peices/sheep/sheep.txt", "w");
            if (fp) {
                fprintf(fp, "x 25\n");
                fprintf(fp, "y 10\n");
                fprintf(fp, "symbol &\n");
                fprintf(fp, "type animal\n");
                fprintf(fp, "health 50\n");
                fprintf(fp, "speed 0.2\n");
                fprintf(fp, "name Sheep\n");
                fprintf(fp, "description \"Friendly Sheep\"\n");
                fprintf(fp, "event_listeners [turn_end]\n");
                fprintf(fp, "methods [wander, update_position]\n");
                fprintf(fp, "status active\n");
                fclose(fp);
            }
        }
        
        printf("NPCs initialized\n");
    } else {
        printf("Usage: %s [respond_to_turn_end <npc_name> | initialize_npcs]\n", argv[0]);
    }
    
    return 0;
}