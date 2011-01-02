#automake for blocker plugin.

blockerpluginlibdir = $(libdir)/seeks/plugins/blocker
blockerplugindatadir = $(datadir)/seeks/plugins/blocker

AM_CPPFLAGS=-Wno-deprecated -Wall -g -pipe \
            -I${srcdir} -I${srcdir}/../../utils -I${srcdir}/../../proxy -I${srcdir}/../../lsh \
	    -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"'
if HAVE_DHT
AM_CPPFLAGS += -I${srcdir}/../../dht -I${srcdir}/../../dht/rpc -I${srcdir}/../../dht/rpc/protob -I${srcdir}/../../dht/protob \
	       -I${srcdir}/../../dht/sg/ -I${srcdir}/../../dht/sg/rpc -I${srcdir}/../../dht/sg/protob `pkg-config --cflags tokyocabinet`
endif
ACLOCAL_AMFLAGS=-I m4

blockerpluginlib_LTLIBRARIES=libblockerplugin.la
libblockerplugin_la_SOURCES=blocker.cpp blocker.h

nobase_dist_blockerplugindata_DATA=blocked-patterns
