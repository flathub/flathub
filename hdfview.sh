#!/bin/bash

# File Name: hdfview.sh
# This script file is used to execute the hdfview utility

# Set up default variable values if not supplied by the user.
# ... hdfview.root property is for the install location
# ...... default location is system property user.dir
# ... hdfview.workdir property is for the working location to find files
# ...... default location is system property user.home
#

export INSTALLDIR=/app
export JAVABIN=/app/jre/bin
export JAVAOPTS=-Xmx1024M

# Default invocation when using modules:
"$JAVABIN/java" "$JAVAOPTS" \
  -Djava.library.path="$INSTALLDIR/lib/hdfview:$INSTALLDIR/lib/hdfview/ext" \
  -Dhdfview.root="$INSTALLDIR/lib/hdfview" \
  -classpath "$INSTALLDIR/lib/hdfview/*" \
  hdf.view.HDFView "$@"
