import subprocess
from tkinter import filedialog, Tk, Label, Button, StringVar, Radiobutton, Frame


input_file = None
output_dir = None

def run_compression():    
    if input_file and output_dir:
        program_output.set("")
        result = subprocess.run(['./decompression.exe', input_file, output_dir], capture_output=True, text=True)
        print(result.stdout)
        program_output.set(result.stdout)
    elif input_file is None and output_dir is None:
        program_output.set("Input file and output directory paths must be specified!")
    elif output_dir is None:
        program_output.set("Output directory path must be specified!")
    elif input_file is None:
        program_output.set("Input file path must be specified!")


def define_input_file():
    global input_file
    input_file = filedialog.askopenfilename(title="Select input file")
    input_file_var.set(str(input_file))


def define_output_dir():
    global output_dir
    output_dir = filedialog.askdirectory(title="Select output directory")
    output_dir_var.set(str(output_dir))




root = Tk()
root.title("Simple FASTA Decompression Program")
root.state('zoomed')

input_file_var = StringVar(value="No file selected")
output_dir_var = StringVar(value="No directory selected")
program_output = StringVar(value="")


file_frame = Frame(root)
file_frame.pack(pady=10)

Button(file_frame, text="Select input BIN file", command=define_input_file, width=20).grid(row=0, column=0, padx=5, pady=5, sticky="ew")
Label(file_frame, textvariable=input_file_var, wraplength=400, anchor="w", bg="white", relief="solid", padx=5, pady=5).grid(row=0, column=1, padx=5, pady=5, sticky="ew")

Button(file_frame, text="Select output directory", command=define_output_dir, width=20).grid(row=1, column=0, padx=5, pady=5, sticky="ew")
Label(file_frame, textvariable=output_dir_var, wraplength=400, anchor="w", bg="white", relief="solid", padx=5, pady=5).grid(row=1, column=1, padx=5, pady=5, sticky="ew")


run_frame = Frame(root)
run_frame.pack(pady=20)

Button(run_frame, text="Run decompression", command=run_compression).grid(row=0, column=0, padx=5, pady=30)
Label(run_frame, textvariable=program_output, wraplength=400, anchor="w", padx=5, pady=5).grid(row=1, column=0, padx=5)


root.mainloop()