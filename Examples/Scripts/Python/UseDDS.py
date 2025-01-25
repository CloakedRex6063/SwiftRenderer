import os
import sys

if len(sys.argv) < 2:
    print("Usage: python convert_textures.py <input_directory>")
    sys.exit(1)

InputDirectory = sys.argv[1]

if not os.path.isdir(InputDirectory):
    print(f"Error: The directory '{InputDirectory}' does not exist.")
    sys.exit(1)

for root, _, Files in os.walk(InputDirectory):
    for File in Files:
        if File.lower().endswith((".gltf")):
            FilePath = os.path.join(root, File)
            try:
                with open(FilePath, 'rb') as f:
                    Content = f.read()

                ContentData = Content.decode('utf-8', errors='ignore')
                UpdatedContent = ContentData.replace('.jpg', '.dds').replace('.png', '.dds')

                # Write the modified content back to the file
                with open(FilePath, 'wb') as f:
                    f.write(UpdatedContent.encode('utf-8'))

                print(f"Updated references in {FilePath}")

            except Exception as e:
                print(f"Error processing {FilePath}: {e}")