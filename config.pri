boost: LIBS += -lboost_serialization
expat: LIBS += -lexpat
expat: PKGCONFIG -= expat
cairo: PKGCONFIG -= cairo
PYSIDE_PKG_CONFIG_PATH = $$system($$PYTHON_CONFIG --prefix)/lib/pkgconfig
pyside: PKGCONFIG += pyside
pyside: INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtCore
pyside: INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtGui

INCLUDEPATH += /usr/src/googletest
INCLUDEPATH += /usr/src/googletest/googletest/include
INCLUDEPATH += /usr/src/googletest/googlemock/include
#INCLUDEPATH += /usr/include/ceres
#INCLUDEPATH += /usr/include/gflags
#INCLUDEPATH += /usr/include/glog

#QMAKE_CXX = clang++
#QMAKE_LINK = clang++

#QMAKE_CXXFLAGS += -Wno-deprecated -Wno-deprecated-declarations -Wno-expansion-to-defined -std=gnu++98 -fpermissive
