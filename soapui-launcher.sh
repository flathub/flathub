#!/bin/sh

SOAPUI_HOME="/app"
SOAPUI_JAR=`ls /app/bin/soapui-*.jar`
JFXRTPATH="/app/jre/lib/jfxrt.jar"
SOAPUI_CLASSPATH=$JFXRTPATH:$SOAPUI_JAR:$SOAPUI_HOME/lib/*:$XDG_DATA_HOME/lib/*

#JAVA OPTS
JAVA_OPTS="-Xms128m -Xmx1024m -XX:MinHeapFreeRatio=20 -XX:MaxHeapFreeRatio=40"
JAVA_OPTS="$JAVA_OPTS -Duser.home=${XDG_CONFIG_HOME}"
JAVA_OPTS="$JAVA_OPTS -Dsoapui.properties=${XDG_CONFIG_HOME}/soapui.properties"
JAVA_OPTS="$JAVA_OPTS -Dsoapui.home=${SOAPUI_HOME}/bin -splash:SoapUI-Spashscreen.png"
JAVA_OPTS="$JAVA_OPTS -Dsoapui.ext.libraries=${XDG_DATA_HOME}/ext"
JAVA_OPTS="$JAVA_OPTS -Dsoapui.ext.listeners=${XDG_DATA_HOME}/listeners"
JAVA_OPTS="$JAVA_OPTS -Dsoapui.ext.actions=${XDG_DATA_HOME}/actions"
JAVA_OPTS="$JAVA_OPTS -Djava.library.path=${SOAPUI_HOME}/bin"
JAVA_OPTS="$JAVA_OPTS -Dwsi.dir=${SOAPUI_HOME}/wsi-test-tools"
#uncomment to disable browser component
#JAVA_OPTS="$JAVA_OPTS -Dsoapui.browser.disabled=true"
#CVE-2021-44228
JAVA_OPTS="$JAVA_OPTS -Dlog4j2.formatMsgNoLookups=true"
#JAVA 16
#JAVA_OPTS="$JAVA_OPTS --illegal-access=permit"

export SOAPUI_HOME
export SOAPUI_CLASSPATH
export JAVA_OPTS

exec java $JAVA_OPTS -cp $SOAPUI_CLASSPATH com.eviware.soapui.SoapUI "$@"
