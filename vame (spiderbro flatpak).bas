REM Variable Map Engine
REM Build 2.7.9
REM By Danielle Pond

REM icon, version info and error handler
ON ERROR GOTO errorhandler
LET consolelog$ = "/var/data/data/consolelog.txt": REM sets console log file location
$VERSIONINFO:CompanyName=STUDIO_POND
$VERSIONINFO:ProductName=VaME
$VERSIONINFO:FileDescription=Variable Map Engine
$VERSIONINFO:InternalName=VaME
$VERSIONINFO:FILEVERSION#=2,7,9,2709
$VERSIONINFO:PRODUCTVERSION#=2,7,9,2709
$EXEICON:'data\icon.ico'
_ICON
LET hardbuild$ = "2.7.9"

setup:
REM initiates engine and assigns values
LET setupboot = 1: REM sets value for engine booting
_ALLOWFULLSCREEN _OFF: REM block alt-enter
REM check os
IF INSTR(_OS$, "[WINDOWS]") THEN LET ros$ = "win"
IF INSTR(_OS$, "[LINUX]") THEN LET ros$ = "lnx"
IF INSTR(_OS$, "[MACOSX]") THEN LET ros$ = "mac"
REM check metadata exists, checks developer console settings and load engine values
IF _FILEEXISTS("/var/data/data/engine.ddf") THEN
    OPEN "/var/data/data/engine.ddf" FOR INPUT AS #1
    INPUT #1, devmode, consolelogging, displayconsole, autoupdate, installtype, title$, filename$, totalobjects, totalplayers, totaltriggers, totalpockets, totalcheckpoints, totalframes, totalsfxs, totalmusics, resx, resy, hertz, exitsave, autotxtsfx, ucontrol, dcontrol, lcontrol, rcontrol, scontrol, pcontrol, bcontrol, enableobjectoffsets, enableplayeroffsets, enablemapoffsets, fadespeed, pace, objectstep, collisionstep, playeridle, footpace, headerfontname$, headerfontsize, headerfontstyle$, defaultfontname$, defaultfontsize, defaultfontstyle$, smallfontname$, smallfontsize, smallfontstyle$, imode, playerwalkdivide, scriptwalkdivide, scriptimage$, scriptimageresx, scriptimageresy, pockethudimage$, pockethudresx, pockethudresy, pocketarrowright$, pocketarrowleft$, pocketarrowselectright$, pocketarrowselectleft$, pocketarrowunavailableright$, pocketarrowunavailableleft$, pocketarrowresx, pocketarrowresy, pockethudanispeed, pocketarrowrlocx, pocketarrowrlocy, pocketarrowllocx, pocketarrowllocy, pocketspritex, pocketspritey, pocketspriteresx, pocketspriteresy, pocketbanner$, pocketbannerresx, pocketbannerresy, textbannersound, textbanner$, textbannername$, textbannerresx, textbannerresy, pocketselect$, pocketselectx, pocketselecty, pocketselectresx, pocketselectresy, lookaction$, lookx, useaction$, giveaction$, combineaction$, usex, givex, combinex, textbannerfacey, textbannerfaceresx, textbannerfaceresy, tos$, tdelay, stposx, stposy, tanidelay, terminalcol1, terminalcol2, terminalcol3, terminalrow1, terminalrow2, terminalfacex, terminalfacey, currencyname$, loadicon$, loadiconresx, loadiconresy, torcheffectfile$, loadbar$, devlogo$, devlogomode, versionno$, engineversionno$, updatelink$
    CLOSE #1
    IF ros$ = "win" THEN
        REM finds metadata directory paths (windoze)
        IF _FILEEXISTS("data\filelocwin.ddf") THEN
            OPEN "data\filelocwin.ddf" FOR INPUT AS #1
            INPUT #1, dloc$, mloc$, ploc$, floc$, sloc$, oloc$, scriptloc$, museloc$, sfxloc$, pocketloc$, uiloc$, tloc$, aloc$, menuloc$
            CLOSE #1
        ELSE
            ERROR 420
        END IF
    ELSE
        REM finds metadata directory paths (mac + linux)
        IF _FILEEXISTS("/var/data/data/filelocother.ddf") THEN
            OPEN "/var/data/data/filelocother.ddf" FOR INPUT AS #1
            INPUT #1, dloc$, mloc$, ploc$, floc$, sloc$, oloc$, scriptloc$, museloc$, sfxloc$, pocketloc$, uiloc$, tloc$, aloc$, menuloc$
            CLOSE #1
        ELSE
            ERROR 420
        END IF
    END IF
    REM loads colours
    IF _FILEEXISTS("/var/data/data/colours.ddf") THEN
        OPEN "/var/data/data/colours.ddf" FOR INPUT AS #1
        INPUT #1, letmenuselectcolourr, letmenuselectcolourg, letmenuselectcolourb, letmenuselectcoloura, bgmenuselectcolourr, bgmenuselectcolourg, bgmenuselectcolourb, bgmenuselectcoloura, letmenudefaultcolourr, letmenudefaultcolourg, letmenudefaultcolourb, letmenudefaultcoloura, bgmenudefaultcolourr, bgmenudefaultcolourg, bgmenudefaultcolourb, bgmenudefaultcoloura, letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura, bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura, letpocketselectcolourr, letpocketselectcolourg, letpocketselectcolourb, letpocketselectcoloura, bgpocketselectcolourr, bgpocketselectcolourg, bgpocketselectcolourb, bgpocketselectcoloura, letpocketdefaultcolourr, letpocketdefaultcolourg, letpocketdefaultcolourb, letpocketdefaultcoloura, bgpocketdefaultcolourr, bgpocketdefaultcolourg, bgpocketdefaultcolourb, bgpocketdefaultcoloura, letcurrencycolourr, letcurrencycolourg, letcurrencycolourb, letcurrencycoloura, bgcurrencycolourr, bgcurrencycolourg, bgcurrencycolourb, bgcurrencycoloura, letspeechcolourr, letspeechcolourg, letspeechcolourb, letspeechcoloura, bgspeechcolourr, bgspeechcolourg, bgspeechcolourb, bgspeechcoloura, letterminalcolourr, letterminalcolourg, letterminalcolourb, letterminalcoloura, bgterminalcolourr, bgterminalcolourg, bgterminalcolourb, bgterminalcoloura
        CLOSE #1
        _PRINTMODE _FILLBACKGROUND
    ELSE
        ERROR 420
    END IF
    $CONSOLE
    IF displayconsole = 1 THEN
        _CONSOLE ON
        IF title$ <> "" THEN
            _CONSOLETITLE title$ + " Console"
        ELSE
            _CONSOLETITLE "VaME Console"
        END IF
    END IF
    IF displayconsole = 0 THEN _CONSOLE OFF
    REM reports system info to console
    GOSUB consoleboot: REM announces system boot to consolelog.txt
    LET eventtitle$ = "OPERATING SYSTEM DETECTED:"
    IF ros$ = "win" THEN LET eventdata$ = "Microsoft Windows"
    IF ros$ = "lnx" THEN LET eventdata$ = "Linux"
    IF ros$ = "mac" THEN LET eventdata$ = "Apple macOS"
    LET eventnumber = 0
    GOSUB consoleprinter
    LET eventtitle$ = "LOADED METADATA:"
    LET eventdata$ = dloc$ + "engine.ddf"
    LET eventnumber = 0
    GOSUB consoleprinter
ELSE
	IF usersetup = 0 THEN
		PRINT "Initialising..."
		SHELL _HIDE "cp -R data $XDG_DATA_HOME"
		SHELL _HIDE "cp -R polydata $XDG_DATA_HOME"
		LET usersetup = 1
		CLS
		GOTO setup 
	ELSE
		BEEP
		PRINT "FLATPAK ERROR!"
		ERROR 420
	END IF
END IF
IF title$ = "" THEN LET title$ = "VaME": REM sets program name if none exists
REM report game title and engine info to console...
LET eventtitle$ = "ENGINE VERSION NUMBER:"
LET eventdata$ = hardbuild$
LET eventnumber = 0
GOSUB consoleprinter
LET eventtitle$ = "GAME DATA FOUND:"
LET eventdata$ = title$
LET eventnumber = 0
GOSUB consoleprinter
LET eventtitle$ = "GAME VERSION NUMBER:"
LET eventdata$ = versionno$
LET eventnumber = 0
GOSUB consoleprinter
REM checks build versions match, checks for developer build
LET finddev% = INSTR(finddev% + 1, engineversionno$, "DEV")
IF finddev% THEN LET hardbuild$ = hardbuild$ + "DEV": LET finddev% = 0
IF engineversionno$ <> hardbuild$ THEN ERROR 427
REM check if remaining metadata directories exist
LET temp6 = 0
DO
    LET temp6 = temp6 + 1
    IF temp6 = 1 THEN LET temp3$ = dloc$
    IF temp6 = 2 THEN LET temp3$ = ploc$
    IF temp6 = 3 THEN LET temp3$ = mloc$
    IF temp6 = 4 THEN LET temp3$ = floc$
    IF temp6 = 5 THEN LET temp3$ = sloc$
    IF temp6 = 6 THEN LET temp3$ = oloc$
    IF temp6 = 7 THEN LET temp3$ = scriptloc$
    IF temp6 = 8 THEN LET temp3$ = museloc$
    IF temp6 = 9 THEN LET temp3$ = sfxloc$
    IF temp6 = 10 THEN LET temp3$ = pocketloc$
    IF temp6 = 11 THEN LET temp3$ = uiloc$
    IF temp6 = 12 THEN LET temp3$ = tloc$
    IF temp6 = 13 THEN LET temp3$ = aloc$
    IF temp6 = 14 THEN LET temp3$ = menuloc$
    IF _DIREXISTS(temp3$) THEN
        LET eventtitle$ = "DIRECTORY ACTIVE:"
        LET eventdata$ = temp3$
        LET eventnumber = 0
        GOSUB consoleprinter
    ELSE
        ERROR 421: REM error if directory unavailable
    END IF
LOOP UNTIL temp6 = 14
GOSUB dimmer: REM assigns array values
GOSUB parameterload: REM loads any launch parameters
GOSUB deleteupdaters: REM deletes any left over updater files
GOSUB inputload: REM checks and informs console of enabled game controls
GOSUB saveload: REM load savedata values
GOSUB screenload: REM sets screen and resolution settings
GOSUB uiload: REM loads misc items into memory for quick access later
GOSUB loadbar: REM display loading bar
GOSUB fontload: REM loads font
GOSUB setdefaultfont: REM sets default font
GOSUB musicload: REM loads music files into memory for quick access later
GOSUB sfxload: REM loads sound effect files into memory for quick access later
GOSUB pocketload: REM loads pocket files into memory for quick access later
GOSUB terminalload: REM loads terminal files into memory for quick access later
IF fixvame = 1 AND noupdate = 1 THEN ERROR 426: REM error for conflicting parameters
IF noupdate = 0 AND erasesaveonly = 0 AND savedisplay = 0 THEN GOSUB updatechecker: REM checks the internet for updates
REM displays developer logo
IF savedisplay = 0 THEN GOSUB devlogo
IF erasesaveonly = 1 THEN GOTO erasesave
IF savedisplay = 1 THEN GOSUB displaysaveerase
REM directs to mainmenu
LET menu$ = "mainmenu"
GOSUB menugenerator
REM setup timer
RANDOMIZE TIMER
LET itime = TIMER: REM timer function
LET ctime = 0: REM timer function
GOSUB mainplayerload: REM loads player data
GOSUB mapload: REM loads map data
REM scrub temporary values
LET temp6 = 0: LET temp3$ = "": LET setupboot = 0
GOTO game

displaysaveerase:
REM displays save erased message
CLS
LET textspeech$ = "Savedata erased!
GOSUB textbannerdraw
LET savedisplay = 0
RETURN

dimmer:
REM assigns array values
REM map object values
DIM objectname(totalobjects) AS STRING
DIM objectx(totalobjects) AS DOUBLE
DIM objecty(totalobjects) AS DOUBLE
DIM objects(totalobjects) AS INTEGER
DIM objectl(totalobjects) AS INTEGER
DIM objectoffset(totalobjects) AS SINGLE
DIM objectresx(totalobjects) AS INTEGER
DIM objectresy(totalobjects) AS INTEGER
DIM objecta(totalobjects) AS INTEGER
DIM objectb(totalobjects) AS INTEGER
DIM findobject(totalobjects) AS INTEGER
REM map player values
DIM playername(totalplayers) AS STRING
DIM playerx(totalplayers) AS DOUBLE
DIM playery(totalplayers) AS DOUBLE
DIM mplayerx(totalplayers) AS INTEGER
DIM mplayery(totalplayers) AS INTEGER
DIM playergrace(totalplayers) AS INTEGER
DIM playerdefault(totalplayers) AS INTEGER
DIM playerresx(totalplayers) AS INTEGER
DIM playerresy(totalplayers) AS INTEGER
DIM players(totalplayers) AS INTEGER
DIM playernote1(totalplayers) AS INTEGER
DIM playernote2(totalplayers) AS INTEGER
DIM carryplayerd(totalplayers) AS INTEGER
DIM carryplayerjourney(totalplayers) AS INTEGER
DIM carryplayerx(totalplayers) AS INTEGER
DIM carryplayery(totalplayers) AS INTEGER
DIM carryplayerlayer(totalplayers) AS INTEGER
DIM carryplayerperiod(totalplayers) AS INTEGER
DIM dplayerx(totalplayers) AS INTEGER
DIM dplayery(totalplayers) AS INTEGER
DIM playerd(totalplayers) AS INTEGER
DIM playerjourney(totalplayers) AS INTEGER
DIM playeroffset(totalplayers) AS SINGLE
DIM playerperiod(totalplayers) AS INTEGER
DIM playerscript(totalplayers) AS INTEGER
DIM playerwalking(totalplayers) AS INTEGER
DIM pfootloop(totalplayers) AS INTEGER
DIM pfoot(totalplayers) AS INTEGER
DIM findplayer(totalplayers) AS INTEGER
DIM playerf(totalplayers) AS INTEGER
DIM playerb(totalplayers) AS INTEGER
DIM playerr(totalplayers) AS INTEGER
DIM playerl(totalplayers) AS INTEGER
DIM playerfl(totalplayers) AS INTEGER
DIM playerfr(totalplayers) AS INTEGER
DIM playerbl(totalplayers) AS INTEGER
DIM playerbr(totalplayers) AS INTEGER
DIM playerrl(totalplayers) AS INTEGER
DIM playerrr(totalplayers) AS INTEGER
DIM playerll(totalplayers) AS INTEGER
DIM playerlr(totalplayers) AS INTEGER
DIM playerfi1(totalplayers) AS INTEGER
DIM playerfi2(totalplayers) AS INTEGER
DIM playerbi1(totalplayers) AS INTEGER
DIM playerbi2(totalplayers) AS INTEGER
DIM playerli1(totalplayers) AS INTEGER
DIM playerli2(totalplayers) AS INTEGER
DIM playerri1(totalplayers) AS INTEGER
DIM playerri2(totalplayers) AS INTEGER
DIM playerface1(totalplayers) AS INTEGER
DIM playerface2(totalplayers) AS INTEGER
DIM playerlayer(totalplayers) AS INTEGER
REM sfx values
DIM sfx(totalsfxs) AS STRING
DIM sfxdata(totalsfxs) AS INTEGER
REM music values
DIM music(totalmusics) AS STRING
DIM musicdata(totalmusics) AS INTEGER
REM map trigger values
DIM triggername(totaltriggers) AS STRING
DIM triggerx1(totaltriggers) AS INTEGER
DIM triggery1(totaltriggers) AS INTEGER
DIM triggerx2(totaltriggers) AS INTEGER
DIM triggery2(totaltriggers) AS INTEGER
DIM triggera(totaltriggers) AS INTEGER
REM pocket values
DIM pocketname(totalpockets) AS STRING
DIM pocketshort(totalpockets) AS STRING
DIM pocketdescription(totalpockets) AS STRING
DIM pocketitem(totalpockets) AS INTEGER
DIM pocketsprite(totalpockets) AS INTEGER
REM checkpoint values
DIM checkpoint(totalcheckpoints) AS INTEGER
REM animation values
DIM frame(totalframes) AS INTEGER
DIM aniframe(totalframes) AS INTEGER
REM print to console
LET eventtitle$ = "OBJECT ARRAY LIMIT:"
LET eventdata$ = ""
LET eventnumber = totalobjects
GOSUB consoleprinter
LET eventtitle$ = "PLAYER ARRAY LIMIT:"
LET eventdata$ = ""
LET eventnumber = totalplayers
GOSUB consoleprinter
LET eventtitle$ = "TRIGGER ARRAY LIMIT:"
LET eventdata$ = ""
LET eventnumber = totaltriggers
GOSUB consoleprinter
LET eventtitle$ = "POCKET ARRAY LIMIT:"
LET eventdata$ = ""
LET eventnumber = totalpockets
GOSUB consoleprinter
LET eventtitle$ = "CHECKPOINT ARRAY LIMIT:"
LET eventdata$ = ""
LET eventnumber = totalcheckpoints
GOSUB consoleprinter
LET eventtitle$ = "ANIMATION FRAME ARRAY LIMIT:"
LET eventdata$ = ""
LET eventnumber = totalframes
GOSUB consoleprinter
LET eventtitle$ = "MUSIC ARRAY LIMIT:"
LET eventdata$ = ""
LET eventnumber = totalmusics
GOSUB consoleprinter
LET eventtitle$ = "SOUND EFFECT ARRAY LIMIT:"
LET eventdata$ = ""
LET eventnumber = totalsfxs
GOSUB consoleprinter
LET eventtitle$ = "ARRAY VALUES ASSIGNED"
LET eventdata$ = ""
LET eventnumber = 0
GOSUB consoleprinter
RETURN

displayspeedrun:
REM displays speedrun time
IF mainmenu = 1 THEN RETURN
COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura)
IF speedrun = 1 THEN
	_PRINTSTRING(0, 0), STR$(speedrunhour) + ":" + STR$(speedrunmin) + ":" + STR$(speedrunsec)
ELSE
	_PRINTSTRING(0, 0), "RUN COMPLETE!"
	_PRINTSTRING(0, 10), STR$(speedrunhour) + ":" + STR$(speedrunmin) + ":" + STR$(speedrunsec)
END IF
COLOR 0, 0
RETURN

generateoffsets:
REM generates random map animation offsets
REM objects
IF enableobjectoffsets = 1 THEN
    DO
        LET x = x + 1
        LET objectoffset(x) = RND
    LOOP UNTIL x >= mapobjectno OR x >= totalobjects
    IF mapobjectno <> 0 THEN
        LET eventtitle$ = "OFFSETS GENERATED:"
        LET eventdata$ = "objects"
        LET eventnumber = mapobjectno
        GOSUB consoleprinter
    END IF
END IF
LET x = 0
REM players
IF enableplayeroffsets = 1 THEN
    DO
        LET x = x + 1
        LET playeroffset(x) = RND
    LOOP UNTIL x >= mapplayerno OR x >= totalplayers
    IF mapplayerno <> 0 THEN
        LET eventtitle$ = "OFFSETS GENERATED:"
        LET eventdata$ = "players"
        LET eventnumber = mapplayerno
        GOSUB consoleprinter
    END IF
END IF
REM map
IF enablemapoffsets = 1 THEN
    LET mapanioffset = RND
    LET eventtitle$ = "OFFSET GENERATED:"
    LET eventdata$ = "map"
    LET eventnumber = mapno
    GOSUB consoleprinter
END IF
LET x = 0: REM scrub temp values
RETURN

inputload:
REM checks and informs console of enabled inputs
DO
    LET temp139 = temp139 + 1
    LET eventtitle$ = "INPUT CONRTOL:"
    IF temp139 = 1 THEN
        IF ucontrol = 1 THEN
            LET eventdata$ = "up control enabled"
        ELSE
            LET eventdata$ = "up control disabled"
        END IF
    END IF
    IF temp139 = 2 THEN
        IF dcontrol = 1 THEN
            LET eventdata$ = "down control enabled"
        ELSE
            LET eventdata$ = "down control disabled"
        END IF
    END IF
    IF temp139 = 3 THEN
        IF lcontrol = 1 THEN
            LET eventdata$ = "left control enabled"
        ELSE
            LET eventdata$ = "left control disabled"
        END IF
    END IF
    IF temp139 = 4 THEN
        IF rcontrol = 1 THEN
            LET eventdata$ = "right control enabled"
        ELSE
            LET eventdata$ = "right control disabled"
        END IF
    END IF
    IF temp139 = 5 THEN
        IF scontrol = 1 THEN
            LET eventdata$ = "select control enabled"
        ELSE
            LET eventdata$ = "select control disabled"
        END IF
    END IF
    IF temp139 = 6 THEN
        IF pcontrol = 1 THEN
            LET eventdata$ = "pocket control enabled"
        ELSE
            LET eventdata$ = "pocket control disabled"
        END IF
    END IF
    IF temp139 = 7 THEN
        IF bcontrol = 1 THEN
            LET eventdata$ = "back control enabled"
        ELSE
            LET eventdata$ = "back control disabled"
        END IF
    END IF
    LET eventnumber = temp139
    GOSUB consoleprinter
LOOP UNTIL temp139 = 7
LET temp139 = 0: REM scrubs temp values
RETURN

deleteupdaters:
REM deletes any updater files if needed
IF _FILEEXISTS(LCASE$(title$) + "updater_win.exe") THEN
    REM windoze updater
    SHELL _HIDE "del "+ LCASE$(title$) + "updater_win.exe"
    LET temp132 = 1
END IF
IF _FILEEXISTS(LCASE$(title$) + "updater_linux") THEN
    REM linux updater
    SHELL _HIDE "rm " + LCASE$(title$) + "updater_linux"
    LET temp132 = 1
END IF
IF _FILEEXISTS(LCASE$(title$) + "updater_macos") THEN
    REM macos updater
    SHELL _HIDE "rm " + LCASE$(title$) + "updater_macos"
    LET temp132 = 1
END IF
IF _FILEEXISTS("windownloader.bat") THEN
    REM download batch file for windoze
    SHELL _HIDE "del windownloader.bat"
    LET temp132 = 1
END IF
IF _FILEEXISTS("checkupdate.ddf") THEN
    REM latest update metadata
    IF ros$ = "win" THEN
        SHELL _HIDE "del checkupdate.ddf"
    ELSE
        SHELL _HIDE "rm checkupdate.ddf"
    END IF
    LET temp132 = 1
END IF
IF _FILEEXISTS(LCASE$(title$) + "update.zip") THEN
    REM compressed update files
    IF ros$ = "win" THEN
        SHELL _HIDE "del " + LCASE$(title$) + "update.zip"
    ELSE
        SHELL _HIDE "rm " + LCASE$(title$) + "update.zip"
    END IF
    LET temp132 = 1
END IF
IF _FILEEXISTS("unzip.exe") THEN
    REM uncompressor for windoze
    SHELL _HIDE "del unzip.exe"
    LET temp132 = 1
END IF
IF ros$ <> "mac" THEN
    IF _DIREXISTS("__MACOSX") THEN
        REM macos metadata folder
        IF ros$ = "win" THEN SHELL _HIDE "rmdir /Q /S __MACOSX"
        IF ros$ = "lnx" THEN SHELL _HIDE "rm -R __MACOSX"
        LET temp132 = 1
    END IF
END IF
IF temp132 = 1 THEN
    REM prints to console
    LET eventtitle$ = "UPDATER FILE DELETED"
    LET eventdata$ = ""
    LET eventnumber = 0
    GOSUB consoleprinter
END IF
LET temp132 = 0
RETURN

parameterload:
REM loads and applies any launch parameters
IF _COMMANDCOUNT = 0 THEN RETURN: REM return for if no parameters
DO
    LET temp129 = temp129 + 1
    LET parameter$ = COMMAND$(temp129)
    IF UCASE$(parameter$) = "-LITE" THEN LET liteload = 1: LET soundmode = 1: LET temp130 = 1
    IF UCASE$(parameter$) = "-NOUPDATE" THEN LET noupdate = 1: LET temp130 = 1
    IF UCASE$(parameter$) = "-DEVMODE" THEN LET devmode = 1: LET temp130 = 1
    IF UCASE$(parameter$) = "-CONSOLE" THEN LET displayconsole = 1: LET temp130 = 1
    IF UCASE$(parameter$) = "-ERASESAVE" THEN LET erasesaveonly = 1: LET temp130 = 1
    IF UCASE$(parameter$) = "-NOSAVE" THEN LET nosave = 1: LET temp130 = 1
    IF UCASE$(parameter$) = "-NOFX" THEN LET disablefade = 1: LET temp130 = 1
    IF UCASE$(parameter$) = "-FIX" THEN LET fixvame = 1: LET temp130 = 1
    IF UCASE$(parameter$) = "-SPEEDRUN" THEN LET speedrun = 1: LET temp130 = 1
    IF UCASE$(parameter$) = "-SPECIAL101" THEN LET savedisplay = 1: LET temp130 = 1
    IF temp130 = 1 THEN
        LET eventtitle$ = "PARAMETER LOADED:"
        LET eventdata$ = parameter$
        LET eventnumber = 0
        GOSUB consoleprinter
    ELSE
        LET eventtitle$ = "INVALID PARAMETER:"
        LET eventdata$ = parameter$
        LET eventnumber = 0
        GOSUB consoleprinter
    END IF
    LET temp130 = 0
LOOP UNTIL temp129 = _COMMANDCOUNT
LET temp129 = 0: LET temp130 = 0: REM scrub temp values
RETURN

devlogo:
REM developer logo
REM play sound (if needed)
IF devlogomode = 1 THEN
    LET playsfx$ = "devlogo"
    GOSUB sfxplay
END IF
FOR i% = 255 TO 0 STEP -5
    _LIMIT fadespeed: REM sets framerate
    _PUTIMAGE (0, 0)-(resx, resy), devlogo
    LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, i%), BF: REM slowly fills screen with black box
    GOSUB timeframecounter: REM timer function
    _DISPLAY
NEXT
_AUTODISPLAY
_PUTIMAGE (0, 0)-(resx, resy), devlogo
REM play sound (if needed)
IF devlogomode = 2 THEN
    LET playsfx$ = "devlogo"
    GOSUB sfxplay
END IF
DO
    LET temp128 = temp128 + 1
    _DELAY 1
LOOP UNTIL INKEY$ = " " OR temp128 = 3
LET eventtitle$ = "DEVELOPER LOGO DISPLAYED"
LET eventdata$ = ""
LET eventnumber = 0
GOSUB consoleprinter
FOR i% = 0 TO 255 STEP 5
    _LIMIT fadespeed: REM sets framerate
    _PUTIMAGE (0, 0)-(resx, resy), devlogo
    LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, i%), BF: REM slowly empties black box from screen
    _DISPLAY
NEXT
_AUTODISPLAY
LET temp128 = 0: REM scrub temp values
RETURN

loadbar:
REM displays loading gfx
LET temp126 = resx - (loadiconresx + 5)
LET temp125 = temp125 / 100
LET temp127 = temp125 * temp126
_PUTIMAGE (1, 1)-(loadiconresx + 1, loadiconresy + 1), loadicon: REM displays load icon
_PUTIMAGE (loadiconresx + 5, 1)-((loadiconresx + 5) + temp127, loadiconresy + 1), loadbar: REM displays load bar
LET temp125 = temp125 * 100
RETURN

pocketload:
REM loads pocket items into memory
REM open file list
OPEN pocketloc$ + "pocketfiles.ddf" FOR INPUT AS #1
REM load sprites and metadata
DO
    LET temp57 = temp57 + 1
    INPUT #1, pocketfile$: REM reads name of pocket file to be loaded
    OPEN pocketloc$ + pocketfile$ + "/" + pocketfile$ + ".ddf" FOR INPUT AS #666
    REM loads pocket files and assigns them a slot
    INPUT #666, pocketname(temp57), pocketdescription(temp57): LET pocketsprite(temp57) = _LOADIMAGE(pocketloc$ + pocketfile$ + "/" + pocketfile$ + ".png"): LET pocketshort(temp57) = pocketfile$
    CLOSE #666
    REM prints load to console
    LET eventtitle$ = "POCKET ITEM LOADED:"
    LET eventdata$ = pocketfile$
    LET eventnumber = temp57
    GOSUB consoleprinter
    REM updates loadbar
    IF setupboot = 1 THEN
        LET temp125 = temp125 + 1
        GOSUB loadbar
    END IF
LOOP UNTIL EOF(1) OR temp57 >= totalpockets
LET pocketnos = temp57: REM set pocketnos
IF setupboot = 1 THEN
    REM load bar
    LET temp125 = 90
    GOSUB loadbar
END IF
CLOSE #1
LET temp57 = 0: REM scrubs temp values
RETURN

pocketunload:
REM unloads all pocket files from memory
OPEN pocketloc$ + "pocketfiles.ddf" FOR INPUT AS #1
DO
    LET temp58 = temp58 + 1
    INPUT #1, pocketfile$
    _FREEIMAGE pocketsprite(temp58)
    REM prints unload to console
    LET eventtitle$ = "POCKET ITEM UNLOADED:"
    LET eventdata$ = pocketfile$
    LET eventnumber = temp58
    GOSUB consoleprinter
LOOP UNTIL EOF(1) OR temp58 >= totalpockets
CLOSE #1
LET temp58 = 0: REM scrub temp values
RETURN

pocketdraw:
REM draws pocket interface and provides input
REM return for if no items in pocket
IF pocketcarry = 0 THEN
    LET textspeech$ = "I have nothing in my pocket!"
    GOSUB textbannerdraw
    RETURN
END IF
REM sets up pocket
GOSUB slightfadeout: REM dims screen
COLOR _RGBA(letpocketdefaultcolourr, letpocketdefaultcolourg, letpocketdefaultcolourb, letpocketdefaultcoloura), _RGBA(bgpocketdefaultcolourr, bgpocketdefaultcolourg, bgpocketdefaultcolourb, bgpocketdefaultcoloura)
REM sets values
LET pocketline = 0
LET pocketdisplay = 0
REM plays sfx
LET playsfx$ = "openpocket"
GOSUB sfxplay
REM pocket animation
DO
    _LIMIT pockethudanispeed
    _PUTIMAGE (0, (0 - temp59))-(pockethudresx - 1, temp59), pockethud
    LET temp59 = temp59 + 1
LOOP UNTIL temp59 >= pockethudresy
IF mainmenu = 0 THEN
    DO
        _LIMIT pockethudanispeed
        _PUTIMAGE (((resx / 2) - (pocketbannerresx / 2)), pockethudresy)-(((resx / 2) + (pocketbannerresx / 2)), (temp59 + 1)), pocketbanner
        LET temp59 = temp59 + 1
    LOOP UNTIL temp59 >= (pockethudresy + pocketbannerresy + 1)
END IF
REM prints pockets active to console
LET eventtitle$ = "POCKETS:"
LET eventdata$ = "ACTIVE!"
LET pocketon = 1
LET eventnumber = 0
GOSUB consoleprinter
DO
    REM sets values
    LET temp60 = 0
    IF pocketdisplay = 0 THEN GOSUB pocketcalcup
    REM draws pocket
    _PUTIMAGE (0, 0)-(pockethudresx - 1, pockethudresy), pockethud
    _PUTIMAGE (pocketarrowrlocx, pocketarrowrlocy)-((pocketarrowrlocx + pocketarrowresx), (pocketarrowrlocy + pocketarrowresy)), pocketarrowr
    _PUTIMAGE (pocketarrowllocx, pocketarrowllocy)-((pocketarrowllocx + pocketarrowresx), (pocketarrowllocy + pocketarrowresy)), pocketarrowl
    _PUTIMAGE (((resx / 2) - (pocketbannerresx / 2)), (pockethudresy + 1))-(((resx / 2) + (pocketbannerresx / 2)), (pocketbannerresy + pockethudresy + 1)), pocketbanner
    _KEYCLEAR
    DO
		_LIMIT 1000
        LET b$ = UCASE$(INKEY$): REM inputter
        REM displays pocket image
        _PUTIMAGE (pocketspritex, pocketspritey)-((pocketspritex + pocketspriteresx), (pocketspritey + pocketspriteresy)), pocketsprite(pocketdisplay)
        REM calculates what text to display
        LET pocketdisplayname$ = pocketname(pocketdisplay): LET pocketdisplaydescription$ = pocketdescription(pocketdisplay): LET currentpocketshort$ = pocketshort(pocketdisplay)
        REM adds value to currency
        IF currentpocketshort$ = "currency" THEN LET temp22$ = pocketdisplayname$: LET pocketdisplayname$ = pocketdisplayname$ + " " + STR$(currency)
        REM calculates centre
        LET centretext$ = pocketdisplayname$
        GOSUB centretext
        REM displays pocket item info text
        COLOR _RGBA(letpocketdefaultcolourr, letpocketdefaultcolourg, letpocketdefaultcolourb, letpocketdefaultcoloura), _RGBA(bgpocketdefaultcolourr, bgpocketdefaultcolourg, bgpocketdefaultcolourb, bgpocketdefaultcoloura)
        _PRINTSTRING ((resx / 2) - (centreno / 2), (pockethudresy)), pocketdisplayname$
        REM removes value from currency
        IF currentpocketshort$ = "currency" THEN LET pocketdisplayname$ = temp22$
        IF speedrun > 0 THEN GOSUB timeframecounter: GOSUB displayspeedrun: COLOR _RGBA(letpocketdefaultcolourr, letpocketdefaultcolourg, letpocketdefaultcolourb, letpocketdefaultcoloura), _RGBA(bgpocketdefaultcolourr, bgpocketdefaultcolourg, bgpocketdefaultcolourb, bgpocketdefaultcoloura)
    LOOP UNTIL b$ = "Q" OR b$ = "I" OR b$ = CHR$(0) + CHR$(77) OR b$ = CHR$(0) + CHR$(75) OR b$ = " "
    REM arrow keys divert
    IF speedrun > 0 THEN GOSUB timeframecounter: GOSUB displayspeedrun: COLOR _RGBA(letpocketdefaultcolourr, letpocketdefaultcolourg, letpocketdefaultcolourb, letpocketdefaultcoloura), _RGBA(bgpocketdefaultcolourr, bgpocketdefaultcolourg, bgpocketdefaultcolourb, bgpocketdefaultcoloura)
    IF b$ = CHR$(0) + CHR$(77) THEN 
		REM right key
		LET temp6177 = pocketline
		GOSUB pocketcalcup
		IF temp6177 <> pocketline THEN
			LET playsfx$ = "move"
			GOSUB sfxplay
			GOSUB pocketarrowright
		END IF
		LET temp6177 = 0
	END IF
    IF b$ = CHR$(0) + CHR$(75) THEN 
		REM left key
		LET temp6177 = pocketline
		GOSUB pocketcalcdown
		IF temp6177 <> pocketline THEN
			LET playsfx$ = "move"
			GOSUB sfxplay
			GOSUB pocketarrowleft
		END IF
		LET temp6177 = 0
	END IF
    IF b$ = " " THEN LET playsfx$ = "select": GOSUB sfxplay: GOSUB pocketext
LOOP UNTIL b$ = "Q" OR b$ = "I"
CLOSE #1
IF pocketdivert = 0 THEN
    REM plays sfx
    LET playsfx$ = "closepocket"
    GOSUB sfxplay
END IF
GOSUB slightfadein
CLOSE #1: REM closes pocketfiles.ddf
LET clearscreen = 1: REM sets screen for refresh
REM resets pocket divert
IF pocketdivert = 1 THEN LET pocketdivert = 0
REM scrub values
COLOR 0, 0
LET temp59 = 0: LET temp60 = 0: LET temp66 = 0: LET temp67 = 0: LET pocketon = 0
LET pocketdisplayname$ = "": LET pocketdisplaydescription$ = "": LET b$ = "": LET temp22$ = "": REM scrub temp values
RETURN

pocketext:
REM draws extension to pocket (controls)
REM draws images
_PUTIMAGE (pocketselectx, pocketselecty)-((pocketselectx + pocketselectresx), (pocketselecty + pocketselectresy)), pocketselect
_PUTIMAGE (((resx / 2) - (pocketbannerresx / 2)), (pockethudresy + 1))-(((resx / 2) + (pocketbannerresx / 2)), (pocketbannerresy + pockethudresy + 1)), pocketbanner
_PUTIMAGE (pocketarrowrlocx, pocketarrowrlocy)-((pocketarrowrlocx + pocketarrowresx), (pocketarrowrlocy + pocketarrowresy)), pocketarrowru
_PUTIMAGE (pocketarrowllocx, pocketarrowllocy)-((pocketarrowllocx + pocketarrowresx), (pocketarrowllocy + pocketarrowresy)), pocketarrowlu
LET temp74 = 1
REM text and input
_KEYCLEAR
DO
	_LIMIT 1000
    LET c$ = UCASE$(INKEY$): REM inputter
    IF temp74 = 1 THEN COLOR _RGBA(letpocketselectcolourr, letpocketselectcolourg, letpocketselectcolourb, letpocketselectcoloura), _RGBA(bgpocketselectcolourr, bgpocketselectcolourg, bgpocketselectcolourb, bgpocketselectcoloura)
    IF temp74 = 2 THEN COLOR _RGBA(letpocketdefaultcolourr, letpocketdefaultcolourg, letpocketdefaultcolourb, letpocketdefaultcoloura), _RGBA(bgpocketdefaultcolourr, bgpocketdefaultcolourg, bgpocketdefaultcolourb, bgpocketdefaultcoloura)
    _PRINTSTRING ((lookx), (pockethudresy)), lookaction$
    IF temp74 = 2 THEN COLOR _RGBA(letpocketselectcolourr, letpocketselectcolourg, letpocketselectcolourb, letpocketselectcoloura), _RGBA(bgpocketselectcolourr, bgpocketselectcolourg, bgpocketselectcolourb, bgpocketselectcoloura)
    IF temp74 = 1 THEN COLOR _RGBA(letpocketdefaultcolourr, letpocketdefaultcolourg, letpocketdefaultcolourb, letpocketdefaultcoloura), _RGBA(bgpocketdefaultcolourr, bgpocketdefaultcolourg, bgpocketdefaultcolourb, bgpocketdefaultcoloura)
    IF objecttype$ = "OBJ" THEN
        _PRINTSTRING ((usex), (pockethudresy)), useaction$
    END IF
    IF objecttype$ = "NPC" THEN
        _PRINTSTRING ((givex), (pockethudresy)), giveaction$
    END IF
    IF objecttype$ = "" THEN
        _PRINTSTRING ((combinex), (pockethudresy)), combineaction$
    END IF
    IF speedrun > 0 THEN GOSUB timeframecounter: GOSUB displayspeedrun
    IF c$ = CHR$(0) + CHR$(77) THEN 
		IF temp74 = 1 THEN
			LET playsfx$ = "move"
			GOSUB sfxplay 
			LET temp74 = 2
		END IF
	END IF
    IF c$ = CHR$(0) + CHR$(75) THEN 
		IF temp74 = 2 THEN
			LET playsfx$ = "move"
			GOSUB sfxplay
			LET temp74 = 1
		END IF
	END IF
LOOP UNTIL c$ = " " OR c$ = "Q"
IF c$ = " " THEN
    REM play select sound
    LET playsfx$ = "select"
    GOSUB sfxplay
    IF temp74 = 1 THEN
        REM Look at item
        COLOR _RGBA(letpocketdefaultcolourr, letpocketdefaultcolourg, letpocketdefaultcolourb, letpocketdefaultcoloura), _RGBA(bgpocketdefaultcolourr, bgpocketdefaultcolourg, bgpocketdefaultcolourb, bgpocketdefaultcoloura)
        _PRINTSTRING ((lookx), (pockethudresy)), lookaction$
        _DELAY 0.1
        COLOR _RGBA(letpocketselectcolourr, letpocketselectcolourg, letpocketselectcolourb, letpocketselectcoloura), _RGBA(bgpocketselectcolourr, bgpocketselectcolourg, bgpocketselectcolourb, bgpocketselectcoloura)
        _PRINTSTRING ((lookx), (pockethudresy)), lookaction$
        REM checks to see if item needs a script running
        LET lookscript% = INSTR(lookscript% + 1, pocketdisplaydescription$, "[RUNSCRIPT]")
        IF lookscript% THEN
            LET temp30$ = LEFT$(pocketdisplaydescription$, INSTR(pocketdisplaydescription$, " ") - 1)
            LET temp31$ = RIGHT$(pocketdisplaydescription$, LEN(pocketdisplaydescription$) - LEN(temp30$))
            LET temp31$ = LTRIM$(temp31$)
            REM checks to see if script exists
            IF _FILEEXISTS(scriptloc$ + "image/" + temp31$ + ".vsf") THEN
                REM runs script
                LET scriptname$ = temp31$
                LET mapscript = 3
                GOSUB script
                LET pocketdivert = 1
            ELSE
                ERROR 423
            END IF
        ELSE
            LET textspeech$ = pocketdisplaydescription$
            GOSUB textbannerdraw
            LET pocketdivert = 1
        END IF
        LET b$ = "Q": LET lookscript% = 0: LET temp30$ = "": LET temp31$ = "": REM scrub temp values
    END IF
    IF temp74 = 2 THEN
        IF objecttype$ = "NPC" THEN
            REM give item to npc
            COLOR _RGBA(letpocketdefaultcolourr, letpocketdefaultcolourg, letpocketdefaultcolourb, letpocketdefaultcoloura), _RGBA(bgpocketdefaultcolourr, bgpocketdefaultcolourg, bgpocketdefaultcolourb, bgpocketdefaultcoloura)
            _PRINTSTRING ((givex), (pockethudresy)), giveaction$
            _DELAY 0.1
            COLOR _RGBA(letpocketselectcolourr, letpocketselectcolourg, letpocketselectcolourb, letpocketselectcoloura), _RGBA(bgpocketselectcolourr, bgpocketselectcolourg, bgpocketselectcolourb, bgpocketselectcoloura)
            _PRINTSTRING ((givex), (pockethudresy)), giveaction$
            GOSUB usepocket
        END IF
        IF objecttype$ = "OBJ" THEN
            REM use item with world
            COLOR _RGBA(letpocketdefaultcolourr, letpocketdefaultcolourg, letpocketdefaultcolourb, letpocketdefaultcoloura), _RGBA(bgpocketdefaultcolourr, bgpocketdefaultcolourg, bgpocketdefaultcolourb, bgpocketdefaultcoloura)
            _PRINTSTRING ((usex), (pockethudresy)), useaction$
            _DELAY 0.1
            COLOR _RGBA(letpocketselectcolourr, letpocketselectcolourg, letpocketselectcolourb, letpocketselectcoloura), _RGBA(bgpocketselectcolourr, bgpocketselectcolourg, bgpocketselectcolourb, bgpocketselectcoloura)
            _PRINTSTRING ((usex), (pockethudresy)), useaction$
            GOSUB usepocket
        END IF
        IF objecttype$ = "" THEN
            REM use item with item
            COLOR _RGBA(letpocketdefaultcolourr, letpocketdefaultcolourg, letpocketdefaultcolourb, letpocketdefaultcoloura), _RGBA(bgpocketdefaultcolourr, bgpocketdefaultcolourg, bgpocketdefaultcolourb, bgpocketdefaultcoloura)
            _PRINTSTRING ((combinex), (pockethudresy)), combineaction$
            _DELAY 0.1
            COLOR _RGBA(letpocketselectcolourr, letpocketselectcolourg, letpocketselectcolourb, letpocketselectcoloura), _RGBA(bgpocketselectcolourr, bgpocketselectcolourg, bgpocketselectcolourb, bgpocketselectcoloura)
            _PRINTSTRING ((combinex), (pockethudresy)), combineaction$
            GOSUB pocketcombine
        END IF
    END IF
END IF
COLOR _RGBA(letpocketdefaultcolourr, letpocketdefaultcolourg, letpocketdefaultcolourb, letpocketdefaultcoloura), _RGBA(bgpocketdefaultcolourr, bgpocketdefaultcolourg, bgpocketdefaultcolourb, bgpocketdefaultcoloura)
LET temp74 = 0: LET c$ = "": REM scrub temp files
RETURN

pocketcombine:
REM combines pocket items
REM return for if pocket only has one item or less
IF pocketcarry <= 1 THEN
    LET textspeech$ = "I have no pocket items to " + combineaction$ + "!"
    GOSUB textbannerdraw
    LET pocketdivert = 1
    LET b$ = "Q"
    RETURN
END IF
REM sets values
LET temp81 = pocketdisplay
LET temp19$ = pocketdisplayname$
LET temp20$ = pocketdisplaydescription$
LET temp21$ = currentpocketshort$
LET pocketdisplayname$ = ""
LET pocketdisplaydescription$ = ""
LET currentpocketshort$ = ""
LET pocketdisplay = 0
LET pocketline = 0
COLOR _RGBA(letpocketdefaultcolourr, letpocketdefaultcolourg, letpocketdefaultcolourb, letpocketdefaultcoloura), _RGBA(bgpocketdefaultcolourr, bgpocketdefaultcolourg, bgpocketdefaultcolourb, bgpocketdefaultcoloura)
REM redraws poccket
_PUTIMAGE (0, 0)-(pockethudresx, pockethudresy), pockethud
REM displays pocket image
_PUTIMAGE (pocketspritex, pocketspritey)-((pocketspritex + pocketspriteresx), (pocketspritey + pocketspriteresy)), pocketsprite(temp81)
_PUTIMAGE (pocketselectx, pocketselecty)-((pocketselectx + pocketselectresx), (pocketselecty + pocketselectresy)), pocketselect
REM calculates what pocket item to display
DO
    LET temp60 = 0
    LET temp83 = temp83 + 1
    IF pocketdisplay = 0 THEN GOSUB pocketcalcup
    IF pocketdisplay = temp81 THEN
        LET temp84 = 0
        DO
            LET temp84 = temp84 + 1
            IF temp83 = 1 THEN GOSUB pocketcalcup
            IF d$ = CHR$(0) + CHR$(77) THEN
                IF temp84 < 5 THEN GOSUB pocketcalcup
                IF temp84 >= 5 THEN GOSUB pocketcalcdown
            END IF
            IF d$ = CHR$(0) + CHR$(75) THEN
                GOSUB pocketcalcdown
                IF temp84 < 5 THEN GOSUB pocketcalcdown
                IF temp84 >= 5 THEN GOSUB pocketcalcup
            END IF
        LOOP UNTIL pocketdisplay <> temp81
    END IF
    REM draws second pocket
    _PUTIMAGE (0, pockethudresy)-(pockethudresx, pockethudresy + pockethudresy), pockethud
    _PUTIMAGE (pocketarrowrlocx, pocketarrowrlocy + pockethudresy)-((pocketarrowrlocx + pocketarrowresx), (pocketarrowrlocy + pocketarrowresy + pockethudresy)), pocketarrowr
    _PUTIMAGE (pocketarrowllocx, pocketarrowllocy + pockethudresy)-((pocketarrowllocx + pocketarrowresx), (pocketarrowllocy + pocketarrowresy + pockethudresy)), pocketarrowl
    _PUTIMAGE (((resx / 2) - (pocketbannerresx / 2)), (pockethudresy + pockethudresy + 1))-(((resx / 2) + (pocketbannerresx / 2)), (pocketbannerresy + pockethudresy + pockethudresy + 1)), pocketbanner
    _KEYCLEAR
    DO
        LET d$ = UCASE$(INKEY$)
        _PUTIMAGE (pocketspritex, pocketspritey + pockethudresy)-((pocketspritex + pocketspriteresx), (pocketspritey + pocketspriteresy + pockethudresy)), pocketsprite(pocketdisplay)
        REM calculates what text to display
        LET pocketdisplayname$ = pocketname(pocketdisplay): LET pocketdisplaydescription$ = pocketdescription(pocketdisplay): LET currentpocketshort$ = pocketshort(pocketdisplay)
        REM calculates centre
        LET centretext$ = pocketdisplayname$
        GOSUB centretext
        REM prints text to screen
        _PRINTSTRING (1, 1), combineaction$
        _PRINTSTRING (1, 1 + defaultfontsize * 2), "WITH"
        _PRINTSTRING ((resx / 2) - (centreno / 2), (pockethudresy + pockethudresy)), pocketdisplayname$
        IF speedrun > 0 THEN GOSUB timeframecounter: GOSUB displayspeedrun: COLOR _RGBA(letpocketdefaultcolourr, letpocketdefaultcolourg, letpocketdefaultcolourb, letpocketdefaultcoloura), _RGBA(bgpocketdefaultcolourr, bgpocketdefaultcolourg, bgpocketdefaultcolourb, bgpocketdefaultcoloura)
    LOOP UNTIL d$ = "Q" OR d$ = "I" OR d$ = CHR$(0) + CHR$(77) OR d$ = CHR$(0) + CHR$(75) OR d$ = " "
    IF speedrun > 0 THEN GOSUB displayspeedrun: COLOR _RGBA(letpocketdefaultcolourr, letpocketdefaultcolourg, letpocketdefaultcolourb, letpocketdefaultcoloura), _RGBA(bgpocketdefaultcolourr, bgpocketdefaultcolourg, bgpocketdefaultcolourb, bgpocketdefaultcoloura)
    IF d$ = CHR$(0) + CHR$(77) THEN LET playsfx$ = "move": GOSUB sfxplay: GOSUB pocketarrowright: GOSUB pocketcalcup
    IF d$ = CHR$(0) + CHR$(75) THEN LET playsfx$ = "move": GOSUB sfxplay: GOSUB pocketarrowleft: GOSUB pocketcalcdown
    IF d$ = " " THEN LET playsfx$ = "select": GOSUB sfxplay: GOSUB usepocketpocket
LOOP UNTIL d$ = "Q" OR d$ = "I"
LET c$ = "Q": LET b$ = "Q": REM quits inventory
LET pocketdisplay = temp81: LET d$ = "": LET temp81 = 0: LET temp83 = 0: LET temp19$ = "": LET temp20$ = "": LET temp21$ = "": REM scrubs temp values
RETURN

usepocketpocket:
REM COMBINES POCKET ITEMS
LET pocketon = 2
REM checks script file exists
LET scriptname$ = LCASE$(currentpocketshort$) + LCASE$(temp21$)
IF _FILEEXISTS(scriptloc$ + "combine/" + scriptname$ + ".vsf") THEN
    REM first match works
    LET mapscript = 2
    GOSUB script
    LET selectobject$ = ""
ELSE
    REM first match fails
    LET scriptname$ = LCASE$(temp21$) + LCASE$(currentpocketshort$)
    IF _FILEEXISTS(scriptloc$ + "combine/" + scriptname$ + ".vsf") THEN
        REM second match works
        LET mapscript = 2
        GOSUB script
        LET selectobject$ = ""
    ELSE
        REM second match fails
        LET objecttype$ = "OBJ"
        LET textspeech$ = "I can't " + LCASE$(combineaction$) + " my " + temp19$ + " with the " + pocketdisplayname$ + "!"
        GOSUB textbannerdraw
        LET objecttype$ = ""
        LET selectobject$ = ""
    END IF
END IF
LET pocketdivert = 1
LET d$ = "Q": LET c$ = "Q": LET b$ = "Q": REM quits pockets
RETURN

usepocket:
REM USES POCKET ITEM
LET pocketon = 2
REM uses pocket item with world object
LET scriptname$ = LCASE$(selectobject$) + LCASE$(currentpocketshort$)
IF _FILEEXISTS(scriptloc$ + mapdir$ + scriptname$ + ".vsf") THEN
    REM if match does work
    LET mapscript = 1
    GOSUB script
    LET terminalhold$ = currentpocketshort$
ELSE
    REM if match doesn't work
    IF objecttype$ = "OBJ" THEN
        LET textspeech$ = "I can't " + LCASE$(useaction$) + " my " + pocketdisplayname$ + " with that!"
        GOSUB textbannerdraw
    END IF
    IF objecttype$ = "NPC" THEN
        LET scriptname$ = LCASE$(selectobject$) + "nope"
        IF _FILEEXISTS(scriptloc$ + mapdir$ + scriptname$ + ".vsf") THEN
            LET mapscript = 1
            GOSUB script
        ELSE
            ERROR 423
        END IF
    END IF
END IF
LET pocketdivert = 1
LET c$ = "Q": LET b$ = "Q": REM quits pockets
RETURN

givecurrency:
REM gives currency to player
IF currencychange = 0 THEN RETURN: REM return for if there is no currency change
LET currency = currency + currencychange: REM adds money
REM changes font colour
COLOR _RGBA(letcurrencycolourr, letcurrencycolourg, letcurrencycolourb, letcurrencycoloura), _RGBA(bgcurrencycolourr, bgcurrencycolourg, bgcurrencycolourb, bgcurrencycoloura)
REM find
OPEN pocketloc$ + "pocketfiles.ddf" FOR INPUT AS #1
DO
    LET temp93 = temp93 + 1
    INPUT #1, pocketfile$
LOOP UNTIL pocketfile$ = "currency" OR EOF(1)
CLOSE #1
IF silentgive = 0 THEN
    REM animation
    GOSUB slightfadeout
    DO
        REM pockets scroll in
        _LIMIT pockethudanispeed
        _PUTIMAGE (0, (0 - temp94))-(pockethudresx - 1, temp94), pockethud
        LET temp94 = temp94 + 1
    LOOP UNTIL temp94 >= pockethudresy
    LET temp94 = (0 - pocketspriteresx)
    DO
        REM Pocket item scrolls in
        _LIMIT pockethudanispeed
        _PUTIMAGE (0, 0)-(pockethudresx - 1, pockethudresy), pockethud
        _PUTIMAGE (temp94, pocketspritey)-((temp94 + pocketspriteresx), (pocketspritey + pocketspriteresy)), pocketsprite(temp93)
        _PRINTSTRING (temp94 - (pocketspriteresx / 2), pocketspritey), STR$(currencychange)
        LET temp94 = temp94 + 1
        IF speedrun > 0 THEN GOSUB timeframecounter: GOSUB displayspeedrun
    LOOP UNTIL temp94 >= pocketspritex
    REM plays sound effect
    LET playsfx$ = "pickup"
    GOSUB sfxplay
    _DELAY 0.5
    GOSUB slightfadein
END IF
REM tells console printer
LET eventtitle$ = "CURRENCY ADDED:"
LET eventdata$ = "+" + STR$(currencychange)
LET eventnumber = currency
GOSUB consoleprinter
COLOR 0, 0
LET currencychange = 0: LET temp93 = 0: LET temp94 = 0: LET silentgive = 0: REM scrubs temp values
RETURN

takecurrency:
REM takes currency from player
IF currencychange = 0 THEN RETURN: REM return for if there is no currency change
LET currency = currency - currencychange: REM removes money
REM changes font colour
COLOR _RGBA(letcurrencycolourr, letcurrencycolourg, letcurrencycolourb, letcurrencycoloura), _RGBA(bgcurrencycolourr, bgcurrencycolourg, bgcurrencycolourb, bgcurrencycoloura)
REM finds currency slot
OPEN pocketloc$ + "pocketfiles.ddf" FOR INPUT AS #1
DO
    LET temp91 = temp91 + 1
    INPUT #1, pocketfile$
LOOP UNTIL pocketfile$ = "currency" OR EOF(1)
CLOSE #1
IF silenttake = 0 THEN
    REM animation
    GOSUB slightfadeout
    DO
        REM pockets scroll in
        _LIMIT pockethudanispeed
        _PUTIMAGE (0, (0 - temp92))-(pockethudresx - 1, temp92), pockethud
        LET temp92 = temp92 + 1
    LOOP UNTIL temp92 >= pockethudresy
    LET temp92 = pocketspritex
    DO
        REM Pocket item scrolls out
        _LIMIT pockethudanispeed
        _PUTIMAGE (0, 0)-(pockethudresx - 1, pockethudresy), pockethud
        _PUTIMAGE (temp92, pocketspritey)-((temp92 + pocketspriteresx), (pocketspritey + pocketspriteresy)), pocketsprite(temp91)
        _PRINTSTRING (temp92 - (pocketspriteresx / 2), pocketspritey), STR$(currencychange)
        LET temp92 = temp92 + 1
        IF speedrun > 0 THEN GOSUB timeframecounter: GOSUB displayspeedrun
    LOOP UNTIL temp92 >= (resx + (pocketspriteresx / 2) + 1)
    REM plays sound effect
    LET playsfx$ = "drop"
    GOSUB sfxplay
    _DELAY 0.5
    GOSUB slightfadein
END IF
REM tells console printer
LET eventtitle$ = "CURRENCY REMOVED:"
LET eventdata$ = "-" + STR$(currencychange)
LET eventnumber = currency
GOSUB consoleprinter
REM checks if currency is below 0
IF currency < 0 THEN
    LET currency = 0
    REM tells console
    LET eventtitle$ = "CURRENCY BELOW ZERO"
    LET eventdata$ = "VALUE FIXED:"
    LET eventnumber = currency
    GOSUB consoleprinter
END IF
COLOR 0, 0
LET currencychange = 0: LET temp91 = 0: LET temp92 = 0: LET silenttake = 0: REM scrubs temp values
RETURN

centretext:
REM calculates position of centre text (centreno)
LET centreno = 0
LET temp70 = LEN(centretext$)
REM header font
IF fontmode = 1 THEN
    IF headerfontsname$ <> "" THEN
        LET centreno = temp70 * (headerfontsize / 2)
    ELSE
        LET centreno = temp70 * (16)
    END IF
END IF
REM default font
IF fontmode = 2 THEN
    IF defaultfontname$ <> "" THEN
        LET centreno = temp70 * (defaultfontsize / 2)
    ELSE
        LET centreno = temp70 * (8)
    END IF
END IF
REM small font
IF fontmode = 3 THEN
    IF smallfontname$ <> "" THEN
        LET centreno = temp70 * (smallfontsize / 2)
    ELSE
        LET centreno = temp70 * (8)
    END IF
END IF
LET temp70 = 0: LET centretext$ = "": REM scrub temp values
RETURN

pocketarrowright:
REM flashes pocket arrow right
IF temp81 = 0 THEN
    _PUTIMAGE (pocketarrowrlocx, pocketarrowrlocy)-((pocketarrowrlocx + pocketarrowresx), (pocketarrowrlocy + pocketarrowresy)), pocketarrowrs
ELSE
    _PUTIMAGE (pocketarrowrlocx, pocketarrowrlocy + pockethudresy)-((pocketarrowrlocx + pocketarrowresx), (pocketarrowrlocy + pocketarrowresy + pockethudresy)), pocketarrowrs
END IF
_DELAY 0.1
RETURN

pocketarrowleft:
REM flashes pocket arrow left
IF temp81 = 0 THEN
    _PUTIMAGE (pocketarrowllocx, pocketarrowllocy)-((pocketarrowllocx + pocketarrowresx), (pocketarrowllocy + pocketarrowresy)), pocketarrowls
ELSE
    _PUTIMAGE (pocketarrowllocx, pocketarrowllocy + pockethudresy)-((pocketarrowllocx + pocketarrowresx), (pocketarrowllocy + pocketarrowresy + pockethudresy)), pocketarrowls
END IF
_DELAY 0.1
RETURN

pocketcalcdown:
REM calculates what pocket item to display
REM sets values
LET temp61 = pocketdisplay
LET temp62 = pocketline
LET pocketdisplay = 0
REM goes back a line and checks if pocket item is available
DO
    OPEN pocketloc$ + "pocketfiles.ddf" FOR INPUT AS #1
    LET temp60 = 0
    LET pocketline = pocketline - 1
    DO
        LET temp60 = temp60 + 1
        INPUT #1, pocketfile$
        IF EOF(1) THEN
            REM if file ends
            LET pocketdisplay = temp61
            LET pocketline = temp62
            LET temp60 = 0
            LET temp61 = 0
            LET temp62 = 0
            CLOSE #1
            RETURN
        END IF
    LOOP UNTIL temp60 = pocketline
    IF pocketitem(temp60) = 1 THEN LET pocketdisplay = temp60
    CLOSE #1
LOOP UNTIL pocketdisplay > 0
LET pocketline = temp60
LET temp60 = 0: LET temp61 = 0: LET temp62 = 0: REM scrubs temp values
RETURN

pocketcalcup:
REM calculates what pocket item to display
REM sets values
LET temp61 = pocketdisplay
LET temp62 = pocketline
LET pocketdisplay = 0
REM gets to current line
OPEN pocketloc$ + "pocketfiles.ddf" FOR INPUT AS #1
IF pocketline > 0 THEN
    DO
        LET temp60 = temp60 + 1
        INPUT #1, pocketfile$
        IF EOF(1) THEN
            REM if file ends
            LET temp60 = 0
            LET pocketdisplay = temp61
            LET pocketline = temp62
            LET temp61 = 0
            LET temp62 = 0
            CLOSE #1
            RETURN
        END IF
    LOOP UNTIL temp60 >= pocketline
END IF
DO
    LET temp60 = temp60 + 1
    INPUT #1, pocketfile$
    IF pocketitem(temp60) = 1 THEN LET pocketdisplay = temp60
    IF pocketdisplay > 0 THEN CLOSE #1: LET pocketline = temp60: LET temp60 = 0: RETURN
    IF EOF(1) THEN
        REM if file ends
        LET temp60 = 0
        LET pocketdisplay = temp61
        LET pocketline = temp62
        LET temp61 = 0
        LET temp62 = 0
        CLOSE #1
        COLOR 0, 0
        RETURN
    END IF
LOOP
CLOSE #1
COLOR 0, 0
LET pocketline = temp60: LET temp60 = 0: LET temp61 = 0: LET temp62 = 0 REM clear temp values
RETURN

textbannerdraw:
REM draws text banner and NPC pic
COLOR _RGBA(letspeechcolourr, letspeechcolourg, letspeechcolourb, letspeechcoloura), _RGBA(bgspeechcolourr, bgspeechcolourg, bgspeechcolourb, bgspeechcoloura)
IF pocketon = 0 AND mainmenu = 0 OR scriptrun = 1 THEN GOSUB slightfadeout
LET temp71 = (resy + 1)
REM matches player
LET x = 0
DO
    LET x = x + 1
    IF selectobject$ = playername(x) THEN LET temp80 = x
LOOP UNTIL x >= mapplayerno OR x >= totalplayers
REM sets speakers name
IF objecttype$ = "NPC" THEN
    LET speakername$ = selectobject$
ELSE
    LET speakername$ = mplayermodel$
END IF
REM draws text banner
DO
    _LIMIT pockethudanispeed
    IF mainmenu = 0 AND setupboot = 0 AND erasesaveonly = 0 AND pocketon <> 1 THEN
		_PUTIMAGE (0, temp71 - defaultfontsize)-(defaultfontsize * LEN(speakername$), temp71), textbannername
		COLOR _RGBA(letspeechcolourr, letspeechcolourg, letspeechcolourb, letspeechcoloura), _RGBA(bgspeechcolourr, bgspeechcolourg, bgspeechcolourb, bgspeechcoloura)
        _PRINTSTRING (defaultfontsize, temp71 - defaultfontsize), speakername$
    END IF
    _PUTIMAGE (0, temp71)-(textbannerresx - 1, (textbannerresy + temp71) - 1), textbanner
    LET temp71 = temp71 - 1
LOOP UNTIL temp71 <= (resy - textbannerresy - 1)
LET temp71 = resx + 1
REM draws player portrait
DO
    _LIMIT pockethudanispeed
    IF mainmenu = 0 AND pocketon <> 1 THEN
        IF objecttype$ = "OBJ" THEN _PUTIMAGE (temp71, textbannerfacey)-((temp71 + textbannerfaceresx - 1), (textbannerfacey + textbannerfaceresy - 1)), mpface1
        IF objecttype$ = "NPC" THEN
            _PUTIMAGE (temp71, textbannerfacey)-((temp71 + textbannerfaceresx - 1), (textbannerfacey + textbannerfaceresy - 1)), playerface1(temp80)
        END IF
    END IF
    LET temp71 = temp71 - 1
LOOP UNTIL temp71 = (resx - textbannerfaceresx)
_PUTIMAGE (0, (resy - textbannerresy))-(textbannerresx - 1, resy - 1), textbanner
IF mainmenu = 0 AND pocketon <> 1 THEN
    IF objecttype$ = "OBJ" THEN _PUTIMAGE (temp71, textbannerfacey)-((temp71 + textbannerfaceresx - 1), (textbannerfacey + textbannerfaceresy - 1)), mpface1
    IF objecttype$ = "NPC" THEN
        _PUTIMAGE (temp71, textbannerfacey)-((temp71 + textbannerfaceresx - 1), (textbannerfacey + textbannerfaceresy - 1)), playerface2(temp80)
    END IF
END IF
GOSUB textbannercalc
COLOR 0, 0
IF pocketon = 0 AND mainmenu = 0 OR scriptrun = 1 THEN GOSUB slightfadein
LET temp71 = 0: LET temp80 = 0: LET temp9000 = 0: LET textspeech$ = "": REM scrub temp values
_KEYCLEAR: REM clears keyboard input
RETURN

talksfx:
REM generates sound effects for player talking
IF soundmode = 1 OR soundmode = 3 THEN RETURN: REM return for if sfx is off.
IF autotxtsfx = 1 THEN
	IF temp124 = 0 THEN LET temp124 = 1: REM sets values
	REM mainplayer
	IF objecttype$ = "OBJ" OR objecttype$ = "" THEN
		IF textline$ <> " " OR textline$ <> "" THEN
			SOUND 0, 0
			IF temp124 = 1 THEN SOUND mpnote1, 1
			IF temp124 = 2 THEN SOUND mpnote2, 1
		ELSE
			SOUND 0, 0
		END IF
	END IF
	REM NPC
	IF objecttype$ = "NPC" THEN
		IF textline$ <> " " OR textline$ <> "" THEN
			SOUND 0, 0
			IF temp124 = 1 THEN LET temp125 = playernote1(temp80)
			IF temp124 = 2 THEN LET temp125 = playernote2(temp80)
			SOUND temp125, 1
		ELSE
			SOUND 0, 0
		END IF
	END IF
	REM changes note value
	IF temp124 = 1 THEN LET temp124 = 2: RETURN
	IF temp124 = 2 THEN LET temp124 = 1: RETURN
END IF
IF autotxtsfx = 2 THEN
	IF temp9000 = 1 THEN RETURN
	LET playsfx$ = "talk": GOSUB sfxplay: REM plays sound efffect
	LET temp9000 = 1
END IF
RETURN

textbannercalc:
REM calculates how long text is to be in text banner
COLOR _RGBA(letspeechcolourr, letspeechcolourg, letspeechcolourb, letspeechcoloura), _RGBA(bgspeechcolourr, bgspeechcolourg, bgspeechcolourb, bgspeechcoloura)
REM delete temp file if temp file already exists
IF _FILEEXISTS(dloc$ + "banner.tmp") THEN
    IF ros$ = "win" THEN SHELL _HIDE "del " + dloc$ + "banner.tmp"
    IF ros$ = "lnx" OR ros$ = "mac" THEN SHELL _HIDE "rm " + dloc$ + "banner.tmp"
END IF
REM outputs text to temp file letter by letter
DO
    LET temp72 = temp72 + 1
    LET temp14$ = MID$(textspeech$, temp72, 1)
    OPEN dloc$ + "banner.tmp" FOR APPEND AS #1
    WRITE #1, temp14$
    CLOSE #1
LOOP UNTIL temp72 >= LEN(textspeech$)
REM prints text banner to console
LET eventtitle$ = "TEXT BANNER:"
LET eventdata$ = textspeech$
LET eventnumber = temp72
GOSUB consoleprinter
REM opens temp file and puts appropiate letters in place
LET temp72 = 5
LET temp75 = 1
LET temp76 = 1
OPEN dloc$ + "banner.tmp" FOR INPUT AS #1
DO
    _LIMIT hertz
	IF speedrun > 0 THEN GOSUB timeframecounter: GOSUB displayspeedrun: COLOR _RGBA(letspeechcolourr, letspeechcolourg, letspeechcolourb, letspeechcoloura), _RGBA(bgspeechcolourr, bgspeechcolourg, bgspeechcolourb, bgspeechcoloura)
    LET d$ = UCASE$(INKEY$)
    INPUT #1, textline$
    REM prints letter to screen adds pixels if the letter is i or a ' or !.
    IF imode = 0 THEN
		IF temp75 = 1 THEN _PRINTSTRING (temp72, 120), textline$
		IF temp75 = 2 THEN _PRINTSTRING (temp72, 130), textline$
	ELSE
		IF imode = 1 THEN
			IF textline$ = "i" THEN
				IF temp75 = 1 THEN _PRINTSTRING (temp72 + (defaultfontsize / 4), 120), textline$
				IF temp75 = 2 THEN _PRINTSTRING (temp72 + (defaultfontsize / 4), 130), textline$
			ELSE
				IF temp75 = 1 THEN _PRINTSTRING (temp72, 120), textline$
				IF temp75 = 2 THEN _PRINTSTRING (temp72, 130), textline$
			END IF
		END IF
		IF imode = 2 THEN
			IF UCASE$(textline$) = "I" THEN
				IF temp75 = 1 THEN _PRINTSTRING (temp72 + (defaultfontsize / 4), 120), textline$
				IF temp75 = 2 THEN _PRINTSTRING (temp72 + (defaultfontsize / 4), 130), textline$
			ELSE
				IF temp75 = 1 THEN _PRINTSTRING (temp72, 120), textline$
				IF temp75 = 2 THEN _PRINTSTRING (temp72, 130), textline$
			END IF
		END IF
	END IF
    REM plays talking sounds
    GOSUB talksfx
    LET temp72 = temp72 + (defaultfontsize / 2) + 1
    IF temp76 = 1 THEN _DELAY 0.05: REM letter draw delay
    REM if space is pressed
    IF d$ = " " THEN LET temp76 = 2
    REM if space happens close to end
    IF textline$ = " " THEN
        LET temp73 = (resx - 5) / 4
        LET temp73 = (resx - 5) - temp73
        IF temp72 >= temp73 THEN LET temp72 = resx
    END IF
    REM if textspeech$ is too long for text banner
    IF temp72 >= resx - 5 THEN
        LET temp72 = 5
        IF temp75 = 1 THEN
            LET temp75 = 2
        ELSE
            DO
                _LIMIT hertz
                GOSUB timeframecounter
                IF INT(ctime) MOD 2 THEN
                    IF mainmenu = 0 AND pocketon <> 1 THEN
                        IF objecttype$ = "OBJ" THEN _PUTIMAGE (temp71, textbannerfacey)-((temp71 + textbannerfaceresx - 1), (textbannerfacey + textbannerfaceresy - 1)), mpface1
                        IF objecttype$ = "NPC" THEN
                            _PUTIMAGE (temp71, textbannerfacey)-((temp71 + textbannerfaceresx - 1), (textbannerfacey + textbannerfaceresy - 1)), playerface1(temp80)
                        END IF
                    END IF
                ELSE
                    IF mainmenu = 0 AND pocketon <> 1 THEN
                        IF objecttype$ = "OBJ" THEN _PUTIMAGE (temp71, textbannerfacey)-((temp71 + textbannerfaceresx - 1), (textbannerfacey + textbannerfaceresy - 1)), mpface2
                        IF objecttype$ = "NPC" THEN
                            _PUTIMAGE (temp71, textbannerfacey)-((temp71 + textbannerfaceresx - 1), (textbannerfacey + textbannerfaceresy - 1)), playerface2(temp80)
                        END IF
                    END IF
                END IF
                IF speedrun > 0 THEN GOSUB displayspeedrun
            LOOP UNTIL INKEY$ = " "
            _PUTIMAGE (0, (resy - textbannerresy))-(textbannerresx - 1, resy - 1), textbanner
            LET temp75 = 1
            LET temp76 = 1
        END IF
    END IF
LOOP UNTIL EOF(1)
CLOSE #1
DO
    _LIMIT hertz
    GOSUB timeframecounter
    IF INT(ctime) MOD 2 THEN
        IF mainmenu = 0 AND pocketon <> 1 THEN
            IF objecttype$ = "OBJ" THEN _PUTIMAGE (temp71, textbannerfacey)-((temp71 + textbannerfaceresx - 1), (textbannerfacey + textbannerfaceresy - 1)), mpface1
            IF objecttype$ = "NPC" THEN
                _PUTIMAGE (temp71, textbannerfacey)-((temp71 + textbannerfaceresx - 1), (textbannerfacey + textbannerfaceresy - 1)), playerface1(temp80)
            END IF
        END IF
    ELSE
        IF mainmenu = 0 AND pocketon <> 1 THEN
            IF objecttype$ = "OBJ" THEN _PUTIMAGE (temp71, textbannerfacey)-((temp71 + textbannerfaceresx - 1), (textbannerfacey + textbannerfaceresy - 1)), mpface2
            IF objecttype$ = "NPC" THEN
                _PUTIMAGE (temp71, textbannerfacey)-((temp71 + textbannerfaceresx - 1), (textbannerfacey + textbannerfaceresy - 1)), playerface2(temp80)
            END IF
        END IF
    END IF
    IF speedrun > 0 THEN GOSUB displayspeedrun
LOOP UNTIL INKEY$ = " "
REM plays select sound effect
IF textbannersound = 1 THEN
    LET playsfx$ = "select"
    GOSUB sfxplay
END IF
REM deletes temp file
_KEYCLEAR
COLOR 0, 0
IF ros$ = "win" THEN SHELL _HIDE "del " + dloc$ + "banner.tmp"
IF ros$ = "lnx" OR ros$ = "mac" THEN SHELL _HIDE "rm " + dloc$ + "banner.tmp"
LET temp72 = 0: LET temp73 = 0: LET temp75 = 0: LET temp76 = 0: LET d$ = "": LET temp14$ = "": LET textline$ = "": LET temp124 = 0: LET temp125 = 0: REM scrub temp values
RETURN

uiload:
REM loads UI items into memory
LET scriptimage = _LOADIMAGE(uiloc$ + scriptimage$ + ".png")
LET pockethud = _LOADIMAGE(uiloc$ + pockethudimage$ + ".png")
LET pocketarrowr = _LOADIMAGE(uiloc$ + pocketarrowright$ + ".png")
LET pocketarrowl = _LOADIMAGE(uiloc$ + pocketarrowleft$ + ".png")
LET pocketarrowrs = _LOADIMAGE(uiloc$ + pocketarrowselectright$ + ".png")
LET pocketarrowls = _LOADIMAGE(uiloc$ + pocketarrowselectleft$ + ".png")
LET pocketbanner = _LOADIMAGE(uiloc$ + pocketbanner$ + ".png")
LET textbanner = _LOADIMAGE(uiloc$ + textbanner$ + ".png")
LET pocketselect = _LOADIMAGE(uiloc$ + pocketselect$ + ".png")
LET pocketarrowlu = _LOADIMAGE(uiloc$ + pocketarrowunavailableleft$ + ".png")
LET pocketarrowru = _LOADIMAGE(uiloc$ + pocketarrowunavailableright$ + ".png")
LET loadicon = _LOADIMAGE(uiloc$ + loadicon$ + ".png")
LET torcheffect = _LOADIMAGE(uiloc$ + torcheffectfile$ + ".png")
LET loadbar = _LOADIMAGE(uiloc$ + loadbar$ + ".png")
LET devlogo = _LOADIMAGE(uiloc$ + devlogo$ + ".png")
LET textbannername = _LOADIMAGE(uiloc$ + textbannername$ + ".png")
LET eventtitle$ = "UI ITEMS LOADED"
LET eventdata$ = ""
LET eventnumber = 0
GOSUB consoleprinter
RETURN

uiunload:
REM unloads UI items from memory
_FREEIMAGE scriptimage
_FREEIMAGE pockethud
_FREEIMAGE pocketarrowr
_FREEIMAGE pocketarrowl
_FREEIMAGE pocketarrowrs
_FREEIMAGE pocketarrowls
_FREEIMAGE pocketbanner
_FREEIMAGE textbanner
_FREEIMAGE pocketselect
_FREEIMAGE pocketarrowru
_FREEIMAGE pocketarrowlu
_FREEIMAGE loadicon
_FREEIMAGE torcheffect
_FREEIMAGE loadbar
_FREEIMAGE devlogo
_FREEIMAGE textbannername
LET eventtitle$ = "UI ITEMS UNLOADED"
LET eventdata$ = ""
LET eventnumber = 0
GOSUB consoleprinter
RETURN

musicplay:
REM plays music
REM diverts
IF soundmode = 1 OR soundmode = 4 THEN RETURN: REM diverts if sound is off
IF playmusic$ = currentmusic$ THEN RETURN: REM diverts if music is the same
IF currentmusic$ <> "" AND musicpause = 0 THEN GOSUB musicstop: REM stops currently playing music
OPEN museloc$ + "musicfiles.ddf" FOR INPUT AS #1
DO
    INPUT #1, musicfile$
    LET temp30 = temp30 + 1
    IF temp30 = 1 THEN
        IF playmusic$ = musicfile$ THEN LET currentmusic$ = musicfile$: _SNDLOOP menumusic%
    ELSE
        IF playmusic$ = musicfile$ THEN LET currentmusic$ = musicfile$: _SNDLOOP musicdata(temp30 - 1)
    END IF
LOOP UNTIL EOF(1)
CLOSE #1
REM prints result to console
IF currentmusic$ <> "" THEN
    IF currentmusic$ = playmusic$ THEN
        LET eventtitle$ = "MUSIC PLAYING:"
        LET eventdata$ = currentmusic$
        LET eventnumber = 0
        GOSUB consoleprinter
        LET musicpause = 0: REM tells engine music is not paused
    ELSE
        LET eventtitle$ = "MUSIC NOT PLAYING:"
        LET eventdata$ = "FILE NOT LOADED"
        LET eventnumber = 0
        GOSUB consoleprinter
    END IF
END IF
LET temp30 = 0: REM scrub temp values
RETURN

musicpause:
REM pauses music
REM if soundmode = 1 then return: rem return for is sound is off
OPEN museloc$ + "musicfiles.ddf" FOR INPUT AS #1
DO
    INPUT #1, musicfile$
    LET temp32 = temp32 + 1
    IF temp32 = 1 THEN
        IF currentmusic$ = musicfile$ THEN _SNDPAUSE menumusic%: LET musicpause = 1
    ELSE
        IF temp32 = 2 THEN IF currentmusic$ = musicfile$ THEN _SNDPAUSE musicdata(temp32 - 1): LET musicpause = 1
    END IF
LOOP UNTIL EOF(1)
CLOSE #1
IF musicpause = 1 THEN
    REM prints result to console
    LET eventtitle$ = "MUSIC PAUSED:"
    LET eventdata$ = currentmusic$
    LET eventnumber = 0
    GOSUB consoleprinter
END IF
LET temp32 = 0: REM scrub temp values
RETURN

musicstop:
REM stops music
IF soundmode = 1 OR soundmode = 4 THEN RETURN: REM return for is sound is off
OPEN museloc$ + "musicfiles.ddf" FOR INPUT AS #1
DO
    INPUT #1, musicfile$
    LET temp31 = temp31 + 1
    IF temp31 = 1 THEN
        IF currentmusic$ = musicfile$ THEN LET oldmusic$ = currentmusic$: LET currentmusic$ = "": _SNDSTOP menumusic%
    ELSE
        IF currentmusic$ = musicfile$ THEN LET oldmusic$ = currentmusic$: LET currentmusic$ = "": _SNDSTOP musicdata(temp31 - 1)
    END IF
LOOP UNTIL EOF(1)
CLOSE #1
REM prints result to console
IF currentmusic$ = "" THEN
    LET eventtitle$ = "MUSIC STOPPED:"
    LET eventdata$ = oldmusic$
    LET eventnumber = 0
    GOSUB consoleprinter
ELSE
    LET eventtitle$ = "MUSIC NOT STOPPED:"
    LET eventdata$ = "FILE NOT LOADED"
    LET eventnumber = 0
    GOSUB consoleprinter
END IF
LET temp31 = 0: REM scrub temp values
RETURN

musicload:
REM loads music files into memory
IF liteload = 1 THEN RETURN: REM return for if liteload is active
OPEN museloc$ + "musicfiles.ddf" FOR INPUT AS #1
DO
    LET temp28 = temp28 + 1
    INPUT #1, musicfile$: REM reads name of music file to be loaded
    REM loads music files and assigns them a slot
    IF temp28 = 1 THEN
        LET menumusic% = _SNDOPEN(museloc$ + musicfile$ + ".ogg")
    ELSE
        LET music$(temp28 - 1) = musicfile$: LET musicdata(temp28 - 1) = _SNDOPEN(museloc$ + musicfile$ + ".ogg")
    END IF
    REM prints load to console
    LET eventtitle$ = "MUSIC LOADED:"
    LET eventdata$ = musicfile$
    LET eventnumber = temp28
    GOSUB consoleprinter
    IF setupboot = 1 THEN
        REM loadbar
        LET temp125 = temp125 + 5
        GOSUB loadbar
    END IF
LOOP UNTIL EOF(1)
CLOSE #1
GOSUB musicvol: REM sets music volume level
IF setupboot = 1 THEN
    REM loadbar
    LET temp125 = 70
    GOSUB loadbar
END IF
REM scrub temp values
LET temp28 = 0
RETURN

musicvol:
REM changes volume of music
IF soundmode = 1 OR soundmode = 4 THEN RETURN: REM return for if music if off
OPEN museloc$ + "musicfiles.ddf" FOR INPUT AS #1
DO
    INPUT #1, musicfile$
    LET temp133 = temp133 + 1
    IF temp133 = 1 THEN
        _SNDVOL menumusic%, musicvol
    ELSE
        _SNDVOL musicdata(temp133 - 1), musicvol
    END IF
LOOP UNTIL EOF(1)
CLOSE #1
REM prints to console
LET eventtitle$ = "MUSIC VOLUME SET:"
LET eventdata$ = ""
LET eventnumber = musicvol
GOSUB consoleprinter
LET temp133 = 0: REM scrub temp values
RETURN

musicunload:
REM unloads music files from memory
OPEN museloc$ + "musicfiles.ddf" FOR INPUT AS #1
DO
    INPUT #1, musicfile$
    LET temp29 = temp29 + 1
    IF temp29 = 1 THEN
        _SNDCLOSE menumusic%
    ELSE
        _SNDCLOSE musicdata(temp29 - 1)
    END IF
    LET eventtitle$ = "MUSIC UNLOADED:"
    LET eventdata$ = musicfile$
    LET eventnumber = temp29
    GOSUB consoleprinter
LOOP UNTIL EOF(1)
CLOSE #1
LET temp29 = 0: REM scrub temp values
RETURN

sfxplay:
REM plays sfx
REM diverts
IF soundmode = 1 OR soundmode = 3 THEN RETURN
OPEN sfxloc$ + "sfxfiles.ddf" FOR INPUT AS #8
DO
    INPUT #8, sfxfile$
    LET temp37 = temp37 + 1
    IF playsfx$ = sfxfile$ THEN _SNDPLAY sfxdata(temp37): LET temp38 = 1
LOOP UNTIL EOF(8)
CLOSE #8
REM prints result to console
IF temp38 = 1 THEN
    LET eventtitle$ = "SOUND EFFECT PLAYING:"
    LET eventdata$ = playsfx$
    LET eventnumber = 0
    GOSUB consoleprinter
ELSE
    LET eventtitle$ = "SOUND EFFECT NOT PLAYING:"
    LET eventdata$ = "file not loaded"
    LET eventnumber = 0
    GOSUB consoleprinter
END IF
LET temp37 = 0: LET temp38 = 0: REM scrub temp values
RETURN

sfxload:
REM loads sfx files into memory
IF liteload = 1 THEN RETURN: REM return for if liteload is active
OPEN sfxloc$ + "sfxfiles.ddf" FOR INPUT AS #1
DO
    LET temp35 = temp35 + 1
    INPUT #1, sfxfile$: REM reads name of music file to be loaded
    REM loads music files and assigns them a slot
    LET sfx$(temp35) = sfxfile$: LET sfxdata(temp35) = _SNDOPEN(sfxloc$ + sfxfile$ + ".ogg")
    REM prints load to console
    LET eventtitle$ = "SOUND EFFECT LOADED:"
    LET eventdata$ = sfxfile$
    LET eventnumber = temp35
    GOSUB consoleprinter
    IF setupboot = 1 THEN
        REM loading bar
        LET temp125 = temp125 + 1
        GOSUB loadbar
    END IF
LOOP UNTIL EOF(1)
CLOSE #1
IF setupboot = 1 THEN
    REM load bar
    LET temp125 = 85
    GOSUB loadbar
END IF
GOSUB sfxvol
REM scrub temp values
LET temp35 = 0
RETURN

sfxvol:
REM sets sfx volume
IF soundmode = 1 OR soundmode = 3 THEN RETURN: REM return for is sound is off
OPEN sfxloc$ + "sfxfiles.ddf" FOR INPUT AS #1
DO
    INPUT #1, sfxfile$
    LET temp134 = temp134 + 1
    _SNDVOL sfxdata(temp134), sfxvol
LOOP UNTIL EOF(1)
CLOSE #1
REM prints to console
LET eventtitle$ = "SOUND EFFECT VOLUME SET:"
LET eventdata$ = ""
LET eventnumber = sfxvol
GOSUB consoleprinter
LET temp134 = 0: REM scub temp values
RETURN

sfxunload:
REM unloads sfx files from memory
OPEN sfxloc$ + "sfxfiles.ddf" FOR INPUT AS #1
DO
    INPUT #1, sfxfile$
    LET temp36 = temp36 + 1
    _SNDCLOSE sfxdata(temp36)
    LET eventtitle$ = "SOUND EFFECT UNLOADED:"
    LET eventdata$ = sfxfile$
    LET eventnumber = temp36
    GOSUB consoleprinter
LOOP UNTIL EOF(1)
CLOSE #1
LET temp36 = 0: REM scrub temp values
RETURN

saveload:
REM loads save data
REM checks save data file exists and diverts to save erase if not
IF _FILEEXISTS(sloc$ + "savedata.ddf") THEN
    REM loads savedata
    OPEN sloc$ + "savedata.ddf" FOR INPUT AS #1
    INPUT #1, mapno, currency, screenmode, posx, posy, direction, soundmode, musicvol, sfxvol, pocketcarry
    REM loads pocket items
    LET x = 0
    DO
        LET x = x + 1
        INPUT #1, pocketitem(x)
    LOOP UNTIL x >= totalpockets
    REM loads checkpoints
    LET x = 0
    DO
        LET x = x + 1
        INPUT #1, checkpoint(x)
    LOOP UNTIL x >= totalcheckpoints
    REM loads main player
    LET x = 0
    INPUT #1, mplayermodel$
    CLOSE #1
    REM prints to console
    LET eventtitle$ = "SAVEDATA LOADED:"
    LET eventdata$ = sloc$ + "savedata.ddf"
    LET eventnumber = 0
    GOSUB consoleprinter
    LET eventtitle$ = "SAVEDATA STATUS:"
    IF exitsave = 1 THEN
        LET eventdata$ = "save on exit enabled"
    ELSE
        LET eventdata$ = "save on exit disabled"
    END IF
    LET eventnumber = exitsave
    GOSUB consoleprinter
ELSE
    REM diverts to save erase
    LET eventtitle$ = "SAVE FILE MISSING OR CORRUPT"
    LET eventdata$ = "resetting to default"
    LET eventnumber = 0
    GOSUB consoleprinter
    IF _FILEEXISTS(sloc$ + "defaultsave.ddf") THEN
        REM nothing
    ELSE
        ERROR 422
    END IF
    REM Linux + MacOS
    IF ros$ = "lnx" OR ros$ = "mac" THEN
        SHELL _HIDE "cp " + sloc$ + "defaultsave.ddf " + sloc$ + "savedata.ddf"
    END IF
    REM Windoze
    IF ros$ = "win" THEN
        SHELL _HIDE "copy " + sloc$ + "defaultsave.ddf " + sloc$ + "savedata.ddf"
    END IF
    GOTO saveload
END IF
RETURN

savedefault:
REM overwrites default save
OPEN sloc$ + "defaultsave.ddf" FOR OUTPUT AS #1
WRITE #1, mapno, currency, screenmode, posx, posy, direction, soundmode, musicvol, sfxvol, pocketcarry
REM writes pocket items
LET x = 0
DO
    LET x = x + 1
    WRITE #1, pocketitem(x)
LOOP UNTIL x >= totalpockets
CLOSE #1
REM writes checkpoints
LET x = 0
DO
    LET x = x + 1
    WRITE #1, checkpoint(x)
LOOP UNTIL x >= totalcheckpoints
REM writes main player
LET x = 0
WRITE #1, mplayermodel$
CLOSE #1
REM prints to console
LET eventtitle$ = "DEFAULT SAVE OVERWRITTEN:"
LET eventdata$ = sloc$ + "defaultsave.ddf"
LET eventnumber = 0
GOSUB consoleprinter
RETURN

savesave:
REM saves save data x
IF nosave = 1 THEN RETURN: REM return for if nosave flag is used.
OPEN sloc$ + "savedata.ddf" FOR OUTPUT AS #1
WRITE #1, mapno, currency, screenmode, posx, posy, direction, soundmode, musicvol, sfxvol, pocketcarry
REM writes pocket items
LET x = 0
DO
    LET x = x + 1
    WRITE #1, pocketitem(x)
LOOP UNTIL x >= totalpockets
REM writes checkpoints
LET x = 0
DO
    LET x = x + 1
    WRITE #1, checkpoint(x)
LOOP UNTIL x >= totalcheckpoints
REM writes main player
LET x = 0
WRITE #1, mplayermodel$
CLOSE #1
REM prints to console
LET eventtitle$ = "SAVEDATA SAVED:"
LET eventdata$ = sloc$ + "savedata.ddf"
LET eventnumber = 0
GOSUB consoleprinter
RETURN

screenload:
REM sets screen mode
_TITLE title$
SCREEN _NEWIMAGE(resx, resy, 32)
$RESIZE:STRETCH
IF screenmode = 2 THEN _FULLSCREEN _OFF
IF screenmode = 1 THEN _FULLSCREEN _SQUAREPIXELS
IF devmode = 0 THEN _MOUSEHIDE: REM hides mouse (if devmode is off)
LET eventtitle$ = "SCREEN MODE SET:"
IF screenmode = 2 THEN LET eventdata$ = "windowed"
IF screenmode = 1 THEN LET eventdata$ = "fullscreen"
LET eventnumber = screenmode
GOSUB consoleprinter
RETURN

animation:
REM animation
REM checks if animation file exists
REM check if filename needs fixing
IF _FILEEXISTS(aloc$ + anifile$ + "/" + anifile$ + ".ddf") THEN
    REM nothing
ELSE
    ERROR 424
    LET anifile$ = ""
    LET anisprite$ = ""
    RETURN
END IF
REM loads metadata file
OPEN aloc$ + anifile$ + "/" + anifile$ + ".ddf" FOR INPUT AS #22
INPUT #22, aniframes
LET x = 0
DO
    LET x = x + 1
    INPUT #22, frame(x)
LOOP UNTIL EOF(22)
CLOSE #22
REM loads frame sprites
LET x = 0
DO
    LET x = x + 1
    LET aniframe(x) = _LOADIMAGE(aloc$ + anifile$ + "/frame" + LTRIM$(STR$(x)) + ".png")
LOOP UNTIL x >= aniframes
REM prints to console
LET eventtitle$ = "ANIMATION LOADED:"
LET eventdata$ = anifile$
LET eventnumber = aniframes
GOSUB consoleprinter
REM display frames
LET temp98 = 0: LET temp99 = 0: LET x = 0
IF anisprite$ = "mainplayer" THEN
    REM mainplayer sprite animation
    DO
        IF temp98 = 0 THEN LET temp98 = 1
        IF temp99 = 0 THEN
            REM sets number of frames the sprite is to be displayed for
            LET temp99 = frames + frame(temp98)
        END IF
        LET effectani = 1
        GOSUB gameloop: REM draws world
        LET effectani = 0
        REM draws animation frames
        _PUTIMAGE (mpposx, mpposy), aniframe(temp98)
        GOSUB effectdraw
        _DISPLAY
        REM calculates when to move onto next frame
        IF frames >= temp99 THEN LET temp98 = temp98 + 1: LET temp99 = 0
    LOOP UNTIL temp98 > aniframes
ELSE
    REM NPC or object sprite animation
    DO
        IF temp98 = 0 THEN LET temp98 = 1
        IF temp99 = 0 THEN
            REM sets number of frames the sprite is to be displayed for
            LET temp99 = frames + frame(temp98)
        END IF
        REM finds which NPC is being animated
        LET x = 0: LET aniplayer = 0
        DO
            LET x = x + 1
            IF anisprite$ = playername(x) THEN LET aniplayer = x
        LOOP UNTIL x >= totalplayers
        LET effectani = 1
        GOSUB gameloop: REM draws world
        LET effectani = 0
        REM draws animation frames
        IF aniplayer <> 0 THEN _PUTIMAGE (playerx(aniplayer) + posx, playery(aniplayer) + posy), aniframe(temp98)
        REM finds which object is being animated
        LET x = 0: LET aniobject = 0
        DO
            LET x = x + 1
            IF anisprite$ = objectname(x) THEN LET aniobject = x
        LOOP UNTIL x >= totalobjects
        IF aniobject <> 0 THEN _PUTIMAGE (objectx(aniobject) + posx, objecty(aniobject) + posy), aniframe(temp98)
        GOSUB effectdraw
        _DISPLAY
        REM calculates when to move onto next frame
        IF frames >= temp99 THEN LET temp98 = temp98 + 1: LET temp99 = 0
    LOOP UNTIL temp98 > aniframes
END IF
REM unloads animation files
LET x = 0
DO
    LET x = x + 1
    _FREEIMAGE aniframe(x)
LOOP UNTIL x >= aniframes
REM prints to console
LET eventtitle$ = "ANIMATION UNLOADED:"
LET eventdata$ = anifile$
LET eventnumber = aniframes
GOSUB consoleprinter
LET anisprite$ = "": LET aniframes = 0: LET temp98 = 0: LET temp99 = 0: LET temp103 = 0: LET x = 0: REM scrub temp values
RETURN

consoleprinter:
REM prints extra engine data to console / error log
IF consolelogging = 1 THEN
    IF _FILEEXISTS(consolelog$) THEN
        REM nothing
    ELSE
        OPEN consolelog$ FOR OUTPUT AS #2
        PRINT #2, DATE$, TIME$, "VaME CONSOLE LOG"
        CLOSE #2
    END IF
    OPEN consolelog$ FOR APPEND AS #2
    IF eventnumber <> 0 THEN PRINT #2, DATE$, TIME$, eventtitle$, eventdata$; eventnumber
    IF eventnumber = 0 THEN PRINT #2, DATE$, TIME$, eventtitle$, eventdata$
    CLOSE #2
END IF
IF displayconsole = 1 THEN
    REM displays in console
    _DEST _CONSOLE
    IF eventnumber <> 0 THEN PRINT DATE$, TIME$, eventtitle$, eventdata$; eventnumber
    IF eventnumber = 0 THEN PRINT DATE$, TIME$, eventtitle$, eventdata$
    _DEST 0
END IF
REM flush values
LET eventtitle$ = "": LET eventdata$ = "": LET eventnumber = 0
RETURN

errorhandler:
REM handles expected in-game errors
LET errdescription$ = "": REM blanks out custom error description
IF ERR = 423 THEN LET errdescription$ = "MISSING SCRIPT FILE - " + scriptname$: GOSUB errorprinter: RESUME NEXT
IF ERR = 424 THEN LET errdescription$ = "MISSING ANIMATION FILE - " + anifile$: GOSUB errorprinter: RESUME NEXT
IF ERR = 425 THEN LET errdescription$ = "MISSING TERMINAL FILE - " + runterminal$: GOSUB errorprinter: RESUME NEXT
IF ERR < 420 THEN
	GOSUB errorprinter
	RESUME NEXT
END IF
REM halts program upon unexpected error
REM == FROM HERE, PROGRAM WILL HALT AND IS CONSIDERED NON-RECOVERABLE ==
ON ERROR GOTO errorduringerror: REM error handler for the error handler (ikr)
IF ERR = 420 THEN LET errdescription$ = "MISSING ENGINE METADATA - TRY REINSTALL"
IF ERR = 421 THEN LET errdescription$ = "MISSING METADATA DIRECTORY - TRY REINSTALL"
IF ERR = 422 THEN LET errdescription$ = "MISSING DEFAULT SAVE FILE - TRY REINSTALL"
IF ERR = 426 THEN LET errdescription$ = "CONFLICTING LAUNCH PARAMETERS - CANNOT USE -FIX AND -NOUPDATE AT THE SAME TIME"
IF ERR = 427 THEN LET errdescription$ = "GAME REQUIRES DIFFERENT ENGINE BUILD - SEEK VaME VERSION " + engineversionno$
IF ERR = 666 THEN LET errdescription$ = "DEMONIC ERROR - CONTACT LOCAL UAC REP"
IF ERR = 999 THEN LET errdescription$ = "UNSUPPORTED OPERATING SYSTEM - LOCATE UNFORKED BUILD"
LET errorcrash = 1: REM sets error crash value to 1
BEEP
PRINT "=== GURU MEDITATION ==="
PRINT DATE$, TIME$
PRINT "ERROR CODE: "; ERR
PRINT "LINE: "; _ERRORLINE
PRINT errdescription$
PRINT "DUMPING ERROR FILE..."
IF ERR = 420 OR ERR = 421 THEN
    PRINT "...ERROR FILE COULD NOT BE DUMPED!"
ELSE
    GOSUB errorprinter
    PRINT "...DONE!"
    GOSUB consolequit
END IF
PRINT
IF title$ <> "" THEN
    PRINT title$; " will now close."
ELSE
    PRINT "VaME will now close."
END IF
END

errorduringerror:
REM if error handler encounters an error
BEEP
PRINT "=== SUPER GURU ==="
PRINT "ERROR MANAGER HAS CRASHED!"
PRINT DATE$, TIME$
PRINT "ERROR CODE: "; ERR
PRINT "LINE: "; _ERRORLINE
PRINT errdescription$: PRINT _errormessage$
PRINT "ERROR INFO WILL NOT BE DUMPED TO FILE."
PRINT
IF title$ <> "" THEN
    PRINT title$; " will now close."
ELSE
    PRINT "VaME will now close."
END IF
END

fadein:
REM fade in utility
IF fadestatus = 1 THEN _AUTODISPLAY: RETURN: REM return if fade already on
IF disablefade = 1 THEN _AUTODISPLAY: RETURN: REM return for if disablefade is on.
FOR i% = 255 TO 0 STEP -5
    _LIMIT fadespeed: REM sets framerate
    GOSUB screendraw: REM draws screen
    LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, i%), BF: REM slowly fills screen with black box
    GOSUB timeframecounter: REM timer function
    IF speedrun > 0 THEN GOSUB displayspeedrun
    _DISPLAY
NEXT
_AUTODISPLAY
REM print to console
LET eventtitle$ = "DISPLAY EFFECT:"
LET eventdata$ = "fade in"
LET eventnumber = 0
GOSUB consoleprinter
LET fadestatus = 1
RETURN

slowfadein:
REM slow fade in utility
IF fadestatus = 1 THEN _AUTODISPLAY: RETURN: REM return if fade already on
IF disablefade = 1 THEN _AUTODISPLAY: RETURN: REM return for if disablefade is on.
FOR i% = 255 TO 0 STEP -5
    _LIMIT fadespeed: REM sets framerate
    GOSUB screendraw: REM draws screen
    LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, i%), BF: REM slowly fills screen with black box
    GOSUB timeframecounter: REM timer function
    IF speedrun > 0 THEN GOSUB displayspeedrun
    _DISPLAY
    _DELAY 0.5
NEXT
_AUTODISPLAY
REM print to console
LET eventtitle$ = "DISPLAY EFFECT:"
LET eventdata$ = "slow fade in"
LET eventnumber = 0
GOSUB consoleprinter
LET fadestatus = 1
RETURN

slightfadein:
REM slight fade in utility
IF fadestatus = 1 THEN _AUTODISPLAY: RETURN: REM return if fade already on
IF disablefade = 1 THEN _AUTODISPLAY: RETURN: REM return for if disablefade is on.
FOR i% = (255 / 2) TO 0 STEP -5
    _LIMIT fadespeed: REM sets framerate
    GOSUB screendraw: REM draws screen
    LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, i%), BF: REM slowly fills screen with black box
    GOSUB timeframecounter: REM timer function
    IF speedrun > 0 THEN GOSUB displayspeedrun
    _DISPLAY
NEXT
_AUTODISPLAY
REM print to console
LET eventtitle$ = "DISPLAY EFFECT:"
LET eventdata$ = "undim screen"
LET eventnumber = 0
GOSUB consoleprinter
LET fadestatus = 1
RETURN

fadeout:
REM fade out utility
IF fadestatus = 0 THEN _AUTODISPLAY: RETURN: REM return if fade already off
IF disablefade = 1 THEN _AUTODISPLAY: RETURN: REM return for if disablefade is on.
FOR i% = 0 TO 255 STEP 5
    _LIMIT fadespeed: REM sets framerate
    GOSUB screendraw: REM draws screen
    LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, i%), BF: REM slowly empties black box from screen
    GOSUB timeframecounter: REM timer function
    IF speedrun > 0 THEN GOSUB displayspeedrun
    _DISPLAY
NEXT
_AUTODISPLAY
REM print to console
LET eventtitle$ = "DISPLAY EFFECT:"
LET eventdata$ = "fade out"
LET eventnumber = 0
GOSUB consoleprinter
LET fadestatus = 0
RETURN

slowfadeout:
REM slow fade out utility
IF fadestatus = 0 THEN _AUTODISPLAY: RETURN: REM return if fade already off
IF disablefade = 1 THEN _AUTODISPLAY: RETURN: REM return for if disablefade is on.
FOR i% = 0 TO 255 STEP 5
    _LIMIT fadespeed: REM sets framerate
    GOSUB screendraw: REM draws screen
    LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, i%), BF: REM slowly empties black box from screen
    GOSUB timeframecounter: REM timer function
    IF speedrun > 0 THEN GOSUB displayspeedrun
    _DISPLAY
    _DELAY 0.5
NEXT
_AUTODISPLAY
REM print to console
LET eventtitle$ = "DISPLAY EFFECT:"
LET eventdata$ = "slow fade out"
LET eventnumber = 0
GOSUB consoleprinter
LET fadestatus = 0
RETURN

slightfadeout:
REM slight fade out utility
IF fadestatus = 0 THEN _AUTODISPLAY: RETURN: REM return if fade already off
IF disablefade = 1 THEN _AUTODISPLAY: RETURN: REM return for if disablefade is on.
FOR i% = 0 TO (255 / 2) STEP 5
    _LIMIT fadespeed: REM sets framerate
    GOSUB screendraw: REM draws screen
    LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, i%), BF: REM slowly empties black box from screen
    GOSUB timeframecounter: REM timer function
    IF speedrun > 0 THEN GOSUB displayspeedrun
    _DISPLAY
NEXT
_AUTODISPLAY
REM print to console
LET eventtitle$ = "DISPLAY EFFECT:"
LET eventdata$ = "dim screen"
LET eventnumber = 0
GOSUB consoleprinter
LET fadestatus = 0
RETURN

fontunload:
REM unloads engine fonts if used (will cause Invalid Handle Error (258) when font data is changed via prompt)
_FONT 8: REM sets font to QB64 default
REM unloads any loaded fonts
IF headerfontname$ <> "" THEN _FREEFONT headerfont&
IF defaultfontname$ <> "" THEN _FREEFONT defaultfont&
IF smallfontname$ <> "" THEN _FREEFONT smallfont&
REM prints to console
DO
    LET temp23 = temp23 + 1
    LET eventtitle$ = "FONT UNLOADED:"
    IF temp23 = 1 THEN LET eventdata$ = headerfontname$: LET eventnumber = headerfontsize
    IF temp23 = 2 THEN LET eventdata$ = defaultfontname$: LET eventnumber = defaultfontsize
    IF temp23 = 3 THEN LET eventdata$ = smallfontname$: LET eventnumber = smallfontsize
    REM if no font set
    IF eventdata$ = "" THEN
        IF temp23 = 1 THEN LET eventtitle$ = "HEADER FONT NOT SELECTED:"
        IF temp23 = 2 THEN LET eventtitle$ = "DEFAULT FONT NOT SELECTED:"
        IF temp23 = 3 THEN LET eventtitle$ = "SMALL FONT NOT SELECTED:"
        LET eventdata$ = "no font to unload"
        LET eventnumber = 0
    END IF
    GOSUB consoleprinter
LOOP UNTIL temp23 = 3
LET temp23 = 0: REM scrub temp values
RETURN

fontload:
REM loads engine fonts
IF setupboot = 0 THEN GOSUB fontunload: REM unloads font
LET temp8 = 1: REM sets temp values
DO
    REM sets up more temp files
    IF temp8 = 1 THEN LET temp7$ = headerfontname$: LET temp9 = headerfontsize: LET temp8$ = headerfontstyle$
    IF temp8 = 2 THEN LET temp7$ = defaultfontname$: LET temp9 = defaultfontsize: LET temp8$ = defaultfontstyle$
    IF temp8 = 3 THEN LET temp7$ = smallfontname$: LET temp9 = smallfontsize: LET temp8$ = smallfontstule$
    IF temp7$ <> "" THEN
        REM if font chosen
        IF temp8 = 1 THEN
            IF temp8$ <> "" THEN
                LET headerfont& = _LOADFONT(floc$ + temp7$, temp9, temp8$)
            ELSE
                LET headerfont& = _LOADFONT(floc$ + temp7$, temp9)
            END IF
        END IF
        IF temp8 = 2 THEN
            IF temp8$ <> "" THEN
                LET defaultfont& = _LOADFONT(floc$ + temp7$, temp9, temp8$)
            ELSE
                LET defaultfont& = _LOADFONT(floc$ + temp7$, temp9)
            END IF
        END IF
        IF temp8 = 3 THEN
            IF temp8$ <> "" THEN
                LET smallfont& = _LOADFONT(floc$ + temp7$, temp9, temp8$)
            ELSE
                LET smallfont& = _LOADFONT(floc$ + temp7$, temp9)
            END IF
        END IF
        IF temp8 = 1 THEN LET eventtitle$ = "HEADER FONT LOADED:"
        IF temp8 = 2 THEN LET eventtitle$ = "DEFAULT FONT LOADED:"
        IF temp8 = 3 THEN LET eventtitle$ = "SMALL FONT LOADED:"
        LET eventdata$ = temp7$ + temp8$
        LET eventnumber = temp9
        GOSUB consoleprinter
    ELSE
        REM font not chosen
        IF temp8 = 1 THEN LET evenetitle$ = "HEADER FONT NOT SELECTED:"
        IF temp8 = 2 THEN LET eventtitle$ = "DEFAULT FONT NOT SELECTED:"
        IF temp8 = 3 THEN LET eventtitle$ = "SMALL FONT NOT SELECTED:"
        LET eventdata$ = "will use engine default"
        GOSUB consoleprinter
    END IF
    LET temp8 = temp8 + 1
    IF setupboot = 1 THEN
        REM loading bar
        LET temp125 = temp125 + 3
        GOSUB loadbar
    END IF
LOOP UNTIL temp8 = 4
IF setupboot = 1 THEN
    REM loading bar
    LET temp125 = 15
    GOSUB loadbar
END IF
LET temp8 = 0: LET temp9 = 0: LET temp7$ = "": LET temp8$ = "": REM scrub temp values
RETURN

setheaderfont:
REM sets font to header
IF headerfontname$ <> "" THEN
    _FONT headerfont&
    LET eventtitle$ = "HEADER FONT SET:"
    LET eventdata$ = headerfontname$
    LET eventnumber = headerfontsize
    GOSUB consoleprinter
ELSE
    _FONT 16
    LET eventtitle$ = "HEADER FONT SET:"
    LET eventdata$ = "engine default"
    LET eventnumber = 16
    GOSUB consoleprinter
END IF
LET fontmode = 1
RETURN

setsmallfont:
REM sets font to small
IF smallfontname$ <> "" THEN
    _FONT smallfont&
    LET eventtitle$ = "SMALL FONT SET:"
    LET eventdata$ = smallfontname$
    LET eventnumber = smallfontsize
    GOSUB consoleprinter
ELSE
    _FONT 8
    LET eventtitle$ = "SMALL FONT SET:"
    LET eventdata$ = "engine default"
    LET eventnumber = 8
    GOSUB consoleprinter
END IF
LET fontmode = 3
RETURN

setdefaultfont:
REM sets font to default
IF defaultfontname$ <> "" THEN
    _FONT defaultfont&
    LET eventtitle$ = "DEFAULT FONT SET:"
    LET eventdata$ = defaultfontname$
    LET eventnumber = defaultfontsize
    GOSUB consoleprinter
ELSE
    _FONT 8
    LET eventtitle$ = "DEFAULT FONT SET:"
    LET eventdata$ = "engine default"
    LET eventnumber = 8
    GOSUB consoleprinter
END IF
LET fontmode = 2
RETURN

mainplayerunload:
REM unloads main player sprites from memory
_FREEIMAGE mpf: _FREEIMAGE mpb: _FREEIMAGE mpl: _FREEIMAGE mpr
_FREEIMAGE mpfl: _FREEIMAGE mpfr
_FREEIMAGE mpbl: _FREEIMAGE mpbr
_FREEIMAGE mpll: _FREEIMAGE mplr
_FREEIMAGE mprl: _FREEIMAGE mprr
_FREEIMAGE mpface1: _FREEIMAGE mpface2
_FREEIMAGE mpfi1: _FREEIMAGE mpfi2
_FREEIMAGE mpbi1: _FREEIMAGE mpbi2
_FREEIMAGE mpli1: _FREEIMAGE mpli2
_FREEIMAGE mpri1: _FREEIMAGE mpri2
REM informs console printer
LET eventtitle$ = "MAIN PLAYER UNLOADED:"
IF userquit = 1 THEN
    LET eventdata$ = mplayermodel$
ELSE
    LET eventdata$ = oldmplayermodel$
END IF
LET eventnumber = 0
GOSUB consoleprinter
RETURN

mapunload:
REM unloads map sprites from memory
_FREEIMAGE mapa: _FREEIMAGE mapb
REM informs console printer
LET eventtitle$ = "MAP UNLOADED:"
IF userquit = 1 THEN
    LET eventdata$ = mapname$
    LET eventnumber = mapno
ELSE
    LET eventdata$ = oldmapname$
    LET eventnumber = oldmapno
END IF
GOSUB consoleprinter
RETURN

collision:
REM collision sub manager
IF noclip = 1 THEN RETURN: REM return if no clip is on
GOSUB mapcollision: REM map sprite boundaries
GOSUB objectcollision: REM object sprite boundaries and selection
GOSUB playercollision: REM NPC collision
GOSUB triggercollision: REM invisible trigger collision
RETURN

errorprinter:
REM dumps error information to file
IF consolelogging = 1 THEN
    OPEN consolelog$ FOR APPEND AS #2
    IF errdescription$ = "" THEN 
		PRINT #2, DATE$, TIME$, "ERROR: "; ERR, "LINE: "; _ERRORLINE, _errormessage$
	ELSE
		PRINT #2, DATE$, TIME$, "ERROR: "; ERR, "LINE: "; _ERRORLINE, errdescription$
	END IF
    CLOSE #2
END IF
REM PRINTS TO CONSOLE
IF displayconsole = 1 THEN
    _DEST _CONSOLE
    IF errdescription$ = "" THEN
		PRINT DATE$, TIME$, "ERROR: "; ERR, "LINE: "; _ERRORLINE, _errormessage$
    ELSE
		PRINT DATE$, TIME$, "ERROR: "; ERR, "LINE: "; _ERRORLINE, errdescription$
	END IF
    _DEST 0
END IF
RETURN

playercollision:
REM handles NPC collision
IF mapplayerno = 0 THEN RETURN: REM return if no players attached to map
DO
    LET temp44 = temp44 + 1
    IF (resx / 2) + ((mpx / 2) - objectstep) >= playerx(temp44) + posx AND (resx / 2) - ((mpx / 2) - (objectstep * 2)) <= (playerx(temp44) + playerresx(temp44)) + posx THEN LET temp45 = temp45 + 1
    IF (resy / 2) - ((mpy / 2) - players(temp44)) >= playery(temp44) + posy AND (resy / 2) + (mpy / 2) <= (playery(temp44) + playerresy(temp44)) + posy THEN LET temp45 = temp45 + 1
    LET proposedobject$ = playername(temp44)
    GOSUB playercollisionchanger
LOOP UNTIL temp44 >= mapplayerno OR temp44 >= totalplayers
LET temp44 = 0: REM scrub temp values
RETURN

playercollisionchanger:
REM changes position values if player is colliding with NPC
IF temp45 = 2 THEN
    REM tells collsion printer
    IF direction = 1 THEN LET temp4$ = STR$(posy): LET temp5$ = STR$(posy - pace): LET temp6$ = "Y"
    IF direction = 2 THEN LET temp4$ = STR$(posy): LET temp5$ = STR$(posy + pace): LET temp6$ = "Y"
    IF direction = 3 THEN LET temp4$ = STR$(posx): LET temp5$ = STR$(posx + pace): LET temp6$ = "X"
    IF direction = 4 THEN LET temp4$ = STR$(posx): LET temp5$ = STR$(posx - pace): LET temp6$ = "X"
    IF direction = 1 THEN IF (posy - pace) <> temp47 THEN GOSUB collisionprinter
    IF direction = 2 THEN IF (posy + pace) <> temp47 THEN GOSUB collisionprinter
    IF direction = 3 THEN IF (posx + pace) <> temp46 THEN GOSUB collisionprinter
    IF direction = 4 THEN IF (posx - pace) <> temp46 THEN GOSUB collisionprinter
    REM changes position
    IF direction = 1 THEN LET posy = posy - pace
    IF direction = 2 THEN LET posy = posy + pace
    IF direction = 3 THEN LET posx = posx + pace
    IF direction = 4 THEN LET posx = posx - pace
    REM tells engine what object has been collided
    LET selectobject$ = proposedobject$
    LET objecttype$ = "NPC"
    REM scrubs temp values / assigns temp values
    LET temp4$ = "": LET temp5$ = "": LET temp46 = posx: LET temp47 = posy
END IF
LET temp45 = 0: REM scrubs temp values
RETURN

triggercollision:
REM handles trigger collision and selection
IF maptriggerno = 0 THEN RETURN: REM return if map has no triggers attached
DO
    LET temp24 = temp24 + 1
    IF (resx / 2) - ((mpx / 2) - objectstep) >= triggerx1(temp24) + posx AND (resx / 2) + ((mpx / 2) - (objectstep * 2)) <= triggerx2(temp24) + posx THEN LET temp25 = temp25 + 1
    IF (resy / 2) + (mpy / 2) >= triggery1(temp24) + posy AND (resy / 2) + (mpy / 2) <= triggery2(temp24) + posy THEN LET temp25 = temp25 + 1
    GOSUB triggercollisionchanger
LOOP UNTIL temp24 >= maptriggerno OR temp24 >= totaltriggers
LET temp24 = 0: REM scrub temp values
RETURN

triggercollisionchanger:
REM sets trigger activated values if trigger has been collided
IF temp25 = 2 THEN
    LET triggera(temp24) = 1: LET eventdata$ = triggername(temp24)
    LET eventtitle$ = "TRIGGER ACVTIVE:"
    LET eventnumber = 0
    GOSUB consoleprinter
END IF
LET temp25 = 0: REM scrubs temp values
RETURN

objectcollision:
REM handles object collision and selection
IF mapobjectno = 0 THEN RETURN: REM return if map has no objects attached
DO
    LET temp18 = temp18 + 1
    IF (resx / 2) + ((mpx / 2) - objectstep) >= objectx(temp18) + posx AND (resx / 2) - ((mpx / 2) - (objectstep * 2)) <= (objectx(temp18) + objectresx(temp18)) + posx THEN LET temp17 = temp17 + 1
    IF (resy / 2) - ((mpy / 2) - objects(temp18)) >= objecty(temp18) + posy AND (resy / 2) + (mpy / 2) <= (objecty(temp18) + objectresy(temp18)) + posy THEN LET temp17 = temp17 + 1
    LET proposedobject$ = objectname(temp18)
    GOSUB objectcollisionchanger
LOOP UNTIL temp18 >= mapobjectno OR temp18 >= totalobjects
LET temp18 = 0: REM scrub temp values
RETURN

objectcollisionchanger:
REM changes position values if player is colliding with object
IF temp17 = 2 THEN
    REM tells collsion printer
    IF direction = 1 THEN LET temp4$ = STR$(posy): LET temp5$ = STR$(posy - pace): LET temp6$ = "Y"
    IF direction = 2 THEN LET temp4$ = STR$(posy): LET temp5$ = STR$(posy + pace): LET temp6$ = "Y"
    IF direction = 3 THEN LET temp4$ = STR$(posx): LET temp5$ = STR$(posx + pace): LET temp6$ = "X"
    IF direction = 4 THEN LET temp4$ = STR$(posx): LET temp5$ = STR$(posx - pace): LET temp6$ = "X"
    IF direction = 1 THEN IF (posy - pace) <> temp20 THEN GOSUB collisionprinter
    IF direction = 2 THEN IF (posy + pace) <> temp20 THEN GOSUB collisionprinter
    IF direction = 3 THEN IF (posx + pace) <> temp19 THEN GOSUB collisionprinter
    IF direction = 4 THEN IF (posx - pace) <> temp19 THEN GOSUB collisionprinter
    REM changes position
    IF direction = 1 THEN LET posy = posy - pace
    IF direction = 2 THEN LET posy = posy + pace
    IF direction = 3 THEN LET posx = posx + pace
    IF direction = 4 THEN LET posx = posx - pace
    REM tells engine what object has been collided
    LET selectobject$ = proposedobject$
    LET objecttype$ = "OBJ"
    REM scrubs temp values / assigns temp values
    LET temp4$ = "": LET temp5$ = "": LET temp19 = posx: LET temp20 = posy
END IF
LET temp17 = 0: REM scrubs temp values
RETURN

mapcollision:
REM map sprite boundaries
REM top corner
REM X
IF posx >= (resx / 2) - (mpx / 2) THEN
    LET temp4$ = STR$(posx)
    LET posx = (resx / 2) - (mpx / 2)
    LET temp5$ = STR$(posx)
    LET temp6$ = "X"
    IF temp21 <> posx THEN GOSUB collisionprinter
    LET temp21 = posx
END IF
REM Y
IF posy >= (resy / 2) - (mpy / 2) THEN
    LET temp4$ = STR$(posy)
    LET posy = (resy / 2) - (mpy / 2)
    LET temp5$ = STR$(posy)
    LET temp6$ = "Y"
    IF temp22 <> posy THEN GOSUB collisionprinter
    LET temp22 = posy
END IF
REM bottom corner
REM X
IF posx <= ((resx / 2) + (mpx / 2)) - mapx THEN
    LET temp4$ = STR$(posx)
    LET posx = ((resx / 2) + (mpx / 2)) - mapx
    LET temp5$ = STR$(posx)
    LET temp6$ = "X"
    IF temp21 <> posx THEN GOSUB collisionprinter
    LET temp21 = posx
END IF
REM Y
IF posy <= ((resy / 2) + (mpy / 2)) - mapy THEN
    LET temp4$ = STR$(posy)
    LET posy = ((resy / 2) + (mpy / 2)) - mapy
    LET temp5$ = STR$(posy)
    LET temp6$ = "Y"
    IF temp22 <> posy THEN GOSUB collisionprinter
    LET temp22 = posy
END IF
REM scrub temp values
LET temp4$ = "": LET temp5$ = "": LET temp6$ = ""
RETURN

collisionprinter:
REM console printer for object collision
REM return for if same collision has been printed
REM prints collision conflict to printer
LET eventtitle$ = "COLLISION CONFLICT:"
LET eventdata$ = temp6$ + temp4$ + " TO " + temp6$ + temp5$
LET eventnumber = 0
GOSUB consoleprinter
REM temp values used scrubbed in mapcollision / objectcollisionchanger
RETURN

updatechecker:
REM automatically checks for updates
IF exitsave = 1 AND setupboot = 0 THEN GOSUB savesave: REM saves game if needed
CLS
REM checks to see if engine is in developer mode
LET finddev% = INSTR(finddev% + 1, versionno$, "DEV")
LET find3rdparty% = INSTR(find3rdparty% + 1, versionno$, "3RDPARTY")
IF finddev% THEN
    REM disables update for developer build
    LET finddev% = 0
    IF fixvame = 1 THEN
        LET eventtitle$ = "ENGINE REPAIR:"
        IF mainmenu = 1 THEN LET textspeech$ = "Engine repair blocked due to developer build!"
    ELSE
        LET eventtitle$ = "UPDATE CHECK:"
        IF mainmenu = 1 THEN LET textspeech$ = "Update blocked due to developer build!"
    END IF
    LET eventdata$ = "Blocked due to developer build!"
    LET eventnumber = 0
    GOSUB consoleprinter
    IF mainmenu = 1 THEN GOSUB textbannerdraw
    RETURN
END IF
IF find3rdparty% THEN
	REM disables update for versions on 3rd party stores such as steam
    LET find3rdparty% = 0
    LET eventtitle$ = "UPDATE CHECK:"
    LET eventdata$ = "Blocked due to 3rd party store build!"
    LET eventnumber = 0
    GOSUB consoleprinter
    RETURN
END IF
REM checks for available updates
IF fixvame = 1 THEN
    LET eventtitle$ = "ENGINE REPAIR:"
ELSE
    LET eventtitle$ = "UPDATE CHECK:"
END IF
LET eventdata$ = "Initialised!"
LET eventnumber = 0
GOSUB consoleprinter
REM downloads update info file
IF ros$ = "mac" THEN SHELL _HIDE "curl -O " + updatelink$
IF ros$ = "lnx" THEN SHELL _HIDE "wget -q " + updatelink$
IF ros$ = "win" THEN
    SHELL _HIDE "copy data\utility\windownloader.bat windownloader.bat"
    SHELL _HIDE "windownloader.bat " + updatelink$ + " checkupdate.ddf"
END IF
REM checks update file
IF _FILEEXISTS("checkupdate.ddf") THEN
    REM file exists
    OPEN "checkupdate.ddf" FOR INPUT AS #1
    INPUT #1, newversionno$, updaterlinkmac$, updaterlinklnx$, updaterlinkwin$, downloadlink$, windownload$, macdownload$, lnxdownload$, unziplink$, updatetype, updatefolder$, updatewinexe$, updatelinuxexe$, updatemacosexe$, updatereadme$, updatechangelog$, updatemanual$, updatesource$, updateupdatersource$, updateupdaterzip2$, updateupdaterzip$
    CLOSE #1
    IF fixvame = 1 THEN LET newversionno$ = "FIX VAME"
    IF newversionno$ <> versionno$ THEN
        REM new version available
        IF fixvame = 1 THEN
            LET textspeech$ = title$ + " has found a potential fix!"
        ELSE
            LET textspeech$ = "A new version of " + title$ + " is available!"
        END IF
        GOSUB textbannerdraw
        CLS
        IF fixvame = 1 THEN
            LET eventtitle$ = "ENGINE REPAIR:"
            LET eventdata$ = "solution found!"
            LET eventnumber = 0
        ELSE
            LET eventtitle$ = "UPDATE CHECK:"
            LET eventdata$ = "Update to " + newversionno$ + " available!"
            LET eventnumber = 0
        END IF
        GOSUB consoleprinter
        IF soundmode = 2 OR soundmode = 3 THEN GOSUB musicstop: REM stops music if needed
        LET soundmode = 1
        IF fixvame = 1 THEN
            LET eventtitle$ = "ENGINE REPAIR:"
            LET eventdata$ = "Downloading repair file!"
            LET eventnumber = 0
        ELSE
            LET eventtitle$ = "UPDATE CHECK:"
            LET eventdata$ = "Downloading " + newversionno$ + " updater!"
            LET eventnumber = 0
        END IF
        GOSUB consoleprinter
        REM download updater
        _PUTIMAGE (1, 1)-(1 + loadiconresx, 1 + loadiconresy), loadicon
        IF ros$ = "mac" THEN
            LET temp29$ = updateupdaterzip$ + "_macos"
            SHELL _HIDE "curl -O " + updaterlinkmac$
        END IF
        IF ros$ = "lnx" THEN
            LET temp29$ = updateupdaterzip$ + "_linux"
            SHELL _HIDE "wget -q " + updaterlinklnx$
        END IF
        IF ros$ = "win" THEN
            LET temp29$ = updateupdaterzip$ + "_win.exe"
            SHELL _HIDE "windownloader.bat " + updaterlinkwin$ + " " + temp29$
        END IF
        CLS
        IF _FILEEXISTS(temp29$) THEN
            REM close engine... mark system as being updated
            LET runupdate = 1
            GOSUB endgame
            REM run updater
            IF fixvame = 1 THEN
                LET eventtitle$ = "ENGINE REPAIR:"
                LET eventdata$ = "Launching " + title$ + " repair!"
                LET eventnumber = 0
            ELSE
                LET eventtitle$ = "UPDATE CHECK:"
                LET eventdata$ = "Launching " + newversionno$ + " updater!"
                LET eventnumber = 0
            END IF
            GOSUB consoleprinter
            IF ros$ = "mac" THEN SHELL _HIDE "chmod 755 " + temp29$: SHELL _DONTWAIT "./" + temp29$
            IF ros$ = "lnx" THEN SHELL _HIDE "chmod +x " + temp29$: SHELL _DONTWAIT "./" + temp29$
            IF ros$ = "win" THEN SHELL _DONTWAIT temp29$
            SYSTEM
        ELSE
            REM updater download failed!
            IF vamefix = 1 THEN LET textspeech$ = "Failed to repair " + title$: CLS: GOSUB textbannerdraw
            IF vamefix = 1 THEN
                LET eventtitle$ = "ENGINE REPAIR FAILED!"
                LET eventdata$ = ""
                LET eventnumber = 0
                IF mainmenu = 1 THEN LET textspeech$ = "Failed to download fixes!"
            ELSE
                LET eventtitle$ = "UPDATE CHECK:"
                LET eventdata$ = "Failed to download updater!"
                LET eventnumber = 0
                IF mainmenu = 1 THEN LET textspeech$ = "Failed to download updater!"
            END IF
            GOSUB consoleprinter
            IF mainmenu = 1 THEN GOSUB textbannerdraw
            IF ros$ = "mac" OR ros$ = "lnx" THEN SHELL _HIDE "rm checkupdate.ddf"
            IF ros$ = "win" THEN SHELL _HIDE "del checkupdate.ddf": SHELL _HIDE "del windownloader.bat"
        END IF
    ELSE
        REM up to date
        LET eventtitle$ = "UPDATE CHECK:"
        LET eventdata$ = versionno$ + " is the current version!"
        LET eventnumber = 0
        GOSUB consoleprinter
        IF mainmenu = 1 THEN
			LET textspeech$ = title$ + " is up to date!"
			GOSUB textbannerdraw
        END IF
        IF ros$ = "mac" OR ros$ = "lnx" THEN SHELL _HIDE "rm checkupdate.ddf"
        IF ros$ = "win" THEN SHELL _HIDE "del checkupdate.ddf": SHELL _HIDE "del windownloader.bat"
    END IF
ELSE
    REM cannot download update file
    LET eventtitle$ = "UPDATE CHECK:"
    LET eventdata$ = "Cannot connect to update server!"
    LET eventnumber = 0
    GOSUB consoleprinter
    IF mainmenu = 1 THEN
		LET textspeech$ = "Cannot connect to update server!"
		GOSUB textbannerdraw
    END IF
    IF ros$ = "win" THEN SHELL _HIDE "del windownloader.bat"
END IF
RETURN

mainplayerload:
REM loads player data and sprites
REM unload divert if map has changed and if system is not booting
IF setupboot = 0 THEN IF mplayermodel$ <> oldmplayermodel$ THEN GOSUB mainplayerunload
IF oldmplayermodel$ = mplayermodel$ THEN RETURN: REM divert for if playermodel hasn't actually changed
REM load data and sprites
OPEN ploc$ + mplayermodel$ + "/" + mplayermodel$ + ".ddf" FOR INPUT AS #1
INPUT #1, temp$, mpx, mpy, mps, mpnote1, mpnote2
CLOSE #1
LET mpf = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-f.png")
LET mpfl = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-fl.png")
LET mpfr = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-fr.png")
LET mpb = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-b.png")
LET mpbl = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-bl.png")
LET mpbr = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-br.png")
LET mpr = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-r.png")
LET mprl = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-rl.png")
LET mprr = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-rr.png")
LET mpl = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-l.png")
LET mpll = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-ll.png")
LET mplr = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-lr.png")
LET mpfi1 = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-fi1.png")
LET mpfi2 = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-fi2.png")
LET mpbi1 = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-bi1.png")
LET mpbi2 = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-bi2.png")
LET mpli1 = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-li1.png")
LET mpli2 = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-li2.png")
LET mpri1 = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-ri1.png")
LET mpri2 = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-ri2.png")
LET mpface1 = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-face1.png")
LET mpface2 = _LOADIMAGE(ploc$ + mplayermodel$ + "/" + mplayermodel$ + "-face2.png")
REM calculates player sprite location
LET mpposx = (resx / 2) - (mpx / 2)
LET mpposy = (resy / 2) - (mpy / 2)
REM sets foot value
LET mpfoot = 1
REM tells console of load
LET eventtitle$ = "MAIN PLAYER LOADED: "
LET eventdata$ = mplayermodel$
LET eventnumber = 0
GOSUB consoleprinter
LET temp$ = ""
RETURN

controlgenerator:
REM generates control text
LET textspeech$ = "": REM wipes text speech
IF ucontrol = 1 AND dcontrol = 1 AND lcontrol = 1 AND rcontrol = 1 THEN
    LET textspeech$ = "ARROW KEYS - MOVE. "
ELSE
    IF ucontrol = 1 THEN LET textspeech$ = "UP KEY - MOVE UP. "
    IF dcontrol = 1 THEN LET textspeech$ = textspeech$ + "DOWN KEY - MOVE DOWN. "
    IF lcontrol = 1 THEN LET textspeech$ = textspeech$ + "LEFT KEY - MOVE LEFT. "
    IF rcontrol = 1 THEN LET textspeech$ = textspeech$ + "RIGHT KEY - MOVE RIGHT. "
END IF
IF scontrol = 1 THEN LET textspeech$ = textspeech$ + "SELECT - SPACE. "
IF pcontrol = 1 THEN LET textspeech$ = textspeech$ + "POCKETS - I. "
IF bcontrol = 1 THEN LET textspeech$ = textspeech$ + "PAUSE/BACK - Q."
RETURN

menugenerator:
REM menu presented to player
IF menu$ = "" THEN
    REM return for if no menu name specified
    LET eventtitle$ = "NO MENU NAME SPECIFIED"
    LET eventdata$ = "MENU NOT LOADED"
    LET eventnumber = 0
    GOSUB consoleprinter
    RETURN
END IF
LET mainmenu = 1: REM tells engine menu is active
CLS
REM menu loop
OPEN menuloc$ + menu$ + ".ddf" FOR INPUT AS #1
INPUT #1, menuchoice1$, menuchoice2$, menuchoice3$, menuchoice4$, menuchoice5$, menuchoice6$, menucommand1$, menucommand2$, menucommand3$, menucommand4$, menucommand5$, menucommand6$, mcy1, mcy2, mcy3, mcy4, mcy5, mcy6, menunos, menuposx, menubackdrop$, menumusic$
CLOSE #1
REM console dump
LET eventtitle$ = "MENU LOADED:"
LET eventdata$ = menu$
LET eventnumber = 0
GOSUB consoleprinter
REM plays music
LET playmusic$ = menumusic$
GOSUB musicplay
LET menubackdrop = _LOADIMAGE(menuloc$ + menubackdrop$ + ".png")
_PUTIMAGE (0, 0)-(resx, resy), menubackdrop
LET temp78 = 1
DO
    _LIMIT hertz
    DO
        LET d$ = UCASE$(INKEY$)
        DO
			_LIMIT 1000
            REM centralises text
            LET temp77 = temp77 + 1
            IF temp77 = 1 THEN
                IF menuchoice1$ <> "" THEN
                    LET centretext$ = menuchoice1$
                    IF menuposx = -1 THEN GOSUB centretext
                    LET temp76 = 1
                END IF
            END IF
            IF temp77 = 2 THEN
                IF menuchoice2$ <> "" THEN
                    LET centretext$ = menuchoice2$
                    IF menuposx = -1 THEN GOSUB centretext
                    LET temp76 = 1
                END IF
            END IF
            IF temp77 = 3 THEN
                IF menuchoice3$ <> "" THEN
                    LET centretext$ = menuchoice3$
                    IF menuposx = -1 THEN GOSUB centretext
                    LET temp76 = 1
                END IF
            END IF
            IF temp77 = 4 THEN
                IF menuchoice4$ <> "" THEN
                    LET centretext$ = menuchoice4$
                    IF menuposx = -1 THEN GOSUB centretext
                    LET temp76 = 1
                END IF
            END IF
            IF temp77 = 5 THEN
                IF menuchoice5$ <> "" THEN
                    LET centretext$ = menuchoice5$
                    IF menuposx = -1 THEN GOSUB centretext
                    LET temp76 = 1
                END IF
            END IF
            IF temp77 = 6 THEN
                IF menuchoice6$ <> "" THEN
                    LET centretext$ = menuchoice6$
                    IF menuposx = -1 THEN GOSUB centretext
                    LET temp76 = 1
                END IF
            END IF
            IF temp77 > 6 THEN LET temp77 = 0
        LOOP UNTIL temp76 = 1
        REM prints text
        IF temp78 = 1 THEN
            COLOR _RGBA(letmenuselectcolourr, letmenuselectcolourg, letmenuselectcolourb, letmenuselectcoloura), _RGBA(bgmenuselectcolourr, bgmenuselectcolourg, bgmenuselectcolourb, bgmenuselectcoloura)
        ELSE
            COLOR _RGBA(letmenudefaultcolourr, letmenudefaultcolourg, letmenudefaultcolourb, letmenudefaultcoloura), _RGBA(bgmenudefaultcolourr, bgmenudefaultcolourg, bgmenudefaultcolourb, bgmenudefaultcoloura)
        END IF
        IF temp77 = 1 THEN
            IF menuposx = -1 THEN
                _PRINTSTRING ((resx / 2) - (centreno / 2), (mcy1)), menuchoice1$
            ELSE
                _PRINTSTRING (menuposx, mcy1), menuchoice1$
            END IF
        END IF
        IF temp78 = 2 THEN
            COLOR _RGBA(letmenuselectcolourr, letmenuselectcolourg, letmenuselectcolourb, letmenuselectcoloura), _RGBA(bgmenuselectcolourr, bgmenuselectcolourg, bgmenuselectcolourb, bgmenuselectcoloura)
        ELSE
            COLOR _RGBA(letmenudefaultcolourr, letmenudefaultcolourg, letmenudefaultcolourb, letmenudefaultcoloura), _RGBA(bgmenudefaultcolourr, bgmenudefaultcolourg, bgmenudefaultcolourb, bgmenudefaultcoloura)
        END IF
        IF temp77 = 2 THEN
            IF menuposx = -1 THEN
                _PRINTSTRING ((resx / 2) - (centreno / 2), (mcy2)), menuchoice2$
            ELSE
                _PRINTSTRING (menuposx, mcy2), menuchoice2$
            END IF
        END IF
        IF temp78 = 3 THEN
            COLOR _RGBA(letmenuselectcolourr, letmenuselectcolourg, letmenuselectcolourb, letmenuselectcoloura), _RGBA(bgmenuselectcolourr, bgmenuselectcolourg, bgmenuselectcolourb, bgmenuselectcoloura)
        ELSE
            COLOR _RGBA(letmenudefaultcolourr, letmenudefaultcolourg, letmenudefaultcolourb, letmenudefaultcoloura), _RGBA(bgmenudefaultcolourr, bgmenudefaultcolourg, bgmenudefaultcolourb, bgmenudefaultcoloura)
        END IF
        IF temp77 = 3 THEN
            IF menuposx = -1 THEN
                _PRINTSTRING ((resx / 2) - (centreno / 2), (mcy3)), menuchoice3$
            ELSE
                _PRINTSTRING (menuposx, mcy3), menuchoice3$
            END IF
        END IF
        IF temp78 = 4 THEN
            COLOR _RGBA(letmenuselectcolourr, letmenuselectcolourg, letmenuselectcolourb, letmenuselectcoloura), _RGBA(bgmenuselectcolourr, bgmenuselectcolourg, bgmenuselectcolourb, bgmenuselectcoloura)
        ELSE
            COLOR _RGBA(letmenudefaultcolourr, letmenudefaultcolourg, letmenudefaultcolourb, letmenudefaultcoloura), _RGBA(bgmenudefaultcolourr, bgmenudefaultcolourg, bgmenudefaultcolourb, bgmenudefaultcoloura)
        END IF
        IF temp77 = 4 THEN
            IF menuposx = -1 THEN
                _PRINTSTRING ((resx / 2) - (centreno / 2), (mcy4)), menuchoice4$
            ELSE
                _PRINTSTRING (menuposx, mcy4), menuchoice4$
            END IF
        END IF
        IF temp78 = 5 THEN
            COLOR _RGBA(letmenuselectcolourr, letmenuselectcolourg, letmenuselectcolourb, letmenuselectcoloura), _RGBA(bgmenuselectcolourr, bgmenuselectcolourg, bgmenuselectcolourb, bgmenuselectcoloura)
        ELSE
            COLOR _RGBA(letmenudefaultcolourr, letmenudefaultcolourg, letmenudefaultcolourb, letmenudefaultcoloura), _RGBA(bgmenudefaultcolourr, bgmenudefaultcolourg, bgmenudefaultcolourb, bgmenudefaultcoloura)
        END IF
        IF temp77 = 5 THEN
            IF menuposx = -1 THEN
                _PRINTSTRING ((resx / 2) - (centreno / 2), (mcy5)), menuchoice5$
            ELSE
                _PRINTSTRING (menuposx, mcy5), menuchoice5$
            END IF
        END IF
        IF temp78 = 6 THEN
            COLOR _RGBA(letmenuselectcolourr, letmenuselectcolourg, letmenuselectcolourb, letmenuselectcoloura), _RGBA(bgmenuselectcolourr, bgmenuselectcolourg, bgmenuselectcolourb, bgmenuselectcoloura)
        ELSE
            COLOR _RGBA(letmenudefaultcolourr, letmenudefaultcolourg, letmenudefaultcolourb, letmenudefaultcoloura), _RGBA(bgmenudefaultcolourr, bgmenudefaultcolourg, bgmenudefaultcolourb, bgmenudefaultcoloura)
        END IF
        LET temp76 = 0
        IF temp77 = 6 THEN
            IF menuposx = -1 THEN
                _PRINTSTRING ((resx / 2) - (centreno / 2), (mcy6)), menuchoice6$
            ELSE
                _PRINTSTRING (menuposx, mcy16), menuchoice6$
            END IF
        END IF
        IF d$ = CHR$(0) + CHR$(80) THEN
			IF temp78 + 1 =< menunos THEN
				LET playsfx$ = "move"
				GOSUB sfxplay
				LET temp78 = temp78 + 1
			END IF
		END IF
        IF d$ = CHR$(0) + CHR$(72) THEN 
			IF temp78 - 1 >= 1 THEN
				LET playsfx$ = "move"
				GOSUB sfxplay
				LET temp78 = temp78 - 1
			END IF
		END IF
        'IF temp78 < 1 THEN LET temp78 = 1
        'IF temp78 > menunos THEN LET temp78 = menunos
        IF temp79 > 0 THEN LET temp79 = 0
    LOOP UNTIL d$ = " "
    REM plays select sound
    LET playsfx$ = "select"
    GOSUB sfxplay
    REM deturmins which choice player made and lines up menu command
    IF temp78 = 1 THEN LET temp15$ = menucommand1$
    IF temp78 = 2 THEN LET temp15$ = menucommand2$
    IF temp78 = 3 THEN LET temp15$ = menucommand3$
    IF temp78 = 4 THEN LET temp15$ = menucommand4$
    IF temp78 = 5 THEN LET temp15$ = menucommand5$
    IF temp78 = 6 THEN LET temp15$ = menucommand6$
    IF temp15$ <> "playgame" THEN
        REM detects possible new menu request
        LET findmenu% = INSTR(findmenu% + 1, temp15$, "menu ")
        IF findmenu% THEN
            LET temp16$ = LEFT$(temp15$, INSTR(temp15$, " ") - 1)
            LET temp17$ = RIGHT$(temp15$, LEN(temp15$) - LEN(temp16$))
            LET temp17$ = LTRIM$(temp17$)
            LET temp16$ = LCASE$(temp16$)
            LET temp17$ = LCASE$(temp17$)
            CLS
            _FREEIMAGE menubackdrop
            LET menu$ = temp17$
            LET mainmenu = 0: LET temp76 = 1: LET temp77 = 0: LET temp78 = 0: LET findmenu% = 0: LET d$ = "": LET temp15$ = "": LET temp16$ = "": LET temp17$ = "": REM scrubs temp values
            GOTO menugenerator
        END IF
        REM executes menucommand
        IF temp15$ = "updategame" THEN 
			GOSUB updatechecker
			CLS
			_PUTIMAGE (0, 0)-(resx, resy), menubackdrop
		END IF
        IF temp15$ = "endgame" THEN
            FOR i% = 0 TO 255 STEP 5
                _LIMIT fadespeed: REM sets framerate
                LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, i%), BF: REM slowly empties black box from screen
                GOSUB timeframecounter: REM timer function
                _DISPLAY
            NEXT
            _AUTODISPLAY
            GOTO endgame
        END IF
        IF temp15$ = "erasesave" THEN CLS: GOSUB erasesave
        IF temp15$ = "musictoggle" THEN
            CLS
            GOSUB musictoggle
            CLS
            _PUTIMAGE (0, 0)-(resx, resy), menubackdrop
        END IF
        IF temp15$ = "sfxtoggle" THEN
            CLS
            GOSUB sfxtoggle
            CLS
            _PUTIMAGE (0, 0)-(resx, resy), menubackdrop
        END IF
        IF temp15$ = "screentoggle" THEN
            CLS
            GOSUB screentoggle
            CLS
            _PUTIMAGE (0, 0)-(resx, resy), menubackdrop
        END IF
        IF temp15$ = "displaycontrols" THEN
            GOSUB controlgenerator
            REM LET textspeech$ = "ARROW KEYS - MOVE. SPACE - USE. I - DISPLAY POCKETS. Q - BACK / PAUSE."
            CLS
            GOSUB textbannerdraw
            CLS
            _PUTIMAGE (0, 0)-(resx, resy), menubackdrop
        END IF
        IF temp15$ = "musicup" THEN
            CLS
            LET musicvol = musicvol + 0.1
            IF musicvol > 1 THEN LET musicvol = 1
            GOSUB musicvol
            _PUTIMAGE (0, 0)-(resx, resy), menubackdrop
        END IF
        IF temp15$ = "musicdown" THEN
            CLS
            LET musicvol = musicvol - 0.1
            IF musicvol < 0.1 THEN LET musicvol = 0.1
            GOSUB musicvol
            _PUTIMAGE (0, 0)-(resx, resy), menubackdrop
        END IF
        IF temp15$ = "sfxup" THEN
            CLS
            LET sfxvol = sfxvol + 0.1
            IF sfxvol > 1 THEN LET sfxvol = 1
            GOSUB sfxvol
            _PUTIMAGE (0, 0)-(resx, resy), menubackdrop
        END IF
        IF temp15$ = "sfxdown" THEN
            CLS
            LET sfxvol = sfxvol - 0.1
            IF sfxvol < 0.1 THEN LET sfxvol = 0.1
            GOSUB sfxvol
            _PUTIMAGE (0, 0)-(resx, resy), menubackdrop
        END IF
        IF temp15$ = "displayconsole" THEN
            CLS
            GOSUB displayconsole
            CLS
            _PUTIMAGE (0, 0)-(resx, resy), menubackdrop
        END IF
    ELSE
        REM executes playgame command
        CLS
        _FREEIMAGE menubackdrop
        LET menu$ = ""
        COLOR 0, 0
        LET mainmenu = 0: LET temp76 = 1: LET temp77 = 0: LET temp78 = 0: LET findmenu% = 0: LET d$ = "": LET temp15$ = "": LET temp16$ = "": LET temp17$ = "": LET temp29$ = "": REM scrubs temp values
        RETURN
    END IF
    LET temp15$ = ""
LOOP
RETURN

displayconsole:
REM displays developer console
IF displayconsole = 0 THEN
    _CONSOLE ON
    _CONSOLETITLE title$ + " Console"
    LET displayconsole = 1
    LET textspeech$ = "Console Switched On!"
    GOSUB textbannerdraw
    RETURN
END IF
IF displayconsole = 1 THEN
    _CONSOLE OFF
    LET displayconsole = 0
    LET textspeech$ = "Console Switched Off!"
    GOSUB textbannerdraw
    RETURN
END IF
RETURN

sfxtoggle:
REM toggles sfx
IF liteload = 1 THEN RETURN
REM turn sfx on
IF soundmode = 1 THEN
    LET soundmode = 4
    LET eventtitle$ = "SOUND MODE SET:"
    LET eventdata$ = "sfx on"
    LET eventnumber = soundmode
    GOSUB consoleprinter
    LET textspeech$ = "Sound Effects On!"
    GOSUB textbannerdraw
    RETURN
END IF
IF soundmode = 3 THEN
    LET soundmode = 2
    LET eventtitle$ = "SOUND MODE SET:"
    LET eventdata$ = "sfx on"
    LET eventnumber = soundmode
    GOSUB consoleprinter
    LET textspeech$ = "Sound Effects On!"
    GOSUB textbannerdraw
    RETURN
END IF
REM turn sfx off
IF soundmode = 2 THEN
    LET soundmode = 3
    LET eventtitle$ = "SOUND MODE SET:"
    LET eventdata$ = "sfx off"
    LET eventnumber = soundmode
    GOSUB consoleprinter
    LET textspeech$ = "Sound Effects Off!"
    GOSUB textbannerdraw
    RETURN
END IF
IF soundmode = 4 THEN
    LET soundmode = 1
    LET eventtitle$ = "SOUND MODE SET:"
    LET eventdata$ = "sfx off"
    LET eventnumber = soundmode
    GOSUB consoleprinter
    LET textspeech$ = "Sound Effects Off!"
    GOSUB textbannerdraw
    RETURN
END IF
RETURN

musictoggle:
REM toggles music
IF liteload = 1 THEN RETURN
REM turn music on
IF soundmode = 1 THEN
    LET soundmode = 3
    GOSUB musicplay
    LET eventtitle$ = "SOUND MODE SET:"
    LET eventdata$ = "music on"
    LET eventnumber = soundmode
    GOSUB consoleprinter
    LET textspeech$ = "Music On!"
    GOSUB textbannerdraw
    RETURN
END IF
IF soundmode = 4 THEN
    LET soundmode = 2
    GOSUB musicplay
    LET eventtitle$ = "SOUND MODE SET:"
    LET eventdata$ = "music on"
    LET eventnumber = soundmode
    GOSUB consoleprinter
    LET textspeech$ = "Music On!"
    GOSUB textbannerdraw
    RETURN
END IF
REM turn music off
IF soundmode = 2 THEN
    GOSUB musicstop
    LET soundmode = 4
    LET eventtitle$ = "SOUND MODE SET:"
    LET eventdata$ = "music off"
    LET eventnumber = soundmode
    GOSUB consoleprinter
    LET textspeech$ = "Music Off!"
    GOSUB textbannerdraw
    RETURN
END IF
IF soundmode = 3 THEN
    GOSUB musicstop
    LET soundmode = 1
    LET eventtitle$ = "SOUND MODE SET:"
    LET eventdata$ = "music off"
    LET eventnumber = soundmode
    GOSUB consoleprinter
    LET textspeech$ = "Music Off!"
    GOSUB textbannerdraw
    RETURN
END IF
RETURN

screentoggle:
REM toggles screen modes
REM switch to window
IF screenmode = 1 THEN
    _FULLSCREEN _OFF
    LET screenmode = 2
    LET eventtitle$ = "SCREEN MODE SET:"
    LET eventdata$ = "windowed"
    LET eventnumber = screenmode
    GOSUB consoleprinter
    LET textspeech$ = "Windowed Mode Set!"
    GOSUB textbannerdraw
    RETURN
END IF
REM switch to fullscreen
IF screenmode = 2 THEN
    _FULLSCREEN _SQUAREPIXELS
    LET screenmode = 1
    LET eventtitle$ = "SCREEN MODE SET:"
    LET eventdata$ = "fullscreen"
    LET eventnumber = screenmode
    GOSUB consoleprinter
    LET textspeech$ = "Fullscreen Mode Set!"
    GOSUB textbannerdraw
    RETURN
END IF
RETURN

effectdraw:
REM draws special map effects
IF disablefade = 1 THEN RETURN: REM return for if effects are disabled
IF effectani = 1 THEN RETURN: REM return for if animation is playing
IF mapeffect = 1 THEN
    REM dark
    LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, (255 / 2)), BF
END IF
IF mapeffect = 2 THEN
    REM rain
    REM setup values
    LET rainspread = resx + (resy / 2)
    IF rainx1 = 0 THEN LET rainx1 = INT(RND * rainspread): LET rainy1 = INT(RND * resy)
    IF rainx2 = 0 THEN LET rainx2 = INT(RND * rainspread): LET rainy2 = INT(RND * resy)
    IF rainx3 = 0 THEN LET rainx3 = INT(RND * rainspread): LET rainy3 = INT(RND * resy)
    IF rainx4 = 0 THEN LET rainx4 = INT(RND * rainspread): LET rainy4 = INT(RND * resy)
    IF rainx5 = 0 THEN LET rainx5 = INT(RND * rainspread): LET rainy5 = INT(RND * resy)
    IF rainx6 = 0 THEN LET rainx6 = INT(RND * rainspread): LET rainy6 = INT(RND * resy)
    IF rainx7 = 0 THEN LET rainx7 = INT(RND * rainspread): LET rainy7 = INT(RND * resy)
    IF rainx8 = 0 THEN LET rainx8 = INT(RND * rainspread): LET rainy8 = INT(RND * resy)
    IF rainx9 = 0 THEN LET rainx9 = INT(RND * rainspread): LET rainy9 = INT(RND * resy)
    IF rainx10 = 0 THEN LET rainx10 = INT(RND * rainspread): LET rainy10 = INT(RND * resy)
    REM draws slight dark
    LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, 20), BF
    REM draws raindrops
    LINE (rainx1, rainy1)-(rainx1 - 2, rainy1 + 3), _RGBA(0, 89, 255, (255 / 2))
    LINE (rainx2, rainy2)-(rainx2 - 2, rainy2 + 3), _RGBA(0, 89, 255, (255 / 2))
    LINE (rainx3, rainy3)-(rainx3 - 2, rainy3 + 3), _RGBA(0, 89, 255, (255 / 2))
    LINE (rainx4, rainy4)-(rainx4 - 2, rainy4 + 3), _RGBA(0, 89, 255, (255 / 2))
    LINE (rainx5, rainy5)-(rainx5 - 2, rainy5 + 3), _RGBA(0, 89, 255, (255 / 2))
    LINE (rainx6, rainy6)-(rainx6 - 2, rainy6 + 3), _RGBA(0, 89, 255, (255 / 2))
    LINE (rainx7, rainy7)-(rainx7 - 2, rainy7 + 3), _RGBA(0, 89, 255, (255 / 2))
    LINE (rainx8, rainy8)-(rainx8 - 2, rainy8 + 3), _RGBA(0, 89, 255, (255 / 2))
    LINE (rainx9, rainy9)-(rainx9 - 2, rainy9 + 3), _RGBA(0, 89, 255, (255 / 2))
    LINE (rainx10, rainy10)-(rainx10 - 2, rainy10 + 3), _RGBA(0, 89, 255, (255 / 2))
    REM calculates movement
    IF rainy1 < resy / 4 THEN
        LET rainy1 = rainy1 + 4
        LET rainx1 = rainx1 - 2
    ELSE
        LET rainy1 = rainy1 + 8
        LET rainx1 = rainx1 - 4
    END IF
    IF rainy2 < resy / 4 THEN
        LET rainy2 = rainy2 + 4
        LET rainx2 = rainx2 - 2
    ELSE
        LET rainy2 = rainy2 + 8
        LET rainx2 = rainx2 - 4
    END IF
    IF rainy3 < resy / 4 THEN
        LET rainy3 = rainy3 + 4
        LET rainx3 = rainx3 - 2
    ELSE
        LET rainy3 = rainy3 + 8
        LET rainx3 = rainx3 - 4
    END IF
    IF rainy4 < resy / 4 THEN
        LET rainy4 = rainy4 + 4
        LET rainx4 = rainx4 - 2
    ELSE
        LET rainy4 = rainy4 + 8
        LET rainx4 = rainx4 - 4
    END IF
    IF rainy5 < resy / 4 THEN
        LET rainy5 = rainy5 + 4
        LET rainx5 = rainx5 - 2
    ELSE
        LET rainy5 = rainy5 + 8
        LET rainx5 = rainx5 - 4
    END IF
    IF rainy6 < resy / 4 THEN
        LET rainy6 = rainy6 + 4
        LET rainx6 = rainx6 - 2
    ELSE
        LET rainy6 = rainy6 + 8
        LET rainx6 = rainx6 - 4
    END IF
    IF rainy7 < resy / 4 THEN
        LET rainy7 = rainy7 + 4
        LET rainx7 = rainx7 - 2
    ELSE
        LET rainy7 = rainy7 + 8
        LET rainx7 = rainx7 - 4
    END IF
    IF rainy8 < resy / 4 THEN
        LET rainy8 = rainy8 + 4
        LET rainx8 = rainx8 - 2
    ELSE
        LET rainy8 = rainy8 + 8
        LET rainx8 = rainx8 - 4
    END IF
    IF rainy9 < resy / 4 THEN
        LET rainy9 = rainy9 + 4
        LET rainx9 = rainx9 - 2
    ELSE
        LET rainy9 = rainy9 + 8
        LET rainx9 = rainx9 - 4
    END IF
    IF rainy10 < resy / 4 THEN
        LET rainy10 = rainy10 + 4
        LET rainx10 = rainx10 - 2
    ELSE
        LET rainy10 = rainy10 + 8
        LET rainx10 = rainx10 - 4
    END IF
    REM resets rain drops
    IF rainy1 > resy THEN LET rainy1 = 0: LET rainx1 = INT(RND * rainspread)
    IF rainy2 > resy THEN LET rainy2 = 0: LET rainx2 = INT(RND * rainspread)
    IF rainy3 > resy THEN LET rainy3 = 0: LET rainx3 = INT(RND * rainspread)
    IF rainy4 > resy THEN LET rainy4 = 0: LET rainx4 = INT(RND * rainspread)
    IF rainy5 > resy THEN LET rainy5 = 0: LET rainx5 = INT(RND * rainspread)
    IF rainy6 > resy THEN LET rainy6 = 0: LET rainx6 = INT(RND * rainspread)
    IF rainy7 > resy THEN LET rainy7 = 0: LET rainx7 = INT(RND * rainspread)
    IF rainy8 > resy THEN LET rainy8 = 0: LET rainx8 = INT(RND * rainspread)
    IF rainy9 > resy THEN LET rainy9 = 0: LET rainx9 = INT(RND * rainspread)
    IF rainy10 > resy THEN LET rainy10 = 0: LET rainx10 = INT(RND * rainspread)
END IF
IF mapeffect = 3 THEN
    REM storm
    REM setup values
    LET rainspread = resx + (resy / 2)
    IF temp97 = 0 THEN LET temp97 = INT(RND * 60) + ctime
    IF rainx1 = 0 THEN LET rainx1 = INT(RND * rainspread): LET rainy1 = INT(RND * resy)
    IF rainx2 = 0 THEN LET rainx2 = INT(RND * rainspread): LET rainy2 = INT(RND * resy)
    IF rainx3 = 0 THEN LET rainx3 = INT(RND * rainspread): LET rainy3 = INT(RND * resy)
    IF rainx4 = 0 THEN LET rainx4 = INT(RND * rainspread): LET rainy4 = INT(RND * resy)
    IF rainx5 = 0 THEN LET rainx5 = INT(RND * rainspread): LET rainy5 = INT(RND * resy)
    IF rainx6 = 0 THEN LET rainx6 = INT(RND * rainspread): LET rainy6 = INT(RND * resy)
    IF rainx7 = 0 THEN LET rainx7 = INT(RND * rainspread): LET rainy7 = INT(RND * resy)
    IF rainx8 = 0 THEN LET rainx8 = INT(RND * rainspread): LET rainy8 = INT(RND * resy)
    IF rainx9 = 0 THEN LET rainx9 = INT(RND * rainspread): LET rainy9 = INT(RND * resy)
    IF rainx10 = 0 THEN LET rainx10 = INT(RND * rainspread): LET rainy10 = INT(RND * resy)
    IF rainx11 = 0 THEN LET rainx11 = INT(RND * rainspread): LET rainy11 = INT(RND * resy)
    IF rainx12 = 0 THEN LET rainx12 = INT(RND * rainspread): LET rainy12 = INT(RND * resy)
    IF rainx13 = 0 THEN LET rainx13 = INT(RND * rainspread): LET rainy13 = INT(RND * resy)
    IF rainx14 = 0 THEN LET rainx14 = INT(RND * rainspread): LET rainy14 = INT(RND * resy)
    IF rainx15 = 0 THEN LET rainx15 = INT(RND * rainspread): LET rainy15 = INT(RND * resy)
    IF rainx16 = 0 THEN LET rainx16 = INT(RND * rainspread): LET rainy16 = INT(RND * resy)
    IF rainx17 = 0 THEN LET rainx17 = INT(RND * rainspread): LET rainy17 = INT(RND * resy)
    IF rainx18 = 0 THEN LET rainx18 = INT(RND * rainspread): LET rainy18 = INT(RND * resy)
    IF rainx19 = 0 THEN LET rainx19 = INT(RND * rainspread): LET rainy19 = INT(RND * resy)
    IF rainx20 = 0 THEN LET rainx20 = INT(RND * rainspread): LET rainy20 = INT(RND * resy)
    REM draws dark
    IF temp97 > ctime THEN
        LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, (255 / 2)), BF
    ELSE
        LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, 20), BF
        LET temp98 = temp98 + 1
        IF temp98 > hertz THEN LET playsfx$ = "lightning": GOSUB sfxplay: LET temp98 = 0: LET temp97 = 0
    END IF
    REM draws raindrops
    LINE (rainx1, rainy1)-(rainx1 - 2, rainy1 + 3), _RGBA(0, 89, 255, 255)
    LINE (rainx2, rainy2)-(rainx2 - 2, rainy2 + 3), _RGBA(0, 89, 255, 255)
    LINE (rainx3, rainy3)-(rainx3 - 2, rainy3 + 3), _RGBA(0, 89, 255, 255)
    LINE (rainx4, rainy4)-(rainx4 - 2, rainy4 + 3), _RGBA(0, 89, 255, 255)
    LINE (rainx5, rainy5)-(rainx5 - 2, rainy5 + 3), _RGBA(0, 89, 255, 255)
    LINE (rainx6, rainy6)-(rainx6 - 2, rainy6 + 3), _RGBA(0, 89, 255, 255)
    LINE (rainx7, rainy7)-(rainx7 - 2, rainy7 + 3), _RGBA(0, 89, 255, 255)
    LINE (rainx8, rainy8)-(rainx8 - 2, rainy8 + 3), _RGBA(0, 89, 255, 255)
    LINE (rainx9, rainy9)-(rainx9 - 2, rainy9 + 3), _RGBA(0, 89, 255, 255)
    LINE (rainx10, rainy10)-(rainx10 - 2, rainy10 + 3), _RGBA(0, 89, 255, 255)
    LINE (rainx11, rainy11)-(rainx11 - 2, rainy11 + 3), _RGBA(0, 89, 255, 255)
    LINE (rainx12, rainy12)-(rainx12 - 2, rainy12 + 3), _RGBA(0, 89, 255, 255)
    LINE (rainx13, rainy13)-(rainx13 - 2, rainy13 + 3), _RGBA(0, 89, 255, 255)
    LINE (rainx14, rainy14)-(rainx14 - 2, rainy14 + 3), _RGBA(0, 89, 255, 255)
    LINE (rainx15, rainy15)-(rainx15 - 2, rainy15 + 3), _RGBA(0, 89, 255, 255)
    LINE (rainx16, rainy16)-(rainx16 - 2, rainy16 + 3), _RGBA(0, 89, 255, 255)
    LINE (rainx17, rainy17)-(rainx17 - 2, rainy17 + 3), _RGBA(0, 89, 255, 255)
    LINE (rainx18, rainy18)-(rainx18 - 2, rainy18 + 3), _RGBA(0, 89, 255, 255)
    LINE (rainx19, rainy19)-(rainx19 - 2, rainy19 + 3), _RGBA(0, 89, 255, 255)
    LINE (rainx20, rainy20)-(rainx20 - 2, rainy20 + 3), _RGBA(0, 89, 255, 255)
    REM calculates movement
    IF rainy1 < resy / 4 THEN
        LET rainy1 = rainy1 + 4
        LET rainx1 = rainx1 - 2
    ELSE
        LET rainy1 = rainy1 + 8
        LET rainx1 = rainx1 - 4
    END IF
    IF rainy2 < resy / 4 THEN
        LET rainy2 = rainy2 + 4
        LET rainx2 = rainx2 - 2
    ELSE
        LET rainy2 = rainy2 + 8
        LET rainx2 = rainx2 - 4
    END IF
    IF rainy3 < resy / 4 THEN
        LET rainy3 = rainy3 + 4
        LET rainx3 = rainx3 - 2
    ELSE
        LET rainy3 = rainy3 + 8
        LET rainx3 = rainx3 - 4
    END IF
    IF rainy4 < resy / 4 THEN
        LET rainy4 = rainy4 + 4
        LET rainx4 = rainx4 - 2
    ELSE
        LET rainy4 = rainy4 + 8
        LET rainx4 = rainx4 - 4
    END IF
    IF rainy5 < resy / 4 THEN
        LET rainy5 = rainy5 + 4
        LET rainx5 = rainx5 - 2
    ELSE
        LET rainy5 = rainy5 + 8
        LET rainx5 = rainx5 - 4
    END IF
    IF rainy6 < resy / 4 THEN
        LET rainy6 = rainy6 + 4
        LET rainx6 = rainx6 - 2
    ELSE
        LET rainy6 = rainy6 + 8
        LET rainx6 = rainx6 - 4
    END IF
    IF rainy7 < resy / 4 THEN
        LET rainy7 = rainy7 + 4
        LET rainx7 = rainx7 - 2
    ELSE
        LET rainy7 = rainy7 + 8
        LET rainx7 = rainx7 - 4
    END IF
    IF rainy8 < resy / 4 THEN
        LET rainy8 = rainy8 + 4
        LET rainx8 = rainx8 - 2
    ELSE
        LET rainy8 = rainy8 + 8
        LET rainx8 = rainx8 - 4
    END IF
    IF rainy9 < resy / 4 THEN
        LET rainy9 = rainy9 + 4
        LET rainx9 = rainx9 - 2
    ELSE
        LET rainy9 = rainy9 + 8
        LET rainx9 = rainx9 - 4
    END IF
    IF rainy10 < resy / 4 THEN
        LET rainy10 = rainy10 + 4
        LET rainx10 = rainx10 - 2
    ELSE
        LET rainy10 = rainy10 + 8
        LET rainx10 = rainx10 - 4
    END IF
    IF rainy11 < resy / 4 THEN
        LET rainy11 = rainy11 + 4
        LET rainx11 = rainx11 - 2
    ELSE
        LET rainy11 = rainy11 + 8
        LET rainx11 = rainx11 - 4
    END IF
    IF rainy12 < resy / 4 THEN
        LET rainy12 = rainy12 + 4
        LET rainx12 = rainx12 - 2
    ELSE
        LET rainy12 = rainy12 + 8
        LET rainx12 = rainx12 - 4
    END IF
    IF rainy13 < resy / 4 THEN
        LET rainy13 = rainy13 + 4
        LET rainx13 = rainx13 - 2
    ELSE
        LET rainy13 = rainy13 + 8
        LET rainx13 = rainx13 - 4
    END IF
    IF rainy14 < resy / 4 THEN
        LET rainy14 = rainy14 + 4
        LET rainx14 = rainx14 - 2
    ELSE
        LET rainy14 = rainy14 + 8
        LET rainx14 = rainx14 - 4
    END IF
    IF rainy15 < resy / 4 THEN
        LET rainy15 = rainy15 + 4
        LET rainx15 = rainx15 - 2
    ELSE
        LET rainy15 = rainy15 + 8
        LET rainx15 = rainx15 - 4
    END IF
    IF rainy16 < resy / 4 THEN
        LET rainy16 = rainy16 + 4
        LET rainx16 = rainx16 - 2
    ELSE
        LET rainy16 = rainy16 + 8
        LET rainx16 = rainx16 - 4
    END IF
    IF rainy17 < resy / 4 THEN
        LET rainy17 = rainy17 + 4
        LET rainx17 = rainx17 - 2
    ELSE
        LET rainy17 = rainy17 + 8
        LET rainx17 = rainx17 - 4
    END IF
    IF rainy18 < resy / 4 THEN
        LET rainy18 = rainy18 + 4
        LET rainx18 = rainx18 - 2
    ELSE
        LET rainy18 = rainy18 + 8
        LET rainx18 = rainx18 - 4
    END IF
    IF rainy19 < resy / 4 THEN
        LET rainy19 = rainy19 + 4
        LET rainx19 = rainx19 - 2
    ELSE
        LET rainy19 = rainy19 + 8
        LET rainx19 = rainx19 - 4
    END IF
    IF rainy20 < resy / 4 THEN
        LET rainy20 = rainy20 + 4
        LET rainx20 = rainx20 - 2
    ELSE
        LET rainy20 = rainy20 + 8
        LET rainx20 = rainx20 - 4
    END IF
    REM resets rain drops
    IF rainy1 > resy THEN LET rainy1 = 0: LET rainx1 = INT(RND * rainspread)
    IF rainy2 > resy THEN LET rainy2 = 0: LET rainx2 = INT(RND * rainspread)
    IF rainy3 > resy THEN LET rainy3 = 0: LET rainx3 = INT(RND * rainspread)
    IF rainy4 > resy THEN LET rainy4 = 0: LET rainx4 = INT(RND * rainspread)
    IF rainy5 > resy THEN LET rainy5 = 0: LET rainx5 = INT(RND * rainspread)
    IF rainy6 > resy THEN LET rainy6 = 0: LET rainx6 = INT(RND * rainspread)
    IF rainy7 > resy THEN LET rainy7 = 0: LET rainx7 = INT(RND * rainspread)
    IF rainy8 > resy THEN LET rainy8 = 0: LET rainx8 = INT(RND * rainspread)
    IF rainy9 > resy THEN LET rainy9 = 0: LET rainx9 = INT(RND * rainspread)
    IF rainy10 > resy THEN LET rainy10 = 0: LET rainx10 = INT(RND * rainspread)
    IF rainy11 > resy THEN LET rainy11 = 0: LET rainx11 = INT(RND * rainspread)
    IF rainy12 > resy THEN LET rainy12 = 0: LET rainx12 = INT(RND * rainspread)
    IF rainy13 > resy THEN LET rainy13 = 0: LET rainx13 = INT(RND * rainspread)
    IF rainy14 > resy THEN LET rainy14 = 0: LET rainx14 = INT(RND * rainspread)
    IF rainy15 > resy THEN LET rainy15 = 0: LET rainx15 = INT(RND * rainspread)
    IF rainy16 > resy THEN LET rainy16 = 0: LET rainx16 = INT(RND * rainspread)
    IF rainy17 > resy THEN LET rainy17 = 0: LET rainx17 = INT(RND * rainspread)
    IF rainy18 > resy THEN LET rainy18 = 0: LET rainx18 = INT(RND * rainspread)
    IF rainy19 > resy THEN LET rainy19 = 0: LET rainx19 = INT(RND * rainspread)
    IF rainy20 > resy THEN LET rainy20 = 0: LET rainx20 = INT(RND * rainspread)
END IF
IF mapeffect = 4 THEN
    REM torch effect
    _PUTIMAGE (0, 0)-(resx, resy), torcheffect
END IF
IF mapeffect = 5 THEN
    REM sunset/dark fade (right)
    LET temp123 = (((resx / 2) - posx) / mapx) * (255 / 2)
    LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, temp123), BF
END IF
IF mapeffect = 6 THEN
    REM sunset/dark fade (left)
    LET temp123 = (((resx / 2) - posx) / mapx) * (255 / 2)
    LET temp124 = (255 / 2) - temp123
    LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, temp124), BF
END IF
IF mapeffect = 7 THEN
    REM sunset/dark fade (down)
    LET temp123 = (((resy / 2) - posy) / mapy) * (255 / 2)
    LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, temp123), BF
END IF
IF mapeffect = 8 THEN
    REM sunset/dark fade (up)
    LET temp123 = (((resy / 2) - posy) / mapy) * (255 / 2)
    LET temp124 = (255 / 2) - temp123
    LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, temp124), BF
END IF
IF mapeffect = 9 THEN
    REM pitch black
    LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, 255), BF
END IF
RETURN

erasesave:
REM erases save data
REM checks default save file exists
IF _FILEEXISTS(sloc$ + "defaultsave.ddf") THEN
    REM nothing
ELSE
    ERROR 422
END IF
REM Linux + MacOS
IF ros$ = "lnx" OR ros$ = "mac" THEN
	IF _FILEEXISTS(sloc$ + "savedata" + DATE$ + ".old") THEN
		LET x = 0
		LET y = 0
		DO
			LET x = x + 1
			IF _FILEEXISTS(sloc$ + "savedata" + DATE$ + "-" + LTRIM$(STR$(x)) + ".old") THEN
				LET y = 0
			ELSE
				LET y = 1
			END IF
		LOOP UNTIL y = 1
		SHELL _HIDE "cp " + sloc$ + "savedata.ddf " + sloc$ + "savedata" + DATE$ + "-" + LTRIM$(STR$(x)) + ".old"
		LET x = 0
		LET y = 0
	ELSE
		SHELL _HIDE "cp " + sloc$ + "savedata.ddf " + sloc$ + "savedata" + DATE$ + ".old"
	END IF
	SHELL _HIDE "rm " + sloc$ + "savedata.ddf"
	SHELL _HIDE "cp " + sloc$ + "defaultsave.ddf " + sloc$ + "savedata.ddf"
END IF
REM Windoze
IF ros$ = "win" THEN
	IF _FILEEXISTS(sloc$ + "savedata" + DATE$ + ".old") THEN
		LET x = 0
		LET y = 0
		DO
			LET x = x + 1
			IF _FILEEXISTS(sloc$ + "savedata" + DATE$ + "-" + LTRIM$(STR$(x)) + ".old") THEN
				LET y = 0
			ELSE
				LET y = 1
			END IF
		LOOP UNTIL y = 1
		SHELL _HIDE "copy " + sloc$ + "savedata.ddf " + sloc$ + "savedata" + DATE$ + "-" + LTRIM$(STR$(x)) + ".old"
		LET x = 0
		LET y = 0
	ELSE
		SHELL _HIDE "copy " + sloc$ + "savedata.ddf " + sloc$ + "savedata" + DATE$ + ".old"
	END IF
    SHELL _HIDE "del " + sloc$ + "savedata.ddf"
    SHELL _HIDE "copy " + sloc$ + "defaultsave.ddf " + sloc$ + "savedata.ddf"
END IF
REM tells console
LET eventtitle$ = "SAVEDATA ERASED:"
LET eventdata$ = sloc$ + "savedata.ddf"
LET eventnumber = 0
GOSUB consoleprinter
'LET temp34 = 3
'LET textspeech$ = "Savedata erased! " + title$ + " will now close!"
'GOSUB textbannerdraw
LET temp82 = 1 REM marks shutdown procedure not to save game
GOTO endgame
RETURN

mapload:
REM loads map data
REM unload divert if map has changed and if system is not booting
IF setupboot = 0 THEN IF mapno <> oldmapno THEN GOSUB playerunload: GOSUB objectunload: GOSUB mapunload
IF setupboot = 0 AND oldmapno = mapno THEN RETURN: REM divert for if mapno hasn't actually changed
REM sets path location data of map metadata
LET mapfile$ = "map" + LTRIM$(STR$(mapno))
LET mapdir$ = "m" + LTRIM$(STR$(mapno)) + "/"
REM loads map bitmaps
LET mapa = _LOADIMAGE(mloc$ + mapdir$ + mapfile$ + "a.png")
LET mapb = _LOADIMAGE(mloc$ + mapdir$ + mapfile$ + "b.png")
REM loads metadata
OPEN mloc$ + "/" + mapdir$ + "/" + mapfile$ + ".ddf" FOR INPUT AS #1
INPUT #1, mapname$, playmusic$, mapeffect, mapx, mapy, mapobjectno, mapplayerno, maptriggerno
REM loads objects
LET x = 0
DO
    LET x = x + 1
    INPUT #1, objectname(x), objectx(x), objecty(x)
LOOP UNTIL x >= totalobjects
REM loads NPCs
LET x = 0
DO
    LET x = x + 1
    INPUT #1, playername(x), playerx(x), playery(x), mplayerx(x), mplayery(x), playergrace(x), playerdefault(x)
LOOP UNTIL x >= totalplayers
REM loads triggers
LET x = 0
DO
    LET x = x + 1
    INPUT #1, triggername(x), triggerx1(x), triggery1(x), triggerx2(x), triggery2(x)
LOOP UNTIL x >= totaltriggers
LET x = 0
CLOSE #1
REM console printer
LET eventtitle$ = "MAP LOADED: "
LET eventdata$ = mapname$
LET eventnumber = mapno
GOSUB consoleprinter
REM diverts to object loader if required
IF mapobjectno > 0 THEN
    GOSUB objectload
ELSE
    LET eventtitle$ = "NO OBJECTS ATTACHED TO MAP"
    LET eventdata$ = ""
    LET eventnumber = 0
    GOSUB consoleprinter
END IF
REM diverts to NPC loader if required
IF mapplayerno > 0 THEN
    GOSUB playerload
    IF carryvalues = 1 THEN GOSUB carryplayervalues
ELSE
    LET eventtitle$ = "NO PLAYERS ATTACHED TO MAP"
    LET eventdata$ = ""
    LET eventnumber = 0
    GOSUB consoleprinter
END IF
REM generates animation offsets
GOSUB generateoffsets
REM plays music
IF playmusic$ <> "" THEN GOSUB musicplay
REM fades in
IF setupboot = 0 AND scriptrun = 0 THEN GOSUB fadein
RETURN

carryplayervalues:
REM carries storied player values into new map
LET x = 0
DO
    LET x = x + 1
    LET playerx(x) = carryplayerx(x): LET playery(x) = carryplayery(x)
    LET playerd(x) = carryplayerx(x): LET playerjourney(x) = carryplayerjourney(x)
    LET playerlayer(x) = carryplayerlayer(x): LET playerperiod(x) = carryplayerperiod(x)
    LET carryplayerx(x) = 0: LET carryplayery(x) = 0
    LET carryplayerd(x) = 0: LET carryplayerjourney(x) = 0
    LET carryplayerlayer(x) = 0: LET carryplayerperiod(x) = 0
LOOP UNTIL x >= totalplayers
LET carryvalues = 0: REM turns off value carrying
LET eventtitle$ = "PLAYER LOCATION VALUES CARRIED!"
LET eventdata$ = ""
LET eventnumber = 0
GOSUB consoleprinter
LET x = 0
RETURN

objectunload:
REM unloads all map objects
IF mapobjectno > 0 THEN
    REM unloads objects if map has any
    DO
        LET temp14 = temp14 + 1
        IF objectname(temp14) <> "[COLLISIONONLY]" THEN _FREEIMAGE objecta(temp14): _FREEIMAGE objectb(temp14): LET temp11$ = objectname(temp14)
        REM prints to console
        IF temp11$ = "" THEN LET temp11$ = "[COLLISIONONLY]"
        LET eventtitle$ = "OBJECT UNLOADED:"
        LET eventdata$ = temp11$
        LET eventnumber = temp14
        GOSUB consoleprinter
    LOOP UNTIL temp14 >= mapobjectno
ELSE
    REM prints to console that map has no objects to unload
    LET eventtitle$ = "NO OBJECTS ATTACHED TO MAP"
    LET eventdata$ = ""
    LET eventnumber = 0
    GOSUB consoleprinter
END IF
LET temp14 = 0: LET temp11$ = "": REM scrub temp values
RETURN

playerload:
REM loads any NPCs into memory
DO
    REM loads sprites and metadata
    LET temp39 = temp39 + 1
    LET temp13$ = playername(temp39): LET temp40 = playerx(temp39): LET temp41 = playery(temp39)
    OPEN ploc$ + temp13$ + "/" + temp13$ + ".ddf" FOR INPUT AS #1
    INPUT #1, temp14$, playerresx(temp39), playerresy(temp39), players(temp39), playernote1(temp39), playernote2(temp39)
    LET playerf(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-f.png")
    LET playerb(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-b.png")
    LET playerl(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-l.png")
    LET playerr(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-r.png")
    LET playerfl(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-fl.png")
    LET playerfr(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-fr.png")
    LET playerbl(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-bl.png")
    LET playerbr(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-br.png")
    LET playerrl(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-rl.png")
    LET playerrr(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-rr.png")
    LET playerll(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-ll.png")
    LET playerlr(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-lr.png")
    LET playerfi1(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-fi1.png")
    LET playerfi2(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-fi2.png")
    LET playerbi1(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-bi1.png")
    LET playerbi2(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-bi2.png")
    LET playerli1(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-li1.png")
    LET playerli2(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-li2.png")
    LET playerri1(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-ri1.png")
    LET playerri2(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-ri2.png")
    LET playerface1(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-face1.png")
    LET playerface2(temp39) = _LOADIMAGE(ploc$ + "/" + temp13$ + "/" + temp13$ + "-face2.png")
    CLOSE #1: REM closes metadata
    REM wipes walking and direction values to default
    LET playerwalking(temp39) = 0
    LET playerd(temp39) = playerdefault(temp39)
    LET playerperiod(temp39) = playergrace(temp39) + INT(ctime)
    LET dplayerx(temp39) = playerx(temp39)
    LET dplayery(temp39) = playery(temp39)
    LET playerjourney(temp39) = 1
    LET pfoot(temp39) = 1
    LET playerscript(temp39) = 0
    REM console printer
    LET eventtitle$ = "PLAYER LOADED:"
    LET eventdata$ = temp13$
    LET eventnumber = temp39
    GOSUB consoleprinter
LOOP UNTIL temp39 >= mapplayerno
LET temp39 = 0: LET temp13$ = "": LET temp40 = 0: LET temp14$ = "": LET temp41 = 0: REM scrub temp values
RETURN

playerunload:
REM unloads all NPCs
IF scriptrun = 0 THEN GOSUB fadeout: REM fades out
IF mapplayerno > 0 THEN
    REM unloads players if map has any
    DO
        LET temp41 = temp41 + 1
        _FREEIMAGE playerf(temp41)
        _FREEIMAGE playerb(temp41)
        _FREEIMAGE playerr(temp41)
        _FREEIMAGE playerl(temp41)
        _FREEIMAGE playerfr(temp41)
        _FREEIMAGE playerfl(temp41)
        _FREEIMAGE playerbr(temp41)
        _FREEIMAGE playerbl(temp41)
        _FREEIMAGE playerlr(temp41)
        _FREEIMAGE playerll(temp41)
        _FREEIMAGE playerrr(temp41)
        _FREEIMAGE playerrl(temp41)
        _FREEIMAGE playerfi1(temp41)
        _FREEIMAGE playerfi2(temp41)
        _FREEIMAGE playerbi1(temp41)
        _FREEIMAGE playerbi2(temp41)
        _FREEIMAGE playerli1(temp41)
        _FREEIMAGE playerli2(temp41)
        _FREEIMAGE playerri1(temp41)
        _FREEIMAGE playerri2(temp41)
        _FREEIMAGE playerface1(temp41)
        _FREEIMAGE playerface2(temp41)
        LET temp14$ = playername$(temp41)
        REM prints to console
        LET eventtitle$ = "PLAYER UNLOADED:"
        LET eventdata$ = temp14$
        LET eventnumber = temp41
        GOSUB consoleprinter
    LOOP UNTIL temp41 >= mapplayerno
ELSE
    REM prints to console that map has no NPCs to unload
    LET eventtitle$ = "NO PLAYERS ATTACHED TO MAP"
    LET eventdata$ = ""
    LET eventnumber = 0
    GOSUB consoleprinter
END IF
LET temp41 = 0: LET temp14$ = "": REM scrub temp values
RETURN

collisionconverter:
REM converts map data into collision data
LET temp141 = objectx(temp10): LET temp142 = objecty(temp10)
REM converts values
LET temp36$ = STR$(temp141)
LET temp37$ = STR$(temp142)
REM first X + Y
LET temp38$ = LEFT$(temp36$, INSTR(temp36$, ".") - 1)
LET temp39$ = MID$(temp36$, INSTR(temp36$, "."))
LET temp143 = VAL(temp38$)
LET temp144 = VAL(temp39$)
IF LEN(temp39$) > 3 THEN
    LET temp144 = temp144 * 1000
ELSE
    LET temp144 = temp144 * 100
END IF
REM second X + Y
LET temp40$ = LEFT$(temp37$, INSTR(temp37$, ".") - 1)
LET temp41$ = MID$(temp37$, INSTR(temp37$, "."))
LET temp145 = VAL(temp40$)
LET temp146 = VAL(temp41$)
IF LEN(temp41$) > 3 THEN
    LET temp146 = temp146 * 1000
ELSE
    LET temp146 = temp146 * 100
END IF
REM tells console
LET eventtitle$ = "COLLISION MAPPED:"
LET eventdata$ = "x1:" + STR$(temp143) + " y1:" + STR$(temp144) + " x2:" + STR$(temp145 + temp143) + " y2:" + STR$(temp146 + temp144)
LET eventnumber = temp10
GOSUB consoleprinter
REM applies values
LET objectx(temp10) = temp143: LET objecty(temp10) = temp144: LET objectresx(temp10) = temp145: LET objectresy(temp10) = temp146: LET objects(temp10) = collisionstep
LET temp141 = 0: LET temp142 = 0: LET temp143 = 0: LET temp144 = 0: LET temp145 = 0: LET temp146 = 0: LET temp36$ = "": LET temp37$ = "": LET temp38$ = "": LET temp39$ = "": LET temp40$ = "": LET temp41$ = "": REM scrub temp values
RETURN

objectload:
REM loads any map objects into memory
DO
    LET temp10 = temp10 + 1
    LET temp10$ = objectname(temp10): LET temp11 = objectx(temp10): LET temp12 = objecty(temp10)
    IF objectname(temp10) <> "[COLLISIONONLY]" THEN
        OPEN oloc$ + temp10$ + "/" + temp10$ + ".ddf" FOR INPUT AS #1
        INPUT #1, temp11$, objectresx(temp10), objectresy(temp10), objects(temp10): LET objecta(temp10) = _LOADIMAGE(oloc$ + temp10$ + "/" + temp10$ + "a.png"): LET objectb(temp10) = _LOADIMAGE(oloc$ + temp10$ + "/" + temp10$ + "b.png")
        CLOSE #1
    ELSE
        GOSUB collisionconverter
    END IF
    REM console printer
    LET eventtitle$ = "OBJECT LOADED:"
    LET eventdata$ = temp10$
    LET eventnumber = temp10
    GOSUB consoleprinter
LOOP UNTIL temp10 >= mapobjectno
LET temp10 = 0: LET temp10$ = "": LET temp11 = 0: LET temp11$ = "": LET temp12 = 0: REM scrub temp values
RETURN

timeframecounter:
REM time + frame counter
IF scriptrun = 0 THEN IF _EXIT THEN GOTO endgame: REM ends game on window close
IF TIMER < 0 OR ctime < 0 THEN
    REM resets timer when value wraparound occurs
    RANDOMIZE TIMER
    LET itime = TIMER
    IF ctime > 0 THEN
        LET eventtitle$ = "TIMER RESET:"
    ELSE
        LET eventtitle$ = "COUNTER RESET:"
    END IF
    LET eventdata$ = TIME$
    LET eventnumber = frames
    GOSUB consoleprinter
END IF
LET ctime = (TIMER - itime): REM time keeper
LET frames = frames + 1: REM frame counter
REM calculate fps
LET temp7 = temp7 + 1
IF temp8 + 1 < ctime THEN
    LET fps = temp7
    LET temp7 = 0: REM scrub temp values
    LET temp8 = ctime: REM reset temp values
END IF
REM calculate speedrun
IF speedrun = 1 THEN
	LET speedrunhour = INT(ctime) \ 3600
	LET speedrunmin = (INT(ctime) - (3600 * speedrunhour)) \ 60
	LET speedrunsec = (INT(ctime) - (3600 * speedrunhour)) - (speedrunmin * 60)
END IF
RETURN

game:
REM Main engine loop (BETWEEN DO AND LOOP)
REM lets console know main loop active
LET eventtitle$ = "ENGINE LOOP: "
LET eventdata$ = "active!"
LET eventnumber = 0
GOSUB consoleprinter
REM fadein
GOSUB fadein
LET temp8 = ctime: REM for fps counter
REM engine loop
DO
    REM value modifyers
    LET xxit = _EXIT: REM sets game exit value
    LET a$ = UCASE$(INKEY$): REM user input
    REM engine loop subs
    GOSUB inputter: REM player input
    GOSUB collision: REM collision sub
    GOSUB footchanger: REM calculates when players foot needs changing
    GOSUB playermove: REM calculates NPC movement
    GOSUB screendraw: REM calls for a screen draw
    GOSUB triggerchecker: REM checks to see if any triggers have been activated
    GOSUB terminaldraw: REM displays a terminal if required
    REM post loop commands
    IF hertz > 0 THEN _LIMIT hertz: REM sets engine loops per second if needed
    GOSUB timeframecounter: REM diverts to time tracker and frame counter
LOOP

gameloop:
REM continues game loop for scripts
IF hertz > 0 THEN _LIMIT hertz: REM sets engine loops per second if needed
LET a$ = UCASE$(INKEY$): REM user input
GOSUB collision
GOSUB footchanger
GOSUB playermove
GOSUB timeframecounter
GOSUB screendraw
GOSUB inputter
RETURN

giveitem:
REM gives item to mainplayer
OPEN pocketloc$ + "pocketfiles.ddf" FOR INPUT AS #1
REM seaches for item in pocketfiles
DO
    LET temp63 = temp63 + 1
    INPUT #1, pocketfile$
LOOP UNTIL pocketfile$ = giveitem$ OR EOF(1)
CLOSE #1
IF pocketfile$ <> giveitem$ THEN
    REM if search finds nothing or currency is attempted to be removed
    REM prints to console
    LET eventtitle$ = "INVALID ITEM:"
    LET eventdata$ = giveitem$
    LET eventnumber = 0
    GOSUB consoleprinter
    LET temp63 = 0
    RETURN
END IF
REM assigns item
LET x = 0
DO
    LET x = x + 1
    IF temp63 = x THEN LET pocketitem(x) = 1
LOOP UNTIL x >= pocketnos
LET x = 0
REM prints to console
LET eventtitle$ = "ITEM GIVEN:"
LET eventdata$ = giveitem$
LET eventnumber = temp63
GOSUB consoleprinter
REM adds to pocket item counter
LET pocketcarry = pocketcarry + 1
IF silentgive = 0 THEN
    REM displays animation
    GOSUB slightfadeout
    DO
        REM pockets scroll in
        _LIMIT pockethudanispeed
        _PUTIMAGE (0, (0 - temp68))-(pockethudresx - 1, temp68), pockethud
        LET temp68 = temp68 + 1
    LOOP UNTIL temp68 >= pockethudresy
    LET temp68 = (0 - pocketspriteresx)
    DO
        REM Pocket item scrolls in
        _LIMIT pockethudanispeed
        _PUTIMAGE (0, 0)-(pockethudresx - 1, pockethudresy), pockethud
        _PUTIMAGE (temp68, pocketspritey)-((temp68 + pocketspriteresx), (pocketspritey + pocketspriteresy)), pocketsprite(temp63)
        LET temp68 = temp68 + 1
        IF speedrun > 0 THEN GOSUb timeframecounter: GOSUB displayspeedrun
    LOOP UNTIL temp68 >= pocketspritex
    REM plays sound effect
    LET playsfx$ = "pickup"
    GOSUB sfxplay
    _DELAY 0.5
    GOSUB slightfadein
END IF
LET temp63 = 0: LET temp68 = 0: LET silentgive = 0: REM scrub temp values
RETURN

takeitem:
REM takes item from mainplayer
OPEN pocketloc$ + "pocketfiles.ddf" FOR INPUT AS #1
REM seaches for item in pocketfiles
DO
    LET temp65 = temp65 + 1
    INPUT #1, pocketfile$
LOOP UNTIL pocketfile$ = takeitem$ OR EOF(1)
CLOSE #1
IF pocketfile$ <> takeitem$ THEN
    REM if search finds nothing or currency is attempted to be removed
    REM prints to console
    LET eventtitle$ = "INVALID ITEM:"
    LET eventdata$ = takeitem$
    LET eventnumber = 0
    GOSUB consoleprinter
    LET temp65 = 0
    RETURN
END IF
REM assigns item
LET x = 0
DO
    LET x = x + 1
    IF temp65 = x THEN LET pocketitem(x) = 0
LOOP UNTIL x >= pocketnos
LET x = 0
REM prints to console
LET eventtitle$ = "ITEM TAKEN:"
LET eventdata$ = takeitem$
LET eventnumber = temp65
GOSUB consoleprinter
REM takes from pocket item counter
LET pocketcarry = pocketcarry - 1
IF silenttake = 0 THEN
    REM displays animation
    GOSUB slightfadeout
    DO
        REM pockets scroll in
        _LIMIT pockethudanispeed
        _PUTIMAGE (0, (0 - temp69))-(pockethudresx - 1, temp69), pockethud
        LET temp69 = temp69 + 1
    LOOP UNTIL temp69 >= pockethudresy
    LET temp69 = pocketspritex
    DO
        REM Pocket item scrolls out
        _LIMIT pockethudanispeed
        _PUTIMAGE (0, 0)-(pockethudresx - 1, pockethudresy), pockethud
        _PUTIMAGE (temp69, pocketspritey)-((temp69 + pocketspriteresx), (pocketspritey + pocketspriteresy)), pocketsprite(temp65)
        LET temp69 = temp69 + 1
        IF speedrun > 0 THEN GOSUB timeframecounter: GOSUB displayspeedrun
    LOOP UNTIL temp69 >= (resx + 1)
    REM plays sound effect
    LET playsfx$ = "drop"
    GOSUB sfxplay
    _DELAY 0.5
    GOSUB slightfadein
END IF
LET temp65 = 0: LET temp69 = 0: LET silenttake = 0: REM scrub temp values
RETURN

markgone:
REM marks pocket item as "gone forever"
OPEN pocketloc$ + "pocketfiles.ddf" FOR INPUT AS #1
REM seaches for item in pocketfiles
DO
    LET temp95 = temp95 + 1
    INPUT #1, pocketfile$
LOOP UNTIL pocketfile$ = takeitem$ OR EOF(1)
CLOSE #1
IF pocketfile$ <> takeitem$ OR pocketfile$ = "currency" THEN
    REM if search finds nothing or currency is attempted to be removed
    REM prints to console
    LET eventtitle$ = "INVALID ITEM:"
    LET eventdata$ = takeitem$
    LET eventnumber = 0
    GOSUB consoleprinter
    LET temp95 = 0
    RETURN
END IF
REM assigns item
LET x = 0
DO
    LET x = x + 1
    IF temp95 = x THEN LET pocketitem(x) = 2
LOOP UNTIL x >= pocketnos
LET x = 0
REM prints to console
LET eventtitle$ = "ITEM FULLY TAKEN:"
LET eventdata$ = takeitem$
LET eventnumber = temp65
GOSUB consoleprinter
LET temp95 = 0: REM scrubs temp values
RETURN

ifholding:
REM checks players hand for item
IF runterminal = 1 THEN
    REM if terminal is running
    IF ifholding$ = terminalhold$ THEN LET ifholding = 1
ELSE
    REM if terminal isnt running
    IF ifholding$ = currentpocketshort$ THEN LET ifholding = 1
END IF
RETURN

ifmodel:
REM checks if player is using certain character sprite
IF ifmodel$ = mplayermodel$ THEN
    LET ifmodel = 1
ELSE
    LET ifmodel = 0
END IF
RETURN

ifmapno:
REM checks if player is on a certain map
IF ifmapno = mapno THEN
    LET ifmapnoresult = 1
ELSE
    LET ifmapnoresult = 0
END IF
RETURN

ifgone:
REM checks for pocket item
OPEN pocketloc$ + "pocketfiles.ddf" FOR INPUT AS #1
REM seaches for item in pocketfiles
DO
    LET temp122 = temp122 + 1
    INPUT #1, pocketfile$
LOOP UNTIL pocketfile$ = ifgone$ OR EOF(1)
CLOSE #1
IF pocketfile$ <> ifgone$ THEN
    REM if search finds nothing
    REM prints to console
    LET eventtitle$ = "INVALID ITEM:"
    LET eventdata$ = ifgone$
    LET eventnumber = 0
    GOSUB consoleprinter
    LET temp122 = 0
    RETURN
END IF
REM assigns item
LET x = 0
DO
    LET x = x + 1
    IF temp122 = x THEN LET ifgone = pocketitem(x)
LOOP UNTIL x >= pocketnos
LET x = 0
LET temp122 = 0: REM scrub temp values
RETURN

ifpocket:
REM checks for if pocket item has been marked "gone forever"
OPEN pocketloc$ + "pocketfiles.ddf" FOR INPUT AS #1
REM seaches for item in pocketfiles
DO
    LET temp85 = temp85 + 1
    INPUT #1, pocketfile$
LOOP UNTIL pocketfile$ = ifpocket$ OR EOF(1)
CLOSE #1
IF pocketfile$ <> ifpocket$ THEN
    REM if search finds nothing
    REM prints to console
    LET eventtitle$ = "INVALID ITEM:"
    LET eventdata$ = ifpocket$
    LET eventnumber = 0
    GOSUB consoleprinter
    LET temp85 = 0
    RETURN
END IF
REM assigns item
LET x = 0
DO
    LET x = x + 1
    IF temp85 = x THEN LET ifpocket = pocketitem(x)
LOOP UNTIL x >= pocketnos
LET x = 0
LET temp85 = 0: REM scrub temp values
RETURN

terminalload:
REM loads terminal data
LET tani1 = _LOADIMAGE(tloc$ + "tani1.png")
LET tani2 = _LOADIMAGE(tloc$ + "tani2.png")
LET tani3 = _LOADIMAGE(tloc$ + "tani3.png")
LET tani4 = _LOADIMAGE(tloc$ + "tani4.png")
LET tfile = _LOADIMAGE(tloc$ + "file.png")
LET tdir = _LOADIMAGE(tloc$ + "dir.png")
LET tno = _LOADIMAGE(tloc$ + "nodata.png")
LET tapp = _LOADIMAGE(tloc$ + "app.png")
LET tselectn = _LOADIMAGE(tloc$ + "selectn.png")
LET tselectd = _LOADIMAGE(tloc$ + "selectd.png")
LET tselectf = _LOADIMAGE(tloc$ + "selectf.png")
LET sysok = _LOADIMAGE(tloc$ + "sysok.png")
LET sysbusy = _LOADIMAGE(tloc$ + "sysbusy.png")
LET syserr = _LOADIMAGE(tloc$ + "syserr.png")
REM prints to terminal
LET eventtitle$ = "TERMINAL ITEMS LOADED"
LET eventdata$ = ""
LET eventnumber = 0
GOSUB consoleprinter
IF setupboot = 1 THEN
    LET temp125 = 100
    GOSUB loadbar
END IF
RETURN

terminalunload:
REM unloads terminal data
_FREEIMAGE tani1
_FREEIMAGE tani2
_FREEIMAGE tani3
_FREEIMAGE tani4
_FREEIMAGE tfile
_FREEIMAGE tdir
_FREEIMAGE tno
_FREEIMAGE tapp
_FREEIMAGE tselectn
_FREEIMAGE tselectd
_FREEIMAGE tselectf
_FREEIMAGE sysok
_FREEIMAGE sysbusy
_FREEIMAGE syserr
REM prints to terminal
LET eventtitle$ = "TERMINAL ITEMS UNLOADED"
LET eventdata$ = ""
LET eventnumber = 0
GOSUB consoleprinter
RETURN

terminaldraw:
REM displays terminal
IF runterminal = 0 THEN RETURN
REM checks if terminal file exists
IF _FILEEXISTS(tloc$ + runterminal$ + "\" + runterminal$ + ".ddf") THEN
    REM nothing
ELSE
    ERROR 425
    RETURN
END IF
GOSUB fadeout
REM transfers values
LET temp87 = tdelay
LET temp88 = stposx
LET temp89 = stposy
REM loads terminal data
OPEN tloc$ + runterminal$ + "\" + runterminal$ + ".ddf" FOR INPUT AS #1
INPUT #1, ct1, cn1$, ct2, cn2$, ct3, cn3$, ct4, cn4$, ct5, cn5$, ct6, cn6$, parentdir$
CLOSE #1
REM tells console
LET eventtitle$ = "TERMINAL LAUNCHED:"
LET eventdata$ = runterminal$
LET eventnumber = 0
GOSUB consoleprinter
REM display terminal open animation (if directory isnt open)
IF terminaldir = 0 THEN
    LET playsfx$ = "terminalon": GOSUB sfxplay: REM plays sound efffect
    FOR x = 1 TO 5
        IF x = 1 THEN _PUTIMAGE (30, 10)-(resx - 30, resy - 10), tani1
        IF x = 2 THEN _PUTIMAGE (30, 10)-(resx - 30, resy - 10), tani2
        IF x = 3 THEN _PUTIMAGE (30, 10)-(resx - 30, resy - 10), tani3
        IF x = 4 THEN _PUTIMAGE (30, 10)-(resx - 30, resy - 10), tani4
        IF speedrun > 0 THEN GOSUB timeframecounter: GOSUB displayspeedrun
        _DELAY tanidelay
    NEXT x
END IF
termloop:
CLS
REM display terminal files animation
IF temp147 <> 1 THEN _PUTIMAGE (terminalfacex, terminalfacey), sysbusy
IF ct1 = 0 THEN _PUTIMAGE (terminalcol1, terminalrow1), tno
IF ct1 = 1 THEN _PUTIMAGE (terminalcol1, terminalrow1), tfile
IF ct1 = 2 THEN _PUTIMAGE (terminalcol1, terminalrow1), tdir
IF ct1 = 3 THEN _PUTIMAGE (terminalcol1, terminalrow1), tapp
_DELAY temp87: LET temp87 = temp87 / 2
IF ct2 = 0 THEN _PUTIMAGE (terminalcol2, terminalrow1), tno
IF ct2 = 1 THEN _PUTIMAGE (terminalcol2, terminalrow1), tfile
IF ct2 = 2 THEN _PUTIMAGE (terminalcol2, terminalrow1), tdir
IF ct2 = 3 THEN _PUTIMAGE (terminalcol2, terminalrow1), tapp
_DELAY temp87: LET temp87 = temp87 / 2
IF ct3 = 0 THEN _PUTIMAGE (terminalcol3, terminalrow1), tno
IF ct3 = 1 THEN _PUTIMAGE (terminalcol3, terminalrow1), tfile
IF ct3 = 2 THEN _PUTIMAGE (terminalcol3, terminalrow1), tdir
IF ct3 = 3 THEN _PUTIMAGE (terminalcol3, terminalrow1), tapp
_DELAY temp87: LET temp87 = temp87 / 2
IF ct4 = 0 THEN _PUTIMAGE (terminalcol1, terminalrow2), tno
IF ct4 = 1 THEN _PUTIMAGE (terminalcol1, terminalrow2), tfile
IF ct4 = 2 THEN _PUTIMAGE (terminalcol1, terminalrow2), tdir
IF ct4 = 3 THEN _PUTIMAGE (terminalcol1, terminalrow2), tapp
_DELAY temp87: LET temp87 = temp87 / 2
IF ct5 = 0 THEN _PUTIMAGE (terminalcol2, terminalrow2), tno
IF ct5 = 1 THEN _PUTIMAGE (terminalcol2, terminalrow2), tfile
IF ct5 = 2 THEN _PUTIMAGE (terminalcol2, terminalrow2), tdir
IF ct5 = 3 THEN _PUTIMAGE (terminalcol2, terminalrow2), tapp
_DELAY temp87: LET temp87 = temp87 / 2
IF ct6 = 0 THEN _PUTIMAGE (terminalcol3, terminalrow2), tno
IF ct6 = 1 THEN _PUTIMAGE (terminalcol3, terminalrow2), tfile
IF ct6 = 2 THEN _PUTIMAGE (terminalcol3, terminalrow2), tdir
IF ct6 = 3 THEN _PUTIMAGE (terminalcol3, terminalrow2), tapp
_DELAY temp87
_PUTIMAGE (terminalfacex, terminalfacey), sysok
IF temp88 = terminalcol1 AND temp89 = terminalrow1 THEN LET ttype = ct1: LET tselect$ = cn1$
IF temp88 = terminalcol2 AND temp89 = terminalrow1 THEN LET ttype = ct2: LET tselect$ = cn2$
IF temp88 = terminalcol3 AND temp89 = terminalrow1 THEN LET ttype = ct3: LET tselect$ = cn3$
IF temp88 = terminalcol1 AND temp89 = terminalrow2 THEN LET ttype = ct4: LET tselect$ = cn4$
IF temp88 = terminalcol2 AND temp89 = terminalrow2 THEN LET ttype = ct5: LET tselect$ = cn5$
IF temp88 = terminalcol3 AND temp89 = terminalrow2 THEN LET ttype = ct6: LET tselect$ = cn6$
COLOR _RGBA(letterminalcolourr, letterminalcolourg, letterminalcolourb, letterminalcoloura), _RGBA(bgterminalcolourr, bgterminalcolourg, bgterminalcolourb, bgterminalcoloura)
PRINT tos$
IF ttype = 1 THEN PRINT "file - "; tselect$: _PUTIMAGE (temp88 - 1, temp89 - 1), tselectf
IF ttype = 2 THEN PRINT "folder - "; tselect$: _PUTIMAGE (temp88 - 1, temp89 - 1), tselectd
IF ttype = 3 THEN PRINT "app - "; tselect$: _PUTIMAGE (temp88 - 1, temp89 - 1), tselectf
IF ttype = 0 THEN PRINT "no data": _PUTIMAGE (temp88 - 1, temp89 - 1), tselectn
LET temp147 = 1
REM input loop
_KEYCLEAR
DO
	_LIMIT 1000
	IF speedrun > 0 THEN GOSUB timeframecounter: GOSUB displayspeedrun
    LET t$ = UCASE$(INKEY$): REM terminal user input
    IF t$ = CHR$(0) + CHR$(72) THEN 
		REM up
		IF temp89 <> terminalrow1 THEN
			LET temp89 = terminalrow1
			LET playsfx$ = "move"
			GOSUB sfxplay
			GOTO termloop
		END IF
	END IF
    IF t$ = CHR$(0) + CHR$(80) THEN 
		REM down
		IF temp89 <> terminalrow2 THEN
			LET temp89 = terminalrow2
			LET playsfx$ = "move"
			GOSUB sfxplay
			GOTO termloop
		END IF
	END IF
    REM left
    IF t$ = CHR$(0) + CHR$(75) THEN
		IF temp88 <> terminalcol1 THEN
			LET playsfx$ = "move"
			GOSUB sfxplay
		END IF
        IF temp88 = terminalcol1 THEN LET temp88 = terminalcol1: GOTO termloop
        IF temp88 = terminalcol2 THEN LET temp88 = terminalcol1: GOTO termloop
        IF temp88 = terminalcol3 THEN LET temp88 = terminalcol2: GOTO termloop
    END IF
    REM right
    IF t$ = CHR$(0) + CHR$(77) THEN
		IF temp88 <> terminalcol3 THEN
			LET playsfx$ = "move"
			GOSUB sfxplay
		END IF
        IF temp88 = terminalcol1 THEN LET temp88 = terminalcol2: GOTO termloop
        IF temp88 = terminalcol2 THEN LET temp88 = terminalcol3: GOTO termloop
        IF temp88 = terminalcol3 THEN LET temp88 = terminalcol3: GOTO termloop
    END IF
    IF t$ = " " THEN
        REM file type
        IF ttype = 1 THEN
            LET playsfx$ = "select"
            GOSUB sfxplay
            GOSUB readtxt
            GOTO termloop
        END IF
        REM directory type
        IF ttype = 2 THEN
            LET playsfx$ = "select"
            GOSUB sfxplay
            CLS
            LET runterminal$ = tselect$
            LET terminaldir = 1
            GOTO terminaldraw
        END IF
        REM app type
        IF ttype = 3 THEN
            LET playsfx$ = "select"
            GOSUB sfxplay
            IF tselect$ = "exit" THEN GOTO endterm: REM quits terminal
            IF tselect$ = "back" THEN
                REM goes back a directory
                CLS
                LET runterminal$ = parentdir$
                LET terminaldir = 2
                GOTO terminaldraw
            END IF
            REM runs script
            LET scriptname$ = tselect$
            LET mapscript = 1
            GOSUB script
            LET terminaldivert = 1
            GOTO endterm
        END IF
    END IF
LOOP
endterm:
IF terminaldivert = 0 THEN
    REM plays sound
    LET playsfx$ = "terminaloff"
    GOSUB sfxplay
END IF
IF terminaldivert = 1 THEN LET terminaldivert = 0
COLOR 0, 0
REM tells console
LET eventtitle$ = "TERMINAL STOPPED:"
LET eventdata$ = runterminal$
LET eventnumber = 0
GOSUB consoleprinter
REM return to game
GOSUB fadein
LET runterminal = 0: LET temp87 = 0: LET temp88 = 0: LET temp89 = 0: LET temp147 = 0: LET terminaldir = 0: LET terminalhold$ = "": REM scub temp values
RETURN

ifcurrency:
REM checks to see if player has enough currency
IF currency >= ifcurrencyamount THEN
    LET ifcurrencyresult = 1
ELSE
    LET ifcurrencyresult = 0
END IF
RETURN

readtxt:
REM terminal file opener
CLS
COLOR _RGBA(letterminalcolourr, letterminalcolourg, letterminalcolourb, letterminalcoloura), _RGBA(bgterminalcolourr, bgterminalcolourg, bgterminalcolourb, bgterminalcoloura)
LET temp90 = tdelay
_PUTIMAGE (terminalfacex, terminalfacey), sysbusy
OPEN tloc$ + runterminal$ + "\" + tselect$ + ".ddf" FOR INPUT AS #1
INPUT #1, txtfile1$, txtfile2$, txtfile3$, txtfile4$, txtfile5$, txtfile6$, sysstat
CLOSE #1
PRINT tos$
PRINT
PRINT txtfile1$
_DELAY temp90: LET temp90 = temp90 / 2
PRINT txtfile2$
_DELAY temp90: LET temp90 = temp90 / 2
PRINT txtfile3$
_DELAY temp90: LET temp90 = temp90 / 2
PRINT txtfile4$
_DELAY temp90: LET temp90 = temp90 / 2
PRINT txtfile5$
_DELAY temp90: LET temp90 = temp90 / 2
PRINT txtfile6$
PRINT: PRINT
PRINT "..."
REM print to console
LET eventtitle$ = "TERMINAL FILE OPEN:"
LET eventdata$ = tselect$
LET eventnumber = 0
GOSUB consoleprinter
IF sysstat = 1 THEN _PUTIMAGE (terminalfacex, terminalfacey), sysok
IF sysstat = 2 THEN _PUTIMAGE (terminalfacex, terminalfacey), sysbusy
IF sysstat = 3 THEN _PUTIMAGE (terminalfacex, terminalfacey), syserr
_KEYCLEAR
DO
	_LIMIT 1000
	IF speedrun > 0 THEN GOSUB timeframecounter: GOSUB displayspeedrun: COLOR _RGBA(letterminalcolourr, letterminalcolourg, letterminalcolourb, letterminalcoloura), _RGBA(bgterminalcolourr, bgterminalcolourg, bgterminalcolourb, bgterminalcoloura)
LOOP UNTIL INKEY$ = " "
LET playsfx$ = "select"
GOSUB sfxplay
LET tselect$ = "": LET temp90 = 0: REM scrub temp values
CLS
RETURN

showimage:
REM displays an image on screen
REM loads values
LET fullscreenimage = _LOADIMAGE(uiloc$ + showimage$ + ".png")
LET eventtitle$ = "FULLSCREEN IMAGE LOADED:"
LET eventdata$ = showimage$
LET eventnumber = 0
GOSUB consoleprinter
REM fades out game
GOSUB fadeout
REM fades in with image
FOR i% = 255 TO 0 STEP -5
    _LIMIT fadespeed: REM sets framerate
    _PUTIMAGE (0, 0)-(resx, resy), fullscreenimage
    LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, i%), BF: REM slowly fills screen with black box
    GOSUB timeframecounter: REM timer function
     IF speedrun > 0 THEN GOSUB timeframecounter: GOSUB displayspeedrun
    _DISPLAY
NEXT
_AUTODISPLAY
_PUTIMAGE (0, 0)-(resx, resy), fullscreenimage: REM displays image
IF speedrun > 0 THEN GOSUB timeframecounter: GOSUB displayspeedrun
LET eventtitle$ = "FULLSCREEN IMAGE DISPLAYED:"
LET eventdata$ = showimage$
LET eventnumber = 0
GOSUB consoleprinter
DO
	IF speedrun = 1 THEN GOSUB timeframecounter: GOSUB displayspeedrun
	_LIMIT hertz
LOOP UNTIL INKEY$ = " "
REM fade out with image
FOR i% = 0 TO 255 STEP 5
    _LIMIT fadespeed: REM sets framerate
    _PUTIMAGE (0, 0)-(resx, resy), fullscreenimage
    LINE (0, 0)-(resx, resy), _RGBA(0, 0, 0, i%), BF: REM slowly empties black box from screen
    IF speedrun > 0 THEN GOSUB timeframecounter: GOSUB displayspeedrun
    _DISPLAY
NEXT
_AUTODISPLAY
REM fade in game
GOSUB fadein
REM free image
_FREEIMAGE fullscreenimage
LET eventtitle$ = "FULLSCREEN IMAGE UNLOADED:"
LET eventdata$ = showimage$
LET eventnumber = 0
GOSUB consoleprinter
RETURN

coordinatefixer:
REM fixes any broken co-ordinates
LET findissue% = INSTR(findissue% + 1, temp13$, "x ")
IF findissue% THEN
    LET temp33$ = "x "
ELSE
    LET findissue% = INSTR(findissue% + 1, temp13$, "y ")
END IF
IF findissue% THEN
    IF temp33$ = "" THEN LET temp33$ = "y "
    LET temp135 = temp135 + 2
    LET temp13$ = RIGHT$(scriptline$, INSTR(scriptline$, " ") - temp135)
END IF
LET findissue% = 0: LET temp33$ = "": LET temp135 = 0: REM scrub temp values
RETURN

script:
REM VaME STAGE DIRECTOR SCRIPT UTILITY
LET scriptrun = 1: REM sets script value to running
IF mapscript = 1 THEN OPEN scriptloc$ + mapdir$ + scriptname$ + ".vsf" FOR INPUT AS #3: REM opens map specific script file
IF mapscript = 2 THEN OPEN scriptloc$ + "combine/" + scriptname$ + ".vsf" FOR INPUT AS #3: REM opens pocket combination script
IF mapscript = 3 THEN OPEN scriptloc$ + "image/" + scriptname$ + ".vsf" FOR INPUT AS #3: REM opens a pocket looking script
LET oldscript$ = scriptname$
IF triggerspoofa = 1 THEN LET triggerspoofa = 0
LET mpwalking = 0: REM prevents eternal moonwalk
REM prints to console
LET eventtitle$ = "SCRIPT LAUNCHED:"
LET eventdata$ = scriptname$
LET eventnumber = 0
GOSUB consoleprinter
DO
    LET scriptline = scriptline + 1: REM counts lines of script
    REM inputs a line from script file and searches for key commands and arguments
    INPUT #3, scriptline$
    LET findfade% = INSTR(findfade% + 1, scriptline$, "fade")
    LET findwait% = INSTR(findwait% + 1, scriptline$, "wait")
    LET findmap% = INSTR(findmap% + 1, scriptline$, "map ")
    LET findwarp% = INSTR(findwarp% + 1, scriptline$, "warp")
    LET findmainplayer% = INSTR(findin% + 1, scriptline$, "mainplayer")
    LET findx% = INSTR(findx% + 1, scriptline$, " x ")
    LET findy% = INSTR(findy% + 1, scriptline$, " y ")
    LET findin% = INSTR(findin% + 1, scriptline$, "in")
    LET findout% = INSTR(findout% + 1, scriptline$, "out")
    LET finddirection% = INSTR(finddirection% + 1, scriptline$, "direction")
    LET findmove% = INSTR(findmove% + 1, scriptline$, "move")
    LET findmodel% = INSTR(findmodel% + 1, scriptline$, "model")
    LET findon% = INSTR(findon% + 1, scriptline$, "on")
    LET findoff% = INSTR(findoff% + 1, scriptline$, "off")
    LET findcollision% = INSTR(findcollision% + 1, scriptline$, "collision")
    LET findscript% = INSTR(findscript% + 1, scriptline$, "script")
    LET findmusic% = INSTR(findmusic% + 1, scriptline$, "music")
    LET findcontrol% = INSTR(findcontrol% + 1, scriptline$, "control ")
    LET findplay% = INSTR(findplay% + 1, scriptline$, " play")
    LET findstop% = INSTR(findstop% + 1, scriptline$, " stop")
    LET findfile% = INSTR(findfile% + 1, scriptline$, " file ")
    LET findpause% = INSTR(findpause% + 1, scriptline$, " pause")
    LET findsfx% = INSTR(findsfx% + 1, scriptline$, "sfx")
    LET findhalt% = INSTR(findhalt% + 1, scriptline$, "halt")
    LET findplayer% = INSTR(findplayer% + 1, scriptline$, " player ")
    LET x = 0
    DO
        LET x = x + 1
        LET findplayer(x) = INSTR(findplayer(x) + 1, scriptline$, playername(x))
    LOOP UNTIL x >= mapplayerno
    LET x = 0
    LET findpilot% = INSTR(findpilot% + 1, scriptline$, "pilot ")
    LET finddim% = INSTR(finddim% + 1, scriptline$, "dim ")
    LET findgive% = INSTR(findgive% + 1, scriptline$, "give ")
    LET findtake% = INSTR(findtake% + 1, scriptline$, "take ")
    LET findsay% = INSTR(findsay% + 1, scriptline$, "say ")
    LET findspeaker% = INSTR(findspeaker% + 1, scriptline$, "speaker ")
    LET findclear% = INSTR(findclear% + 1, scriptline$, "clear")
    LET findeffects% = INSTR(findeffects% + 1, scriptline$, "effects ")
    LET findifpocket% = INSTR(findifpocket% + 1, scriptline$, "ifpocket ")
    LET findterminal% = INSTR(findterminal% + 1, scriptline$, "terminal ")
    LET findgivecurrency% = INSTR(findgivecurrency% + 1, scriptline$, "givecurrency ")
    LET findtakecurrency% = INSTR(findtakecurrency% + 1, scriptline$, "takecurrency ")
    LET findifholding% = INSTR(findifholding% + 1, scriptline$, "ifholding ")
    LET findifcurrency% = INSTR(findifcurrency% + 1, scriptline$, "ifcurrency ")
    LET findmarkgone% = INSTR(findmarkgone% + 1, scriptline$, "markgone ")
    LET findloading% = INSTR(findloading% + 1, scriptline$, "loading")
    LET findmapeffect% = INSTR(findloading% + 1, scriptline$, "mapeffect ")
    LET finddark% = INSTR(finddark% + 1, scriptline$, "dark")
    LET findrain% = INSTR(findrain% + 1, scriptline$, "rain")
    LET findstorm% = INSTR(findstorm% + 1, scriptline$, "storm")
    LET findtorch% = INSTR(findtorch% + 1, scriptline$, "torch")
    LET findsunsetleft% = INSTR(findsunsetleft% + 1, scriptline$, "sunsetleft")
    LET findsunsetright% = INSTR(findsunsetright% + 1, scriptline$, "sunsetright")
    LET findsunsetdown% = INSTR(findsunsetdown% + 1, scriptline$, "sunsetdown")
    LET findsunsetup% = INSTR(findsunsetup% + 1, scriptline$, "sunsetup")
    LET findanimate% = INSTR(findanimate% + 1, scriptline$, "animate ")
    LET findsavegame% = INSTR(findsavegame% + 1, scriptline$, "savegame")
    LET findifgone% = INSTR(findifgone% + 1, scriptline$, "ifgone ")
    LET findsprint% = INSTR(findsprint% + 1, scriptline$, "sprint ")
    LET findshowimage% = INSTR(findshowimage% + 1, scriptline$, "showimage ")
    LET findslowfade% = INSTR(findslowfade% + 1, scriptline$, "slowfade ")
    LET findsilenttake% = INSTR(findsilenttake% + 1, scriptline$, "silenttake ")
    LET findsilentgive% = INSTR(findsilentgive% + 1, scriptline$, "silentgive ")
    LET findsilentgivecurrency% = INSTR(findsilentgivecurrency% + 1, scriptline$, "silentgivecurrency ")
    LET findsilenttakecurrency% = INSTR(findsilenttakecurrency% + 1, scriptline$, "silenttakecurrency ")
    LET findifmapno% = INSTR(findifmapno% + 1, scriptline$, "ifmapno ")
    LET findifmodel% = INSTR(findifmodel% + 1, scriptline$, "ifmodel ")
    LET findfaceplayer% = INSTR(findfaceplayer% + 1, scriptline$, "faceplayer")
    LET findback% = INSTR(findback% + 1, scriptline$, "back")
    LET findrun% = INSTR(findrun% + 1, scriptline$, "run ")
    LET findminus% = INSTR(findminus% + 1, scriptline$, "-")
    LET findifdirection% = INSTR(ifdirection% + 1, scriptline$, "ifdirection ")
    LET findcarryvalues% = INSTR(findcarryvalues% + 1, scriptline$, "carryvalues")
    LET findpitchblack% = INSTR(findpitchblack% + 1, scriptline$, " pitchblack")
    LET findloadgame% = INSTR(findloadgame% + 1, scriptline$, "loadgame")
    LET findobject% = INSTR(findobject% + 1, scriptline$, " object ")
    LET x = 0
    DO
        LET x = x + 1
        LET findobject(x) = INSTR(findobject(x) + 1, scriptline$, objectname(x))
    LOOP UNTIL x >= mapobjectno
    LET x = 0
    LET findcheckpoint% = INSTR(findcheckpoint% + 1, scriptline$, "checkpoint ")
    LET findifcheckpoint% = INSTR(findifcheckpoint% + 1, scriptline$, "ifcheckpoint ")
    LET findpockets% = INSTR(findpockets% + 1, scriptline$, " pockets ")
    LET findup% = INSTR(findup% + 1, scriptline$, " up ")
    LET finddown% = INSTR(finddown% + 1, scriptline$, " down ")
    LET findleft% = INSTR(findleft% + 1, scriptline$, " left ")
    LET findright% = INSTR(findright% + 1, scriptline$, " right ")
    LET findselect% = INSTR(findselect% + 1, scriptline$, " select ")
    LET findcompletespeedrun% = INSTR(findcompletespeedrun% + 1, scriptline$, "complete speedrun")
    REM processes a line
    IF findsay% THEN
        REM display text in text banner
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET textspeech$ = temp13$
        GOSUB textbannerdraw
        LET temp26 = 1
        GOTO scriptsay: REM skips other search terms to prevent executing additional commands with said words
    END IF
    IF findcompletespeedrun% THEN
		REM marks speedrun as complete and prints time to file
		IF speedrun = 1 THEN
			LET speedrun = 2
			LET speedrunhour = ctime \ 3600
			LET speedrunmin = (ctime - (3600 * speedrunhour)) \ 60
			LET speedrunsec = (ctime - (3600 * speedrunhour)) - (speedrunmin * 60)
			OPEN "speedrun - " + DATE$ + "-" + TIME$ + ".txt" FOR OUTPUT AS #54336
			WRITE #54336, speedrunhour, speedrunmin, speedrunsec
			CLOSE #54336
			LET temp26 = 1
			REM print to console
			LET eventtitle$ = "SPEEDRUN COMPLETE:"
			LET eventdata$ = STR$(speedrunhour) + ":" + STR$(speedrunmin) + ":" + STR$(speedrunsec)
			LET eventnumber = 0
			GOSUB consoleprinter
		ELSE
			REM if speedrun isn't active
			LET temp26 = 1
			LET eventtitle$ = "SPEEDRUN MODE NOT ACTIVE!"
			LET eventdata$ = ""
			LET eventnumber = 0
			GOSUB consoleprinter
		END IF
    END IF
    IF findcarryvalues% THEN
        REM copies NPC location values into memory to be copied to the next map
        LET x = 0
        DO
            LET x = x + 1
            LET carryplayerx(x) = playerx(x): LET carryplayery(x) = playery(x)
            LET carryplayerd(x) = playerd(x): LET carryplayerjourney(x) = playerjourney(x)
            LET carryplayerlayer(x) = playerlayer(x): LET carryplayerperiod(x) = playerperiod(x)
        LOOP UNTIL x >= mapplayerno
        LET x = 0
        LET carryvalues = 1
        LET temp26 = 1
        REM tells console
        LET eventtitle$ = "PLAYER LOCATION VALUES COPIED!"
        LET eventdata$ = ""
        LET eventnumber = 0
        GOSUB consoleprinter
    END IF
    IF findcontrol% THEN
        REM enables or disables a player input
        IF findup% THEN
            REM enables or disables up control
            IF findon% THEN LET ucontrol = 1: LET eventdata$ = "up control enabled": LET temp26 = 1
            IF findoff% THEN LET ucontrol = 0: LET eventdata$ = "up control disabled": LET temp26 = 1
        END IF
        IF finddown% THEN
            REM enables or disables down control
            IF findon% THEN LET dcontrol = 1: LET eventdata$ = "down control enabled": LET temp26 = 1
            IF findoff% THEN LET dcontrol = 0: LET eventdata$ = "down control disabled": LET temp26 = 1
        END IF
        IF findleft% THEN
            REM enables or disables left control
            IF findon% THEN LET lcontrol = 1: LET eventdata$ = "left control enabled": LET temp26 = 1
            IF findoff% THEN LET lcontrol = 0: LET eventdata$ = "left control disabled": LET temp26 = 1
        END IF
        IF findright% THEN
            REM enables or disables right control
            IF findon% THEN LET rcontrol = 1: LET eventdata$ = "right control enabled": LET temp26 = 1
            IF findoff% THEN LET rcontrol = 0: LET eventdata$ = "right control disabled": LET temp26 = 1
        END IF
        IF findselect% THEN
            REM enables or disables select control
            IF findon% THEN LET scontrol = 1: LET eventdata$ = "select control enabled": LET temp26 = 1
            IF findoff% THEN LET scontrol = 0: LET eventdata$ = "select control disabled": LET temp26 = 1
        END IF
        IF findpockets% THEN
            REM enables or disables up control
            IF findon% THEN LET pcontrol = 1: LET eventdata$ = "pocket control enabled": LET temp26 = 1
            IF findoff% THEN LET pcontrol = 0: LET eventdata$ = "pocket control disabled": LET temp26 = 1
        END IF
        IF findback% THEN
            REM enables or disables up control
            IF findon% THEN LET bcontrol = 1: LET eventdata$ = "back control enabled": LET temp26 = 1
            IF findoff% THEN LET bcontrol = 0: LET eventdata$ = "back control disabled": LET temp26 = 1
        END IF
        IF temp26 = 1 THEN
            REM prints to console
            LET eventtitle$ = "INPUT CONTROL:"
            LET eventnumber = 0
            GOSUB consoleprinter
        END IF
    END IF
    IF findrun% THEN
        REM launches external application
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        CLS
        REM tells console
        LET eventtitle$ = "EXTERNAL APP:"
        LET eventdata$ = "Attempting to run " + temp13$ + "..."
        LET eventnumber = 0
        GOSUB consoleprinter
        REM saves game
        IF exitsave = 1 THEN GOSUB savesave
        REM copies savedata file
        IF ros$ = "mac" OR ros$ = "lnx" THEN SHELL _HIDE "cp " + sloc$ + "savedata.ddf /var/data/polydata/savedata.ddf"
        IF ros$ = "win" THEN SHELL _HIDE "copy " + sloc$ + "savedata.ddf " + dloc$ + "utility\savedata.ddf"
        REM hides window
        IF screenmode <> 1 THEN
            _SCREENHIDE
        ELSE
            _FULLSCREEN _OFF
        END IF
        REM assigns permissions (macos + linux)
        IF ros$ = "mac" THEN SHELL "chmod 755 " + dloc$ + "utility/" + temp13$ + "_macos"
        'IF ros$ = "lnx" THEN SHELL "chmod +x " + temp13$ + "_linux"
        REM runs app
        IF ros$ = "mac" THEN SHELL "./" + dloc$ + "utility/" + temp13$ + "_macos"
        IF ros$ = "lnx" THEN SHELL "./" + temp13$ + "_linux"
        IF ros$ = "win" THEN SHELL dloc$ + "utility\" + temp13$ + "_win.exe"
        REM deletes savedata file
        IF ros$ = "mac" OR ros$ = "lnx" THEN SHELL _HIDE "rm /var/data/polydata/savedata.ddf"
        IF ros$ = "win" THEN SHELL _HIDE "del " + dloc$ + "utility\savedata.ddf"
        REM shows window
        IF screennode <> 1 THEN
            _SCREENSHOW
        ELSE
            _FULLSCREEN _SQUAREPIXELS
        END IF
        REM tells console
        LET eventtitle$ = "EXTERNAL APP:"
        LET eventdata$ = temp13$ + " closed!"
        LET eventnumber = 0
        GOSUB consoleprinter
        LET temp26 = 1
        COLOR 0, 0
        GOTO scriptsay: REM skips other search terms
    END IF
    IF findpilot% THEN
        REM sets an NPC for script control
        IF findon% THEN
            LET x = 0
            DO
                LET x = x + 1
                IF findplayer(x) THEN LET playerscript(x) = 1: LET playerwalking(x) = 0: LET playerperiod(x) = 0: LET temp26 = 1
            LOOP UNTIL x >= mapplayerno
            LET x = 0
        END IF
        IF findoff% THEN
            LET x = 0
            DO
                LET x = x + 1
                IF findplayer(x) THEN LET playerscript(x) = 0: LET playerperiod(x) = INT(ctime): LET temp26 = 1
            LOOP UNTIL x >= mapplayerno
            LET x = 0
        END IF
    END IF
    IF findsavegame% THEN
        REM saves game
        GOSUB savesave
        LET temp26 = 1
    END IF
    IF findloadgame% THEN
        REM loads previous save
        GOSUB loadgame
        LET temp26 = 1
    END IF
    IF findmapeffect% THEN
        REM map effects
        IF findoff% THEN
            LET mapeffect = 0
            LET temp26 = 1
            LET eventtitle$ = "MAP EFFECT:"
            LET eventdata$ = "off"
            LET eventnumber = mapeffect
            GOSUB consoleprinter
        END IF
        IF finddark% THEN
            LET mapeffect = 1
            LET temp26 = 1
            LET eventtitle$ = "MAP EFFECT:"
            LET eventdata$ = "dark"
            LET eventnumber = mapeffect
            GOSUB consoleprinter
        END IF
        IF findrain% THEN
            LET mapeffect = 2
            LET temp26 = 1
            LET eventtitle$ = "MAP EFFECT:"
            LET eventdata$ = "rain"
            LET eventnumber = mapeffect
            GOSUB consoleprinter
        END IF
        IF findstorm% THEN
            LET mapeffect = 3
            LET temp26 = 1
            LET eventtitle$ = "MAP EFFECT:"
            LET eventdata$ = "storm"
            LET eventnumber = mapeffect
            GOSUB consoleprinter
        END IF
        IF findtorch% THEN
            LET mapeffect = 4
            LET temp26 = 1
            LET eventtitle$ = "MAP EFFECT:"
            LET eventdata$ = "torch"
            LET eventnumber = mapeffect
            GOSUB consoleprinter
        END IF
        IF findsunsetright% THEN
            LET mapeffect = 5
            LET temp26 = 1
            LET eventtitle$ = "MAP EFFECT:"
            LET eventdata$ = "sunset (right)"
            LET eventnumber = mapeffect
            GOSUB consoleprinter
        END IF
        IF findsunsetleft% THEN
            LET mapeffect = 6
            LET temp26 = 1
            LET eventtitle$ = "MAP EFFECT:"
            LET eventdata$ = "sunset (left)"
            LET eventnumber = mapeffect
            GOSUB consoleprinter
        END IF
        IF findsunsetdown% THEN
            LET mapeffect = 7
            LET temp26 = 1
            LET eventtitle$ = "MAP EFFECT:"
            LET eventdata$ = "sunset (down)"
            LET eventnumber = mapeffect
            GOSUB consoleprinter
        END IF
        IF findsunsetup% THEN
            LET mapeffect = 8
            LET temp26 = 1
            LET eventtitle$ = "MAP EFFECT:"
            LET eventdata$ = "sunset (up)"
            LET eventnumber = mapeffect
            GOSUB consoleprinter
        END IF
        IF findpitchblack% THEN
            LET mapeffect = 9
            LET temp26 = 1
            LET eventtitle$ = "MAP EFFECT:"
            LET eventdata$ = "pitch black"
            LET eventnumber = mapeffect
            GOSUB consoleprinter
        END IF
    END IF
    IF findgivecurrency% OR findsilentgivecurrency% THEN
        REM gives player currency
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET currencychange = VAL(temp13$)
        IF findsilentgivecurrency% THEN LET silentgive = 1
        GOSUB givecurrency
        LET temp26 = 1
    END IF
    IF findtakecurrency% OR findsilenttakecurrency% THEN
        REM takes player currency
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET currencychange = VAL(temp13$)
        IF findsilenttakecurrency% THEN LET silenttake = 1
        GOSUB takecurrency
        LET temp26 = 1
    END IF
    IF findsfx% THEN
        REM plays sound effect
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET playsfx$ = temp13$
        GOSUB sfxplay
        LET temp26 = 1
    END IF
    IF findeffects% THEN
        REM enables or disables effects
        IF findon% THEN
            LET disablefade = 0
            LET temp26 = 1
        END IF
        IF findoff% THEN
            LET disablefade = 1
            LET temp26 = 1
        END IF
    END IF
    IF findclear% THEN
        REM clears screen
        LET clearscreen = 1
        GOSUB screendraw
        LET temp26 = 1
    END IF
    IF findloading% THEN
        REM displays loading icon
        _PUTIMAGE (1, 1)-(1 + loadiconresx, 1 + loadiconresy), loadicon
        LET temp26 = 1
    END IF
    IF findspeaker% THEN
        REM sets speaker for text banner
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET temp86 = 1
        LET temp23$ = selectobject$
        LET selectobject$ = temp13$
        IF temp13$ = "mainplayer" THEN
            LET objecttype$ = "OBJ"
        ELSE
            LET objecttype$ = "NPC"
        END IF
        LET temp26 = 1
    END IF
    IF findcheckpoint% THEN
        REM sets checkpoints
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET temp140 = VAL(temp13$)
        IF findon% THEN
            REM set checkpoint on
            LET checkpoint(temp140) = 1: LET temp26 = 1
            LET eventtitle$ = "CHECKPOINT CHANGE:"
            LET eventdata$ = "on"
            LET eventnumber = temp140
            GOSUB consoleprinter
        END IF
        IF findoff% THEN
            REM set checkpoint off
            LET checkpoint(temp140) = 0: LET temp26 = 1
            LET eventtitle$ = "CHECKPOINT CHANGE:"
            LET eventdata$ = "off"
            LET eventnumber = temp140
            GOSUB consoleprinter
        END IF
        LET temp140 = 0
    END IF
    IF findifcheckpoint% THEN
        REM checks checkpoints
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET temp140 = VAL(temp13$)
        LET ifcheckpointresult = 0
        IF checkpoint(temp140) = 1 THEN LET ifcheckpointresult = 1: LET temp26 = 1
        IF ifcheckpointresult = 1 THEN
            REM diverts script if requested checkpoint is active
            LET temp26 = 2
            REM enables a spoof trigger to run a script
            LET triggerspoofa = 1
            LET nextmapscript = mapscript
            LET triggerspoofname$ = scriptname$ + "-ifcheckpoint"
            LET temp33 = 2
        END IF
        LET temp140 = 0
    END IF
    IF findifcurrency% THEN
        REM checks pocket for currency amount
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET ifcurrencyamount = VAL(temp13$)
        LET ifcurrencyresult = 0
        GOSUB ifcurrency
        LET temp26 = 1
        IF ifcurrencyresult = 1 THEN
            REM diverts script if player has enough currency
            LET temp26 = 2
            REM enables a spoof trigger to run a script
            LET triggerspoofa = 1
            LET nextmapscript = mapscript
            LET triggerspoofname$ = scriptname$ + "-ifcurrency"
            LET temp33 = 2
        END IF
    END IF
    IF findifdirection% THEN
        REM checks if mainplayer is standing in certain direction
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET temp140 = VAL(temp13$)
        LET temp26 = 1
        IF direction = temp140 THEN
            REM diverts script if direction matches check
            LET temp26 = 2
            LET temp140 = 0
            REM enables a spoof trigger to run a script
            LET triggerspoofa = 1
            LET nextmapscript = mapscript
            LET triggerspoofname$ = scriptname$ + "-ifdirection"
            LET temp33 = 2
        END IF
        LET temp140 = 0
    END IF
    IF findifpocket% THEN
        REM checks pocket for item
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET ifpocket$ = temp13$
        LET ifpocket = 0
        GOSUB ifpocket
        LET temp26 = 1
        IF ifpocket = 1 OR ifpocket = 2 THEN
            REM diverts script if item is in pocket or is gone forever
            LET temp26 = 2
            REM enables a spoof trigger to run a script
            LET triggerspoofa = 1
            LET nextmapscript = mapscript
            LET triggerspoofname$ = scriptname$ + "-ifpocket"
            LET temp33 = 2
        END IF
    END IF
    IF findifholding% THEN
        REM checks players hand for item
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET ifholding$ = temp13$
        LET ifholding = 0
        GOSUB ifholding
        LET temp26 = 1
        IF ifholding = 1 THEN
            REM diverts script if item is in hand
            LET temp26 = 2
            REM enables a spoof trigger to run a script
            LET triggerspoofa = 1
            LET nextmapscript = mapscript
            LET triggerspoofname$ = scriptname$ + "-ifholding"
            LET temp33 = 2
        END IF
    END IF
    IF findifmodel% THEN
        REM checks to see if player is using a certain sprite model
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET ifmodel = 0
        LET ifmodel$ = temp13$
        GOSUB ifmodel
        LET temp26 = 1
        IF ifmodel = 1 THEN
            REM diverts script if player model is correct
            LET temp26 = 2
            REM enables a spoof trigger to run a script
            LET triggerspoofa = 1
            LET nextmapscript = mapscript
            LET triggerspoofname$ = scriptname$ + "-ifmodel"
            LET temp33 = 2
        END IF
    END IF
    IF findifmapno% THEN
        REM checks to see if player is on a certain map
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET ifmapno = VAL(temp13$)
        LET ifmapnoresult = 0
        GOSUB ifmapno
        LET temp26 = 1
        IF ifmapnoresult = 1 THEN
            REM diverts script if map number is correct
            LET temp26 = 2
            REM enables a spoof trigger to run a script
            LET triggerspoofa = 1
            LET nextmapscript = mapscript
            LET triggerspoofname$ = scriptname$ + "-ifmapno"
            LET temp33 = 2
        END IF
    END IF
    IF findifgone% THEN
        REM checks pocket item to see if it has been marked "gone forever"
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET ifgone$ = temp13$
        LET ifgone = 0
        GOSUB ifgone
        LET temp26 = 1
        IF ifgone = 2 THEN
            REM diverts script if item is in hand
            LET temp26 = 2
            REM enables a spoof trigger to run a script
            LET triggerspoofa = 1
            LET nextmapscript = mapscript
            LET triggerspoofname$ = scriptname$ + "-ifgone"
            LET temp33 = 2
        END IF
    END IF
    IF findgive% OR findsilentgive% THEN
        REM gives main player an item
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET giveitem$ = temp13$
        IF findsilentgive% THEN LET silentgive = 1
        GOSUB giveitem
        IF pocketfile$ = giveitem$ THEN LET temp26 = 1
    END IF
    IF findtake% OR findsilenttake% THEN
        REM takes item off main player
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET takeitem$ = temp13$
        IF findsilenttake% THEN LET silenttake = 1
        GOSUB takeitem
        IF pocketfile$ = takeitem$ THEN LET temp26 = 1
    END IF
    IF findmarkgone% THEN
        REM marks pocket item as "gone forever"
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET takeitem$ = temp13$
        GOSUB markgone
        IF pocketfile$ = takeitem$ THEN LET temp26 = 1
    END IF
    IF findslowfade% THEN
        REM slow fade
        IF findin% THEN GOSUB slowfadein: LET temp26 = 1
        IF findout% THEN GOSUB slowfadeout: LET temp26 = 1
    END IF
    IF finddim% THEN
        REM dim
        IF findoff% THEN GOSUB slightfadein: LET temp26 = 1
        IF findon% THEN GOSUB slightfadeout: LET temp26 = 1
    END IF
    IF findfade% THEN
        REM fade
        IF findin% THEN GOSUB fadein: LET temp26 = 1
        IF findout% THEN GOSUB fadeout: LET temp26 = 1
    END IF
    IF findwait% THEN
        REM waits
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET temp27 = VAL(temp13$)
        LET temp26 = ctime + temp27
        DO
            GOSUB gameloop
        LOOP UNTIL ctime >= temp26
        LET temp26 = 1
    END IF
    IF findhalt% THEN
        REM halts engine
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET temp27 = VAL(temp13$)
        _DELAY temp27
        LET temp26 = 1
    END IF
    IF findmap% THEN
        REM map change
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = RIGHT$(scriptline$, LEN(scriptline$) - LEN(temp12$))
        LET temp13$ = LTRIM$(temp13$)
        LET temp27 = VAL(temp13$)
        LET oldmapno = mapno
        LET oldmapname$ = mapname$
        LET mapno = temp27
        GOSUB mapload
        LET temp26 = 1
    END IF
    IF findwarp% THEN
        REM warps players and objects
        IF findmainplayer% THEN
            REM main player
            LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
            LET temp135 = 2
            LET temp13$ = RIGHT$(scriptline$, INSTR(scriptline$, " ") - temp135)
            GOSUB coordinatefixer
            LET temp27 = VAL(temp13$)
            IF findminus% AND temp27 >= 100 THEN LET temp27 = (temp27 - temp27) - temp27
            IF findx% THEN LET posx = ((resx / 2) - temp27): LET temp26 = 1
            IF findy% THEN LET posy = ((resy / 2) - temp27): LET temp26 = 1
        END IF
        IF findobject% THEN
            LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
            LET temp135 = 2
            LET temp13$ = RIGHT$(scriptline$, INSTR(scriptline$, " ") - temp135)
            GOSUB coordinatefixer
            LET temp27 = VAL(temp13$)
            IF findminus% AND temp27 >= 100 THEN LET temp27 = (temp27 - temp27) - temp27
            LET x = 0
            DO
                LET x = x + 1
                IF findobject(x) THEN
                    IF findx% THEN LET objectx(x) = temp27: LET temp26 = 1
                    IF findy% THEN LET objecty(x) = temp27: LET temp26 = 1
                END IF
            LOOP UNTIL x >= mapobjectno
            LET x = 0
        END IF
        IF findplayer% THEN
            LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
            LET temp135 = 2
            LET temp13$ = RIGHT$(scriptline$, INSTR(scriptline$, " ") - temp135)
            GOSUB coordinatefixer
            LET temp27 = VAL(temp13$)
            IF findminus% AND temp27 >= 100 THEN LET temp27 = (temp27 - temp27) - temp27
            LET x = 0
            DO
                LET x = x + 1
                IF findplayer(x) THEN
                    IF findx% THEN LET playerx(x) = temp27: LET temp26 = 1
                    IF findy% THEN LET playery(x) = temp27: LET temp26 = 1
                END IF
            LOOP UNTIL x >= mapplayerno
            LET x = 0
        END IF
    END IF
    IF findmusic% THEN
        REM changes music
        IF findcontrol% THEN
            REM plays or stops music
            IF findplay% THEN
                IF musicpause = 0 THEN
                    LET playmusic$ = oldmusic$: GOSUB musicplay: LET temp26 = 1
                ELSE
                    GOSUB musicplay: LET temp26 = 1
                END IF
            END IF
            IF findstop% THEN GOSUB musicstop: LET temp26 = 1
            IF findpause% THEN GOSUB musicpause: LET temp26 = 1
        END IF
        IF findfile% THEN
            REM changes current music files and plays (and stops previous music)
            LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
            LET temp13$ = MID$(scriptline$, INSTR(scriptline$, " ") + 6)
            LET playmusic$ = temp13$
            GOSUB musicplay
            LET temp26 = 1
        END IF
    END IF
    IF findmodel% THEN
        REM changes player model
        IF findmainplayer% THEN
            REM mainplayer
            LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
            LET temp13$ = MID$(scriptline$, INSTR(scriptline$, " ") + 12)
            LET temp26 = 1
            LET oldmplayermodel$ = mplayermodel$
            LET mplayermodel$ = temp13$
            GOSUB mainplayerload
        END IF
    END IF
    IF findshowimage% THEN
        REM displays image
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = MID$(scriptline$, INSTR(scriptline$, " "))
        LET temp13$ = LTRIM$(temp13$)
        IF _FILEEXISTS(uiloc$ + temp13$ + ".png") THEN
            LET showimage$ = temp13$
            GOSUB showimage
            LET temp26 = 1
        END IF
    END IF
    IF findcollision% THEN
        REM changes noclip value
        IF findon% THEN LET noclip = 0: LET temp26 = 1: REM switches noclip off
        IF findoff% THEN LET noclip = 1: LET temp26 = 1: REM switches noclip on
    END IF
    IF findscript% THEN
        REM runs script
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = MID$(scriptline$, INSTR(scriptline$, " "))
        LET temp13$ = LTRIM$(temp13$)
        LET temp26 = 2
        LET nextmapscript = mapscript
        REM enables a spoof trigger to run a script
        LET triggerspoofa = 1
        LET triggerspoofname$ = temp13$
        LET temp33 = 1
        LET nextmapscript = mapscript
    END IF
    IF findterminal% THEN
        REM launches terminal file
        LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
        LET temp13$ = MID$(scriptline$, INSTR(scriptline$, " "))
        LET temp13$ = LTRIM$(temp13$)
        LET runterminal$ = temp13$
        LET runterminal = 1
        LET temp26 = 2
    END IF
    IF findanimate% THEN
        IF findmainplayer% THEN
            REM animates mainplayer
            LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
            LET temp13$ = MID$(scriptline$, INSTR(scriptline$, "r") + 2)
            LET anisprite$ = "mainplayer"
            LET anifile$ = temp13$
            GOSUB animation
            LET temp26 = 1
            GOTO scriptsay
        END IF
        IF findplayer% THEN
            LET x = 0
            DO
                LET x = x + 1
                IF findplayer(x) THEN
                    REM animates NPC
                    LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
                    LET temp13$ = MID$(scriptline$, INSTR(scriptline$, playername(x)) + LEN(playername(x)) + 1)
                    LET anisprite$ = playername(x)
                    LET anifile$ = temp13$
                    GOSUB animation
                    LET temp26 = 1
                    GOTO scriptsay
                END IF
            LOOP UNTIL x >= mapplayerno
            LET x = 0
        END IF
        IF findobject% THEN
            LET x = 0
            DO
                LET x = x + 1
                IF findobject(x) THEN
                    REM animates object
                    LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
                    LET temp13$ = MID$(scriptline$, INSTR(scriptline$, objectname(x)) + LEN(objectname(x)) + 1)
                    LET anisprite$ = objectname(x)
                    LET anifile$ = temp13$
                    GOSUB animation
                    LET temp26 = 1
                    GOTO scriptsay
                END IF
            LOOP UNTIL x >= mapobjectno
            LET x = 0
        END IF
    END IF
    IF findmove% OR findsprint% THEN
        REM moves a player or object
        IF findmove% THEN
            REM sets move speed
            LET temp131 = (pace / playerwalkdivide)
            LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
            IF findback% THEN
                LET temp135 = 6
            ELSE
                LET temp135 = 2
            END IF
            LET temp13$ = RIGHT$(scriptline$, INSTR(scriptline$, " ") - temp135)
            GOSUB coordinatefixer
            LET temp27 = VAL(temp13$)
            IF findminus% AND temp27 >= 100 THEN LET temp27 = (temp27 - temp27) - temp27
        END IF
        IF findsprint% THEN
            REM sets sprint speed
            LET temp131 = (pace)
            LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
            IF findback% THEN
                LET temp135 = 8
            ELSE
                LET temp135 = 4
            END IF
            LET temp13$ = RIGHT$(scriptline$, INSTR(scriptline$, " ") - temp135)
            GOSUB coordinatefixer
            LET temp27 = VAL(temp13$)
            IF findminus% AND temp27 >= 100 THEN LET temp27 = (temp27 - temp27) - temp27
        END IF
        IF findobject% THEN
            REM object
            IF findx% THEN
                REM X
                LET x = 0
                DO
                    LET x = x + 1
                    IF findobject(x) THEN
                        DO
                            IF objectx(x) > temp27 THEN
                                LET objectx(x) = objectx(x) - (temp131)
                                IF objectx(x) <= temp27 THEN LET temp56 = 1
                            END IF
                            IF objectx(x) < temp27 THEN
                                LET objectx(x) = objectx(x) + (temp131)
                                IF objectx(x) >= temp27 THEN LET temp56 = 1
                            END IF
                            GOSUB gameloop
                        LOOP UNTIL temp56 = 1
                        LET temp26 = 1
                    END IF
                LOOP UNTIL x >= mapobjectno
                LET x = 0
            END IF
            IF findy% THEN
                REM Y
                LET x = 0
                DO
                    LET x = x + 1
                    IF findobject(x) THEN
                        DO
                            IF objecty(x) > temp27 THEN
                                LET objecty(x) = objecty(x) - (temp131)
                                IF objecty(x) <= temp27 THEN LET temp56 = 1
                            END IF
                            IF objecty(x) < temp27 THEN
                                LET objecty(x) = objecty(x) + (temp131)
                                IF objecty(x) >= temp27 THEN LET temp56 = 1
                            END IF
                            GOSUB gameloop
                        LOOP UNTIL temp56 = 1
                        LET temp26 = 1
                    END IF
                LOOP UNTIL x >= mapobjectno
                LET x = 0
            END IF
        END IF
        IF findplayer% THEN
            REM NPC
            IF findx% THEN
                REM X
                LET x = 0
                DO
                    LET x = x + 1
                    IF findplayer(x) THEN
                        LET playerwalking(x) = 1
                        DO
                            IF playerx(x) > temp27 THEN
                                LET playerx(x) = playerx(x) - (temp131)
                                IF findback% THEN
                                    LET playerd(x) = 3
                                ELSE
                                    LET playerd(x) = 4
                                END IF
                                IF playerx(x) <= temp27 THEN LET temp56 = 1
                            END IF
                            IF playerx(x) < temp27 THEN
                                LET playerx(x) = playerx(x) + (temp131)
                                IF findback% THEN
                                    LET playerd(x) = 4
                                ELSE
                                    LET playerd(x) = 3
                                END IF
                                IF playerx(x) >= temp27 THEN LET temp56 = 1
                            END IF
                            GOSUB gameloop
                        LOOP UNTIL temp56 = 1
                        LET playerwalking(x) = 0
                        LET temp26 = 1
                    END IF
                LOOP UNTIL x >= mapplayerno
                LET x = 0
            END IF
            IF findy% THEN
                REM Y
                LET x = 0
                DO
                    LET x = x + 1
                    IF findplayer(x) THEN
                        LET playerwalking(x) = 1
                        DO
                            IF playery(x) > temp27 THEN
                                LET playery(x) = playery(x) - (temp131)
                                IF findback% THEN
                                    LET playerd(x) = 2
                                ELSE
                                    LET playerd(x) = 1
                                END IF
                                IF playery(x) <= temp27 THEN LET temp56 = 1
                            END IF
                            IF playery(x) < temp27 THEN
                                LET playery(x) = playery(x) + (temp131)
                                IF findback% THEN
                                    LET playerd(x) = 1
                                ELSE
                                    LET playerd(x) = 2
                                END IF
                                IF playery(x) >= temp27 THEN LET temp56 = 1
                            END IF
                            GOSUB gameloop
                        LOOP UNTIL temp56 = 1
                        LET playerwalking(x) = 0
                        LET temp26 = 1
                    END IF
                LOOP UNTIL x >= mapplayerno
                LET x = 0
            END IF
        END IF
        IF findmainplayer% THEN
            REM mainplayer
            REM sets movement value
            IF findmove% THEN
                LET temp131 = (pace / scriptwalkdivide)
                LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
                IF findback% THEN
                    LET temp135 = 6
                ELSE
                    LET temp135 = 2
                END IF
                LET temp13$ = RIGHT$(scriptline$, INSTR(scriptline$, " ") - temp135)
                GOSUB coordinatefixer
                LET temp27 = VAL(temp13$)
                IF findminus% AND temp27 >= 100 THEN LET temp27 = (temp27 - temp27) - temp27
            END IF
            IF findsprint% THEN
                LET temp131 = (pace)
                LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
                IF findback% THEN
                    LET temp135 = 8
                ELSE
                    LET temp135 = 4
                END IF
                LET temp13$ = RIGHT$(scriptline$, INSTR(scriptline$, " ") - temp135)
                GOSUB coordinatefixer
                LET temp27 = VAL(temp13$)
                IF findminus% AND temp27 >= 100 THEN LET temp27 = (temp27 - temp27) - temp27
            END IF
            IF findx% THEN
                IF ((resx / 2) - posx) > temp27 THEN
                    IF findback% THEN
                        LET direction = 3
                    ELSE
                        LET direction = 4
                    END IF
                    LET temp26 = 1
                    LET mpwalking = 1
                    DO
                        LET posx = posx + (temp131)
                        GOSUB gameloop
                    LOOP UNTIL ((resx / 2) - posx) <= temp27
                    LET mpwalking = 0
                END IF
                IF ((resx / 2) - posx) < temp27 THEN
                    IF findback% THEN
                        LET direction = 4
                    ELSE
                        LET direction = 3
                    END IF
                    LET temp26 = 1
                    LET mpwalking = 1
                    DO
                        LET posx = posx - (temp131)
                        GOSUB gameloop
                    LOOP UNTIL ((resx / 2) - posx) >= temp27
                    LET mpwalking = 0
                END IF
            END IF
            IF findy% THEN
                IF ((resy / 2) - posy) > temp27 THEN
                    IF findback% THEN
                        LET direction = 2
                    ELSE
                        LET direction = 1
                    END IF
                    LET temp26 = 1
                    LET mpwalking = 1
                    DO
                        LET posy = posy + (temp131)
                        GOSUB gameloop
                    LOOP UNTIL ((resy / 2) - posy) <= temp27
                    LET mpwalking = 0
                END IF
                IF ((resy / 2) - posy) < temp27 THEN
                    IF findback% THEN
                        LET direction = 1
                    ELSE
                        LET direction = 2
                    END IF
                    LET temp26 = 1
                    LET mpwalking = 1
                    DO
                        LET posy = posy - (temp131)
                        GOSUB gameloop
                    LOOP UNTIL ((resy / 2) - posy) >= temp27
                    LET mpwalking = 0
                END IF
            END IF
        END IF
    END IF
    IF finddirection% THEN
        REM changes direction of player
        IF findmainplayer% THEN
            REM main player
            LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
            LET temp13$ = RIGHT$(scriptline$, INSTR(scriptline$, " ") - 9)
            LET temp27 = VAL(temp13$)
            LET direction = temp27
            LET temp26 = 1
        END IF
        IF findplayer% THEN
            LET temp12$ = LEFT$(scriptline$, INSTR(scriptline$, " ") - 1)
            LET temp13$ = RIGHT$(scriptline$, INSTR(scriptline$, " ") - 9)
            IF findfaceplayer% THEN
                IF direction = 1 THEN LET temp27 = 2
                IF direction = 2 THEN LET temp27 = 1
                IF direction = 3 THEN LET temp27 = 4
                IF direction = 4 THEN LET temp27 = 3
            ELSE
                LET temp27 = VAL(temp13$)
            END IF
            LET x = 0
            DO
                LET x = x + 1
                IF findplayer(x) THEN LET playerd(x) = temp27: LET temp26 = 1
            LOOP UNTIL x >= mapplayerno
            LET x = 0
        END IF
    END IF
    scriptsay:
    REM prints to console upon sucessful script line execution or prints invalid/blank line
    IF temp26 = 1 OR temp26 = 2 THEN
        LET eventtitle$ = "SCRIPT LINE:"
        LET eventdata$ = scriptline$
        LET eventnumber = scriptline
    ELSE
        IF scriptline$ <> "" THEN
            LET eventtitle$ = "INVALID SCRIPT LINE:"
            LET eventdata$ = scriptline$
            LET eventnumber = scriptline
        ELSE
            LET eventtitle$ = "BLANK SCRIPT LINE:"
            LET eventdata$ = "LINE:"
            LET eventnumber = scriptline
        END IF
    END IF
    GOSUB consoleprinter
    REM scrubs search terms and temp values
    IF temp26 = 1 THEN LET temp26 = 0
    LET temp27 = 0: LET temp56 = 0: LET temp12$ = "": LET temp13$ = "": LET temp131 = 0: LET findfade% = 0: LET findin% = 0: LET findout% = 0: LET findwait% = 0: LET findmap% = 0: LET findwarp% = 0: LET findx% = 0: LET findy% = 0: LET findmainplayer% = 0: LET finddirection% = 0: LET findmove% = 0: LET findmodel% = 0: LET findon% = 0: LET findoff% = 0: LET findcollision% = 0: LET findscript% = 0: LET findmusic% = 0: LET findcontrol% = 0: LET findplay% = 0: LET findstop% = 0: LET findfile% = 0: LET findpause% = 0: LET findsfx% = 0: LET findhalt% = 0: LET findplayer% = 0: LET findpilot% = 0: LET finddim% = 0: LET findgive% = 0: LET findtake% = 0: LET findsay% = 0: LET findspeaker% = 0: LET findclear% = 0: LET findeffects% = 0: LET findifpocket% = 0: LET findterminal% = 0: LET findgivecurrency% = 0: LET findtakecurrency% = 0: LET findifholding% = 0: LET findifcurrency% = 0: LET findmarkgone% = 0: LET findloading% = 0: LET findmapeffect% = 0: LET finddark% = 0: LET findrain% = 0: LET findstorm% = 0: LET findtorch% = 0: LET findanimate% = 0: LET findsavegame% = 0: LET findifgone% = 0: LET findsunsetup% = 0: LET findsunsetdown% = 0: LET findsunsetleft% = 0: LET findsunsetright% = 0: LET findsprint% = 0: LET findshowimage% = 0: LET findslowfade% = 0: LET findsilenttake% = 0: LET findsilentgive% = 0: LET findsilentgivecurrency% = 0: LET findsilenttakecurrency% = 0: LET findifmapno% = 0: LET findifmodel% = 0: LET findfaceplayer% = 0: LET findback% = 0: LET findrun% = 0: LET findminus% = 0: LET findifdirection% = 0: LET findcarryvalues% = 0: LET findpitchblack% = 0: LET findloadgame% = 0: LET findobject% = 0: LET findcheckpoint% = 0: LET findifcheckpoint% = 0: LET findpockets% = 0: LET findup% = 0: LET finddown% = 0: LET findleft% = 0: LET findright% = 0: LET findselect% = 0
    LET x = 0
    DO
        LET x = x + 1
        LET findplayer(x) = 0
    LOOP UNTIL x >= totalplayers
    LET x = 0
    DO
        LET x = x + 1
        LET findobject(x) = 0
    LOOP UNTIL x >= totalobjects
    LET x = 0
LOOP UNTIL EOF(3) OR temp26 = 2
CLOSE #3
REM prints to console script has ended
LET eventtitle$ = "SCRIPT ENDED:"
LET eventdata$ = scriptname$
LET eventnumber = 0
GOSUB consoleprinter
REM returns value of selectobject$ if needed
IF temp86 = 1 THEN
    LET selectobject$ = temp23$
    LET temp86 = 0
    LET temp23$ = ""
END IF
LET clearscreen = 1: REM calls for a screen clear
LET temp26 = 0: LET temp64 = 0: LET scriptrun = 0: LET mapscript = 0: LET scriptline = 0: LET scriptline$ = "": REM scrub temp values
REM makes sure triggers are cleared
LET x = 0
DO
    LET x = x + 1
    LET triggera(x) = 0
LOOP UNTIL x >= maptriggerno
LET x = 0
RETURN

triggerchecker:
REM checks to see if triggers have been activated and launches required scripts
REM spoof trigger
IF triggerspoofa = 1 THEN
    LET scriptname$ = triggerspoofname$
    LET mapscript = nextmapscript
    LET nextmapscript = 0
    IF scriptname$ <> oldscript$ THEN
        REM prints to consolelog.txt
        LET eventtitle$ = "SPOOF TRIGGER ACTIVE:"
        LET eventdata$ = triggerspoofname$
        LET eventnumber = 0
        GOSUB consoleprinter
        IF oldscript$ = scriptname$ THEN LET triggerspoofa = 0: LET scriptrun = 0: RETURN
        GOSUB script
    ELSE
        LET triggerspoofa = 0
    END IF
    IF temp33 = 1 OR temp33 = 2 THEN
        IF temp33 = 1 THEN LET triggerspoofa = 0
    ELSE
        LET triggerspoofa = 1
    END IF
    LET temp33 = 0: REM scrub temp values
END IF
IF maptriggerno = 0 THEN RETURN: REM return if map has no triggers attached
LET x = 0
DO
    LET x = x + 1
    IF triggera(x) = 1 THEN
        IF _FILEEXISTS(scriptloc$ + mapdir$ + triggername(x) + ".vsf") THEN
            LET scriptname$ = triggername(x)
            LET mapscript = 1
            GOSUB script
        ELSE
            ERROR 423
        END IF
        LET triggera(x) = 0
    END IF
LOOP UNTIL x >= maptriggerno
LET x = 0
RETURN

footchanger:
REM changes players foot
REM builds values to keep players feet changing
LET mpfootloop = mpfootloop + 1
DO
    LET temp49 = temp49 + 1
    LET pfootloop(temp49) = pfootloop(temp49) + 1
LOOP UNTIL temp49 >= mapplayerno OR temp49 >= totalplayers
REM changes players foot when walking mainplayer
IF mpfootloop >= footpace THEN
    IF temp2 = 0 THEN IF mpfoot = 1 THEN LET mpfoot = 2: LET mpfootloop = 0: LET temp2 = 1
    IF temp2 = 0 THEN IF mpfoot = 2 THEN LET mpfoot = 1: LET mpfootloop = 0: LET temp2 = 1
END IF
REM changes NPCs feet
LET temp49 = 0
DO
    LET temp49 = temp49 + 1
    IF pfootloop(temp49) >= footpace THEN
        IF temp50 = 0 THEN IF pfoot(temp49) = 1 THEN LET pfoot(temp49) = 2: LET pfootloop(temp49) = 0: LET temp50 = 1
        IF temp50 = 0 THEN IF pfoot(temp49) = 2 THEN LET pfoot(temp49) = 1: LET pfootloop(temp49) = 0: LET temp50 = 1
    END IF
    LET temp50 = 0
LOOP UNTIL temp49 >= mapplayerno OR temp49 >= totalplayers
REM flushes temporary values
LET temp2 = 0: LET temp49 = 0: LET temp50 = 0
RETURN

worlddraw:
REM draws world
IF INT(ctime + mapanioffset) MOD 2 THEN
    _PUTIMAGE (maploc1x, maploc1y)-(maploc2x, maploc2y), mapa
ELSE
    _PUTIMAGE (maploc1x, maploc1y)-(maploc2x, maploc2y), mapb
END IF
RETURN

mainplayerdraw:
REM draws main player
REM draws main player standing
IF anisprite$ = "mainplayer" THEN RETURN: REM return for if mainplayer animation is playing.
IF mpwalking = 0 THEN
    IF mpidle <= INT(ctime) THEN
        IF direction = 1 THEN
            IF INT(ctime) MOD 2 THEN
                _PUTIMAGE (mpposx, mpposy), mpbi1: REM draws main player standing BACK using IDLE
            ELSE
                _PUTIMAGE (mpposx, mpposy), mpbi2: REM draws main player standing BACK using IDLE
            END IF
        END IF
        IF direction = 2 THEN
            IF INT(ctime) MOD 2 THEN
                _PUTIMAGE (mpposx, mpposy), mpfi1: REM draws main player standing FRONT using IDLE
            ELSE
                _PUTIMAGE (mpposx, mpposy), mpfi2: REM draws main player standing FRONT using IDLE
            END IF
        END IF
        IF direction = 3 THEN
            IF INT(ctime) MOD 2 THEN
                _PUTIMAGE (mpposx, mpposy), mpri1: REM draws main player standing RIGHT using IDLE
            ELSE
                _PUTIMAGE (mpposx, mpposy), mpri2: REM draws main player standing RIGHT using IDLE
            END IF
        END IF
        IF direction = 4 THEN
            IF INT(ctime) MOD 2 THEN
                _PUTIMAGE (mpposx, mpposy), mpli1: REM draws main player standing LEFT using IDLE
            ELSE
                _PUTIMAGE (mpposx, mpposy), mpli2: REM draws main player standing LEFT using IDLE
            END IF
        END IF
    ELSE
        IF direction = 1 THEN _PUTIMAGE (mpposx, mpposy), mpb: REM draws main player standing BACK
        IF direction = 2 THEN _PUTIMAGE (mpposx, mpposy), mpf: REM draws main player standing FRONT
        IF direction = 3 THEN _PUTIMAGE (mpposx, mpposy), mpr: REM draws main player standing RIGHT
        IF direction = 4 THEN _PUTIMAGE (mpposx, mpposy), mpl: REM draws main player standing LEFT
    END IF
END IF
REM draws main player walking
IF mpwalking = 1 THEN
    REM draws main player walking BACK
    IF direction = 1 THEN
        IF mpfoot = 1 THEN _PUTIMAGE (mpposx, mpposy), mpbr: REM draws main player walking BACK (right foot)
        IF mpfoot = 2 THEN _PUTIMAGE (mpposx, mpposy), mpbl: REM draws main player walking BACK (left foot)
    END IF
    REM draws main player walking FRONT
    IF direction = 2 THEN
        IF mpfoot = 1 THEN _PUTIMAGE (mpposx, mpposy), mpfr: REM draws main player walking FRONT (right foot)
        IF mpfoot = 2 THEN _PUTIMAGE (mpposx, mpposy), mpfl: REM draws main player walking FRONT (left foot)
    END IF
    REM draws main player walking RIGHT
    IF direction = 3 THEN
        IF mpfoot = 1 THEN _PUTIMAGE (mpposx, mpposy), mprr: REM draws main player walking RIGHT (right foot)
        IF mpfoot = 2 THEN _PUTIMAGE (mpposx, mpposy), mprl: REM draws main player walking RIGHT (left foot)
    END IF
    REM draws main player walking LEFT
    IF direction = 4 THEN
        IF mpfoot = 1 THEN _PUTIMAGE (mpposx, mpposy), mplr: REM draws main player walking LEFT (right foot)
        IF mpfoot = 2 THEN _PUTIMAGE (mpposx, mpposy), mpll: REM draws main player walking LEFT (left foot)
    END IF
END IF
RETURN

playermove:
REM calculates NPC movement
IF mapplayerno = 0 THEN RETURN: REM return for no npcs attached to map
DO
    LET temp48 = temp48 + 1
    IF mplayerx(temp48) <> 0 AND mplayery(temp48) <> 0 THEN GOSUB playermover
LOOP UNTIL temp48 >= mapplayerno OR temp48 >= totalplayers
LET temp48 = 0: REM scrubs temp values
RETURN

playermover:
REM moves NPC
REM Walk away
IF playerscript(temp48) = 1 THEN RETURN
IF playerjourney(temp48) = 1 THEN
    IF INT(ctime) >= playerperiod(temp48) THEN
        LET playerwalking(temp48) = 1
        IF mplayerx(temp48) > playerx(temp48) THEN
            LET playerd(temp48) = 3
            LET playerx(temp48) = playerx(temp48) + (pace / playerwalkdivide)
            RETURN
        END IF
        IF mplayerx(temp48) < playerx(temp48) THEN
            LET playerd(temp48) = 4
            LET playerx(temp48) = playerx(temp48) - (pace / playerwalkdivide)
            RETURN
        END IF
        IF mplayery(temp48) > playery(temp48) THEN
            LET playerd(temp48) = 2
            LET playery(temp48) = playery(temp48) + (pace / playerwalkdivide)
            RETURN
        END IF
        IF mplayery(temp48) < playery(temp48) THEN
            LET playerd(temp48) = 1
            LET playery(temp48) = playery(temp48) - (pace / playerwalkdivide)
            RETURN
        END IF
        IF playerx(temp48) = mplayerx(temp48) AND playery(temp48) = mplayery(temp48) THEN
            LET playerjourney(temp48) = 2
            LET playerperiod(temp48) = playergrace(temp48) + INT(ctime)
            LET playerd(temp48) = playerdefault(temp48)
            LET playerwalking(temp48) = 0
        END IF
    END IF
END IF
REM walk back
IF playerjourney(temp48) = 2 THEN
    IF INT(ctime) >= playerperiod(temp48) THEN
        LET playerwalking(temp48) = 1
        IF dplayerx(temp48) > playerx(temp48) THEN
            LET playerd(temp48) = 3
            LET playerx(temp48) = playerx(temp48) + (pace / playerwalkdivide)
            RETURN
        END IF
        IF dplayerx(temp48) < playerx(temp48) THEN
            LET playerd(temp48) = 4
            LET playerx(temp48) = playerx(temp48) - (pace / playerwalkdivide)
            RETURN
        END IF
        IF dplayery(temp48) > playery(temp48) THEN
            LET playerd(temp48) = 2
            LET playery(temp48) = playery(temp48) + (pace / playerwalkdivide)
            RETURN
        END IF
        IF dplayery(temp48) < playery(temp48) THEN
            LET playerd(temp48) = 1
            LET playery(temp48) = playery(temp48) - (pace / playerwalkdivide)
            RETURN
        END IF
        IF playerx(temp48) = dplayerx(temp48) AND playery(temp48) = dplayery(temp48) THEN
            LET playerjourney(temp48) = 1
            LET playerperiod(temp48) = playergrace(temp48) + INT(ctime)
            LET playerd(temp48) = playerdefault(temp48)
            LET playerwalking(temp48) = 0
        END IF
    END IF
END IF
RETURN

playerdraw:
REM draws npcs
DO
    LET temp42 = temp42 + 1
    IF anisprite$ <> playername(temp42) AND objectl = playerlayer(temp42) THEN
        IF playerwalking(temp42) = 0 THEN
            IF playerd(temp42) = 2 THEN
                IF INT(ctime + playeroffset(temp42)) MOD 2 THEN
                    _PUTIMAGE (playerx(temp42) + posx, playery(temp42) + posy), playerfi1(temp42)
                ELSE
                    _PUTIMAGE (playerx(temp42) + posx, playery(temp42) + posy), playerfi2(temp42)
                END IF
            END IF
            IF playerd(temp42) = 1 THEN
                IF INT(ctime + playeroffset(temp42)) MOD 2 THEN
                    _PUTIMAGE (playerx(temp42) + posx, playery(temp42) + posy), playerbi1(temp42)
                ELSE
                    _PUTIMAGE (playerx(temp42) + posx, playery(temp42) + posy), playerbi2(temp42)
                END IF
            END IF
            IF playerd(temp42) = 3 THEN
                IF INT(ctime + playeroffset(temp42)) MOD 2 THEN
                    _PUTIMAGE (playerx(temp42) + posx, playery(temp42) + posy), playerri1(temp42)
                ELSE
                    _PUTIMAGE (playerx(temp42) + posx, playery(temp42) + posy), playerri2(temp42)
                END IF
            END IF
            IF playerd(temp42) = 4 THEN
                IF INT(ctime + playeroffset(temp42)) MOD 2 THEN
                    _PUTIMAGE (playerx(temp42) + posx, playery(temp42) + posy), playerli1(temp42)
                ELSE
                    _PUTIMAGE (playerx(temp42) + posx, playery(temp42) + posy), playerli2(temp42)
                END IF
            END IF
        END IF
        IF playerwalking(temp42) = 1 THEN
            IF playerd(temp42) = 2 THEN
                IF pfoot(temp42) = 1 THEN _PUTIMAGE (playerx(temp42) + posx, playery(temp42) + posy), playerfl(temp42)
                IF pfoot(temp42) = 2 THEN _PUTIMAGE (playerx(temp42) + posx, playery(temp42) + posy), playerfr(temp42)
            END IF
            IF playerd(temp42) = 1 THEN
                IF pfoot(temp42) = 1 THEN _PUTIMAGE (playerx(temp42) + posx, playery(temp42) + posy), playerbl(temp42)
                IF pfoot(temp42) = 2 THEN _PUTIMAGE (playerx(temp42) + posx, playery(temp42) + posy), playerbr(temp42)
            END IF
            IF playerd(temp42) = 3 THEN
                IF pfoot(temp42) = 1 THEN _PUTIMAGE (playerx(temp42) + posx, playery(temp42) + posy), playerrl(temp42)
                IF pfoot(temp42) = 2 THEN _PUTIMAGE (playerx(temp42) + posx, playery(temp42) + posy), playerrr(temp42)
            END IF
            IF playerd(temp42) = 4 THEN
                IF pfoot(temp42) = 1 THEN _PUTIMAGE (playerx(temp42) + posx, playery(temp42) + posy), playerll(temp42)
                IF pfoot(temp42) = 2 THEN _PUTIMAGE (playerx(temp42) + posx, playery(temp42) + posy), playerlr(temp42)
            END IF
        END IF
    END IF
LOOP UNTIL temp42 >= mapplayerno
LET temp42 = 0: REM scrub temp values
RETURN

objectdraw:
REM draws map objects to screen
DO
    LET temp13 = temp13 + 1
    IF anisprite$ <> objectname(temp13) THEN
        IF objectl = objectl(temp13) AND objectname(temp13) <> "[COLLISIONONLY]" THEN
            IF INT(ctime + objectoffset(temp13)) MOD 2 THEN
                _PUTIMAGE (objectx(temp13) + posx, objecty(temp13) + posy), objecta(temp13)
            ELSE
                _PUTIMAGE (objectx(temp13) + posx, objecty(temp13) + posy), objectb(temp13)
            END IF
        END IF
    END IF
LOOP UNTIL temp13 >= mapobjectno OR temp13 >= totalobjects
LET temp13 = 0: REM scrub temp values
RETURN

layercalc:
REM calculates object and player layers
REM OBJECT LAYER CALC
DO
    LET temp999 = temp999 + 1
    IF ((resy / 2) - (objectresy(temp999) / 2) + (mpy / 2) - objects(temp999)) - posy > objecty(temp999) THEN
        LET objectl(temp999) = 1
    ELSE
        LET objectl(temp999) = 2
    END IF
LOOP UNTIL temp999 >= mapobjectno OR temp999 >= totalobjects
LET temp999 = 0
REM NPC LAYER CALC
DO
    LET temp999 = temp999 + 1
    IF ((resy / 2) - (playerresy(temp999) / 2) + (mpy / 2) - players(temp999)) - posy > playery(temp999) THEN
        LET playerlayer(temp999) = 1
    ELSE
        LET playerlayer(temp999) = 2
    END IF
LOOP UNTIL temp999 >= mapplayerno OR temp999 >= totalplayers
LET temp999 = 0: REM scrub temp values
RETURN

screendraw:
REM draws game on screen
REM clears screen if needed
IF clearscreen = 1 THEN
    CLS
    LET clearscreen = 0
    REM tells console
    LET eventtitle$ = "SCREEN CLEARED!"
    LET eventdata$ = "FRAMES:"
    LET eventnumber = frames
    GOSUB consoleprinter
END IF
REM calculates map location
LET posx = INT(posx): REM remove decimals
LET posy = INT(posy): REM remove decimals
LET maploc1x = 0 + posx
LET maploc1y = 0 + posy
LET maploc2x = mapx + posx
LET maploc2y = mapy + posy
REM calulates object draw order
LET objectl = 1: REM resets layer counter
GOSUB layercalc: REM calculates layers
GOSUB worlddraw: REM draws world
IF mapobjectno > 0 THEN GOSUB objectdraw: REM draws map objects (first layer)
IF mapplayerno > 0 THEN GOSUB playerdraw: REM draws NPCs (first layer)
GOSUB mainplayerdraw: REM draws mainplayer
LET objectl = objectl + 1: REM increases layer counter
IF mapobjectno > 0 THEN GOSUB objectdraw: REM draws map objects (second layer)
IF mapplayerno > 0 THEN GOSUB playerdraw: REM draws NPCs (second layer)
IF hud <> 0 THEN GOSUB hud: REM calls for developer hud to be drawn if needed
IF mapeffect > 0 THEN GOSUB effectdraw: REM draws special map effects
REM draws cutscene running image
IF scriptrun = 1 THEN _PUTIMAGE (0, 0)-(scriptimageresx, scriptimageresy), scriptimage
REM displays speedrun time
IF speedrun > 0 THEN GOSUB displayspeedrun
IF effectani = 0 THEN _DISPLAY
RETURN

inputter:
REM input sub
REM detects diagonal movement
IF scriptrun = 1 THEN IF devmode = 1 THEN IF a$ = "\" OR a$ = "/" THEN GOSUB prompt: REM developer prompt if developer mode is on
IF scriptrun = 0 THEN
    REM controls for when script isn't running.
    IF _KEYDOWN(18432) AND _KEYDOWN(19712) THEN LET diagonalmove = 1
    IF _KEYDOWN(18432) AND _KEYDOWN(19200) THEN LET diagonalmove = 1
    IF _KEYDOWN(20480) AND _KEYDOWN(19712) THEN LET diagonalmove = 1
    IF _KEYDOWN(20480) AND _KEYDOWN(19200) THEN LET diagonalmove = 1
    IF _KEYDOWN(18432) AND _KEYDOWN(20480) THEN LET diagonalmove = 1
    IF _KEYDOWN(19712) AND _KEYDOWN(19200) THEN LET diagonalmove = 1
    REM skips controls when diagonal movement detected
    IF diagonalmove = 0 THEN
        REM keyboard up
        IF _KEYDOWN(18432) AND ucontrol = 1 THEN
            LET oposy = posy
            LET posy = posy + pace
            LET direction = 1
            LET selectobject$ = ""
            LET objecttype$ = ""
            LET mpwalking = 1
        END IF
        REM keyboard down
        IF _KEYDOWN(20480) AND dcontrol = 1 THEN
            LET oposy = posy
            LET posy = posy - pace
            LET direction = 2
            LET selectobject$ = ""
            LET objecttype$ = ""
            LET mpwalking = 1
        END IF
        REM keyboard right
        IF _KEYDOWN(19712) AND rcontrol = 1 THEN
            LET oposx = posx
            LET posx = posx - pace
            LET direction = 3
            LET selectobject$ = ""
            LET objecttype$ = ""
            LET mpwalking = 1
        END IF
        REM keyboard left
        IF _KEYDOWN(19200) AND lcontrol = 1 THEN
            LET oposx = posx
            LET posx = posx + pace
            LET direction = 4
            LET selectobject$ = ""
            LET objecttype$ = ""
            LET mpwalking = 1
        END IF
    ELSE
        LET mpfootloop = 0: LET mpwalking = 0: REM stops moonwalking bug
    END IF
    LET diagonalmove = 0
    IF a$ = "Q" AND bcontrol = 1 THEN GOSUB fadeout: LET menu$ = "mainmenu": GOSUB menugenerator: GOSUB mapmusicsetter: GOSUB musicplay: GOSUB fadein: REM opens main menu
    IF a$ = "I" AND pcontrol = 1 THEN GOSUB pocketdraw: REM opens pockets
    IF a$ = " " AND scontrol = 1 THEN GOSUB useobject: REM interacts world object or player
END IF
IF devmode = 1 THEN IF a$ = "\" OR a$ = "/" THEN GOSUB prompt: REM developer prompt if developer mode is on
IF scriptrun = 0 THEN
    REM walking and idle animation switch
    IF _KEYDOWN(19200) = 0 THEN LET temp1 = temp1 + 1
    IF _KEYDOWN(19712) = 0 THEN LET temp1 = temp1 + 1
    IF _KEYDOWN(20480) = 0 THEN LET temp1 = temp1 + 1
    IF _KEYDOWN(18432) = 0 THEN LET temp1 = temp1 + 1
    IF temp1 = 4 THEN
        IF mpwalking = 1 THEN LET mpidle = INT(ctime) + playeridle
        LET mpwalking = 0
    END IF
END IF
REM flush temporary values
LET temp1 = 0
RETURN

useobject:
REM interacts with real world object or player
IF selectobject$ = "" THEN RETURN: REM return if mainplayer not around any object or player
LET scriptname$ = LCASE$(selectobject$)
IF _FILEEXISTS(scriptloc$ + mapdir$ + scriptname$ + ".vsf") THEN
    LET mapscript = 1
    GOSUB script
ELSE
    IF selectobject$ <> "[COLLISIONONLY]" THEN ERROR 423
END IF
LET clearscreen = 1
RETURN

mapmusicsetter:
REM sets playmusic$ to music attached to map
OPEN mloc$ + "/" + mapdir$ + "/" + mapfile$ + ".ddf" FOR INPUT AS #1
INPUT #1, dummy$, playmusic$
CLOSE #1
LET dummy$ = ""
RETURN

loadgame:
REM game load sequence whilst in gameplay
REM shifts values for possible mainplayer sprite change
LET oldmplayermodel$ = mplayermodel$
LET oldmapno = mapno
GOSUB saveload
GOSUB mainplayerload
GOSUB mapload
RETURN

prompt:
REM ENGINE COMMAND PROMPT
COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura)
DO
    LET temp = 0: REM flush value for loop purposes
    INPUT ">"; prompt$
    IF prompt$ = "" THEN
        COLOR 0, 0
        CLS
        LET temp = 0
        LET temp5 = 0
        LET temp1$ = ""
        LET prompt$ = ""
        LET action$ = ""
        LET value$ = ""
        RETURN
    END IF
    LET action$ = LEFT$(prompt$, INSTR(prompt$, " ") - 1)
    LET value$ = RIGHT$(prompt$, LEN(prompt$) - LEN(action$))
    LET value$ = LTRIM$(value$)
    LET action$ = LCASE$(action$)
    LET value$ = LCASE$(value$)
    REM say
    IF action$ = "save" THEN
        IF value$ = "erase" THEN GOSUB erasesave: PRINT "SAVE ERASED!": LET temp = 1
        IF value$ = "game" THEN GOSUB savesave: PRINT "GAME SAVED!": LET temp = 1
        IF value$ = "load" THEN
			GOSUB loadgame
			PRINT "GAME LOADED!"
			LET temp = 1
		END IF
        IF value$ = "default" THEN GOSUB savedefault: PRINT "SAVE DEFAULT SET!": LET temp = 1
    END IF
    IF action$ = "say" THEN
        LET textspeech$ = value$
        GOSUB textbannerdraw
        COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura)
        LET temp = 1
    END IF
    REM mark item as "gone forever"
    IF action$ = "markgone" THEN
        PRINT "WARNING: this will remove the " + value$ + " from the gameplay FOREVER!"
        PRINT "are you sure you want to proceed? (Y/N)"
        INPUT temp24$
        IF UCASE$(temp24$) = "Y" THEN
            LET takeitem$ = value$
            GOSUB markgone
            PRINT value$ + " marked as gone forever"
        END IF
        LET temp = 1
    END IF
    REM recover item
    IF action$ = "markback" THEN
        OPEN pocketloc$ + "pocketfiles.ddf" FOR INPUT AS #1
        REM seaches for item in pocketfiles
        DO
            LET temp96 = temp96 + 1
            INPUT #1, pocketfile$
        LOOP UNTIL pocketfile$ = value$ OR EOF(1)
        CLOSE #1
        IF pocketfile$ <> value$ OR pocketfile$ = "currency" THEN
            REM if search finds nothing or currency is attempted to be removed
            REM prints to console
            LET eventtitle$ = "INVALID ITEM:"
            LET eventdata$ = value$
            LET eventnumber = 0
            GOSUB consoleprinter
        ELSE
            REM assigns item
            LET pocketitem(temp96) = 0
            REM prints to console
            LET eventtitle$ = "ITEM RECOVERED:"
            LET eventdata$ = value$
            LET eventnumber = temp65
            GOSUB consoleprinter
        END IF
        LET temp = 1
        LET temp96 = 0: REM scrubs temp values
    END IF
    REM give currency
    IF action$ = "givecurrency" THEN
        LET currencychange = VAL(value$)
        GOSUB givecurrency
        COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura)
        LET temp = 1
    END IF
    REM take currency
    IF action$ = "takecurrency" THEN
        LET currencychange = VAL(value$)
        GOSUB takecurrency
        COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura)
        LET temp = 1
    END IF
    REM give item
    IF action$ = "giveitem" THEN
        LET giveitem$ = value$
        GOSUB giveitem
        IF pocketfile$ = giveitem$ THEN LET temp = 1
    END IF
    REM take item
    IF action$ = "takeitem" THEN
        LET takeitem$ = value$
        GOSUB takeitem
        IF pocketfile$ = takeitem$ THEN LET temp = 1
    END IF
    REM play music
    REM play sound effect
    IF action$ = "sfx" THEN
        LET playsfx$ = value$
        GOSUB sfxplay
        LET temp = 1
    END IF
    REM run script
    IF action$ = "script" THEN
        REM map script
        IF value$ = "stop" THEN
            REM stop script
            LET temp26 = 2
            LET temp = 1
            PRINT "SCRIPT STOPPED!"
        ELSE
            IF _FILEEXISTS(scriptloc$ + mapdir$ + value$ + ".vsf") THEN
                LET scriptname$ = value$: LET mapscript = 1: GOSUB script: LET temp = 1
            ELSE
                REM combination script
                IF _FILEEXISTS(scriptloc$ + "combine/" + value$ + ".vsf") THEN
                    LET scriptname$ = value$: LET mapscript = 2: GOSUB script: LET temp = 1
                ELSE
                    PRINT "SCRIPT NOT FOUND": LET action$ = "ilovexander": LET temp = 1
                END IF
            END IF
        END IF
    END IF
    REM simulate errors
    IF action$ = "error" THEN
        ERROR VAL(value$): LET temp = 1
        IF temp = 0 THEN LET temp = 2: REM sets invalid argument error
    END IF
    REM SHELL
    IF action$ = "shell" THEN SHELL value$: LET temp = 1
    REM change  value
    IF action$ = "change" THEN
        IF value$ = "pace" THEN INPUT "INSERT VALUE> "; temp5: LET pace = temp5: LET temp = 1
        IF value$ = "mapno" THEN INPUT "INSERT VALUE> "; temp5: LET oldmapname$ = mapname$: LET oldmapno = mapno: LET mapno = temp5: LET temp = 1
        IF value$ = "direction" THEN INPUT "INSERT VALUE> "; temp5: LET direction = temp5: LET temp = 1
        IF value$ = "mainplayer" THEN PRINT "INSERT VALUE>": INPUT temp1$: LET oldmplayermodel$ = mplayermodel$: LET mplayermodel$ = temp1$: LET temp = 1
        IF value$ = "posx" THEN INPUT "INSERT VALUE> "; temp5: LET posx = temp5: LET temp = 1
        IF value$ = "posy" THEN INPUT "INSERT VALUE> "; temp5: LET posy = temp5: LET temp = 1
        IF value$ = "resx" THEN INPUT "INSERT VALUE> "; temp5: LET resx = temp5: PRINT "VaME requires reolution to be set in "; dloc$; "engine.ddf for object collsion to function.": LET temp = 1
        IF value$ = "resy" THEN INPUT "INSERT VALUE> "; temp5: LET resy = temp5: PRINT "VaME requires reolution to be set in "; dloc$; "engine.ddf for object collsion to function.": LET temp = 1
        IF value$ = "defaultfontsize" THEN INPUT "INSERT VALUE> "; temp5: LET defaultfontsize = temp5: LET temp = 1
        IF value$ = "defaultfontname" THEN PRINT "INSERT VALUE>": INPUT temp1$: LET defaultfontname$ = temp1$: LET temp = 1
        IF value$ = "defaultfontstyle" THEN PRINT "INSERT VALUE>": INPUT temp1$: LET defaultfontstyle$ = temp1$: LET temp = 1
        IF value$ = "headerfontsize" THEN INPUT "INSERT VALUE> "; temp5: LET headerfontsize = temp5: LET temp = 1
        IF value$ = "headerfontname" THEN PRINT "INSERT VALUE>": INPUT temp1$: LET headerfontname$ = temp1$: LET temp = 1
        IF value$ = "headerfontstyle" THEN PRINT "INSERT VALUE>": INPUT temp1$: LET headertfontstyle$ = temp1$: LET temp = 1
        IF value$ = "smallfontsize" THEN INPUT "INSERT VALUE> "; temp5: LET smallfontsize = temp5: LET temp = 1
        IF value$ = "smallfontname" THEN PRINT "INSERT VALUE>": INPUT temp1$: LET smalltfontname$ = temp1$: LET temp = 1
        IF value$ = "smallfontstyle" THEN PRINT "INSERT VALUE>": INPUT temp1$: LET smallfontstyle$ = temp1$: LET temp = 1
        IF value$ = "noclip" THEN INPUT "INSERT VALUE> "; temp5: LET noclip = temp5: LET temp = 1
        IF value$ = "devmode" THEN INPUT "INSERT VALUE> "; temp5: LET devmode = temp5: LET temp = 1
        IF value$ = "playmusic" THEN INPUT "INSERT VALUE> "; temp1$: LET playmusic$ = temp1$: LET temp = 1
        IF value$ = "soundmode" THEN INPUT "INSERT VALUE> "; temp5: LET soundmode = temp5: LET temp = 1
        IF value$ = "ros" THEN INPUT "INSERT VALUE> "; temp1$: LET ros$ = temp1$: LET temp = 1
        IF value$ = "disablefade" THEN INPUT "INSERT VALUE> "; temp5: LET disablefade = temp5: LET temp = 1
        IF value$ = "currency" THEN INPUT "INSERT VALUE> "; temp5: LET currency = temp5: LET temp = 1
        IF value$ = "mapeffect" THEN INPUT "INSERT VALUE> "; temp5: LET mapeffect = temp5: LET temp = 1
        IF value$ = "versionno" THEN INPUT "INSERT VALUE> "; temp1$: LET versionno$ = temp1$: LET temp = 1
        IF value$ = "musicvol" THEN INPUT "INSERT VALUE> "; temp5: LET musicvol = temp5: GOSUB musicvol: LET temp = 1
        IF value$ = "sfxvol" THEN INPUT "INSERT VALUE> "; temp5: LET sfxvol = temp5: GOSUB sfxvol: LET temp = 1
        IF value$ = "displayconsole" THEN INPUT "INSERT VALUE> "; temp5: LET displayconsole = temp5: LET temp = 1
        IF value$ = "timer" THEN
            RANDOMIZE TIMER
            LET itime = TIMER: REM timer function
            LET ctime = 0: REM timer function
            PRINT "TIMER RESET"
            LET temp = 1
        END IF
        REM prints extra console data confirming value change
        IF temp = 1 THEN
            LET eventtitle$ = "VALUE CHANGE: "
            IF temp5 <> 0 THEN
                LET eventdata$ = value$ + " = "
                LET eventnumber = temp5
            ELSE
                LET eventdata$ = value$ + " = " + temp1$
            END IF
            GOSUB consoleprinter
            LET eventtitle$ = "": LET eventdata$ = "": LET eventnumber = 0
        END IF
        IF temp = 0 THEN LET temp = 2: REM sets invalid argument error
    END IF
    REM Whatis?
    IF action$ = "whatis" THEN
        IF value$ = "noclip" THEN PRINT noclip: LET temp = 1
        IF value$ = "resx" THEN PRINT resx: LET temp = 1
        IF value$ = "resy" THEN PRINT resy: LET temp = 1
        IF value$ = "mapno" THEN PRINT mapno: LET temp = 1
        IF value$ = "frames" THEN PRINT frames: LET temp = 1
        IF value$ = "ros" THEN PRINT ros$: LET temp = 1
        IF value$ = "mplayermodel" THEN PRINT mplayermodel$: LET temp = 1
        IF value$ = "pace" THEN PRINT pace: LET temp = 1
        IF value$ = "ctime" THEN PRINT ctime: LET temp = 1
        IF value$ = "itime" THEN PRINT itime: LET temp = 1
        IF value$ = "location" THEN PRINT "X: "; posx: PRINT "Y: "; posy: LET temp = 1
        IF value$ = "oldlocation" THEN PRINT "oX: "; oposx: PRINT "oY :"; oposy: LET temp = 1
        IF value$ = "direction" THEN PRINT direction: LET temp = 1
        IF value$ = "posx" THEN PRINT posx: LET temp = 1
        IF value$ = "posy" THEN PRINT posy: LET temp = 1
        IF value$ = "oposx" THEN PRINT oposx: LET temp = 1
        IF value$ = "oposy" THEN PRINT oposy: LET temp = 1
        IF value$ = "mapname" THEN PRINT mapname$: LET temp = 1
        IF value$ = "mpx" THEN PRINT mpx: LET temp = 1
        IF value$ = "mpy" THEN PRINT mpy: LET temp = 1
        IF value$ = "mpposx" THEN PRINT mpposx: LET temp = 1
        IF value$ = "mpposy" THEN PRINT mpposy: LET temp = 1
        IF value$ = "mapx" THEN PRINT mapx: LET temp = 1
        IF value$ = "mapy" THEN PRINT mapy: LET temp = 1
        IF value$ = "mpwalking" THEN PRINT mpwalking: LET temp = 1
        IF value$ = "mpfoot" THEN PRINT mpfoot: LET temp = 1
        IF value$ = "mpfootloop" THEN PRINT mpfootloop: LET temp = 1
        IF value$ = "footpace" THEN PRINT footpace: LET temp = 1
        IF value$ = "hud" THEN PRINT hud: LET temp = 1
        IF value$ = "clearscreen" THEN PRINT clearscreen: LET temp = 1
        IF value$ = "errdescription" THEN PRINT errdescription$: LET temp = 1
        IF value$ = "err" THEN PRINT ERR: LET temp = 1
        IF value$ = "errorline" THEN PRINT _ERRORLINE: LET temp = 1
        IF value$ = "date" THEN PRINT DATE$: LET temp = 1
        IF value$ = "time" THEN PRINT TIME$: LET temp = 1
        IF value$ = "fps" THEN PRINT fps: LET temp = 1
        IF value$ = "oldmapno" THEN PRINT oldmapno: LET temp = 1
        IF value$ = "oldmapname" THEN PRINT oldmapname$: LET temp = 1
        IF value$ = "oldmplayermodel" THEN PRINT oldmplayermodel$: LET temp = 1
        IF value$ = "headerfontname" THEN PRINT headerfontsname$: LET temp = 1
        IF value$ = "headerfontstyle" THEN PRINT headerfontstyle$: LET temp = 1
        IF value$ = "headerfontsize" THEN PRINT headerfontsize: LET temp = 1
        IF value$ = "defaultfontname" THEN PRINT defaultfontsname$: LET temp = 1
        IF value$ = "defaultfontstyle" THEN PRINT defaultfontstyle$: LET temp = 1
        IF value$ = "defaultfontsize" THEN PRINT defaultfontsize: LET temp = 1
        IF value$ = "smallfontname" THEN PRINT smallfontsname$: LET temp = 1
        IF value$ = "smallfontstyle" THEN PRINT smallfontstyle$: LET temp = 1
        IF value$ = "smallfontsize" THEN PRINT smallfontsize: LET temp = 1
        IF value$ = "devmode" THEN PRINT devmode: LET temp = 1
        IF value$ = "mapobjectno" THEN PRINT mapobjectno: LET temp = 1
        IF value$ = "maptriggerno" THEN PRINT maptriggerno: LET temp = 1
        IF value$ = "mapplayerno" THEN PRINT mapplayerno: LET temp = 1
        IF value$ = "playmusic" THEN PRINT playmusic$: LET temp = 1
        IF value$ = "currentmusic" THEN PRINT currentmusic$: LET temp = 1
        IF value$ = "oldmusic" THEN PRINT oldmusic$: LET temp = 1
        IF value$ = "soundmode" THEN PRINT soundmode: LET temp = 1
        IF value$ = "mpidle" THEN PRINT mpidle: LET temp = 1
        IF value$ = "playeridle" THEN PRINT playeridle: LET temp = 1
        IF value$ = "pocketnos" THEN PRINT pocketnos: LET temp = 1
        IF value$ = "disablefade" THEN PRINT disablefade: LET temp = 1
        IF value$ = "currency" THEN PRINT currency: LET temp = 1
        IF value$ = "mapeffect" THEN PRINT mapeffect: LET temp = 1
        IF value$ = "pocketcarry" THEN PRINT pocketcarry: LET temp = 1
        IF value$ = "versionno" THEN PRINT versionno$: LET temp = 1
        IF value$ = "timer" THEN PRINT TIMER: LET temp = 1
        IF value$ = "scriptline" THEN PRINT scriptline$: LET temp = 1
        IF value$ = "musicvol" THEN PRINT musicvol: LET temp = 1
        IF value$ = "sfxvol" THEN PRINT sfxvol: LET temp = 1
        IF value$ = "engineversionno" THEN PRINT engineversionno$: LET temp = 1
        IF value$ = "exitsave" THEN PRINT exitsave: LET temp = 1
        IF value$ = "collisionstep" THEN PRINT collisionstep: LET temp = 1
        IF value$ = "objectname" THEN
			LET temp = 1
			IF mapobjectno > 0 THEN
				LET x = 0
				DO
					LET x = x + 1
					PRINT objectname(x)
				LOOP UNTIL x >= mapobjectno
				LET x = 0
			ELSE
				PRINT "NO OBJECTS ATTACHED TO MAP"
			END IF
		END IF
        IF value$ = "pocketname" THEN
			LET temp = 1
			IF pocketnos > 0 THEN
				LET x = 0
				DO
					LET x = x + 1
					PRINT pocketname(x)
				LOOP UNTIL x >= pocketnos
				LET x = 0
			ELSE
				PRINT "NO POCKET ITEMS LOADED"
			END IF
		END IF
        IF value$ = "triggername" THEN
            LET temp = 1
            IF maptriggerno > 0 THEN
                DO
                    LET temp15 = temp15 + 1
                    PRINT triggername(temp15)
                LOOP UNTIL temp15 >= maptriggerno
                LET temp15 = 0: REM scrub temp values
            ELSE
                PRINT "NO TRIGGERS ATTACHED TO MAP"
            END IF
        END IF
        IF value$ = "playername" THEN
            LET temp = 1
            IF mapplayerno > 0 THEN
                DO
                    LET temp43 = temp43 + 1
                    PRINT playername$(temp43)
                LOOP UNTIL temp43 >= mapplayerno
                LET temp43 = 0: REM scrub temp values
            ELSE
                PRINT "NO PLAYERS ATTACHED TO MAP"
            END IF
        END IF
        IF temp = 0 THEN LET temp = 2: REM sets invalid argument error
    END IF
    REM system (resets or halts system subs)
    IF action$ = "system" THEN
        LET promptquit = 1
        IF value$ = "now" THEN LET temp = 1: COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura): PRINT "SYSTEM NOW!": GOSUB consolequit: SYSTEM
        IF value$ = "hang" THEN LET temp = 1: COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura): PRINT "SYSTEM HUNG!": GOSUB consolequit: END
        IF value$ = "map" THEN LET temp = 1: GOSUB mapload: COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura): PRINT "MAP DATA RELOADED!"
        IF value$ = "mainplayer" THEN LET temp = 1: GOSUB mainplayerload: COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura): PRINT "MAINPLAYER DATA RELOADED!"
        IF value$ = "screen" THEN LET temp = 1: GOSUB screenload: GOSUB fontload: LET clearscreen = 1: COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura): PRINT "SCREEN DATA RELOADED!"
        IF value$ = "font" THEN LET temp = 1: GOSUB fontunload: GOSUB fontload: COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura): PRINT "FONT DATA RELOADED!"
        IF value$ = "quit" THEN LET temp = 1: COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura): PRINT "SYSTEM QUIT!": GOSUB consolequit: GOTO endgame
        IF value$ = "ui" THEN LET temp = 1: GOSUB uiunload: GOSUB uiload: COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura): PRINT "UI DATA RELOADED!"
        IF value$ = "pockets" THEN LET temp = 1: GOSUB pocketunload: GOSUB pocketload: COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura): PRINT "POCKET DATA RELOADED!"
        IF value$ = "music" THEN LET temp = 1: GOSUB musicunload: GOSUB musicload: COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura): PRINT "MUSIC DATA RELOADED!"
        IF value$ = "sfx" THEN LET temp = 1: GOSUB sfxunload: GOSUB sfxload: COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura): PRINT "SOUND EFFECT DATA RELOADED!"
        IF value$ = "terminal" THEN LET temp = 1: GOSUB terminalunload: GOSUB terminalload: COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura): PRINT "TERMINAL DATA RELOADED!"
        IF value$ = "update" THEN LET temp = 1: GOSUB updatechecker: COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura): PRINT "UPDATE CHECK COMPLETE!"
        IF value$ = "speedrun" THEN LET temp = 1: LET hud = 12: LET speedrun = 1: COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura): PRINT "SPEEDRUN ENABLED!"
        IF value$ = "fix" THEN LET temp = 1: LET fixvame = 1: GOSUB updatechecker: LET fixvame = 0: COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura): PRINT "FIXING COMPLETE!"
        IF value$ = "colour" THEN
            OPEN "data\colours.ddf" FOR INPUT AS #1
            INPUT #1, letmenuselectcolourr, letmenuselectcolourg, letmenuselectcolourb, letmenuselectcoloura, bgmenuselectcolourr, bgmenuselectcolourg, bgmenuselectcolourb, bgmenuselectcoloura, letmenudefaultcolourr, letmenudefaultcolourg, letmenudefaultcolourb, letmenudefaultcoloura, bgmenudefaultcolourr, bgmenudefaultcolourg, bgmenudefaultcolourb, bgmenudefaultcoloura, letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura, bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura, letpocketselectcolourr, letpocketselectcolourg, letpocketselectcolourb, letpocketselectcoloura, bgpocketselectcolourr, bgpocketselectcolourg, bgpocketselectcolourb, bgpocketselectcoloura, letpocketdefaultcolourr, letpocketdefaultcolourg, letpocketdefaultcolourb, letpocketdefaultcoloura, bgpocketdefaultcolourr, bgpocketdefaultcolourg, bgpocketdefaultcolourb, bgpocketdefaultcoloura, letcurrencycolourr, letcurrencycolourg, letcurrencycolourb, letcurrencycoloura, bgcurrencycolourr, bgcurrencycolourg, bgcurrencycolourb, bgcurrencycoloura, letspeechcolourr, letspeechcolourg, letspeechcolourb, letspeechcoloura, bgspeechcolourr, bgspeechcolourg, bgspeechcolourb, bgspeechcoloura, letterminalcolourr, letterminalcolourg, letterminalcolourb, letterminalcoloura, bgterminalcolourr, bgterminalcolourg, bgterminalcolourb, bgterminalcoloura
            CLOSE #1
            COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura)
            PRINT "COLOURS RELOADED!"
            LET temp = 1
        END IF
        IF value$ = "consolelog" THEN
            REM erase consolelog.txt
            OPEN consolelog$ FOR OUTPUT AS #3
            PRINT #3, DATE$, TIME$, "VaME CONSOLE LOG"
            CLOSE #3
            COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura)
            PRINT "CONSOLELOG.TXT ERASED"
            LET temp = 1
        END IF
        IF temp = 0 THEN LET temp = 2: REM sets invalid argument error
        LET promptquit = 0
    END IF
    REM hud display
    IF action$ = "display" THEN
        IF value$ = "off" THEN LET hud = 0: LET clearscreen = 1: LET temp = 1
        IF value$ = "location" THEN LET hud = 1: LET temp = 1
        IF value$ = "olocation" THEN LET hud = 2: LET temp = 1
        IF value$ = "time" THEN LET hud = 3: LET temp = 1
        IF value$ = "frames" THEN LET hud = 4: LET temp = 1
        IF value$ = "maplocation" THEN LET hud = 5: LET temp = 1
        IF value$ = "fadein" THEN GOSUB fadein: LET temp = 1
        IF value$ = "fadeout" THEN GOSUB fadeout: LET temp = 1
        IF value$ = "techspecs" THEN LET hud = 6: LET temp = 1
        IF value$ = "layers" THEN LET hud = 7: LET temp = 1
        IF value$ = "mplayerlocation" THEN LET hud = 8: LET temp = 1
        IF value$ = "selectobject" THEN LET hud = 9: LET temp = 1
        IF value$ = "playerlocation" THEN LET hud = 10: LET temp = 1
        IF value$ = "mouselocation" THEN LET hud = 11: LET temp = 1
        IF value$ = "console" THEN GOSUB displayconsole: LET temp = 1
        IF temp = 0 THEN LET temp = 2: REM sets invalid argument error
    END IF
    REM write console activity to consolelog.txt and display console command results on screen
    OPEN consolelog$ FOR APPEND AS #2
    IF temp = 0 THEN
        IF consolelogging = 1 THEN PRINT #2, DATE$, TIME$, "INVALID PROMPT COMMAND: ", prompt$
        PRINT "INVALID COMMAND - "; prompt$
        IF displayconsole = 1 THEN
            _DEST _CONSOLE
            PRINT DATE$, TIME$, "INVALID PROMPT COMMAND: ", prompt$
            _DEST 0
        END IF
    END IF
    IF temp = 1 THEN
        IF consolelogging = 1 THEN PRINT #2, DATE$, TIME$, "PROMPT COMMAND: ", prompt$
        PRINT "OK!"
        IF displayconsole = 1 THEN
            _DEST _CONSOLE
            PRINT DATE$, TIME$, "PROMPT COMMAND: ", prompt$
            _DEST 0
        END IF
    END IF
    IF temp = 2 THEN
        PRINT #2, DATE$, TIME$, "INVALID PROMPT ARGUMENT: ", prompt$
        PRINT "INVALID ARGUMENT - "; value$
        IF displayconsole = 1 THEN
            _DEST _CONSOLE
            PRINT DATE$, TIME$, "INVALID PROMPT ARGUMENT: ", prompt$
            _DEST 0
        END IF
    END IF
    CLOSE #2
    REM quits prompt if script has been run
    IF action$ = "script" THEN
        REM flush temp values
        LET temp = 0
        LET temp5 = 0
        LET temp1$ = ""
        LET prompt$ = "": LET action$ = "": LET value$ = ""
        REM return to engine loop
        RETURN
    END IF
LOOP
RETURN

consoleboot:
REM writes console boot data to consolelog.txt
IF consolelogging = 1 THEN
    IF _FILEEXISTS(consolelog$) THEN
        REM nothing
    ELSE
        OPEN consolelog$ FOR OUTPUT AS #2
        PRINT #2, DATE$, TIME$, "VaME CONSOLE LOG"
        CLOSE #2
    END IF
    OPEN consolelog$ FOR APPEND AS #2
    PRINT #2, DATE$, TIME$, "=== SYSTEM BOOT ==="
    CLOSE #2
END IF
REM prints to console
IF displayconsole = 1 THEN
    _DEST _CONSOLE
    PRINT DATE$, TIME$, "=== SYSTEM BOOT ==="
    _DEST 0
END IF
RETURN

consolequit:
REM writes console quit data to consolelog.txt
IF consolelogging = 1 THEN
    IF _FILEEXISTS(consolelog$) THEN
        REM nothing
    ELSE
        OPEN consolelog$ FOR OUTPUT AS #2
        PRINT #2, DATE$, TIME$, "VaME CONSOLE LOG"
        CLOSE #2
    END IF
    OPEN consolelog$ FOR APPEND AS #2
    IF errorcrash = 1 THEN LET temp2$ = "=== GURU MEDITATION ==="
    IF userquit = 1 THEN LET temp2$ = "=== SYSTEM QUIT ==="
    IF promptquit = 1 THEN
        IF value$ = "hang" THEN LET temp2$ = "=== SYSTEM HUNG ==="
        IF value$ = "now" THEN LET temp2$ = "=== SYSTEM HALT ==="
    END IF
    PRINT #2, DATE$, TIME$, temp2$
    CLOSE #2
END IF
REM displays on console
IF displayconsole = 1 THEN
    _DEST _CONSOLE
    PRINT DATE$, TIME$, temp2$
    _DEST 0
END IF
REM scrub temp values
LET temp2$ = ""
RETURN

hud:
REM developer hud display
COLOR _RGBA(letpromptcolourr, letpromptcolourg, letpromptcolourb, letpromptcoloura), _RGBA(bgpromptcolourr, bgpromptcolourg, bgpromptcolourb, bgpromptcoloura)
REM location hud
IF hud = 1 THEN
    LOCATE 1, 1: PRINT "X: "; posx
    LOCATE 2, 1: PRINT "Y: "; posy
    LOCATE 3, 1: PRINT "D: "; direction
END IF
REM old location hud
IF hud = 2 THEN
    LOCATE 1, 1: PRINT "oX: "; oposx
    LOCATE 2, 1: PRINT "oY: "; oposy
END IF
REM time hud
IF hud = 3 THEN
    LOCATE 1, 1: PRINT "ctime: "; ctime
    LOCATE 2, 1: PRINT "time: "; TIME$
END IF
REM frames hud
IF hud = 4 THEN
    LOCATE 1, 1: PRINT "frame: "; frames
    LOCATE 2, 1: PRINT "fps: "; fps
END IF
REM map location hud
IF hud = 5 THEN
    LOCATE 1, 1: PRINT "MX1: "; maploc1x; " MY1: "; maploc1y
    LOCATE 2, 1: PRINT "MX2: "; maploc2x; " MY2: "; maploc2y
END IF
REM tech specs
IF hud = 6 THEN
    LOCATE 1, 1: PRINT title$ + " " + versionno$
    LOCATE 2, 1: PRINT "VaME " + engineversionno$
    LOCATE 3, 1: PRINT "TIMER: "; INT(TIMER); " CTIME: "; INT(ctime)
    LOCATE 4, 1: PRINT "FPS: "; fps; "FRAMES: "; frames
    LOCATE 5, 1: PRINT "RES: "; resx; "x"; resy
END IF
REM layers
IF hud = 7 THEN
    IF mapobjectno > 0 THEN
		LET x = 0
        DO
            LET temp16 = temp16 + 1
            LET x = x + 1
            LOCATE temp16, 1: PRINT objectname$(x) + " " + STR$(objectl(x))
        LOOP UNTIL x >= mapobjectno
        LET temp16 = 0
    ELSE
        LOCATE 1, 1: PRINT "No objects attached to map."
    END IF
    IF mapplayerno > 0 THEN
		LET x = 0
		DO
			LET temp16 = temp16 + 1
			LET x = x + 1
			LOCATE temp16, 1: PRINT playername$(x) + " " + STR$(objectl(x))
		LOOP UNTIL x >= mapplayerno
	ELSE
		LET x = x + 1
		LOCATE x, 1: PRINT "No players attached to map."
	END IF
    LET temp16 = 0: REM scrub temp values
END IF
REM main player location
IF hud = 8 THEN
    LOCATE 1, 1: PRINT "X: "; (resx / 2) - posx
    LOCATE 2, 1: PRINT "Y: "; (resy / 2) - posy
    LOCATE 3, 1: PRINT "D: "; direction
END IF
REM selected world object
IF hud = 9 THEN
    LOCATE 1, 1: PRINT selectobject$
END IF
REM NPC location
IF hud = 10 THEN
    IF mapplayerno > 0 THEN
        DO
            LET temp16 = temp16 + 1
            LOCATE temp16, 1: PRINT playername$(temp16) + " X:" + STR$(playerx(temp16)) + " Y:" + STR$(playery(temp16))
        LOOP UNTIL temp16 >= mapplayerno
    ELSE
        LOCATE 1, 1: PRINT "No players attached to map."
    END IF
    LET temp16 = 0: REM scrub temp values
END IF
REM mouse location
IF hud = 11 THEN
    LET mouse = _MOUSEINPUT
    LOCATE 1, 1: PRINT _MOUSEX
    LOCATE 2, 1: PRINT _MOUSEY
END IF
COLOR 0, 0
RETURN

endgame:
REM quits game
REM prints user requested quit to console
LET userquit = 1: REM tells engine user has requested a system quit
LET eventtitle$ = "SYSTEM QUIT REQUESTED!"
LET eventdata$ = "FRAMES: "
LET eventnumber = frames
GOSUB consoleprinter
REM saves game, unloads data and fades out game
REM if game is running
IF setupboot = 0 THEN
    IF temp82 <> 1 AND runupdate <> 1 AND exitsave = 1 THEN GOSUB savesave
    GOSUB playerunload
    GOSUB objectunload
    GOSUB mapunload
    GOSUB mainplayerunload
    GOSUB musicstop
    GOSUB sfxunload
    GOSUB musicunload
    GOSUB pocketunload
    GOSUB terminalunload
    GOSUB uiunload
    GOSUB fontunload
END IF
REM if game isn't running (main menu)
IF setupboot = 1 THEN
    GOSUB musicstop
    GOSUB sfxunload
    GOSUB musicunload
    GOSUB pocketunload
    GOSUB terminalunload
    GOSUB uiunload
    GOSUB fontunload
END IF
GOSUB consolequit: REM writes quit to consolelog.txt
CLS
IF runupdate = 1 THEN RETURN: REM return to continue update process if needed
IF temp82 = 1 THEN
	REM reboot game for when a save has been erased
	IF installtype = 1 THEN
		REM cross platform
		IF ros$ = "win" THEN SHELL _DONTWAIT filename$ + "_win.exe -special101"
		IF ros$ = "mac" THEN SHELL _DONTWAIT "./" + filename$ + "_macos -special101"
		IF ros$ = "lnx" THEN SHELL _DONTWAIT "./" + filename$ + "_linux -special101"
	ELSE
		REM platform specific
		IF ros$ = "win" THEN SHELL _DONTWAIT filename$ + ".exe - special101"
		IF ros$ = "mac" OR ros$ = "lnx" THEN SHELL _DONTWAIT "./" + filename$ + " -special101"
	END IF
END IF
SYSTEM
