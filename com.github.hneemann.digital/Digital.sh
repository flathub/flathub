#!/bin/sh

CONFIGFILE=${XDG_CONFIG_HOME}/digital-options.ini

# write default configuration file if not existent
if [ ! -f ${CONFIGFILE} ]; then
    cat << EOF > ${CONFIGFILE}
#user_interface_font=sans-bold-16
user_interface_font="Cantarell Regular 18"
EOF
fi

# read configuration parameters
CFG_UI_FONT=$(cat ${CONFIGFILE} | grep user_interface_font | tail -1 | awk -F '=' '{print $2}')
echo CFG_UI_FONT=${CFG_UI_FONT}

export JAVA_HOME=/app/jre
export JAVA=${JAVA_HOME}/bin/java
export _JAVA_OPTIONS="-Dawt.useSystemAAFontSettings=gasp -Dswing.aatext=true -Dsun.java2d.xrender=True -Dswing.systemlaf=javax.swing.plaf.metal.MetalLookAndFeel -Dswing.defaultlaf=javax.swing.plaf.metal.MetalLookAndFeel -Dswing.plaf.metal.controlFont=${CFG_UI_FONT} -Dswing.plaf.metal.userFont=${CFG_UI_FONT}"
${JAVA} -jar /app/bin/Digital.jar
