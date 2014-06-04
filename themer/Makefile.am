## Process this file with automake to produce Makefile.in

include $(top_srcdir)/common.am

emerald_theme_manager_SOURCES = main.cc window.cc theme_list.cc

noinst_HEADERS = window.h compat.h

emerald_theme_manager_LDFLAGS =         \
    $(BOOST_FILESYSTEM_LDFLAGS)         \
    $(BOOST_SYSTEM_LDFLAGS)

emerald_theme_manager_LDADD =           \
    $(EMERALD_LIBS)                     \
    ../libengine/libemeraldengine.la    \
    $(BOOST_FILESYSTEM_LIBS)            \
    $(BOOST_SYSTEM_LIBS)

bin_PROGRAMS = emerald-theme-manager

AM_CXXFLAGS = $(COMMON_CXXFLAGS)

AM_CPPFLAGS = @EMERALD_CFLAGS@ \
		   -DPIXMAPS_DIR=\""$(datadir)/pixmaps"\"\
		   -DDATA_DIR=\""$(datadir)"\"\
		   -DLOCALEDIR="\"@datadir@/locale"\"\
           -I$(top_srcdir)
