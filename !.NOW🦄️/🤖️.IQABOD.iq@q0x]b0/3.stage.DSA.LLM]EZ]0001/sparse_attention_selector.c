#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <libgen.h>

#define MAX_VOCAB_SIZE 100000
#define EMBEDDING_DIM 7
#define K_SPARSITY 2048  // Sparsity parameter (k in DSA)

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

// Structure to hold index-score pairs for sorting
typedef struct {
    int index;
    float score;
} IndexScorePair;

// Comparison function for qsort in descending order
int compare_desc(const void *a, const void *b) {
    IndexScorePair *pair_a = (IndexScorePair *)a;
    IndexScorePair *pair_b = (IndexScorePair *)b;
    if (pair_a->score < pair_b->score) return 1;
    if (pair_a->score > pair_b->score) return -1;
    return 0;
}

// Function to get top-k indices from attention scores
void get_top_k_indices(float *attn_scores, int vocab_size, int *top_k_indices, int k) {
    // Create array of index-score pairs
    IndexScorePair *pairs = malloc(vocab_size * sizeof(IndexScorePair));
    
    for (int i = 0; i < vocab_size; i++) {
        pairs[i].index = i;
        pairs[i].score = attn_scores[i];
    }
    
    // Sort in descending order by score
    qsort(pairs, vocab_size, sizeof(IndexScorePair), compare_desc);
    
    // Copy top-k indices
    int actual_k = (vocab_size < k) ? vocab_size : k;
    for (int i = 0; i < actual_k; i++) {
        top_k_indices[i] = pairs[i].index;
    }
    
    // If we have fewer than k elements, fill the rest with -1 or a sentinel value
    for (int i = actual_k; i < k; i++) {
        top_k_indices[i] = -1;  // Sentinel value indicating invalid index
    }
    
    free(pairs);
}

// Function to create sparse mask based on top-k indices
void create_sparse_mask(int *top_k_indices, int k, int vocab_size, float *mask) {
    // Initialize mask to 0
    for (int i = 0; i < vocab_size; i++) {
        mask[i] = 0.0f;
    }
    
    // Set mask to 1 for selected indices
    for (int i = 0; i < k; i++) {
        if (top_k_indices[i] != -1) {  // Check for sentinel value
            mask[top_k_indices[i]] = 1.0f;
        }
    }
}

// Function to validate sparse context (ensure we have valid selected tokens)
int validate_sparse_context(int *top_k_indices, int k) {
    int valid_count = 0;
    for (int i = 0; i < k; i++) {
        if (top_k_indices[i] != -1) {
            valid_count++;
        }
    }
    return valid_count;
}

// Main function to select sparse attention indices based on indexer scores
void apply_sparse_selection(float *indexer_scores, int vocab_size, int *selected_indices, int k) {
    get_top_k_indices(indexer_scores, vocab_size, selected_indices, k);
}

// Example usage function
void sparse_selection_demo(float *scores, int vocab_size) {
    int *top_k_indices = malloc(K_SPARSITY * sizeof(int));
    float *mask = malloc(vocab_size * sizeof(float));
    
    printf("Applying sparse selection with k=%d on vocab_size=%d\n", K_SPARSITY, vocab_size);
    
    apply_sparse_selection(scores, vocab_size, top_k_indices, K_SPARSITY);
    
    create_sparse_mask(top_k_indices, K_SPARSITY, vocab_size, mask);
    
    int valid_count = validate_sparse_context(top_k_indices, K_SPARSITY);
    printf("Selected %d valid tokens out of %d requested\n", valid_count, K_SPARSITY);
    
    // Print first few selected indices for verification
    printf("First 10 selected indices: ");
    for (int i = 0; i < 10 && i < valid_count; i++) {
        printf("%d ", top_k_indices[i]);
    }
    printf("\n");
    
    free(top_k_indices);
    free(mask);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <operation> [args...]\n", argv[0]);
        fprintf(stderr, "Operations:\n");
        fprintf(stderr, "  select <input_scores_file> <vocab_size> <k> - Select top-k indices from scores\n");
        fprintf(stderr, "  demo <vocab_size> - Run a demo with random scores\n");
        return 1;
    }
    
    char *operation = argv[1];
    
    if (strcmp(operation, "select") == 0) {
        if (argc < 5) {
            fprintf(stderr, "Usage: %s select <input_scores_file> <vocab_size> <k>\n", argv[0]);
            return 1;
        }
        
        char *input_scores_file = argv[2];
        int vocab_size = atoi(argv[3]);
        int k = atoi(argv[4]);
        
        // Load attention scores from file
        FILE *scores_file = fopen(input_scores_file, "r");
        if (!scores_file) {
            perror("Error opening input scores file");
            return 1;
        }
        
        float *attn_scores = malloc(vocab_size * sizeof(float));
        if (!attn_scores) {
            fprintf(stderr, "Memory allocation failed for attention scores\n");
            fclose(scores_file);
            return 1;
        }
        
        for (int i = 0; i < vocab_size; i++) {
            if (fscanf(scores_file, "%f", &attn_scores[i]) != 1) {
                fprintf(stderr, "Error reading score at index %d\n", i);
                free(attn_scores);
                fclose(scores_file);
                return 1;
            }
        }
        
        fclose(scores_file);
        
        // Perform top-k selection
        int *selected_indices = malloc(k * sizeof(int));
        get_top_k_indices(attn_scores, vocab_size, selected_indices, k);
        
        // Save selected indices to output file
        char output_path[1024];
        sprintf(output_path, "%s_selected_indices.txt", input_scores_file);
        FILE *output_file = fopen(output_path, "w");
        if (output_file) {
            for (int i = 0; i < k; i++) {
                fprintf(output_file, "%d\n", selected_indices[i]);
            }
            fclose(output_file);
            printf("Top-%d indices saved to %s\n", k, output_path);
        } else {
            perror("Error opening output file for selected indices");
            free(selected_indices);
            free(attn_scores);
            return 1;
        }
        
        // Also create and save the sparse mask
        char mask_path[1024];
        sprintf(mask_path, "%s_sparse_mask.txt", input_scores_file);
        float *mask = malloc(vocab_size * sizeof(float));
        create_sparse_mask(selected_indices, k, vocab_size, mask);
        
        FILE *mask_file = fopen(mask_path, "w");
        if (mask_file) {
            for (int i = 0; i < vocab_size; i++) {
                fprintf(mask_file, "%.6f\n", mask[i]);
            }
            fclose(mask_file);
            printf("Sparse mask saved to %s\n", mask_path);
        } else {
            perror("Error opening output file for sparse mask");
        }
        
        free(selected_indices);
        free(attn_scores);
        free(mask);
        
    } else if (strcmp(operation, "demo") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s demo <vocab_size>\n", argv[0]);
            return 1;
        }
        
        int vocab_size = atoi(argv[2]);
        
        // Generate random scores for demo
        float *random_scores = malloc(vocab_size * sizeof(float));
        srand(time(NULL));
        for (int i = 0; i < vocab_size; i++) {
            random_scores[i] = ((float)rand() / RAND_MAX);
        }
        
        sparse_selection_demo(random_scores, vocab_size);
        
        free(random_scores);
        
    } else {
        fprintf(stderr, "Unknown operation: %s\n", operation);
        return 1;
    }
    
    return 0;
}