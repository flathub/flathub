# Copyright (c) 2017-2025, Md Imam Hossain (emamhd at gmail dot com)
# see LICENSE.txt for details

from subprocess import Popen, PIPE

GNGEO_AUDIO_SAMPLE_RATES = ['default', '8000', '11025', '16000', '22050', '24000', '44100']
GNGEO_VIDEO_EFFECTS = ['default', 'hq2x', 'hq3x', 'lq2x', 'lq3x', 'scanline', 'scanline50', 'scale2x50', 'scale2x75', 'disabled']
GNGEO_BLITTERS = ['default', 'soft', 'yuv']
GNGEO_GAME_COUNTRIES = {'Default':'default', 'Japan':'japan', 'Asia':'asia', 'USA':'usa', 'Europe':'europe'}
GNGEO_VIDEO_SCALINGS = ['default', '2', '3']
GNGEO_KEYBOARD_KEYCODES = {'A': 'K97', 'B': 'K98', 'C': 'K99', 'D': 'K100', 'E': 'K101', 'F': 'K102', 'G': 'K103', 'H': 'K104', 'I': 'K105', 'J': 'K106', 'K': 'K107', 'L': 'K108', 'M': 'K109', 'N': 'K110', 'O': 'K111', 'P': 'K112', 'Q': 'K113', 'R': 'K114', 'S': 'K115', 'T': 'K116', 'U': 'K117', 'V': 'K118', 'W': 'K119', 'X': 'K120', 'Y': 'K121', 'Z': 'K122', '0': 'K256', 'Numeric keypad 1': 'K257', 'Numeric keypad 2': 'K258', 'Numeric keypad 3': 'K259', 'Numeric keypad 4': 'K260', 'Numeric keypad 5': 'K261', 'Numeric keypad 6': 'K262', 'Numeric keypad 7': 'K263', 'Numeric keypad 8': 'K264', 'Numeric keypad 9': 'K265', 'Numeric keypad PERIOD': 'K266', 'Numeric keypad DIVIDE': 'K267', 'Numeric keypad MULTIPLY': 'K268', 'Numeric keypad MINUS': 'K269', 'Numeric keypad PLUS': 'K270', 'Numeric keypad ENTER': 'K271', 'Numeric keypad EQUALS': 'K272', 'UP': 'K273', 'DOWN': 'K274', 'RIGHT': 'K275', 'LEFT': 'K276', 'Insert': 'K277', 'Home': 'K278', 'End': 'K279', 'Page up': 'K280', 'Page down': 'K281', 'Left bracket': 'K91', 'Backslash': 'K92', 'Right bracket': 'K93', 'Caret': 'K94', 'Underscore': 'K95', 'Backquote': 'K96', 'Exclamation': 'K33', 'QUOTEDBL': 'K34', 'Hash': 'K35', 'Dollar': 'K36', 'Ampersand': 'K38', 'Quote': 'K39', 'Left parenthesis': 'K40', 'Right parenthesis': 'K41', 'Asterish': 'K42', 'Plus': 'K43', 'Comma': 'K44', 'Minus': 'K45', 'Period': 'K46', 'Slash': 'K47', '0': 'K48', '1': 'K49', '2': 'K50', '3': 'K51', '4': 'K52', '5': 'K53', '6': 'K54', '7': 'K55', '8': 'K56', '9': 'K57', 'Colon': 'K58', 'Semicolon': 'K59', 'Less than': 'K60', 'Equals': 'K61', 'Greater than': 'K62', 'Question mark': 'K63', 'AT': 'K64'}
GNGEO_GAMEPAD1_KEYCODES = {'1': 'J0B0', '2': 'J0B1', '3': 'J0B2', '4': 'J0B3', '5': 'J0B4', '6': 'J0B5', '7': 'J0B6', '8': 'J0B7', '9': 'J0B8', '10': 'J0B9', '11': 'J0B10', '12': 'J0B11', 'Up': 'J0a1', 'Down': 'J0a1', 'Left': 'J0A0', 'Right': 'J0A0'}
GNGEO_GAMEPAD2_KEYCODES = {'1': 'J1B0', '2': 'J1B1', '3': 'J1B2', '4': 'J1B3', '5': 'J1B4', '6': 'J1B5', '7': 'J1B6', '8': 'J1B7', '9': 'J1B8', '10': 'J1B9', '11': 'J1B10', '12': 'J1B11', 'Up': 'J1a1', 'Down': 'J1a1', 'Left': 'J1A0', 'Right': 'J1A0'}
GNGEO_CONTROLLERS = ['Keyboard', 'Gamepad 1', 'Gamepad 2']
GNGEO_ROMS_LIST = {'2020bb': '2020 Super Baseball (set 1)', '2020bbh': '2020 Super Baseball (set 2)', '2020bba': '2020bba', '3countb': '3 Count Bout', 'sonicwi2': 'Aero Fighters 2', 'sonicwi3': 'Aero Fighters 3', 'aodk': 'Aggressors of Dark Kombat', 'alpham2': 'Alpha Mission II', 'androdun': 'Andro Dunos', 'aof2a': 'aof2a', 'aof': 'Art of Fighting', 'aof2': 'Art of Fighting 2', 'aof3': 'Art of Fighting 3 - The Path of the Warrior', 'bakatono': 'Bakatonosama Mahjong Manyuki', 'b2b': 'Bang Bang Busters', 'bangbead': 'Bang Bead', 'bstars2': 'Baseball Stars 2', 'bstars': 'Baseball Stars Professional', 'flipshot': 'Battle Flip Shot', 'blazstar': 'Blazing Star', 'bjourney': 'Blue\'s Journey', 'breakers': 'Breakers', 'breakrev': 'Breakers Revenge', 'burningf': 'Burning Fight (set 1)', 'burningh': 'Burning Fight (set 2)', 'ctomaday': 'Captain Tomaday', 'marukodq': 'Chibi Marukochan Deluxe Quiz', 'crsword': 'Crossed Swords', 'crsword2': 'Crossed Swords 2', 'cthd2003': 'Crouching Tiger Hidden Dragon', 'cthd2003sp': 'Crouching Tiger Hidden Dragon Plus', 'cyberlip': 'Cyber-Lip', 'doubledr': 'Double Dragon (Neo-Geo)', 'eightman': 'Eight Man', 'fatfury1': 'Fatal Fury - King of Fighters', 'fatfury2': 'Fatal Fury 2', 'fatfury3': 'Fatal Fury 3 - Road to the Final Victory', 'fatfursp': 'Fatal Fury Special', 'fightfev': 'Fight Fever', 'fightfva': 'fightfva', 'fbfrenzy': 'Football Frenzy', 'galaxyfg': 'Galaxy Fight - Universal Warriors', 'pbobble': 'game pbobble MVS', 'ganryu': 'Ganryu', 'garoup': 'Garou - Mark of the Wolves (prototype)', 'garou': 'Garou - Mark of the Wolves (set 1)', 'gpilots': 'Ghost Pilots', 'ghostlop': 'ghostlop', 'goalx3': 'Goal! Goal! Goal!', 'gururin': 'Gururin', 'jockeygp': 'Jockey Grandprix', 'janshin': 'Jyanshin Densetsu - Quest of Jongmaster', 'kabukikl': 'Kabuki Klash - Far East of Eden', 'karnovr': 'Karnov\'s Revenge', 'kotm': 'King of the Monsters', 'kotm2': 'King of the Monsters 2 - The Next Thing', 'kizuna': 'Kizuna Encounter - Super Tag Battle', 'kof2002': 'KoF 2002', 'kof2003b': 'KoF 2003', 'kof2003': 'KoF 2003', 'kof2003a': 'KoF 2003a', 'kof95a': 'kof95a', 'kof96h': 'kof96h', 'kof97a': 'kof97a', 'kof97pls': 'kof97pls', 'kof98k': 'kof98k', 'kof98n': 'kof98n', 'kotmh': 'kotmh', 'lresort': 'Last Resort', 'lastblda': 'lastblda', 'lbowling': 'League Bowling', 'legendos': 'Legend of Success Joe', 'magdrop2': 'Magical Drop II', 'magdrop3': 'Magical Drop III', 'maglord': 'Magician Lord (set 1)', 'maglordh': 'Magician Lord (set 2)', 'mahretsu': 'Mahjong Kyoretsuden', 'matrim': 'Matrimelee', 'mslug': 'Metal Slug - Super Vehicle-001', 'mslug2': 'Metal Slug 2 - Super Vehicle-001', 'mslug3': 'Metal Slug 3', 'mslug3n': 'Metal Slug 3 (Encrypted GFX)', 'mslug4': 'Metal Slug 4(non encrypted)', 'mslug5': 'Metal Slug 5', 'mslugx': 'Metal Slug X - Super Vehicle-001', 'minasan': 'Minnasanno Okagesamadesu', 'miexchng': 'Money Puzzle Exchanger', 'mutnat': 'Mutation Nation', 'nam1975': 'NAM-1975', 'ncombata': 'ncombata', 'neobombe': 'Neo Bomberman', 'neodrift': 'Neo Drift Out - New Technology', 'neomrdo': 'Neo Mr. Do!', 'neonopon': 'Neo No Panepon', 'turfmast': 'Neo Turf Masters', 'neocup98': 'Neo-Geo Cup \'98 - The Road to the Victory', 'neopong': 'Neopong 1.1', 'nitd': 'Nightmare in the Dark', 'ncombat': 'Ninja Combat', 'ncommand': 'Ninja Commando', 'ninjamas': 'Ninja Master\'s - haoh-ninpo-cho', 'ragnagrd': 'Operation Ragnagard', 'overtop': 'Over Top', 'panicbom': 'Panic Bomber', 'pbobblna': 'pbobblna', 'pgoal': 'Pleasure Goal', 'pochi': 'Pochi and Nyaa', 'popbounc': 'Pop \'n Bounce', 'pim': 'Power Instinct - Matrimelee', 'pspikes2': 'Power Spikes II', 'preisle2': 'Prehistoric Island 2', 'pulstar': 'Pulstar', 'pbobblen': 'Puzzle Bobble', 'pbobbl2n': 'Puzzle Bobble 2', 'puzzledp': 'Puzzle De Pon', 'puzzldpr': 'Puzzle De Pon R', 'joyjoy': 'Puzzled', 'quizdais': 'Quiz Daisousa Sen - The Last Count Down', 'quizkof': 'Quiz King of Fighters', 'quizdai2': 'Quiz Meintantei Neo Geo - Quiz Daisousa Sen Part 2', 'rotd': 'Rage of the Dragons', 'rotd': 'Rage of the Dragons', 'rodd': 'Rage of the dragons (decrypted)', 'rbff2a': 'rbff2a', 'rbff1': 'Real Bout Fatal Fury', 'rbff2': 'Real Bout Fatal Fury 2 - The Newcomers', 'rbffspec': 'Real Bout Fatal Fury Special', 'ridhero': 'Riding Hero (set 1)', 'ridheroh': 'Riding Hero (set 2)', 'roboarmy': 'Robo Army', 'roboarma': 'roboarma', 'samsho3a': 'samsho3a', 'samsho': 'Samurai Shodown', 'samsho5': 'Samurai Shodown 5', 'samsho5sp': 'Samurai Shodown 5 Special', 'samsho2': 'Samurai Shodown II', 'samsho3': 'Samurai Shodown III', 'samsho4': 'Samurai Shodown IV - Amakusa\'s Revenge', 'samsh5sp': 'Samurai Shodown V Special', 'savagere': 'Savage Reign', 'sengoku': 'Sengoku', 'sengokh': 'Sengoku', 'sengoku2': 'Sengoku 2', 'sengoku3': 'sengoku3', 'shocktro': 'Shock Troopers', 'shocktr2': 'Shock Troopers - 2nd Squad', 'shocktrj': 'Shock Troopers (Japan)', 'shocktra': 'shocktra', 'svcplus': 'SNK vs Capcom - SVC Chaos', 'svc': 'SNK Vs. CAPCOM', 'socbrawl': 'Soccer Brawl', 'spinmast': 'Spinmaster', 'ssvsp': 'ssvsp', 'stakwin': 'Stakes Winner', 'stakwin2': 'Stakes Winner 2', 'strhoop': 'Street Hoop', 's1945p': 'Striker 1945 Plus', 'sbp': 'Super Bubble Pop', 'sdodgeb': 'Super Dodge Ball', 'ssideki': 'Super Sidekicks', 'ssideki2': 'Super Sidekicks 2 - The World Championship', 'ssideki3': 'Super Sidekicks 3 - The Next Glory', 'mosyougi': 'Syougi No Tatsujin - Master of Syougi', 'tws96': 'Tecmo World Soccer \'96', 'irrmaze': 'The Irritating Maze', 'kof2k2': 'The King of Fighter 2002', 'kof2k2pls': 'The King of Fighter 2002 plus', 'kof94': 'The King of Fighters \'94', 'kof95': 'The King of Fighters \'95', 'kof96': 'The King of Fighters \'96', 'kof97': 'The King of Fighters \'97', 'kof98': 'The King of Fighters \'98 - The Slugfest', 'kof99': 'The King of Fighters \'99 - Millennium Battle', 'kof99p': 'The King of Fighters \'99 - Millennium Battle (prototype)', 'kof10thu': 'The King of Fighters 10th Unique hack', 'kof2000': 'The King of Fighters 2000', 'kof2000n': 'The King of Fighters 2000 (Encrypted GFX)', 'kof2k1rp': 'The King of Fighters 2001 plus', 'kof2k4es': 'The King of Fighters 2004 hack', 'kog': 'The King of Gladiator', 'kof2001': 'The King ofFighter 2001', 'kof2001n': 'The King ofFighter 2001 (non encrypted)', 'lastblad': 'The Last Blade', 'lastbld2': 'The Last Blade 2', 'superspy': 'The Super Spy', 'ssideki4': 'The Ultimate 11', 'trally': 'Thrash Rally', 'tophuntr': 'Top Hunter - Roddy & Cathy', 'tpgolf': 'Top Player\'s Golf', 'tophunta': 'tophunta', 'twinspri': 'Twinkle Star Sprites', 'viewpoin': 'Viewpoint', 'gowcaizr': 'Voltage Fighter - Gowcaizer', 'wakuwak7': 'Waku Waku 7', 'wh1h': 'wh1h', 'wjammers': 'Windjammers', 'wh1': 'World Heroes', 'wh2': 'World Heroes 2', 'wh2j': 'World Heroes 2 Jet', 'whp': 'World Heroes Perfect', 'zedblade': 'Zed Blade', 'zintrckb': 'zintrckb', 'zupapa': 'Zupapa'}

class GnGeo():

    def __init__(self):

        self.version = 0.0
        self.copyright = ''
        self.exe = ''
        self.exe_dir = ''
        self.datafile = ''
        self.scale = 'default'
        self.autoframeskip = True
        self.interpolation = True
        self.blitter = 'default'
        self.effect = 'default'
        self.hwsurface = True
        self.vsync = False
        self.country = 'default'
        self.fullscreen = False
        self.joystick = True
        self.samplerate = 'default'
        self.sound = True
        self.p1control = ''
        self.p2control = ''

    def set_exe(self, _exe_path, _data_path, _exe_dir):

        command_buffer = 'LD_LIBRARY_PATH=\"$LD_LIBRARY_PATH\":' + '\"' + _exe_dir + '\" ' + '\"' + _exe_path + '\"' + ' --version'

        command = Popen(command_buffer, shell=True, stdout=PIPE, stderr=PIPE, text=True)

        command_output, stderr = command.communicate()

        if len(stderr) == 0:

            for command_output_line in command_output.split('\n'):

                if command_output_line.split(' ')[0] == 'Gngeo':

                    self.version = command_output_line.split(' ')[1]

                elif command_output_line.split(' ')[0] == 'Copyright':

                    self.copyright = command_output_line

            self.exe = _exe_path
            self.datafile = _data_path
            self.exe_dir = _exe_dir

            return 0

        else:

            return 1

    def run_game(self, _rom, _rom_path):

        command_buffer = ""
        command_arguments = ""

        if (self.scale != 'default' and self.scale != 'disabled'):

            command_arguments += ' --scale=' + self.scale

        if (self.autoframeskip == True):

            command_arguments += ' --autoframeskip'

        else:

            command_arguments += ' --no-autoframeskip'

        if (self.interpolation == True):

            command_arguments += ' --interpolation'

        else:

            command_arguments += ' --no-interpolation'

        if (self.blitter == 'default'):

            command_arguments += ' --blitter=soft'

        else:
        
            command_arguments += ' --blitter=' + self.blitter
            

        if (self.effect == 'default'):

            command_arguments += ' --effect=hq2x'

        else:

            command_arguments += ' --effect=' + self.effect

        if (self.hwsurface == True):

            command_arguments +=  ' --hwsurface'

        else:

            command_arguments += ' --no-hwsurface'

        if (self.vsync == True):

            command_arguments += ' --vsync'

        else:

            command_arguments += ' --no-vsync'

        if (self.country != 'default'):

            command_arguments += ' --country=' + self.country

        if (self.fullscreen == True):

            command_arguments += ' --fullscreen'

        else:

            command_arguments += ' --no-fullscreen'

        if (self.joystick == True):

            command_arguments += ' --joystick'

        else:

            command_arguments += ' --no-joystick'

        if (self.samplerate != 'default' and self.sound == True):

            command_arguments += ' --samplerate=' + self.samplerate

        if (self.sound == True):

            command_arguments += ' --sound'

        else:

            command_arguments += ' --no-sound'

        if (self.p1control != ''):

            command_arguments += ' --p1control=' + self.p1control

        if (self.p2control != ''):

            command_arguments += ' --p2control=' + self.p2control

        command_buffer = 'LD_LIBRARY_PATH=\"$LD_LIBRARY_PATH\":' + '\"' + self.exe_dir + '\" ' + '\"' + self.exe + '\"' + ' --datafile=' + '\"' + self.datafile + '\"' + ' --rompath=' + '\"' + _rom_path + '\"' + command_arguments + ' ' + _rom

        print('Shopne Arcade: loading game ...\n', command_buffer, '\n')

        command = Popen(command_buffer, shell=True, stdout=PIPE, stderr=PIPE, text=True)

        command_output, stderr = command.communicate()

        print('Shopne Arcade: GnGeo output\n', command_output, '\n')

        if len(stderr) == 0:
            return 0
        return 1
