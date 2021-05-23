// disables deprecation warning for fopen
#define _CRT_SECURE_NO_WARNINGS

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aes.h"
#include "io.h"

static inline unsigned get_Nb();
static inline unsigned get_Nr(unsigned Nk);

static char *cipher_hex_interface(unsigned Nb, unsigned Nk, unsigned Nr, word **key, word **in, size_t block_count);
static char *inv_cipher_hex_interface(unsigned Nb, unsigned Nk, unsigned Nr, word **key, word **in, size_t block_count);

static word **hex_string_to_expanded_key(unsigned Nb, unsigned Nr, const char *key, unsigned Nk);
static word **hex_string_to_expanded_inv_key(unsigned Nb, unsigned Nr, const char *key, unsigned Nk);

// ISO/IEC 9797-1, padding method 2
static char *string_padding(unsigned Nb, const char *str);
static int remove_string_padding(char *str);
static int get_block_padding_position(unsigned Nb, byte block[]);

static word *hex_string_to_block(unsigned Nb, const char *str);
static word **hex_string_to_blocks(unsigned Nb, char *str, size_t block_count);
static char *block_to_string(unsigned Nb, word block[]);

char *cipher_hex(unsigned Nk, const char *key, const char *in) {
    unsigned Nb = get_Nb(), Nr = get_Nr(Nk);

    char *in_processed = process_hex_string(in);
    if (strlen(in_processed) != 8 * Nb) {
        free(in_processed);
        error("Incorrect input length.", NULL);
    }
    word **in_blocks = hex_string_to_blocks(Nb, in_processed, 1);
    free(in_processed);

    word **key_processed = hex_string_to_expanded_key(Nb, Nr, key, Nk);

    char *out = cipher_hex_interface(Nb, Nk, Nr, key_processed, in_blocks, 1);

    for (unsigned i = 0; i <= Nr; ++i) free(key_processed[i]);
    free(key_processed);
    free(in_blocks[0]);
    free(in_blocks);

    return out;
}

char *inv_cipher_hex(unsigned Nk, const char *key, const char *in) {
    unsigned Nb = get_Nb(), Nr = get_Nr(Nk);

    char *in_processed = process_hex_string(in);
    if (strlen(in_processed) != 8 * Nb) {
        free(in_processed);
        error("Incorrect input length.", NULL);
    }
    word **in_blocks = hex_string_to_blocks(Nb, in_processed, 1);
    free(in_processed);

    word **key_processed = hex_string_to_expanded_inv_key(Nb, Nr, key, Nk);

    char *out = inv_cipher_hex_interface(Nb, Nk, Nr, key_processed, in_blocks, 1);

    for (unsigned i = 0; i <= Nr; ++i) free(key_processed[i]);
    free(key_processed);
    free(in_blocks[0]);
    free(in_blocks);

    return out;
}

char *cipher_hex_multiblock(unsigned Nk, const char *key, const char *in) {
    unsigned Nb = get_Nb(), Nr = get_Nr(Nk);

    char *in_processed = process_hex_string(in);
    char *in_padded = string_padding(Nb, in_processed);
    free(in_processed);
    const size_t block_count = strlen(in_padded) / (8 * Nb);
    word **in_blocks = hex_string_to_blocks(Nb, in_padded, block_count);
    free(in_padded);

    word **key_processed = hex_string_to_expanded_key(Nb, Nr, key, Nk);

    char *out = cipher_hex_interface(Nb, Nk, Nr, key_processed, in_blocks, block_count);

    for (unsigned i = 0; i <= Nr; ++i) free(key_processed[i]);
    free(key_processed);
    for (size_t i = 0; i < block_count; ++i) free(in_blocks[i]);
    free(in_blocks);

    return out;
}

char *inv_cipher_hex_multiblock(unsigned Nk, const char *key, const char *in) {
    unsigned Nb = get_Nb(), Nr = get_Nr(Nk);

    char *in_processed = process_hex_string(in);
    const size_t n = strlen(in_processed);
    if (n % (8 * Nb)) {
        free(in_processed);
        error("Incorrect input length.", NULL);
    }
    const size_t block_count = n / (8 * Nb);
    word **in_blocks = hex_string_to_blocks(Nb, in_processed, block_count);
    free(in_processed);

    word **key_processed = hex_string_to_expanded_inv_key(Nb, Nr, key, Nk);

    char *out = inv_cipher_hex_interface(Nb, Nk, Nr, key_processed, in_blocks, block_count);

    for (unsigned i = 0; i <= Nr; ++i) free(key_processed[i]);
    free(key_processed);
    for (size_t i = 0; i < block_count; ++i) free(in_blocks[i]);
    free(in_blocks);

    if (remove_string_padding(out)) {
        free(out);
        error("Could not correctly interpret input.", NULL);
    }

    return out;
}

void cipher_file(unsigned Nk, const char *key, const char *in_dir, const char *out_dir) {
    unsigned Nb = get_Nb(), Nr = get_Nr(Nk);

    FILE *in_file, *out_file;
    if (!(in_file = fopen(in_dir, "rb"))) {
        error(": Failed to open input file.", in_dir);
    }
    if (!(out_file = fopen(out_dir, "wb"))) {
        fclose(in_file);
        error(": Failed to open output file.", out_dir);
    }

    word **key_processed = hex_string_to_expanded_key(Nb, Nr, key, Nk);

    {
        byte *in_buffer = (byte *)malloc(4 * Nb * sizeof(byte));
        size_t bytes_read = 0;

        while ((bytes_read = fread(in_buffer, sizeof(byte), 4 * Nb, in_file)) == 4 * Nb) {
            change_endianness(Nb, (word *)in_buffer);
            word *out_buffer = Cipher(Nb, Nr, (word *)in_buffer, key_processed);
            change_endianness(Nb, out_buffer);
            fwrite(out_buffer, sizeof(word), Nb, out_file);
            free(out_buffer);
        }

        {
            in_buffer[bytes_read] = 0x80;
            for (size_t i = bytes_read + 1; i < 4 * Nb; ++i) {
                in_buffer[i] = 0x00;
            }
            change_endianness(Nb, (word *)in_buffer);
            word *out_buffer = Cipher(Nb, Nr, (word *)in_buffer, key_processed);
            change_endianness(Nb, out_buffer);
            fwrite(out_buffer, sizeof(word), Nb, out_file);
            free(out_buffer);
        }

        free(in_buffer);
    }

    for (unsigned i = 0; i <= Nr; ++i) free(key_processed[i]);
    free(key_processed);

    fclose(in_file);
    fclose(out_file);
}

void inv_cipher_file(unsigned Nk, const char *key, const char *in_dir, const char *out_dir) {
    unsigned Nb = get_Nb(), Nr = get_Nr(Nk);

    FILE *in_file, *out_file;
    if (!(in_file = fopen(in_dir, "rb"))) {
        error(": Failed to open input file.", in_dir);
    }
    fseek(in_file, 0, SEEK_END);
    long file_size = ftell(in_file);
    rewind(in_file);
    if (file_size == 0 || file_size % (4 * Nb)) {
        fclose(in_file);
        error(": Incorrect input file. Is it empty or modified?", in_dir);
    }
    if (!(out_file = fopen(out_dir, "wb"))) {
        fclose(in_file);
        error(": Failed to open output file.", out_dir);
    }

    word **key_processed = hex_string_to_expanded_inv_key(Nb, Nr, key, Nk);

    {
        byte *in_buffer = (byte *)malloc(4 * Nb * sizeof(byte));
        size_t bytes_read = 0;

        while ((bytes_read += fread(in_buffer, sizeof(byte), 4 * Nb, in_file)) < file_size) {
            change_endianness(Nb, (word *)in_buffer);
            word *out_buffer = InvCipher(Nb, Nr, (word *)in_buffer, key_processed);
            change_endianness(Nb, out_buffer);
            fwrite(out_buffer, sizeof(word), Nb, out_file);
            free(out_buffer);
        }

        {
            change_endianness(Nb, (word *)in_buffer);
            byte *out_buffer = (byte *)InvCipher(Nb, Nr, (word *)in_buffer, key_processed);
            change_endianness(Nb, (word *)out_buffer);
            int pos = get_block_padding_position(Nb, out_buffer);
            if (pos < 0) {
                free(in_buffer);
                free(out_buffer);
                for (unsigned i = 0; i <= Nr; ++i) free(key_processed[i]);
                free(key_processed);
                fclose(in_file);
                fclose(out_file);
                remove(out_dir);
                error(": Could not correctly interpret input.", in_dir);
            }
            fwrite(out_buffer, sizeof(byte), pos, out_file);
            free(out_buffer);
        }

        free(in_buffer);
    }

    for (unsigned i = 0; i <= Nr; ++i) free(key_processed[i]);
    free(key_processed);

    fclose(in_file);
    fclose(out_file);
}

char *process_hex_string(const char *str) {
    const size_t str_len = strlen(str);
    char *new_str = (char *)malloc((str_len + 1) * sizeof(char));
    size_t n = 0;
    for (size_t i = 0; i < str_len; ++i) {
        if (isspace(str[i])) continue;
        if (!isxdigit(str[i])) {
            free(new_str);
            error("Input contains invalid hexadecimal digit.", NULL);
        }
        new_str[n++] = str[i];
    }
    new_str[n] = '\0';
    return new_str;
}

static inline unsigned get_Nb() {
    return 4;
}

static inline unsigned get_Nr(unsigned Nk) {
    switch (Nk) {
        case 4:
            return 10;
        case 6:
            return 12;
        case 8:
            return 14;
    }
    return 0;
}

static char *cipher_hex_interface(unsigned Nb, unsigned Nk, unsigned Nr, word **key, word **in, size_t block_count) {
    char *out = (char *)malloc((block_count * 8 * Nb + 1) * sizeof(char));
    for (size_t i = 0; i < block_count; ++i) {
        word *out_bytes = Cipher(Nb, Nr, in[i], key);
        char *out_str = block_to_string(Nb, out_bytes);
        free(out_bytes);
        memcpy(out + i * 8 * Nb, out_str, 8 * Nb * sizeof(char));
        free(out_str);
    }
    out[block_count * 8 * Nb] = '\0';
    return out;
}

static char *inv_cipher_hex_interface(unsigned Nb, unsigned Nk, unsigned Nr, word **key, word **in, size_t block_count) {
    char *out = (char *)malloc((block_count * 8 * Nb + 1) * sizeof(char));
    for (size_t i = 0; i < block_count; ++i) {
        word *out_bytes = InvCipher(Nb, Nr, in[i], key);
        char *out_str = block_to_string(Nb, out_bytes);
        free(out_bytes);
        memcpy(out + i * 8 * Nb, out_str, 8 * Nb * sizeof(char));
        free(out_str);
    }
    out[block_count * 8 * Nb] = '\0';
    return out;
}

static word **hex_string_to_expanded_key(unsigned Nb, unsigned Nr, const char *key_str, unsigned Nk) {
    word *key = (word *)malloc(Nk * sizeof(word *));
    for (unsigned i = 0; i < Nk; ++i) {
        char buffer[9];
        memcpy(buffer, key_str + i * 8, 8 * sizeof(char));
        buffer[8] = '\0';
        key[i] = strtoul(buffer, NULL, 16);
    }
    word **key_expanded = KeyExpansion(Nb, Nr, key, Nk);
    free(key);
    return key_expanded;
}

static word **hex_string_to_expanded_inv_key(unsigned Nb, unsigned Nr, const char *key_str, unsigned Nk) {
    word **key_expanded = hex_string_to_expanded_key(Nb, Nr, key_str, Nk);
    for (unsigned round = 1; round < Nr; ++round) {
        for (unsigned j = 0; j < Nb; ++j) {
            const uword w = {key_expanded[round][j]};
            key_expanded[round][j] =
                InvMixColumns_table[0][w.bytes[3]] ^
                InvMixColumns_table[1][w.bytes[2]] ^
                InvMixColumns_table[2][w.bytes[1]] ^
                InvMixColumns_table[3][w.bytes[0]];
        }
    }
    return key_expanded;
}

static char *string_padding(unsigned Nb, const char *str) {
    const size_t n = strlen(str);
    size_t padded_length = ((n / (8 * Nb)) + 1) * (8 * Nb);
    char *new_str = (char *)malloc((padded_length + 1) * sizeof(char));
    memcpy(new_str, str, n * sizeof(char));
    new_str[n] = '8';
    for (size_t i = n + 1; i < padded_length; ++i) new_str[i] = '0';
    new_str[padded_length] = '\0';
    return new_str;
}

static int remove_string_padding(char *str) {
    const size_t str_len = strlen(str);
    for (size_t i = str_len - 1; i != -1; --i) {
        if (str[i] != '0') {
            if (str[i] != '8') return 1;
            str[i] = '\0';
            return 0;
        }
    }
    return 1;
}

static int get_block_padding_position(unsigned Nb, byte block[]) {
    for (signed i = 4 * Nb - 1; i >= 0; --i) {
        if (block[i] != 0x00) {
            if (block[i] != 0x80) return -1;
            return i;
        }
    }
    return -1;
}

static word *hex_string_to_block(unsigned Nb, const char *str) {
    word *block = (word *)malloc(Nb * sizeof(word));
    for (unsigned j = 0; j < Nb; ++j) {
        char buffer[9];
        memcpy(buffer, str + j * 8, 8 * sizeof(char));
        buffer[8] = '\0';
        block[j] = strtoul(buffer, NULL, 16);
    }
    return block;
}

static word **hex_string_to_blocks(unsigned Nb, char *str, size_t block_count) {
    word **blocks = (word **)malloc(block_count * sizeof(word *));
    for (size_t i = 0; i < block_count; ++i) {
        blocks[i] = hex_string_to_block(Nb, str + i * 8 * Nb);
    }
    return blocks;
}

static char *block_to_string(unsigned Nb, word block[]) {
    char *str = (char *)malloc((8 * Nb + 1) * sizeof(char));
    for (unsigned j = 0; j < Nb; ++j) {
        snprintf(str + j * 8, 9, "%08x", block[j]);
    }
    // the terminating null character is handled by the last snprintf() call
    return str;
}
