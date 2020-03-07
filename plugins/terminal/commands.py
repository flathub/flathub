
def set_commands(plugin):
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "Show Terminal",
			"ref": plugin.show_terminal,
			"shortcut": "<Ctrl> + t",
		}
	)


