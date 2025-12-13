#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define NUM_SUBJECTS 10
#define EMBED_DIM 16
#define MAX_WORDS 1000
#define MAX_PROMPT 1000  // Added missing definition
#define LEARNING_RATE 0.01f
#define EPOCHS 50

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

void save_matrix(const char* filename, float* matrix, int rows, int cols) {
    FILE* f = fopen(filename, "w");
    if (!f) { printf("Error opening %s\n", filename); exit(1); }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) fprintf(f, "%.4f ", matrix[i * cols + j]);
        fprintf(f, "\n");
    }
    fclose(f);
}

void tokenize(char* text, char words[MAX_WORDS][50], int* word_count) {
    *word_count = 0;
    char* token = strtok(text, " \n");
    while (token && *word_count < MAX_WORDS) {
        strncpy(words[(*word_count)++], token, 49);
        words[*word_count - 1][49] = '\0';
        token = strtok(NULL, " \n");
    }
}

int word_to_id(char* word, char vocab[MAX_PROMPT][50], int vocab_size) {  // Fixed declaration
    for (int i = 0; i < vocab_size; i++) {
        if (strcmp(word, vocab[i]) == 0) return i;
    }
    return -1;
}

int main() {
    srand(time(NULL));

    char* subjects[NUM_SUBJECTS] = {"Mathematics", "Physics", "Biology", "History", "Literature",
                                    "Programming", "Chemistry", "Geography", "Economics", "Astronomy"};

    // Load initial random gating weights
    float* gating_weights = malloc(NUM_SUBJECTS * EMBED_DIM * sizeof(float));
    load_matrix("gating_weights.txt", gating_weights, NUM_SUBJECTS, EMBED_DIM);

    // Load vocabulary and embeddings for reference expert (Mathematics)
    int vocab_size;
    char filepath[100];
    snprintf(filepath, 100, "curricula/Mathematics/model.txt");
    FILE* model_f = fopen(filepath, "r");
    if (!model_f) { printf("Error opening %s\n", filepath); return 1; }
    int embed_dim;
    fscanf(model_f, "vocab_size: %d\nembed_dim: %d\n", &vocab_size, &embed_dim);
    fclose(model_f);

    float* word_emb = malloc(vocab_size * EMBED_DIM * sizeof(float));
    snprintf(filepath, 100, "curricula/Mathematics/hash_embeddings.txt");
    load_matrix(filepath, word_emb, vocab_size, EMBED_DIM);

    char vocab[MAX_PROMPT][50];
    snprintf(filepath, 100, "curricula/Mathematics/index.txt");
    FILE* index_f = fopen(filepath, "r");
    if (!index_f) { printf("Error opening %s\n", filepath); return 1; }
    for (int i = 0; i < vocab_size; i++) fscanf(index_f, "%*d %s\n", vocab[i]);
    fclose(index_f);

    // Load curriculum data
    FILE* f = fopen("curriculums.txt", "r");
    if (!f) { printf("Error opening curriculums.txt\n"); return 1; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* buffer = malloc(size + 1);
    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);

    char corpora[NUM_SUBJECTS][MAX_WORDS * 50];
    char* ptr = buffer;
    for (int s = 0; s < NUM_SUBJECTS; s++) {
        char marker[50];
        snprintf(marker, 50, "[%s]", subjects[s]);
        ptr = strstr(ptr, marker);
        if (!ptr) { printf("Subject %s not found\n", subjects[s]); return 1; }
        ptr += strlen(marker);
        char* end = strstr(ptr, "[");
        if (!end) end = buffer + size;
        int len = end - ptr;
        if (len > MAX_WORDS * 50 - 1) len = MAX_WORDS * 50 - 1;
        strncpy(corpora[s], ptr, len);
        corpora[s][len] = '\0';
    }

    // Training loop
    for (int epoch = 0; epoch < EPOCHS; epoch++) {
        float total_loss = 0;
        int total_samples = 0;

        for (int s = 0; s < NUM_SUBJECTS; s++) {
            char words[MAX_WORDS][50];
            int word_count;
            tokenize(corpora[s], words, &word_count);

            float input[EMBED_DIM] = {0};
            for (int i = 0; i < word_count; i++) {
                int id = word_to_id(words[i], vocab, vocab_size);
                if (id != -1) {
                    for (int d = 0; d < EMBED_DIM; d++) input[d] += word_emb[id * EMBED_DIM + d];
                }
            }
            for (int d = 0; d < EMBED_DIM; d++) input[d] /= word_count;

            float scores[NUM_SUBJECTS];
            float max_score = -INFINITY;
            for (int j = 0; j < NUM_SUBJECTS; j++) {
                scores[j] = 0;
                for (int d = 0; d < EMBED_DIM; d++) scores[j] += input[d] * gating_weights[j * EMBED_DIM + d];
                if (scores[j] > max_score) max_score = scores[j];
            }
            float sum = 0;
            for (int j = 0; j < NUM_SUBJECTS; j++) {
                scores[j] = expf(scores[j] - max_score);
                sum += scores[j];
            }
            if (sum == 0) sum = 1e-10;
            float weights[NUM_SUBJECTS];
            for (int j = 0; j < NUM_SUBJECTS; j++) weights[j] = scores[j] / sum;

            // Compute loss and gradients
            float loss = -logf(weights[s] + 1e-10);
            total_loss += loss;
            total_samples++;

            float grad[NUM_SUBJECTS];
            for (int j = 0; j < NUM_SUBJECTS; j++) {
                grad[j] = weights[j] - (j == s ? 1.0f : 0.0f);
            }

            // Update gating weights
            for (int j = 0; j < NUM_SUBJECTS; j++) {
                for (int d = 0; d < EMBED_DIM; d++) {
                    float g = grad[j] * input[d];
                    if (g > 1.0f) g = 1.0f; // Gradient clipping
                    if (g < -1.0f) g = -1.0f;
                    gating_weights[j * EMBED_DIM + d] -= LEARNING_RATE * g;
                }
            }
        }

        printf("Epoch %d: Avg Loss %.4f\n", epoch + 1, total_loss / total_samples);
    }

    // Save trained weights
    save_matrix("gating_weights.txt", gating_weights, NUM_SUBJECTS, EMBED_DIM);

    free(gating_weights);
    free(word_emb);
    free(buffer);
    return 0;
}
