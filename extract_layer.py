import tarfile
import os
import sys

# Define directories
tmp_dir = '/tmp/image'
final_dir = '/tmp/layer'

if len(sys.argv) > 1:
    final_dir = sys.argv[1]

# Create directories if they don't exist
os.makedirs(tmp_dir, exist_ok=True)
os.makedirs(final_dir, exist_ok=True)

# Find the tar file in the current directory
tar_file = next((f for f in os.listdir('.') if f.endswith('.tar')), None)

if not tar_file:
    print("No tar file found in the current directory.")
    exit(1)

# Extract the main tar file to TMP_DIR
with tarfile.open(tar_file, 'r') as tar:
    tar.extractall(path=tmp_dir)

# Find the layer.tar file
layer_tar_path = None
for root, dirs, files in os.walk(tmp_dir):
    for file in files:
        if file == 'layer.tar':
            layer_tar_path = os.path.join(root, file)
            break

if layer_tar_path:
    with tarfile.open(layer_tar_path, 'r') as layer_tar:
        layer_tar.extractall(path=final_dir)
    print("Extraction completed successfully.")
else:
    print("layer.tar not found")
    exit(1)
