# automake for no_tracking plugin.

notrackingpluginlibdir = $(libdir)/seeks/plugins/no_tracking
notrackingplugindatadir = $(datadir)/seeks/plugins/no_tracking

AM_CXXFLAGS=-Wall -g -pipe
AM_CPPFLAGS=-I${srcdir} -I${srcdir}/../../utils  -I${srcdir}/../../proxy -I${srcdir}/../../lsh \
            -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet`
ACLOCAL_AMFLAGS=-I m4

notrackingpluginlib_LTLIBRARIES=libnotrackingplugin.la
libnotrackingplugin_la_SOURCES=no_tracking.cpp no_tracking.h

nobase_dist_notrackingplugindata_DATA=lm-patterns 