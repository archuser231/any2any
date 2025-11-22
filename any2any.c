#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <emmintrin.h>



static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static unsigned char base64_inv[256];

void init_base64_inv() {
    for (int i = 0; i < 256; i++) base64_inv[i] = 255;
    for (int i = 0; i < 64; i++) base64_inv[(unsigned char)base64_table[i]] = i;
    base64_inv['='] = 0;
}

static inline void encode_block(const unsigned char in[3], size_t len, char out[4]) {
    uint32_t triple = (in[0] << 16) | ((len > 1 ? in[1] : 0) << 8) | (len > 2 ? in[2] : 0);

    out[0] = base64_table[(triple >> 18) & 0x3F];
    out[1] = base64_table[(triple >> 12) & 0x3F];
    out[2] = (len > 1) ? base64_table[(triple >> 6) & 0x3F] : '=';
    out[3] = (len > 2) ? base64_table[triple & 0x3F] : '=';
}

void encode_file(const char *infile, const char *outfile, int wrap) {
    FILE *fin = (strcmp(infile, "-") == 0) ? stdin : fopen(infile, "rb");
    if (!fin) { perror("input"); exit(1); }

    FILE *fout = (strcmp(outfile, "-") == 0) ? stdout : fopen(outfile, "w");
    if (!fout) { perror("output"); exit(1); }

    unsigned char buf[3];
    char out[4];
    size_t n;
    int col = 0;

    while ((n = fread(buf, 1, 3, fin)) > 0) {
        encode_block(buf, n, out);
        fwrite(out, 1, 4, fout);
        col += 4;
        if (wrap > 0 && col >= wrap) {
            fputc('\n', fout);  
            col = 0;
        }
    }

    if (wrap > 0 && col > 0) fputc('\n', fout);

    if (fin != stdin) fclose(fin);
    if (fout != stdout) fclose(fout);

    printf("Encoding finished → %s\n", outfile);
}

unsigned char *base64_decode(const char *input, size_t len, size_t *out_len) {
    // Nettoyer whitespace
    char *clean = malloc(len);
    if (!clean) { perror("malloc"); exit(1); }

    size_t L = 0;
    for (size_t i = 0; i < len; i++) {
        if (!isspace((unsigned char)input[i])) clean[L++] = input[i];
    }

    if (L % 4 != 0) {
        free(clean);
        return NULL;
    }

    size_t padding = 0;
    if (L > 0 && clean[L - 1] == '=') padding++;
    if (L > 1 && clean[L - 2] == '=') padding++;

    size_t decoded_len = (L / 4) * 3 - padding;
    unsigned char *out = malloc(decoded_len);
    if (!out) { perror("malloc"); exit(1); }

    size_t i = 0, j = 0;
    while (i < L) {
        unsigned char s1 = base64_inv[(unsigned char)clean[i++]];
        unsigned char s2 = base64_inv[(unsigned char)clean[i++]];
        unsigned char s3 = base64_inv[(unsigned char)clean[i++]];
        unsigned char s4 = base64_inv[(unsigned char)clean[i++]];

        if (s1 == 255 || s2 == 255 || s3 == 255 || s4 == 255) {
            free(clean);
            free(out);
            return NULL;
        }

        uint32_t triple = (s1 << 18) | (s2 << 12) | (s3 << 6) | s4;

        if (j < decoded_len) out[j++] = (triple >> 16) & 0xFF;
        if (j < decoded_len) out[j++] = (triple >> 8) & 0xFF;
        if (j < decoded_len) out[j++] = triple & 0xFF;
    }

    free(clean);
    *out_len = decoded_len;
    return out;
}

void decode_file(const char *infile, const char *outfile) {
    FILE *fin = (strcmp(infile, "-") == 0) ? stdin : fopen(infile, "r");
    if (!fin) { perror("input"); exit(1); }

    fseek(fin, 0, SEEK_END);
    long size = ftell(fin);
    rewind(fin);

    char *buffer = malloc(size + 1);
    if (!buffer) { perror("malloc"); exit(1); }

    fread(buffer, 1, size, fin);
    buffer[size] = '\0';  

    if (fin != stdin) fclose(fin);

    size_t dec_len;
    unsigned char *decoded = base64_decode(buffer, size, &dec_len);
    free(buffer);

    if (!decoded) {
        fprintf(stderr, "Decode failed , invalid caracthers\n");
        exit(1);
    }

    FILE *fout = (strcmp(outfile, "-") == 0) ? stdout : fopen(outfile, "wb");
    if (!fout) { perror("output"); exit(1); }

    fwrite(decoded, 1, dec_len, fout);

    if (fout != stdout) fclose(fout);
    free(decoded);

    printf("Decoding finished → %s\n", outfile);
}


void usage(const char *p) {
    printf("Usage:\n");
    printf("  %s encode <in> <out> [--wrap 76]\n", p);
    printf("  %s decode <in> <out>\n", p);
    printf("  Use '-' for stdin/stdout\n");
}

int main(int argc, char *argv[]) {
    init_base64_inv();

    if (argc < 4) {
        usage(argv[0]);
        return 1;
    }

    int wrap = 0;

    if (argc >= 6 && strcmp(argv[4], "--wrap") == 0) {
        wrap = atoi(argv[5]);
    }

    if (strcmp(argv[1], "encode") == 0) {
        encode_file(argv[2], argv[3], wrap);

    } else if (strcmp(argv[1], "decode") == 0) {
        decode_file(argv[2], argv[3]);

    } else {
        usage(argv[0]);
        return 1;
    }

    return 0;
}

