
# commands and set_commands are important for
# the commander plugin to know this plugin methods, key bindings, description

# properties
# "plugin-name"		the name of the plugin is needed by the commander to 
#					know which is the plugin this command belong
#
# "name":			the name of the command "toggle maximize"
# "ref":			the method reference of the command (plugin.toggle_maximize)
# "shortcut": 		key binding for the command "<Alt>+m"
#
#

def set_commands(plugin):
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "Zoom In",
			"ref": plugin.update_font,
			"parameters": 1,
			"shortcut": "<Ctrl> + =",
		}
	)
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "Zoom Out",
			"ref": plugin.update_font,
			"parameters": -1,
			"shortcut": "<Ctrl> + -",
		}
	)
	
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "Zoom Reset",
			"ref": plugin.update_font,
			"parameters": 0,
			"shortcut": "<Ctrl> + 0",
		}
	)
	
