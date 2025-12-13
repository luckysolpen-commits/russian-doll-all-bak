#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_PROMPT 1000
#define EMBED_DIM 16
#define NUM_HEADS 2
#define FF_DIM 32
#define NUM_SUBJECTS 10
#define TOP_K_EXPERTS 2
#define TEMPERATURE 2.0f

void load_matrix(const char* filename, float* matrix, int rows, int cols) {
    FILE* f = fopen(filename, "r");
    if (!f) { printf("Error opening %s\n", filename); exit(1); }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (fscanf(f, "%f", &matrix[i * cols + j]) != 1) {
                printf("Error reading %s at row %d, col %d\n", filename, i, j);
                fclose(f);
                exit(1);
            }
        }
    }
    fclose(f);
}

void masked_attention(float* input, int seq_len, float* weights, float* output) {
    float* q = malloc(seq_len * EMBED_DIM * sizeof(float));
    float* k = malloc(seq_len * EMBED_DIM * sizeof(float));
    float* v = malloc(seq_len * EMBED_DIM * sizeof(float));
    if (!q || !k || !v) { printf("Malloc failed\n"); exit(1); }
    for (int i = 0; i < seq_len * EMBED_DIM; i++) q[i] = k[i] = v[i] = input[i];

    float* scores = malloc(seq_len * seq_len * sizeof(float));
    if (!scores) { printf("Malloc failed\n"); exit(1); }
    for (int i = 0; i < seq_len; i++) {
        for (int j = 0; j <= i; j++) {
            float score = 0;
            for (int d = 0; d < EMBED_DIM; d++) score += q[i * EMBED_DIM + d] * k[j * EMBED_DIM + d];
            scores[i * seq_len + j] = score / sqrtf(EMBED_DIM);
        }
        for (int j = i + 1; j < seq_len; j++) scores[i * seq_len + j] = -INFINITY;
    }

    for (int i = 0; i < seq_len; i++) {
        float max_score = -INFINITY;
        for (int j = 0; j < seq_len; j++) if (scores[i * seq_len + j] > max_score) max_score = scores[i * seq_len + j];
        float sum = 0;
        for (int j = 0; j < seq_len; j++) {
            if (scores[i * seq_len + j] != -INFINITY) {
                scores[i * seq_len + j] = expf(scores[i * seq_len + j] - max_score);
                sum += scores[i * seq_len + j];
            }
        }
        for (int j = 0; j < seq_len; j++) scores[i * seq_len + j] = (scores[i * seq_len + j] == -INFINITY) ? 0 : scores[i * seq_len + j] / sum;
    }

    for (int i = 0; i < seq_len; i++) {
        for (int d = 0; d < EMBED_DIM; d++) {
            output[i * EMBED_DIM + d] = 0;
            for (int j = 0; j < seq_len; j++) output[i * EMBED_DIM + d] += scores[i * seq_len + j] * v[j * EMBED_DIM + d];
        }
    }
    free(q); free(k); free(v); free(scores);
}

void feed_forward(float* input, int seq_len, float* w1, float* w2, float* output) {
    float* hidden = malloc(seq_len * FF_DIM * sizeof(float));
    if (!hidden) { printf("Malloc failed\n"); exit(1); }
    for (int i = 0; i < seq_len; i++) {
        for (int j = 0; j < FF_DIM; j++) {
            hidden[i * FF_DIM + j] = 0;
            for (int d = 0; d < EMBED_DIM; d++) hidden[i * FF_DIM + j] += input[i * EMBED_DIM + d] * w1[d * FF_DIM + j];
            hidden[i * FF_DIM + j] = hidden[i * FF_DIM + j] > 0 ? hidden[i * FF_DIM + j] : 0;
        }
    }
    for (int i = 0; i < seq_len; i++) {
        for (int d = 0; d < EMBED_DIM; d++) {
            output[i * EMBED_DIM + d] = 0;
            for (int j = 0; j < FF_DIM; j++) output[i * EMBED_DIM + d] += hidden[i * FF_DIM + j] * w2[j * EMBED_DIM + d];
        }
    }
    free(hidden);
}

void predict(float* word_emb, float* attn_weights, float* ff_weights1, float* ff_weights2,
             int vocab_size, float* input, int seq_len, float* logits) {
    float* attn_out = malloc(seq_len * EMBED_DIM * sizeof(float));
    float* ff_out = malloc(seq_len * EMBED_DIM * sizeof(float));
    if (!attn_out || !ff_out) { printf("Malloc failed\n"); exit(1); }
    masked_attention(input, seq_len, attn_weights, attn_out);
    feed_forward(attn_out, seq_len, ff_weights1, ff_weights2, ff_out);
    float max_logit = -INFINITY;
    for (int i = 0; i < vocab_size; i++) {
        logits[i] = 0;
        for (int d = 0; d < EMBED_DIM; d++) logits[i] += ff_out[(seq_len - 1) * EMBED_DIM + d] * word_emb[i * EMBED_DIM + d];
        if (logits[i] > max_logit) max_logit = logits[i];
    }
    float sum = 0;
    for (int i = 0; i < vocab_size; i++) {
        logits[i] = expf(logits[i] - max_logit);
        sum += logits[i];
    }
    if (sum == 0) sum = 1e-10;
    for (int i = 0; i < vocab_size; i++) logits[i] /= sum;
    free(attn_out); free(ff_out);
}

int word_to_id(char* word, char vocab[MAX_PROMPT][50], int vocab_size) {
    for (int i = 0; i < vocab_size; i++) {
        if (strcmp(word, vocab[i]) == 0) return i;
    }
    return -1;
}

void tokenize(char* text, char words[MAX_PROMPT][50], int* word_count) {
    *word_count = 0;
    char* token = strtok(text, " \n");
    while (token && *word_count < MAX_PROMPT) {
        strncpy(words[(*word_count)++], token, 49);
        words[*word_count - 1][49] = '\0';
        token = strtok(NULL, " \n");
    }
}

void compute_gating_weights(float* input, int seq_len, float* gating_weights, float* expert_weights) {
    float input_sum[EMBED_DIM] = {0};
    for (int i = 0; i < seq_len; i++) {
        for (int d = 0; d < EMBED_DIM; d++) input_sum[d] += input[i * EMBED_DIM + d];
    }
    for (int d = 0; d < EMBED_DIM; d++) input_sum[d] /= seq_len;

    float scores[NUM_SUBJECTS];
    float max_score = -INFINITY;
    for (int s = 0; s < NUM_SUBJECTS; s++) {
        scores[s] = 0;
        for (int d = 0; d < EMBED_DIM; d++) scores[s] += input_sum[d] * gating_weights[s * EMBED_DIM + d];
        if (scores[s] > max_score) max_score = scores[s];
    }

    float sum = 0;
    for (int s = 0; s < NUM_SUBJECTS; s++) {
        scores[s] = expf(scores[s] - max_score);
        sum += scores[s];
    }
    if (sum == 0) sum = 1e-10;
    for (int s = 0; s < NUM_SUBJECTS; s++) expert_weights[s] = scores[s] / sum;
}

void select_top_k_experts(float* expert_weights, int* top_indices) {
    float sorted_weights[NUM_SUBJECTS];
    int indices[NUM_SUBJECTS];
    for (int s = 0; s < NUM_SUBJECTS; s++) {
        sorted_weights[s] = expert_weights[s];
        indices[s] = s;
    }
    for (int i = 0; i < TOP_K_EXPERTS; i++) {
        for (int j = 0; j < NUM_SUBJECTS - i - 1; j++) {
            if (sorted_weights[j] < sorted_weights[j + 1]) {
                float tmp = sorted_weights[j];
                sorted_weights[j] = sorted_weights[j + 1];
                sorted_weights[j + 1] = tmp;
                int tmp_idx = indices[j];
                indices[j] = indices[j + 1];
                indices[j + 1] = tmp_idx;
            }
        }
    }
    for (int k = 0; k < TOP_K_EXPERTS; k++) top_indices[k] = indices[k];
}

int main() {
    char* subjects[NUM_SUBJECTS] = {"Mathematics", "Physics", "Biology", "History", "Literature",
                                    "Programming", "Chemistry", "Geography", "Economics", "Astronomy"};

    float* gating_weights = malloc(NUM_SUBJECTS * EMBED_DIM * sizeof(float));
    if (!gating_weights) { printf("Malloc failed\n"); exit(1); }
    load_matrix("gating_weights.txt", gating_weights, NUM_SUBJECTS, EMBED_DIM);

    int vocab_sizes[NUM_SUBJECTS];
    float* word_embs[NUM_SUBJECTS];
    float* attn_weights[NUM_SUBJECTS];
    float* ff_weights1[NUM_SUBJECTS];
    float* ff_weights2[NUM_SUBJECTS];
    char vocab[NUM_SUBJECTS][MAX_PROMPT][50];
    for (int s = 0; s < NUM_SUBJECTS; s++) {
        char folder[50];
        snprintf(folder, 50, "curricula/%s", subjects[s]);
        char filepath[100];

        snprintf(filepath, 100, "%s/model.txt", folder);
        FILE* model_f = fopen(filepath, "r");
        if (!model_f) { printf("Error opening %s\n", filepath); return 1; }
        int embed_dim;
        fscanf(model_f, "vocab_size: %d\nembed_dim: %d\n", &vocab_sizes[s], &embed_dim);
        fclose(model_f);

        word_embs[s] = malloc(vocab_sizes[s] * EMBED_DIM * sizeof(float));
        attn_weights[s] = malloc(EMBED_DIM * EMBED_DIM * sizeof(float));
        ff_weights1[s] = malloc(EMBED_DIM * FF_DIM * sizeof(float));
        ff_weights2[s] = malloc(FF_DIM * EMBED_DIM * sizeof(float));
        if (!word_embs[s] || !attn_weights[s] || !ff_weights1[s] || !ff_weights2[s]) { printf("Malloc failed\n"); exit(1); }

        snprintf(filepath, 100, "%s/hash_embeddings.txt", folder);
        load_matrix(filepath, word_embs[s], vocab_sizes[s], EMBED_DIM);
        snprintf(filepath, 100, "%s/attention_weights.txt", folder);
        load_matrix(filepath, attn_weights[s], EMBED_DIM, EMBED_DIM);
        snprintf(filepath, 100, "%s/ff_weights1.txt", folder);
        load_matrix(filepath, ff_weights1[s], EMBED_DIM, FF_DIM);
        snprintf(filepath, 100, "%s/ff_weights2.txt", folder);
        load_matrix(filepath, ff_weights2[s], FF_DIM, EMBED_DIM);

        snprintf(filepath, 100, "%s/index.txt", folder);
        FILE* index_f = fopen(filepath, "r");
        if (!index_f) { printf("Error opening %s\n", filepath); return 1; }
        for (int i = 0; i < vocab_sizes[s]; i++) fscanf(index_f, "%*d %s\n", vocab[s][i]);
        fclose(index_f);
    }

    FILE* test_f = fopen("test.txt", "r");
    if (!test_f) { printf("Error opening test.txt\n"); return 1; }
    fseek(test_f, 0, SEEK_END);
    long size = ftell(test_f);
    rewind(test_f);
    char* test_text = malloc(size + 1);
    fread(test_text, 1, size, test_f);
    test_text[size] = '\0';
    fclose(test_f);

    char words[MAX_PROMPT][50];
    int word_count;
    tokenize(test_text, words, &word_count);

    float* input = malloc(word_count * EMBED_DIM * sizeof(float));
    if (!input) { printf("Malloc failed\n"); exit(1); }
    for (int i = 0; i < word_count; i++) {
        int id = word_to_id(words[i], vocab[0], vocab_sizes[0]);
        if (id != -1) memcpy(&input[i * EMBED_DIM], &word_embs[0][id * EMBED_DIM], EMBED_DIM * sizeof(float));
        else memset(&input[i * EMBED_DIM], 0, EMBED_DIM * sizeof(float));
    }

    float expert_weights[NUM_SUBJECTS];
    int top_indices[TOP_K_EXPERTS];
    float* logits = malloc(vocab_sizes[0] * sizeof(float));
    float* expert_logits[TOP_K_EXPERTS];
    for (int k = 0; k < TOP_K_EXPERTS; k++) expert_logits[k] = malloc(vocab_sizes[0] * sizeof(float));
    double total_log_prob = 0;
    int total_words = 0;

    for (int i = 1; i < word_count; i++) {
        compute_gating_weights(input, i, gating_weights, expert_weights);
        select_top_k_experts(expert_weights, top_indices);

        for (int j = 0; j < vocab_sizes[0]; j++) logits[j] = 0;
        for (int k = 0; k < TOP_K_EXPERTS; k++) {
            int s = top_indices[k];
            predict(word_embs[s], attn_weights[s], ff_weights1[s], ff_weights2[s], vocab_sizes[s], input, i, expert_logits[k]);
            for (int j = 0; j < vocab_sizes[0]; j++) {
                int id = word_to_id(vocab[0][j], vocab[s], vocab_sizes[s]);
                if (id != -1) logits[j] += expert_weights[s] * expert_logits[k][id];
            }
        }

        float max_logit = -INFINITY;
        for (int j = 0; j < vocab_sizes[0]; j++) if (logits[j] > max_logit) max_logit = logits[j];
        float sum = 0;
        for (int j = 0; j < vocab_sizes[0]; j++) {
            logits[j] = expf((logits[j] - max_logit) / TEMPERATURE);
            sum += logits[j];
        }
        if (sum == 0) sum = 1e-10;
        for (int j = 0; j < vocab_sizes[0]; j++) logits[j] /= sum;

        int target_id = word_to_id(words[i], vocab[0], vocab_sizes[0]);
        if (target_id != -1) {
            float prob = logits[target_id];
            if (prob < 1e-10) prob = 1e-10; // Avoid log(0)
            total_log_prob += log(prob);
            total_words++;
        }
    }

    double perplexity = exp(-total_log_prob / total_words);
    printf("Perplexity: %.4f\n", perplexity);

    for (int s = 0; s < NUM_SUBJECTS; s++) {
        free(word_embs[s]); free(attn_weights[s]); free(ff_weights1[s]); free(ff_weights2[s]);
    }
    for (int k = 0; k < TOP_K_EXPERTS; k++) free(expert_logits[k]);
    free(gating_weights); free(input); free(logits); free(test_text);
    return 0;
}
