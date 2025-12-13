#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <libgen.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>

#define MAX_SEQUENCE_LENGTH 1000000

// Structure to hold a sequence window
struct SequenceWindow {
    struct VocabEntry *entries;
    int length;
    int start_pos;
};

#define EPSILON 1e-8
#define MAX_GRADIENT_NORM 1.0f

#define MAX_LINE_LENGTH 1024
#define MAX_VOCAB_SIZE 100000
#define EMBEDDING_DIM 7
#define HIDDEN_DIM 16

// Global variable to handle interruption
volatile sig_atomic_t should_exit = 0;

// Signal handlers for interrupts
void signal_handler(int sig) {
    fprintf(stderr, "\nReceived signal %d. Killing process group...\n", sig);
    // Kill the entire process group - same approach as SUP.c
    kill(0, SIGTERM);
    exit(0);
}

void force_exit_handler(int sig) {
    fprintf(stderr, "\nReceived force exit signal %d. Terminating immediately.\n", sig);
    _exit(1);  // Immediate termination without cleanup
}

// --- Configuration Structure ---
typedef struct {
    int epochs;
    float learning_rate;
    float beta1;
    float beta2;
    float max_gradient_norm;
    float attention_dropout;
    float mlp_dropout;
    float attention_noise;
    float attention_weights_noise;
    int causal_attention;
} Config;

// --- Configuration Functions ---
int load_config(Config *config, const char *config_file) {
    FILE *file = fopen(config_file, "r");
    if (!file) {
        fprintf(stderr, "Warning: Could not open config file %s, using defaults\n", config_file);
        // Set default values
        config->epochs = 10;
        config->learning_rate = 0.001f;
        config->beta1 = 0.9f;
        config->beta2 = 0.999f;
        config->max_gradient_norm = 1.0f;
        config->attention_dropout = 0.1f;
        config->mlp_dropout = 0.2f;
        config->attention_noise = 0.01f;
        config->attention_weights_noise = 0.005f;
        config->causal_attention = 0;
        return 0;
    }
    
    char line[1024];
    float value;
    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        
        // Parse key=value pairs
        char key[100];
        if (sscanf(line, "%99[^=]=%f", key, &value) == 2) {
            // Handle integer parameters
            if (strcmp(key, "epochs") == 0) {
                config->epochs = (int)value;
                fprintf(stderr, "Loaded epochs: %d\n", config->epochs);
            }
            else if (strcmp(key, "causal_attention") == 0) {
                config->causal_attention = (int)value;
            }
            // Handle float parameters
            else if (strcmp(key, "learning_rate") == 0) {
                config->learning_rate = value;
            }
            else if (strcmp(key, "beta1") == 0) {
                config->beta1 = value;
            }
            else if (strcmp(key, "beta2") == 0) {
                config->beta2 = value;
            }
            else if (strcmp(key, "max_gradient_norm") == 0) {
                config->max_gradient_norm = value;
            }
            else if (strcmp(key, "attention_dropout") == 0) {
                config->attention_dropout = value;
            }
            else if (strcmp(key, "mlp_dropout") == 0) {
                config->mlp_dropout = value;
            }
            else if (strcmp(key, "attention_noise") == 0) {
                config->attention_noise = value;
            }
            else if (strcmp(key, "attention_weights_noise") == 0) {
                config->attention_weights_noise = value;
            }
        }
    }
    
    fclose(file);
    return 1;
}

void print_config(Config *config) {
    fprintf(stderr, "Training configuration:\n");
    fprintf(stderr, "  Epochs: %d\n", config->epochs);
    fprintf(stderr, "  Learning rate: %f\n", config->learning_rate);
    fprintf(stderr, "  Beta1: %f\n", config->beta1);
    fprintf(stderr, "  Beta2: %f\n", config->beta2);
    fprintf(stderr, "  Max gradient norm: %f\n", config->max_gradient_norm);
    fprintf(stderr, "  Attention dropout: %f\n", config->attention_dropout);
    fprintf(stderr, "  MLP dropout: %f\n", config->mlp_dropout);
    fprintf(stderr, "  Attention noise: %f\n", config->attention_noise);
    fprintf(stderr, "  Attention weights noise: %f\n", config->attention_weights_noise);
    fprintf(stderr, "  Causal attention: %s\n", config->causal_attention ? "enabled" : "disabled");
}

// --- Data Structures ---
struct VocabEntry { int number; char word[100]; float embedding, pe, weight, bias1, bias2, bias3, bias4; };

// --- Utility Functions ---
void write_loss(float loss, const char *output_dir) { char loss_path[1024]; sprintf(loss_path, "%s/loss.txt", output_dir); FILE *file = fopen(loss_path, "a"); if (file) { fprintf(file, "%f\n", loss); fclose(file); } }
int save_vocab(struct VocabEntry *vocab, int vocab_size, const char *filename) { FILE *outfile = fopen(filename, "w"); if (!outfile) { perror("Failed to open output file"); return 0; } fprintf(outfile, "number word embedding pe weight bias1 bias2 bias3 bias4\n"); for (int i = 0; i < vocab_size; i++) { fprintf(outfile, "%d %s %f %f %f %f %f %f %f\n", vocab[i].number, vocab[i].word, vocab[i].embedding, vocab[i].pe, vocab[i].weight, vocab[i].bias1, vocab[i].bias2, vocab[i].bias3, vocab[i].bias4); } fclose(outfile); return 1; }
int load_vocab(struct VocabEntry *vocab, int max_vocab_size, const char *filename, int *vocab_size) { 
    FILE *infile = fopen(filename, "r"); 
    if (!infile) { 
        perror("Error opening vocab file"); 
        return 0; 
    } 
    char line[MAX_LINE_LENGTH]; 
    *vocab_size = 0; 
    fgets(line, sizeof(line), infile); 
    while (fgets(line, sizeof(line), infile) && *vocab_size < max_vocab_size) { 
        int result = sscanf(line, "%d %s %f %f %f %f %f %f %f", 
                          &vocab[*vocab_size].number, vocab[*vocab_size].word, 
                          &vocab[*vocab_size].embedding, &vocab[*vocab_size].pe, 
                          &vocab[*vocab_size].weight, &vocab[*vocab_size].bias1, 
                          &vocab[*vocab_size].bias2, &vocab[*vocab_size].bias3, 
                          &vocab[*vocab_size].bias4);
        if (result == 9) { // Successfully parsed all fields
            (*vocab_size)++;
        }
    } 
    fclose(infile); 
    return 1; 
}
float compute_cross_entropy_loss_and_gradient(float *predictions, int target_index, int size, float *grad) { 
    // Check for NaN values in predictions
    for (int i = 0; i < size; i++) {
        if (isnan(predictions[i]) || isinf(predictions[i])) {
            fprintf(stderr, "Warning: NaN or Inf detected in predictions at index %d\n", i);
            // Reset to small values
            predictions[i] = 0.0f;
        }
    }
    
    float *probs = malloc(size * sizeof(float)); 
    float max_val = predictions[0]; 
    for (int i = 1; i < size; i++) { 
        if (predictions[i] > max_val) max_val = predictions[i]; 
    } 
    float sum = 0.0f; 
    for (int i = 0; i < size; i++) { 
        probs[i] = expf(predictions[i] - max_val); 
        // Prevent overflow
        if (isinf(probs[i]) || isnan(probs[i])) {
            probs[i] = 0.0f;
        }
        sum += probs[i]; 
    } 
    // Handle case where sum is zero
    if (sum < 1e-12f) {
        for (int i = 0; i < size; i++) {
            probs[i] = 1.0f / size;
        }
        sum = 1.0f;
    }
    for (int i = 0; i < size; i++) { 
        probs[i] /= sum; 
        grad[i] = probs[i]; 
    } 
    grad[target_index] -= 1.0f; 
    float loss = -logf(probs[target_index] + EPSILON); 
    free(probs); 
    return loss; 
}

// --- Sequence Operations ---
// Function to load tokens from corpus file
int load_tokens_from_corpus(const char *corpus_file, char ***tokens, int *token_count) {
    FILE *cf = fopen(corpus_file, "r");
    if (!cf) {
        fprintf(stderr, "Error opening corpus file: %s\n", corpus_file);
        return 0;
    }
    
    // First pass: count total tokens
    char line[1024];
    int total_tokens = 0;
    
    while (fgets(line, sizeof(line), cf)) {
        char *line_copy = strdup(line);
        if (!line_copy) continue;
        
        char *token = strtok(line_copy, " \t\n\r");
        while (token != NULL) {
            total_tokens++;
            token = strtok(NULL, " \t\n\r");
        }
        free(line_copy);
    }
    
    // Allocate memory for tokens
    *tokens = malloc(total_tokens * sizeof(char*));
    if (!(*tokens)) {
        fprintf(stderr, "Failed to allocate memory for tokens\n");
        fclose(cf);
        return 0;
    }
    
    // Second pass: extract tokens
    *token_count = 0;
    rewind(cf);
    
    while (fgets(line, sizeof(line), cf) && *token_count < total_tokens) {
        char *line_copy = strdup(line);
        if (!line_copy) continue;
        
        char *token = strtok(line_copy, " \t\n\r");
        while (token != NULL && *token_count < total_tokens) {
            // Clean up the token (remove punctuation but keep internal apostrophes)
            char clean_token[100];
            int j = 0;
            for (int i = 0; token[i] != '\0'; i++) {
                if (isalnum(token[i]) || (token[i] == '\'' && i > 0 && i < strlen(token) - 1)) {
                    clean_token[j++] = token[i];
                }
            }
            clean_token[j] = '\0';
            
            if (strlen(clean_token) > 0) {
                (*tokens)[*token_count] = malloc(strlen(clean_token) + 1);
                strcpy((*tokens)[*token_count], clean_token);
                (*token_count)++;
            }
            
            token = strtok(NULL, " \t\n\r");
        }
        free(line_copy);
    }
    
    fclose(cf);
    return 1;
}

// Function to create sequence windows from tokens and vocab
int create_sequence_windows(struct VocabEntry *vocab, int vocab_size, 
                           char **tokens, int token_count, 
                           struct SequenceWindow **windows, int window_size) {
    if (token_count == 0 || window_size <= 0) {
        return 0;
    }
    
    // Calculate number of windows needed
    int num_windows = (token_count + window_size - 1) / window_size;  // Ceiling division
    
    // Allocate memory for windows
    *windows = malloc(num_windows * sizeof(struct SequenceWindow));
    if (!(*windows)) {
        fprintf(stderr, "Failed to allocate memory for sequence windows\n");
        return 0;
    }
    
    // Create each window
    for (int w_idx = 0; w_idx < num_windows; w_idx++) {
        int start_idx = w_idx * window_size;
        int end_idx = start_idx + window_size;
        if (end_idx > token_count) {
            end_idx = token_count;
        }
        
        int window_len = end_idx - start_idx;
        (*windows)[w_idx].entries = malloc(window_len * sizeof(struct VocabEntry));
        if (!(*windows)[w_idx].entries) {
            fprintf(stderr, "Failed to allocate memory for window entries\n");
            // Free previously allocated windows
            for (int i = 0; i < w_idx; i++) {
                free((*windows)[i].entries);
            }
            free(*windows);
            return 0;
        }
        
        // Fill the window with actual vocab entries
        for (int i = 0; i < window_len; i++) {
            int token_idx = start_idx + i;
            int found = 0;
            
            // Find the token in the vocabulary
            for (int v_idx = 0; v_idx < vocab_size; v_idx++) {
                if (strcmp(vocab[v_idx].word, tokens[token_idx]) == 0) {
                    // Copy the vocab entry
                    (*windows)[w_idx].entries[i] = vocab[v_idx];
                    found = 1;
                    break;
                }
            }
            
            // If token not in vocab, create a simple entry
            if (!found) {
                strcpy((*windows)[w_idx].entries[i].word, tokens[token_idx]);
                (*windows)[w_idx].entries[i].number = 0;
                (*windows)[w_idx].entries[i].embedding = 0.0f;
                (*windows)[w_idx].entries[i].pe = 0.0f;
                (*windows)[w_idx].entries[i].weight = 0.0f;
                (*windows)[w_idx].entries[i].bias1 = 0.0f;
                (*windows)[w_idx].entries[i].bias2 = 0.0f;
                (*windows)[w_idx].entries[i].bias3 = 0.0f;
                (*windows)[w_idx].entries[i].bias4 = 0.0f;
            }
        }
        
        (*windows)[w_idx].length = window_len;
        (*windows)[w_idx].start_pos = start_idx;
    }
    
    return num_windows;
}

// Function to apply RoPE to a sequence window based on actual positions
void apply_rope_to_sequence_window(struct SequenceWindow *window, float base) {
    for (int i = 0; i < window->length; i++) {
        int actual_pos = window->start_pos + i;  // Actual position in the overall text
        
        // Compute RoPE-enhanced positional embedding
        float freq = powf(base, -(float)(i % 4) / 4);  // Use a simple RoPE formula
        float cos_val = cosf(actual_pos * freq);
        float sin_val = sinf(actual_pos * freq);
        
        // Apply RoPE transformation to the embedding
        float expanded_val = window->entries[i].embedding * cos_val;
        window->entries[i].pe = expanded_val;
    }
}

// Free memory allocated for tokens
void free_tokens(char **tokens, int token_count) {
    for (int i = 0; i < token_count; i++) {
        if (tokens[i]) {
            free(tokens[i]);
        }
    }
    free(tokens);
}

// Free memory allocated for sequence windows
// Function to load vocabulary from file
int load_vocab_from_file(const char *vocab_file, struct VocabEntry *vocab, int *vocab_size) {
    FILE *vf = fopen(vocab_file, "r");
    if (!vf) {
        fprintf(stderr, "Error opening vocab file: %s\n", vocab_file);
        return 0;
    }
    
    char line[1024];
    *vocab_size = 0;
    
    // Skip header line
    fgets(line, sizeof(line), vf);
    
    while (fgets(line, sizeof(line), vf) && *vocab_size < MAX_VOCAB_SIZE) {
        int result = sscanf(line, "%d %s %f %f %f %f %f %f %f",
               &vocab[*vocab_size].number,
               vocab[*vocab_size].word,
               &vocab[*vocab_size].embedding,
               &vocab[*vocab_size].pe,
               &vocab[*vocab_size].weight,
               &vocab[*vocab_size].bias1,
               &vocab[*vocab_size].bias2,
               &vocab[*vocab_size].bias3,
               &vocab[*vocab_size].bias4);
        if (result == 9) { // Successfully parsed all fields
            (*vocab_size)++;
        }
    }
    
    fclose(vf);
    return 1;
}

void free_sequence_windows(struct SequenceWindow *windows, int num_windows) {
    for (int i = 0; i < num_windows; i++) {
        if (windows[i].entries) {
            free(windows[i].entries);
        }
    }
    free(windows);
}

// --- Main Training Logic ---
void train_model(struct VocabEntry *vocab, int vocab_size, const char *vocab_filename) {
    fprintf(stderr, "Training model...\n");
    
    // Load configuration - try multiple approaches to locate config.txt
    Config config;
    
    // Method 1: Try to get directory from vocab_filename using dynamic allocation
    char *vocab_path_copy = strdup(vocab_filename);  // Duplicate to avoid modifying original
    if (!vocab_path_copy) {
        fprintf(stderr, "Failed to allocate memory for vocab path\n");
        return;
    }
    
    char *last_slash = strrchr(vocab_path_copy, '/');
    char *config_path = NULL;
    
    if (last_slash != NULL) {
        *(last_slash + 1) = '\0'; // Keep slash and terminate after it
        
        // Allocate sufficient memory for the path + "config.txt"
        size_t path_len = strlen(vocab_path_copy);
        size_t config_len = strlen("config.txt");
        size_t total_len = path_len + config_len + 1;  // +1 for null terminator
        config_path = malloc(total_len);
        
        if (config_path) {
            snprintf(config_path, total_len, "%sconfig.txt", vocab_path_copy);
        } else {
            fprintf(stderr, "Failed to allocate memory for config path\n");
            free(vocab_path_copy);
            return;
        }
    } else {
        // If no slash found, config is in current directory
        config_path = strdup("config.txt");
        if (!config_path) {
            fprintf(stderr, "Failed to allocate memory for config path\n");
            free(vocab_path_copy);
            return;
        }
    }
    
    fprintf(stderr, "Attempting to load config from: %s\n", config_path);
    
    int config_loaded = load_config(&config, config_path);
    if (!config_loaded) {
        fprintf(stderr, "Failed to load config from: %s\n", config_path);
        fprintf(stderr, "Trying current directory...\n");
        config_loaded = load_config(&config, "config.txt");
        if (config_loaded) {
            fprintf(stderr, "Config loaded from current directory.\n");
        }
    }
    
    print_config(&config);
    
    // Free the dynamically allocated memory for config_path
    free(vocab_path_copy);
    free(config_path);
    
    char *output_dir = dirname(strdup(vocab_filename));
    char *cmd = NULL;  // Will use dynamic allocation
    int cmd_len;       // For dynamic command string length calculation
    char pth[1024];
    char attn_path[1024], mlp_path[1024], optim_path[1024], out_path[1024];
    sprintf(attn_path, "%s/attention_model.txt", output_dir); sprintf(mlp_path, "%s/mlp_model.txt", output_dir);
    sprintf(optim_path, "%s/optimizer_state.txt", output_dir); sprintf(out_path, "%s/output_layer.txt", output_dir);

    // Use dynamic allocation for command string
        free(cmd);
        cmd_len = snprintf(NULL, 0, "./+x/optimizer.+x adam-init %s %s %d", optim_path, output_dir, vocab_size);
        cmd = malloc(cmd_len + 1);
        if (cmd) {
            snprintf(cmd, cmd_len + 1, "./+x/optimizer.+x adam-init %s %s %d", optim_path, output_dir, vocab_size);
        }
    if (system(cmd) != 0) { fprintf(stderr, "Failed to initialize optimizer\n"); return; }
    
    // Update optimizer state with config values
    FILE *sf = fopen(optim_path, "w");
    if (sf) {
        fprintf(sf, "%f %f %f 0", config.learning_rate, config.beta1, config.beta2);
        fclose(sf);
    }

    for (int epoch = 0; epoch < config.epochs; epoch++) {
        // Check if exit signal was received at start of epoch
        if (should_exit) {
            fprintf(stderr, "Training interrupted by user. Force killing all processes and exiting...\n");
            system("pkill -f 'trainer' 2>/dev/null; pkill -f '+x/forward_prop' 2>/dev/null; pkill -f '+x/backward_prop' 2>/dev/null; pkill -f '+x/optimizer' 2>/dev/null");
            _exit(1);
        }
        
        float total_loss = 0.0f;
        
        // Phase 1: Forward Propagation - Process all words first
        fprintf(stderr, "\nEpoch %d/%d - Phase 1: Forward Propagation\n", epoch + 1, config.epochs);
        for (int i = 0; i < vocab_size - 1; i++) {
            // Check for interruption more frequently - add usleep to allow signal processing
            if (should_exit) {
                fprintf(stderr, "Training interrupted by user. Force killing all processes and exiting...\n");
                system("pkill -f 'trainer' 2>/dev/null; pkill -f '+x/forward_prop' 2>/dev/null; pkill -f '+x/backward_prop' 2>/dev/null; pkill -f '+x/optimizer' 2>/dev/null");
                _exit(1);
            }
            
            fprintf(stderr, "\rForward Prop: Word %d/%d", i + 1, vocab_size - 1);
            fflush(stdout); // Force output to update
            
            // Use dynamic allocation for command string
            if (cmd) { free(cmd); cmd = NULL; }  // Safely free previous command
            cmd_len = snprintf(NULL, 0, "./+x/forward_prop.+x %s %d %s %s %s %d", vocab_filename, i, attn_path, mlp_path, out_path, config.causal_attention);
            cmd = malloc(cmd_len + 1);
            if (cmd) {
                snprintf(cmd, cmd_len + 1, "./+x/forward_prop.+x %s %d %s %s %s %d", vocab_filename, i, attn_path, mlp_path, out_path, config.causal_attention);
            }
            if (cmd) {
                // Reset signal handling before executing command
                signal(SIGINT, signal_handler);
                signal(SIGTERM, signal_handler);
                
                // Fork and exec to run the command so we can track its PID
                pid_t child_pid = fork();
                if (child_pid == 0) {
                    // Child process - execute the command
                    execl("/bin/sh", "sh", "-c", cmd, NULL);
                    // If execl returns, it failed
                    perror("execl failed");
                    _exit(1);
                } else if (child_pid > 0) {
                    // Parent process - wait for child but allow interruption
                    int status;
                    pid_t result = waitpid(child_pid, &status, 0);
                    if (result == -1) {
                        // If waitpid was interrupted (by our signal handler), kill the child
                        kill(child_pid, SIGTERM);
                        // Give it a moment to terminate gracefully, then force kill
                        sleep(1);
                        kill(child_pid, SIGKILL);
                    }
                    
                    // Check if the command succeeded
                    if (result != -1 && !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                        fprintf(stderr, "\nForward prop failed for word %d\n", i); 
                        if (cmd) { free(cmd); cmd = NULL; }  // Free and reset cmd
                        continue; 
                    }
                } else {
                    // Fork failed
                    perror("fork failed");
                    if (cmd) { free(cmd); cmd = NULL; }
                    continue;
                }
            }
        }
        
        // Phase 2: Loss Computation & Gradient Calculation 
        fprintf(stderr, "\nEpoch %d/%d - Phase 2: Loss Computation\n", epoch + 1, config.epochs);
        for (int i = 0; i < vocab_size - 1; i++) {
            // Check for interruption
            if (should_exit) {
                fprintf(stderr, "Training interrupted by user. Force killing all processes and exiting...\n");
                system("pkill -f 'trainer' 2>/dev/null; pkill -f '+x/forward_prop' 2>/dev/null; pkill -f '+x/backward_prop' 2>/dev/null; pkill -f '+x/optimizer' 2>/dev/null");
                _exit(1);
            }

            char pred_path[1024], gloss_path[1024];
            sprintf(pred_path, "%s/predictions.txt", output_dir); 
            sprintf(gloss_path, "%s/grad_loss.txt", output_dir);

            FILE *pred_f = fopen(pred_path, "r"); 
            if (!pred_f) { 
                fprintf(stderr, "\nNo predictions file for word %d.\n", i); 
                continue; 
            }
            
            float *preds = malloc(vocab_size * sizeof(float)); 
            for(int j=0; j<vocab_size; j++) {
                fscanf(pred_f, "%f", &preds[j]); 
            } 
            fclose(pred_f);
            
            float *grad = malloc(vocab_size * sizeof(float));
            float loss = compute_cross_entropy_loss_and_gradient(preds, i + 1, vocab_size, grad);
            
            // Check for NaN loss
            if (isnan(loss) || isinf(loss)) {
                fprintf(stderr, "Warning: NaN or Inf loss detected for word %d, setting to 0\n", i);
                loss = 0.0f;
                // Reset gradients to zero
                for (int j = 0; j < vocab_size; j++) {
                    grad[j] = 0.0f;
                }
            }
            total_loss += loss;
            
            FILE *gloss_f = fopen(gloss_path, "w"); 
            if(gloss_f) { 
                for(int j=0; j<vocab_size; j++) {
                    fprintf(gloss_f, "%f ", grad[j]); 
                } 
                fclose(gloss_f); 
            }
            
            free(preds); 
            free(grad);
        }
        
        // Phase 3: Backward Propagation - Process all words with gradients
        fprintf(stderr, "\nEpoch %d/%d - Phase 3: Backward Propagation\n", epoch + 1, config.epochs);
        for (int i = 0; i < vocab_size - 1; i++) {
            // Check for interruption
            if (should_exit) {
                fprintf(stderr, "Training interrupted by user. Force killing all processes and exiting...\n");
                system("pkill -f 'trainer' 2>/dev/null; pkill -f '+x/forward_prop' 2>/dev/null; pkill -f '+x/backward_prop' 2>/dev/null; pkill -f '+x/optimizer' 2>/dev/null");
                _exit(1);
            }

            char pred_path[1024], gloss_path[1024], hidden_path[1024], ctx_path[1024], q_path[1024], k_path[1024], v_path[1024], asr_path[1024];
            sprintf(pred_path, "%s/predictions.txt", output_dir); sprintf(gloss_path, "%s/grad_loss.txt", output_dir);
            sprintf(hidden_path, "%s/hidden_state.txt", output_dir); sprintf(ctx_path, "%s/context.txt", output_dir);
            sprintf(q_path, "%s/q.txt", output_dir); sprintf(k_path, "%s/k.txt", output_dir); sprintf(v_path, "%s/v.txt", output_dir);
            sprintf(asr_path, "%s/attn_scores_raw.txt", output_dir);

            // Use dynamic allocation for command string
            if (cmd) { free(cmd); cmd = NULL; }  // Safely free previous command
            cmd_len = snprintf(NULL, 0, "./+x/backward_prop.+x %s %d %s %s %s %s %s %s %s %s %s %s", vocab_filename, i, gloss_path, attn_path, mlp_path, out_path, hidden_path, ctx_path, q_path, k_path, v_path, asr_path);
            cmd = malloc(cmd_len + 1);
            if (cmd) {
                snprintf(cmd, cmd_len + 1, "./+x/backward_prop.+x %s %d %s %s %s %s %s %s %s %s %s %s", vocab_filename, i, gloss_path, attn_path, mlp_path, out_path, hidden_path, ctx_path, q_path, k_path, v_path, asr_path);
            }
            if (cmd) {
                // Reset signal handling before executing command
                signal(SIGINT, signal_handler);
                signal(SIGTERM, signal_handler);
                
                // Fork and exec to run the command so we can track its PID
                pid_t child_pid = fork();
                if (child_pid == 0) {
                    // Child process - execute the command
                    execl("/bin/sh", "sh", "-c", cmd, NULL);
                    // If execl returns, it failed
                    perror("execl failed");
                    _exit(1);
                } else if (child_pid > 0) {
                    // Parent process - wait for child but allow interruption
                    int status;
                    pid_t result = waitpid(child_pid, &status, 0);
                    if (result == -1) {
                        // If waitpid was interrupted (by our signal handler), kill the child
                        kill(child_pid, SIGTERM);
                        // Give it a moment to terminate gracefully, then force kill
                        sleep(1);
                        kill(child_pid, SIGKILL);
                    }
                    
                    // Check if the command succeeded
                    if (result != -1 && !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                        fprintf(stderr, "\nBackward prop failed for word %d\n", i); 
                        if (cmd) { free(cmd); cmd = NULL; }  // Free and reset cmd
                        continue; 
                    }
                } else {
                    // Fork failed
                    perror("fork failed");
                    if (cmd) { free(cmd); cmd = NULL; }
                    continue;
                }
            }
        }
        
        // Phase 4: Optimizer Update - Update all model parameters
        fprintf(stderr, "\nEpoch %d/%d - Phase 4: Optimizer Update\n", epoch + 1, config.epochs);
        char grad_prefix[1024]; sprintf(grad_prefix, "%s/grad", output_dir);
        // Use dynamic allocation for command string
        if (cmd) { free(cmd); cmd = NULL; }  // Safely free previous command
        cmd_len = snprintf(NULL, 0, "./+x/optimizer.+x update %d %s %s %s %s %s", vocab_size, optim_path, attn_path, mlp_path, out_path, grad_prefix);
        cmd = malloc(cmd_len + 1);
        if (cmd) {
            snprintf(cmd, cmd_len + 1, "./+x/optimizer.+x update %d %s %s %s %s %s", vocab_size, optim_path, attn_path, mlp_path, out_path, grad_prefix);
        }
        if (cmd) {
            // Reset signal handling before executing command
            signal(SIGINT, signal_handler);
            signal(SIGTERM, signal_handler);
            
            // Fork and exec to run the command so we can track its PID
            pid_t child_pid = fork();
            if (child_pid == 0) {
                // Child process - execute the command
                execl("/bin/sh", "sh", "-c", cmd, NULL);
                // If execl returns, it failed
                perror("execl failed");
                _exit(1);
            } else if (child_pid > 0) {
                // Parent process - wait for child but allow interruption
                int status;
                pid_t result = waitpid(child_pid, &status, 0);
                if (result == -1) {
                    // If waitpid was interrupted (by our signal handler), kill the child
                    kill(child_pid, SIGTERM);
                    // Give it a moment to terminate gracefully, then force kill
                    sleep(1);
                    kill(child_pid, SIGKILL);
                }
                
                // Check if the command succeeded
                if (result != -1 && !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                    fprintf(stderr, "\nOptimizer update failed\n"); 
                    if (cmd) { free(cmd); cmd = NULL; }  // Free and reset cmd
                    continue; 
                }
            } else {
                // Fork failed
                perror("fork failed");
                if (cmd) { free(cmd); cmd = NULL; }
                continue;
            }
        }
        write_loss(total_loss / (vocab_size - 1), output_dir);
        fprintf(stderr, "\nEpoch %d/%d, Loss: %f\n", epoch + 1, config.epochs, total_loss / (vocab_size - 1));
        
        // Check for interruption between epochs
        if (should_exit) {
            fprintf(stderr, "Training interrupted by user after epoch %d. Force killing all processes and exiting...\n", epoch + 1);
            system("pkill -f 'trainer' 2>/dev/null; pkill -f '+x/forward_prop' 2>/dev/null; pkill -f '+x/backward_prop' 2>/dev/null; pkill -f '+x/optimizer' 2>/dev/null");
            _exit(1);
        }
    }
    
    fprintf(stderr, "Training complete.\n");
    
    // Free dynamically allocated command buffer if it exists
    if (cmd) { 
        free(cmd); 
        cmd = NULL;  // Prevent double free
    }
}



int main(int argc, char *argv[]) {
    // Register signal handlers with sigaction for more reliable interruption
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;  // Don't use SA_RESTART so system calls can be interrupted
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    
    struct sigaction sa_force;
    sa_force.sa_handler = force_exit_handler;
    sigemptyset(&sa_force.sa_mask);
    sa_force.sa_flags = 0;
    sigaction(SIGQUIT, &sa_force, NULL);
    
    if (argc < 2) { fprintf(stderr, "Usage: %s <vocab_model.txt> [corpus_file]\n", argv[0]); return 1; }
    
    char *vocab_filename = argv[1];
    char *corpus_filename = (argc >= 3) ? argv[2] : NULL;
    
    // If corpus file is provided, use sequence-based training
    if (corpus_filename != NULL) {
        fprintf(stderr, "Starting sequence-based training with corpus: %s\n", corpus_filename);
        
        // Load vocabulary
        struct VocabEntry *vocab = NULL;
        int vocab_size = 0;
        if (!load_vocab_from_file(vocab_filename, NULL, &vocab_size)) {
            fprintf(stderr, "Failed to determine vocab size\n");
            return 1;
        }
        
        // Allocate memory for the full vocabulary
        vocab = malloc(vocab_size * sizeof(struct VocabEntry));
        if (!vocab) {
            fprintf(stderr, "Failed to allocate memory for vocab\n");
            return 1;
        }
        
        if (!load_vocab_from_file(vocab_filename, vocab, &vocab_size)) {
            fprintf(stderr, "Failed to load vocab from file\n");
            free(vocab);
            return 1;
        }
        
        // Load tokens from corpus
        char **tokens = NULL;
        int token_count = 0;
        if (!load_tokens_from_corpus(corpus_filename, &tokens, &token_count)) {
            fprintf(stderr, "Failed to load tokens from corpus\n");
            free(vocab);
            return 1;
        }
        
        fprintf(stderr, "Loaded %d tokens from corpus\n", token_count);
        
        // Create sequence windows (using window size of 8 for now)
        struct SequenceWindow *windows = NULL;
        int window_size = 8; // Default window size
        int num_windows = create_sequence_windows(vocab, vocab_size, tokens, token_count, &windows, window_size);
        
        if (num_windows <= 0) {
            fprintf(stderr, "Failed to create sequence windows\n");
            free(vocab);
            free_tokens(tokens, token_count);
            return 1;
        }
        
        fprintf(stderr, "Created %d sequence windows\n", num_windows);
        
        // For each epoch, process all sequence windows
        Config config;
        char *output_dir = dirname(strdup(vocab_filename));
        
        // Load configuration
        char *vocab_path_copy = strdup(vocab_filename);
        if (!vocab_path_copy) {
            fprintf(stderr, "Failed to allocate memory for vocab path\n");
            free(vocab);
            free_tokens(tokens, token_count);
            free_sequence_windows(windows, num_windows);
            return 1;
        }
        
        char *last_slash = strrchr(vocab_path_copy, '/');
        char *config_path = NULL;
        
        if (last_slash != NULL) {
            *(last_slash + 1) = '\0';
            size_t path_len = strlen(vocab_path_copy);
            size_t config_len = strlen("config.txt");
            size_t total_len = path_len + config_len + 1;
            config_path = malloc(total_len);
            
            if (config_path) {
                snprintf(config_path, total_len, "%sconfig.txt", vocab_path_copy);
            } else {
                fprintf(stderr, "Failed to allocate memory for config path\n");
                free(vocab_path_copy);
                free(vocab);
                free_tokens(tokens, token_count);
                free_sequence_windows(windows, num_windows);
                return 1;
            }
        } else {
            config_path = strdup("config.txt");
            if (!config_path) {
                fprintf(stderr, "Failed to allocate memory for config path\n");
                free(vocab_path_copy);
                free(vocab);
                free_tokens(tokens, token_count);
                free_sequence_windows(windows, num_windows);
                return 1;
            }
        }
        
        int config_loaded = load_config(&config, config_path);
        if (!config_loaded) {
            fprintf(stderr, "Failed to load config from: %s\n", config_path);
            fprintf(stderr, "Trying current directory...\n");
            config_loaded = load_config(&config, "config.txt");
            if (config_loaded) {
                fprintf(stderr, "Config loaded from current directory.\n");
            }
        }
        
        print_config(&config);
        
        // Free the dynamically allocated memory for config_path
        free(vocab_path_copy);
        free(config_path);
        
        char *cmd = NULL;
        int cmd_len;
        char attn_path[1024], mlp_path[1024], optim_path[1024], out_path[1024];
        sprintf(attn_path, "%s/attention_model.txt", output_dir); 
        sprintf(mlp_path, "%s/mlp_model.txt", output_dir);
        sprintf(optim_path, "%s/optimizer_state.txt", output_dir); 
        sprintf(out_path, "%s/output_layer.txt", output_dir);

        // Initialize optimizer
        free(cmd);
        cmd_len = snprintf(NULL, 0, "./+x/optimizer.+x adam-init %s %s %d", optim_path, output_dir, vocab_size);
        cmd = malloc(cmd_len + 1);
        if (cmd) {
            snprintf(cmd, cmd_len + 1, "./+x/optimizer.+x adam-init %s %s %d", optim_path, output_dir, vocab_size);
        }
        if (system(cmd) != 0) { fprintf(stderr, "Failed to initialize optimizer\n"); return 1; }
        
        // Update optimizer state with config values
        FILE *sf = fopen(optim_path, "w");
        if (sf) {
            fprintf(sf, "%f %f %f 0", config.learning_rate, config.beta1, config.beta2);
            fclose(sf);
        }

        // Main training loop over epochs
        for (int epoch = 0; epoch < config.epochs; epoch++) {
            // Check if exit signal was received at start of epoch
            if (should_exit) {
                fprintf(stderr, "Training interrupted by user. Force killing all processes and exiting...\n");
                system("pkill -f 'trainer' 2>/dev/null; pkill -f '+x/forward_prop' 2>/dev/null; pkill -f '+x/backward_prop' 2>/dev/null; pkill -f '+x/optimizer' 2>/dev/null");
                _exit(1);
            }
            
            float total_loss = 0.0f;
            fprintf(stderr, "\nEpoch %d/%d - Processing %d sequence windows\n", epoch + 1, config.epochs, num_windows);
            
            // Process each sequence window (gradient accumulation across the whole sequence)
            for (int w_idx = 0; w_idx < num_windows; w_idx++) {
                // Apply RoPE to the current sequence window based on actual positions
                apply_rope_to_sequence_window(&windows[w_idx], 10000.0f);  // Default base frequency
                
                // Process each token in the sequence as a training example, accumulating gradients
                for (int pos = 0; pos < windows[w_idx].length - 1; pos++) {  // -1 to have target
                    // Check for interruption
                    if (should_exit) {
                        fprintf(stderr, "Training interrupted by user. Force killing all processes and exiting...\n");
                        system("pkill -f 'trainer' 2>/dev/null; pkill -f '+x/forward_prop' 2>/dev/null; pkill -f '+x/backward_prop' 2>/dev/null; pkill -f '+x/optimizer' 2>/dev/null");
                        _exit(1);
                    }
                    
                    fprintf(stderr, "\rProcessing sequence %d/%d, token %d/%d", 
                           w_idx + 1, num_windows, pos + 1, windows[w_idx].length - 1);
                    fflush(stdout);
                    
                    // Find vocabulary index for current token (source)
                    int src_idx = -1;
                    for (int v_idx = 0; v_idx < vocab_size; v_idx++) {
                        if (strcmp(vocab[v_idx].word, windows[w_idx].entries[pos].word) == 0) {
                            src_idx = v_idx;
                            break;
                        }
                    }
                    
                    // Find vocabulary index for next token (target)
                    int tgt_idx = -1;
                    for (int v_idx = 0; v_idx < vocab_size; v_idx++) {
                        if (strcmp(vocab[v_idx].word, windows[w_idx].entries[pos + 1].word) == 0) {
                            tgt_idx = v_idx;
                            break;
                        }
                    }
                    
                    // If both tokens are in vocabulary, process them
                    if (src_idx != -1 && tgt_idx != -1) {
                        // Phase 1: Forward Propagation
                        if (cmd) { free(cmd); cmd = NULL; }
                        cmd_len = snprintf(NULL, 0, "./+x/forward_prop.+x %s %d %s %s %s %d", 
                                          vocab_filename, src_idx, attn_path, mlp_path, out_path, config.causal_attention);
                        cmd = malloc(cmd_len + 1);
                        if (cmd) {
                            snprintf(cmd, cmd_len + 1, "./+x/forward_prop.+x %s %d %s %s %s %d", 
                                    vocab_filename, src_idx, attn_path, mlp_path, out_path, config.causal_attention);
                        }
                        if (cmd && system(cmd) != 0) { 
                            fprintf(stderr, "\nForward prop failed for word %d\n", src_idx); 
                            if (cmd) { free(cmd); cmd = NULL; }
                            continue; 
                        }
                        
                        // Phase 2: Loss Computation & Gradient Calculation
                        char pred_path[1024], gloss_path[1024];
                        sprintf(pred_path, "%s/predictions.txt", output_dir); 
                        sprintf(gloss_path, "%s/grad_loss.txt", output_dir);

                        FILE *pred_f = fopen(pred_path, "r"); 
                        if (!pred_f) { 
                            fprintf(stderr, "\nNo predictions file for word %d.\n", src_idx); 
                            continue; 
                        }
                        
                        float *preds = malloc(vocab_size * sizeof(float)); 
                        for(int j=0; j<vocab_size; j++) {
                            fscanf(pred_f, "%f", &preds[j]); 
                        } 
                        fclose(pred_f);
                        
                        float *grad = malloc(vocab_size * sizeof(float));
                        float loss = compute_cross_entropy_loss_and_gradient(preds, tgt_idx, vocab_size, grad);
                        
                        // Check for NaN loss
                        if (isnan(loss) || isinf(loss)) {
                            fprintf(stderr, "Warning: NaN or Inf loss detected for word %d, setting to 0\n", src_idx);
                            loss = 0.0f;
                            // Reset gradients to zero
                            for (int j = 0; j < vocab_size; j++) {
                                grad[j] = 0.0f;
                            }
                        }
                        total_loss += loss;
                        
                        FILE *gloss_f = fopen(gloss_path, "w"); 
                        if(gloss_f) { 
                            for(int j=0; j<vocab_size; j++) {
                                fprintf(gloss_f, "%f ", grad[j]); 
                            } 
                            fclose(gloss_f); 
                        }
                        
                        // Phase 3: Backward Propagation
                        char hidden_path[1024], ctx_path[1024], q_path[1024], k_path[1024], v_path[1024], asr_path[1024];
                        sprintf(hidden_path, "%s/hidden_state.txt", output_dir); 
                        sprintf(ctx_path, "%s/context.txt", output_dir);
                        sprintf(q_path, "%s/q.txt", output_dir); 
                        sprintf(k_path, "%s/k.txt", output_dir); 
                        sprintf(v_path, "%s/v.txt", output_dir);
                        sprintf(asr_path, "%s/attn_scores_raw.txt", output_dir);

                        if (cmd) { free(cmd); cmd = NULL; }
                        cmd_len = snprintf(NULL, 0, "./+x/backward_prop.+x %s %d %s %s %s %s %s %s %s %s %s %s", 
                                          vocab_filename, src_idx, gloss_path, attn_path, mlp_path, out_path, 
                                          hidden_path, ctx_path, q_path, k_path, v_path, asr_path);
                        cmd = malloc(cmd_len + 1);
                        if (cmd) {
                            snprintf(cmd, cmd_len + 1, "./+x/backward_prop.+x %s %d %s %s %s %s %s %s %s %s %s %s", 
                                    vocab_filename, src_idx, gloss_path, attn_path, mlp_path, out_path, 
                                    hidden_path, ctx_path, q_path, k_path, v_path, asr_path);
                        }
                        if (cmd && system(cmd) != 0) { 
                            fprintf(stderr, "\nBackward prop failed for word %d\n", src_idx); 
                            if (cmd) { free(cmd); cmd = NULL; }
                            continue; 
                        }
                        
                        free(preds); 
                        free(grad);
                    }
                }  // End of sequence processing
                
                // After processing entire sequence window, update optimizer
                fprintf(stderr, "\nSequence %d/%d complete - performing optimizer update\n", w_idx + 1, num_windows);
                char grad_prefix[1024]; sprintf(grad_prefix, "%s/grad", output_dir);
                if (cmd) { free(cmd); cmd = NULL; }
                cmd_len = snprintf(NULL, 0, "./+x/optimizer.+x update %d %s %s %s %s %s", 
                                  vocab_size, optim_path, attn_path, mlp_path, out_path, grad_prefix);
                cmd = malloc(cmd_len + 1);
                if (cmd) {
                    snprintf(cmd, cmd_len + 1, "./+x/optimizer.+x update %d %s %s %s %s %s", 
                            vocab_size, optim_path, attn_path, mlp_path, out_path, grad_prefix);
                }
                if (cmd && system(cmd) != 0) { 
                    fprintf(stderr, "\nOptimizer update failed for sequence %d\n", w_idx); 
                    if (cmd) { free(cmd); cmd = NULL; }
                }
            }
            
            write_loss(total_loss, output_dir);
            fprintf(stderr, "\nEpoch %d/%d, Total Loss: %f\n", epoch + 1, config.epochs, total_loss);
            
            // Check for interruption between epochs
            if (should_exit) {
                fprintf(stderr, "Training interrupted by user after epoch %d. Force killing all processes and exiting...\n", epoch + 1);
                system("pkill -f 'trainer' 2>/dev/null; pkill -f '+x/forward_prop' 2>/dev/null; pkill -f '+x/backward_prop' 2>/dev/null; pkill -f '+x/optimizer' 2>/dev/null");
                _exit(1);
            }
        }
        
        // Save the updated vocab
        if (!save_vocab(vocab, vocab_size, vocab_filename)) { 
            fprintf(stderr, "Failed to save updated vocab to %s\n", vocab_filename); 
        }
        
        // Free resources
        free(vocab);
        free_tokens(tokens, token_count);
        free_sequence_windows(windows, num_windows);
        if (cmd) { free(cmd); }
        
        fprintf(stderr, "Sequence-based training complete.\n");
        return 0;
    } else {
        // Use the original single-word training
        fprintf(stderr, "Starting single-word training (legacy mode)\n");
        
        // Dynamically allocate vocab array
        struct VocabEntry *vocab = NULL;
        int vocab_size = 0;
        int capacity = 1000; // Start with reasonable capacity
        vocab = malloc(capacity * sizeof(struct VocabEntry));
        if (!vocab) {
            fprintf(stderr, "Failed to allocate memory for vocab\n");
            return 1;
        }
        
        // Load vocab with dynamic resizing
        FILE *infile = fopen(vocab_filename, "r");
        if (!infile) { 
            perror("Error opening vocab file"); 
            free(vocab);
            return 1; 
        }
        
        char line[MAX_LINE_LENGTH];
        fgets(line, sizeof(line), infile); // Skip header
        
        while (fgets(line, sizeof(line), infile)) {
            // Check for interruption even during data loading
            if (should_exit) {
                fprintf(stderr, "Loading interrupted by user.\n");
                free(vocab);
                fclose(infile);
                return 1;
            }
            
            // Expand array if needed
            if (vocab_size >= capacity) {
                capacity *= 2;
                struct VocabEntry *temp = realloc(vocab, capacity * sizeof(struct VocabEntry));
                if (!temp) {
                    fprintf(stderr, "Failed to reallocate memory for vocab\n");
                    free(vocab);
                    fclose(infile);
                    return 1;
                }
                vocab = temp;
            }
            
            // Parse line
            int result = sscanf(line, "%d %s %f %f %f %f %f %f %f",
                               &vocab[vocab_size].number, vocab[vocab_size].word, 
                               &vocab[vocab_size].embedding, &vocab[vocab_size].pe, 
                               &vocab[vocab_size].weight, &vocab[vocab_size].bias1, 
                               &vocab[vocab_size].bias2, &vocab[vocab_size].bias3, 
                               &vocab[vocab_size].bias4);
            if (result == 9) { // Successfully parsed all fields
                vocab_size++;
            }
        }
        fclose(infile);
        
        srand(time(NULL));
        train_model(vocab, vocab_size, vocab_filename);
        
        // Save the updated vocab (even if interrupted)
        if (!save_vocab(vocab, vocab_size, vocab_filename)) { 
            fprintf(stderr, "Failed to save updated vocab to %s\n", vocab_filename); 
        }
        
        // Note: OS will automatically free memory when process exits
        fprintf(stderr, "Vocab model updated in %s\n", vocab_filename);
        return 0;
    }
}