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
#define LEARNING_RATE 0.005f
#define EPOCHS 50
#define CLIP_VALUE 1.0f
#define NUM_SUBJECTS 10

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
    if (!f) return NULL; // If file doesn’t exist, return NULL
    float* matrix = malloc(rows * cols * sizeof(float));
    if (!matrix) { printf("Malloc failed\n"); exit(1); }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (fscanf(f, "%f", &matrix[i * cols + j]) != 1) {
                fclose(f);
                free(matrix);
                return NULL;
            }
        }
    }
    fclose(f);
    return matrix;
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
        for (int j = 0; j < embed_dim; j++) {
            emb[i * embed_dim + j] = ((float)rand() / RAND_MAX - 0.5f) * sqrtf(6.0f / (vocab_size + embed_dim));
        }
    }
    return emb;
}

float* rope_embedding(float* emb, int seq_len, int embed_dim) {
    float* rope = malloc(seq_len * embed_dim * sizeof(float));
    if (!rope) { printf("Malloc failed\n"); exit(1); }
    for (int i = 0; i < seq_len * embed_dim; i++) rope[i] = 0.0f;
    for (int i = 0; i < seq_len; i++) {
        for (int j = 0; j < embed_dim; j += 2) {
            if (j + 1 < embed_dim) {
                float theta = 10000.0f / powf(10000.0f, (float)j / (embed_dim / 2.0f));
                float angle = i * theta;
                rope[i * embed_dim + j] = emb[i * embed_dim + j] * cosf(angle) - emb[i * embed_dim + j + 1] * sinf(angle);
                rope[i * embed_dim + j + 1] = emb[i * embed_dim + j] * sinf(angle) + emb[i * embed_dim + j + 1] * cosf(angle);
            }
        }
    }
    return rope;
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
            scores[i * seq_len + j] = score;
        }
        for (int j = i + 1; j < seq_len; j++) scores[i * seq_len + j] = -INFINITY;
    }
    printf("Scores[0]: %.6f\n", scores[0]);

    for (int i = 0; i < seq_len; i++) {
        float max_score = -INFINITY;
        for (int j = 0; j < seq_len; j++) if (scores[i * seq_len + j] > max_score) max_score = scores[i * seq_len + j];
        float sum = 0.0f;
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

void normalize(float* vec, int size) {
    float mean = 0.0f, variance = 0.0f;
    for (int i = 0; i < size; i++) mean += vec[i];
    mean /= size;
    for (int i = 0; i < size; i++) variance += (vec[i] - mean) * (vec[i] - mean);
    variance = sqrtf(variance / size + 1e-5f);
    for (int i = 0; i < size; i++) vec[i] = (vec[i] - mean) / variance;
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
    printf("Hidden[0]: %.6f\n", hidden[(seq_len - 1) * FF_DIM + 0]);
    for (int i = 0; i < seq_len; i++) {
        for (int d = 0; d < EMBED_DIM; d++) {
            output[i * EMBED_DIM + d] = 0;
            for (int j = 0; j < FF_DIM; j++) output[i * EMBED_DIM + d] += hidden[i * FF_DIM + j] * w2[j * EMBED_DIM + d];
        }
    }
    normalize(output, seq_len * EMBED_DIM);
    free(hidden);
}

float train(float* word_emb, float* attn_weights, float* ff_weights1, float* ff_weights2, float* biases,
            float* input, int seq_len, int* targets, int vocab_size) {
    float* attn_out = malloc(seq_len * EMBED_DIM * sizeof(float));
    float* ff_out = malloc(seq_len * EMBED_DIM * sizeof(float));
    float* logits = malloc(vocab_size * sizeof(float));
    if (!attn_out || !ff_out || !logits) { printf("Malloc failed\n"); exit(1); }

    printf("Attn Weights[0]: %.6f\n", attn_weights[0]);
    masked_attention(input, seq_len, attn_weights, attn_out);
    printf("Attn Out[0]: %.6f\n", attn_out[(seq_len - 1) * EMBED_DIM + 0]);
    feed_forward(attn_out, seq_len, ff_weights1, ff_weights2, ff_out);

    printf("FF Out[0]: %.6f, FF Out[1]: %.6f\n", ff_out[(seq_len - 1) * EMBED_DIM + 0], ff_out[(seq_len - 1) * EMBED_DIM + 1]);

    float max_logit = -INFINITY;
    for (int i = 0; i < vocab_size; i++) {
        logits[i] = 0;
        for (int d = 0; d < EMBED_DIM; d++) {
            logits[i] += ff_out[(seq_len - 1) * EMBED_DIM + d] * word_emb[i * EMBED_DIM + d];
        }
        if (logits[i] > max_logit) max_logit = logits[i];
    }
    float sum = 0.0f;
    for (int i = 0; i < vocab_size; i++) {
        logits[i] = expf(logits[i] - max_logit);
        sum += logits[i];
    }
    for (int i = 0; i < vocab_size; i++) logits[i] /= sum;

    float target_prob = logits[targets[seq_len - 1]];
    if (target_prob < 1e-10f) target_prob = 1e-10f;
    float loss = -logf(target_prob);
    if (isnan(loss)) {
        printf("NaN detected, resetting loss to 10.0\n");
        loss = 10.0f;
    }
    printf("Target Prob: %.6f, Loss: %.6f\n", target_prob, loss);

    float* d_logits = malloc(vocab_size * sizeof(float));
    float* gradients = malloc(vocab_size * EMBED_DIM * sizeof(float));
    if (!d_logits || !gradients) { printf("Malloc failed\n"); exit(1); }
    for (int i = 0; i < vocab_size; i++) d_logits[i] = logits[i] - (i == targets[seq_len - 1] ? 1.0f : 0.0f);
    printf("d_logits[target]: %.6f\n", d_logits[targets[seq_len - 1]]);

    for (int i = 0; i < vocab_size; i++) {
        for (int d = 0; d < EMBED_DIM; d++) {
            float grad = d_logits[i] * ff_out[(seq_len - 1) * EMBED_DIM + d];
            if (grad > CLIP_VALUE) grad = CLIP_VALUE;
            else if (grad < -CLIP_VALUE) grad = -CLIP_VALUE;
            gradients[i * EMBED_DIM + d] = grad;
        }
    }
    printf("Gradient[0]: %.6f, Gradient[1]: %.6f\n", gradients[0], gradients[1]);

    float old_weight = word_emb[0];
    for (int i = 0; i < vocab_size; i++) {
        for (int d = 0; d < EMBED_DIM; d++) {
            float update = LEARNING_RATE * gradients[i * EMBED_DIM + d];
            word_emb[i * EMBED_DIM + d] -= update;
        }
    }
    printf("Old Weight[0]: %.6f, New Weight[0]: %.6f\n", old_weight, word_emb[0]);

    float* d_ff_out = malloc(seq_len * EMBED_DIM * sizeof(float));
    if (!d_ff_out) { printf("Malloc failed\n"); exit(1); }
    for (int i = 0; i < seq_len * EMBED_DIM; i++) d_ff_out[i] = 0;
    for (int i = 0; i < vocab_size; i++) {
        for (int d = 0; d < EMBED_DIM; d++) {
            d_ff_out[(seq_len - 1) * EMBED_DIM + d] += gradients[i * EMBED_DIM + d];
        }
    }

    float* d_hidden = malloc(seq_len * FF_DIM * sizeof(float));
    if (!d_hidden) { printf("Malloc failed\n"); exit(1); }
    for (int i = 0; i < seq_len; i++) {
        for (int j = 0; j < FF_DIM; j++) {
            d_hidden[i * FF_DIM + j] = 0;
            for (int d = 0; d < EMBED_DIM; d++) {
                float grad = d_ff_out[i * EMBED_DIM + d] * ff_weights2[j * EMBED_DIM + d];
                if (grad > CLIP_VALUE) grad = CLIP_VALUE;
                else if (grad < -CLIP_VALUE) grad = -CLIP_VALUE;
                d_hidden[i * FF_DIM + j] += grad;
                ff_weights2[j * EMBED_DIM + d] -= LEARNING_RATE * grad * (ff_out[i * EMBED_DIM + j] > 0 ? 1.0f : 0.0f);
            }
        }
    }

    for (int i = 0; i < seq_len; i++) {
        for (int d = 0; d < EMBED_DIM; d++) {
            for (int j = 0; j < FF_DIM; j++) {
                float grad = d_hidden[i * FF_DIM + j] * input[i * EMBED_DIM + d];
                if (grad > CLIP_VALUE) grad = CLIP_VALUE;
                else if (grad < -CLIP_VALUE) grad = -CLIP_VALUE;
                ff_weights1[d * FF_DIM + j] -= LEARNING_RATE * grad;
            }
        }
    }

    free(attn_out); free(ff_out); free(logits); free(d_logits); free(d_ff_out); free(d_hidden); free(gradients);
    return loss;
}

int main(int argc, char** argv) {
    if (argc != 2 || (strcmp(argv[1], "init") != 0 && strcmp(argv[1], "resume") != 0)) {
        printf("Usage: %s [init|resume]\n", argv[0]);
        return 1;
    }
    int resume = strcmp(argv[1], "resume") == 0;

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

        char* words[MAX_WORDS];
        int word_count;
        tokenize(corpora[s], words, &word_count);

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
            attn_weights = load_matrix(filepath, EMBED_DIM, EMBED_DIM);
            snprintf(filepath, 100, "%s/ff_weights1.txt", folder);
            ff_weights1 = load_matrix(filepath, EMBED_DIM, FF_DIM);
            snprintf(filepath, 100, "%s/ff_weights2.txt", folder);
            ff_weights2 = load_matrix(filepath, FF_DIM, EMBED_DIM);
            snprintf(filepath, 100, "%s/biases.txt", folder);
            biases = load_matrix(filepath, 1, EMBED_DIM);

            if (!word_emb || !attn_weights || !ff_weights1 || !ff_weights2 || !biases) {
                printf("%s - No previous weights found, initializing instead\n", subjects[s]);
                resume = 0; // Fall back to init if any file is missing
            }
        }

        if (!resume) {
            word_emb = hash_embedding(vocab, vocab_size, EMBED_DIM);
            attn_weights = malloc(EMBED_DIM * EMBED_DIM * sizeof(float));
            ff_weights1 = malloc(EMBED_DIM * FF_DIM * sizeof(float));
            ff_weights2 = malloc(FF_DIM * EMBED_DIM * sizeof(float));
            biases = malloc(EMBED_DIM * sizeof(float));
            if (!attn_weights || !ff_weights1 || !ff_weights2 || !biases) { printf("Malloc failed\n"); exit(1); }
            for (int i = 0; i < EMBED_DIM * EMBED_DIM; i++) attn_weights[i] = ((float)rand() / RAND_MAX - 0.5f) * sqrtf(6.0f / (EMBED_DIM + EMBED_DIM));
            for (int i = 0; i < EMBED_DIM * FF_DIM; i++) ff_weights1[i] = ((float)rand() / RAND_MAX - 0.5f) * sqrtf(6.0f / (EMBED_DIM + FF_DIM)) * 2.0f;
            for (int i = 0; i < FF_DIM * EMBED_DIM; i++) ff_weights2[i] = ((float)rand() / RAND_MAX - 0.5f) * sqrtf(6.0f / (FF_DIM + EMBED_DIM)) * 2.0f;
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

        // Initial loss check
        for (int j = 0; j < word_count - 1; j++) {
            if (targets[j] != -1) memcpy(&input[j * EMBED_DIM], &word_emb[targets[j] * EMBED_DIM], EMBED_DIM * sizeof(float));
        }
        float* rope_emb = rope_embedding(input, word_count - 1, EMBED_DIM);
        float init_loss = train(word_emb, attn_weights, ff_weights1, ff_weights2, biases, rope_emb, word_count - 1, targets, vocab_size);
        printf("%s - Initial Loss: %.4f\n", subjects[s], init_loss);
        free(rope_emb);

        snprintf(filepath, 100, "%s/loss.txt", folder);
        FILE* loss_f = fopen(filepath, "w");
        if (!loss_f) { printf("Error opening %s\n", filepath); return 1; }
        for (int epoch = 0; epoch < EPOCHS; epoch++) {
            float total_loss = 0;
            int steps = 0;
            for (int i = 0; i < word_count - 1; i++) {
                for (int j = 0; j <= i; j++) {
                    if (targets[j] != -1) memcpy(&input[j * EMBED_DIM], &word_emb[targets[j] * EMBED_DIM], EMBED_DIM * sizeof(float));
                }
                float* rope_emb = rope_embedding(input, i + 1, EMBED_DIM);
                total_loss += train(word_emb, attn_weights, ff_weights1, ff_weights2, biases, rope_emb, i + 1, targets, vocab_size);
                free(rope_emb);
                steps++;
            }
            float avg_loss = total_loss / steps;
            fprintf(loss_f, "Epoch %d: Loss %.4f\n", epoch + 1, avg_loss);
            printf("%s - Epoch %d: Loss %.4f\n", subjects[s], epoch + 1, avg_loss);
        }
        fclose(loss_f);

        if (word_count > 1) {
            float* last_input = malloc(EMBED_DIM * sizeof(float));
            memcpy(last_input, &word_emb[targets[word_count - 2] * EMBED_DIM], EMBED_DIM * sizeof(float));
            float* rope_emb = rope_embedding(last_input, 1, EMBED_DIM);
            float* attn_out = malloc(EMBED_DIM * sizeof(float));
            masked_attention(rope_emb, 1, attn_weights, attn_out);
            float* ff_out = malloc(EMBED_DIM * sizeof(float));
            feed_forward(attn_out, 1, ff_weights1, ff_weights2, ff_out);
            float max_prob = -INFINITY;
            int predicted = 0;
            for (int i = 0; i < vocab_size; i++) {
                float logit = 0;
                for (int d = 0; d < EMBED_DIM; d++) logit += ff_out[d] * word_emb[i * EMBED_DIM + d];
                if (logit > max_prob) { max_prob = logit; predicted = i; }
            }
            printf("%s - Predicted next token after '%s': %s\n", subjects[s], words[word_count - 2], vocab[predicted]);
            free(last_input); free(rope_emb); free(attn_out); free(ff_out);
        }

        snprintf(filepath, 100, "%s/hash_embeddings.txt", folder);
        save_matrix(word_emb, vocab_size, EMBED_DIM, filepath);
        snprintf(filepath, 100, "%s/attention_weights.txt", folder);
        save_matrix(attn_weights, EMBED_DIM, EMBED_DIM, filepath);
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
