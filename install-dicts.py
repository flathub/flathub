#!/usr/bin/python3
import sys
import xml.etree.ElementTree as ET
import os
import shutil
import subprocess

def getName(elem):
    return elem.attrib['{http://openoffice.org/2001/registry}name']

def getType(elem):
    return elem.attrib['{http://openoffice.org/2001/registry}type']

spell_dest = "hunspell"
hyph_dest = "hyphen"
thes_dest = "mythes"

def parseProps(elem, origin):
    props = {}
    for prop in elem.getchildren():
        prop_name = getName(prop)
        prop_type = getType(prop)
        res = None
        if prop_type == "xs:string":
            res = ""
            for i in prop.getchildren()[0].itertext():
                res = res + i.replace("%origin%", origin)
        elif prop_type == "oor:string-list":
            res = []
            for i in prop.getchildren()[0].itertext():
                res = res + i.replace("%origin%", origin).split()
        else:
            print("Unknown type %s" % prop_type)
        props[prop_name] = res;
    return props

def handle_file(filename):
    tree = ET.parse(filename)
    lang = os.path.basename(os.path.dirname(filename)).split("_")[0]
    root = tree.getroot()
    dicts = root.getchildren()[0].getchildren()[0].getchildren()
    for dict in dicts:
        name = getName(dict)
        props = parseProps(dict, os.path.dirname(filename))
        format = props['Format']
        print ("Installing %s dictionary %s for lang %s:" % (format, name, lang))
        if format == 'DICT_SPELL':
            dest = spell_dest
            prefix = ""
            suffix = ""
        elif format == 'DICT_HYPH':
            dest = hyph_dest
            prefix = "hyph_"
            suffix = ""
        elif format == 'DICT_THES':
            dest = thes_dest
            prefix = "th_"
            suffix = "_v2"
        else:
            print ("Unknown format %s" % format)
            continue

        full_dest = "/app/share/" + dest + "/"
        os.makedirs(full_dest, exist_ok = True)

        for file in props['Locations']:
            if format == 'DICT_THES' and file.endswith(".dat") and os.path.isfile(file):
                idxname = file.replace(".dat", ".idx")
                if not os.path.isfile(idxname):
                    print (" Generate %s from %s" % (idxname, file))
                    f_in = open(file, "r")
                    f_out = open(idxname, "w")
                    subprocess.run(["perl", "th_gen_idx.pl"], stdin=f_in, stdout=f_out)
                    if not (idxname in props['Locations']):
                        props['Locations'].append(idxname)

        for file in props['Locations']:
            if not os.path.isfile(file):
                continue
            f, ext = os.path.splitext(file)
            basename = os.path.basename(file)
            dirname = os.path.dirname(file)
            full_dest_file = full_dest + basename
            print (" copy %s to %s" % (file, full_dest_file))
            shutil.copyfile(file, full_dest_file)

for i in sys.argv[1:]:
    handle_file(i)
