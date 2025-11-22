#!/usr/bin/env python

import json
import yaml
import argparse
import os
import base64

OUT_PATH_BASE = 'third_party/node/{}'

def parse_base64(base64_string):
    """Parse base64 string to retrieve algorithm and hash."""
    algorithm, base64_hash = base64_string.split("-")
    decoded_bytes = base64.b64decode(base64_hash)
    hexadecimal_hash = decoded_bytes.hex()
    return algorithm, hexadecimal_hash

def generate_sources(dependencies_json):
    """Generate sources from package-lock.json data."""
    dependencies = dependencies_json.get('packages', {})
    sources = []
    
    for dep_name, dep_info in dependencies.items():
        if not dep_info.get('integrity'):
            continue
        
        archive_url = dep_info.get('resolved')
        algorithm, hexadecimal_hash = parse_base64(dep_info['integrity'])

        node = {
            'type': "archive",
            'url': archive_url,
            algorithm: hexadecimal_hash,
            'dest': OUT_PATH_BASE.format(dep_name)
        }

        sources.append(node)
    
    return sources

def write_sources_to_file(sources):
    """Write generated sources to YAML file."""
    with open('third-party-node-modules.yaml', 'w', encoding='utf-8') as f:
        yaml.dump(sources, f, explicit_start=False, default_flow_style=False, sort_keys=False)

def main():
    """Main function to parse package-lock.json and generate sources YAML."""
    parser = argparse.ArgumentParser(description='Parse a package-lock.json file.')
    parser.add_argument('filepath', type=str, help='Path to the package-lock.json file')
    args = parser.parse_args()

    if not os.path.isfile(args.filepath):
        print("Error: The provided path is not a valid file.")
        return
    
    with open(args.filepath, 'r') as file:
        package_lock_json = json.load(file)

    sources = generate_sources(package_lock_json)
    write_sources_to_file(sources)

if __name__ == "__main__":
    main()
