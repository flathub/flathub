#!/usr/bin/env bash

# <functions>
usage() {
  cat << EOF
usage: ${0} [OPTIONS] [DIRS...]

Copy files from selected directories.

OPTIONS:
  -h        Show help.
  -o [dir]  REQUIRED: Set [dir] as the output directory.
  -f [file] REQUIRED: Set [file] as the input filename.
  -g [file] REQUIRED: Set [file] as the output filename.
  -v        Verbose mode.

Examples:
  * ${0} -v -o "/app/share/icons/hicolor" -f "document-edit-symbolic.svg" -g "document-edit.svg" "/app/share/icons/Adwaita" "/usr/share/icons/breeze"
  * ${0} -h
EOF
}

is_valid_filename() {
  filename="${1}"
  
  regexp_dir="/"
  if [[ -z "${filename}" || "${filename}" == "." || "${filename}" == ".." || "${filename}" =~ ${regexp_dir} ]]; then
    return 1
  else
    return 0
  fi
}
# </functions>

# <parameters>
dirname_output=
filename_input=
filename_output=
verbose=

while getopts "ho:f:g:v" OPTION; do
  case "${OPTION}" in
    h)
      usage
      exit
      ;;
    o)
      dirname_output="${OPTARG}"
      ;;
    f)
      filename_input="${OPTARG}"
      if ! is_valid_filename "{filename_input}"; then
        echo "Error: Invalid input filename." >&2
      fi
      ;;
    g)
      filename_output="${OPTARG}"
      if ! is_valid_filename "{filename_output}"; then
        echo "Error: Invalid output filename." >&2
      fi
      ;;
    v)
      verbose=1
      ;;
    ?)
      usage
      exit 1
      ;;
  esac
done

shift "$(( OPTIND - 1 ))"

if [[ ! -z "${verbose}" ]]; then
  echo "OPTIND: ${OPTIND}" >&2
  echo "ARGV: ${#@}" >&2
  echo >&2

  echo "dirname_output: ${dirname_output}" >&2
  echo "filename_input: ${filename_input}" >&2
  echo "filename_output: ${filename_output}" >&2
  echo "verbose: $verbose" >&2
  echo >&2
  
  if [[ "${#}" -gt "0" ]]; then
    echo "DIRS:" >&2
    for arg in "${@}"; do
      echo " - ${arg}" >&2
    done
    echo >&2
  fi
fi
# </parameters>

# <conditions>
if [[ -z "${dirname_output}" ]] || [[ -z "${filename_input}" ]] || [[ -z "${filename_output}" ]]; then
  echo "Error: The required arguments are missing." >&2
  
  usage
  exit 1
fi

if [[ ! "${#}" -gt "0" ]]; then
  echo "Error: No input directories were specified." >&2
  
  usage
  exit 1
fi
# </conditions>

# <main>
for dirname_input in "${@}"; do
  [[ -d "${dirname_input}" ]] || continue
  
  while read -r -d $'\0'; do
    filename="${REPLY#./}"
    f="$( basename "${filename}" )"
    d="$( dirname "${filename}" )"
    
    if [[ "${f}" == "${filename_input}" ]]; then
      if [[ ! -e "${dirname_output}/${d}/${f}" ]]; then
        echo "< ${dirname_input}/${d}/${f}"
        echo "> ${dirname_output}/${d}/${filename_output}"
        install -D -p -m 0644 "${dirname_input}/${d}/${f}" "${dirname_output}/${d}/${filename_output}"
      fi
    fi
    
  done < <( cd "${dirname_input}"; find ./ -xtype f -print0 | sort -zV )
done
# </main>

