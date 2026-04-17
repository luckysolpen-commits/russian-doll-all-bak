#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

#define MAX_LINE 1024
#define MAX_WORD_LEN 100

char project_root[4096] = ".";
char secret_word[MAX_WORD_LEN] = "";
char display_word[MAX_WORD_LEN] = "";
int remaining_lives = 6;
char last_guess_result[64] = ""; // Stores "Correct" or "Incorrect"
char correct_answer[MAX_WORD_LEN] = ""; // Stores the correctly revealed word

void trim_str_inplace(char *str) {
    char *end;
    if (!str) return;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
}

void resolve_paths() {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), kvp)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(line);
                char *v = trim_str(eq + 1);
                if (strcmp(k, "project_root") == 0) snprintf(project_root, sizeof(project_root), "%s", v);
            }
        }
        fclose(kvp);
    }
}

int main(int argc, char *argv[]) {
    resolve_paths();

    if (argc < 4) {
        fprintf(stderr, "Usage: hangman_guess <secret_word> <guess_letter> <display_word_state_path>
");
        return 1;
    }

    char *secret_word_arg = argv[1];
    char guess_letter = argv[2][0];
    char *display_word_state_path = argv[3];

    strcpy(secret_word, secret_word_arg);
    
    int found = 0;
    for (int i = 0; i < strlen(secret_word); i++) {
        if (secret_word[i] == tolower(guess_letter)) {
            display_word[i] = tolower(guess_letter);
            found = 1;
        }
    }

    if (found) {
        strcpy(last_guess_result, "Correct");
    } else {
        remaining_lives--;
        strcpy(last_guess_result, "Incorrect");
    }
    strcpy(correct_answer, secret_word);

    FILE *state_f = fopen(display_word_state_path, "w");
    if (state_f) {
        fprintf(state_f, "secret_word=%s
", secret_word);
        fprintf(state_f, "display_word=%s
", display_word);
        fprintf(state_f, "remaining_lives=%d
", remaining_lives);
        fprintf(state_f, "last_guess_result=%s
", last_guess_result);
        fprintf(state_f, "correct_answer=%s
", correct_answer);
        fclose(sf);
    } else {
        fprintf(stderr, "Error: Could not write to state file %s
", state_path);
        return 1;
    }

    printf("%s guess! Lives left: %d
", last_guess_result, remaining_lives);

    return 0;
}
