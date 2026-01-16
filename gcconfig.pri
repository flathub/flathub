CONFIG += release
QMAKE_LEX = flex
QMAKE_YACC = bison
QMAKE_MOVE = cp
LIBZ_LIBS = -lz

# GSL
GSL_INCLUDES = /app/include
GSL_LIBS = -L/app/lib -lgsl -lgslcblas -lm

# Python 3.13 from KDE 6.10 SDK
DEFINES += GC_WANT_PYTHON
PYTHONINCLUDES = -I/usr/include/x86_64-linux-gnu/python3.13 -I/usr/include/python3.13
PYTHONLIBS = -L/usr/lib/x86_64-linux-gnu -lpython3.13 -ldl -lm

# LIBUSB for ANT+ trainers
LIBUSB_INSTALL = /usr
LIBUSB_LIBS = -lusb-1.0 -ldl -ludev
LIBUSB_USE_V_1 = true

# iCalendar support
ICAL_INSTALL = /usr

# Sample rate conversion
SAMPLERATE_INSTALL = /usr

# SRMIO - SRM power meter support
SRMIO_INSTALL = /app

# Video support (Qt6)
DEFINES += GC_VIDEO_QT6

# HTTP Server
HTPATH = ../httpserver

# TrainerDay API
DEFINES += GC_WANT_TRAINERDAY_API
DEFINES += GC_TRAINERDAY_API_PAGESIZE=25

# R language support
DEFINES += GC_WANT_R

# Qt includes
QMAKE_INCDIR += /app/include
QMAKE_INCDIR += /app/include/QtWebEngine
QMAKE_INCDIR += /app/include/QtWebEngineCore
QMAKE_INCDIR += /app/include/QtWebEngineWidgets
