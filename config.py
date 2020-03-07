# 
#### Author: Hamad Al Marri <hamad.s.almarri@gmail.com>
#### Date: Feb 11th, 2020
#
#	This program is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
#
#
#

# from pathlib import Path
import os


# path of gamma files
gamma_path = os.path.dirname(os.path.realpath(__file__))


# default .ui layout file
ui_file = "builder.ui"

# default css style file
# style_file = "default.css"
style_file = "chocolate-icecream-solid.css"

# style scheme for srource view style
# usually sourceview style files are in 
# ~/.local/share/gtksourceview-4/styles
style_scheme = "chocolateicecream"



# from config_paths_and_settings can get 
# all important paths
config_paths_and_settings = {
	"ui-path":		gamma_path + "/ui/"		+ ui_file,
	"style-path":	gamma_path + "/style/"	+ style_file,
	"style-scheme":	style_scheme,
	"show_grid": False,
	"font-family": "Droid Sans Mono, Monospace",
	"font-size": 18, # in px
}

