#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_LINE_LENGTH 1024
#define MAX_VOCAB_SIZE 100000
#define MAX_RESPONSE_TOKENS 100
#define MAX_CURRICULA 10

// Structure to hold a vocabulary entry
struct VocabEntry {
    int number;
    char word[100];
    float embedding;
    float pe;
    float weight;
    float bias1;
    float bias2;
    float bias3;
    float bias4;
};

// Structure to hold a complete vocabulary
struct Vocabulary {
    struct VocabEntry *entries;
    int size;
};

// Function to find a word in the vocabulary
int find_word(struct VocabEntry *vocab, int vocab_size, const char *word) {
    for (int i = 0; i < vocab_size; i++) {
        if (strcmp(vocab[i].word, word) == 0) {
            return i;
        }
    }
    return -1;
}

// Function to apply softmax to a set of scores
void softmax(float *scores, int size, float temperature) {
    float max_score = scores[0];
    for (int i = 1; i < size; i++) {
        if (scores[i] > max_score) {
            max_score = scores[i];
        }
    }

    float sum = 0.0f;
    for (int i = 0; i < size; i++) {
        scores[i] = expf((scores[i] - max_score) / temperature);
        sum += scores[i];
    }

    for (int i = 0; i < size; i++) {
        scores[i] /= sum;
    }
}

// Function to load vocabulary from file
int load_vocabulary(struct VocabEntry *vocab, const char *filename) {
    FILE *infile = fopen(filename, "r");
    if (!infile) {
        perror("Error opening vocab file");
        return 0;
    }

    char line[MAX_LINE_LENGTH];
    int vocab_size = 0;

    // Skip header line
    fgets(line, sizeof(line), infile);

    while (fgets(line, sizeof(line), infile) && vocab_size < MAX_VOCAB_SIZE) {
        sscanf(line, "%d %s %f %f %f %f %f %f %f",
               &vocab[vocab_size].number,
               vocab[vocab_size].word,
               &vocab[vocab_size].embedding,
               &vocab[vocab_size].pe,
               &vocab[vocab_size].weight,
               &vocab[vocab_size].bias1,
               &vocab[vocab_size].bias2,
               &vocab[vocab_size].bias3,
               &vocab[vocab_size].bias4);
        vocab_size++;
    }

    fclose(infile);
    return vocab_size;
}

// Function to merge multiple vocabularies (MOE approach)
// Simply concatenate all vocabularies without deduplication to preserve unique positional encodings
int merge_vocabularies(struct VocabEntry *merged_vocab, struct Vocabulary *vocabularies, int num_vocabularies) {
    int total_size = 0;
    
    // For MOE, we'll combine all vocabularies without deduplication
    // This preserves unique positional encodings which can be very helpful
    for (int v = 0; v < num_vocabularies && total_size < MAX_VOCAB_SIZE; v++) {
        for (int i = 0; i < vocabularies[v].size && total_size < MAX_VOCAB_SIZE; i++) {
            merged_vocab[total_size] = vocabularies[v].entries[i];
            total_size++;
        }
    }
    
    return total_size;
}

// Function to predict the next word using temperature sampling
const char* predict_next_word(struct VocabEntry *vocab, int vocab_size, int current_word_index, float temperature) {
    float *scores = malloc(vocab_size * sizeof(float));
    if (!scores) {
        return "end-token";
    }

    // Get the vector of the current word
    float current_word_vec[] = {vocab[current_word_index].embedding, vocab[current_word_index].pe, vocab[current_word_index].weight, vocab[current_word_index].bias1, vocab[current_word_index].bias2, vocab[current_word_index].bias3, vocab[current_word_index].bias4};

    for (int i = 0; i < vocab_size; i++) {
        float next_word_vec[] = {vocab[i].embedding, vocab[i].pe, vocab[i].weight, vocab[i].bias1, vocab[i].bias2, vocab[i].bias3, vocab[i].bias4};
        // Simple dot product to influence the score
        float dot_product = 0.0f;
        for(int j=0; j<7; j++){
            dot_product += current_word_vec[j] * next_word_vec[j];
        }
        scores[i] = dot_product;
    }

    softmax(scores, vocab_size, temperature);

    // Determine how many top scores to consider based on temperature
    int top_n = 10;
    if (temperature < 0.5) {
        top_n = 5;  // More restrictive
    } else if (temperature > 2.0) {
        top_n = 20; // Less restrictive
    }
    
    // Make sure top_n doesn't exceed vocab_size
    if (top_n > vocab_size) {
        top_n = vocab_size;
    }

    // Save debug information to file
    FILE *debug_file = fopen("debug_chain.txt", "a");
    if (debug_file) {
        fprintf(debug_file, "\nDebug: Current word: %s (index: %d)\n", vocab[current_word_index].word, current_word_index);
        fprintf(debug_file, "Temperature: %f\n", temperature);
        fprintf(debug_file, "Considering top %d scores:\n", top_n);
        fclose(debug_file);
    }

    // Create array to store indices of top N scores
    int *top_indices = malloc(top_n * sizeof(int));
    float *top_scores = malloc(top_n * sizeof(float));
    
    // Initialize with first top_n elements
    for (int i = 0; i < top_n; i++) {
        top_indices[i] = i;
        top_scores[i] = scores[i];
    }
    
    // Find the actual top N scores using a simple selection sort approach
    for (int i = 0; i < top_n; i++) {
        int max_idx = i;
        for (int j = i + 1; j < top_n; j++) {
            if (top_scores[j] > top_scores[max_idx]) {
                max_idx = j;
            }
        }
        // Swap scores
        float temp_score = top_scores[i];
        top_scores[i] = top_scores[max_idx];
        top_scores[max_idx] = temp_score;
        // Swap indices
        int temp_idx = top_indices[i];
        top_indices[i] = top_indices[max_idx];
        top_indices[max_idx] = temp_idx;
    }
    
    // Now check the rest of the vocabulary to see if any scores are higher
    for (int i = top_n; i < vocab_size; i++) {
        for (int j = 0; j < top_n; j++) {
            if (scores[i] > top_scores[j]) {
                // Shift elements down
                for (int k = top_n - 1; k > j; k--) {
                    top_scores[k] = top_scores[k-1];
                    top_indices[k] = top_indices[k-1];
                }
                // Insert new element
                top_scores[j] = scores[i];
                top_indices[j] = i;
                break;
            }
        }
    }

    // Log top scores to debug file
    debug_file = fopen("debug_chain.txt", "a");
    if (debug_file) {
        for (int i = 0; i < top_n; i++) {
            fprintf(debug_file, "  %d. %s (index: %d): %f\n", i+1, vocab[top_indices[i]].word, top_indices[i], top_scores[i]);
        }
        fclose(debug_file);
    }

    // Select based on temperature - more deterministic at lower temperatures
    int next_word_index = top_indices[0]; // Default to top score
    
    if (top_n > 1) {
        if (temperature < 0.2) {
            // Very low temperature: almost always pick the top score
            float r = (float)rand() / (float)RAND_MAX;
            if (r > 0.1) { // 90% of the time pick top score with very low temperature
                next_word_index = top_indices[0];
            } else {
                // 10% of the time pick randomly from top 2
                float r2 = (float)rand() / (float)RAND_MAX;
                int selected_idx = (r2 < 0.5) ? 0 : 1;
                next_word_index = top_indices[selected_idx];
            }
        } else {
            // Use temperature to influence selection from top N
            float r = (float)rand() / (float)RAND_MAX;
            int selected_idx = (int)(r * top_n);
            if (selected_idx >= top_n) selected_idx = top_n - 1;
            next_word_index = top_indices[selected_idx];
        }
    }

    // Log the chosen word
    debug_file = fopen("debug_chain.txt", "a");
    if (debug_file) {
        fprintf(debug_file, "Chosen word: %s (index: %d)\n", vocab[next_word_index].word, next_word_index);
        fclose(debug_file);
    }

    free(scores);
    free(top_indices);
    free(top_scores);

    return vocab[next_word_index].word;
}

// New function that uses aggregated context from all tokens in the sequence
const char* predict_next_word_with_context(struct VocabEntry *vocab, int vocab_size, float *aggregated_context, float temperature) {
    float *scores = malloc(vocab_size * sizeof(float));
    if (!scores) {
        return "end-token";
    }

    // Use the aggregated context vector as input
    float current_context_vec[7];
    for (int j = 0; j < 7; j++) {
        current_context_vec[j] = aggregated_context[j];
    }

    for (int i = 0; i < vocab_size; i++) {
        float next_word_vec[] = {vocab[i].embedding, vocab[i].pe, vocab[i].weight, vocab[i].bias1, vocab[i].bias2, vocab[i].bias3, vocab[i].bias4};
        // Simple dot product between aggregated context and next word vector
        float dot_product = 0.0f;
        for(int j = 0; j < 7; j++){
            dot_product += current_context_vec[j] * next_word_vec[j];
        }
        scores[i] = dot_product;
    }

    softmax(scores, vocab_size, temperature);

    // Determine how many top scores to consider based on temperature
    int top_n = 10;
    if (temperature < 0.5) {
        top_n = 5;  // More restrictive
    } else if (temperature > 2.0) {
        top_n = 20; // Less restrictive
    }
    
    // Make sure top_n doesn't exceed vocab_size
    if (top_n > vocab_size) {
        top_n = vocab_size;
    }

    // Save debug information to file
    FILE *debug_file = fopen("debug_chain.txt", "a");
    if (debug_file) {
        fprintf(debug_file, "\nDebug: Using aggregated context from input sequence\n");
        fprintf(debug_file, "Aggregated context: [%.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f]\n", 
                aggregated_context[0], aggregated_context[1], aggregated_context[2], 
                aggregated_context[3], aggregated_context[4], aggregated_context[5], aggregated_context[6]);
        fprintf(debug_file, "Temperature: %f\n", temperature);
        fprintf(debug_file, "Considering top %d scores:\n", top_n);
        fclose(debug_file);
    }

    // Create array to store indices of top N scores
    int *top_indices = malloc(top_n * sizeof(int));
    float *top_scores = malloc(top_n * sizeof(float));
    
    // Initialize with first top_n elements
    for (int i = 0; i < top_n; i++) {
        top_indices[i] = i;
        top_scores[i] = scores[i];
    }
    
    // Find the actual top N scores using a simple selection sort approach
    for (int i = 0; i < top_n; i++) {
        int max_idx = i;
        for (int j = i + 1; j < top_n; j++) {
            if (top_scores[j] > top_scores[max_idx]) {
                max_idx = j;
            }
        }
        // Swap scores
        float temp_score = top_scores[i];
        top_scores[i] = top_scores[max_idx];
        top_scores[max_idx] = temp_score;
        // Swap indices
        int temp_idx = top_indices[i];
        top_indices[i] = top_indices[max_idx];
        top_indices[max_idx] = temp_idx;
    }
    
    // Now check the rest of the vocabulary to see if any scores are higher
    for (int i = top_n; i < vocab_size; i++) {
        for (int j = 0; j < top_n; j++) {
            if (scores[i] > top_scores[j]) {
                // Shift elements down
                for (int k = top_n - 1; k > j; k--) {
                    top_scores[k] = top_scores[k-1];
                    top_indices[k] = top_indices[k-1];
                }
                // Insert new element
                top_scores[j] = scores[i];
                top_indices[j] = i;
                break;
            }
        }
    }

    // Log top scores to debug file
    debug_file = fopen("debug_chain.txt", "a");
    if (debug_file) {
        for (int i = 0; i < top_n; i++) {
            fprintf(debug_file, "  %d. %s (index: %d): %f\n", i+1, vocab[top_indices[i]].word, top_indices[i], top_scores[i]);
        }
        fclose(debug_file);
    }

    // Select based on temperature - more deterministic at lower temperatures
    int next_word_index = top_indices[0]; // Default to top score
    
    if (top_n > 1) {
        if (temperature < 0.2) {
            // Very low temperature: almost always pick the top score
            float r = (float)rand() / (float)RAND_MAX;
            if (r > 0.1) { // 90% of the time pick top score with very low temperature
                next_word_index = top_indices[0];
            } else {
                // 10% of the time pick randomly from top 2
                float r2 = (float)rand() / (float)RAND_MAX;
                int selected_idx = (r2 < 0.5) ? 0 : 1;
                next_word_index = top_indices[selected_idx];
            }
        } else {
            // Use temperature to influence selection from top N
            float r = (float)rand() / (float)RAND_MAX;
            int selected_idx = (int)(r * top_n);
            if (selected_idx >= top_n) selected_idx = top_n - 1;
            next_word_index = top_indices[selected_idx];
        }
    }

    // Log the chosen word
    debug_file = fopen("debug_chain.txt", "a");
    if (debug_file) {
        fprintf(debug_file, "Chosen word: %s (index: %d)\n", vocab[next_word_index].word, next_word_index);
        fclose(debug_file);
    }

    free(scores);
    free(top_indices);
    free(top_scores);

    return vocab[next_word_index].word;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <curriculum_bank.txt> \"<prompt>\" [length] [temperature]\n", argv[0]);
        return 1;
    }

    srand(time(NULL));

    char *curriculum_bank_file = argv[1];
    char *prompt = argv[2];
    int desired_length = -1;
    float temperature = 1.0f;

    if (argc >= 4) {
        desired_length = atoi(argv[3]);
    }
    if (argc >= 5) {
        temperature = atof(argv[4]);
    }

    // Read curriculum paths from the bank file
    FILE *bank_file = fopen(curriculum_bank_file, "r");
    if (!bank_file) {
        perror("Error opening curriculum bank file");
        return 1;
    }

    char curriculum_paths[MAX_CURRICULA][MAX_LINE_LENGTH];
    int num_curricula = 0;
    
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), bank_file) && num_curricula < MAX_CURRICULA) {
        // Remove newline character
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) > 0) {
            strcpy(curriculum_paths[num_curricula], line);
            num_curricula++;
        }
    }
    fclose(bank_file);

    if (num_curricula == 0) {
        fprintf(stderr, "No curriculum paths found in bank file\n");
        return 1;
    }

    // Load vocabularies from all curriculum files
    struct Vocabulary vocabularies[MAX_CURRICULA];
    for (int i = 0; i < num_curricula; i++) {
        vocabularies[i].entries = malloc(MAX_VOCAB_SIZE * sizeof(struct VocabEntry));
        if (!vocabularies[i].entries) {
            perror("Failed to allocate memory for vocabulary");
            // Free previously allocated vocabularies
            for (int j = 0; j < i; j++) {
                free(vocabularies[j].entries);
            }
            return 1;
        }
        vocabularies[i].size = load_vocabulary(vocabularies[i].entries, curriculum_paths[i]);
        fprintf(stderr, "Loaded %d words from %s\n", vocabularies[i].size, curriculum_paths[i]);
    }

    // Merge vocabularies (for MOE implementation, this would be more sophisticated)
    struct VocabEntry *merged_vocab = malloc(MAX_VOCAB_SIZE * sizeof(struct VocabEntry));
    if (!merged_vocab) {
        perror("Failed to allocate memory for merged vocabulary");
        // Free allocated vocabularies
        for (int i = 0; i < num_curricula; i++) {
            free(vocabularies[i].entries);
        }
        return 1;
    }
    
    int merged_vocab_size = merge_vocabularies(merged_vocab, vocabularies, num_curricula);
    fprintf(stderr, "Merged vocabulary size: %d\n", merged_vocab_size);

    printf("Prompt: %s\n", prompt);
    
    // Create debug file and write prompt
    FILE *debug_file = fopen("debug_chain.txt", "w");
    if (debug_file) {
        fprintf(debug_file, "=== Chatbot MOE Debug Log ===\n");
        fprintf(debug_file, "Prompt: %s\n", prompt);
        fprintf(debug_file, "Desired length: %d\n", desired_length);
        fprintf(debug_file, "Temperature: %f\n", temperature);
        fprintf(debug_file, "Using %d training sets:\n", num_curricula);
        for (int i = 0; i < num_curricula; i++) {
            fprintf(debug_file, "  %d. %s\n", i+1, curriculum_paths[i]);
        }
        fprintf(debug_file, "========================\n\n");
        fclose(debug_file);
    }
    
    printf("Generating response: ");

    // Tokenize the prompt and compute aggregated context vector from all tokens
    char *token = strtok(prompt, " ");
    float aggregated_context[7] = {0}; // embedding, pe, weight, bias1-4 combined into 7 dimensions
    int token_count = 0;
    int last_word_index = -1; // Keep track of the last word in the prompt

    while (token != NULL) {
        int word_index = find_word(merged_vocab, merged_vocab_size, token);
        if (word_index != -1) {
            // Add this token's features to the aggregated context
            aggregated_context[0] += merged_vocab[word_index].embedding;
            aggregated_context[1] += merged_vocab[word_index].pe;
            aggregated_context[2] += merged_vocab[word_index].weight;
            aggregated_context[3] += merged_vocab[word_index].bias1;
            aggregated_context[4] += merged_vocab[word_index].bias2;
            aggregated_context[5] += merged_vocab[word_index].bias3;
            aggregated_context[6] += merged_vocab[word_index].bias4;
            last_word_index = word_index; // Keep track of the last word processed
            token_count++;
        }
        token = strtok(NULL, " ");
    }

    // Normalize the aggregated context
    if (token_count > 0) {
        for (int i = 0; i < 7; i++) {
            aggregated_context[i] /= token_count;
        }
    } else {
        // If no tokens found, use start-token
        int start_idx = find_word(merged_vocab, merged_vocab_size, "start-token");
        if (start_idx != -1) {
            aggregated_context[0] = merged_vocab[start_idx].embedding;
            aggregated_context[1] = merged_vocab[start_idx].pe;
            aggregated_context[2] = merged_vocab[start_idx].weight;
            aggregated_context[3] = merged_vocab[start_idx].bias1;
            aggregated_context[4] = merged_vocab[start_idx].bias2;
            aggregated_context[5] = merged_vocab[start_idx].bias3;
            aggregated_context[6] = merged_vocab[start_idx].bias4;
            last_word_index = start_idx;
            token_count = 1;
        }
    }

    int length_count = 0;
    char response_buffer[MAX_RESPONSE_TOKENS * 100]; // Max 100 tokens, each max 100 chars
    response_buffer[0] = '\0'; // Initialize empty string

    // Ensure we have a minimum temperature for variety
    float min_temperature = 0.1f;
    if (temperature < min_temperature) temperature = min_temperature;

    const char* next_word = predict_next_word_with_context(merged_vocab, merged_vocab_size, aggregated_context, temperature);

    // Generate at least one word
    if (strcmp(next_word, "end-token") == 0) {
        // Try once more with higher temperature using the aggregated context
        next_word = predict_next_word_with_context(merged_vocab, merged_vocab_size, aggregated_context, temperature * 2.0f);
    }

    // For subsequent generations, we'll keep track of the current token context
    int current_word_index = last_word_index;
    
    while (strcmp(next_word, "end-token") != 0 && (desired_length == -1 || length_count < desired_length) && length_count < MAX_RESPONSE_TOKENS) {
        snprintf(response_buffer + strlen(response_buffer), sizeof(response_buffer) - strlen(response_buffer), "%s ", next_word);
        fprintf(stderr, "\rGenerating response: %d/%d tokens", length_count + 1, desired_length == -1 ? MAX_RESPONSE_TOKENS : desired_length);
        fflush(stderr);

        current_word_index = find_word(merged_vocab, merged_vocab_size, next_word);
        if (current_word_index == -1) {
            // If word not found, use start-token
            current_word_index = find_word(merged_vocab, merged_vocab_size, "start-token");
        }
        
        float current_context[7] = {merged_vocab[current_word_index].embedding, merged_vocab[current_word_index].pe, merged_vocab[current_word_index].weight, 
                                    merged_vocab[current_word_index].bias1, merged_vocab[current_word_index].bias2, merged_vocab[current_word_index].bias3, merged_vocab[current_word_index].bias4};
        next_word = predict_next_word_with_context(merged_vocab, merged_vocab_size, current_context, temperature);
        length_count++;
    }

    printf("\nResponse: %s\n", response_buffer);
    
    // Write final response to debug file
    debug_file = fopen("debug_chain.txt", "a");
    if (debug_file) {
        fprintf(debug_file, "\n=== Final Response ===\n");
        fprintf(debug_file, "%s\n", response_buffer);
        fprintf(debug_file, "======================\n");
        fclose(debug_file);
    }

    // Free allocated memory
    free(merged_vocab);
    for (int i = 0; i < num_curricula; i++) {
        free(vocabularies[i].entries);
    }

    return 0;
}