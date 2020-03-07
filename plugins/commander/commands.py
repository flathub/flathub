



def set_commands(plugin):
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "Open/Close This Window!",
			"ref": plugin.run,
			"shortcut": "<Alt>",
		}
	)
	
