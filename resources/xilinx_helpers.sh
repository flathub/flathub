#/bin/sh

if [ -z "$XILINX_INSTALL_PATH" ]; then
	export XILINX_INSTALL_PATH="/home/$USER/.Xilinx/install"
fi

export LD_LIBRARY_PATH=/app/lib
export LC_NUMERIC=en_US.UTF-8	# Vivado may throw errors on locales which uses comma as decimal separator

function xilinx_install() {
	if [ -f "$XILINX_INSTALL_PATH/.xinstall/Vitis_2020.2/xsetup" ]; then
		installer_path="$XILINX_INSTALL_PATH/.xinstall/Vitis_2020.2/xsetup"
	else
		# Get the installer path
		installer_path=$(zenity --file-selection --title "Select the Xilinx installer (Xilinx_Unified_*_Lin64.bin)")

		zenity --warning --text "The Xilinx installer will now start. Make sure to select $XILINX_INSTALL_PATH as installation path."
		mkdir -p "$XILINX_INSTALL_PATH"
	fi

	# Run the installer
	sh "$installer_path"
}

function xilinx_install_if_needed() {
	if [ ! -f "$XILINX_INSTALL_PATH/$1" ]; then
		zenity --question --title "Missing software" --text "$1 is not installed in $XILINX_INSTALL_PATH. Do you want to install it now?" && xilinx_install
	fi
}

function xilinx_install_if_needed_then_run() {
	command="$1"
	shift

	xilinx_install_if_needed "$command"
	"$XILINX_INSTALL_PATH/$command" "$@"
}
