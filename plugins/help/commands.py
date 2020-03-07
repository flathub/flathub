
def set_commands(plugin):
	plugin.commands.append( 
		{
			"plugin-name": plugin.name,
			"name": "Help",
			"ref": plugin.show_help,
			"shortcut": "F1",
		}
	)
	