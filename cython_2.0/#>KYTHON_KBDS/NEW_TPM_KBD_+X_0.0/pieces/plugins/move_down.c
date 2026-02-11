#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // Get current position
    char pos_x_str[10], pos_y_str[10];
    
    FILE *fp = popen("./pieces/plugins/+x/piece_manager.+x mover get-state position_x", "r");
    if (fp) {
        fgets(pos_x_str, sizeof(pos_x_str), fp);
        pos_x_str[strcspn(pos_x_str, "\n")] = '\0';
        pclose(fp);
    }
    
    fp = popen("./pieces/plugins/+x/piece_manager.+x mover get-state position_y", "r");
    if (fp) {
        fgets(pos_y_str, sizeof(pos_y_str), fp);
        pos_y_str[strcspn(pos_y_str, "\n")] = '\0';
        pclose(fp);
    }
    
    int pos_x = atoi(pos_x_str);
    int pos_y = atoi(pos_y_str);
    
    // Move down (increase y)
    int new_y = pos_y + 1;
    
    // Update mover piece
    char cmd[200];
    sprintf(cmd, "./pieces/plugins/+x/piece_manager.+x mover set-state position_y %d", new_y);
    system(cmd);
    
    sprintf(cmd, "./pieces/plugins/+x/piece_manager.+x mover set-state last_direction down");
    system(cmd);
    
    // Log to master ledger
    sprintf(cmd, "./pieces/plugins/+x/log_event.+x MovementEvent \"down | From: (%d,%d) To: (%d,%d) | Target: mover\"", pos_x, pos_y, pos_x, new_y);
    system(cmd);
    
    printf("Moved down from (%d,%d) to (%d,%d)\n", pos_x, pos_y, pos_x, new_y);
    
    return 0;
}