#!/bin/sh

# We copy a lot of needed dirs because we're using the data home as the
# working dir. This is not elegant, but if we leave the working dir as
# Robocode's installation dir then it will fail as it tries to save some
# things in the working directory.
DIRS=(robots battles templates theme compilers)
for dir in ${DIRS[@]}; do
  fulldir="$XDG_DATA_HOME/$dir"
  if [ ! -d $fulldir ] || [ ! "$(ls -A $fulldir)" ]; then
    echo "Copying $dir -> $fulldir …"
    mkdir -p $fulldir
    cp -r /app/share/robocode/$dir/* $fulldir
  fi
done

# Other read-only things that need to be in the working dir.
ln -s /app/share/robocode/javadoc $XDG_DATA_HOME/ 2> /dev/null
ln -s /app/share/robocode/ReadMe.html $XDG_DATA_HOME/ 2> /dev/null
ln -s /app/share/robocode/versions.md $XDG_DATA_HOME/versions.md 2> /dev/null

COMPILER_PROPERTIES="$XDG_DATA_HOME/config/compiler.properties"
if [ ! -f $COMPILER_PROPERTIES ]; then
  echo "Copying compiler properties…"
  install -Dm644 /app/share/robocode/compiler.properties $COMPILER_PROPERTIES
fi

# Run the actual program
sh /app/share/robocode/robocode.sh
