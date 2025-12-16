#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define EMBED_DIM 16
#define N_HEADS 4
#define N_LAYERS 2
#define SEQ_LEN 32
#define FF_HIDDEN (EMBED_DIM * 4)
#define KV_DIM (EMBED_DIM / N_HEADS)

// Safe memory freeing macro to prevent double free errors
#define SAFE_FREE(ptr) do { if (ptr) { free(ptr); ptr = NULL; } } while(0)

// Model structure (simplified for backprop)
typedef struct {
    // Token embedding table
    float *token_embedding_table; // [vocab_size * dim]
    
    // RMSNorm weights
    float *rms_att_weight; // [n_layers * dim] 
    float *rms_ffn_weight; // [n_layers * dim]
    float *rms_final_weight; // [dim]
    
    // Attention weights (multi-query attention like karp model)
    float *wq; // [n_layers * dim * dim] - query weights
    float *wk; // [n_layers * dim * kv_dim] - key weights
    float *wv; // [n_layers * dim * kv_dim] - value weights
    float *wo; // [n_layers * dim * dim] - output weights
    
    // Feed-forward network weights (SwiGLU like karp model)
    float *w1; // [n_layers * hidden_dim * dim] - gate projection
    float *w2; // [n_layers * dim * hidden_dim] - output projection
    float *w3; // [n_layers * hidden_dim * dim] - up projection
    
    // Output classifier weights
    float *wcls; // [dim * vocab_size]
    
    int dim;        // transformer dimension
    int hidden_dim; // for ffn layers
    int n_layers;   // number of layers
    int n_heads;    // number of query heads
    int n_kv_heads; // number of key/value heads (can be < query heads because of multiquery)
    int vocab_size; // vocabulary size
    int seq_len;    // max sequence length
    int kv_dim;     // kv dimension per head (dim/n_heads for multiquery)
} SimpleModel;

// Gradient structure for backpropagation - stores gradients for all model parameters
typedef struct {
    // Gradients for token embedding table
    float *grad_token_embedding_table; // [vocab_size * dim]
    
    // Gradients for RMSNorm weights
    float *grad_rms_att_weight; // [n_layers * dim] 
    float *grad_rms_ffn_weight; // [n_layers * dim]
    float *grad_rms_final_weight; // [dim]
    
    // Gradients for attention weights (multi-query attention like karp model)
    float *grad_wq; // [n_layers * dim * dim] - query weights
    float *grad_wk; // [n_layers * dim * kv_dim] - key weights
    float *grad_wv; // [n_layers * dim * kv_dim] - value weights
    float *grad_wo; // [n_layers * dim * dim] - output weights
    
    // Gradients for feed-forward network weights (SwiGLU like karp model)
    float *grad_w1; // [n_layers * hidden_dim * dim] - gate projection
    float *grad_w2; // [n_layers * dim * hidden_dim] - output projection
    float *grad_w3; // [n_layers * hidden_dim * dim] - up projection
    
    // Gradients for output classifier weights
    float *grad_wcls; // [dim * vocab_size]
    
    int dim;        // transformer dimension
    int hidden_dim; // for ffn layers
    int n_layers;   // number of layers
    int n_heads;    // number of query heads
    int n_kv_heads; // number of key/value heads
    int vocab_size; // vocabulary size
    int seq_len;    // max sequence length
    int kv_dim;     // kv dimension per head
} Gradient;

// FUNCTION DECLARATIONS

void init_gradients(Gradient* grad, SimpleModel* model);
void zero_gradients(Gradient* grad);
void free_gradients(Gradient* grad);
void compute_output_gradients(float* output_logits, int* target_tokens, float* grad_logits, int vocab_size, int batch_size);
void update_weights_sgd(SimpleModel* model, Gradient* grad, float learning_rate);
float cross_entropy_loss(float* predicted_logits, int* actual_tokens, int vocab_size, int batch_size);

// FUNCTION DECLARATIONS 
int update_parameters_in_curriculum(const char* vocab_file, float learning_rate);
int load_and_update_curriculum(const char* vocab_file, float learning_rate);

// MAIN FUNCTION
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <command> [args...]\n", argv[0]);
        printf("Commands:\n");
        printf("  compute_gradients <vocab_size> <batch_size> - Compute output gradients\n");
        printf("  update <vocab_file> <learning_rate> - Update curriculum weights using gradients\n");
        printf("  cross_entropy <vocab_size> <batch_size> - Compute cross-entropy loss\n");
        printf("  save_updated_vocab <updated_vocab_file> - Save updated parameters to file\n");
        return 1;
    }
    
    char* command = argv[1];
    
    if (strcmp(command, "compute_gradients") == 0) {
        if (argc < 4) {
            printf("Error: Missing vocab_size and batch_size\n");
            return 1;
        }
        
        int vocab_size = atoi(argv[2]);
        int batch_size = atoi(argv[3]);
        
        // Create sample data
        float* output_logits = malloc(vocab_size * batch_size * sizeof(float));
        int* target_tokens = malloc(batch_size * sizeof(int));
        float* grad_logits = malloc(vocab_size * batch_size * sizeof(float));
        
        // Initialize with sample values
        for (int i = 0; i < vocab_size * batch_size; i++) {
            output_logits[i] = ((float)(i % 100) / 100.0f) - 0.5f;  // Values between -0.5 and 0.49
        }
        for (int i = 0; i < batch_size; i++) {
            target_tokens[i] = i % vocab_size;  // Target tokens in range [0, vocab_size)
        }
        
        printf("Computing gradients for vocab_size=%d, batch_size=%d\n", vocab_size, batch_size);
        
        // Compute gradients
        compute_output_gradients(output_logits, target_tokens, grad_logits, vocab_size, batch_size);
        
        printf("Computed gradients (first 5): ");
        for (int i = 0; i < 5 && i < vocab_size; i++) {
            printf("%.3f ", grad_logits[i]);
        }
        printf("...\n");
        
        free(output_logits);
        free(target_tokens);
        free(grad_logits);
        
    } else if (strcmp(command, "update") == 0) {
        if (argc < 4) {
            printf("Error: Missing vocab_file and learning_rate\n");
            printf("Usage: %s update <vocab_file> <learning_rate>\n", argv[0]);
            return 1;
        }
        
        char* vocab_file = argv[2];
        float learning_rate = atof(argv[3]);
        
        printf("Updating curriculum file: %s with learning_rate: %.6f\n", vocab_file, learning_rate);
        
        // Update parameters in curriculum based on internal gradient computation
        int result = load_and_update_curriculum(vocab_file, learning_rate);
        if (result != 0) {
            printf("Error updating curriculum parameters\n");
            return result;
        }
        
        printf("Parameters successfully updated in curriculum file\n");
        
    } else if (strcmp(command, "cross_entropy") == 0) {
        if (argc < 4) {
            printf("Error: Missing vocab_size and batch_size\n");
            return 1;
        }
        
        int vocab_size = atoi(argv[2]);
        int batch_size = atoi(argv[3]);
        
        // Create sample data
        float* predicted_logits = malloc(vocab_size * batch_size * sizeof(float));
        int* actual_tokens = malloc(batch_size * sizeof(int));
        
        // Initialize with sample values
        for (int i = 0; i < vocab_size * batch_size; i++) {
            predicted_logits[i] = ((float)(i % 100) / 100.0f) - 0.5f;  // Values between -0.5 and 0.49
        }
        for (int i = 0; i < batch_size; i++) {
            actual_tokens[i] = i % vocab_size;  // Actual tokens in range [0, vocab_size)
        }
        
        printf("Computing cross-entropy loss for vocab_size=%d, batch_size=%d\n", vocab_size, batch_size);
        
        // Compute cross-entropy loss
        float loss = cross_entropy_loss(predicted_logits, actual_tokens, vocab_size, batch_size);
        
        printf("Cross-entropy loss: %.6f\n", loss);
        
        free(predicted_logits);
        free(actual_tokens);
        
    } else if (strcmp(command, "save_updated_vocab") == 0) {
        printf("Save updated vocab function not implemented in this module\n");
        printf("This function should be called from the training process\n");
        return 1;
    } else {
        printf("Error: Unknown command '%s'\n", command);
        return 1;
    }
    
    return 0;
}

void init_gradients(Gradient* grad, SimpleModel* model) {
    grad->dim = model->dim;
    grad->hidden_dim = model->hidden_dim;
    grad->n_layers = model->n_layers;
    grad->n_heads = model->n_heads;
    grad->n_kv_heads = model->n_kv_heads;
    grad->vocab_size = model->vocab_size;
    grad->seq_len = model->seq_len;
    grad->kv_dim = model->kv_dim;
    
    // Allocate memory for gradients
    grad->grad_token_embedding_table = calloc(model->vocab_size * model->dim, sizeof(float));
    grad->grad_rms_att_weight = calloc(model->n_layers * model->dim, sizeof(float));
    grad->grad_rms_ffn_weight = calloc(model->n_layers * model->dim, sizeof(float));
    grad->grad_rms_final_weight = calloc(model->dim, sizeof(float));
    
    // Attention weight gradients
    grad->grad_wq = calloc(model->n_layers * model->dim * model->dim, sizeof(float));
    grad->grad_wk = calloc(model->n_layers * model->dim * model->kv_dim, sizeof(float));
    grad->grad_wv = calloc(model->n_layers * model->dim * model->kv_dim, sizeof(float));
    grad->grad_wo = calloc(model->n_layers * model->dim * model->dim, sizeof(float));
    
    // FFN weight gradients
    grad->grad_w1 = calloc(model->n_layers * model->hidden_dim * model->dim, sizeof(float));
    grad->grad_w2 = calloc(model->n_layers * model->dim * model->hidden_dim, sizeof(float));
    grad->grad_w3 = calloc(model->n_layers * model->hidden_dim * model->dim, sizeof(float));
    
    // Output classifier gradients
    grad->grad_wcls = calloc(model->dim * model->vocab_size, sizeof(float));
    
    // Check allocation success
    if (!grad->grad_token_embedding_table || !grad->grad_rms_att_weight || 
        !grad->grad_rms_ffn_weight || !grad->grad_rms_final_weight ||
        !grad->grad_wq || !grad->grad_wk || !grad->grad_wv || !grad->grad_wo ||
        !grad->grad_w1 || !grad->grad_w2 || !grad->grad_w3 || !grad->grad_wcls) {
        fprintf(stderr, "Gradient allocation failed!\n");
        exit(EXIT_FAILURE);
    }
}

void zero_gradients(Gradient* grad) {
    if (grad->grad_token_embedding_table) 
        memset(grad->grad_token_embedding_table, 0, grad->vocab_size * grad->dim * sizeof(float));
    if (grad->grad_rms_att_weight) 
        memset(grad->grad_rms_att_weight, 0, grad->n_layers * grad->dim * sizeof(float));
    if (grad->grad_rms_ffn_weight) 
        memset(grad->grad_rms_ffn_weight, 0, grad->n_layers * grad->dim * sizeof(float));
    if (grad->grad_rms_final_weight) 
        memset(grad->grad_rms_final_weight, 0, grad->dim * sizeof(float));
    
    if (grad->grad_wq) 
        memset(grad->grad_wq, 0, grad->n_layers * grad->dim * grad->dim * sizeof(float));
    if (grad->grad_wk) 
        memset(grad->grad_wk, 0, grad->n_layers * grad->dim * grad->kv_dim * sizeof(float));
    if (grad->grad_wv) 
        memset(grad->grad_wv, 0, grad->n_layers * grad->dim * grad->kv_dim * sizeof(float));
    if (grad->grad_wo) 
        memset(grad->grad_wo, 0, grad->n_layers * grad->dim * grad->dim * sizeof(float));
    
    if (grad->grad_w1) 
        memset(grad->grad_w1, 0, grad->n_layers * grad->hidden_dim * grad->dim * sizeof(float));
    if (grad->grad_w2) 
        memset(grad->grad_w2, 0, grad->n_layers * grad->dim * grad->hidden_dim * sizeof(float));
    if (grad->grad_w3) 
        memset(grad->grad_w3, 0, grad->n_layers * grad->hidden_dim * grad->dim * sizeof(float));
    
    if (grad->grad_wcls) 
        memset(grad->grad_wcls, 0, grad->dim * grad->vocab_size * sizeof(float));
}

void free_gradients(Gradient* grad) {
    SAFE_FREE(grad->grad_token_embedding_table);
    SAFE_FREE(grad->grad_rms_att_weight);
    SAFE_FREE(grad->grad_rms_ffn_weight);
    SAFE_FREE(grad->grad_rms_final_weight);
    SAFE_FREE(grad->grad_wq);
    SAFE_FREE(grad->grad_wk);
    SAFE_FREE(grad->grad_wv);
    SAFE_FREE(grad->grad_wo);
    SAFE_FREE(grad->grad_w1);
    SAFE_FREE(grad->grad_w2);
    SAFE_FREE(grad->grad_w3);
    SAFE_FREE(grad->grad_wcls);
}

// Compute gradients for output layer (simplified - just for the final logits layer)
void compute_output_gradients(float* output_logits, int* target_tokens, float* grad_logits, int vocab_size, int batch_size) {
    // Apply softmax to get probabilities
    float* probs = malloc(vocab_size * sizeof(float));
    
    for (int b = 0; b < batch_size; b++) {
        // Find max logit for numerical stability
        float max_val = output_logits[b * vocab_size];
        for (int i = 1; i < vocab_size; i++) {
            float val = output_logits[b * vocab_size + i];
            if (val > max_val) {
                max_val = val;
            }
        }
        
        // Compute exp and sum
        float sum = 0.0f;
        for (int i = 0; i < vocab_size; i++) {
            probs[i] = expf(output_logits[b * vocab_size + i] - max_val);
            sum += probs[i];
        }
        
        // Normalize to get probabilities
        for (int i = 0; i < vocab_size; i++) {
            probs[i] /= sum;
        }
        
        // Compute gradient: grad = probs - targets (for cross-entropy loss)
        for (int i = 0; i < vocab_size; i++) {
            grad_logits[b * vocab_size + i] = probs[i];
            if (i == target_tokens[b]) {
                grad_logits[b * vocab_size + i] -= 1.0f;  // Subtract 1 for correct token
            }
        }
    }
    
    free(probs);
}

// Simple SGD weight update function
void update_weights_sgd(SimpleModel* model, Gradient* grad, float learning_rate) {
    // Update token embedding table
    for (int i = 0; i < model->vocab_size * model->dim; i++) {
        model->token_embedding_table[i] -= learning_rate * grad->grad_token_embedding_table[i];
    }
    
    // Update RMSNorm weights
    for (int i = 0; i < model->n_layers * model->dim; i++) {
        model->rms_att_weight[i] -= learning_rate * grad->grad_rms_att_weight[i];
        model->rms_ffn_weight[i] -= learning_rate * grad->grad_rms_ffn_weight[i];
    }
    for (int i = 0; i < model->dim; i++) {
        model->rms_final_weight[i] -= learning_rate * grad->grad_rms_final_weight[i];
    }
    
    // Update attention weights
    int att_params_q = model->n_layers * model->dim * model->dim;
    for (int i = 0; i < att_params_q; i++) {
        model->wq[i] -= learning_rate * grad->grad_wq[i];
        model->wo[i] -= learning_rate * grad->grad_wo[i];
    }
    
    int att_params_kv = model->n_layers * model->dim * model->kv_dim;
    for (int i = 0; i < att_params_kv; i++) {
        model->wk[i] -= learning_rate * grad->grad_wk[i];
        model->wv[i] -= learning_rate * grad->grad_wv[i];
    }
    
    // Update FFN weights
    int ffn_params1 = model->n_layers * model->hidden_dim * model->dim;
    int ffn_params2 = model->n_layers * model->dim * model->hidden_dim;
    for (int i = 0; i < ffn_params1; i++) {
        model->w1[i] -= learning_rate * grad->grad_w1[i];
        model->w3[i] -= learning_rate * grad->grad_w3[i];
    }
    for (int i = 0; i < ffn_params2; i++) {
        model->w2[i] -= learning_rate * grad->grad_w2[i];
    }
    
    // Update output classifier weights
    for (int i = 0; i < model->dim * model->vocab_size; i++) {
        model->wcls[i] -= learning_rate * grad->grad_wcls[i];
    }
}

// Cross-entropy loss function
float cross_entropy_loss(float* predicted_logits, int* actual_tokens, int vocab_size, int batch_size) {
    float total_loss = 0.0f;
    
    for (int i = 0; i < batch_size; i++) {
        int actual_token = actual_tokens[i];
        
        // Find max logit for numerical stability
        float max_logit = predicted_logits[i * vocab_size];
        for (int j = 1; j < vocab_size; j++) {
            float logit = predicted_logits[i * vocab_size + j];
            if (logit > max_logit) {
                max_logit = logit;
            }
        }
        
        // Compute log(sum(exp(logits))) for numerical stability
        float log_sum_exp = 0.0f;
        for (int j = 0; j < vocab_size; j++) {
            log_sum_exp += expf(predicted_logits[i * vocab_size + j] - max_logit);
        }
        log_sum_exp = logf(log_sum_exp) + max_logit;
        
        // Add negative log probability of correct token
        float correct_logit = predicted_logits[i * vocab_size + actual_token];
        total_loss -= (correct_logit - log_sum_exp);
    }
    
    return total_loss / batch_size;
}

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

// Function to load and update curriculum parameters (internal gradient computation)
int load_and_update_curriculum(const char* vocab_file, float learning_rate) {
    printf("Loading and updating curriculum parameters from: %s with learning_rate: %.6f\n", vocab_file, learning_rate);
    
    // First, load the current vocabulary from the curriculum file
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
    
    // Close the original file to prepare for update
    fclose(file);
    
    // Create a temporary file to write the updated vocabulary
    char temp_file[2048];
    snprintf(temp_file, sizeof(temp_file), "%s.temp", vocab_file);
    
    FILE *read_file = fopen(vocab_file, "r");
    FILE *write_file = fopen(temp_file, "w");
    
    if (!read_file || !write_file) {
        printf("Error: could not open files for updating vocabulary\n");
        if (read_file) fclose(read_file);
        if (write_file) fclose(write_file);
        return -1;
    }
    
    // Copy header line
    if (fgets(line, sizeof(line), read_file) != NULL) {
        fputs(line, write_file);
    }
    
    // Process each token entry and update parameters based on gradients
    int line_num = 0;
    char word[1000];
    int index;
    float embedding, pe_value, attention_bias_val, ffn_bias_val, weight1_val, weight2_val, bias1_val, bias2_val, bias3_val, bias4_val, q_val, k_val, v_val;
    char note_str[1000];
    
    while (fscanf(read_file, "%d %s %f %f %f %f %f %f %f %f %f %f %f %f %f %s", 
                  &index, word, &embedding, &pe_value, 
                  &attention_bias_val, &ffn_bias_val,
                  &weight1_val, &weight2_val, 
                  &bias1_val, &bias2_val, &bias3_val, &bias4_val,
                  &q_val, &k_val, &v_val, note_str) == 16) {
        line_num++;
        
        // SIMULATE GRADIENT CALCULATION AND PARAMETER UPDATES
        // In a real implementation, these would be computed based on training loss
        // For simulation, we'll update parameters based on a simple heuristic
        float simulated_gradient = ((float)line_num) * 0.0001f;  // Create a small gradient
        
        // Update parameters with gradients (SGD style)
        float updated_embedding = embedding - learning_rate * simulated_gradient;
        float updated_pe_value = pe_value - learning_rate * (simulated_gradient * 0.5f);
        float updated_attn_bias = attention_bias_val - learning_rate * (simulated_gradient * 0.3f);
        float updated_ffn_bias = ffn_bias_val - learning_rate * (simulated_gradient * 0.2f);
        float updated_weight1 = weight1_val - learning_rate * (simulated_gradient * 0.4f);
        float updated_weight2 = weight2_val - learning_rate * (simulated_gradient * 0.4f);
        float updated_bias1 = bias1_val - learning_rate * (simulated_gradient * 0.3f);
        float updated_bias2 = bias2_val - learning_rate * (simulated_gradient * 0.3f);
        float updated_bias3 = bias3_val - learning_rate * (simulated_gradient * 0.3f);
        float updated_bias4 = bias4_val - learning_rate * (simulated_gradient * 0.3f);
        float updated_q_val = q_val - learning_rate * (simulated_gradient * 0.6f);
        float updated_k_val = k_val - learning_rate * (simulated_gradient * 0.6f);
        float updated_v_val = v_val - learning_rate * (simulated_gradient * 0.6f);
        
        // Write updated parameters to temp file in the same format
        fprintf(write_file, "%d %s %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %s\n",
                index, word, updated_embedding, updated_pe_value, updated_attn_bias, updated_ffn_bias,
                updated_weight1, updated_weight2, updated_bias1, updated_bias2, updated_bias3, updated_bias4,
                updated_q_val, updated_k_val, updated_v_val, note_str);
    }
    
    fclose(read_file);
    fclose(write_file);
    
    // Replace the original file with the updated one
    if (rename(temp_file, vocab_file) != 0) {
        printf("Error: could not replace original vocabulary file with updated version\n");
        return -1;
    }
    
    printf("Successfully updated %d tokens in curriculum file with new parameters\n", line_num);
    return 0; // Success
}