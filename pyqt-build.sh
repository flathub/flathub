#!/usr/bin/env sh

PYTHON_VERSION=$(python3 -c 'import sys; print("{}.{}".format(*sys.version_info))')

sip-install \
    --qt-shared \
    --concatenate 1 \
    --confirm-license \
    --debug \
    --verbose \
    --disable QtBluetooth \
    --disable QtDBus \
    --disable QtDesigner \
    --disable QtHelp \
    --disable QtLocation \
    --disable QtMultimedia \
    --disable QtMultimediaWidgets \
    --disable QtNetwork \
    --disable QtNfc \
    --disable QtOpenGL \
    --disable QtPositioning \
    --disable QtPrintSupport \
    --disable QtQml \
    --disable QtQuick \
    --disable QtQuickWidgets \
    --disable QtRemoteObjects \
    --disable QtSensors \
    --disable QtSerialPort \
    --disable QtSql \
    --disable QtSvg \
    --disable QtTest \
    --disable QtTextToSpeech \
    --disable QtWebChannel \
    --disable QtWebSockets \
    --disable QtXml \
    --disable QtXmlPatterns \
    --disable _QOpenGLFunctions_2_0 \
    --disable _QOpenGLFunctions_2_1 \
    --disable _QOpenGLFunctions_4_1_Core \
    --no-dbus-python \
    --no-designer-plugin \
    --no-docstrings \
    --no-qml-plugin \
    --no-tools \
    --jobs="${FLATPAK_BUILDER_N_JOBS}" \
    --build-dir="${FLATPAK_BUILDER_BUILDDIR}/tmp" \
    --target-dir="${FLATPAK_DEST}/lib/python${PYTHON_VERSION}/site-packages" \
    --qmake-setting QMAKE_CFLAGS="$CFLAGS" \
    --qmake-setting QMAKE_CXXFLAGS="$CXXFLAGS" \
    --qmake-setting QMAKE_LFLAGS="$LDFLAGS"