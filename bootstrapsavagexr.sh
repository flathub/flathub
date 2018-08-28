#!/bin/bash
#
VERSION="0.7.0"
#
# Newerth's Savage XR "Battle for Newerth" unofficial Flatpak Bootstrap (installer/update/runner).
# By C.I.A. June 2018.
#
# For the Newerth.com community and the Evo clan, which together maintain Savage XR. Your doing a fantastic job people! :)
# Say NO to racism, xenophobia and all types of bigotry..
#
# May not install on 2GB machines due to bug in OSTree layer running out of ram (installer consumes all free ram, wont use swap then crash).
#
# A BIG cheery thanks to John Ramsden, whose guide to Flatpak'ing 'Path of Exile' was invaluable and can be found here:
#	* https://ramsdenj.com/2018/03/26/packaging-pathofexile-with-flatpak.html
#
# Use TABs not SPACES!
#
#
# ----------------------------------------------------------------------
#export PULSE_LATENCY_MSEC=60
#export __GL_THREADED_OPTIMIZATIONS=1
#export vblank_mode=0
#
# ----------------------------------------------------------------------
#
INSTALL_LOCATION="${HOME}/.var/app/org.newerth.savagexr/data"
#
# ----------------------------------------------------------------------
#
# Savage XR Installer is now downloaded during the Flatpak install.
#
INSTALLER_NAME="xr_setup-1.0-cl_lin_prod.bin"
INSTALLER="/app/extra/${INSTALLER_NAME}"
#
# Installer directory name is where the Savage XR installer will install to by default, can't be changed.
INSTALLER_DIR_NAME="savage-xr"
INSTALLER_DIR_LOCATION="~/${INSTALLER_DIR_NAME}"
#
# Savage legacy preference directory, which cannot be relocated from ${HOME}.
# Not sure if its used for preferences any more but log files do get dump here.
PREFERENCE_DIR=".savage"
#
#
RUN_SCRIPT="silverback.bin"
SAVAGE_UPDATER="savage.sh"
SAVAGE_BINARY="silverback.bin"
#
# AutoUpdater config file location.
#
AUTOUPDATER_CONFIG="${INSTALL_LOCATION}/${INSTALLER_DIR_NAME}/au.cfg"


#
# ----------------------------------------------------------------------
# Library files which get updated by the Savage XR installer but which 
# stop the program from running!
#
aDeleteLibraries=( "libs/libz.so.1" "libs/libstdc++.so.6" )

#
# ----------------------------------------------------------------------
# banner() pretty print some text to the console.
# @param string [string|string] The message(s) to display on the console.
#
banner(){
	if [ "$1" ]; then
	
		echo "###############################################################################"
		echo "#"

		until [ -z "$1" ]
		do
			echo "#	$1"
			shift
		done

		echo "#"
		echo "###############################################################################"
	fi
}

#
# ----------------------------------------------------------------------
# exitScript() wait for user to press a key, then exit script.
#
exitScript(){
	
	read -p "Press any key to exit..." -n1 -s
	exit 1
}

#
# ----------------------------------------------------------------------
# runInstaller() Prepare for the Savage XR installer (create appropriate
# directories) and run it.
#
runInstaller(){
	banner "Preparing for installer, creating symbolic links etc"

	# Make symbolic link from where Savage XR defaults its install directory
	# to where it can persisted in flatpak, which is a stupidly long path that the
	# user will no doubt get wrong, hence the symbolic link.
	#
	mkdir "${INSTALL_LOCATION}/${INSTALLER_DIR_NAME}"
	cd ~
	ln -s -T "${INSTALL_LOCATION}/${INSTALLER_DIR_NAME}" "${INSTALLER_DIR_NAME}"

	banner "Running Savage XR installer, please choose default location." "But DE-SELECT:" "	* Launch Savage XR (as this installer needs to run further)" "	* Create menu Shortcuts (not needed)" "	* Create Desktop Shortcut (not needed)" "Then click on the 'Finish' button."

	"${INSTALLER}"
}

#
# ----------------------------------------------------------------------
# cleanLibs() Remove library files which stop Savage XR from running under Flatpak,
# this is compounded by the Savage XR Updater which puts them back!.
#
cleanLibs(){

	for sLibraryName in "${aDeleteLibraries[@]}"
	do
	
echo "Checking: '${INSTALL_LOCATION}/${INSTALLER_DIR_NAME}/${sLibraryName}'"
		if [ -f "${INSTALL_LOCATION}/${INSTALLER_DIR_NAME}/${sLibraryName}" ]; then
echo "Removing: '${INSTALL_LOCATION}/${INSTALLER_DIR_NAME}/${sLibraryName}'"
			rm "${INSTALL_LOCATION}/${INSTALLER_DIR_NAME}/${sLibraryName}"
			
		fi

	done
}

#
# ----------------------------------------------------------------------
# restoreSavagePreferences() There is a hidden ~/.savage directory which
# contains persistent data logs and preferences, which would be lost on exist.
# Not uber needed, but just in case...
#
restoreSavagePreferences(){
	banner "Restoring Savage preference/log directory."

	# Make symbolic link from where Savage XR defaults its install directory
	# to where it can persisted in flatpak, which is a stupidly long path that the
	# user will no doubt get wrong, hence the symbolic link.
	#
	if [ ! -d "${INSTALL_LOCATION}/${PREFERENCE_DIR}" ]; then
		mkdir "${INSTALL_LOCATION}/${PREFERENCE_DIR}"
	fi

	cd ~
	if [ ! -d "${PREFERENCE_DIR}" ]; then
		ln -s -T "${INSTALL_LOCATION}/${PREFERENCE_DIR}" "${PREFERENCE_DIR}"
	fi
}

#
# ----------------------------------------------------------------------
# turnOffAutoStart() Stop AutoUpdater from starting automatically Savage XR after update,
# by changing config param in au.cfg.
#
turnOffAutoStart(){
	 sed --in-place --expression='s/startApplication "1"/startApplication "0"/' "${AUTOUPDATER_CONFIG}"
}

#
# ----------------------------------------------------------------------
# updateSavageXR() Run the Updater to pull in latest changes.
#
updateSavageXR(){

	if [ ! -f "${SAVAGE_UPDATER}" ]; then
		banner "Unable to Update Savage XR...." "Cannot locate '${SAVAGE_UPDATER}' to Update Savage XR."

		exitScript
	fi

pwd
	banner "Starting Savage XR Updater."
	"./${SAVAGE_UPDATER}" 2>&1
	local iUpdaterErrorCode=$?

	# Check if Updater crashed (most likely the Updater replaced the libs we need to delete again).
	if [ "$iUpdaterErrorCode" -ne 0 ]; then
		banner "Updater crashed for unknown reasons." "Error code: ${iUpdaterErrorCode}"

#		exitScript
	fi
}

#
# ----------------------------------------------------------------------
# startSavageXR() Start Savage XR.
#
startSavageXR(){
	banner "Starting Savage XR..."
	chmod +x "./${SAVAGE_BINARY}"
	"./${SAVAGE_BINARY}" 2>&1
	local iSavageXRErrorCode=$?

	# Check if Savage XR crashed.
	if [ "$iSavageXRErrorCode" -ne 0 ]; then
		banner "Savage XR crashed for unknown reasons." "Error code: ${iSavageXRErrorCode}" "It may have just crashed whilst existing the program..."

#		exitScript
	fi
}

#
# ----------------------------------------------------------------------
# welcome() Welcome screen.
#
welcome(){
	banner "Welcome to Newerth's Savage XR 'Battle for Newerth'" "unofficial Flatpak Bootstrap (installer/update/runner) Version: ${VERSION}." "by C.I.A. June 2018." " " "For the Newerth.com community, which maintain Savage XR." "Your doing a fantastic job people! :)" "Say NO to racism, xenophobia and all types of bigotry.."
}

#
# ----------------------------------------------------------------------
# main() function to check if Savage has been installed, if it has been
# updated and finally run Savage XR.
#
main(){

	welcome
	restoreSavagePreferences

	if [ ! -d "${INSTALL_LOCATION}/${INSTALLER_DIR_NAME}" ] || [ ! -f "${INSTALL_LOCATION}/${INSTALLER_DIR_NAME}/${SAVAGE_UPDATER}" ]; then
		runInstaller
		turnOffAutoStart
	fi

	cd "${INSTALL_LOCATION}/${INSTALLER_DIR_NAME}"

	cleanLibs
	updateSavageXR
	cleanLibs
	startSavageXR
}


#
# ----------------------------------------------------------------------
# Start application.
#
main
