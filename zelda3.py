#!/usr/bin/env python3

import os
import shutil
from pathlib import Path
import hashlib

dir_home = str(Path.home()) + '/.var/app/io.github.snesrev.Zelda3/data/'
os.chdir(dir_home)

if not os.path.isfile('zelda3.ini'):
    shutil.copytree('/app/bin/src', 'src', dirs_exist_ok=True)
    shutil.copytree('/app/bin/src/shader', 'shader', dirs_exist_ok=True)
    from src.tables.util import ZELDA3_SHA1_US, ZELDA3_SHA1
    
    #
    # english ROM installation
    #
    file = os.popen('zenity --info --text "In order to play you have to select an english ROM file!\n\nSHA1: ' + ZELDA3_SHA1_US + '" --ok-label "Select ROM"').read().strip('\n')
    file = os.popen('zenity --file-selection --title="Select english ROM (SHA1: ' + ZELDA3_SHA1_US + ')"').read().strip('\n')

    if not os.path.isfile(file):
        os.popen('zenity --error --text "No correct file selected"').read()
        quit()

    with open(file, 'rb') as f:
        sha1 = hashlib.sha1()
        sha1.update(f.read())
        if sha1.hexdigest().upper() == ZELDA3_SHA1_US:
            shutil.copyfile(file, 'src/tables/zelda3.sfc')
            os.chdir(dir_home + 'src/tables/')
            os.popen('python3 restool.py --extract-from-rom -r zelda3.sfc | zenity --progress --title="Extracting" --text="Extracting ROM" --pulsate --auto-close --auto-kill').read()
            if not os.path.isfile('zelda3_assets.dat'):
                quit()
            os.chdir(dir_home)
            shutil.copyfile('src/tables/zelda3_assets.dat', 'zelda3_assets.dat')
        else:
            os.popen('zenity --error --text "No supported ROM!\nEnglish ROM needed\n\nSHA1 needed: ' + ZELDA3_SHA1_US + '\nSHA1 got: ' + sha1.hexdigest().upper()  + '"').read()
            quit()
            
    #
    # localized ROM installation
    #
    lang = None
    if os.system('zenity --question --text "Do you want to install a translation?"') == 0:
        tmp = '\n'.join([(value[0]).ljust(10) + "\t" + key for key, value in ZELDA3_SHA1.items()])
        os.popen('zenity --info --text "Select a localized ROM file!\n\nSHA1:\n' + tmp + '" --ok-label "Select ROM"').read().strip('\n')
        file = os.popen('zenity --file-selection --title="Select a localized ROM file!"').read().strip('\n')
        
        if not os.path.isfile(file):
            os.popen('zenity --error --text "No correct file selected"').read()
            quit()
            
        with open(file, 'rb') as f:
            sha1 = hashlib.sha1()
            sha1.update(f.read())
            
            if sha1.hexdigest().upper() in ZELDA3_SHA1.keys() and sha1.hexdigest().upper() != ZELDA3_SHA1_US:
                lang = ZELDA3_SHA1[sha1.hexdigest().upper()][0]
                shutil.copyfile(file, 'src/tables/translation.sfc')
                os.chdir(dir_home + 'src/tables/')
                os.remove('zelda3_assets.dat')
                os.popen('python restool.py --extract-dialogue -r translation.sfc | zenity --progress --title="Extracting" --text="Extracting ROM" --pulsate --auto-close --auto-kill').read()
                os.popen('python restool.py --languages=' + lang + '| zenity --progress --title="Extracting" --text="Extracting ROM" --pulsate --auto-close --auto-kill').read()
                if not os.path.isfile('zelda3_assets.dat'):
                    quit()
                os.chdir(dir_home)
                shutil.copyfile('src/tables/zelda3_assets.dat', 'zelda3_assets.dat')
            else:
                os.popen('zenity --error --text "No supported ROM!\n\nSHA1 got: ' + sha1.hexdigest().upper()  + '"').read()
                quit()

    #
    # configuration & cleanup
    #
    shutil.copyfile('src/zelda3.ini', 'zelda3.ini')
    os.system('sed -i "s/DimFlashes = 0/DimFlashes = 1/g" zelda3.ini')
    os.system('sed -i "s/ExtendedAspectRatio = 4:3/ExtendedAspectRatio = 16:9/g" zelda3.ini')
    os.system('sed -i "s/Fullscreen = 0/Fullscreen = 1/g" zelda3.ini')
    os.system('sed -i "s/OutputMethod = SDL/OutputMethod = OpenGL/g" zelda3.ini')
    os.system('sed -i "s/Shader =/Shader = shader\/xbrz\/shaders\/xbrz-freescale.glsl/g" zelda3.ini')
    if lang != None: os.system('sed -i "s/# Language = de/Language = ' + lang + '/g" zelda3.ini')
    shutil.rmtree('src/')
    
    if os.system('zenity --question --text "How to proceed?" --cancel-label "Open Config file" --ok-label "Start game"') != 0:
        os.system('xdg-open zelda3.ini')
        quit()
else:
    if os.system('for i in {1..20}; do echo $((i * 5)); sleep 0.1; done | zenity --progress --title="Starting..." --text="Wait for start" --cancel-label "Open Config file" --ok-label "" --auto-close --auto-kill') != 0:
        os.system('xdg-open zelda3.ini')
        quit()
os.system('zelda3')
