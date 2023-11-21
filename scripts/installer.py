#!/usr/bin/python3

import requests
import json
import base64
import gzip
import pprint
import os
from pathlib import Path
import errno
import sys
import base64
import pprint
from hashlib import sha256
import datetime
import yaml

dirname = os.getcwd()

def mkdir_p(path):
    try:
        parent_path = Path(path)
        os.makedirs(parent_path.parent.absolute(), exist_ok=True)
    except OSError as exc:
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else: raise

def get_abs_path(relative_path):
    return os.path.join(dirname, relative_path)

def cleanup():
    for p in Path(".").glob("*.solidpiece"):
        p.unlink()
    for p in Path(".").glob("combined_file"):
        p.unlink()


def decode_base64(encoded_str):
    try:
        # Ensure the string is padded to a multiple of 4 characters
        padded_str = encoded_str + '=' * ((4 - len(encoded_str) % 4) % 4)
        
        # Decode the base64 string
        decoded_bytes = base64.b64decode(padded_str)
        
        # Return the decoded bytes
        return decoded_bytes
    except binascii.Error:
        print("An error occurred: Incorrect padding or corrupted data.")
        return None

def main():
    # try:
    global digest_list
    global pad_array
    global file_list
    
    metafile_yaml = []

    with open("metafile.json", "r") as stream:
        try:
            content = stream.read()
            decoded_jwt_body_json = json.loads(base64.b64decode(content.split(".")[1]).decode("utf-8"))
        except yaml.YAMLError as exc:
            print(exc)

    digest_list = decoded_jwt_body_json["pieces"]["digests"]
    file_list = decoded_jwt_body_json["files"]
    pad_array = decoded_jwt_body_json["pad"]

    for item in digest_list:
        file_name = "{}.solidpiece".format(base64.b64decode(item).hex())
        with open(get_abs_path(file_name), 'rb') as non_gzip_file:
            content = non_gzip_file.read()[6:]
            with open(get_abs_path(file_name), 'wb') as gzipped_file:
                gzipped_file.write(content)
        try:
            with gzip.open(file_name, 'rb') as compressed_file:
                print("Decompressing and writing {}".format(file_name))
                # Read and decompress the data
                decompressed_data = compressed_file.read()
                # Save the decompressed data to an output file
            with open(file_name, 'wb') as output_file:
                output_file.write(decompressed_data)
        except Exception as e:
            print("Skipping because it is not a gzip archive... {}".format(file_name))
            continue
    with open(get_abs_path('combined_file'), 'wb') as combined_file:
        for item in digest_list:
            file_name = "{}.solidpiece".format(base64.b64decode(item).hex())
            with open(file_name, 'rb') as temp_file:
                content = temp_file.read()
                combined_file.write(content)

    with open(get_abs_path('combined_file'), 'rb') as source_file:
        for file in file_list:
            print('Building {} by splitting off {} bytes of the combined zip'.format(file['name'], file['size']))                
            file_output = source_file.read(file['size'])
            output_file_path = get_abs_path(file['name'])
            # Make sure the directory we are writing to exists
            mkdir_p(output_file_path)
            with open(output_file_path, 'wb') as output:
                print('Writing {} to {}'.format(file['name'], output_file_path))
                output.write(file_output)

if __name__ == "__main__":
    main()
    cleanup()