# print installation instructions for AppDir
# USAGE: print_instructions_appdir
# CALLS: print_instructions
# NEEDED VARS: (LANG) GAME_ID GAME_NAME OPTION_PREFIX
print_instructions_appdir() {
	local string
	case "${LANG%_*}" in
		('fr')
			string='"%s" est maintenant installé.\nPour le désinstaller, supprimez son répertoire d’installation :\n%s\n'
		;;
		('en'|*)
			string='"%s" installed successfully.\nTo uninstall it, just delete the Application Directory:\n%s\n'
		;;
	esac
	printf "$string" "$GAME_NAME" "$OPTION_PREFIX/play.it/$GAME_ID"

}

