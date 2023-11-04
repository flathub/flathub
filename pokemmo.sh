#!/bin/sh

if [ ! -d $XDG_DATA_HOME/data ]
then
	cp -r /app/extra/data $XDG_DATA_HOME
fi

if [ ! -d $XDG_DATA_HOME/config ]
then
	cp -r /app/extra/config $XDG_DATA_HOME
fi

cd $XDG_DATA_HOME
java -Xmx384M -Dfile.encoding="UTF-8" -Dorg.lwjgl.glfw.libname=/app/lib/libglfw.so -cp /app/extra/PokeMMO.exe com.pokeemu.client.Client "$@"