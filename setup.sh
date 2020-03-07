#! /usr/bin/bash

# copy icon to ~/.icons
mkdir -p ~/.icons/ && cp ./bin/icon.svg ~/.icons/com.editor.gamma.svg


# edit com.editor.gamma.desktop put the path 
cp ./bin/com.editor.gamma.desktop.bak ./bin/com.editor.gamma.desktop
sed -i -e "s,\[gamma path placeholder\],$PWD/bin/gamma," ./bin/com.editor.gamma.desktop

# and copy to ~/.local/share/applications
mkdir -p ~/.local/share/applications/ && cp ./bin/com.editor.gamma.desktop ~/.local/share/applications/
 
 
 # create a symbolic link to gamma sh file
sudo ln -s $PWD/bin/gamma /usr/bin/gamma

