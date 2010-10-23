#automake for blocker plugin.

blockerpluginlibdir = $(libdir)/seeks/plugins/blocker
blockerplugindatadir = $(datadir)/seeks/plugins/blocker

AM_CXXFLAGS=-Wall -g -pipe
AM_CPPFLAGS= -I${srcdir} -I${srcdir}/../../utils -I${srcdir}/../../proxy -I${srcdir}/../../lsh \
	    -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet` \
	    -I../../proxy
ACLOCAL_AMFLAGS=-I m4

blockerpluginlib_LTLIBRARIES=libblockerplugin.la
libblockerplugin_la_SOURCES=blocker.cpp blocker.h

nobase_dist_blockerplugindata_DATA=blocked-patterns
