#!/bin/bash
#/******************************************************************************************************
# * @copyright by Ricardo Wagemaker (["java"].join(".") + "@wagemaker.co.uk") 2010-2018
# *
# * @author - Execuatble Script launcher by Ricardo Wagemaker 
# *
# * @license - This executable script launcher is part of FREE Software Collections and provided as is!
# * @homepage - http:/www.wagemaker.co.uk
# *
# ******************************************************************************************************/
#
#Setting up working environment
PROGRAM_DIR="/app/bin"
EXEC="SmallTextPad.jar"
export DISPLAY=:0.0

if type -p java; then
    echo found java executable in PATH
    _java=java
elif [[ -n "$JAVA_HOME" ]] && [[ -x "$JAVA_HOME/bin/java" ]];  then
    echo found java executable in JAVA_HOME     
    _java="$JAVA_HOME/bin/java"
else
    echo "no java"
fi

if [[ "$_java" ]]; then
    version=$("$_java" -version 2>&1 | awk -F '"' '/version/ {print $2}')
    echo version "$version"
    if [[ "$version" > "1.7.0" ]]; then
        echo "Java Version is OK"
	"${JAVA_PROGRAM_DIR}java" -jar ${PROGRAM_DIR}/${EXEC}
    else         
        echo "Java Version should be updated"
    fi
fi
