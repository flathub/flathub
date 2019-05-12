for script in "${BASH_SOURCE%/*}"/[0-9]* ; do
	. $script
done
