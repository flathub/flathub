#!/bin/bash
GAME=$1
ARGS="${@:2}"
echo "Game: $GAME"
echo "Arguments: $ARGS"

if [[ -z $GAME ]]; then
    if [[ ! -f $XDG_DATA_HOME/.default_game ]]; then
        defaultgame=$(zenity --question --title="Select default game" --text "Choose whether to launch Descent or Descent II by default" --extra-button="Descent" --extra-button="Descent II" --extra-button="Cancel" --switch)
        case $defaultgame in
            "Descent")
                echo "d1x" > $XDG_DATA_HOME/.default_game
                ;;
            "Descent II")
                echo "d2x" > $XDG_DATA_HOME/.default_game
                ;;
            *)
                exit 1
                ;;
        esac
    fi
    GAME=$(cat $XDG_DATA_HOME/.default_game)
fi

case $GAME in
    "d1x")
        DATAFILE="descent.hog"
        DATADIR="$XDG_DATA_HOME/.d1x-rebirth"
        EXECUTABLE="d1x-rebirth"
        HOMEVAR="D1X_REBIRTH_HOME"
        GAMETITLE="Descent"
        ;;
    "d2x")
        DATAFILE="descent2.hog"
        DATADIR="$XDG_DATA_HOME/.d2x-rebirth"
        EXECUTABLE="d2x-rebirth"
        HOMEVAR="D2X_REBIRTH_HOME"
        GAMETITLE="Descent II"
        ;;
    *)
        zenity --error --title="Unknown game specified"\
            --text "<b>Unknown game: <tt>$GAME</tt></b>\\n\\nPlease run this Flatpak launcher with either <tt><b>d1x</b></tt> or <tt><b>d2x</b></tt> as a parameter to select Descent or Descent II." \
            --ok-label 'Quit' \
            --no-wrap
        exit 1
        ;;
esac

if [[ $(find "$DATADIR/data/" -iname $DATAFILE) ]]; then
    # Prefer data and savegames in Flatpak installation
    export $HOMEVAR="$DATADIR/saves"
    $EXECUTABLE -hogdir "$DATADIR/data/" $ARGS
elif [[ $(find "$HOME/.$EXECUTABLE/data/" -iname "$DATAFILE") ]]; then
    # Otherwise try and run from existing installation
    exec $EXECUTABLE
else
    # Create game data/save paths if needed to take at least that step off the user
    if [[ ! -d "$DATADIR/data" ]]; then mkdir -p "$DATADIR/data"; fi
    if [[ ! -d "$DATADIR/saves" ]]; then mkdir -p "$DATADIR/saves"; fi
    # Finally, fail with error message.
    zenity --error \
        --text "<b>Could not find $GAMETITLE game data!</b>\\n\\nPlease copy the game data files for $GAMETITLE to <tt><b>$DATADIR/data/</b></tt>." \
        --ok-label 'Quit'
    exit 1
fi
