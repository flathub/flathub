#!/bin/sh
java -Xmx4096m -Dstartupdir=$XDG_DATA_HOME -cp /app/share/ rotp.Rotp arg1
