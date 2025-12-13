#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_WORDS 10000
#define VOCAB_SIZE 100
#define EMBED_DIM 16
#define NUM_SUBJECTS 10
#define DIFFUSION_STEPS 50
#define NOISE_SCALE 0.1f

void save_matrix(float* matrix, int rows, int cols, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) { printf("Error opening %s\n", filename); exit(1); }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) fprintf(f, "%.4f ", matrix[i * cols + j]);
        fprintf(f, "\n");
    }
    fclose(f);
}

void tokenize(char* text, char** words, int* word_count) {
    *word_count = 0;
    char* token = strtok(text, " \n");
    while (token && *word_count < MAX_WORDS) {
        words[(*word_count)++] = strdup(token);
        token = strtok(NULL, " \n");
    }
}

void build_vocab(char** words, int word_count, char** vocab, int* vocab_size) {
    *vocab_size = 0;
    for (int i = 0; i < word_count; i++) {
        int found = 0;
        for (int j = 0; j < *vocab_size; j++) {
            if (strcmp(words[i], vocab[j]) == 0) { found = 1; break; }
        }
        if (!found && *vocab_size < VOCAB_SIZE) vocab[(*vocab_size)++] = strdup(words[i]);
    }
}

float* hash_embedding(char** vocab, int vocab_size, int embed_dim) {
    float* emb = malloc(vocab_size * embed_dim * sizeof(float));
    if (!emb) { printf("Malloc failed\n"); exit(1); }
    for (int i = 0; i < vocab_size; i++) {
        for (int j = 0; j < embed_dim; j++) emb[i * embed_dim + j] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
    }
    return emb;
}

void add_noise(float* emb, int size, float scale) {
    for (int i = 0; i < size; i++) {
        float noise = ((float)rand() / RAND_MAX - 0.5f) * scale;
        emb[i] += noise;
    }
}

void blend_embeddings(float* target, float* source, int size, float alpha) {
    for (int i = 0; i < size; i++) {
        target[i] = (1.0f - alpha) * target[i] + alpha * source[i];
    }
}

int main() {
    srand(time(NULL));

    char* subjects[NUM_SUBJECTS] = {"Mathematics", "Physics", "Biology", "History", "Literature",
                                    "Programming", "Chemistry", "Geography", "Economics", "Astronomy"};

    // Load all corpora from curriculums.txt (reusing I/O from training code)
    FILE* f = fopen("curriculums.txt", "r");
    if (!f) { printf("Error opening curriculums.txt\n"); return 1; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* buffer = malloc(size + 1);
    if (!buffer) { printf("Malloc failed\n"); exit(1); }
    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);

    char corpora[NUM_SUBJECTS][1000];
    int corpus_lengths[NUM_SUBJECTS];
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
        if (len > 999) len = 999;
        strncpy(corpora[s], ptr, len);
        corpora[s][len] = '\0';
        corpus_lengths[s] = len;
    }

    // Process each corpus: tokenize and build vocab
    char* words[NUM_SUBJECTS][MAX_WORDS];
    int word_counts[NUM_SUBJECTS];
    char* vocabs[NUM_SUBJECTS][VOCAB_SIZE];
    int vocab_sizes[NUM_SUBJECTS];
    float* embeddings[NUM_SUBJECTS];

    for (int s = 0; s < NUM_SUBJECTS; s++) {
        tokenize(corpora[s], words[s], &word_counts[s]);
        build_vocab(words[s], word_counts[s], vocabs[s], &vocab_sizes[s]);
        embeddings[s] = hash_embedding(vocabs[s], vocab_sizes[s], EMBED_DIM);
    }

    // Unified vocabulary (combine all vocabs)
    char* unified_vocab[VOCAB_SIZE];
    int unified_vocab_size = 0;
    for (int s = 0; s < NUM_SUBJECTS; s++) {
        for (int i = 0; i < vocab_sizes[s]; i++) {
            int found = 0;
            for (int j = 0; j < unified_vocab_size; j++) {
                if (strcmp(vocabs[s][i], unified_vocab[j]) == 0) { found = 1; break; }
            }
            if (!found && unified_vocab_size < VOCAB_SIZE) unified_vocab[unified_vocab_size++] = strdup(vocabs[s][i]);
        }
    }

    // Initialize unified embeddings
    float* unified_emb = hash_embedding(unified_vocab, unified_vocab_size, EMBED_DIM);

    // Simple diffusion: iteratively blend embeddings with noise
    for (int step = 0; step < DIFFUSION_STEPS; step++) {
        float alpha = (float)(DIFFUSION_STEPS - step) / DIFFUSION_STEPS; // Annealing factor
        for (int s = 0; s < NUM_SUBJECTS; s++) {
            // Map subject embeddings to unified vocab space
            float* temp_emb = malloc(unified_vocab_size * EMBED_DIM * sizeof(float));
            if (!temp_emb) { printf("Malloc failed\n"); exit(1); }
            for (int i = 0; i < unified_vocab_size; i++) {
                int id = -1;
                for (int j = 0; j < vocab_sizes[s]; j++) {
                    if (strcmp(unified_vocab[i], vocabs[s][j]) == 0) { id = j; break; }
                }
                if (id != -1) memcpy(&temp_emb[i * EMBED_DIM], &embeddings[s][id * EMBED_DIM], EMBED_DIM * sizeof(float));
                else memset(&temp_emb[i * EMBED_DIM], 0, EMBED_DIM * sizeof(float));
            }
            // Add noise and blend
            add_noise(temp_emb, unified_vocab_size * EMBED_DIM, NOISE_SCALE * alpha);
            blend_embeddings(unified_emb, temp_emb, unified_vocab_size * EMBED_DIM, alpha / NUM_SUBJECTS);
            free(temp_emb);
        }
    }

    // Save unified embeddings
    save_matrix(unified_emb, unified_vocab_size, EMBED_DIM, "unified_embeddings.txt");

    // Generate a simple combined corpus by sampling words based on embedding magnitudes
    FILE* output_f = fopen("unified_corpus.txt", "w");
    if (!output_f) { printf("Error opening unified_corpus.txt\n"); return 1; }
    for (int i = 0; i < 100; i++) { // Generate 100 words
        float max_mag = -INFINITY;
        int best_idx = 0;
        for (int j = 0; j < unified_vocab_size; j++) {
            float mag = 0;
            for (int d = 0; d < EMBED_DIM; d++) mag += unified_emb[j * EMBED_DIM + d] * unified_emb[j * EMBED_DIM + d];
            if (mag > max_mag) { max_mag = mag; best_idx = j; }
        }
        fprintf(output_f, "%s ", unified_vocab[best_idx]);
        unified_emb[best_idx * EMBED_DIM] = 0; // Prevent repetition
    }
    fclose(output_f);

    // Cleanup
    for (int s = 0; s < NUM_SUBJECTS; s++) {
        for (int i = 0; i < word_counts[s]; i++) free(words[s][i]);
        for (int i = 0; i < vocab_sizes[s]; i++) free(vocabs[s][i]);
        free(embeddings[s]);
    }
    for (int i = 0; i < unified_vocab_size; i++) free(unified_vocab[i]);
    free(unified_emb);
    free(buffer);

    return 0;
}
