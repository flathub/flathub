#! /bin/sh

# run_oclint.sh: a simple script to run oclint on all the source code, or on
# just part of it.

# ./run_oclint.sh:
#   Run a non-detailed run on all source code (default)
# ./run_oclint.sh detailed:
#   Run a detailed check on all source code by checking each file
#   individually. This coaxes oclint into producing output about code style
#   instead of just outright mistakes.
# ./run_oclint.sh client/*.c, ./run_oclint.sh detailed client/*.c
#   Examples of picking specific files to check.

PREPROC_OPTIONS='-std=gnu90 -I. -I../config -I./game -DHAVE_CONFIG_H'
FILES="`echo game/*.c qcommon/*.c ref_gl/*.c unix/*.c server/*.c null/*.c client/*.c`"
DETAILED=false

if [ "$1" = "detailed" ] ; then
    DETAILED=true
    shift
fi
    
if [ "$#" != "0" ] ; then
    FILES="$@"
fi

if $DETAILED ; then
    for f in $FILES ; do oclint -text $f -- $PREPROC_OPTIONS ; done
else
    oclint -text $FILES  -- $PREPROC_OPTIONS
fi
