# update dependencies list with commands needed for icons extraction
# USAGE: icons_list_dependencies
icons_list_dependencies() {
	local script
	script="$0"
	if grep\
		--regexp="^APP_[^_]\\+_ICON='.\\+'"\
		--regexp="^APP_[^_]\\+_ICON_.\\+='.\\+'"\
		"$script" 1>/dev/null
	then
		SCRIPT_DEPS="$SCRIPT_DEPS identify"
		if grep\
			--regexp="^APP_[^_]\\+_ICON='.\\+\\.bmp'"\
			--regexp="^APP_[^_]\\+_ICON_.\\+='.\\+\\.bmp'"\
			--regexp="^APP_[^_]\\+_ICON='.\\+\\.ico'"\
			--regexp="^APP_[^_]\\+_ICON_.\\+='.\\+\\.ico'"\
			"$script" 1>/dev/null
		then
			SCRIPT_DEPS="$SCRIPT_DEPS convert"
		fi
		if grep\
			--regexp="^APP_[^_]\\+_ICON='.\\+\\.exe'"\
			--regexp="^APP_[^_]\\+_ICON_.\\+='.\\+\\.exe'"\
			"$script" 1>/dev/null
		then
			SCRIPT_DEPS="$SCRIPT_DEPS convert wrestool"
		fi
	fi
	export SCRIPT_DEPS
}

# get .png file(s) from various icon sources in current package
# USAGE: icons_get_from_package $app[…]
# NEEDED VARS: APP_ID|GAME_ID PATH_GAME PATH_ICON_BASE PLAYIT_WORKDIR PKG
# CALLS: icons_get_from_path
icons_get_from_package() {
	local path
	local path_pkg
	path_pkg="$(get_value "${PKG}_PATH")"
	[ -n "$path_pkg" ] || missing_pkg_error 'icons_get_from_package' "$PKG"
	path="${path_pkg}${PATH_GAME}"
	icons_get_from_path "$path" "$@"
}
# compatibility alias
extract_and_sort_icons_from() { icons_get_from_package "$@"; }

# get .png file(s) from various icon sources in temporary work directory
# USAGE: icons_get_from_package $app[…]
# NEEDED VARS: APP_ID|GAME_ID PATH_ICON_BASE PLAYIT_WORKDIR PKG
# CALLS: icons_get_from_path
icons_get_from_workdir() {
	local path
	path="$PLAYIT_WORKDIR/gamedata"
	icons_get_from_path "$path" "$@"
}
# compatibility alias
get_icon_from_temp_dir() { icons_get_from_workdir "$@"; }

# get .png file(s) from various icon sources
# USAGE: icons_get_from_path $directory $app[…]
# NEEDED VARS: APP_ID|GAME_ID PATH_ICON_BASE PLAYIT_WORKDIR PKG
# CALLS: icon_extract_png_from_file icons_include_png_from_directory testvar liberror
icons_get_from_path() {
	local app
	local destination
	local directory
	local file
	local icon
	local list
	local path_pkg
	local wrestool_id
	directory="$1"
	shift 1
	destination="$PLAYIT_WORKDIR/icons"
	path_pkg="$(get_value "${PKG}_PATH")"
	[ -n "$path_pkg" ] || missing_pkg_error 'icons_get_from_package' "$PKG"
	for app in "$@"; do
		testvar "$app" 'APP' || liberror 'app' 'icons_get_from_package'
		list="$(get_value "${app}_ICONS_LIST")"
		[ -n "$list" ] || list="${app}_ICON"
		for icon in $list; do
			use_archive_specific_value "$icon"
			file="$(get_value "$icon")"
			[ -f "$directory/$file" ] || icon_file_not_found_error "$directory/$file"
			wrestool_id="$(get_value "${icon}_ID")"
			icon_extract_png_from_file "$directory/$file" "$destination"
		done
		icons_include_png_from_directory "$app" "$destination"
	done || [ $OPTION_PACKAGE = 'appdir' ]
}

# extract .png file(s) from target file
# USAGE: icon_extract_png_from_file $file $destination
# CALLS: icon_convert_bmp_to_png icon_extract_png_from_exe icon_extract_png_from_ico icon_copy_png
# CALLED BY: icons_get_from_path
icon_extract_png_from_file() {
	local destination
	local extension
	local file
	file="$1"
	destination="$2"
	extension="${file##*.}"
	mkdir --parents "$destination"
	case "$extension" in
		('bmp')
			icon_convert_bmp_to_png "$file" "$destination"
		;;
		('exe')
			icon_extract_png_from_exe "$file" "$destination"
		;;
		('ico')
			icon_extract_png_from_ico "$file" "$destination"
		;;
		('png')
			icon_copy_png "$file" "$destination"
		;;
		(*)
			liberror 'extension' 'icon_extract_png_from_file'
		;;
	esac
}
# compatibility alias
extract_icon_from() {
	local destination
	local file
	destination="$PLAYIT_WORKDIR/icons"
	for file in "$@"; do
		extension="${file##*.}"
		if [ "$extension" = 'exe' ]; then
			mkdir --parents "$destination"
			icon_extract_ico_from_exe "$file" "$destination"
		else
			icon_extract_png_from_file "$file" "$destination"
		fi
	done
}


# extract .png file(s) for .exe
# USAGE: icon_extract_png_from_exe $file $destination
# CALLS: icon_extract_ico_from_exe icon_extract_png_from_ico
# CALLED BY: icon_extract_png_from_file
icon_extract_png_from_exe() {
	[ "$DRY_RUN" = '1' ] && return 0
	local destination
	local file
	file="$1"
	destination="$2"
	icon_extract_ico_from_exe "$file" "$destination"
	for file in "$destination"/*.ico; do
		icon_extract_png_from_ico "$file" "$destination"
		rm "$file"
	done
}

# extract .ico file(s) from .exe
# USAGE: icon_extract_ico_from_exe $file $destination
# CALLED BY: icon_extract_png_from_exe
icon_extract_ico_from_exe() {
	[ "$DRY_RUN" = '1' ] && return 0
	local destination
	local file
	local options
	file="$1"
	destination="$2"
	[ "$wrestool_id" ] && options="--name=$wrestool_id"
	wrestool --extract --type=14 $options --output="$destination" "$file" 2>/dev/null
}

# convert .bmp file to .png
# USAGE: icon_convert_bmp_to_png $file $destination
# CALLED BY: icon_extract_png_from_file
icon_convert_bmp_to_png() { icon_convert_to_png "$@"; }

# extract .png file(s) from .ico
# USAGE: icon_extract_png_from_ico $file $destination
# CALLED BY: icon_extract_png_from_file icon_extract_png_from_exe
icon_extract_png_from_ico() { icon_convert_to_png "$@"; }

# convert multiple icon formats to .png
# USAGE: icon_convert_to_png $file $destination
# CALLED BY: icon_extract_png_from_bmp icon_extract_png_from_ico
icon_convert_to_png() {
	[ "$DRY_RUN" = '1' ] && return 0
	local destination
	local file
	local name
	file="$1"
	destination="$2"
	name="${file##*/}"
	convert "$file" "$destination/${name%.*}.png"
}

# copy .png file to directory
# USAGE: icon_copy_png $file $destination
# CALLED BY: icon_extract_png_from_file
icon_copy_png() {
	[ "$DRY_RUN" = '1' ] && return 0
	local destination
	local file
	file="$1"
	destination="$2"
	cp "$file" "$destination"
}

# get .png file(s) from target directory and put them in current package
# USAGE: icons_include_png_from_directory $app $directory
# NEEDED VARS: APP_ID|GAME_ID PATH_ICON_BASE PKG
# CALLS: icon_get_resolution_from_file
# CALLED BY: icons_get_from_path
icons_include_png_from_directory() {
	[ "$DRY_RUN" = '1' ] && return 0
	local app
	local directory
	local file
	local path
	local path_icon
	local path_pkg
	local resolution
	app="$1"
	directory="$2"
	name="$(get_value "${app}_ID")"
	[ -n "$name" ] || name="$GAME_ID"
	path_pkg="$(get_value "${PKG}_PATH")"
	[ -n "$path_pkg" ] || missing_pkg_error 'icons_include_png_from_directory' "$PKG"
	for file in "$directory"/*.png; do
		icon_get_resolution_from_file "$file"
		path_icon="$PATH_ICON_BASE/$resolution/apps"
		path="${path_pkg}${path_icon}"
		mkdir --parents "$path"
		mv "$file" "$path/$name.png"
	done
}
# comaptibility alias
sort_icons() {
	local app
	local directory
	directory="$PLAYIT_WORKDIR/icons"
	for app in "$@"; do
		icons_include_png_from_directory "$app" "$directory"
	done
}

# get image resolution for target file, exported as $resolution
# USAGE: icon_get_resolution_from_file $file
# CALLED BY: icons_include_png_from_directory
icon_get_resolution_from_file() {
	local file
	local version_major_target
	local version_minor_target
	file="$1"
	version_major_target="${target_version%%.*}"
	version_minor_target=$(printf '%s' "$target_version" | cut --delimiter='.' --fields=2)
	if
		{ [ $version_major_target -lt 2 ] || [ $version_minor_target -lt 8 ] ; } &&
		[ -n "${file##* *}" ]
	then
		field=2
		unset resolution
		while [ -z "$resolution" ] || [ -n "$(printf '%s' "$resolution" | sed 's/[0-9]*x[0-9]*//')" ]; do
			resolution="$(identify $file)" || break
			resolution="$(sed "s;^$file ;;" <<< "$resolution" | cut --delimiter=' ' --fields=$field)"
			field=$((field + 1))
		done
	else
		resolution="$(identify "$file" | sed "s;^$file ;;" | cut --delimiter=' ' --fields=2)"
	fi
	export resolution
}

# link icons in place post-installation from game directory
# USAGE: icons_linking_postinst $app[…]
# NEEDED VARS: APP_ID|GAME_ID PATH_GAME PATH_ICON_BASE PKG
icons_linking_postinst() {
	[ "$DRY_RUN" = '1' ] && return 0
	local app
	local file
	local icon
	local list
	local name
	local path
	local path_icon
	local path_pkg
	local version_major_target
	local version_minor_target
	version_major_target="${target_version%%.*}"
	version_minor_target=$(printf '%s' "$target_version" | cut --delimiter='.' --fields=2)
	path_pkg="$(get_value "${PKG}_PATH")"
	[ -n "$path_pkg" ] || missing_pkg_error 'icons_linking_postinst' "$PKG"
	path="${path_pkg}${PATH_GAME}"
	for app in "$@"; do
		list="$(get_value "${app}_ICONS_LIST")"
		[ "$list" ] || list="${app}_ICON"
		name="$(get_value "${app}_ID")"
		[ "$name" ] || name="$GAME_ID"
		for icon in $list; do
			file="$(get_value "$icon")"
			if
				{ [ $version_major_target -lt 2 ] || [ $version_minor_target -lt 8 ] ; } &&
				( ls "$path/$file" >/dev/null 2>&1 || ls "$path"/$file >/dev/null 2>&1 )
			then
				icon_get_resolution_from_file "$path/$file"
			else
				icon_get_resolution_from_file "${PKG_DATA_PATH}${PATH_GAME}/$file"
			fi
			path_icon="$PATH_ICON_BASE/$resolution/apps"
			if
				{ [ $version_major_target -lt 2 ] || [ $version_minor_target -lt 8 ] ; } &&
				[ -n "${file##* *}" ]
			then
				cat >> "$postinst" <<- EOF
				if [ ! -e "$path_icon/$name.png" ]; then
				  mkdir --parents "$path_icon"
				  ln --symbolic "$PATH_GAME"/$file "$path_icon/$name.png"
				fi
				EOF
			else
				cat >> "$postinst" <<- EOF
				if [ ! -e "$path_icon/$name.png" ]; then
				  mkdir --parents "$path_icon"
				  ln --symbolic "$PATH_GAME/$file" "$path_icon/$name.png"
				fi
				EOF
			fi
			cat >> "$prerm" <<- EOF
			if [ -e "$path_icon/$name.png" ]; then
			  rm "$path_icon/$name.png"
			  rmdir --parents --ignore-fail-on-non-empty "$path_icon"
			fi
			EOF
		done
	done
}
# compatibility alias
postinst_icons_linking() { icons_linking_postinst "$@"; }

# move icons to the target package
# USAGE: icons_move_to $pkg
# NEEDED VARS: PATH_ICON_BASE PKG
icons_move_to() {
	local destination
	local source
	destination="$1"
	destination_path="$(get_value "${destination}_PATH")"
	[ -n "$destination_path" ] || missing_pkg_error 'icons_move_to' "$destination"
	source="$PKG"
	source_path="$(get_value "${source}_PATH")"
	[ -n "$source_path" ] || missing_pkg_error 'icons_move_to' "$source"
	[ "$DRY_RUN" = '1' ] || [ "$source_path" = "$destination_path" ] && return 0
	(
		cd "$source_path"
		cp --link --parents --recursive --no-dereference --preserve=links "./$PATH_ICON_BASE" "$destination_path"
		rm --recursive "./$PATH_ICON_BASE"
		rmdir --ignore-fail-on-non-empty --parents "./${PATH_ICON_BASE%/*}"
	)
}
# compatibility alias
move_icons_to() { icons_move_to "$@"; }

# print an error message if an icon can not be found
# USAGE: icon_file_not_found_error $file
# CALLED BY: icons_get_from_path
icon_file_not_found_error() {
	local file
	local string1
	local string2
	file="$1"
	case "${LANG%_*}" in
		('fr')
			string1='Le fichier d’icône suivant est introuvable : %s'
			string2='Merci de signaler cette erreur sur notre outil de gestion de bugs : %s'
		;;
		('en'|*)
			string1='The following icon file could not be found: %s'
			string2='Please report this issue in our bug tracker: %s'
		;;
	esac
	print_error
	printf "$string1\\n" "$1"
	printf "$string2\\n" "$PLAYIT_GAMES_BUG_TRACKER_URL"
	return 1
}

