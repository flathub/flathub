#!/bin/sh

cd $XDG_DATA_HOME
if [ ! -d "ampl.linux-intel64" ]
then

    (
        cp -r "/app/extra/ampl.linux-intel64" . ;
    ) |
    zenity --progress \
    --title="AMPL IDE" \
    --text="Configuring the application" \
    --percentage=0 \
    --auto-close \
    --auto-kill

    (( $? != 0 )) && zenity --error --text="Error while copying the client."

fi
exec ampl.linux-intel64/amplide/amplide "$@"