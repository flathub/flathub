#!/bin/sh
if [ $# = 1 ]
then
    case $1 in
	--help | -h |  -'?')
	    cat /app/manpage | less
	    exit 0 ;;
    esac
fi
java -jar /app/cvrdecode.jar "$@"
