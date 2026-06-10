#!/bin/bash

# pyside
rm -rfv ${FLATPAK_DEST}/bin/pyside6-*
rm -rfv ${FLATPAK_DEST}/lib/python*/site-packages/PySide6/scripts
rm -rfv ${FLATPAK_DEST}/lib/python*/site-packages/PySide6/doc

# webengine cleanup
# krb5
rm -rfv ${FLATPAK_DEST}/etc/krb5.conf
rm -rfv ${FLATPAK_DEST}/lib/krb5
rm -rfv ${FLATPAK_DEST}/lib/lib{com_err,gss{api_krb5,rpc},k5crypto,kadm5{clnt{,_mit},srv{,_mit}},kdb5,krad,krb5{,support},verto}.so*
rm -rfv ${FLATPAK_DEST}/share/locale/*/LC_MESSAGES/mit-krb5.mo

# libevent
rm -rfv ${FLATPAK_DEST}/lib/libevent{,_core,_extra,_openssl,_pthreads}*.so*

# minizip, pciutils, re2, snappy
rm -rfv ${FLATPAK_DEST}/lib/lib{minizip,pci,re2,snappy}.so*

# pyside webengine
rm -rfv ${FLATPAK_DEST}/lib/python*/site-packages/PySide6/QtWebEngine{,Core,Quick,Widgets}.abi3.so
rm -rfv ${FLATPAK_DEST}/lib/python*/site-packages/PySide6/QtWebEngine{,Core,Quick,Widgets}.pyi

# pyside qtpdf
rm -rfv ${FLATPAK_DEST}/lib/python*/site-packages/PyQt6/Qt{,Pdf,PdfWidgets}.abi3.so
rm -rfv ${FLATPAK_DEST}/lib/python*/site-packages/PyQt6/Qt{,Pdf,PdfWidgets}.pyi

# pyside qtwebview
rm -rfv ${FLATPAK_DEST}/lib/qml/QtWebView
rm -rfv ${FLATPAK_DEST}/lib/plugins/webview
rm -rfv ${FLATPAK_DEST}/lib/${FLATPAK_ARCH}-linux-gnu/libQt*WebView{,Quick}.so*
rm -rfv ${FLATPAK_DEST}/lib/python*/site-packages/PySide6/QtWebView.abi3.so
rm -rfv ${FLATPAK_DEST}/lib/python*/site-packages/PySide6/QtWebView.pyi

# qtwebengine
rm -rfv ${FLATPAK_DEST}/bin/QtWebEngineProcess
rm -rfv ${FLATPAK_DEST}/lib/plugins/imageformats
rm -rfv ${FLATPAK_DEST}/lib/qml/{QtQuick/Pdf,QtWebEngine}
rm -rfv ${FLATPAK_DEST}/lib/${FLATPAK_ARCH}-linux-gnu/libQt*{Pdf{,Quick,Widgets},WebEngine{,Core,Quick{,DelegatesQml},Widgets}}.so*
rm -fv ${FLATPAK_DEST}/lib/libQt*{Pdf{,Quick,Widgets},WebEngine{,Core,Quick{,DelegatesQml},Widgets}}.so*
rm -rfv ${FLATPAK_DEST}/lib/libQt6WebView*.so*
rm -rfv ${FLATPAK_DEST}/qtwebengine_dictionaries
rm -rfv ${FLATPAK_DEST}/resources/qtwebengine*.pak
rm -rfv ${FLATPAK_DEST}/resources/v8_context_snapshot.bin
rm -rfv ${FLATPAK_DEST}/share/locale/*/qtwebengine_dictionaries
rm -rfv ${FLATPAK_DEST}/translations/qtwebengine_locales
rm -rfv ${FLATPAK_DEST}/lib/libexec/webenginedriver
rm -rfv ${FLATPAK_DEST}/lib/libexec/QtWebEngineProcess

pip uninstall -y numpy
pip uninstall -y build
