
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
			"name": "A command name 1",
			"ref": plugin.a_method1,
			"shortcut": "<Alt><Ctrl> + n",
		}
	)
	
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "A command name 2",
			"ref": plugin.a_method2,
			"shortcut": "<Alt> + t",
		}
	)
	
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "A command name 3",
			"ref": plugin.a_method3,
			"shortcut": "<Shift><Ctrl> + a",
		}
	)
	
