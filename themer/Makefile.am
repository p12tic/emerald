## Process this file with automake to produce Makefile.in
emerald_theme_manager_SOURCES = main.cc
emerald_theme_manager_LDADD = $(EMERALD_LIBS) ../libengine/libemeraldengine.la

bin_PROGRAMS = emerald-theme-manager

AM_CXXFLAGS = -fpermissive -std=c++11

AM_CPPFLAGS = @EMERALD_CFLAGS@ \
		   -DPIXMAPS_DIR=\""$(datadir)/pixmaps"\"\
		   -DDATA_DIR=\""$(datadir)"\"\
		   -DLOCALEDIR="\"@datadir@/locale"\"\
		   -I$(top_srcdir)/include
