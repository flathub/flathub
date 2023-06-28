#! /bin/sh

# run_splint.sh: a simple script to run Splint on all the source code.

PREPROC_FLAGS="-I. -I../config/ -I./game -I/usr/include/i386-linux-gnu -I/usr/include/x86_64-linux-gnu"
DEFINE_FLAGS="-DHAVE_CONFIG_H"
WARNING_FLAGS="-weak -nestcomment -warnposix -fixedformalarray +relaxtypes -namechecks"
DO_SPLINT="splint $PREPROC_FLAGS $DEFINE_FLAGS $WARNING_FLAGS"

ALL_FILES="`echo "game/*.c game/acesrc/*.c ref_gl/*.c unix/*.c server/*.c qcommon/*.c null/*.c client/*.c"`"
IGNORE_FILES_SED="s/ref_gl\\/r_ttf.c//"
TESTED_FILES="`echo $ALL_FILES | sed $IGNORE_FILES_SED`"

$DO_SPLINT $TESTED_FILES
