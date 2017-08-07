#!/usr/bin/env python3
import yaml
import json


def main():
    '''The master copy is only YAML so we can have comments.'''
    with open('org.freedesktop.Bustle.yaml', 'r', encoding='utf-8') as f:
        manifest = yaml.load(f)

    with open('org.freedesktop.Bustle.json', 'w', encoding='utf-8') as g:
        json.dump(obj=manifest, fp=g, indent=4)
        g.write('\n')


if __name__ == '__main__':
    main()
