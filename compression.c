#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>
#include <stdint.h>

#define BUFFER_SIZE 1024
uint64_t length = 0ULL;


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


void write_compressed_sequence(FILE *output_file, const char *sequence, int is_rna) {
    unsigned char byte = 0;
    int bit_pos = 6;

    for (int i = 0; sequence[i] != '\0'; i++) {
        char nucleotide = sequence[i];
        if (nucleotide == '\n') continue;

        unsigned char bits = nucleotide_to_bits(nucleotide, is_rna);
        if (bits == 0xFF) {
            fprintf(stderr, "Error: Mismatched DNA/RNA nucleotide: %c\n", nucleotide); // ovo isto idiot proofat
            exit(EXIT_FAILURE);
        }

        byte |= (bits << bit_pos);
        bit_pos -= 2;

        if (bit_pos < 0) {
            fwrite(&byte, sizeof(byte), 1, output_file);
            byte = 0;
            bit_pos = 6;
        }

        length++;
    }

    if (bit_pos < 6) {
        fwrite(&byte, sizeof(byte), 1, output_file);
    }
}


void update_sequence_size(FILE *output_file) {
    fseek(output_file, 1, SEEK_SET);
    fwrite(&length, sizeof(length), 1, output_file);
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
    unsigned char is_rna = strcmp(argv[2], "rna") == 0 ? 0x01 : 0x00;

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

    unsigned int sequence_length = 0;

    FILE *output_file = NULL;

    while (fgets(buffer, sizeof(buffer), input_file)) {
        if (buffer[0] == '>') {
            if (output_file) {
                write_compressed_sequence(output_file, sequence, is_rna);
                update_sequence_size(output_file);
                fclose(output_file);
            }

            char sequence_description[BUFFER_SIZE];
            strncpy(sequence_description, buffer + 1, sizeof(sequence_description) - 1);
            sequence_description[strcspn(sequence_description, "\n")] = '\0';

            sanitize_filename(sequence_description);

            char output_filename[200];
            snprintf(output_filename, sizeof(output_filename), "output_bin/%s/%s.bin", foldername, sequence_description);

            output_file = fopen(output_filename, "wb");
            if (!output_file) {
                perror("Error creating FASTA output file");
                fclose(input_file);
                return EXIT_FAILURE;
            }

            fwrite(&is_rna, sizeof(is_rna), 1, output_file);
            length = 0;
            fwrite(&length, sizeof(length), 1, output_file);

            sequence[0] = '\0';
            sequence_length = 0;
        } else {
            size_t buffer_length = strlen(buffer);
            if (sequence_length + buffer_length >= sizeof(sequence) - 1) {
                write_compressed_sequence(output_file, sequence, is_rna);
                sequence[0] = '\0';
                sequence_length = 0;
            }
            sequence_length += buffer_length;
            strcat(sequence, buffer);
        }
    }

    if (output_file && strlen(sequence) > 0) {
        write_compressed_sequence(output_file, sequence, is_rna);
        printf("%lu\n", length);
        update_sequence_size(output_file);
        fclose(output_file);
    }

    fclose(input_file);
    return EXIT_SUCCESS;
}
