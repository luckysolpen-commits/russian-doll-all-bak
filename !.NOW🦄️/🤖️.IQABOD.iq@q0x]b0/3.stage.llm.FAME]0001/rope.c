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

#define EMBEDDING_DIM 7
#define MAX_VOCAB_SIZE 100000
#define ROPE_EXPANDED_DIM 8  // Even dimension for proper RoPE processing

// Structure to hold a vocabulary entry
struct VocabEntry {
    int number;
    char word[100];
    float embedding;
    float pe;          // Position embedding (will be updated with RoPE result)
    float weight;
    float bias1;
    float bias2;
    float bias3;
    float bias4;
};

// Function to compute rotary positional embedding on an expanded array
void apply_rope(float *x, int dim, int pos, float base) {
    if (dim % 2 != 0) {
        fprintf(stderr, "RoPE requires even dimension size, got %d\n", dim);
        return;
    }
    
    for (int i = 0; i < dim; i += 2) {
        float freq = powf(base, -(float)i / dim);
        float cos_val = cosf(pos * freq);
        float sin_val = sinf(pos * freq);
        
        float x1 = x[i];
        float x2 = x[i + 1];
        
        x[i] = x1 * cos_val - x2 * sin_val;
        x[i + 1] = x1 * sin_val + x2 * cos_val;
    }
}

// Function to batch apply RoPE to embeddings
void apply_rope_batch(float *embeddings, int vocab_size, int embedding_dim, float base) {
    for (int pos = 0; pos < vocab_size; pos++) {
        apply_rope(&embeddings[pos * embedding_dim], embedding_dim, pos, base);
    }
}

// Enhanced function to process vocab with RoPE and return a single pe value
float compute_rope_pe_value(struct VocabEntry *vocab_entry, int position) {
    // Create an expanded vector for RoPE processing using vocabulary fields
    float expanded_vec[ROPE_EXPANDED_DIM];
    
    // Map the 7 vocab fields to an 8-element vector (pad with position info)
    expanded_vec[0] = vocab_entry->embedding;
    expanded_vec[1] = vocab_entry->pe;      // This is our main position field
    expanded_vec[2] = vocab_entry->weight;
    expanded_vec[3] = vocab_entry->bias1;
    expanded_vec[4] = vocab_entry->bias2;
    expanded_vec[5] = vocab_entry->bias3;
    expanded_vec[6] = vocab_entry->bias4;
    expanded_vec[7] = (float)position * 0.001f;  // Additional position information
    
    // Apply RoPE transformation to the expanded vector
    apply_rope(expanded_vec, ROPE_EXPANDED_DIM, position, 10000.0f);
    
    // Combine the transformed values back to a single pe value
    // Using weighted combination to preserve important information
    float combined_pe = 0.0f;
    float weights[ROPE_EXPANDED_DIM] = {0.15f, 0.25f, 0.12f, 0.12f, 0.12f, 0.12f, 0.12f, 0.0f};
    
    for (int i = 0; i < ROPE_EXPANDED_DIM; i++) {
        combined_pe += expanded_vec[i] * weights[i];
    }
    
    return combined_pe;
}

// Convert vocab entry to embedding vector with RoPE - maintains compatibility with existing code
void vocab_to_rope_embedding(struct VocabEntry *vocab_entry, float *embedding, int position) {
    // Copy original values first
    embedding[0] = vocab_entry->embedding;
    embedding[1] = compute_rope_pe_value(vocab_entry, position);  // Updated with RoPE result
    embedding[2] = vocab_entry->weight;
    embedding[3] = vocab_entry->bias1;
    embedding[4] = vocab_entry->bias2;
    embedding[5] = vocab_entry->bias3;
    embedding[6] = vocab_entry->bias4;
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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <operation> [args...]\n", argv[0]);
        fprintf(stderr, "Operations:\n");
        fprintf(stderr, "  encode <vocab_file> <output_file> <base_freq>  - Apply RoPE to vocab embeddings and save separately\n");
        fprintf(stderr, "  update <vocab_file> <output_file> [base_freq]  - Apply RoPE and update vocab entries in place\n");
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
        
        // Apply RoPE to each vocab entry
        for (int i = 0; i < vocab_size; i++) {
            float temp_embedding[EMBEDDING_DIM];
            vocab_to_rope_embedding(&vocab[i], temp_embedding, i);
            
            for (int j = 0; j < EMBEDDING_DIM; j++) {
                rope_embeddings[i * EMBEDDING_DIM + j] = temp_embedding[j];
            }
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
        
        // Apply RoPE transformation to each vocab entry in place
        for (int i = 0; i < vocab_size; i++) {
            // Use the enhanced compute_rope_pe_value function for the pe field
            vocab[i].pe = compute_rope_pe_value(&vocab[i], i);
        }
        
        // Save updated vocab
        save_updated_vocab(output_file, vocab, vocab_size);
        
        printf("RoPE applied to %d vocab entries and saved to %s\n", vocab_size, output_file);
        
        free(vocab);
        
    } else {
        fprintf(stderr, "Unknown operation: %s\n", operation);
        return 1;
    }
    
    return 0;
}
