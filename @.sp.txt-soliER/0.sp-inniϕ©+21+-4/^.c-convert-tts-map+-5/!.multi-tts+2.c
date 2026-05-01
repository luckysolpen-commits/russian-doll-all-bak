// batch_tts.c - Robust version using execvp (no shell, handles & # ^ = etc.)
// FIXED: Output MP3 now goes to the same directory as the input TXT file

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_PATH 2048
#define CONVERTER "./+x/c-convert-tts-2-mp3-d8_04.+x"

int main(void) {
    const char* listfile = "sessions_to_convert.txt";
    FILE* f = fopen(listfile, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open %s\n", listfile);
        fprintf(stderr, "Please create sessions_to_convert.txt with one full path per line.\n");
        return 1;
    }

    printf("=== Multi-TTS Batch Started (direct exec) ===\n");
    printf("Converter : %s\n", CONVERTER);
    printf("List      : %s\n", listfile);
    printf("==================================================\n\n");

    char line[MAX_PATH];
    int total = 0;
    int success_count = 0;

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n\r")] = '\0';
        if (strlen(line) == 0 || line[0] == '#') continue;

        // Trim whitespace
        char* input_file = line;
        while (*input_file == ' ' || *input_file == '\t') input_file++;
        char* end = input_file + strlen(input_file) - 1;
        while (end >= input_file && (*end == ' ' || *end == '\t')) *end-- = '\0';

        if (strlen(input_file) == 0) continue;

        total++;

        // === FIXED: Build output filename in the SAME directory as input ===
        char output_file[MAX_PATH] = {0};

        char* last_slash = strrchr(input_file, '/');
        if (last_slash) {
            // Input has directory path → put output in same directory
            size_t dir_len = last_slash - input_file + 1;  // include the trailing slash
            strncpy(output_file, input_file, dir_len);
            output_file[dir_len] = '\0';

            // Add the basename (without path)
            const char* basename = last_slash + 1;
            strncat(output_file, basename, sizeof(output_file) - strlen(output_file) - 1);
        } else {
            // No directory in input → use current directory
            strncpy(output_file, input_file, sizeof(output_file)-1);
        }

        // Change .txt to .mp3 (or append .mp3 if no .txt extension)
        char* dot = strrchr(output_file, '.');
        if (dot && strcmp(dot, ".txt") == 0) {
            strcpy(dot, ".mp3");
        } else {
            strncat(output_file, ".mp3", sizeof(output_file) - strlen(output_file) - 1);
        }

        printf("[Processing %d] %s → %s\n", total, input_file, output_file);

        // Prepare arguments for execvp (no shell!)
        char* args[4];
        args[0] = CONVERTER;
        args[1] = (char*)input_file;
        args[2] = output_file;
        args[3] = NULL;

        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            execvp(CONVERTER, args);
            // If execvp fails
            perror("execvp failed");
            exit(1);
        } else if (pid > 0) {
            // Parent waits
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                printf("   ✅ Success\n");
                success_count++;
            } else {
                printf("   ❌ Failed (exit code: %d)\n", WEXITSTATUS(status));
            }
        } else {
            perror("fork failed");
        }

        printf("--------------------------------------------------\n");
        sleep(3);   // pause between conversions
    }

    fclose(f);

    printf("\n=== Batch Finished ===\n");
    printf("Total files     : %d\n", total);
    printf("Successful      : %d\n", success_count);
    if (total > 0)
        printf("Success rate    : %.1f%%\n", (success_count * 100.0) / total);

    return 0;
}
