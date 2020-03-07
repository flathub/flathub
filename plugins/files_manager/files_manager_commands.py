

def set_commands(plugin):
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "Close File",
			"ref": plugin.close_current_file,
			"shortcut": "<Ctrl> + w",
		}
	) 
	
	
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "Close All",
			"ref": plugin.close_all,
			"shortcut": "<Shift><Ctrl> + W",
		}
	)
	
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "New File",
			"ref": plugin.create_new_file,
			"shortcut": "<Ctrl> + n",
		}
	)
