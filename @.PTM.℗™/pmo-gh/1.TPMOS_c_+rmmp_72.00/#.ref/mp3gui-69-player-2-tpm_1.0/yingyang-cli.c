#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <complex.h>
#include <math.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <pulse/simple.h>
#include <pulse/error.h>

#define MAX_PATH_LEN 4096
#define MAX_PLAYLIST_SIZE 10000
#define COMMAND_FILE "player_command.txt"
#define STATUS_FILE "player_status.txt"
#define VIZ_DATA_FILE "player_viz.dat"
#define POLL_INTERVAL_MS 100
#define FFT_SIZE 1024
#define PI 3.14159265358979323846

typedef enum { STOPPED, PLAYING, PAUSED } PlayerState;
typedef enum { SHUFFLE_OFF, SHUFFLE_ON } ShuffleState;

typedef struct {
    AVFormatContext *fmt_ctx;
    AVCodecContext *audio_codec_ctx;
    struct SwrContext *swr_ctx;
    pa_simple *pa_stream;
    int audio_stream_idx;
    PlayerState state;
    char current_filepath[MAX_PATH_LEN];
    long long current_playback_pts;
    long long duration_pts;
    int current_song_index;
    int seek_requested;
    long long seek_target_pts;
    int eof_reached;
    ShuffleState shuffle_state;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_t audio_thread_id;
    int audio_thread_running;
    float fft_data[FFT_SIZE / 2];
} PlayerContext;

PlayerContext player_ctx;
char playlist[MAX_PLAYLIST_SIZE][MAX_PATH_LEN];
int playlist_size = 0;

void fft_radix2(float* x, double complex* X, unsigned int N, unsigned int s) {
    if (N == 1) { X[0] = x[0]; return; }
    fft_radix2(x, X, N/2, 2*s);
    fft_radix2(x+s, X + N/2, N/2, 2*s);
    for (unsigned int k = 0; k < N/2; k++) {
        double complex t = X[k];
        double complex e = cexp(-2 * PI * I * k / N) * X[k + N/2];
        X[k] = t + e;
        X[k + N/2] = t - e;
    }
}

void write_status(void) {
    FILE *f = fopen(STATUS_FILE, "w");
    if (f) {
        fprintf(f, "STATE: %s\n", player_ctx.state == PLAYING ? "PLAYING" : player_ctx.state == PAUSED ? "PAUSED" : "STOPPED");
        fprintf(f, "SHUFFLE: %s\n", player_ctx.shuffle_state == SHUFFLE_ON ? "ON" : "OFF");
        fprintf(f, "CURRENT_SONG_INDEX: %d\n", player_ctx.current_song_index);
        fprintf(f, "CURRENT_SONG_PATH: %s\n", player_ctx.current_filepath);
        if (player_ctx.fmt_ctx && player_ctx.audio_stream_idx != -1) {
            double ts = av_q2d(player_ctx.fmt_ctx->streams[player_ctx.audio_stream_idx]->time_base);
            fprintf(f, "PLAYBACK_TIME: %.2f/%.2f\n", (double)player_ctx.current_playback_pts * ts, (double)player_ctx.duration_pts * ts);
        } else fprintf(f, "PLAYBACK_TIME: N/A\n");
        fclose(f);
    }
}

void write_viz_data(void) {
    FILE *f = fopen(VIZ_DATA_FILE, "wb");
    if (f) { fwrite(player_ctx.fft_data, sizeof(float), FFT_SIZE / 2, f); fclose(f); }
}

void read_playlist(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return;
    playlist_size = 0; char buffer[MAX_PATH_LEN];
    while (fgets(buffer, MAX_PATH_LEN, f) && playlist_size < MAX_PLAYLIST_SIZE) {
        buffer[strcspn(buffer, "\n")] = 0;
        if (strlen(buffer) > 0) strcpy(playlist[playlist_size++], buffer);
    }
    fclose(f);
}

int init_audio(const char *filepath) {
    printf("CLI: Initializing audio for %s\n", filepath);
    if (avformat_open_input(&player_ctx.fmt_ctx, filepath, NULL, NULL) < 0) { printf("CLI ERROR: Could not open file\n"); return -1; }
    if (avformat_find_stream_info(player_ctx.fmt_ctx, NULL) < 0) return -1;
    player_ctx.audio_stream_idx = -1;
    for (int i = 0; i < player_ctx.fmt_ctx->nb_streams; i++)
        if (player_ctx.fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            player_ctx.audio_stream_idx = i; break;
        }
    if (player_ctx.audio_stream_idx == -1) return -1;
    AVCodec *codec = avcodec_find_decoder(player_ctx.fmt_ctx->streams[player_ctx.audio_stream_idx]->codecpar->codec_id);
    player_ctx.audio_codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(player_ctx.audio_codec_ctx, player_ctx.fmt_ctx->streams[player_ctx.audio_stream_idx]->codecpar);
    if (avcodec_open2(player_ctx.audio_codec_ctx, codec, NULL) < 0) return -1;

    player_ctx.swr_ctx = swr_alloc();
    av_opt_set_int(player_ctx.swr_ctx, "in_channel_layout",  player_ctx.audio_codec_ctx->channel_layout, 0);
    av_opt_set_int(player_ctx.swr_ctx, "in_sample_rate",     player_ctx.audio_codec_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(player_ctx.swr_ctx, "in_sample_fmt", player_ctx.audio_codec_ctx->sample_fmt, 0);
    av_opt_set_int(player_ctx.swr_ctx, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
    av_opt_set_int(player_ctx.swr_ctx, "out_sample_rate",    player_ctx.audio_codec_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(player_ctx.swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    swr_init(player_ctx.swr_ctx);

    int pa_err;
    pa_sample_spec ss = { .format = PA_SAMPLE_S16LE, .rate = player_ctx.audio_codec_ctx->sample_rate, .channels = 2 };
    player_ctx.pa_stream = pa_simple_new(NULL, "yingyang", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &pa_err);
    if (!player_ctx.pa_stream) { printf("CLI ERROR: pa_simple_new failed: %s\n", pa_strerror(pa_err)); return -1; }

    player_ctx.duration_pts = player_ctx.fmt_ctx->streams[player_ctx.audio_stream_idx]->duration;
    strcpy(player_ctx.current_filepath, filepath);
    printf("CLI: Audio initialized successfully\n");
    return 0;
}

void cleanup_audio(void) {
    if (player_ctx.pa_stream) { pa_simple_free(player_ctx.pa_stream); player_ctx.pa_stream = NULL; }
    if (player_ctx.swr_ctx) { swr_free(&player_ctx.swr_ctx); player_ctx.swr_ctx = NULL; }
    if (player_ctx.audio_codec_ctx) { avcodec_free_context(&player_ctx.audio_codec_ctx); player_ctx.audio_codec_ctx = NULL; }
    if (player_ctx.fmt_ctx) { avformat_close_input(&player_ctx.fmt_ctx); player_ctx.fmt_ctx = NULL; }
}

void *audio_thread(void *arg) {
    AVPacket pkt; AVFrame *frame = av_frame_alloc();
    float fft_input[FFT_SIZE]; int fft_ptr = 0;
    printf("CLI: Audio thread started\n");
    while (1) {
        pthread_mutex_lock(&player_ctx.mutex);
        while (player_ctx.state == PAUSED) pthread_cond_wait(&player_ctx.cond, &player_ctx.mutex);
        if (player_ctx.state == STOPPED) { pthread_mutex_unlock(&player_ctx.mutex); break; }
        if (player_ctx.seek_requested) {
            av_seek_frame(player_ctx.fmt_ctx, player_ctx.audio_stream_idx, player_ctx.seek_target_pts, AVSEEK_FLAG_ANY);
            avcodec_flush_buffers(player_ctx.audio_codec_ctx);
            player_ctx.current_playback_pts = player_ctx.seek_target_pts;
            player_ctx.seek_requested = 0;
        }
        pthread_mutex_unlock(&player_ctx.mutex);

        if (av_read_frame(player_ctx.fmt_ctx, &pkt) < 0) {
            pthread_mutex_lock(&player_ctx.mutex);
            player_ctx.state = STOPPED; player_ctx.eof_reached = 1;
            pthread_mutex_unlock(&player_ctx.mutex);
            break;
        }

        if (pkt.stream_index == player_ctx.audio_stream_idx) {
            if (avcodec_send_packet(player_ctx.audio_codec_ctx, &pkt) == 0) {
                while (avcodec_receive_frame(player_ctx.audio_codec_ctx, frame) == 0) {
                    int out_samples = swr_get_out_samples(player_ctx.swr_ctx, frame->nb_samples);
                    uint8_t *out_buf; av_samples_alloc(&out_buf, NULL, 2, out_samples, AV_SAMPLE_FMT_S16, 0);
                    swr_convert(player_ctx.swr_ctx, &out_buf, out_samples, (const uint8_t**)frame->data, frame->nb_samples);
                    pa_simple_write(player_ctx.pa_stream, out_buf, out_samples * 4, NULL);

                    int16_t *s16 = (int16_t*)out_buf;
                    for(int i=0; i<out_samples; i++) {
                        fft_input[fft_ptr++] = (float)s16[i*2] / 32768.0f;
                        if (fft_ptr == FFT_SIZE) {
                            double complex X[FFT_SIZE];
                            fft_radix2(fft_input, X, FFT_SIZE, 1);
                            for(int k=0; k<FFT_SIZE/2; k++) player_ctx.fft_data[k] = cabs(X[k]);
                            write_viz_data();
                            fft_ptr = 0;
                        }
                    }
                    av_freep(&out_buf);
                    pthread_mutex_lock(&player_ctx.mutex);
                    player_ctx.current_playback_pts = frame->pts;
                    pthread_mutex_unlock(&player_ctx.mutex);
                }
            }
        }
        av_packet_unref(&pkt);
    }
    av_frame_free(&frame);
    printf("CLI: Audio thread exiting\n");
    return NULL;
}

void process_command(const char *cmd) {
    printf("CLI: Processing command: %s\n", cmd);
    if (strncmp(cmd, "PLAY ", 5) == 0) {
        int idx = atoi(cmd + 5);
        if (idx < 0 || idx >= playlist_size) return;
        if (player_ctx.audio_thread_running) {
            player_ctx.state = STOPPED; pthread_cond_signal(&player_ctx.cond);
            pthread_join(player_ctx.audio_thread_id, NULL);
            cleanup_audio(); player_ctx.audio_thread_running = 0;
        }
        player_ctx.current_song_index = idx;
        if (init_audio(playlist[idx]) == 0) {
            player_ctx.state = PLAYING; player_ctx.eof_reached = 0;
            pthread_create(&player_ctx.audio_thread_id, NULL, audio_thread, NULL);
            player_ctx.audio_thread_running = 1;
        }
    } else if (strcmp(cmd, "PAUSE") == 0) {
        pthread_mutex_lock(&player_ctx.mutex);
        player_ctx.state = (player_ctx.state == PLAYING) ? PAUSED : PLAYING;
        pthread_cond_signal(&player_ctx.cond);
        pthread_mutex_unlock(&player_ctx.mutex);
    } else if (strcmp(cmd, "STOP") == 0) {
        player_ctx.state = STOPPED;
    } else if (strcmp(cmd, "NEXT") == 0) {
        int next = (player_ctx.shuffle_state == SHUFFLE_ON) ? rand() % playlist_size : (player_ctx.current_song_index + 1) % playlist_size;
        char buf[64]; sprintf(buf, "PLAY %d", next); process_command(buf);
    } else if (strcmp(cmd, "PREV") == 0) {
        int prev = (player_ctx.current_song_index - 1 + playlist_size) % playlist_size;
        char buf[64]; sprintf(buf, "PLAY %d", prev); process_command(buf);
    } else if (strcmp(cmd, "SHUFFLE_ON") == 0) player_ctx.shuffle_state = SHUFFLE_ON;
    else if (strcmp(cmd, "SHUFFLE_OFF") == 0) player_ctx.shuffle_state = SHUFFLE_OFF;
    write_status();
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    read_playlist("playlist.txt");
    if (playlist_size == 0) { printf("CLI ERROR: playlist.txt is empty\n"); return 1; }
    pthread_mutex_init(&player_ctx.mutex, NULL);
    pthread_cond_init(&player_ctx.cond, NULL);
    
    struct stat last_st;
    if (stat(COMMAND_FILE, &last_st) == -1) {
        FILE *f = fopen(COMMAND_FILE, "w"); if (f) fclose(f);
        stat(COMMAND_FILE, &last_st);
    }

    printf("CLI: Ready and monitoring %s\n", COMMAND_FILE);
    while (1) {
        struct stat st;
        if (stat(COMMAND_FILE, &st) == 0 && st.st_mtime > last_st.st_mtime) {
            FILE *f = fopen(COMMAND_FILE, "r");
            if (f) {
                char buf[MAX_PATH_LEN];
                if (fgets(buf, MAX_PATH_LEN, f)) { buf[strcspn(buf, "\n")] = 0; process_command(buf); }
                fclose(f);
            }
            last_st = st;
        }
        if (player_ctx.state == STOPPED && player_ctx.eof_reached) process_command("NEXT");
        usleep(POLL_INTERVAL_MS * 1000);
    }
    return 0;
}
