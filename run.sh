#!/bin/sh
exec /app/jre/bin/java --module-path /app/openjfx --add-modules=javafx.controls,javafx.fxml -jar /app/icebox.jar
