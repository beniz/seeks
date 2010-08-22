#automake for blocker plugin.

blockerpluginlibdir = $(libdir)/seeks/plugins/blocker
blockerplugindatadir = $(datadir)/seeks/plugins/blocker

AM_CPPFLAGS=
AM_CXXFLAGS=-Wno-deprecated -Wall -g -pipe \
            -I${srcdir} -I${srcdir}/../../utils -I${srcdir}/../../proxy -I${srcdir}/../../lsh \
	    -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"'
ACLOCAL_AMFLAGS=-I m4

blockerpluginlib_LTLIBRARIES=libblockerplugin.la
libblockerplugin_la_SOURCES=blocker.cpp blocker.h

nobase_dist_blockerplugindata_DATA=blocked-patterns
