#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <direct.h>
#include <stdint.h>

#define BUFFER_SIZE 1024


char bits_to_nucleotide(unsigned char bits, int is_rna) {
    switch (bits) {
        case 0b00: return 'A'; // 00 -> A
        case 0b01: return 'C'; // 01 -> C
        case 0b10: return 'G'; // 10 -> G
        case 0b11: return is_rna ? 'U' : 'T'; // 11 -> U (RNA), 11 -> T (DNA)
        default:
            fprintf(stderr, "Invalid nucleotide found: %c\n", bits);
            exit(EXIT_FAILURE);
    }
}


void write_decompressed_sequence(FILE *output_file, FILE *input_file, char *entry_name) {
    fputs(entry_name, output_file);

    unsigned char byte;
    int is_rna;
    uint64_t length;

    fread(&byte, sizeof(byte), 1, input_file);
    if (byte == 0x01) {
        is_rna = 1;
    } else if (byte == 0x00) {
        is_rna = 0;
    } else {
        printf("Is RNA/DNA error\n"); // ovo ce trebat jos idiot-proofat
    }

    fread(&length, sizeof(length), 1, input_file);
    printf("%lu\n", length);

    int line_count = 0;
    uint64_t total_count = 0;
    unsigned char mask = 0b11000000;
    char nucleotide;
    while (fread(&byte, sizeof(byte), 1, input_file) == 1) {
        for(int i = 0; i < 4; i++) {
            line_count++;
            total_count++;
            nucleotide = bits_to_nucleotide((byte & (mask >> (2*i))) >> (6 - 2*i), is_rna);
            fputc(nucleotide, output_file);
            if (line_count == 70) {
                fputc('\n', output_file);
                line_count = 0;
            }
            if (total_count == length) {
                fputs("\n\n", output_file);
                return;
            }
        }
    }

    fputs("\n\n", output_file);
}


void desanitize_filename(char *filename) {
    int i = 0, j = 0;

    while (filename[i] != '\0') {
        if (filename[i] == '_') {
            filename[j++] = '.';
        } else {
            filename[j++] = filename[i];
        }
        i++;
    }

    filename[j] = '\0';
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <path_to_compressed_folder>\n", argv[0]);
        return EXIT_FAILURE;
    }

    mkdir("output_fasta");

    const char *dir_path = argv[1];
    DIR *dir = opendir(dir_path);

    if (dir == NULL) {
        perror("Error opening directory");
        return EXIT_FAILURE;
    }

    const char *sequence_name = strrchr(dir_path, '\\');
    sequence_name++;
    char sequence_name_buffer[200];
    snprintf(sequence_name_buffer, sizeof(sequence_name_buffer), "output_fasta/%s.fasta", sequence_name);
    printf("%s\n", sequence_name_buffer);

    FILE *output_file = fopen(sequence_name_buffer, "w");
    if (!output_file) {
        perror("Error opening output FASTA file\n");
        return EXIT_FAILURE;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        const char *dot = strrchr(entry->d_name, '.');

        char bin_name_buffer[400]; 
        char entry_name[200];

        if (dot != NULL && strcmp(dot, ".bin") == 0) {
            snprintf(entry_name, sizeof(entry_name), ">%s\n", entry->d_name);
            desanitize_filename(entry_name);

            snprintf(bin_name_buffer, sizeof(bin_name_buffer), "%s\\%s", dir_path, entry->d_name);
            FILE *input_file = fopen(bin_name_buffer, "rb");
            if (!input_file) {
                perror("Error opening input BIN file\n");
                fclose(output_file);
                return EXIT_FAILURE;
            }

            write_decompressed_sequence(output_file, input_file, entry_name);

            fclose(input_file);
        }
    }

    closedir(dir);
    fclose(output_file);
    return EXIT_SUCCESS;
}