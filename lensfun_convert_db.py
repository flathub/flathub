#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# original file: https://github.com/lensfun/lensfun/blob/master/tools/update_database/follow_db_changes.py
# modifications: stripped down to db conversion

import glob, os, subprocess, calendar, time, tarfile, io, argparse, shutil, configparser, smtplib, textwrap
from subprocess import DEVNULL
from email.mime.text import MIMEText
from lxml import etree

parser = argparse.ArgumentParser(description="Generate tar balls of the Lensfun database, also for older versions.")
parser.add_argument("output_path", help="Directory where to put the XML files.  They are put in the db/ subdirectory.  "
                    "It needn't exist yet.")
parser.add_argument("db_path", help="Path to database.")
args = parser.parse_args()

class XMLFile:

    def __init__(self, root, filepath):
        self.filepath = filepath
        self.tree = etree.parse(os.path.join(root, filepath))

    @staticmethod
    def indent(tree, level=0):
        i = "\n" + level*"    "
        if len(tree):
            if not tree.text or not tree.text.strip():
                tree.text = i + "    "
            if not tree.tail or not tree.tail.strip():
                tree.tail = i
            for tree in tree:
                XMLFile.indent(tree, level + 1)
            if not tree.tail or not tree.tail.strip():
                tree.tail = i
        else:
            if level and (not tree.tail or not tree.tail.strip()):
                tree.tail = i

    def write_to_tar(self, tar):
        tarinfo = tarfile.TarInfo(self.filepath)
        root = self.tree.getroot()
        self.indent(root)
        content = etree.tostring(root, encoding="utf-8")
        tarinfo.size = len(content)
        tar.addfile(tarinfo, io.BytesIO(content))

def fetch_xml_files():
    os.chdir(args.db_path)
    xml_filenames = glob.glob("*.xml")
    xml_files = set(XMLFile(os.getcwd(), filename) for filename in xml_filenames)
    return xml_files


class Converter:
    from_version = None
    to_version = None
    def __call__(self, tree):
        root = tree.getroot()
        if self.to_version == 0:
            if "version" in root.attrib:
                del root.attrib["version"]
        else:
            root.attrib["version"] = str(self.to_version)

converters = []
current_version = 0
def converter(converter_class):
    global current_version
    current_version = max(current_version, converter_class.from_version)
    converters.append(converter_class())
    return converter_class


@converter
class From1To0(Converter):
    from_version = 1
    to_version = 0

    @staticmethod
    def round_aps_c_cropfactor(lens_or_camera):
        element = lens_or_camera.find("cropfactor")
        if element is not None:
            cropfactor = float(element.text)
            if 1.5 < cropfactor < 1.56:
                element.text = "1.5"
            elif 1.6 < cropfactor < 1.63:
                element.text = "1.6"

    def __call__(self, tree):
        super().__call__(tree)
        for lens in tree.findall("lens"):
            element = lens.find("aspect-ratio")
            if element is not None:
                lens.remove(element)
            calibration = lens.find("calibration")
            if calibration is not None:
                for real_focal_length in calibration.findall("real-focal-length"):
                    # Note that while one could convert it to the old
                    # <field-of-view> element, we simply remove it.  It is not
                    # worth the effort.
                    calibration.remove(real_focal_length)
            self.round_aps_c_cropfactor(lens)
        for camera in tree.findall("camera"):
            self.round_aps_c_cropfactor(camera)


@converter
class From2To1(Converter):
    from_version = 2
    to_version = 1

    def __call__(self, tree):
        super().__call__(tree)
        for acm_model in tree.findall("//calibration/*[@model='acm']"):
            acm_model.getparent().remove(acm_model)
        for distortion in tree.findall("//calibration/distortion[@real-focal]"):
            etree.SubElement(distortion.getparent(), "real-focal-length", {"focal": distortion.get("focal"),
                                                                           "real-focal": distortion.get("real-focal")})
            del distortion.attrib["real-focal"]


def generate_database_tarballs(xml_files):
    version = current_version
    output_path = os.path.join(args.output_path, "db")
    shutil.rmtree(output_path, ignore_errors=True)
    os.makedirs(output_path)
    metadata = [[], []]
    while True:
        metadata[1].insert(0, version)

        tar = tarfile.open(os.path.join(output_path, "version_{}.tar".format(version)), "w")
        for xml_file in xml_files:
            xml_file.write_to_tar(tar)
        tar.close()

        try:
            converter_instance = converters.pop()
        except IndexError:
            break
        assert converter_instance.from_version == version
        for xml_file in xml_files:
            converter_instance(xml_file.tree)
        version = converter_instance.to_version

xml_files = fetch_xml_files()
generate_database_tarballs(xml_files)
