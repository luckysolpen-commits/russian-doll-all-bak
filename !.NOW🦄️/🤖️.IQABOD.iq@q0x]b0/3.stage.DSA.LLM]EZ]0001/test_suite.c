#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_VOCAB_SIZE 100000
#define EMBEDDING_DIM 7

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

// Timing structure
typedef struct {
    double original_time;
    double dsa_time;
    int original_valid_tokens;
    int dsa_valid_tokens;
    double avg_original_score;
    double avg_dsa_score;
    double cosine_similarity;
} PerformanceMetrics;

// Function to calculate cosine similarity between two vectors
double cosine_similarity(float *vec1, float *vec2, int dim) {
    double dot_product = 0.0, norm1 = 0.0, norm2 = 0.0;
    
    for (int i = 0; i < dim; i++) {
        dot_product += vec1[i] * vec2[i];
        norm1 += vec1[i] * vec1[i];
        norm2 += vec2[i] * vec2[i];
    }
    
    if (norm1 == 0 || norm2 == 0) {
        return 0.0;  // Return 0 if one of the vectors is zero
    }
    
    return dot_product / (sqrt(norm1) * sqrt(norm2));
}

// Function to load vocab from file
int load_vocab(const char *filename, struct VocabEntry *vocab) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open vocabulary file %s\n", filename);
        return -1;
    }
    
    char line[1024];
    int vocab_size = 0;
    
    // Skip header
    fgets(line, sizeof(line), file);
    
    while (fgets(line, sizeof(line), file) && vocab_size < MAX_VOCAB_SIZE) {
        int result = sscanf(line, "%d %s %f %f %f %f %f %f %f",
                           &vocab[vocab_size].number,
                           vocab[vocab_size].word,
                           &vocab[vocab_size].embedding,
                           &vocab[vocab_size].pe,
                           &vocab[vocab_size].weight,
                           &vocab[vocab_size].bias1,
                           &vocab[vocab_size].bias2,
                           &vocab[vocab_size].bias3,
                           &vocab[vocab_size].bias4);
        
        if (result == 9) {
            vocab_size++;
        }
    }
    
    fclose(file);
    return vocab_size;
}

// Function to run original attention and measure performance
void run_original_attention(const char *vocab_file, const char *model_file, PerformanceMetrics *metrics) {
    printf("Running original attention test...\n");
    
    clock_t start = clock();
    
    // Formulate command to run original attention forward pass
    char cmd[1024];
    sprintf(cmd, "./+x/attention.+x forward %s %s 0", model_file, vocab_file);
    
    // Execute the command and capture timing
    int result = system(cmd);
    if (result != 0) {
        fprintf(stderr, "Original attention execution failed\n");
    }
    
    clock_t end = clock();
    metrics->original_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Original attention completed in %.4f seconds\n", metrics->original_time);
}

// Function to run DSA attention and measure performance
void run_dsa_attention(const char *vocab_file, const char *model_file, PerformanceMetrics *metrics) {
    printf("Running DSA attention test...\n");
    
    clock_t start = clock();
    
    // Formulate command to run DSA attention forward pass
    char cmd[1024];
    sprintf(cmd, "./dsa_attention forward %s %s 0", model_file, vocab_file);
    
    // Execute the command and capture timing
    int result = system(cmd);
    if (result != 0) {
        fprintf(stderr, "DSA attention execution failed\n");
    }
    
    clock_t end = clock();
    metrics->dsa_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("DSA attention completed in %.4f seconds\n", metrics->dsa_time);
}

// Function to read attention scores from file for comparison
double calculate_avg_attention_score(const char *score_file, int expected_size) {
    FILE *file = fopen(score_file, "r");
    if (!file) {
        fprintf(stderr, "Could not open score file: %s\n", score_file);
        return 0.0;
    }
    
    double sum = 0.0;
    float score;
    int count = 0;
    int valid_count = 0;
    
    while (fscanf(file, "%f", &score) == 1 && count < expected_size) {
        if (!isnan(score) && !isinf(score)) {
            sum += score;
            valid_count++;
        }
        count++;
    }
    
    fclose(file);
    
    return (valid_count > 0) ? sum / valid_count : 0.0;
}

// Function to load attention scores from file into array
int load_attention_scores(const char *filename, float *scores, int max_size) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Could not open attention scores file: %s\n", filename);
        return 0;
    }
    
    int count = 0;
    float score;
    while (fscanf(file, "%f", &score) == 1 && count < max_size) {
        scores[count] = score;
        count++;
    }
    
    fclose(file);
    return count;
}

// Function to run the full comparison test suite
void run_comparison_tests(const char *vocab_file, const char *original_model, const char *dsa_model) {
    printf("=== DSA vs Original Attention Comparison Test Suite ===\n\n");
    
    PerformanceMetrics metrics = {0};
    
    // Load vocabulary
    struct VocabEntry *vocab = malloc(MAX_VOCAB_SIZE * sizeof(struct VocabEntry));
    if (!vocab) {
        fprintf(stderr, "Error: Failed to allocate memory for vocabulary\n");
        return;
    }
    
    int vocab_size = load_vocab(vocab_file, vocab);
    if (vocab_size <= 0) {
        fprintf(stderr, "Error: Failed to load vocabulary\n");
        free(vocab);
        return;
    }
    
    printf("Loaded vocabulary with %d entries\n", vocab_size);
    
    // 1. Run Original Attention
    run_original_attention(vocab_file, original_model, &metrics);
    
    // 2. Run DSA Attention
    run_dsa_attention(vocab_file, dsa_model, &metrics);
    
    // 3. Calculate average attention scores for both implementations
    printf("\nCalculating average attention scores...\n");
    metrics.avg_original_score = calculate_avg_attention_score("./attn_scores.txt", vocab_size);
    metrics.avg_dsa_score = calculate_avg_attention_score("./dsa_attn_scores.txt", vocab_size);
    
    printf("Average original attention score: %.6f\n", metrics.avg_original_score);
    printf("Average DSA attention score: %.6f\n", metrics.avg_dsa_score);
    
    // 4. Load attention scores for cosine similarity calculation
    float *orig_scores = malloc(vocab_size * sizeof(float));
    float *dsa_scores = malloc(vocab_size * sizeof(float));
    
    if (orig_scores && dsa_scores) {
        int orig_count = load_attention_scores("./attn_scores.txt", orig_scores, vocab_size);
        int dsa_count = load_attention_scores("./dsa_attn_scores.txt", dsa_scores, vocab_size);
        
        if (orig_count > 0 && dsa_count > 0) {
            int min_count = (orig_count < dsa_count) ? orig_count : dsa_count;
            
            // Calculate cosine similarity between attention score distributions
            metrics.cosine_similarity = cosine_similarity(orig_scores, dsa_scores, min_count);
            printf("Cosine similarity between attention distributions: %.6f\n", metrics.cosine_similarity);
        }
    }
    
    if (orig_scores) free(orig_scores);
    if (dsa_scores) free(dsa_scores);
    
    // 5. Print performance summary
    printf("\n=== Performance Summary ===\n");
    printf("Vocabulary size: %d\n", vocab_size);
    printf("Original attention time: %.4f seconds\n", metrics.original_time);
    printf("DSA attention time: %.4f seconds\n", metrics.dsa_time);
    
    if (metrics.original_time > 0) {
        double speedup = metrics.original_time / metrics.dsa_time;
        printf("Speedup factor: %.2fx\n", speedup);
    }
    
    printf("Original avg score: %.6f\n", metrics.avg_original_score);
    printf("DSA avg score: %.6f\n", metrics.avg_dsa_score);
    printf("Cosine similarity: %.6f\n", metrics.cosine_similarity);
    
    // 6. Efficiency comparison (theoretical based on complexity)
    printf("\n=== Theoretical Complexity Comparison ===\n");
    printf("Original attention: O(L²) - where L is sequence length\n");
    printf("DSA attention: O(L*K) - where K is sparsity parameter (%d)\n", 128);  // Using the K_SPARSITY parameter from dsa_attention.c
    printf("Expected efficiency gain for long sequences: ~%.2fx (based on K/L ratio)\n", (double)vocab_size/128);
    
    free(vocab);
}

// Main test runner function
int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <vocab_file> <original_model_file> <dsa_model_file>\n", argv[0]);
        fprintf(stderr, "Example: %s ../vocab_model.txt attention_model.txt dsa_attention_model.txt\n", argv[0]);
        return 1;
    }
    
    const char *vocab_file = argv[1];
    const char *original_model = argv[2];
    const char *dsa_model = argv[3];
    
    printf("Starting DSA vs Original Attention Comparison Test\n");
    printf("Vocabulary file: %s\n", vocab_file);
    printf("Original model: %s\n", original_model);
    printf("DSA model: %s\n\n", dsa_model);
    
    run_comparison_tests(vocab_file, original_model, dsa_model);
    
    printf("\nComparison test completed!\n");
    return 0;
}