#!/bin/sh

##LIBRARY_PATH
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/app/lib/
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/app/jre/lib/
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/lib


##CLASSPATH
CLASSPATH=${CLASSPATH}:/app/lib/tuxguitar.jar
CLASSPATH=${CLASSPATH}:/app/lib/tuxguitar-ui-toolkit.jar
CLASSPATH=${CLASSPATH}:/app/lib/tuxguitar-ui-toolkit-qt4.jar
CLASSPATH=${CLASSPATH}:/app/lib/tuxguitar-lib.jar
CLASSPATH=${CLASSPATH}:/app/lib/tuxguitar-editor-utils.jar
CLASSPATH=${CLASSPATH}:/app/lib/tuxguitar-gm-utils.jar
CLASSPATH=${CLASSPATH}:/app/lib/tuxguitar-awt-graphics.jar
CLASSPATH=${CLASSPATH}:/app/lib/qtjambi.jar
CLASSPATH=${CLASSPATH}:/app/lib/qtjambi-native.jar
CLASSPATH=${CLASSPATH}:/app/lib/gervill.jar
CLASSPATH=${CLASSPATH}:/app/lib/itext-pdf.jar
CLASSPATH=${CLASSPATH}:/app/lib/itext-xmlworker.jar
CLASSPATH=${CLASSPATH}:/app/lib/commons-compress.jar
CLASSPATH=${CLASSPATH}:/app/lib/icedtea-sound.jar
CLASSPATH=${CLASSPATH}:/app/share/
CLASSPATH=${CLASSPATH}:/app/dist/
##MAINCLASS
MAINCLASS=org.herac.tuxguitar.app.TGMainSingleton
##JVM ARGUMENTS
VM_ARGS="-Xmx512m"
##EXPORT VARS
export CLASSPATH
export LD_LIBRARY_PATH
##LAUNCH
${JAVA} ${VM_ARGS} -cp :${CLASSPATH} -Dtuxguitar.home.path="${APP_HOME_PATH}" -Dtuxguitar.share.path="/app/share/" -Djava.library.path="${LD_LIBRARY_PATH}" -Dorg.herac.tuxguitar.ui.qt.style=plastique ${MAINCLASS} "$1" "$2"