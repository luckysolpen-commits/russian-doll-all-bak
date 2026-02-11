#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // Use piece_manager to get current level
    char level_str[10];
    FILE *fp = popen("./pieces/plugins/+x/piece_manager.+x fuzzpet get-state level", "r");
    if (fp) {
        fgets(level_str, sizeof(level_str), fp);
        pclose(fp);
    }
    
    int level = atoi(level_str);
    int new_level = (level < 5) ? level + 1 : level;  // Max level 5
    
    if (new_level > level) {
        // Update the level
        char cmd[200];
        sprintf(cmd, "./pieces/plugins/+x/piece_manager.+x fuzzpet set-state level %d", new_level);
        system(cmd);
        
        printf("[EVOLVE] Level up! Now level %d\r", new_level);
    } else {
        printf("[EVOLVE] FuzzPet reached max level!\r");
    }
    
    return 0;
}