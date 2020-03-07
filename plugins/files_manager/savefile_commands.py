

def set_commands(plugin):
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "Save File",
			"ref": plugin.save_current_file,
			"shortcut": "<Ctrl> + s",
		}
	)
	
