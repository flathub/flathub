

def set_commands(plugin):
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "Show Log",
			"ref": plugin.show_log,
			"shortcut": "<Shift><Ctrl> + L",
		}
	)
	
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "Show WARNINGS",
			"ref": plugin.show_log,
			"parameters": 1,
			"shortcut": "",
		}
	)
	
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "Show ERRORS",
			"ref": plugin.show_log,
			"parameters": 2,
			"shortcut": "",
		}
	)
	
	
