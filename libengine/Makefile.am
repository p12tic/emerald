## Process this file with automake to produce Makefile.in
include $(top_srcdir)/common.am

AM_CPPFLAGS = @EMERALD_CFLAGS@ -I$(top_srcdir) \
		   -DENGINE_DIR=\"$(enginesdir)\" \
		   -DDEFTHEMEDIR='"@datadir@/emerald/theme/"' \
		   -DDEFSETTINGSFILE='"@datadir@/emerald/settings.ini"'
AM_CXXFLAGS = $(COMMON_CXXFLAGS)
libemeraldengine_la_LIBADD =    \
    @EMERALD_LIBS@              \
    $(BOOST_FILESYSTEM_LIBS)    \
    $(BOOST_SYSTEM_LIBS)

libemeraldengine_la_LDFLAGS =   \
    $(BOOST_FILESYSTEM_LDFLAGS) \
    $(BOOST_SYSTEM_LDFLAGS)

libemeraldengine_la_SOURCES =	\
	emerald.cc 	\
	format.cc 	\
	keyfile.cc	\
	themer.cc 	\
	setting_item.cc
lib_LTLIBRARIES = libemeraldengine.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = emeraldengine.pc

emeraldinclude_HEADERS = \
	emerald.h   \
	engine.h    \
	format.h   \
	keyfile.h   \
	filesystem.h    \
	libengine.h     \
	titlebar.h  \
	setting_item.h
emeraldincludedir = $(includedir)/emerald/libengine
