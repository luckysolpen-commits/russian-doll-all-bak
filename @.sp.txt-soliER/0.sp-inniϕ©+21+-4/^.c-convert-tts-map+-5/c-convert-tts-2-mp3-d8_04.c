// Filename: convert_session.c
#define _GNU_SOURCE   // Required for asprintf()

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_LINE_LENGTH 2048
#define MAX_SEGMENTS 2000
#define MAX_VOICE_ENTRIES 100
#define MAX_TEMP_FILES 2000
#define MAX_RETRIES 5          // Max retries per segment
#define BASE_BACKOFF 2         // Initial backoff in seconds

#define VOICE_MAP_FILE "voice_map.txt"

// Voice mapping with rate and pitch
typedef struct {
    char speaker_name[MAX_LINE_LENGTH];
    char voice_id[128];
    char rate[32];
    char pitch[32];
} VoiceMapEntry;

VoiceMapEntry EDGE_VOICE_MAP[MAX_VOICE_ENTRIES];
int NUM_VOICES = 0;

typedef struct {
    char speaker[MAX_LINE_LENGTH];
    char text[MAX_LINE_LENGTH * 20];
} Segment;

Segment segments[MAX_SEGMENTS];
int segment_count = 0;

// Global overrides
char global_rate[32] = "";
char global_pitch[32] = "";

// Unique temp directory
char TEMP_DIR[512] = "";

int keep_temp = 0;   // 0 = cleanup (default), 1 = keep

// --- Voice Map ---
void load_voice_map(const char* map_filename) {
    FILE* f = fopen(map_filename, "r");
    if (!f) {
        fprintf(stderr, "Warning: Cannot open voice map '%s'. Using default.\n", map_filename);
        strcpy(EDGE_VOICE_MAP[0].speaker_name, "default");
        strcpy(EDGE_VOICE_MAP[0].voice_id, "zh-CN-XiaoxiaoNeural");
        EDGE_VOICE_MAP[0].rate[0] = '\0';
        EDGE_VOICE_MAP[0].pitch[0] = '\0';
        NUM_VOICES = 1;
        return;
    }

    char line[1024];
    NUM_VOICES = 0;

    while (fgets(line, sizeof(line), f) && NUM_VOICES < MAX_VOICE_ENTRIES) {
        line[strcspn(line, "\n\r")] = 0;
        if (line[0] == '#' || strlen(line) < 3) continue;

        char* token = strtok(line, "|");
        if (!token) continue;
        strncpy(EDGE_VOICE_MAP[NUM_VOICES].speaker_name, token, sizeof(EDGE_VOICE_MAP[0].speaker_name)-1);

        token = strtok(NULL, "|");
        if (!token) continue;
        strncpy(EDGE_VOICE_MAP[NUM_VOICES].voice_id, token, sizeof(EDGE_VOICE_MAP[0].voice_id)-1);

        token = strtok(NULL, "|");
        if (token) strncpy(EDGE_VOICE_MAP[NUM_VOICES].rate, token, sizeof(EDGE_VOICE_MAP[0].rate)-1);
        else EDGE_VOICE_MAP[NUM_VOICES].rate[0] = '\0';

        token = strtok(NULL, "|");
        if (token) strncpy(EDGE_VOICE_MAP[NUM_VOICES].pitch, token, sizeof(EDGE_VOICE_MAP[0].pitch)-1);
        else EDGE_VOICE_MAP[NUM_VOICES].pitch[0] = '\0';

        char* fields[4] = {EDGE_VOICE_MAP[NUM_VOICES].speaker_name,
                           EDGE_VOICE_MAP[NUM_VOICES].voice_id,
                           EDGE_VOICE_MAP[NUM_VOICES].rate,
                           EDGE_VOICE_MAP[NUM_VOICES].pitch};
        for (int i = 0; i < 4; i++) {
            char* s = fields[i];
            while (*s == ' ' || *s == '\t') s++;
            memmove(fields[i], s, strlen(s) + 1);
            char* e = fields[i] + strlen(fields[i]) - 1;
            while (e >= fields[i] && (*e == ' ' || *e == '\t')) *e-- = '\0';
        }
        NUM_VOICES++;
    }
    fclose(f);

    if (NUM_VOICES == 0) {
        strcpy(EDGE_VOICE_MAP[0].speaker_name, "default");
        strcpy(EDGE_VOICE_MAP[0].voice_id, "zh-CN-XiaoxiaoNeural");
        NUM_VOICES = 1;
    }

    printf("Loaded %d voice mappings from '%s'\n", NUM_VOICES, map_filename);
}

void get_voice_settings(const char* speaker_name, const char** voice_id_out,
                        char* rate_out, size_t rsize, char* pitch_out, size_t psize) {
    *voice_id_out = "zh-CN-XiaoxiaoNeural";
    rate_out[0] = pitch_out[0] = '\0';

    for (int i = 0; i < NUM_VOICES; i++) {
        if (strcmp(speaker_name, EDGE_VOICE_MAP[i].speaker_name) == 0) {
            *voice_id_out = EDGE_VOICE_MAP[i].voice_id;
            if (EDGE_VOICE_MAP[i].rate[0])  strncpy(rate_out, EDGE_VOICE_MAP[i].rate, rsize-1);
            else if (global_rate[0])        strncpy(rate_out, global_rate, rsize-1);
            if (EDGE_VOICE_MAP[i].pitch[0]) strncpy(pitch_out, EDGE_VOICE_MAP[i].pitch, psize-1);
            else if (global_pitch[0])       strncpy(pitch_out, global_pitch, psize-1);
            return;
        }
    }
    if (global_rate[0])  strncpy(rate_out, global_rate, rsize-1);
    if (global_pitch[0]) strncpy(pitch_out, global_pitch, psize-1);
}

// --- Helpers ---
void generate_temp_filename(char* buf, int size, const char* prefix, const char* ext) {
    static int counter = 0;
    snprintf(buf, size, "%s/%s_%03d_%d.%s", TEMP_DIR, prefix, counter++, rand() % 10000, ext);
}

long get_file_size(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fclose(f);
    return sz;
}

void clean_text(char* text) {
    char* s = text; char* d = text;
    while (*s) {
        if (*s == '*' && *(s+1) == '*') { s += 2; continue; }
        if (*s == '`' || *s == '_') { s++; continue; }
        *d++ = *s++;
    }
    *d = '\0';
}

void create_unique_temp_dir() {
    char timestamp[32];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", t);

    snprintf(TEMP_DIR, sizeof(TEMP_DIR), "./edge_tts_c_output_%s_%04d", 
             timestamp, rand() % 10000);

    if (mkdir(TEMP_DIR, 0777) != 0) {
        perror("Failed to create temp directory");
        exit(1);
    }
    printf("Using temp directory: %s\n", TEMP_DIR);
}

void cleanup_temp_resources() {
    if (keep_temp) {
        printf("Keeping temporary directory: %s\n", TEMP_DIR);
        return;
    }

    DIR *dir = opendir(TEMP_DIR);
    if (!dir) return;

    struct dirent *entry;
    char path[1024];
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        snprintf(path, sizeof(path), "%s/%s", TEMP_DIR, entry->d_name);
        remove(path);
    }
    closedir(dir);
    rmdir(TEMP_DIR);
    printf("Cleaned up temporary directory.\n");
}

int write_text_to_temp_file(const char* text, char* temp_text_path, size_t size) {
    generate_temp_filename(temp_text_path, size, "text", "txt");
    FILE* f = fopen(temp_text_path, "w");
    if (!f) {
        perror("Failed to create temp text file");
        return 0;
    }
    fputs(text, f);
    fclose(f);
    return 1;
}

int execute_edge_tts_command(const char* text, const char* speaker_name, const char* output_filename) {
    if (!text || strlen(text) < 3) {
        printf("  [SKIP] Empty/short segment for '%s'\n", speaker_name);
        return 0;
    }

    const char* voice_id;
    char rate[32] = "", pitch[32] = "";
    get_voice_settings(speaker_name, &voice_id, rate, sizeof(rate), pitch, sizeof(pitch));

    char temp_text_path[512];
    if (!write_text_to_temp_file(text, temp_text_path, sizeof(temp_text_path))) return 0;

    int success = 0;
    for (int attempt = 1; attempt <= MAX_RETRIES; attempt++) {
        char* command = NULL;
        int ret;

        if (rate[0] && pitch[0])
            ret = asprintf(&command, "edge-tts --voice '%s' --rate=%s --pitch=%s --file \"%s\" --write-media \"%s\"",
                           voice_id, rate, pitch, temp_text_path, output_filename);
        else if (rate[0])
            ret = asprintf(&command, "edge-tts --voice '%s' --rate=%s --file \"%s\" --write-media \"%s\"",
                           voice_id, rate, temp_text_path, output_filename);
        else if (pitch[0])
            ret = asprintf(&command, "edge-tts --voice '%s' --pitch=%s --file \"%s\" --write-media \"%s\"",
                           voice_id, pitch, temp_text_path, output_filename);
        else
            ret = asprintf(&command, "edge-tts --voice '%s' --file \"%s\" --write-media \"%s\"",
                           voice_id, temp_text_path, output_filename);

        if (ret == -1 || !command) {
            remove(temp_text_path);
            return 0;
        }

        printf("  → %s", speaker_name);
        if (rate[0])  printf(" [rate:%s]", rate);
        if (pitch[0]) printf(" [pitch:%s]", pitch);
        if (attempt > 1) printf(" (retry %d/%d)", attempt, MAX_RETRIES);
        printf("\n");

        int system_ret = system(command);
        free(command);

        if (system_ret == 0 && get_file_size(output_filename) > 100) {
            success = 1;
            remove(temp_text_path);
            break;
        }

        // Cleanup failed file
        if (access(output_filename, F_OK) != -1) remove(output_filename);

        if (attempt < MAX_RETRIES) {
            int backoff = BASE_BACKOFF * (1 << (attempt - 1));  // 2, 4, 8, 16...
            printf("  [FAIL] Retrying in %d seconds...\n", backoff);
            sleep(backoff);
        }
    }

    if (!success) {
        fprintf(stderr, "  [FAILED] '%s' after %d attempts\n", speaker_name, MAX_RETRIES);
        remove(temp_text_path);
        return 0;
    }
    return 1;
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

int concatenate_audio_files(const char* final_output) {
    char list_path[1024];
    snprintf(list_path, sizeof(list_path), "%s/filelist.txt", TEMP_DIR);

    char* files[MAX_TEMP_FILES];
    int count = 0;

    DIR *dir = opendir(TEMP_DIR);
    if (!dir) return 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && count < MAX_TEMP_FILES) {
        if (strstr(entry->d_name, "segment_") && strstr(entry->d_name, ".mp3")) {
            files[count] = strdup(entry->d_name);
            count++;
        }
    }
    closedir(dir);

    if (count == 0) {
        fprintf(stderr, "No segments to concatenate.\n");
        return 0;
    }

    // Sort by segment number
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            int num_i = 0, num_j = 0;
            sscanf(files[i], "segment_%d", &num_i);
            sscanf(files[j], "segment_%d", &num_j);
            if (num_i > num_j) {
                char* temp = files[i];
                files[i] = files[j];
                files[j] = temp;
            }
        }
    }

    FILE *list = fopen(list_path, "w");
    if (!list) {
        perror("Cannot create filelist.txt");
        for (int i = 0; i < count; i++) free(files[i]);
        return 0;
    }

    for (int i = 0; i < count; i++) {
        fprintf(list, "file '%s'\n", files[i]);
        free(files[i]);
    }
    fclose(list);

    printf("\nConcatenating %d audio segments with ffmpeg...\n", count);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "ffmpeg -f concat -safe 0 -i \"%s\" -c copy \"%s\"", list_path, final_output);

    int ret = system(cmd);
    remove(list_path);
    return (ret == 0);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.txt> [output.mp3] [--rate=XX%%] [--pitch=XXHz] [--keep-temp]\n", argv[0]);
        return 1;
    }

    const char* input = NULL;
    char output[1024] = "";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--keep-temp") == 0) {
            keep_temp = 1;
        } else if (strncmp(argv[i], "--rate=", 7) == 0) {
            strncpy(global_rate, argv[i] + 7, sizeof(global_rate)-1);
        } else if (strncmp(argv[i], "--pitch=", 8) == 0) {
            strncpy(global_pitch, argv[i] + 8, sizeof(global_pitch)-1);
        } else if (!input) {
            input = argv[i];
        } else if (output[0] == '\0') {
            strncpy(output, argv[i], sizeof(output)-1);
        }
    }

    if (!input) {
        fprintf(stderr, "Error: No input file specified.\n");
        return 1;
    }

    if (output[0] == '\0') {
        strncpy(output, input, sizeof(output)-1);
        char* dot = strrchr(output, '.');
        if (dot) *dot = '\0';
        strncat(output, ".mp3", sizeof(output) - strlen(output) - 1);
    }

    srand((unsigned int)time(NULL));

    load_voice_map(VOICE_MAP_FILE);
    if (global_rate[0])  printf("Global rate override: %s\n", global_rate);
    if (global_pitch[0]) printf("Global pitch override: %s\n", global_pitch);

    create_unique_temp_dir();

    parse_session_file(input);
    printf("Found %d valid segments in '%s'.\n", segment_count, input);

    if (segment_count == 0) {
        fprintf(stderr, "No segments found.\n");
        cleanup_temp_resources();
        return 1;
    }

    int generated = 0;
    for (int i = 0; i < segment_count; i++) {
        char tmpfile[512];
        generate_temp_filename(tmpfile, sizeof(tmpfile), "segment", "mp3");

        if (execute_edge_tts_command(segments[i].text, segments[i].speaker, tmpfile))
            generated++;

        usleep(400000);  // 400ms delay to help avoid rate limits
    }

    int success = concatenate_audio_files(output);

    cleanup_temp_resources();

    if (success) {
        printf("\n✅ Conversion complete!\n");
        printf("   Final output: %s\n", output);
        printf("   Segments processed: %d / %d\n", generated, segment_count);
        if (keep_temp) printf("   Temporary files kept in: %s\n", TEMP_DIR);
        return 0;
    } else {
        printf("\n⚠️  Conversion finished but concatenation failed.\n");
        return 1;
    }
}
