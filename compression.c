#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>

#define BUFFER_SIZE 1024


unsigned char nucleotide_to_bits(char nucleotide, int is_rna) {
    switch (nucleotide) {
        case 'A': return 0b00; // A -> 00
        case 'C': return 0b01; // C -> 01
        case 'G': return 0b10; // G -> 10
        case 'T': return is_rna ? 0xFF : 0b11; // T -> 11 (DNA only)
        case 'U': return is_rna ? 0b11 : 0xFF; // U -> 11 (RNA only)
        default:
            fprintf(stderr, "Invalid nucleotide found: %c\n", nucleotide);
            exit(EXIT_FAILURE);
    }
}


void write_compressed_sequence(FILE *output_file, const char *sequence, int is_rna, unsigned int length) {
    unsigned char byte = 0;
    int bit_pos = 6;

    byte = is_rna ? 0x01 : 0x00;
    fwrite(&byte, sizeof(byte), 1, output_file);
    fwrite(&length, sizeof(length), 1, output_file);
    printf("%d\n", length);
    byte = 0;

    for (int i = 0; sequence[i] != '\0'; i++) {
        char nucleotide = sequence[i];
        if (nucleotide == '\n') continue;

        unsigned char bits = nucleotide_to_bits(nucleotide, is_rna);
        if (bits == 0xFF) {
            fprintf(stderr, "Error: Mismatched DNA/RNA nucleotide: %c\n", nucleotide);
            exit(EXIT_FAILURE);
        }

        byte |= (bits << bit_pos);
        bit_pos -= 2;

        if (bit_pos < 0) {
            fwrite(&byte, sizeof(byte), 1, output_file);
            byte = 0;
            bit_pos = 6;
        }
    }

    if (bit_pos < 6) {
        fwrite(&byte, sizeof(byte), 1, output_file);
    }
}


unsigned int get_number_of_nucleotides(const char *sequence) {
    unsigned int length = 0;
    for (int i = 0; sequence[i] != '\0'; i++) {
        char nucleotide = sequence[i];
        if (nucleotide == '\n') continue;
        length++;
    }
    return length;
}


void sanitize_filename(char *filename) {
    int i = 0, j = 0;

    while (filename[i] != '\0') {
        if (filename[i] == '.') {
            filename[j++] = '_';
        } else if (isspace(filename[i]) || isalnum(filename[i]) || filename[i] == '_' || filename[i] == '-' || filename[i] == ',') {
            filename[j++] = filename[i];
        }
        i++;
    }

    filename[j] = '\0';
}


void create_directories(const char *fasta_file, char *buffer, size_t buffer_size) {
    mkdir("output_bin");

    char fasta_name[200];
    const char *dot_position = strrchr(fasta_file, '.');
    int length = dot_position - fasta_file;
    strncpy(fasta_name, fasta_file, length);
    fasta_name[length] = '\0';

    const char *foldername = strrchr(fasta_name, '\\');
    if (!foldername) {
        foldername = fasta_name;
    } else {
        foldername++;
    }

    char path[400];
    snprintf(path, sizeof(path), "output_bin/%s", foldername);
    mkdir(path);

    strncpy(buffer, foldername, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <fasta_file> <dna_or_rna>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *fasta_file = argv[1];
    int is_rna = strcmp(argv[2], "rna") == 0 ? 1 : 0;

    char foldername[200];
    create_directories(fasta_file, foldername, sizeof(foldername));

    FILE *input_file = fopen(fasta_file, "r");
    if (!input_file) {
        perror("Error opening FASTA file");
        return EXIT_FAILURE;
    }

    char buffer[BUFFER_SIZE];
    char sequence[BUFFER_SIZE * 1024];
    sequence[0] = '\0';
    FILE *output_file = NULL;

    while (fgets(buffer, sizeof(buffer), input_file)) {
        if (buffer[0] == '>') {
            if (output_file) {
                write_compressed_sequence(output_file, sequence, is_rna, strlen(sequence));
                fclose(output_file);
            }

            char sequence_description[BUFFER_SIZE];
            strncpy(sequence_description, buffer + 1, sizeof(sequence_description) - 1);
            sequence_description[strcspn(sequence_description, "\n")] = '\0';

            sanitize_filename(sequence_description);

            char output_filename[200];
            snprintf(output_filename, sizeof(output_filename), "output_bin/%s/%s.bin", foldername, sequence_description);
            printf("%s\n", output_filename);
            output_file = fopen(output_filename, "wb");
            if (!output_file) {
                perror("Error creating FASTA output file");
                fclose(input_file);
                return EXIT_FAILURE;
            }

            sequence[0] = '\0';
        } else {
            strcat(sequence, buffer);
        }
    }

    if (output_file && strlen(sequence) > 0) {
        unsigned int length = get_number_of_nucleotides(sequence);
        write_compressed_sequence(output_file, sequence, is_rna, length);
        fclose(output_file);
    }

    fclose(input_file);
    return EXIT_SUCCESS;
}
