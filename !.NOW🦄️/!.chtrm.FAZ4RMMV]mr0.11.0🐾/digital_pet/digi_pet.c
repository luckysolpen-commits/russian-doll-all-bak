#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#define MAX_PET_NAME 50
#define VAR_BUFFER_SIZE 4096

// Global variables for tracking state
int g_sig_received = 0;
int g_sig_num = 0;

// Pet state variables
char pet_name[MAX_PET_NAME];
int pet_age = 0;              // Age in days/turns
int pet_hunger = 30;          // Hunger level (0-100, 100 = starving)
int pet_energy = 80;          // Energy level (0-100, 100 = fully rested)
int pet_happiness = 70;       // Happiness level (0-100, 100 = very happy)
int pet_level = 1;            // Current level (1-10 initially)
int pet_xp = 0;              // Current XP points (0-100 for next level)
int pet_health = 100;         // Health level (0-100, 100 = full health)
int pet_state = 0;            // Current state (0=awake, 1=sleeping, 2=eating, 3=playing, 4=dead)
int turn_count = 0;           // Number of turns taken
char pet_message[256] = "Your new pet has arrived! Take good care of it!";

// Buffer for batching variable updates
char var_buffer[VAR_BUFFER_SIZE];
int buf_pos = 0;

// Macro for adding variables to the buffer
#define ADD_VAR_INT(name, value) \
    buf_pos += snprintf(var_buffer + buf_pos, sizeof(var_buffer) - buf_pos, \
    "%s=%d,", #name, value)

#define ADD_VAR_STR(name, value) \
    buf_pos += snprintf(var_buffer + buf_pos, sizeof(var_buffer) - buf_pos, \
    "%s=%s,", #name, value)

// Function declarations
void init_pet();
void save_pet_state();
bool load_pet_state();
void age_pet();
void update_pet_needs();
const char* get_pet_state_text();
const char* get_pet_status_emoji();
void feed_pet();
void train_pet();
void put_pet_to_sleep();
void wake_pet_up();
void play_with_pet();
void heal_pet();

// Initialize the pet
void init_pet() {
    strcpy(pet_name, "Fluffy");
    pet_age = 0;
    pet_hunger = 30;
    pet_energy = 80;
    pet_happiness = 70;
    pet_level = 1;
    pet_xp = 0;
    pet_health = 100;
    pet_state = 0; // awake
    turn_count = 0;
    strcpy(pet_message, "Your new pet has arrived! Take good care of it!");
}

// Save pet state to file
void save_pet_state() {
    // Create data directory if it doesn't exist
    mkdir("digital_pet/data", 0755);
    
    FILE* state_file = fopen("digital_pet/data/pet_state.txt", "w");
    if (state_file != NULL) {
        fprintf(state_file, "PET_NAME:%s\n", pet_name);
        fprintf(state_file, "PET_AGE:%d\n", pet_age);
        fprintf(state_file, "PET_HUNGER:%d\n", pet_hunger);
        fprintf(state_file, "PET_ENERGY:%d\n", pet_energy);
        fprintf(state_file, "PET_HAPPINESS:%d\n", pet_happiness);
        fprintf(state_file, "PET_LEVEL:%d\n", pet_level);
        fprintf(state_file, "PET_XP:%d\n", pet_xp);
        fprintf(state_file, "PET_HEALTH:%d\n", pet_health);
        fprintf(state_file, "PET_STATE:%d\n", pet_state);
        fprintf(state_file, "TURN_COUNT:%d\n", turn_count);
        
        fclose(state_file);
        printf("Pet state saved to digital_pet/data/pet_state.txt\n");
    } else {
        printf("Error: Could not save pet state to digital_pet/data/pet_state.txt\n");
    }
}

// Load pet state from file
bool load_pet_state() {
    FILE* state_file = fopen("digital_pet/data/pet_state.txt", "r");
    if (state_file == NULL) {
        // No state file exists, return false
        return false;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), state_file)) {
        // Remove newline character
        line[strcspn(line, "\n")] = 0;
        
        if (strncmp(line, "PET_NAME:", 9) == 0) {
            strncpy(pet_name, line + 9, MAX_PET_NAME - 1);
            pet_name[MAX_PET_NAME - 1] = '\0';
        } else if (strncmp(line, "PET_AGE:", 8) == 0) {
            pet_age = atoi(line + 8);
        } else if (strncmp(line, "PET_HUNGER:", 11) == 0) {
            pet_hunger = atoi(line + 11);
        } else if (strncmp(line, "PET_ENERGY:", 11) == 0) {
            pet_energy = atoi(line + 11);
        } else if (strncmp(line, "PET_HAPPINESS:", 14) == 0) {
            pet_happiness = atoi(line + 14);
        } else if (strncmp(line, "PET_LEVEL:", 10) == 0) {
            pet_level = atoi(line + 10);
        } else if (strncmp(line, "PET_XP:", 7) == 0) {
            pet_xp = atoi(line + 7);
        } else if (strncmp(line, "PET_HEALTH:", 11) == 0) {
            pet_health = atoi(line + 11);
        } else if (strncmp(line, "PET_STATE:", 10) == 0) {
            pet_state = atoi(line + 10);
        } else if (strncmp(line, "TURN_COUNT:", 11) == 0) {
            turn_count = atoi(line + 11);
        }
    }
    fclose(state_file);
    
    strcpy(pet_message, "Welcome back! Your pet is waiting for you!");
    return true;
}

// Age the pet and update needs over time
void age_pet() {
    pet_age++;
    turn_count++;
    
    // Random events based on age and stats
    srand(time(NULL) + getpid());
    int event = rand() % 100;
    
    if (event < 2) { // 2% chance of getting sick
        pet_health -= 10;
        if (pet_health < 0) pet_health = 0;
        strcpy(pet_message, "Oh no! Your pet got sick! Use HEAL action to help it recover.");
    }
}

// Update pet needs based on current state
void update_pet_needs() {
    // If pet is sleeping, restore energy
    if (pet_state == 1) { // sleeping
        pet_energy += 15;
        if (pet_energy > 100) pet_energy = 100;
        // Hunger still increases but at half rate when sleeping
        pet_hunger += 1;
    } else {
        // Normal energy drain when awake
        pet_energy -= 2;
        if (pet_energy < 0) pet_energy = 0;
        // Normal hunger increase
        pet_hunger += 2;
    }
    
    // Happiness decreases slowly over time
    pet_happiness -= 1;
    if (pet_happiness < 0) pet_happiness = 0;
    
    // Cap values
    if (pet_hunger > 100) pet_hunger = 100;
    if (pet_happiness > 100) pet_happiness = 100;
    
    // Check health effects of poor conditions
    if (pet_hunger > 90) {
        pet_health -= 2; // Starving damages health
        if (strcmp(pet_message, "Your pet is starving! Feed it immediately!") == 0) {
            // Don't overwrite if it's already a hunger message
        } else {
            strcpy(pet_message, "Your pet is starving! Feed it immediately!");
        }
    }
    
    if (pet_energy < 10) {
        pet_health -= 1; // Exhaustion damages health
        if (strcmp(pet_message, "Your pet is exhausted! Let it sleep!") == 0) {
            // Don't overwrite if it's already an energy message
        } else {
            strcpy(pet_message, "Your pet is exhausted! Let it sleep!");
        }
    }
    
    if (pet_happiness < 20) {
        if (strcmp(pet_message, "Your pet is very sad! Play with it to cheer it up!") == 0) {
            // Don't overwrite if it's already a happiness message
        } else {
            strcpy(pet_message, "Your pet is very sad! Play with it to cheer it up!");
        }
    }
    
    // Check if pet dies
    if (pet_health <= 0) {
        pet_health = 0;
        pet_state = 4; // dead
        strcpy(pet_message, "Your pet has passed away... You will remember the good times together.");
    }
    
    // Check if it's time to level up
    if (pet_xp >= 100 && pet_level < 10) {
        pet_level++;
        pet_xp = 0;
        pet_health = 100; // Fully heal when leveling up
        char level_msg[256];
        snprintf(level_msg, sizeof(level_msg), "Congratulations! Your pet reached level %d!", pet_level);
        strcpy(pet_message, level_msg);
    }
    
    // Cap health at 100
    if (pet_health > 100) pet_health = 100;
}

// Get string representation of pet state
const char* get_pet_state_text() {
    switch (pet_state) {
        case 0: return "Awake";
        case 1: return "Sleeping";
        case 2: return "Eating";
        case 3: return "Playing";
        case 4: return "Deceased";
        default: return "Unknown";
    }
}

// Get emoji representation of pet status
const char* get_pet_status_emoji() {
    if (pet_state == 4) return " RIP "; // Dead
    if (pet_state == 1) return " zzz "; // Sleeping
    if (pet_hunger > 80) return " 💀 "; // Near death from hunger
    if (pet_energy < 20) return " 😴 "; // Very tired
    if (pet_happiness < 20) return " 😞 "; // Sad
    if (pet_health < 30) return " 😷 "; // Sick
    if (pet_happiness > 70 && pet_health > 80 && pet_energy > 70) return " 😊 "; // Happy and healthy
    if (pet_health < 70) return " 🤒 "; // Somewhat sick
    return " 🐾 "; // Default
}

// Feed the pet
void feed_pet() {
    if (pet_state == 4) { // dead
        strcpy(pet_message, "You can't feed a deceased pet...");
        return;
    }
    
    if (pet_state == 1) { // sleeping
        strcpy(pet_message, "Your pet is sleeping. Wake it up first.");
        return;
    }
    
    if (pet_hunger < 10) {
        strcpy(pet_message, "Your pet is not hungry right now.");
        return;
    }
    
    // Reduce hunger significantly
    pet_hunger -= 40;
    if (pet_hunger < 0) pet_hunger = 0;
    
    // Slightly increase happiness
    pet_happiness += 5;
    if (pet_happiness > 100) pet_happiness = 100;
    
    // Set state to eating
    pet_state = 2; // eating
    strcpy(pet_message, "You fed your pet. It looks happy and satisfied!");
    
    // Age the pet after action
    age_pet();
}

// Train the pet
void train_pet() {
    if (pet_state == 4) { // dead
        strcpy(pet_message, "You can't train a deceased pet...");
        return;
    }
    
    if (pet_state == 1) { // sleeping
        strcpy(pet_message, "Your pet is sleeping. Wake it up first.");
        return;
    }
    
    if (pet_energy < 20) {
        strcpy(pet_message, "Your pet is too tired to train. Let it rest first.");
        return;
    }
    
    if (pet_happiness < 30) {
        strcpy(pet_message, "Your pet is too sad to focus on training. Play with it first.");
        return;
    }
    
    // Increase XP
    int xp_gain = 10 + (pet_level * 2); // Higher levels get less XP per action
    if (xp_gain > 20) xp_gain = 20; // Cap XP gain
    
    pet_xp += xp_gain;
    if (pet_xp > 100) pet_xp = 100;
    
    // Reduce energy
    pet_energy -= 15;
    if (pet_energy < 0) pet_energy = 0;
    
    // Increase happiness if successful
    if (rand() % 100 < 70) { // 70% success rate
        pet_happiness += 8;
        if (pet_happiness > 100) pet_happiness = 100;
        strcpy(pet_message, "Training was successful! Your pet gained experience.");
    } else {
        pet_happiness -= 5;
        if (pet_happiness < 0) pet_happiness = 0;
        strcpy(pet_message, "Training was difficult. Your pet is frustrated.");
    }
    
    // Age the pet after action
    age_pet();
}

// Put pet to sleep
void put_pet_to_sleep() {
    if (pet_state == 4) { // dead
        strcpy(pet_message, "A deceased pet doesn't need sleep...");
        return;
    }
    
    if (pet_state == 1) { // already sleeping
        strcpy(pet_message, "Your pet is already sleeping.");
        return;
    }
    
    pet_state = 1; // sleeping
    strcpy(pet_message, "Your pet is now sleeping and will restore energy.");
}

// Wake pet up
void wake_pet_up() {
    if (pet_state != 1) { // not sleeping
        strcpy(pet_message, "Your pet is not sleeping.");
        return;
    }
    
    pet_state = 0; // awake
    strcpy(pet_message, "Your pet woke up refreshed!");
}

// Play with the pet
void play_with_pet() {
    if (pet_state == 4) { // dead
        strcpy(pet_message, "You can't play with a deceased pet...");
        return;
    }
    
    if (pet_state == 1) { // sleeping
        strcpy(pet_message, "Your pet is sleeping. Wake it up first.");
        return;
    }
    
    if (pet_energy < 15) {
        strcpy(pet_message, "Your pet is too tired to play. Let it rest first.");
        return;
    }
    
    // Increase happiness significantly
    pet_happiness += 20;
    if (pet_happiness > 100) pet_happiness = 100;
    
    // Reduce energy
    pet_energy -= 10;
    if (pet_energy < 0) pet_energy = 0;
    
    // Slightly increase hunger
    pet_hunger += 5;
    if (pet_hunger > 100) pet_hunger = 100;
    
    strcpy(pet_message, "You played with your pet. It seems very happy!");
    
    // Age the pet after action
    age_pet();
}

// Heal the pet
void heal_pet() {
    if (pet_state == 4) { // dead
        strcpy(pet_message, "You can't heal a deceased pet...");
        return;
    }
    
    // Increase health
    pet_health += 30;
    if (pet_health > 100) pet_health = 100;
    
    // Also improves happiness
    pet_happiness += 10;
    if (pet_happiness > 100) pet_happiness = 100;
    
    strcpy(pet_message, "You helped heal your pet. It looks much better now!");
    
    // Age the pet after action
    age_pet();
}

int main() {
    // Initialize random seed
    srand(time(NULL));
    
    // Check for saved state and load if available
    bool state_loaded = load_pet_state();
    
    if (!state_loaded) {
        // No saved state, initialize with defaults
        init_pet();
    }
    
    char input[256];
    
    // Main loop to process commands from parent process (CHTM renderer)
    while (fgets(input, sizeof(input), stdin) && !g_sig_received) {
        // Remove newline character
        input[strcspn(input, "\n")] = 0;
        
        // Check if this is a DISPLAY command for potential UI refresh
        if (strncmp(input, "DISPLAY", 7) == 0) {  // Check if command starts with "DISPLAY"
            // This is a display request - send complete current state
            buf_pos = 0;
            ADD_VAR_INT(pet_age, pet_age);
            ADD_VAR_INT(pet_hunger, pet_hunger);
            ADD_VAR_INT(pet_energy, pet_energy);
            ADD_VAR_INT(pet_happiness, pet_happiness);
            ADD_VAR_INT(pet_level, pet_level);
            ADD_VAR_INT(pet_xp, pet_xp);
            ADD_VAR_INT(pet_health, pet_health);
            ADD_VAR_INT(pet_state, pet_state);
            ADD_VAR_INT(turn_count, turn_count);
            ADD_VAR_STR(pet_name, pet_name);
            ADD_VAR_STR(pet_message, pet_message);
            ADD_VAR_STR(current_state_text, get_pet_state_text());
            ADD_VAR_STR(status_emoji, get_pet_status_emoji());
            
            printf("VARS:%s;\n", var_buffer);
            fflush(stdout);
        }
        // Handle regular commands
        else if (strcmp(input, "FEED") == 0) {
            feed_pet();
            
            // Send updated variables
            buf_pos = 0;
            ADD_VAR_INT(pet_age, pet_age);
            ADD_VAR_INT(pet_hunger, pet_hunger);
            ADD_VAR_INT(pet_energy, pet_energy);
            ADD_VAR_INT(pet_happiness, pet_happiness);
            ADD_VAR_INT(pet_level, pet_level);
            ADD_VAR_INT(pet_xp, pet_xp);
            ADD_VAR_INT(pet_health, pet_health);
            ADD_VAR_INT(pet_state, pet_state);
            ADD_VAR_INT(turn_count, turn_count);
            ADD_VAR_STR(pet_name, pet_name);
            ADD_VAR_STR(pet_message, pet_message);
            ADD_VAR_STR(current_state_text, get_pet_state_text());
            ADD_VAR_STR(status_emoji, get_pet_status_emoji());
            
            printf("VARS:%s;\n", var_buffer);
            fflush(stdout);
        }
        else if (strcmp(input, "TRAIN") == 0) {
            train_pet();
            
            // Send updated variables
            buf_pos = 0;
            ADD_VAR_INT(pet_age, pet_age);
            ADD_VAR_INT(pet_hunger, pet_hunger);
            ADD_VAR_INT(pet_energy, pet_energy);
            ADD_VAR_INT(pet_happiness, pet_happiness);
            ADD_VAR_INT(pet_level, pet_level);
            ADD_VAR_INT(pet_xp, pet_xp);
            ADD_VAR_INT(pet_health, pet_health);
            ADD_VAR_INT(pet_state, pet_state);
            ADD_VAR_INT(turn_count, turn_count);
            ADD_VAR_STR(pet_name, pet_name);
            ADD_VAR_STR(pet_message, pet_message);
            ADD_VAR_STR(current_state_text, get_pet_state_text());
            ADD_VAR_STR(status_emoji, get_pet_status_emoji());
            
            printf("VARS:%s;\n", var_buffer);
            fflush(stdout);
        }
        else if (strcmp(input, "SLEEP") == 0) {
            put_pet_to_sleep();
            
            // Send updated variables
            buf_pos = 0;
            ADD_VAR_INT(pet_age, pet_age);
            ADD_VAR_INT(pet_hunger, pet_hunger);
            ADD_VAR_INT(pet_energy, pet_energy);
            ADD_VAR_INT(pet_happiness, pet_happiness);
            ADD_VAR_INT(pet_level, pet_level);
            ADD_VAR_INT(pet_xp, pet_xp);
            ADD_VAR_INT(pet_health, pet_health);
            ADD_VAR_INT(pet_state, pet_state);
            ADD_VAR_INT(turn_count, turn_count);
            ADD_VAR_STR(pet_name, pet_name);
            ADD_VAR_STR(pet_message, pet_message);
            ADD_VAR_STR(current_state_text, get_pet_state_text());
            ADD_VAR_STR(status_emoji, get_pet_status_emoji());
            
            printf("VARS:%s;\n", var_buffer);
            fflush(stdout);
        }
        else if (strcmp(input, "WAKE") == 0) {
            wake_pet_up();
            
            // Send updated variables
            buf_pos = 0;
            ADD_VAR_INT(pet_age, pet_age);
            ADD_VAR_INT(pet_hunger, pet_hunger);
            ADD_VAR_INT(pet_energy, pet_energy);
            ADD_VAR_INT(pet_happiness, pet_happiness);
            ADD_VAR_INT(pet_level, pet_level);
            ADD_VAR_INT(pet_xp, pet_xp);
            ADD_VAR_INT(pet_health, pet_health);
            ADD_VAR_INT(pet_state, pet_state);
            ADD_VAR_INT(turn_count, turn_count);
            ADD_VAR_STR(pet_name, pet_name);
            ADD_VAR_STR(pet_message, pet_message);
            ADD_VAR_STR(current_state_text, get_pet_state_text());
            ADD_VAR_STR(status_emoji, get_pet_status_emoji());
            
            printf("VARS:%s;\n", var_buffer);
            fflush(stdout);
        }
        else if (strcmp(input, "PLAY") == 0) {
            play_with_pet();
            
            // Send updated variables
            buf_pos = 0;
            ADD_VAR_INT(pet_age, pet_age);
            ADD_VAR_INT(pet_hunger, pet_hunger);
            ADD_VAR_INT(pet_energy, pet_energy);
            ADD_VAR_INT(pet_happiness, pet_happiness);
            ADD_VAR_INT(pet_level, pet_level);
            ADD_VAR_INT(pet_xp, pet_xp);
            ADD_VAR_INT(pet_health, pet_health);
            ADD_VAR_INT(pet_state, pet_state);
            ADD_VAR_INT(turn_count, turn_count);
            ADD_VAR_STR(pet_name, pet_name);
            ADD_VAR_STR(pet_message, pet_message);
            ADD_VAR_STR(current_state_text, get_pet_state_text());
            ADD_VAR_STR(status_emoji, get_pet_status_emoji());
            
            printf("VARS:%s;\n", var_buffer);
            fflush(stdout);
        }
        else if (strcmp(input, "HEAL") == 0) {
            heal_pet();
            
            // Send updated variables
            buf_pos = 0;
            ADD_VAR_INT(pet_age, pet_age);
            ADD_VAR_INT(pet_hunger, pet_hunger);
            ADD_VAR_INT(pet_energy, pet_energy);
            ADD_VAR_INT(pet_happiness, pet_happiness);
            ADD_VAR_INT(pet_level, pet_level);
            ADD_VAR_INT(pet_xp, pet_xp);
            ADD_VAR_INT(pet_health, pet_health);
            ADD_VAR_INT(pet_state, pet_state);
            ADD_VAR_INT(turn_count, turn_count);
            ADD_VAR_STR(pet_name, pet_name);
            ADD_VAR_STR(pet_message, pet_message);
            ADD_VAR_STR(current_state_text, get_pet_state_text());
            ADD_VAR_STR(status_emoji, get_pet_status_emoji());
            
            printf("VARS:%s;\n", var_buffer);
            fflush(stdout);
        }
        else if (strcmp(input, "PASS_TURN") == 0) {
            // Pass a turn without doing any action, just aging the pet and updating needs
            age_pet();
            
            // Send updated variables
            buf_pos = 0;
            ADD_VAR_INT(pet_age, pet_age);
            ADD_VAR_INT(pet_hunger, pet_hunger);
            ADD_VAR_INT(pet_energy, pet_energy);
            ADD_VAR_INT(pet_happiness, pet_happiness);
            ADD_VAR_INT(pet_level, pet_level);
            ADD_VAR_INT(pet_xp, pet_xp);
            ADD_VAR_INT(pet_health, pet_health);
            ADD_VAR_INT(pet_state, pet_state);
            ADD_VAR_INT(turn_count, turn_count);
            ADD_VAR_STR(pet_name, pet_name);
            ADD_VAR_STR(pet_message, pet_message);
            ADD_VAR_STR(current_state_text, get_pet_state_text());
            ADD_VAR_STR(status_emoji, get_pet_status_emoji());
            
            printf("VARS:%s;\n", var_buffer);
            fflush(stdout);
        }
        else if (strncmp(input, "CLI_INPUT:", 10) == 0) {
            char *command_text = input + 10;
            
            // Handle name change command
            if (strncmp(command_text, "name:", 5) == 0) {
                char *new_name = command_text + 5;
                if (strlen(new_name) > 0 && strlen(new_name) < MAX_PET_NAME) {
                    strncpy(pet_name, new_name, MAX_PET_NAME - 1);
                    pet_name[MAX_PET_NAME - 1] = '\0';
                    char msg[256];
                    snprintf(msg, sizeof(msg), "You renamed your pet to %s!", pet_name);
                    strcpy(pet_message, msg);
                } else {
                    strcpy(pet_message, "Invalid name. Please enter a name between 1 and 49 characters.");
                }
                
                // Send updated variables
                buf_pos = 0;
                ADD_VAR_INT(pet_age, pet_age);
                ADD_VAR_INT(pet_hunger, pet_hunger);
                ADD_VAR_INT(pet_energy, pet_energy);
                ADD_VAR_INT(pet_happiness, pet_happiness);
                ADD_VAR_INT(pet_level, pet_level);
                ADD_VAR_INT(pet_xp, pet_xp);
                ADD_VAR_INT(pet_health, pet_health);
                ADD_VAR_INT(pet_state, pet_state);
                ADD_VAR_INT(turn_count, turn_count);
                ADD_VAR_STR(pet_name, pet_name);
                ADD_VAR_STR(pet_message, pet_message);
                ADD_VAR_STR(current_state_text, get_pet_state_text());
                ADD_VAR_STR(status_emoji, get_pet_status_emoji());
                
                printf("VARS:%s;\n", var_buffer);
                fflush(stdout);
            }
            else {
                // Send current state
                buf_pos = 0;
                ADD_VAR_INT(pet_age, pet_age);
                ADD_VAR_INT(pet_hunger, pet_hunger);
                ADD_VAR_INT(pet_energy, pet_energy);
                ADD_VAR_INT(pet_happiness, pet_happiness);
                ADD_VAR_INT(pet_level, pet_level);
                ADD_VAR_INT(pet_xp, pet_xp);
                ADD_VAR_INT(pet_health, pet_health);
                ADD_VAR_INT(pet_state, pet_state);
                ADD_VAR_INT(turn_count, turn_count);
                ADD_VAR_STR(pet_name, pet_name);
                ADD_VAR_STR(pet_message, pet_message);
                ADD_VAR_STR(current_state_text, get_pet_state_text());
                ADD_VAR_STR(status_emoji, get_pet_status_emoji());
                
                printf("VARS:%s;\n", var_buffer);
                fflush(stdout);
            }
        }
        else if (strcmp(input, "QUIT") == 0) {
            break;
        }
        
        // Update pet needs based on current state (this happens each cycle)
        if (pet_state != 4) { // Only if pet is alive
            update_pet_needs();
        }
    }
    
    // Save state before exiting
    save_pet_state();
    
    return 0;
}