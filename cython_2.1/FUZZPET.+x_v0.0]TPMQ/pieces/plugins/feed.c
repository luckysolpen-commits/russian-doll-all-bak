#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // Use piece_manager to get current state and update hunger
    char hunger_str[10];
    FILE *fp = popen("./pieces/plugins/+x/piece_manager.+x fuzzpet get-state hunger", "r");
    if (fp) {
        fgets(hunger_str, sizeof(hunger_str), fp);
        pclose(fp);
    }
    
    int hunger = atoi(hunger_str);
    int new_hunger = (hunger > 20) ? hunger - 20 : 0;
    
    // Also increase happiness
    char happiness_str[10];
    fp = popen("./pieces/plugins/+x/piece_manager.+x fuzzpet get-state happiness", "r");
    if (fp) {
        fgets(happiness_str, sizeof(happiness_str), fp);
        pclose(fp);
    }
    
    int happiness = atoi(happiness_str);
    int new_happiness = (happiness < 80) ? happiness + 20 : 100;
    
    // Update the state values
    char cmd[200];
    sprintf(cmd, "./pieces/plugins/+x/piece_manager.+x fuzzpet set-state hunger %d", new_hunger);
    system(cmd);
    
    sprintf(cmd, "./pieces/plugins/+x/piece_manager.+x fuzzpet set-state happiness %d", new_happiness);
    system(cmd);
    
    return 0;
}