#! /bin/sh

# run_cppcheck.sh: a simple script to run cppcheck on all the source code.
# Note: older versions of cppcheck can have false positives. I recommend 
# installing the latest version if you can.

PREPROC_FLAGS="-I. -I../config/ -I./game -DHAVE_CONFIG_H -U__cplusplus"
OUTPUT_FLAGS="-v"
PERF_FLAGS="-j 4 --max-configs=6"
DO_CPPCHECK="cppcheck $PREPROC_FLAGS $OUTPUT_FLAGS $PERF_FLAGS"

TESTED_FILES="game ref_gl unix server qcommon null client win32"

$DO_CPPCHECK $TESTED_FILES
