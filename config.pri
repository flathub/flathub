boost: LIBS += -lboost_serialization
expat: LIBS += -lexpat
expat: PKGCONFIG -= expat
INCLUDEPATH+=/app/include
LIBS+="-L/app/lib"

pyside {
PKGCONFIG -= pyside
INCLUDEPATH += $$system(pkg-config --variable=includedir pyside)
INCLUDEPATH += $$system(pkg-config --variable=includedir pyside)/QtCore
INCLUDEPATH += $$system(pkg-config --variable=includedir pyside)/QtGui
INCLUDEPATH += $$system(pkg-config --variable=includedir QtGui)
}
shiboken {
PKGCONFIG -= shiboken
INCLUDEPATH += $$system(pkg-config --variable=includedir shiboken)
}

QMAKE_LFLAGS += -Wl,-rpath,\\\$\$ORIGIN/../lib

# Mocha compat
LIBS += -lGLU
