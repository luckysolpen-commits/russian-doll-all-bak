#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_SUBJECTS 10
#define MAX_CORPUS 1000
#define MAX_WORDS 1000
#define MAX_WORD_LEN 50

void tokenize(char* text, char words[MAX_WORDS][MAX_WORD_LEN], int* word_count) {
    *word_count = 0;
    char* token = strtok(text, " \n");
    while (token && *word_count < MAX_WORDS) {
        strncpy(words[(*word_count)++], token, MAX_WORD_LEN - 1);
        words[*word_count - 1][MAX_WORD_LEN - 1] = '\0';
        token = strtok(NULL, " \n");
    }
}

float overlap_score(char prompt_words[MAX_WORDS][MAX_WORD_LEN], int prompt_count,
                   char corpus_words[MAX_WORDS][MAX_WORD_LEN], int corpus_count) {
    int matches = 0;
    for (int i = 0; i < prompt_count; i++) {
        for (int j = 0; j < corpus_count; j++) {
            if (strcmp(prompt_words[i], corpus_words[j]) == 0) {
                matches++;
                break;
            }
        }
    }
    return (float)matches / prompt_count;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s \"your prompt here\"\n", argv[0]);
        return 1;
    }

    char* subjects[NUM_SUBJECTS] = {"Mathematics", "Physics", "Biology", "History", "Literature",
                                    "Programming", "Chemistry", "Geography", "Economics", "Astronomy"};
    float weights[NUM_SUBJECTS];

    char prompt_words[MAX_WORDS][MAX_WORD_LEN];
    int prompt_count;
    tokenize(argv[1], prompt_words, &prompt_count);

    for (int s = 0; s < NUM_SUBJECTS; s++) {
        char filepath[100];
        snprintf(filepath, 100, "curricula/%s/curriculum.txt", subjects[s]);
        FILE* f = fopen(filepath, "r");
        if (!f) { printf("Error opening %s\n", filepath); return 1; }
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        rewind(f);
        char* corpus = malloc(size + 1);
        if (!corpus) { printf("Malloc failed\n"); exit(1); }
        fread(corpus, 1, size, f);
        corpus[size] = '\0';
        fclose(f);

        char corpus_words[MAX_WORDS][MAX_WORD_LEN];
        int corpus_count;
        tokenize(corpus, corpus_words, &corpus_count);
        weights[s] = overlap_score(prompt_words, prompt_count, corpus_words, corpus_count);
        free(corpus);
    }

    float sum = 0;
    for (int i = 0; i < NUM_SUBJECTS; i++) sum += weights[i];
    if (sum > 0) for (int i = 0; i < NUM_SUBJECTS; i++) weights[i] /= sum;

    FILE* weight_f = fopen("weights.txt", "w");
    if (!weight_f) { printf("Error opening weights.txt\n"); return 1; }
    for (int i = 0; i < NUM_SUBJECTS; i++) fprintf(weight_f, "%.4f ", weights[i]);
    fprintf(weight_f, "\n");
    fclose(weight_f);

    return 0;
}
