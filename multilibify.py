#!/usr/bin/env python3

import copy
import json
import os
import argparse
import yaml

PROP_MULTILIB = "x-multilib"
PROP_COMPAT = "x-compat32-module"
PROP_NATIVE = "x-native-module"

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

def multilibify(holder_object, holder_file, add_native=True, add_compat32=True, native_props=None, compat32_props=None):
    module_dir = os.path.dirname(holder_file)

    default_multilib = holder_object.pop(PROP_MULTILIB, True)
    if native_props is None:
        native_props = holder_object.pop(PROP_NATIVE, {})
        if isinstance(native_props, str):
            native_props_file = os.path.join(module_dir, native_props)
            native_props = load_dict_file(native_props_file)
    if compat32_props is None:
        compat32_props = holder_object.pop(PROP_COMPAT, {})
        if isinstance(compat32_props, str):
            compat32_props_file = os.path.join(module_dir, compat32_props)
            compat32_props = load_dict_file(compat32_props_file)
    orig_modules = holder_object["modules"]
    modules = []

    for orig_module in orig_modules:
        if not isinstance(orig_module, dict) or not orig_module.pop(PROP_MULTILIB, default_multilib):
            modules.append(orig_module)
            continue

        module_native_props = orig_module.pop(PROP_NATIVE, {})
        module_compat32_props = orig_module.pop(PROP_COMPAT, {})

        if add_native:
            native_module = merge_dicts(orig_module, native_props)
            native_module = merge_dicts(native_module, module_native_props)
            if "modules" in native_module:
                native_module = multilibify(native_module, holder_file, add_compat32=False, native_props=native_props)
            modules.append(native_module)

        if add_compat32:
            compat32_module = merge_dicts(orig_module, compat32_props)
            compat32_module = merge_dicts(compat32_module, module_compat32_props)
            compat32_module["name"] = "{0}-32bit".format(orig_module["name"])
            if "modules" in compat32_module:
                compat32_module = multilibify(compat32_module, holder_file, add_native=False, compat32_props=compat32_props)
            modules.append(compat32_module)

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
