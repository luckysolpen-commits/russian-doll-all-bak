#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // Use piece_manager to get current state values
    char name_str[50];
    char hunger_str[10];
    char happiness_str[10];
    char energy_str[10];
    char level_str[10];
    
    FILE *fp;
    
    fp = popen("./pieces/plugins/+x/piece_manager.+x fuzzpet get-state name", "r");
    if (fp) {
        fgets(name_str, sizeof(name_str), fp);
        pclose(fp);
    }
    
    fp = popen("./pieces/plugins/+x/piece_manager.+x fuzzpet get-state hunger", "r");
    if (fp) {
        fgets(hunger_str, sizeof(hunger_str), fp);
        pclose(fp);
    }
    
    fp = popen("./pieces/plugins/+x/piece_manager.+x fuzzpet get-state happiness", "r");
    if (fp) {
        fgets(happiness_str, sizeof(happiness_str), fp);
        pclose(fp);
    }
    
    fp = popen("./pieces/plugins/+x/piece_manager.+x fuzzpet get-state energy", "r");
    if (fp) {
        fgets(energy_str, sizeof(energy_str), fp);
        pclose(fp);
    }
    
    fp = popen("./pieces/plugins/+x/piece_manager.+x fuzzpet get-state level", "r");
    if (fp) {
        fgets(level_str, sizeof(level_str), fp);
        pclose(fp);
    }
    
    printf("\n=== %s Status ===\n", name_str[0] != '\0' ? name_str : "Pet");
    printf("Hunger:    %s\n", hunger_str[0] != '\0' ? hunger_str : "0");
    printf("Happiness: %s\n", happiness_str[0] != '\0' ? happiness_str : "0");
    printf("Energy:    %s\n", energy_str[0] != '\0' ? energy_str : "0");
    printf("Level:     %s\n", level_str[0] != '\0' ? level_str : "1");
    
    return 0;
}