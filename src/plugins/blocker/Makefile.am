#automake for blocker plugin.

blockerpluginlibdir = $(libdir)/plugins/blocker
blockerplugindatadir = $(datadir)/plugins/blocker

AM_CPPFLAGS=
AM_CXXFLAGS=-Wno-deprecated -Wall -g -pipe \
            -I../../utils -I../../proxy -I../../lsh \
	    -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"'
ACLOCAL_AMFLAGS=-I m4

blockerpluginlib_LTLIBRARIES=libblockerplugin.la
libblockerplugin_la_SOURCES=blocker.cpp 

nobase_blockerplugindata_DATA=blocked-patterns