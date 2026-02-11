#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // Use piece_manager to get current state and update energy
    char energy_str[10];
    FILE *fp = popen("./pieces/plugins/+x/piece_manager.+x fuzzpet get-state energy", "r");
    if (fp) {
        fgets(energy_str, sizeof(energy_str), fp);
        pclose(fp);
    }
    
    int energy = atoi(energy_str);
    int new_energy = (energy < 92) ? energy + 8 : 100;
    
    // Update the state value
    char cmd[200];
    sprintf(cmd, "./pieces/plugins/+x/piece_manager.+x fuzzpet set-state energy %d", new_energy);
    system(cmd);
    
    return 0;
}