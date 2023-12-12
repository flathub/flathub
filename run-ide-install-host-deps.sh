#!/bin/bash

AskContinue() {
  read -p "Continue (Y/n)?" answer
  if [ "$answer" != "${answer//[nN]/x}" ]; then
    exit
  fi
}

echo "You are running flatpaked TheIDE for the first time!"
echo "It is highly recommended that you install all required dependencies on your host operating system."
echo "Without it, some features might now work, including project compilation."
echo

uname=$(uname)

if [ -x "$(command -v apt-get)" ]; then
  DEP="apt-get install g++ clang git make libgtk-3-dev libnotify-dev libbz2-dev libssl-dev xdotool"
elif [ -x "$(command -v yum)" ]; then
  DEP="yum install gcc-c++ clang git make gtk3-devel libnotify-devel bzip2-devel freetype-devel openssl-devel"
elif [ -x "$(command -v dnf)" ]; then
  DEP="dnf install gcc-c++ clang git make gtk3-devel libnotify-devel bzip2-devel freetype-devel openssl-devel"
elif [ -x "$(command -v urpmi)" ]; then
  DEP="urpmi install gcc-c++ clang git make gtk3-devel libnotify-devel bzip2-devel freetype-devel openssl-devel"
elif [ -x "$(command -v zypper)" ]; then
  DEP="zypper install gcc-c++ clang git make gtk3-devel libnotify-devel bzip2-devel freetype-devel libopenssl-devel"
elif [ -x "$(command -v pacman)" ]; then
  DEP="pacman -Sy --needed gcc clang git make zlib bzip2 gtk3 libnotify openssl pkgconf gdb"
elif [ -x "$(command -v pkg)" ]; then
  DEP="pkg install bash gmake git gtk3 libnotify llvm90 pkgconf"
  if [[ "$uname" == 'SunOS' ]]; then
    DEP="pkg install bash git gtk3 libnotify developer/clang-80 build-essential"
  fi
elif [ -x "$(command -v pkg_add)" ]; then
  DEP="pkg_add bash git gmake gtk3 libnotify clang-devel"
fi

if [[ "$uname" == 'OpenBSD' ]]; then
  DEP=""
fi

if [ -z "$DEP" ]; then
  if [[ "$uname" == 'OpenBSD' ]]; then
    echo Automatic dependecies installation is not supported on OpenBSD.
    echo See README for details.
  else
    echo Packaging system was not identified.
    echo Automatic dependency instalation has failed.
    echo You will have to install required packages manually.
  fi
  echo Please make sure that build dependecies are satisfied.
  AskContinue
else
  echo Following command should be used to install required packages:
  echo sudo $DEP
  echo
  echo Install script can run this command for you, but that will require
  echo your sudo password.
  read -p "Do you want the script to do that (Y/n)?" answer
  if [ "$answer" == "${answer//[nN]/x}" ]; then
    if ! eval 'sudo $DEP'; then
      echo Failed to install all required packages.
      echo You will have to install required packages manually.
      AskContinue
    fi
    echo
    read -p "Everything done! Press enter to continue..."
  else
    echo Please make sure you install this dependencies.
    echo Without them you will not fully enjoy U++ and TheIDE
    AskContinue
  fi
fi
