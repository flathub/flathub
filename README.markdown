# HandBrake on Flathub

This is where HandBrake Flathub builds are hosted.

All HandBrake development, including maintenance of Flatpak manifests, is performed in our [HandBrake GitHub repository](https://github.com/HandBrake/HandBrake).  Please report any issues or create pull requests there.

# Flatpak Installation

Installing HandBrake from the Flathub repository
```
flatpak --user remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak --user install flathub fr.handbrake.ghb
```

# Flatpak Updates

Updating HandBrake from the Flathub repository
```
flatpak --user update fr.handbrake.ghb
```

# Run Flatpak

To run the HandBrake GUI
```
flatpak run fr.handbrake.ghb
```
To run the HandBrake CLI
```
flatpak run --command=HandBrakeCLI fr.handbrake.ghb <cli args>
```

# About HandBrake

HandBrake is an open-source video transcoder available for Linux, Mac, and Windows, licensed under the [GNU General Public License (GPL) Version 2](LICENSE).

HandBrake takes videos you already have and makes new ones that work on your mobile phone, tablet, TV media player, game console, computer, or web browser—nearly anything that supports modern video formats.

HandBrake works with most common video files and formats, including ones created by consumer and professional video cameras, mobile devices such as phones and tablets, game and computer screen recordings, and DVD and Blu-ray discs. HandBrake leverages tools such as FFmpeg, x264, and x265 to create new MP4 or MKV video files from these *Sources*.

For information on downloading, building/installing, and using HandBrake, see the official [HandBrake Documentation](https://handbrake.fr/docs).

For information about contributing to HandBrake, please visit our [HandBrake GitHub repository](https://github.com/HandBrake/HandBrake).
