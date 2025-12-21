#!/bin/sh
# This scripts requires playerctl and dbus-send

alias playerctl="playerctl -p my_player "

playerctl metadata
playerctl play
playerctl pause
playerctl play-pause
playerctl next
playerctl previous
playerctl stop
playerctl position 30
playerctl position 10-
playerctl position 10+
playerctl volume 0.5
playerctl open "https://testlink.com"
# TODO: Shuffle and repeat.
# playerctl shuffle
# playerctl repeat

# The following are commands not supported by playerctl, thus we use dbus-send
call() {
  dbus-send --dest=org.mpris.MediaPlayer2.my_player --print-reply /org/mpris/MediaPlayer2 "$1"
}

call org.mpris.MediaPlayer2.Raise
call org.mpris.MediaPlayer2.Quit
