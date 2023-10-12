# Pd-L2Ork packaged for Flatpak

This is a Flatpak for [Pd-L2Ork](http://l2ork.music.vt.edu/main/make-your-own-l2ork/software/).

Pd-L2Ork is a popular fork of the [Pure Data](http://puredata.info/), an open
source visual programming language for multimedia.

## How to build

This Flatpak uses the standard
[flatpak-builder](docs.flatpak.org/en/latest/flatpak-builder-command-reference.html)
tool to build.

You can run a command like the following to build the package from this repo
and install it in your 'user' Flatpak installation:

    flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
    flatpak install flathub org.freedesktop.Platform//22.08 org.freedesktop.Sdk//22.08
    (press Y to continue)
    git config --global --add protocol.file.allow always
    flatpak-builder --verbose --install ./build net.pdl2ork.PdL2Ork.yml --force-clean --user

If you encounter trouble building, you can try the following line which will force the use
of single CPU core (which will be slower but also safer based on my tests, as the compilation
can otherwise exit with code 9 at random times), and store the output into build.log file
that you will be able to analyze and share with the dev team.

    flatpak-builder --verbose --install ./build net.pdl2ork.PdL2Ork.yml --force-clean --user --jobs=1 > build.log 2>&

During development you can also run a build without installing it, like this:

    flatpak-builder --run build net.pdl2ork.PdL2Ork.yml pd-l2ork

See the [Flatpak manual](http://docs.flatpak.org/en/latest/) for more information.

## How to run

Once you've built the program, simply type:

    flatpak run net.pdl2ork.PdL2Ork

Enjoy!
