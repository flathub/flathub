Open Syobon Action
==================
###### Original game developer: Chiku (ちく)
###### Open-source port developer: Mathew Velasquez
###### Developer/maintainer of the original repository: Karen／明美 (@angelXwind)
###### Packaged the game as a flatpak: eugenioLR 
#### [English] [[日本語]](README_ja.md)

Open Syobon Action (オープンしょぼんのアクション) is an open-source cross-platform port of the unforgiving Japanese platformer using SDL.

It is also called "Shobon no Action," "Neko Mario," "Cat Mario," or "Cat MeOUCHio."

Notes
-----
* Rather than just playing this game by yourself, it is much more enjoyable to let someone who's never tried it before try playing the game.
* Those who are under 12 years old are not allowed to play.
* Please be aware that by playing this game, there is a possibility that you may develop trust issues towards other humans.
* Please be aware that there is a possibility of getting stressed out while playing this game.
* If any problems are caused by this game, please understand that the developers cannot take any responsibility.
* If you wish to distribute this game to a large amount of people, please give credit to the original developer, Chiku (ちく).
* The game's text is entirely in Japanese, but you do not need to understand the Japanese language in order to play.

Controls (Keyboard)
-------------------
* Left/Right: Move
* Z or Up: Jump
* Space: Double-speed
* 0 (while in-game): Self-destruction
* Escape: Exit
* F1: Return to title screen
* 1-9 (while at the title screen): Select level
* 0 at the title screen: Mystery Dungeon (there is a possibility that some bugs may occur)

Bugs
----
Please report any bugs at the GitHub issue tracker: https://github.com/angelXwind/OpenSyobonAction/issues

Command Line Arguments
----------------------
`-nosound` - Disables sound

Dependencies
------------
* SDL
* SDL_image
* SDL_mixer
* SDL_ttf
* SDL_gfx

Gamepad Notes
-------------
Your gamepad or joystick may not work by default with Open Syobon Action. Check joyconfig.h for more information.

Unimplemented
-------------
* Font thickness (Not really important)
* Port to other operating systems (OS X, iOS, Android, etc.)

Release Changelog
-----------------
**Open Syobon Action RC 3**
* Fixed audio on Windows 7 and above

**Open Syobon Action RC 2**
* First Windows release!
* Dvorak keyboard support
* Error messages more detailed and directed to stdout
* Compiled code optimized for speed
* Better initialization and de-initialization code, so you can be confident
 that all resources are freed.
* Optional sound: The game will no longer crash if there is no audio device
 available, and all sound can be disabled with the -nosound parameter
* Minor bugfixes here and there

**Open Syobon Action RC 1**
* Fixed incorrect "box" rendering (used to draw green pipes)
* Fixed random crashing at certain parts (thanks to mekuso for the bug report)
* Volumes now appropriately altered
* Gamepad support added
* Now uses original indexed PNGs, with original filenames (capital PNG)
* Minor tweaks and optimizations

**Open Syobon Action v0.9:**
* Fixed incorrect character rendering, uses font very similar to the original game
 (sazanami-gothic.ttf rather than msgothic.ttf)
* Changed encoding of the original readme and source code to UTF-8, so they can
 be viewed correctly with most software
* Changed DOS newlines to Unix newlines in source and readme
* Source code has been cleaned up with "indent", removed a few obsolete comments

**Open Syobon Action v0.8:**
* First release
