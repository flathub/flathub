#!/bin/sh

# Java2D and Swing APIs use Xlib and support HiDPI via GDK_SCALE var
# http://hg.openjdk.java.net/jdk9/jdk9/jdk/rev/bc2d1130105f#l27.8
GDK_SCALE=1
is_natural_number='^[0-9]+$'
DPI=`xgetres Xft.dpi`
# if $DPI is a natural number, taken from https://stackoverflow.com/a/806923
if [[ $DPI =~ $is_natural_number ]] ; then
   # float division in bash, taken from https://stackoverflow.com/a/21032001
   SCALE=`echo "$DPI 96" | awk '{printf "%.1f \n", $1/$2}'`
   # Round in bash, taken from https://stackoverflow.com/a/26465573
   GDK_SCALE=$(LC_ALL=C printf "%.0f\n" $SCALE)
fi

LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/app/lib/
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/app/jre/lib/
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/lib
CLASSPATH=${CLASSPATH}:/app/rups/*
#CLASSPATH=${CLASSPATH}:/app/lib/swt.jar
#CLASSPATH=${CLASSPATH}:/app/share/
#CLASSPATH=${CLASSPATH}:/app/dist/

##MAINCLASS
MAINCLASS=com.itextpdf.rups.RupsLauncher
##JVM ARGUMENTS
VM_ARGS="-Xms512m -Xmx2048m"

export CLASSPATH
export LD_LIBRARY_PATH
export GDK_SCALE

# Java launch script based on:
# https://intellij-support.jetbrains.com/hc/en-us/community/posts/360009469040/comments/360002026560

${JAVA} ${VM_ARGS} -cp :${CLASSPATH} -Djava.library.path="${LD_LIBRARY_PATH}" ${MAINCLASS} "$1" "$2"
