#!/bin/sh

# Persistent tmp
mkdir -p "$HOME/.java/tmp"

# Initialize external providers with defaults
if [ ! -f "$HOME/external_providers.xml" ]; then
    cat > "$HOME/external_providers.xml" << 'XML_EOF'
<?xml version="1.0" encoding="UTF-8"?>
<Providers>
<Provider>
<Name>libCCGraphiteP11</Name>
<URI>file:/app/extra/libCCGraphiteP11.2.0.5.6.so</URI>
</Provider>
</Providers>
XML_EOF
fi

INSTALL_DIR="${XDG_DATA_HOME}/szafir_host"
LOCK_FILE="${XDG_DATA_HOME}/.szafirhost-install.lock"
JAR_PATH="${INSTALL_DIR}/SzafirHost.jar"

# Run IzPack installer on first launch; flock prevents parallel installs.
if [ ! -f "$JAR_PATH" ]; then
    mkdir -p "$XDG_DATA_HOME"
    (
        flock -x 200
        # Re-check after acquiring the lock (another instance may have just finished).
        if [ ! -f "$JAR_PATH" ]; then
            AUTO_XML="$(mktemp)"
            cat > "$AUTO_XML" << EOF
<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<AutomatedInstallation langpack="pol">
    <com.izforge.izpack.panels.htmllicence.HTMLLicencePanel id="SHLicensePanel"/>
    <com.izforge.izpack.panels.target.TargetPanel id="SHTargetPanel">
        <installpath>${INSTALL_DIR}</installpath>
    </com.izforge.izpack.panels.target.TargetPanel>
    <com.izforge.izpack.panels.packs.PacksPanel id="SHPacksPanel">
        <pack index="0" name="Szafir Host" selected="true"/>
    </com.izforge.izpack.panels.packs.PacksPanel>
    <com.izforge.izpack.panels.install.InstallPanel id="SHInstallPanel"/>
    <com.izforge.izpack.panels.finish.FinishPanel id="SHFinishPanel"/>
</AutomatedInstallation>
EOF
            /app/jre/bin/java -jar /app/extra/szafirhost-install.jar "$AUTO_XML" >&2
            rm -f "$AUTO_XML"
        fi
    ) 200>"$LOCK_FILE"
fi

cd "$INSTALL_DIR" || exit 1

exec /app/jre/bin/java \
    -Djava.io.tmpdir="$HOME/.java/tmp" \
    -Dsun.java2d.uiScale.enabled=true \
    -jar SzafirHost.jar "$@"
