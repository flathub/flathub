REM Polybius.
REM By Sinnesloschen.

setup:
REM error handler
ON ERROR GOTO errorhandler
REM checks VAME settings for integration
IF _FILEEXISTS("/var/data/polydata/savedata.ddf") THEN
	OPEN "/var/data/polydata/savedata.ddf" FOR INPUT AS #1
	INPUT #1, nope1, nope2, screenmode, nope3, nope4, nope5, soundmode, nope6, nope7, nope8
	CLOSE #1
ELSE
	ERROR 666
END IF
REM sets up game and loads files
REM sets screen mode
SCREEN _NEWIMAGE(255, 302, 32)
$RESIZE:ON
_RESIZE ON , _STRETCH
IF screenmode = 1 THEN _FULLSCREEN _SQUAREPIXELS
_TITLE "Polybius"
_MOUSEHIDE
REM launches developer console
'$CONSOLE
'_CONSOLE ON
REM launches timer
RANDOMIZE TIMER
LET itime = TIMER
LET ctime = 0
REM checks data folder exists
IF _DIREXISTS("polydata\") THEN
    REM nothing :)
ELSE
    ERROR 666
END IF
REM loads and applies font
LET gamefont = _LOADFONT("/var/data/polydata/gamefont.ttf", 12)
_FONT gamefont
REM loads image files
LET titledisplay = _LOADIMAGE("/var/data/polydata/title.png")
LET titledisplay2 = _LOADIMAGE("/var/data/polydata/title2.png")
REM loads audio files
LET titlemusic = _SNDOPEN("/var/data/polydata/titlemusic.ogg")
LET bulletfire = _SNDOPEN("/var/data/polydata/bullet.ogg")
LET titlesfx = _SNDOPEN("/var/data/polydata/titlesfx.ogg")
LET gamestart = _SNDOPEN("/var/data/polydata/gamestart.ogg")
LET gameover = _SNDOPEN("/var/data/polydata/gameover.ogg")
LET hit = _SNDOPEN("/var/data/polydata/hit.ogg")
LET newlevel = _SNDOPEN("/var/data/polydata/newlevel.ogg")
LET playerhit = _SNDOPEN("/var/data/polydata/playerhit.ogg")
LET playerdeath = _SNDOPEN("/var/data/polydata/playerdeath.ogg")
LET highscore = _SNDOPEN("/var/data/polydata/highscore.ogg")
REM sets draw values
LET playership$ = "E5 F5 L10 D10 R10 U10 L10 G10 R20 H10 R10 F10 L20 E10"
LET bullet$ = "U10"
LET triangle$ = "F15 L30 E15"
LET square$ = "R10 U20 L20 D20 R10"
LET diamond$ = "E10 H10 G10 F10"
LET particle$ = "D1"
REM displays loading visuals
DO
    LET temp = INT(RND * 302)
    LINE (0, temp)-(302, temp), &HFF5454FC
    _DELAY 0.05
    LET temp2 = temp2 + 1
    CLS
LOOP UNTIL temp2 >= 20
COLOR &HFF5454FC
_PRINTSTRING ((225 / 2), (302 / 2)), "ROM OK"
_DELAY 2
CLS
GOSUB titlescreen
GOTO playgame

timekeeper:
REM keeps game time
LET ctime = (TIMER - itime)
LET ctime = INT(ctime)
IF playinggame = 0 THEN RETURN
REM generates time events
IF nextlevel = 0 THEN
	REM next level
	LET nextlevel = ctime + 40
END IF
IF level > 1 THEN 
	IF nextmessage = 0 THEN
		REM next message
		LET nextmessage = ctime + INT(RND * 60) + 1
	END IF
END IF
REM executes time events
IF nextlevel =< ctime THEN
	REM next level
	LET nextlevel = 0
	CLS
	GOSUB nextlevel
END IF
IF level > 1 THEN
	IF nextmessage =< ctime THEN
		REM next message
		LET nextmessage = 0
		GOSUB subliminalmessage
	END IF
END IF
RETURN

subliminalmessage:
REM brainwashing messages
LET messagem = INT(RND * 6) + 1
CLS
IF messagem = 1 THEN LET messagem$ = "CONSUME": LET messagel = 7
IF messagem = 2 THEN LET messagem$ = "OBEY": LET messagel = 4
IF messagem = 3 THEN LET messagem$ = "BE AFRAID": LET messagel = 8
IF messagem = 4 THEN LET messagem$ = "STAY AWAKE": LET messagel = 10
IF messagem = 5 THEN LET messagem$ = "KEEP WORKING": LET messagel = 12
IF messagem = 6 THEN LET messagem$ = "FEAR ME": LET messagel = 7
_PRINTSTRING(((225 / 2)-(messagel / 2)), (302 / 2)), messagem$
_DELAY 0.5
CLS
RETURN

newvalues:
REM generates values for new game
LET playinggame = 1
LET score = 0
LET health = 100
LET level = 0
LET playerx = (255 / 2)
LET oldplayerx = playerx
LET playery = 290
LET oldplayery = playery
LET fire = 0
LET particle = 0
LET speed = 4
LET frames = 60
REM generates new stars
LET star1x = INT(RND * 255) + 1
LET star1y = INT(RND * 302) + 1
LET star2x = INT(RND * 255) + 1
LET star2y = INT(RND * 302) + 1
LET star3x = INT(RND * 255) + 1
LET star3y = INT(RND * 302) + 1
LET star4x = INT(RND * 255) + 1
LET star4y = INT(RND * 302) + 1
LET star5x = INT(RND * 255) + 1
LET star5y = INT(RND * 302) + 1
LET star6x = INT(RND * 255) + 1
LET star6y = INT(RND * 302) + 1
LET star7x = INT(RND * 255) + 1
LET star7y = INT(RND * 302) + 1
LET star8x = INT(RND * 255) + 1
LET star8y = INT(RND * 302) + 1
LET star9x = INT(RND * 255) + 1
LET star9y = INT(RND * 302) + 1
LET star10x = INT(RND * 255) + 1
LET star10y = INT(RND * 302) + 1
REM generates old stars
LET oldstar1y = star1y
LET oldstar2y = star2y
LET oldstar3y = star3y
LET oldstar4y = star4y
LET oldstar5y = star5y
LET oldstar6y = star6y
LET oldstar7y = star7y
LET oldstar8y = star8y
LET oldstar9y = star9y
LET oldstar10y = star10y
LET oldstar1x = star1x
LET oldstar2x = star2x
LET oldstar3x = star3x
LET oldstar4x = star4x
LET oldstar5x = star5x
LET oldstar6x = star6x
LET oldstar7x = star7x
LET oldstar8x = star8x
LET oldstar9x = star9x
LET oldstar10x = star10x
LET particlelimit = 20
RETURN

playgame:
REM new game starts
GOSUB newvalues: REM sets new game values
DO
    LET a$ = UCASE$(INKEY$)
    IF level = 0 THEN GOSUB nextlevel
    GOSUB drawstars
    GOSUB drawhud
    GOSUB drawplayer
    GOSUB drawfire
    GOSUB drawparticle
    GOSUB calcenemy
    GOSUB timekeeper
    GOSUB inputter
    GOSUB objectcollision
    IF health <= 0 THEN GOTO gameover
    'GOSUB devconsole
    _LIMIT frames
LOOP
RETURN

gameover:
REM game over sequence
_DELAY 1
PSET (playerx, playery), &HFF000000
DRAW playership$
IF soundmode <> 1 THEN _SNDPLAY playerdeath: REM plays sound
REM draws particles
LET particle = 1
LET particlex = playerx
LET particley = playery
LET particlec = 1
LET particlelimit = particlelimit * 2
DO
	GOSUB drawparticle
	_LIMIT frames
LOOP UNTIL particle = 0
REM plays game over music
IF soundmode <> 1 THEN _SNDPLAY gameover
DO
	LET gameovertemp = _SNDPLAYING(gameover)
LOOP UNTIL gameovertemp = 0
CLS
_PRINTSTRING(1, 1), "GAME OVER!"
_PRINTSTRING(1, 15), "SCORE: " + STR$(score)
DO: LOOP UNTIL INKEY$ = " "
GOSUB highscores
GOTO endgame

highscores:
REM high score table
CLS
OPEN "polydata\highscores.ddf" FOR INPUT AS #1
INPUT #1, high1, name1$, high2, name2$, high3, name3$, high4, name4$, high5, name5$, high6, name6$, high7, name7$, high8, name8$, high9, name9$, high10, name10$ 
CLOSE #1
LET x = 10000
PRINT "HIGH SCORES!"
PRINT
LET highest$ = ""
LET lowest$ = ""
LET highest = 0
LET lowest = 0
DO
	IF high1 = x THEN 
		PRINT STR$(high1) + " - " + name1$
		IF high1 > 0 THEN LET lowest$ = name1$: LET lowest = high1
		IF highest$ = "" THEN LET highest$ = name1$: LET highest = high1
	END IF
	IF high2 = x THEN 
		PRINT STR$(high2) + " - " + name2$
		LET lowest$ = name2$: LET lowest = high2
		IF highest$ = "" THEN LET highest$ = name2$: LET highest = high2
	END IF
	IF high3 = x THEN 
		PRINT STR$(high3) + " - " + name3$
		LET lowest$ = name3$: LET lowest = high3
		IF highest$ = "" THEN LET highest$ = name3$: LET highest = high3
	END IF
	IF high4 = x THEN 
		PRINT STR$(high4) + " - " + name4$
		LET lowest$ = name4$: LET lowest = high4
		IF highest$ = "" THEN LET highest$ = name4$: LET highest = high4
	END IF
	IF high5 = x THEN 
		PRINT STR$(high5) + " - " + name5$
		LET lowest$ = name5$: LET lowest = high5
		IF highest$ = "" THEN LET highest$ = name5$: LET highest = high5
	END IF
	IF high6 = x THEN 
		PRINT STR$(high6) + " - " + name6$
		LET lowest$ = name6$: LET lowest = high6
		IF highest$ = "" THEN LET highest$ = name6$: LET highest = high6
	END IF
	IF high7 = x THEN 
		PRINT STR$(high7) + " - " + name7$
		LET lowest$ = name7$: LET lowest = high7
		IF highest$ = "" THEN LET highest$ = name7$: LET highest = high7
	END IF
	IF high8 = x THEN 
		PRINT STR$(high8) + " - " + name8$
		LET lowest$ = name8$: LET lowest = high8
		IF highest$ = "" THEN LET highest$ = name8$: LET highest = high8
	END IF
	IF high9 = x THEN 
		PRINT STR$(high9) + " - " + name9$
		LET lowest$ = name9$: LET lowest = high9
		IF highest$ = "" THEN LET highest$ = name9$: LET highest = high9
	END IF
	IF high10 = x THEN 
		PRINT STR$(high10) + " - " + name10$
		LET lowest$ = name10$: LET lowest = high10
		IF highest$ = "" THEN LET highest$ = name10$: LET highest = high10
	END IF
	LET x = x - 1
LOOP UNTIL x < 0
PRINT
IF score > lowest THEN
	IF soundmode <> 1 THEN _SNDPLAY highscore
	IF score > lowest THEN PRINT "HIGH SCORE!"
	IF score > highest THEN PRINT "HIGHEST SCORE!"
	INPUT "WHAT IS YOUR NAME? "; playername$
	IF high1 = lowest THEN LET high1 = score: LET name1$ = playername$: LET lowest = -1
	IF high2 = lowest THEN LET high2 = score: LET name2$ = playername$: LET lowest = -1
	IF high3 = lowest THEN LET high3 = score: LET name3$ = playername$: LET lowest = -1
	IF high4 = lowest THEN LET high4 = score: LET name4$ = playername$: LET lowest = -1
	IF high5 = lowest THEN LET high5 = score: LET name5$ = playername$: LET lowest = -1
	IF high6 = lowest THEN LET high6 = score: LET name6$ = playername$: LET lowest = -1
	IF high7 = lowest THEN LET high7 = score: LET name7$ = playername$: LET lowest = -1
	IF high8 = lowest THEN LET high8 = score: LET name8$ = playername$: LET lowest = -1
	IF high9 = lowest THEN LET high9 = score: LET name9$ = playername$: LET lowest = -1
	IF high10 = lowest THEN LET high10 = score: LET name10$ = playername$: LET lowest = -1
	OPEN "polydata\highscores.ddf" FOR OUTPUT AS #1
	WRITE #1, high1, name1$, high2, name2$, high3, name3$, high4, name4$, high5, name5$, high6, name6$, high7, name7$, high8, name8$, high9, name9$, high10, name10$ 
	CLOSE #1
	PRINT
END IF
PRINT "THANK YOU FOR PLAYING POLYBIUS!" 
PRINT "STAY OBEDIENT!"
DO: LOOP UNTIL INKEY$ = " "
RETURN

drawstars:
REM draws stars
REM erase old stars
IF oldstar1 <> star1y THEN
    REM erase star1
    LINE (oldstar1x, oldstar1y)-(oldstar1x, oldstar1y), &HFF000000
    LET oldstar1y = star1y
    LET oldstar1x = star1x
END IF
IF oldstar2y <> star2y THEN
    REM erase star2
    LINE (oldstar2x, oldstar2y)-(oldstar2x, oldstar2y), &HFF000000
    LET oldstar2y = star2y
    LET oldstar2x = star2x
END IF
IF oldstar3y <> star3y THEN
    REM erase star3
    LINE (oldstar3x, oldstar3y)-(oldstar3x, oldstar3y), &HFF000000
    LET oldstar3y = star3y
    LET oldstar3x = star3x
END IF
IF oldstar4y <> star4y THEN
    REM erase star4
    LINE (oldstar4x, oldstar4y)-(oldstar4x, oldstar4y), &HFF000000
    LET oldstar4y = star4y
    LET oldstar4x = star4x
END IF
IF oldstar5y <> star5y THEN
    REM erase star5
    LINE (oldstar5x, oldstar5y)-(oldstar5x, oldstar5y), &HFF000000
    LET oldstar5y = star5y
    LET oldstar5x = star5x
END IF
IF oldstar6y <> star6y THEN
    REM erase star6
    LINE (oldstar6x, oldstar6y)-(oldstar6x, oldstar6y), &HFF000000
    LET oldstar6y = star6y
    LET oldstar6x = star6x
END IF
IF oldstar7y <> star7y THEN
    REM erase star7
    LINE (oldstar7x, oldstar7y)-(oldstar7x, oldstar7y), &HFF000000
    LET oldstar7y = star7y
    LET oldstar7x = star7x
END IF
IF oldstar8y <> star8y THEN
    REM erase star8
    LINE (oldstar8x, oldstar8y)-(oldstar8x, oldstar8y), &HFF000000
    LET oldstar8y = star8y
    LET oldstar8x = star8x
END IF
IF oldstar9y <> star9y THEN
    REM erase star9
    LINE (oldstar9x, oldstar9y)-(oldstar9x, oldstar9y), &HFF000000
    LET oldstar9y = star9y
    LET oldstar9x = star9x
END IF
IF oldstar10y <> star10y THEN
    REM erase star10
    LINE (oldstar10x, oldstar10y)-(oldstar10x, oldstar10y), &HFF000000
    LET oldstar10y = star10y
    LET oldstar10x = star10x
END IF
REM draw stars
LINE (star1x, star1y)-(star1x, star1y), &HFFFCFCFC
LINE (star2x, star2y)-(star2x, star2y), &HFFFCFCFC
LINE (star3x, star3y)-(star3x, star3y), &HFFFCFCFC
LINE (star4x, star4y)-(star4x, star4y), &HFFFCFCFC
LINE (star5x, star5y)-(star5x, star5y), &HFFFCFCFC
LINE (star6x, star6y)-(star6x, star6y), &HFFFCFCFC
LINE (star7x, star7y)-(star7x, star7y), &HFFFCFCFC
LINE (star8x, star8y)-(star8x, star8y), &HFFFCFCFC
LINE (star9x, star9y)-(star9x, star9y), &HFFFCFCFC
LINE (star10x, star10y)-(star10x, star10y), &HFFFCFCFC
REM calc star movement
LET star1y = star1y + speed / 2
LET star2y = star2y + speed / 2
LET star3y = star3y + speed / 2
LET star4y = star4y + speed / 2
LET star5y = star5y + speed / 2
LET star6y = star6y + speed / 2
LET star7y = star7y + speed / 2
LET star8y = star8y + speed / 2
LET star9y = star9y + speed / 2
LET star10y = star10y + speed / 2
REM keep stars within window
IF star1y > 302 THEN LET star1y = 1: LET star1x = INT(RND * 255) + 1
IF star2y > 302 THEN LET star2y = 1: LET star2x = INT(RND * 255) + 1
IF star3y > 302 THEN LET star3y = 1: LET star3x = INT(RND * 255) + 1
IF star4y > 302 THEN LET star4y = 1: LET star4x = INT(RND * 255) + 1
IF star5y > 302 THEN LET star5y = 1: LET star5x = INT(RND * 255) + 1
IF star6y > 302 THEN LET star6y = 1: LET star6x = INT(RND * 255) + 1
IF star7y > 302 THEN LET star7y = 1: LET star7x = INT(RND * 255) + 1
IF star8y > 302 THEN LET star8y = 1: LET star8x = INT(RND * 255) + 1
IF star9y > 302 THEN LET star9y = 1: LET star9x = INT(RND * 255) + 1
IF star10y > 302 THEN LET star10y = 1: LET star10x = INT(RND * 255) + 1
RETURN

nextlevel:
REM increases level / game intro
REM generates level
LET level = level + 1
REM generates player ship location
LET levelship = playery
LET olevelship = levelship
REM plays music
IF soundmode <> 1 THEN
	IF level = 1 THEN _SNDPLAY gamestart
	IF level > 1 THEN _SNDPLAY newlevel
END IF
LET starend = 100: REM temp values
DO
	GOSUB drawstars
    REM erase old player ship
    IF olevelship <> levelship THEN
        PSET (255 / 2, olevelship), &HFF000000
        DRAW playership$
        LET olevelship = levelship
    END IF
    REM draw player ship
    IF health > 75 THEN PSET (255 / 2, levelship), &HFF54FC54
    IF health <= 75 AND health > 25 THEN PSET (255 / 2, levelship), &HFFFCFC54
    IF health <= 25 THEN PSET (255 / 2, levelship), &HFFFC5454
    DRAW playership$
    LET levelship = levelship - speed
    IF levelship < (302 / 2) THEN LET levelship = (302 / 2)
    REM display level text
    COLOR &HFF5454FC
    _PRINTSTRING ((255 / 2) - 8, 10), "LEVEL " + STR$(level)
    _LIMIT frames: REM fps limit
    REM detects when music ends
    IF soundmode <> 1 THEN
		IF level = 1 THEN starend = _SNDPLAYING(gamestart)
		IF level > 1 THEN starend = _SNDPLAYING(newlevel)
	ELSE
		_DELAY 0.01
		LET starend = starend - 1
	END IF
LOOP UNTIL starend = 0
CLS
RETURN


drawparticle:
REM draws explopsion particles
IF particle = 0 THEN RETURN: REM return for if particles arent needed
IF particle = 1 THEN
    REM sets up particles
    LET p1x = particlex
    LET p2x = particlex
    LET p3x = particlex
    LET p4x = particlex
    LET p1y = particley
    LET p2y = particley
    LET p3y = particley
    LET p4y = particley
    LET particle = 2
    LET particlemove = 0
    REM sets old values
    LET op1y = p1y
    LET op2y = p2y
    LET op3y = p3y
    LET op4y = p4y
    LET op1x = p1x
    LET op2x = p2x
    LET op3x = p3x
    LET op4x = p4x
END IF
IF particle = 2 THEN
    IF p1x <> op1x THEN
        REM erase old particle 1
        LINE (op1x, op1y)-(op1x, op1y), &HFF000000
        LET op1y = p1y
        LET op1x = p1x
    END IF
    IF p2x <> op2x THEN
        REM erase old particle 2
        LINE (op2x, op2y)-(op2x, op2y), &HFF000000
        LET op2y = p2y
        LET op2x = p2x
    END IF
    IF p3x <> op3x THEN
        REM erase old particle 3
        LINE (op3x, op3y)-(op3x, op3y), &HFF000000
        LET op3y = p3y
        LET op3x = p3x
    END IF
    IF p4x <> op4x THEN
        REM erase old particle 4
        LINE (op4x, op4y)-(op4x, op4y), &HFF000000
        LET op4y = p4y
        LET op4x = p4x
    END IF
    REM draws particles
    IF particlec = 1 THEN
		LINE (p1x, p1y)-(p1x, p1y), &HFFFC54FC
		LINE (p2x, p2y)-(p2x, p2y), &HFFFC54FC
		LINE (p3x, p3y)-(p3x, p3y), &HFFFC54FC
		LINE (p4x, p4y)-(p4x, p4y), &HFFFC54FC
    END IF
    IF particlec = 2 THEN
		LINE (p1x, p1y)-(p1x, p1y), &HFF54FCFC
		LINE (p2x, p2y)-(p2x, p2y), &HFF54FCFC
		LINE (p3x, p3y)-(p3x, p3y), &HFF54FCFC
		LINE (p4x, p4y)-(p4x, p4y), &HFF54FCFC
    END IF
    IF particlec = 3 THEN
		LINE (p1x, p1y)-(p1x, p1y), &HFFFCFC54
		LINE (p2x, p2y)-(p2x, p2y), &HFFFCFC54
		LINE (p3x, p3y)-(p3x, p3y), &HFFFCFC54
		LINE (p4x, p4y)-(p4x, p4y), &HFFFCFC54
    END IF
    REM sets particle values for next
    LET p1x = p1x - 1: LET p1y = p1y - 1
    LET p2x = p2x - 1: LET p2y = p2y + 1
    LET p3x = p3x + 1: LET p3y = p3y - 1
    LET p4x = p4x + 1: LET p4y = p4y + 1
    LET particlemove = particlemove + 1
    IF particlemove >= particlelimit THEN
        REM ends particles
        LET particle = 0
        LINE (p1x, p1y)-(p1x, p1y), &HFF000000
        LINE (p2x, p2y)-(p2x, p2y), &HFF000000
        LINE (p3x, p3y)-(p3x, p3y), &HFF000000
        LINE (p4x, p4y)-(p4x, p4y), &HFF000000
        LINE (op1x, op1y)-(op1x, op1y), &HFF000000
        LINE (op2x, op2y)-(op2x, op2y), &HFF000000
        LINE (op3x, op3y)-(op3x, op3y), &HFF000000
        LINE (op4x, op4y)-(op4x, op4y), &HFF000000
    END IF
END IF
RETURN

calcenemy:
REM calculates enemy sprites
FOR x = 1 TO level
    IF x = 1 THEN
        REM enemy 1
        IF etype1 = 0 THEN
            REM generates enemy type and location
            LET etype1 = INT(RND * 3) + 1
            LET enemy1x = INT(RND * 255)
            LET enemy1y = 0
            LET oldenemy1x = enemy1x
            LET oldenemy1y = enemy1y
        END IF
        REM sets draw values
        LET enemy1y = enemy1y + (speed / 4)
        LET drawenemyx = enemy1x
        LET drawenemyy = enemy1y
        LET drawetype = etype1
        LET olddrawenemyx = oldenemy1x
        LET olddrawenemyy = oldenemy1y
        GOSUB drawenemy
        LET oldenemy1y = enemy1y
        IF enemy1y > 302 THEN LET etype1 = 0: GOSUB hurtplayer
    END IF
    IF x = 2 THEN
        REM enemy 2
        IF etype2 = 0 THEN
            REM generates enemy type and location
            LET etype2 = INT(RND * 3) + 1
            LET enemy2x = INT(RND * 255)
            LET enemy2y = 0
            LET oldenemy2x = enemy2x
            LET oldenemy2y = enemy2y
        END IF
        REM sets draw values
        LET enemy2y = enemy2y + (speed / 4)
        LET drawenemyx = enemy2x
        LET drawenemyy = enemy2y
        LET drawetype = etype2
        LET olddrawenemyx = oldenemy2x
        LET olddrawenemyy = oldenemy2y
        GOSUB drawenemy
        LET oldenemy2y = enemy2y
        IF enemy2y > 302 THEN LET etype2 = 0: GOSUB hurtplayer
    END IF
    IF x = 3 THEN
        REM enemy 3
        IF etype3 = 0 THEN
            REM generates enemy type and location
            LET etype3 = INT(RND * 3) + 1
            LET enemy3x = INT(RND * 255)
            LET enemy3y = 0
            LET oldenemy3x = enemy3x
            LET oldenemy3y = enemy3y
        END IF
        REM sets draw values
        LET enemy3y = enemy3y + (speed / 4)
        LET drawenemyx = enemy3x
        LET drawenemyy = enemy3y
        LET drawetype = etype3
        LET olddrawenemyx = oldenemy3x
        LET olddrawenemyy = oldenemy3y
        GOSUB drawenemy
        LET oldenemy3y = enemy3y
        IF enemy3y > 302 THEN LET etype3 = 0: GOSUB hurtplayer
    END IF
    IF x = 4 THEN
        REM enemy 4
        IF etype4 = 0 THEN
            REM generates enemy type and location
            LET etype4 = INT(RND * 3) + 1
            LET enemy4x = INT(RND * 255)
            LET enemy4y = 0
            LET oldenemy4x = enemy4x
            LET oldenemy4y = enemy4y
        END IF
        REM sets draw values
        LET enemy4y = enemy4y + (speed / 4)
        LET drawenemyx = enemy4x
        LET drawenemyy = enemy4y
        LET drawetype = etype4
        LET olddrawenemyx = oldenemy4x
        LET olddrawenemyy = oldenemy4y
        GOSUB drawenemy
        LET oldenemy4y = enemy4y
        IF enemy4y > 302 THEN LET etype4 = 0: GOSUB hurtplayer
    END IF
    IF x = 5 THEN
        REM enemy 5
        IF etype5 = 0 THEN
            REM generates enemy type and location
            LET etype5 = INT(RND * 3) + 1
            LET enemy5x = INT(RND * 255)
            LET enemy5y = 0
            LET oldenemy5x = enemy5x
            LET oldenemy5y = enemy5y
        END IF
        REM sets draw values
        LET enemy5y = enemy5y + (speed / 4)
        LET drawenemyx = enemy5x
        LET drawenemyy = enemy5y
        LET drawetype = etype5
        LET olddrawenemyx = oldenemy5x
        LET olddrawenemyy = oldenemy5y
        GOSUB drawenemy
        LET oldenemy5y = enemy5y
        IF enemy5y > 302 THEN LET etype5 = 0: GOSUB hurtplayer
    END IF
    IF x = 6 THEN
        REM enemy 6
        IF etype6 = 0 THEN
            REM generates enemy type and location
            LET etype6 = INT(RND * 3) + 1
            LET enemy6x = INT(RND * 255)
            LET enemy6y = 0
            LET oldenemy6x = enemy6x
            LET oldenemy6y = enemy6y
        END IF
        REM sets draw values
        LET enemy6y = enemy6y + (speed / 4)
        LET drawenemyx = enemy6x
        LET drawenemyy = enemy6y
        LET drawetype = etype6
        LET olddrawenemyx = oldenemy6x
        LET olddrawenemyy = oldenemy6y
        GOSUB drawenemy
        LET oldenemy6y = enemy6y
        IF enemy6y > 302 THEN LET etype6 = 0: GOSUB hurtplayer
    END IF
    IF x = 7 THEN
        REM enemy 7
        IF etype7 = 0 THEN
            REM generates enemy type and location
            LET etype7 = INT(RND * 3) + 1
            LET enemy7x = INT(RND * 255)
            LET enemy7y = 0
            LET oldenemy7x = enemy7x
            LET oldenemy7y = enemy7y
        END IF
        REM sets draw values
        LET enemy7y = enemy7y + (speed / 4)
        LET drawenemyx = enemy7x
        LET drawenemyy = enemy7y
        LET drawetype = etype7
        LET olddrawenemyx = oldenemy7x
        LET olddrawenemyy = oldenemy7y
        GOSUB drawenemy
        LET oldenemy7y = enemy7y
        IF enemy7y > 302 THEN LET etype7 = 0: GOSUB hurtplayer
    END IF
    IF x = 8 THEN
        REM enemy 8
        IF etype8 = 0 THEN
            REM generates enemy type and location
            LET etype8 = INT(RND * 3) + 1
            LET enemy8x = INT(RND * 255)
            LET enemy8y = 0
            LET oldenemy8x = enemy8x
            LET oldenemy8y = enemy8y
        END IF
        REM sets draw values
        LET enemy8y = enemy8y + (speed / 4)
        LET drawenemyx = enemy8x
        LET drawenemyy = enemy8y
        LET drawetype = etype8
        LET olddrawenemyx = oldenemy8x
        LET olddrawenemyy = oldenemy8y
        GOSUB drawenemy
        LET oldenemy8y = enemy8y
        IF enemy8y > 302 THEN LET etype8 = 0: GOSUB hurtplayer
    END IF
    IF x = 9 THEN
        REM enemy 9
        IF etype9 = 0 THEN
            REM generates enemy type and location
            LET etype9 = INT(RND * 3) + 1
            LET enemy9x = INT(RND * 255)
            LET enemy9y = 0
            LET oldenemy9x = enemy9x
            LET oldenemy9y = enemy9y
        END IF
        REM sets draw values
        LET enemy9y = enemy9y + (speed / 4)
        LET drawenemyx = enemy9x
        LET drawenemyy = enemy9y
        LET drawetype = etype9
        LET olddrawenemyx = oldenemy9x
        LET olddrawenemyy = oldenemy9y
        GOSUB drawenemy
        LET oldenemy9y = enemy9y
        IF enemy9y > 302 THEN LET etype9 = 0: GOSUB hurtplayer
    END IF
    IF x = 10 THEN
        REM enemy 10
        IF etype10 = 0 THEN
            REM generates enemy type and location
            LET etype10 = INT(RND * 3) + 1
            LET enemy10x = INT(RND * 255)
            LET enemy10y = 0
            LET oldenemy10x = enemy10x
            LET oldenemy10y = enemy10y
        END IF
        REM sets draw values
        LET enemy10y = enemy10y + (speed / 4)
        LET drawenemyx = enemy10x
        LET drawenemyy = enemy10y
        LET drawetype = etype10
        LET olddrawenemyx = oldenemy10x
        LET olddrawenemyy = oldenemy10y
        GOSUB drawenemy
        LET oldenemy10y = enemy10y
        IF enemy10y > 302 THEN LET etype10 = 0: GOSUB hurtplayer
    END IF
NEXT x
RETURN

hurtplayer:
REM hurts player and removes old enemy
IF drawetype = 1 THEN
    REM remove square
    PSET (drawenemyx, drawenemyy), &HFF000000
    DRAW square$
    LET health = health - 3
END IF
IF drawetype = 2 THEN
    REM remove triangle
    PSET (drawenemyx, drawenemyy), &HFF000000
    DRAW triangle$
    LET health = health - 5
END IF
IF drawetype = 3 THEN
    REM remove diamond
    PSET (drawenemyx, drawenemyy), &HFF000000
    DRAW diamond$
    LET health = health - 10
END IF
REM play sound
IF soundmode <> 1 THEN _SNDPLAY playerhit
IF health > 0 THEN
	REM particles
	LET particle = 1
	LET particlex = drawenemyx
	LET particley = drawenemyy
	LET particlec = drawetype
	CLS
END IF
RETURN

drawenemy:
REM draws enemy sprites
IF drawetype = 1 THEN
    REM square
    IF olddrawenemyy <> drawenemyy THEN
        REM erases old square
        PSET (olddrawenemyx, olddrawenemyy), &HFF000000
        DRAW square$
    END IF
    PSET (drawenemyx, drawenemyy), &HFFFC54FC
    DRAW square$
END IF
IF drawetype = 2 THEN
    REM triangle
    IF olddrawenemyy <> drawenemyy THEN
        REM erases old triangle
        PSET (olddrawenemyx, olddrawenemyy), &HFF000000
        DRAW triangle$
    END IF
    PSET (drawenemyx, drawenemyy), &HFF54FCFC
    DRAW triangle$
END IF
IF drawetype = 3 THEN
    REM diamond
    IF olddrawenemyy <> drawenemyy THEN
        REM erases old diamond
        PSET (olddrawenemyx, olddrawenemyy), &HFF000000
        DRAW diamond$
    END IF
    PSET (drawenemyx, drawenemyy), &HFFFCFC54
    DRAW diamond$
END IF
RETURN

errorhandler:
REM handles errors
PRINT "ERROR: "; ERR
PRINT "LINE: "; _ERRORLINE
IF ERR = 666 THEN PRINT "DATA FOLDER OR VAME SAVE NOT FOUND"
PRINT "POLYBIUS WILL NOW CLOSE"
END

drawfire:
REM draws bullets
IF fire = 0 THEN RETURN: REM return if no bullets fired
IF fire = 1 THEN
    REM FIRE BULLET
    IF oldfirey <> firey THEN
        REM erases old bullet
        PSET (firex, oldfirey), &HFF000000
        DRAW bullet$
        LET oldfirey = firey
    END IF
    IF health > 75 THEN PSET (firex, firey), &HFF54FC54
	IF health <= 75 AND health > 25 THEN PSET (firex, firey), &HFFFCFC54
	IF health <= 25 THEN PSET (firex, firey), &HFFFC5454
    DRAW bullet$
    LET firey = firey - speed
    IF firey < 12 THEN
        REM ends fire
        LET fire = 0
        PSET (firex, oldfirey), &HFF000000
        DRAW bullet$
    END IF
END IF
RETURN

objectcollision:
REM object collision
REM player window boundaries
IF playerx < -4 THEN LET playerx = -4
IF playerx > 249 THEN LET playerx = 249
REM bullet and enemy
FOR x = 1 TO level
    IF x = 1 THEN
        IF firex > enemy1x - 10 AND firex < enemy1x + 10 THEN
            IF firey <= enemy1y THEN
                IF etype1 = 1 THEN LET score = score + 1
                IF etype1 = 2 THEN LET score = score + 3
                IF etype1 = 3 THEN LET score = score + 5
                LET fire = 0
                LET blowx = oldenemy1x
                LET blowy = oldenemy1y
                LET blowtype = etype1
                LET etype1 = 0
                GOSUB blowenemy
            END IF
        END IF
    END IF
    IF x = 2 THEN
        IF firex > enemy2x - 10 AND firex < enemy2x + 10 THEN
            IF firey <= enemy2y THEN
                IF etype2 = 1 THEN LET score = score + 1
                IF etype2 = 2 THEN LET score = score + 3
                IF etype2 = 3 THEN LET score = score + 5
                LET fire = 0
                LET blowx = oldenemy2x
                LET blowy = oldenemy2y
                LET blowtype = etype2
                LET etype2 = 0
                GOSUB blowenemy
            END IF
        END IF
    END IF
    IF x = 3 THEN
        IF firex > enemy3x - 10 AND firex < enemy3x + 10 THEN
            IF firey <= enemy3y THEN
                IF etype3 = 1 THEN LET score = score + 1
                IF etype3 = 2 THEN LET score = score + 3
                IF etype3 = 3 THEN LET score = score + 5
                LET fire = 0
                LET blowx = oldenemy3x
                LET blowy = oldenemy3y
                LET blowtype = etype3
                LET etype3 = 0
                GOSUB blowenemy
            END IF
        END IF
    END IF
    IF x = 4 THEN
        IF firex > enemy4x - 10 AND firex < enemy4x + 10 THEN
            IF firey <= enemy4y THEN
                IF etype4 = 1 THEN LET score = score + 1
                IF etype4 = 2 THEN LET score = score + 3
                IF etype4 = 3 THEN LET score = score + 5
                LET fire = 0
                LET blowx = oldenemy4x
                LET blowy = oldenemy4y
                LET blowtype = etype4
                LET etype4 = 0
                GOSUB blowenemy
            END IF
        END IF
    END IF
    IF x = 5 THEN
        IF firex > enemy5x - 10 AND firex < enemy5x + 10 THEN
            IF firey <= enemy5y THEN
                IF etype5 = 1 THEN LET score = score + 1
                IF etype5 = 2 THEN LET score = score + 3
                IF etype5 = 3 THEN LET score = score + 5
                LET fire = 0
                LET blowx = oldenemy5x
                LET blowy = oldenemy5y
                LET blowtype = etype5
                LET etype5 = 0
                GOSUB blowenemy
            END IF
        END IF
    END IF
    IF x = 6 THEN
        IF firex > enemy6x - 10 AND firex < enemy6x + 10 THEN
            IF firey <= enemy6y THEN
                IF etype6 = 1 THEN LET score = score + 1
                IF etype6 = 2 THEN LET score = score + 3
                IF etype6 = 3 THEN LET score = score + 5
                LET fire = 0
                LET blowx = oldenemy6x
                LET blowy = oldenemy6y
                LET blowtype = etype6
                LET etype6 = 0
                GOSUB blowenemy
            END IF
        END IF
    END IF
    IF x = 7 THEN
        IF firex > enemy7x - 10 AND firex < enemy7x + 10 THEN
            IF firey <= enemy7y THEN
                IF etype7 = 1 THEN LET score = score + 1
                IF etype7 = 2 THEN LET score = score + 3
                IF etype7 = 3 THEN LET score = score + 5
                LET fire = 0
                LET blowx = oldenemy7x
                LET blowy = oldenemy7y
                LET blowtype = etype7
                LET etype7 = 0
                GOSUB blowenemy
            END IF
        END IF
    END IF
    IF x = 8 THEN
        IF firex > enemy8x - 10 AND firex < enemy8x + 10 THEN
            IF firey <= enemy8y THEN
                IF etype8 = 1 THEN LET score = score + 1
                IF etype8 = 2 THEN LET score = score + 3
                IF etype8 = 3 THEN LET score = score + 5
                LET fire = 0
                LET blowx = oldenemy8x
                LET blowy = oldenemy8y
                LET blowtype = etype8
                LET etype8 = 0
                GOSUB blowenemy
            END IF
        END IF
    END IF
    IF x = 9 THEN
        IF firex > enemy9x - 10 AND firex < enemy9x + 10 THEN
            IF firey <= enemy9y THEN
                IF etype9 = 1 THEN LET score = score + 1
                IF etype9 = 2 THEN LET score = score + 3
                IF etype9 = 3 THEN LET score = score + 5
                LET fire = 0
                LET blowx = oldenemy9x
                LET blowy = oldenemy9y
                LET blowtype = etype9
                LET etype9 = 0
                GOSUB blowenemy
            END IF
        END IF
    END IF
    IF x = 10 THEN
        IF firex > enemy10x - 10 AND firex < enemy10x + 10 THEN
            IF firey <= enemy10y THEN
                IF etype10 = 1 THEN LET score = score + 1
                IF etype10 = 2 THEN LET score = score + 3
                IF etype10 = 3 THEN LET score = score + 5
                LET fire = 0
                LET blowx = oldenemy10x
                LET blowy = oldenemy10y
                LET blowtype = etype10
                LET etype10 = 0
                GOSUB blowenemy
            END IF
        END IF
    END IF
NEXT x
RETURN

blowenemy:
REM blows up enemy
IF blowtype = 1 THEN
    REM erases old square
    PSET (blowx, blowy), &HFF000000
    DRAW square$
END IF
IF blowtype = 2 THEN
    REM erases old triangle
    PSET (blowx, blowy), &HFF000000
    DRAW triangle$
END IF
IF blowtype = 3 THEN
    REM erases old diamond
    PSET (blowx, blowy), &HFF000000
    DRAW diamond$
END IF
REM erases bullet
PSET (firex, oldfirey), &HFF000000
DRAW bullet$
REM plays hit sound
IF soundmode <> 1 THEN _SNDPLAY hit
LET particle = 1
LET particlex = blowx
LET particley = blowy
LET particlec = blowtype
RETURN

devconsole:
REM developer console
_DEST _CONSOLE
REM console lines go here
_DEST 0
RETURN

drawplayer:
REM draws player ship
IF oldplayerx <> playerx THEN
    REM removes old player ship
    PSET (oldplayerx, oldplayery), &HFF000000
    DRAW playership$
    LET oldplayerx = playerx
END IF
REM draws new ship
IF health > 75 THEN PSET (playerx, playery), &HFF54FC54
IF health <= 75 AND health > 25 THEN PSET (playerx, playery), &HFFFCFC54
IF health <= 25 THEN PSET (playerx, playery), &HFFFC5454
DRAW playership$
RETURN

drawhud:
REM draws the game hud (score and health)
COLOR &HFF5454FC
_PRINTSTRING (1, 1), "HEALTH: " + STR$(health)
_PRINTSTRING (180, 1), "SCORE: " + STR$(score)
REM draws seperator lines
LINE (0, 13)-(302, 13), &HFF5454FC
LINE (0, 15)-(302, 15), &HFF5454FC
RETURN

inputter:
REM game input
IF a$ = "Q" THEN GOTO endgame
IF _KEYDOWN(19712) THEN LET playerx = playerx + speed
IF _KEYDOWN(19200) THEN LET playerx = playerx - speed
IF a$ = " " THEN
    IF fire = 0 THEN
        LET fire = 1
        LET firex = playerx + 5
        LET firey = playery + 10
        LET oldfirex = firex
        LET oldfirey = firey
        IF soundmode <> 1 THEN _SNDPLAY bulletfire
    END IF
END IF
RETURN

titlescreen:
REM displays titlescreen
_PUTIMAGE (0, 0)-(255, 302), titledisplay
IF soundmode <> 1 THEN _SNDPLAY titlemusic
REM displays occasional flashes after music
DO
    IF _SNDPLAYING(titlemusic) = 0 THEN
        IF titletemp = 0 THEN LET titletemp = ctime + 20
        IF titletemp <= ctime THEN
            IF soundmode <> 1 THEN _SNDLOOP titlesfx
            FOR t = 1 TO 10
                _PUTIMAGE (0, 0)-(255, 302), titledisplay2
                _DELAY 0.1
                _PUTIMAGE (0, 0)-(255, 302), titledisplay
                _DELAY 0.1
            NEXT t
            IF soundmode <> 1 THEN _SNDSTOP titlesfx
            LET titletemp = ctime + 20
        END IF
    END IF
    GOSUB timekeeper
LOOP WHILE INKEY$ = ""
CLS
IF soundmode <> 1 THEN _SNDSTOP titlemusic
RETURN

endgame:
REM unloads all files from memory and quits game
REM unloads images
_FREEIMAGE titledisplay
_FREEIMAGE titledisplay2
REM unloads audio
_SNDCLOSE titlemusic
_SNDCLOSE bulletfire
_SNDCLOSE titlesfx
_SNDCLOSE hit
_SNDCLOSE gamestart
_SNDCLOSE gameover
_SNDCLOSE newlevel
_SNDCLOSE playerhit
_SNDCLOSE playerdeath
_SNDCLOSE highscore
REM Danni Pond
SYSTEM
