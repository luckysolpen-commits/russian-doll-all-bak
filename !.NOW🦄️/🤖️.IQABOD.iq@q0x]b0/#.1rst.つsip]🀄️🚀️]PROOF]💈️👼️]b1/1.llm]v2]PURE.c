#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_PROMPT 1000
#define EMBED_DIM 16
#define NUM_HEADS 2
#define FF_DIM 32
#define MAX_ITERATIONS 50
#define TEMPERATURE 1.0f
#define NUM_SUBJECTS 10
#define EPSILON 0.1f

int next_id; 

void load_matrix(const char* filename, float* matrix, int rows, int cols) {
    FILE* f = fopen(filename, "r");
    if (!f) { printf("Error opening %s\n", filename); exit(1); }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) fscanf(f, "%f", &matrix[i * cols + j]);
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
        logits[i] = expf((logits[i] - max_logit) / TEMPERATURE);
        sum += logits[i];
    }
    for (int i = 0; i < vocab_size; i++) logits[i] /= sum;
    free(attn_out); free(ff_out);
}

int sample_logits(float* logits, int vocab_size) {
    float r = (float)rand() / RAND_MAX;
    float cumulative = 0;
    for (int i = 0; i < vocab_size; i++) {
        cumulative += logits[i];
        if (r <= cumulative) return i;
    }
    return vocab_size - 1;
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

int main() {
    srand(time(NULL));

    char* subjects[NUM_SUBJECTS] = {"Mathematics", "Physics", "Biology", "History", "Literature",
                                    "Programming", "Chemistry", "Geography", "Economics", "Astronomy"};
    float weights[NUM_SUBJECTS];
    FILE* weight_f = fopen("weights.txt", "r");
    if (!weight_f) { printf("Error opening weights.txt\n"); return 1; }
    for (int i = 0; i < NUM_SUBJECTS; i++) fscanf(weight_f, "%f ", &weights[i]);
    fclose(weight_f);

    int chosen_idx;
    if ((float)rand() / RAND_MAX < EPSILON) {
        chosen_idx = rand() % NUM_SUBJECTS; // Explore
    } else {
        chosen_idx = 0; // Exploit
        for (int i = 1; i < NUM_SUBJECTS; i++) if (weights[i] > weights[chosen_idx]) chosen_idx = i;
    }

    char folder[50];
    snprintf(folder, 50, "curricula/%s", subjects[chosen_idx]);
    char filepath[100];

    int vocab_size, embed_dim;
    snprintf(filepath, 100, "%s/model.txt", folder);
    FILE* model_f = fopen(filepath, "r");
    if (!model_f) { printf("Error opening %s\n", filepath); return 1; }
    fscanf(model_f, "vocab_size: %d\nembed_dim: %d\n", &vocab_size, &embed_dim);
    fclose(model_f);

    float* word_emb = malloc(vocab_size * EMBED_DIM * sizeof(float));
    float* attn_weights = malloc(EMBED_DIM * EMBED_DIM * sizeof(float));
    float* ff_weights1 = malloc(EMBED_DIM * FF_DIM * sizeof(float));
    float* ff_weights2 = malloc(FF_DIM * EMBED_DIM * sizeof(float));
    float* biases = malloc(EMBED_DIM * sizeof(float));
    if (!word_emb || !attn_weights || !ff_weights1 || !ff_weights2 || !biases) { printf("Malloc failed\n"); exit(1); }
    snprintf(filepath, 100, "%s/hash_embeddings.txt", folder);
    load_matrix(filepath, word_emb, vocab_size, EMBED_DIM);
    snprintf(filepath, 100, "%s/attention_weights.txt", folder);
    load_matrix(filepath, attn_weights, EMBED_DIM, EMBED_DIM);
    snprintf(filepath, 100, "%s/ff_weights1.txt", folder);
    load_matrix(filepath, ff_weights1, EMBED_DIM, FF_DIM);
    snprintf(filepath, 100, "%s/ff_weights2.txt", folder);
    load_matrix(filepath, ff_weights2, FF_DIM, EMBED_DIM);
    snprintf(filepath, 100, "%s/biases.txt", folder);
    load_matrix(filepath, biases, 1, EMBED_DIM);

    char vocab[MAX_PROMPT][50];
    snprintf(filepath, 100, "%s/index.txt", folder);
    FILE* index_f = fopen(filepath, "r");
    if (!index_f) { printf("Error opening %s\n", filepath); return 1; }
    for (int i = 0; i < vocab_size; i++) fscanf(index_f, "%*d %s\n", vocab[i]);
    fclose(index_f);

    // Load curriculum
    snprintf(filepath, 100, "%s/curriculum.txt", folder);
    FILE* curriculum_f = fopen(filepath, "r");
    if (!curriculum_f) { printf("Error opening %s\n", filepath); return 1; }
    char curriculum[MAX_PROMPT];
    fgets(curriculum, MAX_PROMPT, curriculum_f);
    fclose(curriculum_f);

    // Load user prompt
    FILE* prompt_f = fopen("prompt.txt", "r");
    if (!prompt_f) { printf("Error opening prompt.txt\n"); return 1; }
    char user_prompt[MAX_PROMPT];
    fgets(user_prompt, MAX_PROMPT, prompt_f);
    fclose(prompt_f);

    // Combine curriculum and user prompt
    char combined[MAX_PROMPT * 2];
    snprintf(combined, MAX_PROMPT * 2, "%s %s", curriculum, user_prompt);

    char words[MAX_PROMPT][50];
    int word_count;
    tokenize(combined, words, &word_count);

    float* input = malloc(word_count * EMBED_DIM * sizeof(float));
    if (!input) { printf("Malloc failed\n"); exit(1); }
    for (int i = 0; i < word_count; i++) {
        int id = word_to_id(words[i], vocab, vocab_size);
        if (id != -1) memcpy(&input[i * EMBED_DIM], &word_emb[id * EMBED_DIM], EMBED_DIM * sizeof(float));
        else memset(&input[i * EMBED_DIM], 0, EMBED_DIM * sizeof(float)); // Zero out unknown words
    }

    FILE* output_f = fopen("output.txt", "w");
    if (!output_f) { printf("Error opening output.txt\n"); return 1; }
    for (int i = 0; i < word_count; i++) fprintf(output_f, "%s ", words[i]);
    float* logits = malloc(vocab_size * sizeof(float));
    if (!logits) { printf("Malloc failed\n"); exit(1); }
    int iterations = 0;
    do {
        predict(word_emb, attn_weights, ff_weights1, ff_weights2, vocab_size, input, word_count, logits);
         next_id = sample_logits(logits, vocab_size);
        fprintf(output_f, "%s ", vocab[next_id]);
        word_count++;
        input = realloc(input, word_count * EMBED_DIM * sizeof(float));
        if (!input) { printf("Realloc failed\n"); exit(1); }
        memcpy(&input[(word_count - 1) * EMBED_DIM], &word_emb[next_id * EMBED_DIM], EMBED_DIM * sizeof(float));
        iterations++;
    } while (strcmp(vocab[next_id], "end_token") != 0 && iterations < MAX_ITERATIONS);
    if (iterations >= MAX_ITERATIONS) fprintf(output_f, "[TRUNCATED]");
    fclose(output_f);

    free(word_emb); free(attn_weights); free(ff_weights1); free(ff_weights2); free(biases);
    free(input); free(logits);
    return 0;
}
