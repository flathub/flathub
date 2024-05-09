#****************************************************************************
#
# Makefile for TinyXml library.
# Lee Thomason
# www.grinninglizard.com
#
# Process with automake to turn into the real Makefile
#****************************************************************************

ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES       = libtinyxml.la
libtinyxml_la_CXXFLAGS = -fvisibility-inlines-hidden
libtinyxml_la_CPPFLAGS = -include $(top_srcdir)/config.h
libtinyxml_la_SOURCES = tinyxml.cpp tinystr.cpp tinyxmlerror.cpp tinyxmlparser.cpp
libtinyxml_la_LDFLAGS = -Wl,-Bsymbolic-functions -Wl,-z,relro,-z,now -no-undefined -version-info 0:0:0
include_HEADERS       = tinyxml.h tinystr.h

check_PROGRAMS  = xmltest
xmltest_SOURCES = xmltest.cpp

if DEBUG
   AM_CFLAGS    := -Wall -Wno-format -g -DDEBUG
   AM_CXXFLAGS  := ${AM_CFLAGS}
   AM_LDFLAGS   := -g
else
   AM_CFLAGS    := -Wall -Wno-unknown-pragmas
   AM_CXXFLAGS  := ${AM_CFLAGS}
   AM_LDFLAGS   :=
endif

if PROFILE
   AM_CFLAGS   := -pg -O3
   AM_CXXFLAGS := -pg -O3
   AM_ LDFLAGS := -pg
endif

if TINYXML_USE_STL
    libtinyxml_la_CPPFLAGS += -DTIXML_USE_STL=1
endif

EXTRA_DIST = changes.txt readme.txt tinyxml_lib.dsp tinyxmlSTL.dsp tinyXmlTestSTL.vcproj \
	     utf8test.xml tinyxml.dsw tinyxml_lib.vcproj tinyxmlSTL.vcproj \
	     tinyXmlTest.vcproj utf8test.gif echo.dsp tinyxml.sln tinyXmlTestSTL.dsp  \
	     utf8testverify.xml use_stl_def

