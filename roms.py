# Copyright (c) 2017-2021, Md Imam Hossain (emamhd at gmail dot com)
# see LICENSE.txt for details

from os import listdir

from engine import GNGEO_ROMS_LIST

class Roms():

    def __init__(self):

        self.directories = []
        self.roms = {}

    def add_directory(self, _dir):

        if _dir not in self.directories:

            self.directories.append(_dir)

    def remove_directory(self, _dir):

        if _dir in self.directories:

            self.directories.remove(_dir)

    def update_roms(self):

        self.roms.clear()

        for directory in self.directories:

            directory_files = []

            try:
                directory_files = listdir(directory)
            except FileNotFoundError:
                print('Error:', '"'+directory+'"', 'does not exist')
            else:
                pass

            for file_name in directory_files:

                if file_name.split('.')[0] in GNGEO_ROMS_LIST.keys():

                    self.roms[file_name.split('.')[0]] = directory

