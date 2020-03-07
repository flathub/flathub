

def set_commands(plugin):
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "Open File",
			"ref": plugin.openfile,
			"shortcut": "<Ctrl> + o",
		}
	)  
	
