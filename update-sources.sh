#!/bin/sh

set -eo pipefail

REPO_GITHUB_USER="TeamWheelWizard"
REPO_NAME="WheelWizard"

DOTNET_GENERATOR="flatpak-dotnet-generator.py"
DOTNET_GENERATOR_URL="https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/master/dotnet/$DOTNET_GENERATOR"

MANIFEST="io.github.TeamWheelWizard.WheelWizard.yaml"

DOTNET_VERSION=""
FREEDESKTOP_VERSION=""
COMMIT=""

usage() {
    echo "Usage: $0 --dotnet <version> --freedesktop <version> --commit <commit>"
    exit 1
}

cleanup() {
    rm -rf "$REPO_NAME"
    rm -f "$DOTNET_GENERATOR"
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --dotnet)
            if [ -n "$2" ] && [ "${2:0:2}" != "--" ]; then
                DOTNET_VERSION="$2"
                shift 2
            else
                echo "[-] --dotnet requires a version argument."
                usage
            fi
            ;;
        --freedesktop)
            if [ -n "$2" ] && [ "${2:0:2}" != "--" ]; then
                FREEDESKTOP_VERSION="$2"
                shift 2
            else
                echo "[-] --freedesktop requires a version argument."
                usage
            fi
            ;;
        --commit)
            if [ -n "$2" ] && [ "${2:0:2}" != "--" ]; then
                COMMIT="$2"
                shift 2
            else
                echo "[-] --commit requires a commit argument."
                usage
            fi
            ;;
        -*)
            echo "[-] Unknown option $1"
            usage
            ;;
        *)
            echo "[-] Invalid argument $1"
	    usage
	    ;;
    esac
done

if [ -z "$DOTNET_VERSION" ] || [ -z "$FREEDESKTOP_VERSION" ] || [ -z "$COMMIT" ]; then
    echo "[-] Options --dotnet, --freedesktop and --commit are required."
    usage
fi

cleanup

flatpak remote-add --user --if-not-exists flathub 'https://dl.flathub.org/repo/flathub.flatpakrepo'
# Required Flatpaks for the .NET generator
flatpak install --user --noninteractive flathub "org.freedesktop.Sdk//$FREEDESKTOP_VERSION"
flatpak install --user --noninteractive flathub "org.freedesktop.Sdk.Extension.dotnet$DOTNET_VERSION//$FREEDESKTOP_VERSION"

git clone "https://github.com/$REPO_GITHUB_USER/$REPO_NAME"
pushd "$REPO_NAME"
git checkout "$COMMIT"
popd

curl -OL "$DOTNET_GENERATOR_URL"
python3 "$DOTNET_GENERATOR" --dotnet "$DOTNET_VERSION" --freedesktop "$FREEDESKTOP_VERSION" nuget-sources.json \
    "$REPO_NAME/$REPO_NAME/$REPO_NAME.csproj"

sed -i -e "s|dotnet[^[:space:]/]|dotnet$DOTNET_VERSION|g" "$MANIFEST"
sed -i -e "s|^runtime-version:.*|runtime-version: '$FREEDESKTOP_VERSION'|g" "$MANIFEST"
yq -y . "$MANIFEST" | tee "$MANIFEST.1" >/dev/null
yq -y "(.modules[] \
    | select(has(\"sources\")) \
    | .sources[] \
    | select(type == \"object\") \
    | select(.url == \"https://github.com/$REPO_GITHUB_USER/$REPO_NAME.git\") \
    | .commit) = \"$COMMIT\"" "$MANIFEST" | tee "$MANIFEST.2" >/dev/null
diff "$MANIFEST.1" "$MANIFEST.2" > "$MANIFEST.diff" || true
patch -o "$MANIFEST.new" "$MANIFEST" < "$MANIFEST.diff"
rm -f "$MANIFEST.1" "$MANIFEST.2" "$MANIFEST.diff" "$MANIFEST"
mv "$MANIFEST.new" "$MANIFEST"

# Note that the Freedesktop SDK + .NET extension flatpaks will still be installed for the current user after this!
# A simple `flatpak uninstall --user --all` command can uninstall all per-user installations

cleanup
