// RoPE (Rotary Positional Embedding) Implementation for Transformer Architecture
//
// This implementation provides the necessary functions to apply RoPE to 
// vocabulary embeddings before they are processed by the attention mechanism.
//
// The transformer architecture pipeline should follow:
// 1. RoPE (Rotary Positional Embedding) - Apply positional encoding to embeddings
// 2. Attention - Compute self-attention with Q, K, V
// 3. FeedForward - Process through MLP layers
// 4. Backpropagation - Update weights based on loss

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#define EMBEDDING_DIM 7
#define MAX_VOCAB_SIZE 100000

// Structure to hold a vocabulary entry
struct VocabEntry {
    int number;
    char word[100];
    float embedding;
    float pe;          // Position embedding
    float weight;
    float bias1;
    float bias2;
    float bias3;
    float bias4;
};

// Function to compute expanded RoPE representation from single embedding
// This creates multiple dimensions based on the single embedding and position, then combines to a single pe value
float compute_single_pe_value(float embedding_val, int pos, float base) {
    // Expand the single embedding into multiple "dimensions" using RoPE principles
    // We'll create several RoPE-transformed values and combine them into a single pe value
    float combined_pe = 0.0f;
    int num_expansions = 4;  // Create 4 RoPE-expanded values to combine
    
    for (int i = 0; i < num_expansions; i++) {
        float freq = powf(base, -(float)i / num_expansions);
        float cos_val = cosf(pos * freq);
        float sin_val = sinf(pos * freq);
        
        // Create a RoPE-like transformation of the original embedding
        float expanded_val = embedding_val * cos_val;  // Could also use sin, or combine
        
        // Add this expanded dimension to our combined result
        combined_pe += expanded_val;
    }
    
    // Normalize by the number of expansions
    combined_pe /= num_expansions;
    
    return combined_pe;
}

// Function to batch apply RoPE to vocab entries, updating only the pe field
void apply_rope_to_vocab(struct VocabEntry *vocab, int vocab_size, float base) {
    for (int pos = 0; pos < vocab_size; pos++) {
        // Compute a single pe value from the embedding using RoPE expansion and combination
        vocab[pos].pe = compute_single_pe_value(vocab[pos].embedding, pos, base);
    }
}

// Function to apply RoPE to a sequence with actual sequence positions
void apply_rope_to_sequence(struct VocabEntry *sequence, int seq_len, float base, int start_pos) {
    for (int i = 0; i < seq_len; i++) {
        int actual_pos = start_pos + i;  // Actual position in the overall text
        sequence[i].pe = compute_single_pe_value(sequence[i].embedding, actual_pos, base);
    }
}

// Save updated vocab to file
void save_updated_vocab(const char *filename, struct VocabEntry *vocab, int vocab_size) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Error opening vocab file for writing");
        return;
    }
    
    fprintf(f, "number word embedding pe weight bias1 bias2 bias3 bias4\n");
    
    for (int i = 0; i < vocab_size; i++) {
        fprintf(f, "%d %s %f %f %f %f %f %f %f\n",
                vocab[i].number,
                vocab[i].word,
                vocab[i].embedding,
                vocab[i].pe,
                vocab[i].weight,
                vocab[i].bias1,
                vocab[i].bias2,
                vocab[i].bias3,
                vocab[i].bias4);
    }
    
    fclose(f);
}

// Function to create sequence windows from text corpus file
void create_sequence_windows_from_corpus(const char *corpus_file, const char *vocab_file, const char *output_file, int window_size, float base_freq) {
    FILE *cf = fopen(corpus_file, "r");
    if (!cf) {
        perror("Error opening corpus file");
        return;
    }
    
    // Load vocabulary
    struct VocabEntry *vocab = malloc(MAX_VOCAB_SIZE * sizeof(struct VocabEntry));
    if (!vocab) {
        perror("Failed to allocate memory for vocabulary");
        fclose(cf);
        return;
    }
    
    FILE *vf = fopen(vocab_file, "r");
    if (!vf) {
        perror("Error opening vocab file");
        free(vocab);
        fclose(cf);
        return;
    }
    
    char line[1024];
    int vocab_size = 0;
    
    // Skip header
    fgets(line, sizeof(line), vf);
    
    while (fgets(line, sizeof(line), vf) && vocab_size < MAX_VOCAB_SIZE) {
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
    
    fclose(vf);
    
    // Read the entire corpus into memory as tokens
    char **tokens = malloc(10000 * sizeof(char*));  // Max 10k tokens
    int token_count = 0;
    
    // Rewind and read corpus
    rewind(cf);
    while (fgets(line, sizeof(line), cf) && token_count < 10000) {
        // Tokenize the line
        char *token = strtok(line, " \t\n\r");
        while (token != NULL && token_count < 10000) {
            // Clean up the token (remove punctuation)
            char clean_token[100];
            int j = 0;
            for (int i = 0; token[i] != '\0'; i++) {
                if (isalnum(token[i]) || token[i] == '\'') {
                    clean_token[j++] = token[i];
                }
            }
            clean_token[j] = '\0';
            
            if (strlen(clean_token) > 0) {
                tokens[token_count] = malloc(strlen(clean_token) + 1);
                strcpy(tokens[token_count], clean_token);
                token_count++;
            }
            
            token = strtok(NULL, " \t\n\r");
        }
    }
    
    fclose(cf);
    
    // Create sequence windows and apply RoPE
    FILE *of = fopen(output_file, "w");
    if (!of) {
        perror("Error opening output file");
        // Free memory
        for (int i = 0; i < token_count; i++) {
            free(tokens[i]);
        }
        free(tokens);
        free(vocab);
        return;
    }
    
    // Write header
    fprintf(of, "number word embedding pe weight bias1 bias2 bias3 bias4\n");
    
    // Process sequence windows
    int seq_idx = 0;
    while (seq_idx < token_count) {
        // Find vocabulary index for current token
        int found = 0;
        for (int v_idx = 0; v_idx < vocab_size; v_idx++) {
            if (strcmp(vocab[v_idx].word, tokens[seq_idx]) == 0) {
                // Apply RoPE with sequence position
                float seq_pos_pe = compute_single_pe_value(vocab[v_idx].embedding, seq_idx, base_freq);
                
                fprintf(of, "%d %s %f %f %f %f %f %f %f\n",
                        vocab[v_idx].number,
                        vocab[v_idx].word,
                        vocab[v_idx].embedding,
                        seq_pos_pe,  // Positional encoding based on actual sequence position
                        vocab[v_idx].weight,
                        vocab[v_idx].bias1,
                        vocab[v_idx].bias2,
                        vocab[v_idx].bias3,
                        vocab[v_idx].bias4);
                found = 1;
                break;
            }
        }
        
        if (!found) {
            // If token not in vocab, create a simple entry with zero pe
            fprintf(of, "0 %s %f %f %f %f %f %f %f\n",
                    tokens[seq_idx], 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        }
        
        seq_idx++;
    }
    
    fclose(of);
    
    // Free memory
    for (int i = 0; i < token_count; i++) {
        free(tokens[i]);
    }
    free(tokens);
    free(vocab);
    
    printf("Sequence windows created from %s and saved to %s\n", corpus_file, output_file);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <operation> [args...]\n", argv[0]);
        fprintf(stderr, "Operations:\n");
        fprintf(stderr, "  encode <vocab_file> <output_file> <base_freq>  - Apply RoPE to vocab embeddings and save separately\n");
        fprintf(stderr, "  update <vocab_file> <output_file> [base_freq]  - Apply RoPE and update vocab entries in place\n");
        fprintf(stderr, "  sequence <corpus_file> <vocab_file> <output_file> <window_size> <base_freq>  - Create sequence embeddings with RoPE\n");
        return 1;
    }
    
    char *operation = argv[1];
    float base_freq = 10000.0f;
    
    if (strcmp(operation, "encode") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s encode <vocab_file> <output_file> [base_freq]\n", argv[0]);
            return 1;
        }
        
        char *vocab_file = argv[2];
        char *output_file = argv[3];
        
        if (argc >= 5) {
            base_freq = atof(argv[4]);
        }
        
        // Load vocabulary
        FILE *vf = fopen(vocab_file, "r");
        if (!vf) {
            perror("Error opening vocab file");
            return 1;
        }
        
        struct VocabEntry *vocab = malloc(MAX_VOCAB_SIZE * sizeof(struct VocabEntry));
        if (!vocab) {
            perror("Failed to allocate memory for vocabulary");
            fclose(vf);
            return 1;
        }
        
        char line[1024];
        int vocab_size = 0;
        
        // Skip header line
        fgets(line, sizeof(line), vf);
        
        while (fgets(line, sizeof(line), vf) && vocab_size < MAX_VOCAB_SIZE) {
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
        
        fclose(vf);
        
        // Allocate memory for RoPE embeddings
        float *rope_embeddings = malloc(vocab_size * EMBEDDING_DIM * sizeof(float));
        if (!rope_embeddings) {
            perror("Failed to allocate memory for RoPE embeddings");
            free(vocab);
            return 1;
        }
        
        // Apply RoPE to each vocab entry to generate expanded embeddings
        for (int i = 0; i < vocab_size; i++) {
            // Generate the expanded RoPE embedding and store in rope_embeddings
            float pe_value = compute_single_pe_value(vocab[i].embedding, i, base_freq);
            
            // Store the values: original embedding, computed pe, and other fields unchanged
            rope_embeddings[i * EMBEDDING_DIM + 0] = vocab[i].embedding;
            rope_embeddings[i * EMBEDDING_DIM + 1] = pe_value;  // Updated pe value
            rope_embeddings[i * EMBEDDING_DIM + 2] = vocab[i].weight;
            rope_embeddings[i * EMBEDDING_DIM + 3] = vocab[i].bias1;
            rope_embeddings[i * EMBEDDING_DIM + 4] = vocab[i].bias2;
            rope_embeddings[i * EMBEDDING_DIM + 5] = vocab[i].bias3;
            rope_embeddings[i * EMBEDDING_DIM + 6] = vocab[i].bias4;
        }
        
        // Save to file
        FILE *f = fopen(output_file, "w");
        if (f) {
            for (int i = 0; i < vocab_size; i++) {
                for (int j = 0; j < EMBEDDING_DIM; j++) {
                    fprintf(f, "%f ", rope_embeddings[i * EMBEDDING_DIM + j]);
                }
                fprintf(f, "\n");
            }
            fclose(f);
        }
        
        printf("RoPE embeddings applied to %d vocab entries and saved to %s\n", vocab_size, output_file);
        
        free(vocab);
        free(rope_embeddings);
        
    } else if (strcmp(operation, "update") == 0) {
        // Update vocab file in place with RoPE-transformed values
        if (argc < 4) {
            fprintf(stderr, "Usage: %s update <input_vocab_file> <output_vocab_file> [base_freq]\n", argv[0]);
            return 1;
        }
        
        char *input_file = argv[2];
        char *output_file = argv[3];
        
        if (argc >= 5) {
            base_freq = atof(argv[4]);
        }
        
        // Load vocabulary
        FILE *vf = fopen(input_file, "r");
        if (!vf) {
            perror("Error opening input vocab file");
            return 1;
        }
        
        struct VocabEntry *vocab = malloc(MAX_VOCAB_SIZE * sizeof(struct VocabEntry));
        if (!vocab) {
            perror("Failed to allocate memory for vocabulary");
            fclose(vf);
            return 1;
        }
        
        char line[1024];
        int vocab_size = 0;
        
        // Skip header line
        fgets(line, sizeof(line), vf);
        
        while (fgets(line, sizeof(line), vf) && vocab_size < MAX_VOCAB_SIZE) {
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
        
        fclose(vf);
        
        // Apply RoPE transformation to each vocab entry in place - updating only the pe field
        apply_rope_to_vocab(vocab, vocab_size, base_freq);
        
        // Save updated vocab
        save_updated_vocab(output_file, vocab, vocab_size);
        
        printf("RoPE applied to %d vocab entries and saved to %s\n", vocab_size, output_file);
        
        free(vocab);
        
    } else if (strcmp(operation, "sequence") == 0) {
        // Create sequence windows with RoPE applied to actual sequence positions
        if (argc < 6) {
            fprintf(stderr, "Usage: %s sequence <corpus_file> <vocab_file> <output_file> <window_size> <base_freq>\n", argv[0]);
            return 1;
        }
        
        char *corpus_file = argv[2];
        char *vocab_file = argv[3];
        char *output_file = argv[4];
        int window_size = atoi(argv[5]);
        base_freq = atof(argv[6]);
        
        create_sequence_windows_from_corpus(corpus_file, vocab_file, output_file, window_size, base_freq);
        
    } else {
        fprintf(stderr, "Unknown operation: %s\n", operation);
        return 1;
    }
    
    return 0;
}
