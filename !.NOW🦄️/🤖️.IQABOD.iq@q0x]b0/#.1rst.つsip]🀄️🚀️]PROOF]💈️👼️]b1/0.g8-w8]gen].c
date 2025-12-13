#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_SUBJECTS 10
#define EMBED_DIM 16

int main() {
    srand(time(NULL)); // Seed random number generator

    FILE* f = fopen("gating_weights.txt", "w");
    if (!f) { printf("Error opening gating_weights.txt\n"); return 1; }

    for (int s = 0; s < NUM_SUBJECTS; s++) {
        for (int d = 0; d < EMBED_DIM; d++) {
            float value = ((float)rand() / RAND_MAX - 0.5f) * 0.1f; // Range: -0.05 to 0.05
            fprintf(f, "%.4f ", value);
        }
        fprintf(f, "\n");
    }

    fclose(f);
    printf("Generated gating_weights.txt with %d rows and %d columns\n", NUM_SUBJECTS, EMBED_DIM);
    return 0;
}
