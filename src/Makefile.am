## Process this file with automake to produce Makefile.in
include $(top_srcdir)/common.am

AM_CXXFLAGS = $(COMMON_CXXFLAGS)

emerald_LDADD   =                       \
    @EMERALD_LIBS@                      \
    $(top_builddir)/libengine/libemeraldengine.la   \
    $(BOOST_FILESYSTEM_LIBS)            \
    $(BOOST_SYSTEM_LIBS)

emerald_LDFLAGS =                       \
    $(BOOST_FILESYSTEM_LDFLAGS)         \
    $(BOOST_SYSTEM_LDFLAGS)

emerald_SOURCES =   \
    main.cc             \
    engine_loader.cc    \
    cairo_utils.cc      \
    decor.cc            \
    quads.cc            \
    window.cc           \
    utils.cc

noinst_HEADERS =   \
    cairo_utils.h   \
    decor.h     \
    window.h    \
    quads.h     \
    utils.h

emerald_program = emerald

AM_CPPFLAGS = @EMERALD_CFLAGS@ -I$(top_srcdir) \
		   -DLOCALEDIR="\"@datadir@/locale"\"\
		   -DENGINE_DIR=\"$(enginesdir)\"

bin_PROGRAMS = $(emerald_program)
