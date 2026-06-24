#!/usr/bin/env bash

set -e

die() {
  echo "$@" >&2
  exit 1
}

USAGE="$0 [update-lockfile|peek-cache] [npm-14|npm-16|npm-18|yarn] <package>"

[[ "$#" -eq 3 ]] || die "$USAGE"

command_arg="$1"
pm_arg="$2"
package_arg="$3"

[[ "$command_arg" == @(update-lockfile|peek-cache) ]] || die "$USAGE"

case "$pm_arg" in
  npm-*)
    case "$pm_arg" in
      npm-18)
        pm_lockfile=package-lock.v3.json
        pm_sdk_ext=node18
      ;;
      npm-16)
        pm_lockfile=package-lock.v2.json
        pm_sdk_ext=node16
      ;;
      npm-14)
        pm_lockfile=package-lock.v1.json
        pm_sdk_ext=node14
      ;;
      *) die "$USAGE" ;;
    esac

    pm_actual_lockfile=package-lock.json

    pm_cache_dir=npm-cache-$pm_sdk_ext
    pm_rc_contents="cache = \"$pm_cache_dir\""
    pm_rc_file=.npmrc
    ;;
  yarn)
    pm_lockfile=yarn.lock
    pm_sdk_ext=node16

    pm_actual_lockfile=yarn.lock

    pm_cache_dir=yarn-mirror
    pm_rc_contents="yarn-offline-mirror \"./$pm_cache_dir\""
    pm_rc_file=.yarnrc
    ;;
  *) die "$USAGE" ;;
esac

pm_name="${pm_arg%-*}"

package_path="$(dirname "$0")/../tests/data/packages/$package_arg"
[[ -d "$package_path" ]] || die "Unknown package: $package_arg"

tmpdir=$(mktemp -d)
trap 'rm -rf -- "$tmpdir"' EXIT

cp "$package_path/package.json" "$tmpdir"

# Special-case handling for our test of a local package.
[[ -d "$package_path/subdir" ]] && cp -r "$package_path/subdir" "$tmpdir"

if [[ "$command_arg" == peek-cache ]]; then
  cp "$package_path/$pm_lockfile" "$tmpdir"
  echo "$pm_rc_contents" > "$tmpdir/$pm_rc_file"
fi

# Workaround for https://github.com/npm/cli/issues/4896.
gitconfig="$tmpdir/gitconfig"
cat > "$gitconfig" <<EOF
[url "https://"]
insteadOf = git://
EOF

(set -x; flatpak run \
  --command=bash \
  --cwd="$tmpdir" \
  --filesystem="$tmpdir" \
  --share=network \
  org.freedesktop.Sdk//22.08 \
  -c "cp $gitconfig ~/.gitconfig && . /usr/lib/sdk/$pm_sdk_ext/enable.sh && $pm_name install")

case "$command_arg" in
  update-lockfile) cp "$tmpdir/$pm_actual_lockfile" "$package_path/$pm_lockfile" ;;
  peek-cache) cp -r "$tmpdir/$pm_cache_dir" "$package_path" ;;
esac
