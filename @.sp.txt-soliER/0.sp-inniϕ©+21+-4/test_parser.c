#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 2048
#define MAX_SEGMENTS 2000

typedef struct {
    char speaker[MAX_LINE_LENGTH];
    char text[MAX_LINE_LENGTH * 20];
} Segment;

Segment segments[MAX_SEGMENTS];
int segment_count = 0;

void clean_text(char* text) {
    char* s = text; char* d = text;
    while (*s) {
        if (*s == '*' && *(s+1) == '*') { s += 2; continue; }
        if (*s == '`' || *s == '_') { s++; continue; }
        *d++ = *s++;
    }
    *d = '\0';
}

void parse_session_file(const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (!f) { perror("Cannot open input file"); return; }

    char line[MAX_LINE_LENGTH];
    char curr_speaker[MAX_LINE_LENGTH] = "";
    char curr_text[MAX_LINE_LENGTH * 20] = "";
    int text_len = 0;

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;

        char* trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        char* end = trimmed + strlen(trimmed) - 1;
        while (end >= trimmed && (*end == ' ' || *end == '\t')) end--;
        *(end + 1) = '\0';

        if (strlen(trimmed) == 0) {
            if (text_len > 0 && strlen(curr_speaker) > 0) {
                clean_text(curr_text);
                if (strlen(curr_text) > 5 && segment_count < MAX_SEGMENTS) {
                    strcpy(segments[segment_count].speaker, curr_speaker);
                    strcpy(segments[segment_count].text, curr_text);
                    segment_count++;
                }
            }
            curr_speaker[0] = '\0';
            text_len = 0;
            continue;
        }

        if (strncmp(trimmed, "#", 1) == 0 || strncmp(trimmed, "---", 3) == 0) continue;

        if (trimmed[0] == '*' && trimmed[1] == '*' && strstr(trimmed, ":")) {
            if (text_len > 0 && strlen(curr_speaker) > 0) {
                clean_text(curr_text);
                if (strlen(curr_text) > 5 && segment_count < MAX_SEGMENTS) {
                    strcpy(segments[segment_count].speaker, curr_speaker);
                    strcpy(segments[segment_count].text, curr_text);
                    segment_count++;
                }
            }
            char* colon = strstr(trimmed, ":");
            if (colon) {
                int namelen = colon - (trimmed + 2);
                strncpy(curr_speaker, trimmed + 2, namelen);
                curr_speaker[namelen] = '\0';

                char* txt = colon + 1;
                while (*txt == ' ' || *txt == '\t') txt++;
                strncpy(curr_text, txt, sizeof(curr_text)-1);
                text_len = strlen(curr_text);
            }
            continue;
        }

        if (strlen(curr_speaker) > 0) {
            if (text_len > 0 && text_len + 1 < sizeof(curr_text))
                curr_text[text_len++] = ' ';
            if (text_len + strlen(trimmed) < sizeof(curr_text)) {
                strcat(curr_text, trimmed);
                text_len += strlen(trimmed);
            }
        }
    }

    if (text_len > 0 && strlen(curr_speaker) > 0) {
        clean_text(curr_text);
        if (strlen(curr_text) > 5 && segment_count < MAX_SEGMENTS) {
            strcpy(segments[segment_count].speaker, curr_speaker);
            strcpy(segments[segment_count].text, curr_text);
            segment_count++;
        }
    }
    fclose(f);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    parse_session_file(argv[1]);

    printf("File: %s\n", argv[1]);
    printf("Segment Count: %d\n", segment_count);
    for (int i = 0; i < segment_count; i++) {
        printf("[%d] Speaker: %s\n", i, segments[i].speaker);
        printf("    Text: %s\n", segments[i].text);
    }

    return 0;
}
