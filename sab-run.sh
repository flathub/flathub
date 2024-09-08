#!/bin/bash
# Scripture App Builder
# Command line app build and launch wrapper
# Use sab.sh -? or see the document 'Building Apps' for more information
exec /app/jre/bin/java -Djava.io.tmpdir="/var/tmp" --module-path "/app/lib/sdk" --add-modules javafx.web,javafx.fxml,javafx.swing,javafx.media --add-opens=javafx.fxml/javafx.fxml=ALL-UNNAMED -jar /app/scripture-app-builder/bin/scripture-app-builder.jar "$@"
