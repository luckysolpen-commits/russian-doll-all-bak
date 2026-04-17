#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <math.h>

#define MAX_PATH 4096
#define MAX_LINE 1024
#define BOARD_SIZE 32

char project_root[MAX_PATH] = ".";
int board[BOARD_SIZE]; // 0=empty, 1=red, -1=black, 2=red king, -2=black king
int game_mode = 0; // Default to Human vs AI

void trim_str_inplace(char *str) {
    char *end;
    if (!str) return;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
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
}

void load_board_state() {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/projects/checkers/board_state.txt", project_root);
    FILE *f = fopen(path, "r");
    if (f) {
        for (int i = 0; i < BOARD_SIZE; i++) {
            if (fscanf(f, "%d", &board[i]) != 1) {
                // Default initialization if file is malformed or incomplete
                if (i < 12) board[i] = -1;
                else if (i < 20) board[i] = 0;
                else board[i] = 1;
            }
        }
        fclose(f);
    } else {
        // Initialize default board if file doesn't exist
        for (int i = 0; i < BOARD_SIZE; i++) {
            if (i < 12) board[i] = -1;
            else if (i < 20) board[i] = 0;
            else board[i] = 1;
        }
    }
}

void save_board_state() {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/projects/checkers/board_state.txt", project_root);
    FILE *f = fopen(path, "w");
    if (f) {
        for (int i = 0; i < BOARD_SIZE; i++) {
            fprintf(f, "%d ", board[i]);
            if ((i + 1) % 4 == 0) fprintf(f, "
");
        }
        fclose(f);
    }
}

void print_board() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i] == 1) printf("r ");
        else if (board[i] == -1) printf("b ");
        else if (board[i] == 2) printf("R ");
        else if (board[i] == -2) printf("B ");
        else printf(". ");
        if ((i + 1) % 4 == 0) printf("%2d
", i);
    }
    printf("
");
}

int get_player_input(int player, int *from, int *to) {
    printf("Player %s - Enter piece to move (0-31): ", player == 1 ? "Red" : "Black");
    if (scanf("%d", from) != 1) return 0;
    printf("Enter destination (0-31): ");
    if (scanf("%d", to) != 1) return 0;
    return 1;
}

int get_ai_move(int player, int *from, int *to) {
    // Simplified AI: iterates through pieces and finds the first valid move
    // A real RL AI would use weights and evaluation
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i] == player || board[i] == player * 2) { // If it's player's piece or king
            int moves[4] = {i - 4, i - 3, i + 3, i + 4}; // Possible moves
            for (int j = 0; j < 4; j++) {
                if (is_valid_move(i, moves[j], player)) {
                    *from = i;
                    *to = moves[j];
                    return 1; // Found a valid move
                }
            }
        }
    }
    return 0; // No valid moves found
}

void update_game_state() {
    int red_pieces = 0, black_pieces = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i] == 1 || board[i] == 2) red_pieces++;
        if (board[i] == -1 || board[i] == -2) black_pieces++;
    }

    // Check for game end condition
    if (red_pieces == 0 || !has_captures(1)) { // Black wins if Red has no pieces or no captures available
        printf("Game Over! Black wins!
");
        // In a real game, update weights based on winner/loser
        exit(0); 
    }
    if (black_pieces == 0 || !has_captures(-1)) { // Red wins if Black has no pieces or no captures available
        printf("Game Over! Red wins!
");
        // In a real game, update weights based on winner/loser
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    resolve_paths();
    
    if (argc > 1) { // Load game mode from args if provided
        game_mode = atoi(argv[1]);
    } else {
        printf("Select mode: 0 (Human vs AI) or 1 (AI vs AI): ");
        scanf("%d", &game_mode);
    }
    
    load_weights(1, NULL); // Load red weights (player 1)
    load_weights(-1, NULL); // Load black weights (player -1)
    
    printf("Initial weights:
");
    print_weights(weights_red, "Red");
    print_weights(weights_black, "Black");
    
    init_game();
    
    int current_player = 1; // 1 for Red, -1 for Black
    while (1) {
        print_board();
        
        int from, to, move_made = 0;
        if (game_mode == 0 && current_player == 1) { // Human's turn (Red)
            move_made = get_human_move(current_player, &from, &to);
        } else { // AI's turn (Black human or AI, or Red AI)
            float *weights = (current_player == 1) ? weights_red : weights_black;
            move_made = get_ai_move(current_player, &from, &to);
            if (move_made) printf("AI (%s) moves %d to %d
", current_player == 1 ? "Red" : "Black", from, to);
        }
        
        if (move_made && is_valid_move(from, to, current_player)) {
            make_move(from, to, current_player);
            update_game_state(); // Check for win condition after move
            current_player = -current_player; // Switch player
        } else if (!move_made) {
            printf("No valid moves found! %s wins!
", current_player == 1 ? "Black" : "Red");
            update_weights(current_player == 1 ? -1 : 1, current_player == 1 ? weights_black : weights_red);
            save_weights(current_player);
            if (game_mode == 1) save_weights(-current_player);
            break;
        } else {
            printf("Invalid move attempted: %d to %d
", from, to);
            if (game_mode == 0 && current_player == 1) {
                printf("Try again.
"); // Let human retry
            } else {
                current_player = -current_player; // Switch player even if AI move was invalid? (Could be improved)
            }
        }
    }
    return 0;
}

void init_game() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (i < 12) board[i] = -1;        // Black pieces
        else if (i < 20) board[i] = 0;     // Empty middle
        else board[i] = 1;                 // Red pieces
    }
}

void print_board() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i] == 1) printf("r ");
        else if (board[i] == -1) printf("b ");
        else if (board[i] == 2) printf("R ");
        else if (board[i] == -2) printf("B ");
        else printf(". ");
        if ((i + 1) % 4 == 0) printf("%2d
", i);
    }
    printf("
");
}

int get_human_move(int player, int *from, int *to) {
    printf("Player %s - Enter piece to move (0-31): ", player == 1 ? "Red" : "Black");
    if (scanf("%d", from) != 1) return 0;
    printf("Enter destination (0-31): ");
    if (scanf("%d", to) != 1) return 0;
    return 1;
}

int get_ai_move(int player, int *from, int *to) {
    float best_score = -1000.0;
    int best_from = -1, best_to = -1;
    
    float *weights = (player == 1) ? weights_red : weights_black;

    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i] != player && board[i] != player * 2) continue;
        
        int moves[4] = {i-4, i-3, i+3, i+4}; // Potential moves
        for (int j = 0; j < 4; j++) {
            if (moves[j] >= 0 && moves[j] < BOARD_SIZE) {
                if (is_valid_move(i, moves[j], player)) {
                    float score = weights[moves[j]] + weights[WEIGHT_COUNT-1]; // Simple evaluation: destination weight + bias
                    if (score > best_score) {
                        best_score = score;
                        best_from = i;
                        best_to = moves[j];
                    }
                }
            }
        }
    }
    
    if (best_from == -1) return 0; // No valid moves found
    *from = best_from;
    *to = best_to;
    return 1;
}

void save_weights(int player) {
    char filename[20];
    sprintf(filename, "weights_%s.dat", player == 1 ? "red" : "black");
    FILE *fp = fopen(filename, "wb");
    if (fp) {
        float *weights = (player == 1) ? weights_red : weights_black;
        fwrite(weights, sizeof(float), WEIGHT_COUNT, fp);
        fclose(fp);
    }
}

void load_weights(int player, float *weights) {
    char filename[20];
    sprintf(filename, "weights_%s.dat", player == 1 ? "red" : "black");
    FILE *fp = fopen(filename, "rb");
    if (fp) {
        fread(weights, sizeof(float), WEIGHT_COUNT, fp);
        fclose(fp);
    } else {
        // Initialize weights if file doesn't exist
        for (int i = 0; i < WEIGHT_COUNT; i++) {
            weights[i] = (float)(rand() % 100) / 100.0; // Random initialization
        }
    }
}

void update_weights(int winner, float *weights) {
    float learning_rate = 0.1;
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i] == winner || board[i] == winner * 2) { // If piece belongs to winner
            weights[i] += learning_rate;
        }
    }
    weights[WEIGHT_COUNT-1] += learning_rate * winner; // Update bias
}

void print_weights(float *weights, const char *player) {
    printf("%s weights: ", player);
    for (int i = 0; i < 5; i++) {  // Print first 5 for brevity
        printf("%.2f ", weights[i]);
    }
    printf("... bias: %.2f
", weights[WEIGHT_COUNT-1]);
}
