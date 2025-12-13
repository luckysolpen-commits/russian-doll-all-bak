#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#define MAX_WORDS 10000
#define VOCAB_SIZE 100
#define EMBED_DIM 16
#define NUM_HEADS 2
#define FF_DIM 32
#define LEARNING_RATE 0.011f
#define EPOCHS 50
#define CLIP_VALUE 5.0f // Increased from 1.0
#define L2_REG 0.0001f // L2 regularization
#define GRAD_ACCUM_STEPS 3 // Gradient accumulation
#define DROPOUT_RATE 0.2f // 20% dropout
#define NUM_SUBJECTS 10
 int word_count;
void save_matrix(float* matrix, int rows, int cols, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) { printf("Error opening %s\n", filename); exit(1); }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) fprintf(f, "%.4f ", matrix[i * cols + j]);
        fprintf(f, "\n");
    }
    fclose(f);
}

float* load_matrix(const char* filename, int rows, int cols) {
    FILE* f = fopen(filename, "r");
    if (!f) { printf("Error opening %s for reading\n", filename); return NULL; }
    float* matrix = malloc(rows * cols * sizeof(float));
    if (!matrix) { printf("Malloc failed\n"); exit(1); }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (fscanf(f, "%f", &matrix[i * cols + j]) != 1) {
                printf("Error reading %s\n", filename);
                free(matrix);
                fclose(f);
                return NULL;
            }
        }
    }
    fclose(f);
    return matrix;
}

float get_learning_rate(int epoch) {
    return LEARNING_RATE * (1.0f - (float)epoch / EPOCHS); // Linear decay
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
    float scale = sqrtf(6.0f / (vocab_size + embed_dim)); // Xavier initialization
    for (int i = 0; i < vocab_size * embed_dim; i++) emb[i] = ((float)rand() / RAND_MAX - 0.5f) * scale;
    return emb;
}

float* rope_embedding(float* emb, int seq_len, int embed_dim) {
    float* rope = malloc(seq_len * embed_dim * sizeof(float));
    if (!rope) { printf("Malloc failed\n"); exit(1); }
    for (int i = 0; i < seq_len * embed_dim; i++) rope[i] = 0.0f;
    for (int i = 0; i < seq_len; i++) {
        for (int j = 0; j < embed_dim; j += 2) {
            if (j + 1 < embed_dim) {
                float theta = 10000.0f / powf(10000.0f, (float)j / embed_dim);
                float angle = i * theta;
                rope[i * embed_dim + j] = emb[i * embed_dim + j] * cosf(angle) - emb[i * embed_dim + j + 1] * sinf(angle);
                rope[i * embed_dim + j + 1] = emb[i * embed_dim + j] * sinf(angle) + emb[i * embed_dim + j + 1] * cosf(angle);
            }
        }
    }
    return rope;
}

void masked_attention(float* input, int seq_len, float* attn_weights, float* output, float** q_out, float** k_out, float** v_out, float** scores_out) {
    float* q = malloc(seq_len * EMBED_DIM * sizeof(float));
    float* k = malloc(seq_len * EMBED_DIM * sizeof(float));
    float* v = malloc(seq_len * EMBED_DIM * sizeof(float));
    float* scores = malloc(seq_len * seq_len * sizeof(float));
    if (!q || !k || !v || !scores) { printf("Malloc failed\n"); exit(1); }

    float* w_q = attn_weights;
    float* w_k = attn_weights + EMBED_DIM * EMBED_DIM;
    float* w_v = attn_weights + 2 * EMBED_DIM * EMBED_DIM;

    for (int i = 0; i < seq_len; i++) {
        for (int d = 0; d < EMBED_DIM; d++) {
            q[i * EMBED_DIM + d] = 0;
            k[i * EMBED_DIM + d] = 0;
            v[i * EMBED_DIM + d] = 0;
            for (int j = 0; j < EMBED_DIM; j++) {
                q[i * EMBED_DIM + d] += input[i * EMBED_DIM + j] * w_q[j * EMBED_DIM + d];
                k[i * EMBED_DIM + d] += input[i * EMBED_DIM + j] * w_k[j * EMBED_DIM + d];
                v[i * EMBED_DIM + d] += input[i * EMBED_DIM + j] * w_v[j * EMBED_DIM + d];
            }
        }
    }

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

    *q_out = q;
    *k_out = k;
    *v_out = v;
    *scores_out = scores;
}

void feed_forward(float* input, int seq_len, float* w1, float* w2, float* output, float** hidden_out) {
    float* hidden = malloc(seq_len * FF_DIM * sizeof(float));
    if (!hidden) { printf("Malloc failed\n"); exit(1); }
    for (int i = 0; i < seq_len; i++) {
        for (int j = 0; j < FF_DIM; j++) {
            hidden[i * FF_DIM + j] = 0;
            for (int d = 0; d < EMBED_DIM; d++) hidden[i * FF_DIM + j] += input[i * EMBED_DIM + d] * w1[d * FF_DIM + j];
            hidden[i * FF_DIM + j] = hidden[i * FF_DIM + j] > 0 ? hidden[i * FF_DIM + j] : 0;
            if ((float)rand() / RAND_MAX < DROPOUT_RATE) hidden[i * FF_DIM + j] = 0; // Dropout
        }
    }
    for (int i = 0; i < seq_len; i++) {
        for (int d = 0; d < EMBED_DIM; d++) {
            output[i * EMBED_DIM + d] = 0;
            for (int j = 0; j < FF_DIM; j++) output[i * EMBED_DIM + d] += hidden[i * FF_DIM + j] * w2[j * EMBED_DIM + d];
        }
    }
    *hidden_out = hidden;
}

float train(float* word_emb, float* attn_weights, float* ff_weights1, float* ff_weights2, float* biases,
            float* input, int seq_len, int* targets, int vocab_size, int step, int epoch) {
    static float* grad_word_emb = NULL;
    static float* grad_attn_weights = NULL;
    static float* grad_ff_weights1 = NULL;
    static float* grad_ff_weights2 = NULL;
    static float* grad_biases = NULL;
    static int accum_count = 0;

    if (!grad_word_emb) {
        grad_word_emb = calloc(vocab_size * EMBED_DIM, sizeof(float));
        grad_attn_weights = calloc(3 * EMBED_DIM * EMBED_DIM, sizeof(float));
        grad_ff_weights1 = calloc(EMBED_DIM * FF_DIM, sizeof(float));
        grad_ff_weights2 = calloc(FF_DIM * EMBED_DIM, sizeof(float));
        grad_biases = calloc(EMBED_DIM, sizeof(float));
    }

    float *attn_out = malloc(seq_len * EMBED_DIM * sizeof(float));
    float *ff_out = malloc(seq_len * EMBED_DIM * sizeof(float));
    float *logits = malloc(vocab_size * sizeof(float));
    float *q, *k, *v, *scores, *hidden;
    if (!attn_out || !ff_out || !logits) { printf("Malloc failed\n"); exit(1); }

    masked_attention(input, seq_len, attn_weights, attn_out, &q, &k, &v, &scores);
    feed_forward(attn_out, seq_len, ff_weights1, ff_weights2, ff_out, &hidden);
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
    for (int i = 0; i < vocab_size; i++) logits[i] /= sum;

    float loss = -logf(logits[targets[seq_len - 1]] + 1e-10f);

    float* d_logits = malloc(vocab_size * sizeof(float));
    float* d_ff_out = malloc(seq_len * EMBED_DIM * sizeof(float));
    float* d_hidden = malloc(seq_len * FF_DIM * sizeof(float));
    float* d_attn_out = malloc(seq_len * EMBED_DIM * sizeof(float));
    float* d_attn_weights = malloc(3 * EMBED_DIM * EMBED_DIM * sizeof(float));
    if (!d_logits || !d_ff_out || !d_hidden || !d_attn_out || !d_attn_weights) { printf("Malloc failed\n"); exit(1); }

    for (int i = 0; i < vocab_size; i++) d_logits[i] = logits[i] - (i == targets[seq_len - 1] ? 1.0f : 0.0f);

    for (int i = 0; i < seq_len * EMBED_DIM; i++) d_ff_out[i] = 0;
    for (int i = 0; i < vocab_size; i++) {
        for (int d = 0; d < EMBED_DIM; d++) {
            float grad = d_logits[i] * word_emb[i * EMBED_DIM + d];
            d_ff_out[(seq_len - 1) * EMBED_DIM + d] += grad;
            if (grad > CLIP_VALUE) grad = CLIP_VALUE;
            if (grad < -CLIP_VALUE) grad = -CLIP_VALUE;
            grad_word_emb[i * EMBED_DIM + d] += grad + L2_REG * word_emb[i * EMBED_DIM + d];
        }
    }

    for (int i = 0; i < seq_len * FF_DIM; i++) d_hidden[i] = 0;
    for (int i = 0; i < seq_len; i++) {
        for (int j = 0; j < FF_DIM; j++) {
            for (int d = 0; d < EMBED_DIM; d++) {
                float grad = d_ff_out[i * EMBED_DIM + d] * ff_weights2[j * EMBED_DIM + d];
                d_hidden[i * FF_DIM + j] += grad * (hidden[i * FF_DIM + j] > 0 ? 1.0f : 0.0f);
                if (grad > CLIP_VALUE) grad = CLIP_VALUE;
                if (grad < -CLIP_VALUE) grad = -CLIP_VALUE;
                grad_ff_weights2[j * EMBED_DIM + d] += grad * hidden[i * FF_DIM + j] + L2_REG * ff_weights2[j * EMBED_DIM + d];
            }
        }
    }

    for (int i = 0; i < seq_len * EMBED_DIM; i++) d_attn_out[i] = 0;
    for (int i = 0; i < seq_len; i++) {
        for (int d = 0; d < EMBED_DIM; d++) {
            for (int j = 0; j < FF_DIM; j++) {
                float grad = d_hidden[i * FF_DIM + j] * ff_weights1[d * FF_DIM + j];
                d_attn_out[i * EMBED_DIM + d] += grad;
                if (grad > CLIP_VALUE) grad = CLIP_VALUE;
                if (grad < -CLIP_VALUE) grad = -CLIP_VALUE;
                grad_ff_weights1[d * FF_DIM + j] += grad * attn_out[i * EMBED_DIM + d] + L2_REG * ff_weights1[d * FF_DIM + j];
            }
        }
    }

    for (int i = 0; i < 3 * EMBED_DIM * EMBED_DIM; i++) d_attn_weights[i] = 0;
    float* d_v = malloc(seq_len * EMBED_DIM * sizeof(float));
    float* d_scores = malloc(seq_len * seq_len * sizeof(float));
    if (!d_v || !d_scores) { printf("Malloc failed\n"); exit(1); }
    for (int i = 0; i < seq_len; i++) {
        for (int d = 0; d < EMBED_DIM; d++) {
            d_v[i * EMBED_DIM + d] = 0;
            for (int j = 0; j < seq_len; j++) d_v[i * EMBED_DIM + d] += d_attn_out[j * EMBED_DIM + d] * scores[j * seq_len + i];
        }
    }
    for (int i = 0; i < seq_len; i++) {
        for (int j = 0; j < seq_len; j++) {
            d_scores[i * seq_len + j] = 0;
            for (int d = 0; d < EMBED_DIM; d++) d_scores[i * seq_len + j] += d_attn_out[i * EMBED_DIM + d] * v[j * EMBED_DIM + d];
            d_scores[i * seq_len + j] *= scores[i * seq_len + j] * (1 - scores[i * seq_len + j]);
        }
    }
    float* w_q = attn_weights;
    float* w_k = attn_weights + EMBED_DIM * EMBED_DIM;
    float* w_v = attn_weights + 2 * EMBED_DIM * EMBED_DIM;
    for (int i = 0; i < seq_len; i++) {
        for (int d = 0; d < EMBED_DIM; d++) {
            for (int j = 0; j < EMBED_DIM; j++) {
                float grad_q = 0, grad_k = 0, grad_v = 0;
                for (int m = 0; m <= i; m++) {
                    grad_q += d_scores[i * seq_len + m] * k[m * EMBED_DIM + d] / sqrtf(EMBED_DIM);
                    grad_k += d_scores[i * seq_len + m] * q[i * EMBED_DIM + d] / sqrtf(EMBED_DIM);
                }
                grad_v = d_v[i * EMBED_DIM + d];
                if (grad_q > CLIP_VALUE) grad_q = CLIP_VALUE; if (grad_q < -CLIP_VALUE) grad_q = -CLIP_VALUE;
                if (grad_k > CLIP_VALUE) grad_k = CLIP_VALUE; if (grad_k < -CLIP_VALUE) grad_k = -CLIP_VALUE;
                if (grad_v > CLIP_VALUE) grad_v = CLIP_VALUE; if (grad_v < -CLIP_VALUE) grad_v = -CLIP_VALUE;
                grad_attn_weights[j * EMBED_DIM + d] += grad_q * input[i * EMBED_DIM + j] + L2_REG * w_q[j * EMBED_DIM + d];
                grad_attn_weights[(EMBED_DIM * EMBED_DIM) + j * EMBED_DIM + d] += grad_k * input[i * EMBED_DIM + j] + L2_REG * w_k[j * EMBED_DIM + d];
                grad_attn_weights[(2 * EMBED_DIM * EMBED_DIM) + j * EMBED_DIM + d] += grad_v * input[i * EMBED_DIM + j] + L2_REG * w_v[j * EMBED_DIM + d];
            }
        }
    }

    for (int i = 0; i < EMBED_DIM; i++) {
        float grad = d_ff_out[(seq_len - 1) * EMBED_DIM + i];
        if (grad > CLIP_VALUE) grad = CLIP_VALUE;
        if (grad < -CLIP_VALUE) grad = -CLIP_VALUE;
        grad_biases[i] += grad + L2_REG * biases[i];
    }

    accum_count++;
    if (accum_count == GRAD_ACCUM_STEPS || step == word_count - 2) {
        float lr = get_learning_rate(epoch);
        for (int i = 0; i < vocab_size * EMBED_DIM; i++) word_emb[i] -= lr * grad_word_emb[i] / GRAD_ACCUM_STEPS;
        for (int i = 0; i < 3 * EMBED_DIM * EMBED_DIM; i++) attn_weights[i] -= lr * grad_attn_weights[i] / GRAD_ACCUM_STEPS;
        for (int i = 0; i < EMBED_DIM * FF_DIM; i++) ff_weights1[i] -= lr * grad_ff_weights1[i] / GRAD_ACCUM_STEPS;
        for (int i = 0; i < FF_DIM * EMBED_DIM; i++) ff_weights2[i] -= lr * grad_ff_weights2[i] / GRAD_ACCUM_STEPS;
        for (int i = 0; i < EMBED_DIM; i++) biases[i] -= lr * grad_biases[i] / GRAD_ACCUM_STEPS;
        memset(grad_word_emb, 0, vocab_size * EMBED_DIM * sizeof(float));
        memset(grad_attn_weights, 0, 3 * EMBED_DIM * EMBED_DIM * sizeof(float));
        memset(grad_ff_weights1, 0, EMBED_DIM * FF_DIM * sizeof(float));
        memset(grad_ff_weights2, 0, FF_DIM * EMBED_DIM * sizeof(float));
        memset(grad_biases, 0, EMBED_DIM * sizeof(float));
        accum_count = 0;
    }

    free(attn_out); free(ff_out); free(logits); free(q); free(k); free(v); free(scores); free(hidden);
    free(d_logits); free(d_ff_out); free(d_hidden); free(d_attn_out); free(d_attn_weights); free(d_v); free(d_scores);
    return loss;
}

int main(int argc, char** argv) {
    if (argc != 2 || (strcmp(argv[1], "init") != 0 && strcmp(argv[1], "resume") != 0)) {
        printf("Usage: %s <init|resume>\n", argv[0]);
        return 1;
    }
    int resume = (strcmp(argv[1], "resume") == 0);

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

    char* subjects[NUM_SUBJECTS] = {"Mathematics", "Physics", "Biology", "History", "Literature", "Programming", "Chemistry", "Geography", "Economics", "Astronomy"};
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

        char folder[50];
        snprintf(folder, 50, "curricula/%s", subjects[s]);
        mkdir(folder, 0755);

        char filepath[100];
        snprintf(filepath, 100, "%s/curriculum.txt", folder);
        FILE* cf = fopen(filepath, "w");
        if (!cf) { printf("Error opening %s\n", filepath); return 1; }
        fprintf(cf, "%s", corpora[s]);
        fclose(cf);

        char augmented[2000];
        snprintf(augmented, 2000, "%s %s", corpora[s], corpora[s]); // Data augmentation: duplicate
        char* words[MAX_WORDS];
       
        tokenize(augmented, words, &word_count);

        char* vocab[VOCAB_SIZE];
        int vocab_size;
        build_vocab(words, word_count, vocab, &vocab_size);

        snprintf(filepath, 100, "%s/index.txt", folder);
        FILE* index_f = fopen(filepath, "w");
        if (!index_f) { printf("Error opening %s\n", filepath); return 1; }
        for (int i = 0; i < vocab_size; i++) fprintf(index_f, "%d %s\n", i, vocab[i]);
        fclose(index_f);

        float* word_emb;
        float* attn_weights;
        float* ff_weights1;
        float* ff_weights2;
        float* biases;

        if (resume) {
            snprintf(filepath, 100, "%s/hash_embeddings.txt", folder);
            word_emb = load_matrix(filepath, vocab_size, EMBED_DIM);
            snprintf(filepath, 100, "%s/attention_weights.txt", folder);
            attn_weights = load_matrix(filepath, 3 * EMBED_DIM, EMBED_DIM);
            snprintf(filepath, 100, "%s/ff_weights1.txt", folder);
            ff_weights1 = load_matrix(filepath, EMBED_DIM, FF_DIM);
            snprintf(filepath, 100, "%s/ff_weights2.txt", folder);
            ff_weights2 = load_matrix(filepath, FF_DIM, EMBED_DIM);
            snprintf(filepath, 100, "%s/biases.txt", folder);
            biases = load_matrix(filepath, 1, EMBED_DIM);
            if (!word_emb || !attn_weights || !ff_weights1 || !ff_weights2 || !biases) {
                printf("Failed to load weights for %s, falling back to init\n", subjects[s]);
                resume = 0;
            }
        }

        if (!resume) {
            word_emb = hash_embedding(vocab, vocab_size, EMBED_DIM);
            attn_weights = malloc(3 * EMBED_DIM * EMBED_DIM * sizeof(float));
            ff_weights1 = malloc(EMBED_DIM * FF_DIM * sizeof(float));
            ff_weights2 = malloc(FF_DIM * EMBED_DIM * sizeof(float));
            biases = malloc(EMBED_DIM * sizeof(float));
            if (!attn_weights || !ff_weights1 || !ff_weights2 || !biases) { printf("Malloc failed\n"); exit(1); }
            float scale_attn = sqrtf(6.0f / (EMBED_DIM + EMBED_DIM));
            float scale_ff1 = sqrtf(6.0f / (EMBED_DIM + FF_DIM));
            float scale_ff2 = sqrtf(6.0f / (FF_DIM + EMBED_DIM));
            for (int i = 0; i < 3 * EMBED_DIM * EMBED_DIM; i++) attn_weights[i] = ((float)rand() / RAND_MAX - 0.5f) * scale_attn;
            for (int i = 0; i < EMBED_DIM * FF_DIM; i++) ff_weights1[i] = ((float)rand() / RAND_MAX - 0.5f) * scale_ff1;
            for (int i = 0; i < FF_DIM * EMBED_DIM; i++) ff_weights2[i] = ((float)rand() / RAND_MAX - 0.5f) * scale_ff2;
            for (int i = 0; i < EMBED_DIM; i++) biases[i] = 0.0f;
        }

        int* targets = malloc(word_count * sizeof(int));
        if (!targets) { printf("Malloc failed\n"); exit(1); }
        for (int i = 0; i < word_count; i++) {
            targets[i] = -1;
            for (int j = 0; j < vocab_size; j++) {
                if (strcmp(words[i], vocab[j]) == 0) { targets[i] = j; break; }
            }
        }

        float* input = malloc(word_count * EMBED_DIM * sizeof(float));
        if (!input) { printf("Malloc failed\n"); exit(1); }
        snprintf(filepath, 100, "%s/loss.txt", folder);
        FILE* loss_f = fopen(filepath, resume ? "a" : "w");
        if (!loss_f) { printf("Error opening %s\n", filepath); return 1; }
        for (int epoch = 0; epoch < EPOCHS; epoch++) {
            float total_loss = 0;
            int steps = 0;
            for (int i = 0; i < word_count - 1; i++) {
                for (int j = 0; j <= i; j++) {
                    if (targets[j] != -1) memcpy(&input[j * EMBED_DIM], &word_emb[targets[j] * EMBED_DIM], EMBED_DIM * sizeof(float));
                }
                float* rope_emb = rope_embedding(input, i + 1, EMBED_DIM);
                total_loss += train(word_emb, attn_weights, ff_weights1, ff_weights2, biases, rope_emb, i + 1, targets, vocab_size, i, epoch);
                free(rope_emb);
                steps++;
            }
            fprintf(loss_f, "Epoch %d: Loss %.4f\n", epoch + 1, total_loss / steps);
            printf("%s - Epoch %d: Loss %.4f\n", subjects[s], epoch + 1, total_loss / steps);
        }
        fclose(loss_f);

        snprintf(filepath, 100, "%s/hash_embeddings.txt", folder);
        save_matrix(word_emb, vocab_size, EMBED_DIM, filepath);
        snprintf(filepath, 100, "%s/attention_weights.txt", folder);
        save_matrix(attn_weights, 3 * EMBED_DIM, EMBED_DIM, filepath);
        snprintf(filepath, 100, "%s/ff_weights1.txt", folder);
        save_matrix(ff_weights1, EMBED_DIM, FF_DIM, filepath);
        snprintf(filepath, 100, "%s/ff_weights2.txt", folder);
        save_matrix(ff_weights2, FF_DIM, EMBED_DIM, filepath);
        snprintf(filepath, 100, "%s/biases.txt", folder);
        save_matrix(biases, 1, EMBED_DIM, filepath);
        snprintf(filepath, 100, "%s/model.txt", folder);
        FILE* model_f = fopen(filepath, "w");
        if (!model_f) { printf("Error opening %s\n", filepath); return 1; }
        fprintf(model_f, "vocab_size: %d\nembed_dim: %d\n", vocab_size, EMBED_DIM);
        fclose(model_f);

        for (int i = 0; i < word_count; i++) free(words[i]);
        for (int i = 0; i < vocab_size; i++) free(vocab[i]);
        free(word_emb); free(attn_weights); free(ff_weights1); free(ff_weights2); free(biases);
        free(targets); free(input);
    }

    free(buffer);
    return 0;
}
