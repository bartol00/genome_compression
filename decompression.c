#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <direct.h>
#include <stdint.h>
#include <time.h>


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


void write_decompressed_sequence(FILE *input_file, FILE *output_file) {
    unsigned char is_rna;
    unsigned char byte;
    uint16_t description_length;
    uint64_t sequence_length;
    int line_count;
    uint64_t total_count;
    unsigned char mask = 0b11000000;
    char nucleotide;
    uint8_t loop_flag;

    // flag for determining if sequence is RNA or DNA is obtained from input binary file
    fread(&is_rna, sizeof(is_rna), 1, input_file);

    // loop begins by reading initial two bytes that represent the length of the sequence description
    // loop will continue as long as there are sequences in the input file
    while (fread(&description_length, sizeof(description_length), 1, input_file) == 1) {
        line_count = 0; // determines where a new line separator should be placed between the sequence 
        total_count = 0;

        // sequence description is read from input file and written to output file, along with a new line separator
        char description_buffer[description_length];
        fread(description_buffer, sizeof(char), description_length, input_file);
        fwrite(description_buffer, sizeof(description_buffer), 1, output_file);
        fputc('\n', output_file);

        // length of the sequence is read from input file
        fread(&sequence_length, sizeof(sequence_length), 1, input_file);

        // input sequence is iterated through
        for (uint64_t i = 0; i < ((sequence_length + 3) / 4); i++) {
            fread(&byte, sizeof(byte), 1, input_file);
            for (int i = 0; i < 4; i++) {
                // the line and total count variables are incremented
                line_count++;
                total_count++;

                // raw bits are converted back into the nucleotide characters and written to output file
                nucleotide = bits_to_nucleotide((byte & (mask >> (2*i))) >> (6 - 2*i), is_rna);
                fputc(nucleotide, output_file);

                if (line_count == 70) {
                    // for every 70 nucleotides in the sequence written to output, a new line separator is added between them
                    fputc('\n', output_file);
                    line_count = 0;
                }

                // when total number of nucleotides processed reaches the length of the sequence, the sequence is complete
                // this is to prevent redundant bits in the last byte from affecting the final output
                if (total_count == sequence_length) {
                    // adds a new line separator for sequences that did not get their separator in the previous if-then clause
                    // these are sequences that have a bp number not divisible by 70
                    if (sequence_length % 70 != 0) {
                        fputc('\n', output_file);
                    }
                    break;
                }
            }
        }

        // adds multiple new line separators for better legibility when working with multiple sequences in the same file
        fputs("\n\n", output_file);
    }
}


int input_validation(const char *input_dir, const char *output_dir) {
    // check to see if the input file exists
    if (access(input_dir, F_OK) != 0) {
        fprintf(stderr, "Input file does not exist or cannot be accessed\n");
        return 1;
    }

    // check to see if the input file is a binary file
    const char *dot = strrchr(input_dir, '.');
    if (!dot || strcmp(dot, ".bin") != 0) {
        fprintf(stderr, "Input file is not a binary file\n");
        return 1;
    }

    // check to see if the output directory exists
    if (access(output_dir, F_OK) != 0) {
        fprintf(stderr, "Output directory does not exist or cannot be accessed\n");
        return 1;
    }

    return 0;
}


void remove_trailing_slashes(char *str) {
    int len = strlen(str);
    while (len > 0 && str[len - 1] == '/') {
        str[len - 1] = '\0';
        len--;
    }
}


void get_output_path(const char *input_dir, const char *output_dir, char *output_filename) {
    const char *dot = strrchr(input_dir, '.');  // gets the last position of '.', i.e. the beginning index of the file extension
    const char *slash = strrchr(input_dir, '/');  // gets the last position of '/', i.e. the beginning index of the filename
    const char *filename_start = slash ? slash + 1 : input_dir; // gets the filename without the initial '/' character
    char filename_buffer[200];

    strncpy(filename_buffer, filename_start, dot - filename_start);
    filename_buffer[dot - filename_start] = '\0'; // contents of filename_buffer are set to the filename without the extension

    // new file extension is added to the filename and the final output path is set
    snprintf(output_filename, 300, "%s/%s.fasta", output_dir, filename_buffer);
}


void get_binary_output (FILE *input_file) {
    // optional function for getting the hex representation of the input binary file
    // warning: will significantly slow down the decompression process if activated
    fseek(input_file, 0, SEEK_SET);
    unsigned char byte;
    int counter = 0;
    int remainder;
    while (fread(&byte, sizeof(byte), 1, input_file) == 1)
    {
        remainder = counter % 10;
        if (remainder == 0) {
            printf("\n");
            if (counter < 100) {
                printf("   %d:", counter);
            } else if (counter < 1000) {
                printf("  %d:", counter);
            } else if (counter < 10000) {
                printf(" %d:", counter);
            } else {
                printf("%d:", counter);
            }
        }
        
        printf("  %02X (%d)", byte, remainder);
        
        counter++;
    }
    printf("\n");
}


int main(int argc, char *argv[]) {
    // the variables to record how long program execution takes
    clock_t start, end; 
    start = clock();
    double cpu_time_used;

    // check to see if the necessary arguments are accounted for
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <path_to_compressed_file> <output_dir>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *input_dir = argv[1];
    remove_trailing_slashes(argv[2]); // unnecessary '/' characters at the end of the output path string are removed
    const char *output_dir = argv[2];

    // input validation is performed on the program arguments before they are passed to the rest of the program
    if (input_validation(input_dir, output_dir) == 1) {
        return EXIT_FAILURE;
    }

    // input file stream is opened
    FILE *input_file = fopen(input_dir, "rb");
    if (!input_file) {
        fprintf(stderr, "Error opening input binary file\n");
        return EXIT_FAILURE;
    }

    // output filename is determined from the input filename and the output file stream is opened
    char output_filename[300];
    get_output_path(input_dir, output_dir, output_filename);
    FILE *output_file = fopen(output_filename, "w");
    if (!input_file) {
        fclose(input_file);
        fprintf(stderr, "Error opening output FASTA file\n");
        return EXIT_FAILURE;
    }

    // sequence is decompressed from a binary file and written to an output FASTA file of the same name
    write_decompressed_sequence(input_file, output_file);

    // optional function for getting the hex representation of binary input file, uncomment for activation
    // get_binary_output(input_file);

    // mandatory closing of file streams to prevent memory leaks
    fclose(input_file);
    fclose(output_file);

    // prints out the time it took to execute the program in seconds
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time taken: %f seconds\n", cpu_time_used);

    return EXIT_SUCCESS;
}