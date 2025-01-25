import os
import subprocess
import sys

DefaultFormat = "BC7_UNORM"
NormalFormat = "BC7_UNORM" 
HdrFormat = "BC6H_UF16"
AoFormat = "BC4_UNORM"
TexConvPath = os.path.join(os.path.dirname(__file__), "../TexConv/texconv.exe")

if len(sys.argv) < 2:
    print("Usage: python convert_textures.py <input_directory>")
    sys.exit(1)

InputDirectory = sys.argv[1]

if not os.path.isdir(InputDirectory):
    print(f"Error: The directory '{InputDirectory}' does not exist.")
    sys.exit(1)

for root, _, Files in os.walk(InputDirectory):
    for File in Files:
        if File.lower().endswith((".png", ".jpg")):
            FilePath = os.path.join(root, File)
            RelativePath = os.path.relpath(FilePath, InputDirectory)
            
            if "normal" in File.lower():
                CompressionFormat = NormalFormat
            elif "metal" in File.lower() and "rough" in File.lower():
                CompressionFormat = NormalFormat
            elif "hdr" in File.lower():
                CompressionFormat = HdrFormat
            elif "ao" in File.lower():
                CompressionFormat = AoFormat
            else:
                CompressionFormat = DefaultFormat

            if os.path.exists(os.path.splitext(FilePath)[0] + ".dds"):
                print(f"{RelativePath} already compressed")
            else:
                command = [
                TexConvPath, 
                "-f", CompressionFormat,  # Set the format
                "-m", "0",                # Generate mipmaps
                FilePath,                 # Input File
		        "-o", os.path.dirname(FilePath)
                ]

                print(f"Processing: {RelativePath} with format {CompressionFormat}")
                subprocess.run(command, check=True)

print("Conversion completed.")