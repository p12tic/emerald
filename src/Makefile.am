## Process this file with automake to produce Makefile.in
AM_CXXFLAGS = -fpermissive -std=c++11
emerald_LDADD   = @EMERALD_LIBS@ $(top_builddir)/libengine/libemeraldengine.la
emerald_SOURCES = main.cc \
               engine_loader.cc
emerald_program = emerald

AM_CPPFLAGS = @EMERALD_CFLAGS@ -I$(top_srcdir)/include \
		   -DLOCALEDIR="\"@datadir@/locale"\"\
		   -DENGINE_DIR=\"$(enginesdir)\"

bin_PROGRAMS = $(emerald_program)
