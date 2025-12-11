#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

#define MAX_WORD_LENGTH 50
#define MAX_VOCAB_LINES 200
#define MAX_GUESSED_LETTERS 26
#define MAX_INCORRECT_GUESSES 6
#define VAR_BUFFER_SIZE 4096

// Game state variables
char current_word[MAX_WORD_LENGTH];
char display_word[MAX_WORD_LENGTH];
char guessed_letters[MAX_GUESSED_LETTERS];
char incorrect_letters[MAX_GUESSED_LETTERS];
int incorrect_count = 0;
int game_status = 0; // 0 = playing, 1 = won, 2 = lost
char game_status_text[50] = "Playing";
int guesses_left = MAX_INCORRECT_GUESSES;
char vocab_filename[256] = "modules/hangman/hangman_data/hangman_chat]vocab.txt";
char message[256] = "Guess a letter!";

// Buffer for batching variable updates
char var_buffer[VAR_BUFFER_SIZE];
int buf_pos = 0;

// Macro for adding variables to the buffer
#define ADD_VAR_INT(name, value) \
    buf_pos += snprintf(var_buffer + buf_pos, sizeof(var_buffer) - buf_pos, \
    "%s=%d,", #name, value)

#define ADD_VAR_CHAR(name, value) \
    buf_pos += snprintf(var_buffer + buf_pos, sizeof(var_buffer) - buf_pos, \
    "%s=%c,", #name, value)

#define ADD_VAR_STR(name, value) \
    buf_pos += snprintf(var_buffer + buf_pos, sizeof(var_buffer) - buf_pos, \
    "%s=%s,", #name, value)

// Function declarations
void init_game();
void load_vocabulary();
void select_random_word();
void update_display_word();
bool process_guess(char letter);
void reset_game();
void render_hangman();

// Initialize the game state
void init_game() {
    strcpy(current_word, "");
    strcpy(display_word, "");
    strcpy(guessed_letters, "");
    strcpy(incorrect_letters, "");
    incorrect_count = 0;
    game_status = 0;
    guesses_left = MAX_INCORRECT_GUESSES;
    strcpy(game_status_text, "Playing");
    strcpy(message, "Guess a letter!");
}

// Load vocabulary from file
void load_vocabulary(char *words[], int *word_count) {
    FILE *file = fopen(vocab_filename, "r");
    if (file == NULL) {
        strcpy(message, "Error: Could not load vocabulary file!");
        return;
    }
    
    char line[MAX_WORD_LENGTH];
    *word_count = 0;
    
    while (fgets(line, sizeof(line), file) && *word_count < MAX_VOCAB_LINES) {
        // Remove newline character
        line[strcspn(line, "\n")] = 0;
        
        // Only add non-empty lines
        if (strlen(line) > 0) {
            words[*word_count] = malloc(strlen(line) + 1);
            strcpy(words[*word_count], line);
            (*word_count)++;
        }
    }
    
    fclose(file);
}

// Select a random word from the vocabulary
void select_random_word(char *words[], int word_count) {
    if (word_count <= 0) {
        strcpy(current_word, "ERROR");
        strcpy(display_word, "E R R O R");
        return;
    }
    
    srand(time(NULL) + getpid());  // Seed with time and process ID for randomness
    int random_index = rand() % word_count;
    strcpy(current_word, words[random_index]);
    
    // Initialize display word with underscores
    int len = strlen(current_word);
    strcpy(display_word, "");
    for (int i = 0; i < len; i++) {
        strcat(display_word, "_ ");
    }
    
    // Remove trailing space for last character
    if (len > 0) {
        display_word[len * 2 - 1] = '\0';
    }
}

// Update the display word with correctly guessed letters
void update_display_word() {
    int word_len = strlen(current_word);
    strcpy(display_word, "");
    
    for (int i = 0; i < word_len; i++) {
        bool found = false;
        for (int j = 0; j < strlen(guessed_letters); j++) {
            if (tolower(current_word[i]) == tolower(guessed_letters[j])) {
                found = true;
                break;
            }
        }
        
        if (found) {
            strncat(display_word, &current_word[i], 1);
            strcat(display_word, " ");
        } else {
            strcat(display_word, "_ ");
        }
    }
    
    // Remove trailing space for last character
    if (word_len > 0) {
        display_word[word_len * 2 - 1] = '\0';
    }
}

// Process a letter guess
bool process_guess(char letter) {
    // Check if letter was already guessed
    for (int i = 0; i < strlen(guessed_letters); i++) {
        if (tolower(guessed_letters[i]) == tolower(letter)) {
            strcpy(message, "Letter already guessed!");
            return false;
        }
    }
    
    // Add letter to guessed letters
    int pos = strlen(guessed_letters);
    guessed_letters[pos] = letter;
    guessed_letters[pos + 1] = '\0';
    
    // Check if letter is in the word
    bool correct = false;
    for (int i = 0; i < strlen(current_word); i++) {
        if (tolower(current_word[i]) == tolower(letter)) {
            correct = true;
        }
    }
    
    if (!correct) {
        // Add to incorrect letters
        pos = strlen(incorrect_letters);
        incorrect_letters[pos] = letter;
        incorrect_letters[pos + 1] = '\0';
        incorrect_count++;
        guesses_left = MAX_INCORRECT_GUESSES - incorrect_count;
        
        if (incorrect_count >= MAX_INCORRECT_GUESSES) {
            game_status = 2; // Lost
            strcpy(game_status_text, "Lost");
            char temp_msg[256];
            snprintf(temp_msg, sizeof(temp_msg), "Game Over! The word was: %s", current_word);
            strcpy(message, temp_msg);
        } else {
            char temp_msg[256];
            snprintf(temp_msg, sizeof(temp_msg), "Wrong guess! %d attempts left.", guesses_left);
            strcpy(message, temp_msg);
        }
    } else {
        // Update display word
        update_display_word();
        
        // Check if word is completely guessed
        bool word_complete = true;
        for (int i = 0; i < strlen(display_word); i += 2) { // Check every other character (the letters)
            if (display_word[i] == '_') {
                word_complete = false;
                break;
            }
        }
        
        if (word_complete) {
            game_status = 1; // Won
            strcpy(game_status_text, "Won");
            strcpy(message, "Congratulations! You won!");
        } else {
            char temp_msg[256];
            snprintf(temp_msg, sizeof(temp_msg), "Correct! Letter '%c' is in the word.", letter);
            strcpy(message, temp_msg);
        }
    }
    
    return true;
}

// Reset the game
void reset_game() {
    init_game();
    
    // Load vocabulary and select a new word
    char *words[MAX_VOCAB_LINES];
    int word_count = 0;
    
    load_vocabulary(words, &word_count);
    select_random_word(words, word_count);
    
    // Free allocated memory
    for (int i = 0; i < word_count; i++) {
        free(words[i]);
    }
}

// Render hangman based on incorrect guesses
void render_hangman() {
    // The hangman visual representation will be sent as output
    // Each call will print the appropriate hangman stage based on incorrect_count
    
    // Print empty lines as placeholders for hangman
    for (int i = 0; i < 7; i++) {
        // Print hangman based on incorrect_count
        switch (incorrect_count) {
            case 0:
                if (i == 0) printf("   +---+\n");
                else if (i == 1) printf("   |   |\n");
                else if (i == 2) printf("       |\n");
                else if (i == 3) printf("       |\n");
                else if (i == 4) printf("       |\n");
                else if (i == 5) printf("       |\n");
                else if (i == 6) printf("=========\n");
                break;
            case 1:
                if (i == 0) printf("   +---+\n");
                else if (i == 1) printf("   |   |\n");
                else if (i == 2) printf("   O   |\n");
                else if (i == 3) printf("       |\n");
                else if (i == 4) printf("       |\n");
                else if (i == 5) printf("       |\n");
                else if (i == 6) printf("=========\n");
                break;
            case 2:
                if (i == 0) printf("   +---+\n");
                else if (i == 1) printf("   |   |\n");
                else if (i == 2) printf("   O   |\n");
                else if (i == 3) printf("   |   |\n");
                else if (i == 4) printf("       |\n");
                else if (i == 5) printf("       |\n");
                else if (i == 6) printf("=========\n");
                break;
            case 3:
                if (i == 0) printf("   +---+\n");
                else if (i == 1) printf("   |   |\n");
                else if (i == 2) printf("   O   |\n");
                else if (i == 3) printf("  /|   |\n");
                else if (i == 4) printf("       |\n");
                else if (i == 5) printf("       |\n");
                else if (i == 6) printf("=========\n");
                break;
            case 4:
                if (i == 0) printf("   +---+\n");
                else if (i == 1) printf("   |   |\n");
                else if (i == 2) printf("   O   |\n");
                else if (i == 3) printf("  /|\\  |\n");
                else if (i == 4) printf("       |\n");
                else if (i == 5) printf("       |\n");
                else if (i == 6) printf("=========\n");
                break;
            case 5:
                if (i == 0) printf("   +---+\n");
                else if (i == 1) printf("   |   |\n");
                else if (i == 2) printf("   O   |\n");
                else if (i == 3) printf("  /|\\  |\n");
                else if (i == 4) printf("  /    |\n");
                else if (i == 5) printf("       |\n");
                else if (i == 6) printf("=========\n");
                break;
            case 6:
                if (i == 0) printf("   +---+\n");
                else if (i == 1) printf("   |   |\n");
                else if (i == 2) printf("   O   |\n");
                else if (i == 3) printf("  /|\\  |\n");
                else if (i == 4) printf("  / \\  |\n");
                else if (i == 5) printf("       |\n");
                else if (i == 6) printf("=========\n");
                break;
            default:
                if (i == 0) printf("   +---+\n");
                else if (i == 1) printf("   |   |\n");
                else if (i == 2) printf("   O   |\n");
                else if (i == 3) printf("  /|\\  |\n");
                else if (i == 4) printf("  / \\  |\n");
                else if (i == 5) printf("       |\n");
                else if (i == 6) printf("=========\n");
                break;
        }
    }
}

int main() {
    // Initialize the game
    init_game();
    
    // Load vocabulary and select a random word
    char *words[MAX_VOCAB_LINES];
    int word_count = 0;
    
    load_vocabulary(words, &word_count);
    select_random_word(words, word_count);
    
    // Free allocated memory
    for (int i = 0; i < word_count; i++) {
        free(words[i]);
    }
    
    // Send initial game state
    buf_pos = 0;
    ADD_VAR_INT(incorrect_count, incorrect_count);
    ADD_VAR_INT(game_status, game_status);
    ADD_VAR_INT(guesses_left, guesses_left);
    ADD_VAR_STR(current_word, current_word);
    ADD_VAR_STR(display_word, display_word);
    ADD_VAR_STR(guessed_letters, guessed_letters);
    ADD_VAR_STR(incorrect_letters, incorrect_letters);
    ADD_VAR_STR(game_status_text, game_status_text);
    ADD_VAR_STR(message, message);
    
    printf("VARS:%s;\n", var_buffer);
    fflush(stdout);
    
    char input[256];
    while (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        
        if (strncmp(input, "DISPLAY", 7) == 0) {
            char canvas_id[256] = {0};
            char *element_id_part = strstr(input, ":ELEMENT_ID:");
            
            if (element_id_part != NULL) {
                char *id_start = element_id_part + 12;
                strncpy(canvas_id, id_start, sizeof(canvas_id) - 1);
                canvas_id[sizeof(canvas_id) - 1] = '\0';
            }
            
            if (strlen(canvas_id) > 0 && strcmp(canvas_id, "hangman_canvas") == 0) {
                // Render the hangman
                render_hangman();
                
                // Send variables
                buf_pos = 0;
                ADD_VAR_INT(incorrect_count, incorrect_count);
                ADD_VAR_INT(guesses_left, guesses_left);
                ADD_VAR_STR(message, message);
                ADD_VAR_STR(game_status_text, game_status_text);
                printf("VARS:%s;\n", var_buffer);
            } else {
                // Default display - just send variables
                buf_pos = 0;
                ADD_VAR_INT(incorrect_count, incorrect_count);
                ADD_VAR_INT(game_status, game_status);
                ADD_VAR_INT(guesses_left, guesses_left);
                ADD_VAR_STR(current_word, current_word);
                ADD_VAR_STR(display_word, display_word);
                ADD_VAR_STR(guessed_letters, guessed_letters);
                ADD_VAR_STR(incorrect_letters, incorrect_letters);
                ADD_VAR_STR(game_status_text, game_status_text);
                ADD_VAR_STR(message, message);
                printf("VARS:%s;\n", var_buffer);
            }
            fflush(stdout);
        }
        else if (strncmp(input, "GUESS:", 6) == 0) {
            char letter = input[6];
            if (isalpha(letter)) {
                letter = tolower(letter);
                if (game_status == 0) { // Only process if game is still playing
                    process_guess(letter);
                } else {
                    strcpy(message, "Game over! Press R to restart.");
                }
                
                // Send updated game state
                buf_pos = 0;
                ADD_VAR_INT(incorrect_count, incorrect_count);
                ADD_VAR_INT(game_status, game_status);
                ADD_VAR_INT(guesses_left, guesses_left);
                ADD_VAR_STR(current_word, current_word);
                ADD_VAR_STR(display_word, display_word);
                ADD_VAR_STR(guessed_letters, guessed_letters);
                ADD_VAR_STR(incorrect_letters, incorrect_letters);
                ADD_VAR_STR(game_status_text, game_status_text);
                ADD_VAR_STR(message, message);
                printf("VARS:%s;\n", var_buffer);
                fflush(stdout);
            }
        }
        else if (strcmp(input, "RESTART") == 0) {
            reset_game();
            
            // Send updated game state
            buf_pos = 0;
            ADD_VAR_INT(incorrect_count, incorrect_count);
            ADD_VAR_INT(game_status, game_status);
            ADD_VAR_INT(guesses_left, guesses_left);
            ADD_VAR_STR(current_word, current_word);
            ADD_VAR_STR(display_word, display_word);
            ADD_VAR_STR(guessed_letters, guessed_letters);
            ADD_VAR_STR(incorrect_letters, incorrect_letters);
            ADD_VAR_STR(game_status_text, game_status_text);
            ADD_VAR_STR(message, message);
            printf("VARS:%s;\n", var_buffer);
            fflush(stdout);
        }
        else if (strncmp(input, "CLI_INPUT:", 10) == 0) {
            char *command_text = input + 10;
            
            // Process single letter guesses
            if (strlen(command_text) == 1 && isalpha(command_text[0])) {
                char letter = tolower(command_text[0]);
                if (game_status == 0) { // Only process if game is still playing
                    process_guess(letter);
                } else {
                    strcpy(message, "Game over! Type 'restart' to play again.");
                }
                
                // Send updated game state
                buf_pos = 0;
                ADD_VAR_INT(incorrect_count, incorrect_count);
                ADD_VAR_INT(game_status, game_status);
                ADD_VAR_INT(guesses_left, guesses_left);
                ADD_VAR_STR(current_word, current_word);
                ADD_VAR_STR(display_word, display_word);
                ADD_VAR_STR(guessed_letters, guessed_letters);
                ADD_VAR_STR(incorrect_letters, incorrect_letters);
                ADD_VAR_STR(game_status_text, game_status_text);
                ADD_VAR_STR(message, message);
                printf("VARS:%s;\n", var_buffer);
            }
            else if (strcmp(command_text, "restart") == 0 || strcmp(command_text, "r") == 0) {
                reset_game();
                
                // Send updated game state
                buf_pos = 0;
                ADD_VAR_INT(incorrect_count, incorrect_count);
                ADD_VAR_INT(game_status, game_status);
                ADD_VAR_INT(guesses_left, guesses_left);
                ADD_VAR_STR(current_word, current_word);
                ADD_VAR_STR(display_word, display_word);
                ADD_VAR_STR(guessed_letters, guessed_letters);
                ADD_VAR_STR(incorrect_letters, incorrect_letters);
                ADD_VAR_STR(game_status_text, game_status_text);
                ADD_VAR_STR(message, message);
                printf("VARS:%s;\n", var_buffer);
            }
            else {
                strcpy(message, "Enter a single letter to guess, or 'restart' to play again.");
                
                // Send updated message
                buf_pos = 0;
                ADD_VAR_INT(incorrect_count, incorrect_count);
                ADD_VAR_INT(game_status, game_status);
                ADD_VAR_INT(guesses_left, guesses_left);
                ADD_VAR_STR(current_word, current_word);
                ADD_VAR_STR(display_word, display_word);
                ADD_VAR_STR(guessed_letters, guessed_letters);
                ADD_VAR_STR(incorrect_letters, incorrect_letters);
                ADD_VAR_STR(game_status_text, game_status_text);
                ADD_VAR_STR(message, message);
                printf("VARS:%s;\n", var_buffer);
            }
            fflush(stdout);
        }
        else if (strcmp(input, "QUIT") == 0) {
            break;
        }
    }
    
    return 0;
}