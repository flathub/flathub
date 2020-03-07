def set_commands(plugin):
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "Copy Line",
			"ref": plugin.copy_line,
			"shortcut": "<Ctrl> + c",
		}
	)
	
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "Cut Line",
			"ref": plugin.cut_line,
			"shortcut": "<Ctrl> + x",
		}
	)
	
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "Duplicate Line",
			"ref": plugin.duplicate_line,
			"shortcut": "<Ctrl> + d",
		}
	)
	
