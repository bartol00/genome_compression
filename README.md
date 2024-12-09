A compression/decompression program designed for the compresion of FASTA genomic sequences into a binary format, where each nucleotide is represented by only 2 bits of data, reducing the file size by roughly 4 times. Ideal for storage optimization when dealing with a large number of genomes in FASTA form.<br><br>

The program has a separate GUI written in Python. The user needs to download the whole archive and store all the files in the same location for the GUI components to work. The GUI components are:

- compression_gui.exe
- decompression_gui.exe

<br>

The compression program works by taking command line arguments from the user, where the input file directory, output directory and the nucleic acid of the genome are specififed. If the user wishes to run the program from the CLI, the argument setup is the following:

<pre>
$ {program name} {path to FASTA input file} {RNA/DNA} {path to desired output directory}<br>
</pre>
  
The compression program creates a binary (.bin) output file that the decompression program can interpret back to the same FASTA genomic sequence. Multiple distinct sequences in a single file are also permitted. In case of multiple sequences, the nucleic acid specification byte remains the same for every sequence in the file and is thus not repeated. The binary file specification is the following:

<pre>
Byte 0:                The nucleic acid specifier (RNA/DNA)
Bytes 1-2:             The length of the sequence description in characters (M)  
Bytes 3-(M+2):         The sequence description bytes  
Bytes (M+3)-(M+10):    The length of the actual genomic sequence in base pairs (N)  
Bytes (M+11)-(M+10+N): The genomic sequence characters<br>
</pre>

The decompression program creates the equivalent FASTA file from the binary file created by the compression program. It works by also taking command line arguments from the user, which specify the input file directory and the output directory. If the user wishes to run the program from the CLI, the argument setup is the following:

<pre>
$ {program name} {path to .bin input file} {path to desired output directory}<br>
</pre>

Both components of the program were tested with genome sequences of varying sizes, downloaded from the NCBI database. Some of those genomes include:

<pre>
Ebola virus strain Ebola virus/M.fascicularis-wt/GAB/2001/untreated-CCL053D9, complete genome
<a href="https://www.ncbi.nlm.nih.gov/nuccore/KY786027.1">https://www.ncbi.nlm.nih.gov/nuccore/KY786027.1</a>
Genome size: 18871 bp
Time required to compress:    0.003 seconds
Time required to decompress:  0.004 seconds
Uncompressed size:  18.7 KB
Compressed size:    5.07 KB

Stenotrophomonas maltophilia K279a complete genome, strain K279a
<a href="https://www.ncbi.nlm.nih.gov/nuccore/AM743169.1">https://www.ncbi.nlm.nih.gov/nuccore/AM743169.1</a>
Genome size: 4851126 bp
Time required to compress:    8.579 seconds
Time required to decompress:  0.516 seconds
Uncompressed size:  4.69 MB
Compressed size:    1.15 MB

Homo sapiens isolate NA21110 chromosome 1, whole genome shotgun sequence
<a href="https://www.ncbi.nlm.nih.gov/nuccore/CM089663.1">https://www.ncbi.nlm.nih.gov/nuccore/CM089663.1</a>
Genome size: 259033254 bp
Time required to compress:    282.781 seconds
Time required to decompress:   16.463 seconds
Uncompressed size:   250  MB
Compressed size:     61.7 MB<br>
</pre>

Note: the program does not support ambiguous nucleotides (for example: the undefined character 'N'). If such a sequence is provided, the program will create an incomplete binary file and return an error.<br><br>

Future ideas: 
1. Adding multithreading to the compression algorithm for speed increases
2. ~~Adding a GUI~~
3. Adding primary and secondary protein structure visualization directly from binary file
