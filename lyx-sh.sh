#!/bin/bash

directory="${HOME}/.lyx"
_pwd=$PWD

Init(){
	
	echo "sh --> Starting LyX ..."
	exec lyx "$@"
}

Prepare() {
	
		echo "sh --> Directory $directory is empty, executing: configure.py"
		cd $directory
		python3 /app/share/lyx/configure.py
		echo "sh --> Copying lyxrc.defaults ..."
		cp /app/temp/lyxrc.defaults ${directory}/
		cd $_pwd
		
		Init
    
}

if [ -d "$directory" ]; then

    if test -z "$(ls -A $directory)"; then
		Prepare
    else
        Init
    fi

else
    mkdir $directory
    Prepare
fi

