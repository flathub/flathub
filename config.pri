boost-serialization-lib: LIBS += -lboost_serialization
boost: LIBS += -lboost_thread -lboost_system
expat: LIBS += -lexpat
expat: PKGCONFIG -= expat
cairo: PKGCONFIG -= cairo
pyside: PYSIDE_PKG_CONFIG_PATH = $$system($$PYTHON_CONFIG --prefix)/lib/pkgconfig:$$(PKG_CONFIG_PATH)
pyside: PKGCONFIG += pyside
pyside: INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtCore
pyside: INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtGui
