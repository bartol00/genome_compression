#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>

#define BUFFER_SIZE 1024
uint64_t length; // the number of nucleotides in a sequence 
uint64_t position = 1ULL; // the position of the bytes that describe how many nucleotides in the sequence there are


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
    // sequence length is written to specified position and file pointer is returned to the end of the file
    fseek(output_file, position, SEEK_SET);
    fwrite(&length, sizeof(length), 1, output_file);
    fseek(output_file, 0, SEEK_END);
}


int input_validation(const char *input_dir, const char *rna_or_dna, const char *output_dir) {
    // check to see if the input file exists
    if (access(input_dir, F_OK) != 0) {
        fprintf(stderr, "Input file does not exist or cannot be accessed\n");
        return 1;
    }

    // check to see if the input file is FASTA
    const char *dot = strrchr(input_dir, '.');
    if (!dot || strcmp(dot, ".fasta") != 0) {
        fprintf(stderr, "Input file is not a FASTA file\n");
        return 1;
    }

    // check to see if the rna/dna flag is set properly to one of the two values
    if (strcmp(rna_or_dna, "rna") != 0 && strcmp(rna_or_dna, "dna") != 0) {
        fprintf(stderr, "The RNA/DNA specifier could not be identified\n");
        return 1;
    }

    // check to see if the output directory exists
    if (access(output_dir, F_OK) != 0) {
        fprintf(stderr, "Output directory does not exist or cannot be accessed\n");
        return 1;
    }

    return 0;
}


void get_output_path(const char *input_dir, const char *output_dir, char *output_filename) {
    const char *dot = strrchr(input_dir, '.');  // gets the last position of '.', i.e. the beginning index of the file extension
    const char *slash = strrchr(input_dir, '/');  // gets the last position of '/', i.e. the beginning index of the filename
    const char *filename_start = slash ? slash + 1 : input_dir; // gets the filename without the initial '/' character
    char filename_buffer[200];

    strncpy(filename_buffer, filename_start, dot - filename_start);
    filename_buffer[dot - filename_start] = '\0'; // contents of filename_buffer are set to the filename without the extension

    // new file extension is added to the filename and the final output path is set
    snprintf(output_filename, 300, "%s/%s.bin", output_dir, filename_buffer);
}


void remove_trailing_slashes(char *str) {
    int len = strlen(str);
    while (len > 0 && str[len - 1] == '/') {
        str[len - 1] = '\0';
        len--;
    }
}


int main(int argc, char *argv[]) {
    // check to see if the necessary arguments are accounted for
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <fasta_file> <dna_or_rna> <output_dir>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *input_dir = argv[1];
    // the RNA/DNA flag string is switched to lowercase for easier validation
    for (int i = 0; argv[2][i]; i++) {
        argv[2][i] = tolower((unsigned char)argv[2][i]);
    }
    const char *rna_or_dna = argv[2];
    remove_trailing_slashes(argv[3]); // unnecessary '/' characters at the end of the output path string are removed
    const char *output_dir = argv[3];

    // input validation is performed on the program arguments before they are passed to the rest of the program
    if (input_validation(input_dir, rna_or_dna, output_dir) == 1) {
        return EXIT_FAILURE;
    }

    // input file stream is opened
    FILE *input_file = fopen(input_dir, "r");
    if (!input_file) {
        fprintf(stderr, "Error opening input FASTA file\n");
        return EXIT_FAILURE;
    }

    // output filename is determined from the input filename and the output file stream is opened
    char output_filename[300];
    get_output_path(input_dir, output_dir, output_filename);
    FILE *output_file = fopen(output_filename, "wb");
    if (!input_file) {
        fclose(input_file);
        fprintf(stderr, "Error opening output binary file\n");
        return EXIT_FAILURE;
    }

    // flag used to tell the decompression program whether the sequence is RNA or DNA is written to the beginning of the output file
    unsigned char is_rna = strcmp(rna_or_dna, "rna") == 0 ? 0x01 : 0x00;
    fwrite(&is_rna, sizeof(is_rna), 1, output_file);

    // variables for keeping track of various elements' lengths
    uint16_t description_length;
    size_t buffer_length;
    unsigned int sequence_length = 0;

    // buffers for line and sequence parsing
    unsigned char buffer[BUFFER_SIZE];
    unsigned char sequence[BUFFER_SIZE * 1024];

    while (fgets(buffer, sizeof(buffer), input_file)) {
        if (buffer[0] == '>') {
            // the leftover genome sequence contents are written as new sequence description is discovered
            // the position is also updated by adding the number of sequence bytes
            if (sequence_length > 0) {
                write_compressed_sequence(output_file, sequence, is_rna);
                update_sequence_size(output_file);
                position += (sizeof(length) + ((length + 3) / 4));
            }

            // the sequence contents and length are reset
            sequence[0] = '\0';
            sequence_length = 0;

            // '\n' character at the end of the description string is removed
            buffer[strlen(buffer)-1] = '\0';

            // the length of the sequence is set/returned to 0
            length = 0ULL;

            // length of description string is calculated and added to output file, along with the description contents
            // a placeholder value for the actual sequence string is also added
            description_length = (uint16_t)strlen(buffer);
            fwrite(&description_length, sizeof(description_length), 1, output_file);
            fwrite(buffer, sizeof(char), description_length, output_file);
            fwrite(&length, sizeof(length), 1, output_file);

            // the position is updated by adding the length of the description along with the number of bytes describing its length
            position += (sizeof(description_length) + description_length);
        } else {
            // buffer length is calculated and checked to see if the total sequence length is larger thn the sequence buffer
            // in which case it needs to be written to the output file and dynamically emptied
            buffer_length = strlen(buffer);
            if (sequence_length + buffer_length > sizeof(sequence)) {
                write_compressed_sequence(output_file, sequence, is_rna);
                sequence[0] = '\0';
                sequence_length = 0;
            }
            sequence_length += buffer_length;
            strcat(sequence, buffer);
        }
    }

    // leftover sequence bytes are written to output file
    if (output_file && strlen(sequence) > 0) {
        write_compressed_sequence(output_file, sequence, is_rna);
        update_sequence_size(output_file);
    }

    // mandatory closing of file streams to prevent memory leaks
    fclose(input_file);
    fclose(output_file);
    return EXIT_SUCCESS;
}
