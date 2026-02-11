#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // Use piece_manager to get current state and update energy and happiness
    char energy_str[10];
    FILE *fp = popen("./pieces/plugins/+x/piece_manager.+x fuzzpet get-state energy", "r");
    if (fp) {
        fgets(energy_str, sizeof(energy_str), fp);
        pclose(fp);
    }
    
    int energy = atoi(energy_str);
    int new_energy = (energy > 30) ? energy - 30 : 0;
    
    // Increase happiness
    char happiness_str[10];
    fp = popen("./pieces/plugins/+x/piece_manager.+x fuzzpet get-state happiness", "r");
    if (fp) {
        fgets(happiness_str, sizeof(happiness_str), fp);
        pclose(fp);
    }
    
    int happiness = atoi(happiness_str);
    int new_happiness = (happiness < 70) ? happiness + 30 : 100;
    
    // Update the state values
    char cmd[200];
    sprintf(cmd, "./pieces/plugins/+x/piece_manager.+x fuzzpet set-state energy %d", new_energy);
    system(cmd);
    
    sprintf(cmd, "./pieces/plugins/+x/piece_manager.+x fuzzpet set-state happiness %d", new_happiness);
    system(cmd);
    
    return 0;
}