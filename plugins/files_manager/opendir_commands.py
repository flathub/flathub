

def set_commands(plugin):
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "Open Directory",
			"ref": plugin.opendir,
			"shortcut": "<Shift><Ctrl> + O",
		}
	)  
	
