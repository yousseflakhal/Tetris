import os

# Specify the directory containing the files
src_dir = "src"
# Output file name
output_file = "merged_output.txt"

# List of files in order
files_in_order = [
    "Board.cpp",
    "Board.hpp",
    "DrawUtils.cpp",
    "DrawUtils.hpp",
    "Game.cpp",
    "Game.hpp",
    "InputHandler.cpp",
    "InputHandler.hpp",
    "main.cpp",
    "Shape.cpp",
    "Shape.hpp",
    "SoundManager.cpp",
    "SoundManager.hpp"
]

# Open the output file
with open(output_file, "w", encoding="utf-8") as outfile:
    for filename in files_in_order:
        file_path = os.path.join(src_dir, filename)
        if os.path.exists(file_path):
            with open(file_path, "r", encoding="utf-8") as infile:
                outfile.write(f"// --- {filename} ---\n")
                outfile.write(infile.read())
                outfile.write("\n\n")
        else:
            print(f"Warning: {filename} not found.")

print(f"All files merged into {output_file}")
