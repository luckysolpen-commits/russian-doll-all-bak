#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <libgen.h>

#define MAX_VOCAB_SIZE 100000
#define EMBEDDING_DIM 7
#define INDEXER_DIM 4  // Lower dimensional space for indexer
#define INDEXER_HEADS 2  // Fewer heads for indexer

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

// Lightning Indexer Layer
typedef struct {
    // Low-dimensional projection matrices for indexer
    float W_q[INDEXER_HEADS][EMBEDDING_DIM][INDEXER_DIM];
    float W_k[INDEXER_HEADS][EMBEDDING_DIM][INDEXER_DIM];
    float indexer_weights[INDEXER_HEADS];  // Scalar weights for indexer heads
} LightningIndexer;

// Function to initialize lightning indexer with small random weights
void initialize_lightning_indexer(LightningIndexer *indexer) {
    for (int h = 0; h < INDEXER_HEADS; h++) {
        for (int i = 0; i < EMBEDDING_DIM; i++) {
            for (int j = 0; j < INDEXER_DIM; j++) {
                // Initialize with smaller random values for efficiency
                indexer->W_q[h][i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.05f;
                indexer->W_k[h][i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.05f;
            }
        }
        // Initialize scalar indexer weight
        indexer->indexer_weights[h] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
    }
}

// Apply ReLU activation
float relu(float x) {
    return (x > 0.0f) ? x : 0.0f;
}

// Compute indexer scores efficiently using low-dimensional projections
void compute_indexer_scores(float *input_vec, LightningIndexer *indexer, 
                           struct VocabEntry *vocab, int vocab_size, 
                           float *indexer_scores) {
    float q_proj[INDEXER_HEADS][INDEXER_DIM];
    float k_proj[INDEXER_HEADS][INDEXER_DIM];
    
    // Compute query projections for each head
    for (int h = 0; h < INDEXER_HEADS; h++) {
        for (int j = 0; j < INDEXER_DIM; j++) {
            q_proj[h][j] = 0;
            for (int l = 0; l < EMBEDDING_DIM; l++) {
                q_proj[h][j] += input_vec[l] * indexer->W_q[h][l][j];
            }
        }
    }
    
    // Compute indexer scores for each token in the vocabulary
    for (int t = 0; t < vocab_size; t++) {
        float token_embedding[EMBEDDING_DIM] = {vocab[t].embedding, vocab[t].pe, vocab[t].weight,
                                                vocab[t].bias1, vocab[t].bias2, vocab[t].bias3, vocab[t].bias4};
        
        float total_score = 0.0f;
        
        // Compute score for each indexer head
        for (int h = 0; h < INDEXER_HEADS; h++) {
            // Compute key projection for this token
            for (int j = 0; j < INDEXER_DIM; j++) {
                k_proj[h][j] = 0;
                for (int l = 0; l < EMBEDDING_DIM; l++) {
                    k_proj[h][j] += token_embedding[l] * indexer->W_k[h][l][j];
                }
            }
            
            // Compute dot product between projected query and key
            float dot_product = 0.0f;
            for (int j = 0; j < INDEXER_DIM; j++) {
                dot_product += q_proj[h][j] * k_proj[h][j];
            }
            
            // Apply ReLU and multiply by learned scalar weight
            total_score += indexer->indexer_weights[h] * relu(dot_product);
        }
        
        indexer_scores[t] = total_score;
    }
}

// Function to save the indexer weights to file
void save_lightning_indexer(const char *filename, LightningIndexer *indexer) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Error opening indexer file for writing");
        return;
    }
    
    // Save indexer weights
    fprintf(file, "# Indexer scalar weights\n");
    for (int h = 0; h < INDEXER_HEADS; h++) {
        fprintf(file, "%f ", indexer->indexer_weights[h]);
    }
    fprintf(file, "\n");
    
    // Save W_q matrices
    fprintf(file, "# W_q matrices\n");
    for (int h = 0; h < INDEXER_HEADS; h++) {
        for (int i = 0; i < EMBEDDING_DIM; i++) {
            for (int j = 0; j < INDEXER_DIM; j++) {
                fprintf(file, "%f ", indexer->W_q[h][i][j]);
            }
            fprintf(file, "\n");
        }
    }
    
    // Save W_k matrices
    fprintf(file, "# W_k matrices\n");
    for (int h = 0; h < INDEXER_HEADS; h++) {
        for (int i = 0; i < EMBEDDING_DIM; i++) {
            for (int j = 0; j < INDEXER_DIM; j++) {
                fprintf(file, "%f ", indexer->W_k[h][i][j]);
            }
            fprintf(file, "\n");
        }
    }
    
    fclose(file);
}

// Function to load indexer weights from file
void load_lightning_indexer(const char *filename, LightningIndexer *indexer) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        // Initialize with random weights if file doesn't exist
        initialize_lightning_indexer(indexer);
        return;
    }
    
    char line[1024];
    int pos = 0;
    
    // Skip comments and read indexer weights
    while (fgets(line, sizeof(line), file) && line[0] == '#');
    
    char *token = strtok(line, " \t\n");
    for (int h = 0; h < INDEXER_HEADS && token; h++) {
        indexer->indexer_weights[h] = atof(token);
        token = strtok(NULL, " \t\n");
    }
    
    // Skip comments and read W_q matrices
    while (fgets(line, sizeof(line), file) && line[0] == '#');
    
    for (int h = 0; h < INDEXER_HEADS; h++) {
        for (int i = 0; i < EMBEDDING_DIM; i++) {
            for (int j = 0; j < INDEXER_DIM; j++) {
                if (fscanf(file, "%f", &indexer->W_q[h][i][j]) != 1) {
                    fprintf(stderr, "Error reading W_q[%d][%d][%d]\n", h, i, j);
                }
            }
        }
    }
    
    // Skip comments and read W_k matrices
    while (fgets(line, sizeof(line), file) && line[0] == '#');
    
    for (int h = 0; h < INDEXER_HEADS; h++) {
        for (int i = 0; i < EMBEDDING_DIM; i++) {
            for (int j = 0; j < INDEXER_DIM; j++) {
                if (fscanf(file, "%f", &indexer->W_k[h][i][j]) != 1) {
                    fprintf(stderr, "Error reading W_k[%d][%d][%d]\n", h, i, j);
                }
            }
        }
    }
    
    fclose(file);
}

// Function to compute indexer scores from saved model and vocab
void indexer_forward(const char *model_file, const char *vocab_file, const char *input_file, 
                    const char *output_file) {
    LightningIndexer indexer;
    load_lightning_indexer(model_file, &indexer);
    
    // Load vocabulary
    FILE *vocab_f = fopen(vocab_file, "r");
    if (!vocab_f) {
        perror("Error opening vocab file");
        return;
    }
    
    struct VocabEntry *vocab = malloc(MAX_VOCAB_SIZE * sizeof(struct VocabEntry));
    if (!vocab) {
        perror("Failed to allocate memory for vocabulary");
        fclose(vocab_f);
        return;
    }
    
    char line[1024];
    int vocab_size = 0;
    
    // Skip header line
    fgets(line, sizeof(line), vocab_f);
    
    while (fgets(line, sizeof(line), vocab_f) && vocab_size < MAX_VOCAB_SIZE) {
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
        
        if(result == 9) {  // Make sure all 9 values were read
            vocab_size++;
        }
    }
    
    fclose(vocab_f);
    
    // Load input vector
    FILE *input_f = fopen(input_file, "r");
    if (!input_f) {
        perror("Error opening input file");
        free(vocab);
        return;
    }
    
    float input_vec[EMBEDDING_DIM];
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        if (fscanf(input_f, "%f", &input_vec[i]) != 1) {
            fprintf(stderr, "Error reading input vector\n");
            fclose(input_f);
            free(vocab);
            return;
        }
    }
    fclose(input_f);
    
    // Compute indexer scores
    float *scores = malloc(vocab_size * sizeof(float));
    compute_indexer_scores(input_vec, &indexer, vocab, vocab_size, scores);
    
    // Save scores to output file
    FILE *output_f = fopen(output_file, "w");
    if (output_f) {
        for (int i = 0; i < vocab_size; i++) {
            fprintf(output_f, "%f\n", scores[i]);
        }
        fclose(output_f);
        printf("Lightning indexer scores computed and saved to %s\n", output_file);
    } else {
        perror("Error opening output file for indexer scores");
    }
    
    free(vocab);
    free(scores);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <operation> [args...]\n", argv[0]);
        fprintf(stderr, "Operations:\n");
        fprintf(stderr, "  init <model_file>                    - Initialize indexer and save to file\n");
        fprintf(stderr, "  forward <model_file> <vocab_file> <input_file> <output_file> - Compute indexer scores\n");
        return 1;
    }
    
    char *operation = argv[1];
    
    if (strcmp(operation, "init") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s init <model_file>\n", argv[0]);
            return 1;
        }
        
        LightningIndexer indexer;
        srand(time(NULL));
        initialize_lightning_indexer(&indexer);
        save_lightning_indexer(argv[2], &indexer);
        printf("Lightning Indexer initialized and saved to %s\n", argv[2]);
        
    } else if (strcmp(operation, "forward") == 0) {
        if (argc < 6) {
            fprintf(stderr, "Usage: %s forward <model_file> <vocab_file> <input_file> <output_file>\n", argv[0]);
            return 1;
        }
        
        indexer_forward(argv[2], argv[3], argv[4], argv[5]);
        
    } else {
        fprintf(stderr, "Unknown operation: %s\n", operation);
        return 1;
    }
    
    return 0;
}