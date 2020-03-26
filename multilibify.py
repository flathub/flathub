#!/usr/bin/env python3

import copy
import json
import os
import argparse
import yaml

MULTILIB_PROP = "x-multilib"
VARIANTS = {
    "native": {
        "prop": "x-native-module"
    },
    "compat": {
        "prop": "x-compat32-module",
        "name-suffix": "-32bit"
    }
}

def load_dict_file(dict_file):
    with open(dict_file, "r") as f:
        if any(dict_file.endswith(i) for i in [".yml", ".yml.in", ".yaml", "yaml.in"]):
            loader = yaml.safe_load
        elif any(dict_file.endswith(i) for i in [".json", ".json.in"]):
            loader = json.load
        else:
            raise ValueError(f"Unknown format of {dict_file}")
        d = loader(f)
    return d

def save_dict_file(dict_data, dict_file):
    with open(dict_file, "w") as f:
        if any(dict_file.endswith(i) for i in [".yml", ".yaml"]):
            writer = yaml.dump
        elif dict_file.endswith(".json"):
            writer = lambda o, f: json.dump(o, f, indent=4, sort_keys=False)
        else:
            raise ValueError(f"Unknown format of {dict_file}")
        writer(dict_data, f)

def merge_dicts(base_dict, update_dict, append_list=False):
    merged_dict = copy.deepcopy(base_dict)
    for upd_key, upd_val in update_dict.items():
        if isinstance(merged_dict.get(upd_key), dict) and isinstance(upd_val, dict):
            merged_dict[upd_key] = merge_dicts(merged_dict[upd_key], upd_val)
        elif append_list and isinstance(merged_dict.get(upd_key), list) and isinstance(upd_val, list):
            merged_dict[upd_key] += upd_val
        else:
            merged_dict[upd_key] = upd_val
    return merged_dict

def multilibify(holder_object, holder_file, add=None, props=None):
    if add is None:
        add = {v: True for v in VARIANTS.keys()}
    if props is None:
        props = {v: None for v in VARIANTS.keys()}
    module_dir = os.path.dirname(holder_file)

    default_multilib = holder_object.pop(MULTILIB_PROP, True)
    for v in VARIANTS.keys():
        if props[v] is None:
            props[v] = holder_object.pop(VARIANTS[v]["prop"], {})
        if isinstance(props[v], str):
            props[v] = load_dict_file(os.path.join(module_dir, props[v]))
    orig_modules = holder_object["modules"]
    modules = []

    for orig_module in orig_modules:
        if not isinstance(orig_module, dict) or not orig_module.pop(MULTILIB_PROP, default_multilib):
            modules.append(orig_module)
            continue

        module_props = {}
        for v in VARIANTS.keys():
            module_props[v] = orig_module.pop(VARIANTS[v]["prop"], {})

        for v in VARIANTS.keys():
            if not add[v]:
                continue
            new_module = merge_dicts(orig_module, props[v])
            new_module = merge_dicts(new_module, module_props[v])
            if "name-suffix" in VARIANTS[v]:
                new_module["name"] = "{0}{1}".format(orig_module["name"], VARIANTS[v]["name-suffix"])
            if "modules" in new_module:
                submodule_add = {i: i == v for i in VARIANTS.keys()}
                new_module = multilibify(new_module, holder_file, props=props, add=submodule_add)
            modules.append(new_module)

    holder_object["modules"] = modules
    return holder_object

def main():
    parser = argparse.ArgumentParser("Make flatpak-builder modules multilib")
    parser.add_argument("source_manifest")
    parser.add_argument("target_manifest")
    args = parser.parse_args()
    holder_object = load_dict_file(args.source_manifest)
    generated_manifest = multilibify(holder_object, args.source_manifest)
    save_dict_file(generated_manifest, args.target_manifest)

if __name__ == "__main__":
    main()
