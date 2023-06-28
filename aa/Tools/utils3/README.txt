
                       Utils3 Map Compiler
                          October 2010

1. Introduction

The utils3 map compiler resides in the Tools/utils3/ subdirectory
in the SVN repository.  (It was recently moved there from the
source/ subdirectory.)  It is derived from id Software's GPL'd
q2map with modifications by others.  Possibly, it was the source
for the compiler in Tools/Map compiling tools/2008/.  Some time
ago, Stratocaster modified qrad3 and implemented a build for
Linux.  Recently, he implemented a Microsoft Visual Studio 2010
build and made some more modifications to qrad3.

This document assumes knowledge of map building and compiling.
It does not attempt describe many of the programs' options.

2. Running the Map Compiler

There are three separate programs.  The only thing new is the
dependency on certain aspects of the Quake file hierarchy is
removed.  The programs find resources (textures, etc.) relative
to the current working directory.  That means programs are run
with the terminal or command program in the arena/maps/
subdirectory or the data1/maps/ subdirectory.  The programs do
not need to be in the current working directory; they can be in
the PATH.

The command lines are:

  qbsp3 <options> <mapname>.map
  qvis3 <options> <mapname>
  qrad3 <options> <mapname>

For each program, the -help option will list the options
available.  The lighting options for qrad3 are described below.
(Editors Note: Stratocaster has only vague ideas about what the
other options do.)

3. Qrad3 Lighting Options

To tune these options to your liking and understand what they do,
experimentation with small test maps is recommended.  The first 4
options here are applied, per sample, as the last steps in
lightmap generation in this sequence:

     ambient is added
     scale is applied and RGB is normalized to 0..255 range
     grayscale is applied
     RGB values are scaled using maxlight

  3.1. -ambient :: range -255..255 : default 0

     Used to add light to the entire lightmap.  This value is
     added to each color in RGB format.  Only small values are
     useful.  Handy for giving the map some light during
     development.  The range is allowed to be negative for
     experimenting.

  3.2. -scale :: range 0.0..n : default 1.0

     A simple scaling factor for the entire lightmap.  The range
     is not constrained in the positive direction for
     experimenting.

  3.3. -grayscale (or -greyscale) :: range 0.0..1.0 : default 0.0

     Formerly, this was -nocolor, which removed all color from
     the lightmap.  With the grayscale option this can be done
     proportionally (0.0 for full color, 1.0 for no color).  Set
     this option to a value greater than zero to tone down the
     color in the lightmap.

  3.4. -maxlight :: range 0..255 : default 255

     When one or more of R, G, or B in a sample exceeds the
     maxlight setting: the sample is scaled by a ratio of
     maxlight and the sample's maximum of R, G and B.  In other
     words, the sample's RGB is scaled to a 0..maxlight range.
     Formerly, it defaulted to 192.

  3.5. -desaturate :: range 0.0..1.0 : default 0.0

     An average of the color of a texture is calculated and used
     as a "reflectance" value during lightmap generation.  The
     desaturate option desaturates the reflectance value
     proportionally (0.0 full color value, 1.0 no color).  Set
     this option to tone down the color related to reflections
     from textures.  Setting it to 1.0 should come close to
     matching the 2007 and 2008 compilers.

  3.6. -direct :: range 0.0..1.0 : default 0.0

     "Direct lights" are brush surfaces with a non-zero light
     value.  These are also known as "emissive" lights.
     Conventionally, this is used to give a glow to a surface and
     does not cast or reflect light to other surfaces.  To
     include these lights in radiosity calculations, set this
     option to a non-zero value.

  3.7. -entity :: range 0.0..1.0 : default 1.0

     This option is a scaling factor for radiosity of "point"
     lights (light entities).  To attenuate these lights' effect
     on the lightmap, set this to less than 1.0.

  3.8. -bounce :: range 0..n : default 4

     For radiosity calculations, controls how many times a light
     ray is reflected; "bounced" from one surface to another.
     This used to default to 8, but 4 seems "good enough".  Set
     it to zero for speed during development.

4. Qrad3 Sun/Directional Light Implementation

The program supports one sun; a "directional" light or, in other
words, a light source at infinite distance.  Add sunlight using
these keywords and values in the world entity.  Values shown are
for example.  Experiment to find settings to your liking.

     _sun          suntarget
     _sun_color    1.0 0.8 0.8
     _sun_light    50
     _sun_ambient  10

     Then:

     Add a light entity with the target key set to "suntarget".
     Add an "info_null" with the targetname key set to
     "suntarget".
     Adjust the positions of the light entity and the info_null
     to set the direction.

Note that it is only the direction from the light to the
info_null that is significant.  According to the program, there
are alternatives to this method, but they have not been tested
recently.

Also, this is a general "directional" light; it does not have to
be a "sun".  It can be any color, from any direction.  It casts
light on surfaces that can "see" a skybox surface, if they are
"looking" in the set direction.  Experiment to see the affect of
different _sun_light, _sun_ambient, and _sun_color settings.

5. Linux Build

See the Makefile for options. The commands are:

  $ make
  $ sudo make install

By default, the programs are installed by copying them to
/usr/local/bin.


--- Copyright and License --------------------------------------

Copyright (C) 2010 COR Entertainment, LLC.

Permission is granted to copy, distribute and/or modify this
document under the terms of the GNU Free Documentation License,
Version 1.3 or any later version published by the Free Software
Foundation; with no Invariant Sections, no Front-Cover Texts, and
no Back-Cover Texts.

----------------------------------------------------------------
