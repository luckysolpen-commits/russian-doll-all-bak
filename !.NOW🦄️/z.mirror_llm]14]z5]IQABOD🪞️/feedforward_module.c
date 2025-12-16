#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_VOCAB_SIZE 100000
#define EMBED_DIM 16
#define FF_HIDDEN (EMBED_DIM * 4)

// Safe memory freeing macro to prevent double free errors
#define SAFE_FREE(ptr) do { if (ptr) { free(ptr); ptr = NULL; } } while(0)

// Vocabulary structure to match curriculum format
typedef struct {
    char **words;
    int *word_to_id;
    float *embeddings;      // [vocab_size * embed_dim] concatenated embeddings
    float *rope_pos_enc;    // [vocab_size * embed_dim] RoPE positional encodings
    // Human-readable/usable biases - more meaningful for architecture understanding
    float *attention_bias;  // [vocab_size] attention bias values (new weight category - human-usable)
    float *ffn_bias;        // [vocab_size] feed-forward network bias values (new weight category - human-usable)
    // Internal computer-used weights and biases
    float *weight1;         // [vocab_size] weight1 values
    float *weight2;         // [vocab_size] weight2 values
    float *bias1;           // [vocab_size] bias1 values 
    float *bias2;           // [vocab_size] bias2 values
    float *bias3;           // [vocab_size] bias3 values
    float *bias4;           // [vocab_size] bias4 values
    // Q, K, V projections for attention mechanism (for shared encoder/decoder use)
    float *q_proj;          // [vocab_size * embed_dim] query projections
    float *k_proj;          // [vocab_size * embed_dim] key projections  
    float *v_proj;          // [vocab_size * embed_dim] value projections
    char **notes;           // [vocab_size] user notes for each token
    int vocab_size;
    int max_size;
} SimpleVocab;

// FUNCTION DECLARATIONS

void matmul(float *xout, float *x, float *w, int n, int d);
void swiglu(float* xb, float* hb, float* hb2, int hidden_dim);
int process_feedforward_with_curriculum(const char* vocab_file, int token_id, float* input_activations, float* output);
int load_model_from_vocab_file(const char* vocab_file, void* model, SimpleVocab* vocab);

// MAIN FUNCTION
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <command> [args...]\n", argv[0]);
        printf("Commands:\n");
        printf("  process <vocab_file> <token_id> - Process feedforward using curriculum parameters\n");
        printf("  swiglu <hidden_dim> - Apply SwiGLU activation function\n");
        printf("  matmul <n> <d> - Perform matrix multiplication\n");
        printf("  load_params <vocab_file> - Load FFN parameters from curriculum\n");
        return 1;
    }
    
    char* command = argv[1];
    
    if (strcmp(command, "process") == 0) {
        if (argc < 4) {
            printf("Error: Missing vocab_file and token_id\n");
            printf("Usage: %s process <vocab_file> <token_id>\n", argv[0]);
            return 1;
        }
        
        char* vocab_file = argv[2];
        int token_id = atoi(argv[3]);
        
        printf("Processing feedforward: vocab_file=%s, token_id=%d\n", vocab_file, token_id);
        
        // Allocate input and output buffers
        float* input_activations = calloc(EMBED_DIM, sizeof(float));
        float* output = calloc(EMBED_DIM, sizeof(float));
        
        // Fill input with sample values based on token
        for (int i = 0; i < EMBED_DIM; i++) {
            input_activations[i] = (float)(i * token_id % 7) * 0.1f - 0.3f;
        }
        
        // Process feedforward using curriculum parameters
        int result = process_feedforward_with_curriculum(vocab_file, token_id, input_activations, output);
        
        if (result == 0) {
            printf("Feedforward processing completed successfully\n");
            // Print first few outputs as example
            printf("First output (first 5 dims): ");
            for (int i = 0; i < 5 && i < EMBED_DIM; i++) {
                printf("%.3f ", output[i]);
            }
            printf("...\n");
        }
        
        free(input_activations);
        free(output);
        
    } else if (strcmp(command, "swiglu") == 0) {
        if (argc < 3) {
            printf("Error: Missing hidden_dim\n");
            return 1;
        }
        
        int hidden_dim = atoi(argv[2]);
        
        // Create sample vectors
        float* xb = calloc(hidden_dim, sizeof(float));
        float* hb = malloc(hidden_dim * sizeof(float));
        float* hb2 = malloc(hidden_dim * sizeof(float));
        
        // Initialize with sample values
        for (int i = 0; i < hidden_dim; i++) {
            hb[i] = (float)(i % 5) * 0.1f - 0.2f;  // Values between -0.2 and 0.2
            hb2[i] = (float)(i % 3) * 0.1f;        // Values between 0 and 0.2
        }
        
        printf("Before SwiGLU: ");
        for (int i = 0; i < 5 && i < hidden_dim; i++) {
            printf("%.3f ", hb[i]);
        }
        printf("...\n");
        
        // Apply SwiGLU
        swiglu(xb, hb, hb2, hidden_dim);
        
        printf("After SwiGLU: ");
        for (int i = 0; i < 5 && i < hidden_dim; i++) {
            printf("%.3f ", xb[i]);
        }
        printf("...\n");
        
        free(xb);
        free(hb);
        free(hb2);
        
    } else if (strcmp(command, "matmul") == 0) {
        if (argc < 4) {
            printf("Error: Missing n and d\n");
            return 1;
        }
        
        int n = atoi(argv[2]);
        int d = atoi(argv[3]);
        
        // Create sample vectors and weight matrix
        float* x = malloc(n * sizeof(float));
        float* w = malloc(d * n * sizeof(float));
        float* xout = malloc(d * sizeof(float));
        
        // Initialize with sample values
        for (int i = 0; i < n; i++) {
            x[i] = (float)i * 0.1f;
        }
        for (int i = 0; i < d * n; i++) {
            w[i] = (float)(i % 10) * 0.01f;
        }
        
        printf("Input vector (first 5): ");
        for (int i = 0; i < 5 && i < n; i++) {
            printf("%.3f ", x[i]);
        }
        printf("...\n");
        
        // Perform matrix multiplication
        matmul(xout, x, w, n, d);
        
        printf("Output vector (first 5): ");
        for (int i = 0; i < 5 && i < d; i++) {
            printf("%.3f ", xout[i]);
        }
        printf("...\n");
        
        free(x);
        free(w);
        free(xout);
        
    } else if (strcmp(command, "load_params") == 0) {
        if (argc < 3) {
            printf("Error: Missing vocab_file\n");
            printf("Usage: %s load_params <vocab_file>\n", argv[0]);
            return 1;
        }
        
        char* vocab_file = argv[2];
        
        // Initialize vocabulary to load parameters
        SimpleVocab vocab;
        int result = load_model_from_vocab_file(vocab_file, NULL, &vocab);
        if (result != 0) {
            printf("Error loading parameters from curriculum file\n");
            return result;
        }
        
        printf("Successfully loaded curriculum with %d tokens:\n", vocab.vocab_size);
        
        // Show some loaded parameters as verification
        if (vocab.vocab_size > 0) {
            printf("Sample FFN bias for token 0: %.6f\n", vocab.ffn_bias[0]);
            printf("Sample weight1 for token 0: %.6f\n", vocab.weight1[0]);
            printf("Sample weight2 for token 0: %.6f\n", vocab.weight2[0]);
        }
        
        // Free vocabulary memory
        for (int i = 0; i < vocab.vocab_size; i++) {
            free(vocab.words[i]);
            if (vocab.notes[i]) free(vocab.notes[i]);
        }
        SAFE_FREE(vocab.words);
        SAFE_FREE(vocab.word_to_id);
        SAFE_FREE(vocab.embeddings);
        SAFE_FREE(vocab.rope_pos_enc);
        SAFE_FREE(vocab.weight1);
        SAFE_FREE(vocab.weight2);
        SAFE_FREE(vocab.bias1);
        SAFE_FREE(vocab.bias2);
        SAFE_FREE(vocab.bias3);
        SAFE_FREE(vocab.bias4);
        SAFE_FREE(vocab.attention_bias);
        SAFE_FREE(vocab.ffn_bias);
        SAFE_FREE(vocab.q_proj);
        SAFE_FREE(vocab.k_proj);
        SAFE_FREE(vocab.v_proj);
        SAFE_FREE(vocab.notes);
        
    } else {
        printf("Error: Unknown command '%s'\n", command);
        return 1;
    }
    
    return 0;
}

void matmul(float *xout, float *x, float *w, int n, int d) {
    // W (d,n) @ x (n,) -> xout (d,)
    // by far the most amount of time is spent inside this little function
    for (int i = 0; i < d; i++) {
        float val = 0.0f;
        for (int j = 0; j < n; j++) {
            val += w[i * n + j] * x[j];
        }
        xout[i] = val;
    }
}

void swiglu(float* xb, float* hb, float* hb2, int hidden_dim) {
    for (int i = 0; i < hidden_dim; i++) {
        float val = hb[i];
        // silu(x)=x*σ(x), where σ(x) is the logistic sigmoid
        val *= (1.0f / (1.0f + expf(-val)));
        // elementwise multiply with w3(x)
        val *= hb2[i];
        xb[i] = val;
    }
}

// Function to process feedforward using parameters from curriculum
int process_feedforward_with_curriculum(const char* vocab_file, int token_id, float* input_activations, float* output) {
    printf("Processing feedforward with curriculum: %s, token_id: %d\n", vocab_file, token_id);
    
    // Load vocabulary parameters from curriculum file
    SimpleVocab vocab;
    int result = load_model_from_vocab_file(vocab_file, NULL, &vocab);
    if (result != 0) {
        printf("Error loading vocabulary from curriculum\n");
        return result;
    }
    
    // Check if token_id is valid
    if (token_id < 0 || token_id >= vocab.vocab_size) {
        printf("Warning: Token ID %d out of range [0, %d)\n", token_id, vocab.vocab_size);
        token_id = 0; // Use first token as fallback
    }
    
    // Use the token-specific parameters from the curriculum to process activations
    for (int d = 0; d < EMBED_DIM; d++) {
        // Get input activation
        float input_val = input_activations[d];
        
        // Apply feedforward network transformations using curriculum parameters
        // This simulates what would happen in a real FFN layer
        
        // Apply weight1 from curriculum (gate projection) 
        float gate_val = input_val * vocab.weight1[token_id];
        gate_val += vocab.bias1[token_id];  // Add bias1
        
        // Apply weight2 from curriculum (up projection) 
        float up_val = input_val * vocab.weight2[token_id];
        up_val += vocab.bias2[token_id];  // Add bias2
        
        // Apply swiglu-like operation: silu(gate) * up
        float silu_gate = gate_val / (1.0f + expf(-gate_val)); // silu(x) = x * sigmoid(x)
        float ff_output = silu_gate * up_val;
        
        // Add FFN bias
        ff_output += vocab.ffn_bias[token_id];
        
        // Store output
        output[d] = ff_output;
    }
    
    // Free vocabulary memory
    for (int i = 0; i < vocab.vocab_size; i++) {
        free(vocab.words[i]);
        if (vocab.notes[i]) free(vocab.notes[i]);
    }
    SAFE_FREE(vocab.words);
    SAFE_FREE(vocab.word_to_id);
    SAFE_FREE(vocab.embeddings);
    SAFE_FREE(vocab.rope_pos_enc);
    SAFE_FREE(vocab.weight1);
    SAFE_FREE(vocab.weight2);
    SAFE_FREE(vocab.bias1);
    SAFE_FREE(vocab.bias2);
    SAFE_FREE(vocab.bias3);
    SAFE_FREE(vocab.bias4);
    SAFE_FREE(vocab.attention_bias);
    SAFE_FREE(vocab.ffn_bias);
    SAFE_FREE(vocab.q_proj);
    SAFE_FREE(vocab.k_proj);
    SAFE_FREE(vocab.v_proj);
    SAFE_FREE(vocab.notes);
    
    return 0; // Success
}

// Function to load model from vocabulary file (same implementation as in other modules)
int load_model_from_vocab_file(const char* vocab_file, void* model, SimpleVocab* vocab) {
    printf("Loading model from vocab file: %s\n", vocab_file);
    
    FILE *file = fopen(vocab_file, "r");
    if (!file) {
        printf("Error: could not open vocabulary file %s\n", vocab_file);
        return -1;
    }
    
    // Read file content to count lines first (excluding header)
    char line[1024];
    int line_count = 0;
    
    // Read header line first
    if (fgets(line, sizeof(line), file) == NULL) {
        printf("Error: could not read header from vocabulary file\n");
        fclose(file);
        return -1;
    }
    
    // Count remaining lines
    while (fgets(line, sizeof(line), file) != NULL) {
        line_count++;
    }
    rewind(file);
    
    // Skip header line
    if (fgets(line, sizeof(line), file) == NULL) {
        printf("Error: could not read header from vocabulary file\n");
        fclose(file);
        return -1;
    }
    
    // Initialize vocabulary from file
    vocab->vocab_size = line_count;
    vocab->max_size = MAX_VOCAB_SIZE;
    vocab->words = malloc(vocab->vocab_size * sizeof(char*));
    vocab->word_to_id = malloc(vocab->vocab_size * sizeof(int));
    vocab->embeddings = malloc(vocab->vocab_size * EMBED_DIM * sizeof(float));
    vocab->rope_pos_enc = malloc(vocab->vocab_size * EMBED_DIM * sizeof(float));
    vocab->weight1 = malloc(vocab->vocab_size * sizeof(float));
    vocab->weight2 = malloc(vocab->vocab_size * sizeof(float));
    vocab->bias1 = malloc(vocab->vocab_size * sizeof(float));
    vocab->bias2 = malloc(vocab->vocab_size * sizeof(float));
    vocab->bias3 = malloc(vocab->vocab_size * sizeof(float));
    vocab->bias4 = malloc(vocab->vocab_size * sizeof(float));
    vocab->attention_bias = malloc(vocab->vocab_size * sizeof(float));
    vocab->ffn_bias = malloc(vocab->vocab_size * sizeof(float));
    vocab->q_proj = malloc(vocab->vocab_size * EMBED_DIM * sizeof(float));
    vocab->k_proj = malloc(vocab->vocab_size * EMBED_DIM * sizeof(float));
    vocab->v_proj = malloc(vocab->vocab_size * EMBED_DIM * sizeof(float));
    vocab->notes = malloc(vocab->vocab_size * sizeof(char*));
    
    // Read vocabulary entries
    for (int i = 0; i < vocab->vocab_size; i++) {
        int index;
        char word[1000];
        char note_str[1000];  // Buffer for note string
        float embedding, pe_value, attention_bias_val, ffn_bias_val, weight1_val, weight2_val, bias1_val, bias2_val, bias3_val, bias4_val, q_val, k_val, v_val;
        
        if (fscanf(file, "%d %s %f %f %f %f %f %f %f %f %f %f %f %f %f %999s", 
                   &index, word, &embedding, &pe_value, 
                   &attention_bias_val, &ffn_bias_val,
                   &weight1_val, &weight2_val, 
                   &bias1_val, &bias2_val, &bias3_val, &bias4_val,
                   &q_val, &k_val, &v_val, note_str) != 16) {
            printf("Error: could not read vocabulary entry %d properly\n", i);
            fclose(file);
            return -1;
        }
        
        // Store word
        vocab->words[i] = malloc((strlen(word) + 1) * sizeof(char));
        strcpy(vocab->words[i], word);
        vocab->word_to_id[i] = i;
        
        // Initialize embedding (for now, using the embedding value from file as first dimension)
        for (int d = 0; d < EMBED_DIM; d++) {
            if (d == 0) {
                vocab->embeddings[i * EMBED_DIM + d] = embedding;
            } else {
                // For other dimensions, we'll initialize using a deterministic approach
                unsigned long hash = 5381;
                int c;
                const char *str = word;
                while ((c = *str++))
                    hash = ((hash << 5) + hash) + c;
                
                vocab->embeddings[i * EMBED_DIM + d] = (float)(hash % 1000000) / 1000000.0f + (float)d * 0.001f;
            }
        }
        
        // Initialize RoPE positional encoding (for now, using the pe value from file as first dimension)
        for (int d = 0; d < EMBED_DIM; d++) {
            if (d == 0) {
                vocab->rope_pos_enc[i * EMBED_DIM + d] = pe_value;
            } else {
                // Generate RoPE based on position i and dimension d
                float freq = 1.0f / powf(10000.0f, (float)(d % (EMBED_DIM/2)) / (EMBED_DIM/2));
                float angle = i * freq;
                
                if (d % 2 == 0) {
                    vocab->rope_pos_enc[i * EMBED_DIM + d] = sinf(angle);
                } else {
                    vocab->rope_pos_enc[i * EMBED_DIM + d] = cosf(angle);
                }
            }
        }
        
        // Initialize Q, K, V projections based on the values from file (for first dimension)
        for (int d = 0; d < EMBED_DIM; d++) {
            if (d == 0) {
                vocab->q_proj[i * EMBED_DIM + d] = q_val;
                vocab->k_proj[i * EMBED_DIM + d] = k_val;
                vocab->v_proj[i * EMBED_DIM + d] = v_val;
            } else {
                // For other dimensions, use deterministic approach based on word content
                unsigned long hash = 5381;
                int c;
                const char *str = word;
                while ((c = *str++))
                    hash = ((hash << 5) + hash) + c;
                
                vocab->q_proj[i * EMBED_DIM + d] = (float)(hash % 1000000) / 1000000.0f + (float)d * 0.001f;
                vocab->k_proj[i * EMBED_DIM + d] = (float)(hash % 1000001) / 1000000.0f + (float)d * 0.001f;  // Slightly different hash
                vocab->v_proj[i * EMBED_DIM + d] = (float)(hash % 1000002) / 1000000.0f + (float)d * 0.001f;  // Slightly different hash
            }
        }
        
        // Store note string for this token
        vocab->notes[i] = malloc((strlen(note_str) + 1) * sizeof(char));
        strcpy(vocab->notes[i], note_str);
        
        // Store other values from vocab file in the new order (human-usable biases first)
        vocab->attention_bias[i] = attention_bias_val;
        vocab->ffn_bias[i] = ffn_bias_val;
        vocab->weight1[i] = weight1_val;
        vocab->weight2[i] = weight2_val;
        vocab->bias1[i] = bias1_val;
        vocab->bias2[i] = bias2_val;
        vocab->bias3[i] = bias3_val;
        vocab->bias4[i] = bias4_val;
    }
    
    fclose(file);
    
    printf("Loaded model with vocabulary from %s (%d tokens)\n", vocab_file, vocab->vocab_size);
    return 0; // Success
}