#!/bin/bash

user_src_dir=~/.local/src
user_upp_src_dir=$user_src_dir/upp

first_run=0
if [ ! -d $user_src_dir/upp ]; then
    first_run=1
fi

is_the_same_src_version() {
    upp_ver_file_path=$user_upp_src_dir/ver.txt
    if [ ! -f $upp_ver_file_path ]; then
        return 0
    fi
        
    flatpak_upp_ver_file_path=/app/src/ver.txt
    if [ ! -f $flatpak_upp_ver_file_path ]; then
        printf "Warning: File with upp sources version number in flatpak doesn't exist.\n"
        return 1
    fi
    
    if ! cmp -s $upp_ver_file_path $flatpak_upp_ver_file_path; then
        return 0
    fi

    return 1
}

is_the_same_src_version
if [ $? -eq 0 ]; then
    printf "Copying/Updating sources to the user home directory.\n"
    if [ -d $user_upp_src_dir ]; then
        rm -rf $user_upp_src_dir
    fi
    mkdir -p $user_src_dir

    cp -r /app/src $user_upp_src_dir
fi

if [ $first_run -eq 1 ]; then
    upp-term "host-spawn /bin/bash $user_upp_src_dir/run-ide-install-host-deps.sh"
    if [ $? -eq 1 ]; then
        rm -rf $user_upp_src_dir
        exit
    fi
fi

theide
