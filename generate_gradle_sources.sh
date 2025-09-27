#!/usr/bin/sh
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
#
# Few portions of this program are based on:
#   https://github.com/flathub/org.ghidra_sre.Ghidra/blob/47e852fe969f351502be8b8bc23189368f8da6c1/generate-deps.sh#L3-L26

REPO_URL='https://github.com/Querz/mcaselector.git'
WORKDIR='_generate_deps_sources'
JDK='openjdk21'

update_or_clone() {
  TAG="$1"

  if [ -z "$TAG" ]; then
    echo "Error: version argument required." >&2
    echo "Usage: $0 <version-tag>" >&2
    return 2
  fi

  # get remote commit for tag (handles lightweight tags; annotated tag objects return tag object SHA)
  REMOTE_REF=$(git ls-remote "$REPO_URL" "refs/tags/$TAG")
  if [ -z "$REMOTE_REF" ]; then
    echo "Tag '$TAG' not found in remote." >&2
    return 3
  fi
  REMOTE_SHA=$(printf "%s" "$REMOTE_REF" | awk '{print $1}')

  # if annotated tag, try to resolve to commit (git ls-remote with ^{} gives dereferenced target)
  DEREF=$(git ls-remote "$REPO_URL" "refs/tags/$TAG^{}")
  if [ -n "$DEREF" ]; then
    REMOTE_SHA=$(printf "%s" "$DEREF" | awk '{print $1}')
  fi

  if [ -d "$WORKDIR/.git" ]; then
    LOCAL_SHA=$(git -C "$WORKDIR" rev-parse --verify HEAD 2>/dev/null)
    if [ "$LOCAL_SHA" = "$REMOTE_SHA" ]; then
      echo "Local repository already at tag '$TAG' (commit $LOCAL_SHA). Skipping clone."
      return 0
    fi
    echo "Local differs (local $LOCAL_SHA vs remote $REMOTE_SHA). Re-cloning."
    rm -rf "$WORKDIR"
  fi

  git -c advice.detachedHead=false clone --depth 1 --branch "$TAG" "$REPO_URL" "$WORKDIR"
  return $?
}

get_sdk() {
	OUTPUT=$(flatpak info org.freedesktop.Sdk 2>&1)
	RET=$?

	if [ $RET -eq 0 ] ; then
		echo $(flatpak info org.freedesktop.Sdk --show-sdk)
	else
		echo $OUTPUT | awk '{print $NF}'
	fi
}

varify_jdk() {
	JDK_EXT=$(flatpak info $SDK | grep "Ref:" | sed -e 's,Ref: runtime/,,' -e "s,org.freedesktop.Sdk,org.freedesktop.Sdk.Extension.$JDK,")
	OUTPUT=$(flatpak info $JDK_EXT 2>&1)
	if [ $? -ne 0 ] ; then
		echo "$JDK_EXT extension missing for runtime $SDK"
		exit 1
	fi
}

update_or_clone "$1" || exit 1
cd $WORKDIR || exit 1

patch -p1 --batch --forward < ../0001-gradle-generate-flatpak-sources.patch

SDK=$(get_sdk)
varify_jdk
echo "Running on $SDK and $JDK"

echo "source /usr/lib/sdk/$JDK/enable.sh && gradle flatpakGradleGenerator" | flatpak run --user --share=network --filesystem=$(pwd) --devel $SDK

mv ./flatpak-sources.json ../gradle-sources.json
