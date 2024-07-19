#!/bin/bash
# Reading App Builder
# App build and launch wrapper

# echo "Running reading-app-builder in:"
# echo $( pwd )
# echo $( /app/jre/bin/java -version )
# echo $( /app/jdk17.0.11/bin/javac -version )
# echo $( python3 --version )
# echo $( which python3 )
# echo $( python3 -m aeneas.diagnostics )
# echo " "
# echo "$@"
# echo " "
exec /app/jre/bin/java --module-path "/app/lib/sdk" --add-modules javafx.web,javafx.fxml,javafx.swing,javafx.media --add-opens=javafx.fxml/javafx.fxml=ALL-UNNAMED -jar /app/reading-app-builder/bin/reading-app-builder.jar $@