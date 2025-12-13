#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <libgen.h>

#define MAX_VOCAB_SIZE 100000
#define EMBEDDING_DIM 7
#define INDEXER_DIM 4
#define INDEXER_HEADS 2
#define K_SPARSITY 128

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

// DSA Attention Layer
typedef struct {
    // Lightning Indexer components
    float W_q_indexer[INDEXER_HEADS][EMBEDDING_DIM][INDEXER_DIM];
    float W_k_indexer[INDEXER_HEADS][EMBEDDING_DIM][INDEXER_DIM];
    float indexer_weights[INDEXER_HEADS];
    
    // Full attention components
    float W_q[EMBEDDING_DIM][EMBEDDING_DIM];
    float W_k[EMBEDDING_DIM][EMBEDDING_DIM];
    float W_v[EMBEDDING_DIM][EMBEDDING_DIM];
    float W_o[EMBEDDING_DIM][EMBEDDING_DIM];
} DSAAttentionLayer;

// Function to initialize DSA attention layer
void initialize_dsa_attention(DSAAttentionLayer *attention) {
    srand(time(NULL));
    
    // Initialize indexer components
    for (int h = 0; h < INDEXER_HEADS; h++) {
        for (int i = 0; i < EMBEDDING_DIM; i++) {
            for (int j = 0; j < INDEXER_DIM; j++) {
                attention->W_q_indexer[h][i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.05f;
                attention->W_k_indexer[h][i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.05f;
            }
        }
        attention->indexer_weights[h] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
    }
    
    // Initialize attention components
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        for (int j = 0; j < EMBEDDING_DIM; j++) {
            attention->W_q[i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
            attention->W_k[i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
            attention->W_v[i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
            attention->W_o[i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
        }
    }
}

// Function to save DSA attention to file
void save_dsa_attention(const char *filename, DSAAttentionLayer *attention) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Error opening file for writing");
        return;
    }
    
    // Save indexer weights
    for (int h = 0; h < INDEXER_HEADS; h++) {
        fprintf(file, "%f ", attention->indexer_weights[h]);
    }
    fprintf(file, "\n");
    
    // Save W_q_indexer
    for (int h = 0; h < INDEXER_HEADS; h++) {
        for (int i = 0; i < EMBEDDING_DIM; i++) {
            for (int j = 0; j < INDEXER_DIM; j++) {
                fprintf(file, "%f ", attention->W_q_indexer[h][i][j]);
            }
            fprintf(file, "\n");
        }
    }
    
    // Save W_k_indexer
    for (int h = 0; h < INDEXER_HEADS; h++) {
        for (int i = 0; i < EMBEDDING_DIM; i++) {
            for (int j = 0; j < INDEXER_DIM; j++) {
                fprintf(file, "%f ", attention->W_k_indexer[h][i][j]);
            }
            fprintf(file, "\n");
        }
    }
    
    // Save W_q
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        for (int j = 0; j < EMBEDDING_DIM; j++) {
            fprintf(file, "%f ", attention->W_q[i][j]);
        }
        fprintf(file, "\n");
    }
    
    // Save W_k
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        for (int j = 0; j < EMBEDDING_DIM; j++) {
            fprintf(file, "%f ", attention->W_k[i][j]);
        }
        fprintf(file, "\n");
    }
    
    // Save W_v
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        for (int j = 0; j < EMBEDDING_DIM; j++) {
            fprintf(file, "%f ", attention->W_v[i][j]);
        }
        fprintf(file, "\n");
    }
    
    // Save W_o
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        for (int j = 0; j < EMBEDDING_DIM; j++) {
            fprintf(file, "%f ", attention->W_o[i][j]);
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
}

// Function to load DSA attention from file
void load_dsa_attention(const char *filename, DSAAttentionLayer *attention) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        initialize_dsa_attention(attention);
        return;
    }
    
    char line[1024];
    char *token;
    
    // Skip the first comment line and read indexer weights from the next line
    while (fgets(line, sizeof(line), file) && line[0] == '#');  // Skip '# DSA Lightning Indexer weights'
    // Parse the indexer weights from the line we just read
    token = strtok(line, " \t\n");
    for (int h = 0; h < INDEXER_HEADS && token; h++) {
        attention->indexer_weights[h] = atof(token);
        token = strtok(NULL, " \t\n");
    }
    
    // Skip comment line for W_q_indexer
    while (fgets(line, sizeof(line), file) && line[0] == '#');
    
    // Read W_q_indexer values - read each line and parse tokens from it
    for (int h = 0; h < INDEXER_HEADS; h++) {
        for (int i = 0; i < EMBEDDING_DIM; i++) {
            if (!fgets(line, sizeof(line), file)) {
                // If we can't read the line, initialize with random values
                for (int j = 0; j < INDEXER_DIM; j++) {
                    attention->W_q_indexer[h][i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.05f;
                }
                continue;
            }
            token = strtok(line, " \t\n");
            int j;
            for (j = 0; j < INDEXER_DIM && token; j++) {
                attention->W_q_indexer[h][i][j] = atof(token);
                token = strtok(NULL, " \t\n");
            }
            // If we didn't read enough values, initialize the rest with random values
            for (; j < INDEXER_DIM; j++) {
                attention->W_q_indexer[h][i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.05f;
            }
        }
    }
    
    // Skip comment line for W_k_indexer
    while (fgets(line, sizeof(line), file) && line[0] == '#');
    
    // Read W_k_indexer values
    for (int h = 0; h < INDEXER_HEADS; h++) {
        for (int i = 0; i < EMBEDDING_DIM; i++) {
            if (!fgets(line, sizeof(line), file)) {
                // If we can't read the line, initialize with random values
                for (int j = 0; j < INDEXER_DIM; j++) {
                    attention->W_k_indexer[h][i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.05f;
                }
                continue;
            }
            token = strtok(line, " \t\n");
            int j;
            for (j = 0; j < INDEXER_DIM && token; j++) {
                attention->W_k_indexer[h][i][j] = atof(token);
                token = strtok(NULL, " \t\n");
            }
            // If we didn't read enough values, initialize the rest with random values
            for (; j < INDEXER_DIM; j++) {
                attention->W_k_indexer[h][i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.05f;
            }
        }
    }
    
    // Skip comment line for W_q
    while (fgets(line, sizeof(line), file) && line[0] == '#');
    
    // Read W_q values
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        if (!fgets(line, sizeof(line), file)) {
            // If we can't read the line, initialize with random values
            for (int j = 0; j < EMBEDDING_DIM; j++) {
                attention->W_q[i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
            }
            continue;
        }
        token = strtok(line, " \t\n");
        int j;
        for (j = 0; j < EMBEDDING_DIM && token; j++) {
            attention->W_q[i][j] = atof(token);
            token = strtok(NULL, " \t\n");
        }
        // If we didn't read enough values, initialize the rest with random values
        for (; j < EMBEDDING_DIM; j++) {
            attention->W_q[i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
        }
    }
    
    // Skip comment line for W_k
    while (fgets(line, sizeof(line), file) && line[0] == '#');
    
    // Read W_k values
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        if (!fgets(line, sizeof(line), file)) {
            // If we can't read the line, initialize with random values
            for (int j = 0; j < EMBEDDING_DIM; j++) {
                attention->W_k[i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
            }
            continue;
        }
        token = strtok(line, " \t\n");
        int j;
        for (j = 0; j < EMBEDDING_DIM && token; j++) {
            attention->W_k[i][j] = atof(token);
            token = strtok(NULL, " \t\n");
        }
        // If we didn't read enough values, initialize the rest with random values
        for (; j < EMBEDDING_DIM; j++) {
            attention->W_k[i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
        }
    }
    
    // Skip comment line for W_v
    while (fgets(line, sizeof(line), file) && line[0] == '#');
    
    // Read W_v values
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        if (!fgets(line, sizeof(line), file)) {
            // If we can't read the line, initialize with random values
            for (int j = 0; j < EMBEDDING_DIM; j++) {
                attention->W_v[i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
            }
            continue;
        }
        token = strtok(line, " \t\n");
        int j;
        for (j = 0; j < EMBEDDING_DIM && token; j++) {
            attention->W_v[i][j] = atof(token);
            token = strtok(NULL, " \t\n");
        }
        // If we didn't read enough values, initialize the rest with random values
        for (; j < EMBEDDING_DIM; j++) {
            attention->W_v[i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
        }
    }
    
    // Skip comment line for W_o
    while (fgets(line, sizeof(line), file) && line[0] == '#');
    
    // Read W_o values
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        if (!fgets(line, sizeof(line), file)) {
            // If we can't read the line, initialize with random values
            for (int j = 0; j < EMBEDDING_DIM; j++) {
                attention->W_o[i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
            }
            continue;
        }
        token = strtok(line, " \t\n");
        int j;
        for (j = 0; j < EMBEDDING_DIM && token; j++) {
            attention->W_o[i][j] = atof(token);
            token = strtok(NULL, " \t\n");
        }
        // If we didn't read enough values, initialize the rest with random values
        for (; j < EMBEDDING_DIM; j++) {
            attention->W_o[i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
        }
    }
    
    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <operation> [args...]\n", argv[0]);
        fprintf(stderr, "Operations:\n");
        fprintf(stderr, "  init <model_file>                  - Initialize DSA attention and save to file\n");
        fprintf(stderr, "  forward <model_file> <vocab_file> <word_index>  - Forward pass using DSA model and input\n");
        return 1;
    }
    
    char *operation = argv[1];
    DSAAttentionLayer attention;
    
    if (strcmp(operation, "init") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s init <model_file>\n", argv[0]);
            return 1;
        }
        
        initialize_dsa_attention(&attention);
        save_dsa_attention(argv[2], &attention);
        printf("DSA Attention initialized and saved to %s\n", argv[2]);
        
    } else if (strcmp(operation, "forward") == 0) {
        if (argc < 5) {
            fprintf(stderr, "Usage: %s forward <model_file> <vocab_file> <word_index>\n", argv[0]);
            return 1;
        }
        
        // Load model
        load_dsa_attention(argv[2], &attention);
        
        // Load vocabulary
        FILE *vocab_file = fopen(argv[3], "r");
        if (!vocab_file) {
            perror("Error opening vocab file");
            return 1;
        }
        
        struct VocabEntry *vocab = malloc(MAX_VOCAB_SIZE * sizeof(struct VocabEntry));
        if (!vocab) {
            perror("Failed to allocate memory for vocabulary");
            fclose(vocab_file);
            return 1;
        }
        
        char line[1024];
        int vocab_size = 0;
        
        // Skip header
        fgets(line, sizeof(line), vocab_file);
        
        while (fgets(line, sizeof(line), vocab_file) && vocab_size < MAX_VOCAB_SIZE) {
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
            
            if(result == 9) {
                vocab_size++;
            }
        }
        
        fclose(vocab_file);
        
        int word_index = atoi(argv[4]);
        if (word_index < 0 || word_index >= vocab_size) {
            fprintf(stderr, "Invalid word index\n");
            free(vocab);
            return 1;
        }
        
        // Prepare input vector
        float input_vec[EMBEDDING_DIM] = {
            vocab[word_index].embedding, 
            vocab[word_index].pe, 
            vocab[word_index].weight,
            vocab[word_index].bias1, 
            vocab[word_index].bias2, 
            vocab[word_index].bias3, 
            vocab[word_index].bias4
        };
        
        // Compute context vector using the attention mechanism
        float context[EMBEDDING_DIM] = {0};
        
        // Compute Q, K, V
        float q[EMBEDDING_DIM], k[EMBEDDING_DIM], v[EMBEDDING_DIM];
        for(int j = 0; j < EMBEDDING_DIM; j++) {
            q[j] = k[j] = v[j] = 0;
            for(int l = 0; l < EMBEDDING_DIM; l++) {
                q[j] += input_vec[l] * attention.W_q[l][j];
                k[j] += input_vec[l] * attention.W_k[l][j];
                v[j] += input_vec[l] * attention.W_v[l][j];
            }
        }
        
        // Compute attention scores for all vocabulary entries
        float *attn_scores = malloc(vocab_size * sizeof(float));
        for(int i = 0; i < vocab_size; i++) {
            float token_k[EMBEDDING_DIM] = {
                vocab[i].embedding, vocab[i].pe, vocab[i].weight,
                vocab[i].bias1, vocab[i].bias2, vocab[i].bias3, vocab[i].bias4
            };
            
            // Compute raw attention score
            attn_scores[i] = 0;
            for(int l = 0; l < EMBEDDING_DIM; l++) {
                attn_scores[i] += q[l] * token_k[l];
            }
        }
        
        // Apply lightning indexer to get top-k indices
        int *top_k_indices = malloc(K_SPARSITY * sizeof(int));
        for(int i = 0; i < K_SPARSITY; i++) {
            top_k_indices[i] = (i < vocab_size) ? i : -1; // Use first tokens for this simple example
        }
        
        // Apply softmax only to top-k indices
        float max_score = attn_scores[top_k_indices[0]];
        for(int i = 1; i < K_SPARSITY && top_k_indices[i] != -1; i++) {
            if(attn_scores[top_k_indices[i]] > max_score) {
                max_score = attn_scores[top_k_indices[i]];
            }
        }
        
        // Compute exp and sum for normalization
        float sum = 0.0f;
        for(int i = 0; i < K_SPARSITY && top_k_indices[i] != -1; i++) {
            attn_scores[top_k_indices[i]] = expf(attn_scores[top_k_indices[i]] - max_score);
            sum += attn_scores[top_k_indices[i]];
        }
        
        // Normalize attention scores
        if(sum > 1e-9f) {
            for(int i = 0; i < K_SPARSITY && top_k_indices[i] != -1; i++) {
                attn_scores[top_k_indices[i]] /= sum;
            }
        }
        
        // Compute context vector from top-k tokens
        for(int i = 0; i < K_SPARSITY && top_k_indices[i] != -1; i++) {
            int idx = top_k_indices[i];
            float token_v[EMBEDDING_DIM] = {
                vocab[idx].embedding, vocab[idx].pe, vocab[idx].weight,
                vocab[idx].bias1, vocab[idx].bias2, vocab[idx].bias3, vocab[idx].bias4
            };
            
            for(int l = 0; l < EMBEDDING_DIM; l++) {
                context[l] += attn_scores[idx] * token_v[l];
            }
        }
        
        // Apply output projection
        float output[EMBEDDING_DIM] = {0};
        for(int l = 0; l < EMBEDDING_DIM; l++) {
            for(int m = 0; m < EMBEDDING_DIM; m++) {
                output[l] += context[m] * attention.W_o[m][l];
            }
        }
        
        // Create output file paths
        char* vocab_path_copy = strdup(argv[3]);
        char* output_dir = dirname(vocab_path_copy);
        
        char attn_scores_path[1024];
        sprintf(attn_scores_path, "%s/dsa_attn_scores.txt", output_dir);
        char context_path[1024];
        sprintf(context_path, "%s/dsa_context.txt", output_dir);
        
        // Save attention scores
        FILE *attn_file = fopen(attn_scores_path, "w");
        if (attn_file) {
            for (int i = 0; i < vocab_size; i++) {
                fprintf(attn_file, "%f\n", attn_scores[i]);
            }
            fclose(attn_file);
        }
        
        // Save context vector
        FILE *context_file = fopen(context_path, "w");
        if (context_file) {
            for (int i = 0; i < EMBEDDING_DIM; i++) {
                fprintf(context_file, "%f ", output[i]);
            }
            fprintf(context_file, "\n");
            fclose(context_file);
        }
        
        printf("DSA forward pass completed. Results saved to %s and %s\n", 
               attn_scores_path, context_path);
        
        free(vocab);
        free(attn_scores);
        free(top_k_indices);
        free(vocab_path_copy);
        
    } else {
        fprintf(stderr, "Unknown operation: %s\n", operation);
        return 1;
    }
    
    return 0;
}