#automake for udb_service plugin.

udbservicepluginlibdir = $(libdir)/seeks/plugins/udb_service
udbserviceplugindatadir = $(datadir)/seeks/plugins/udb_service

AM_CXXFLAGS=-Wall -g -pipe
AM_CPPFLAGS=-I${srcdir} -I${srcdir}/../../utils -I${srcdir}/../../proxy -I${srcdir}/../../lsh -I${srcdir}/../../dht \
            -I${srcdir}/../websearch -I${srcdir}/../query_capture -I${srcdir}/../uri_capture \
            -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet` \
            @PCRE_CFLAGS@ -I../../proxy/

ACLOCAL_AMFLAGS=-I m4

udbservicepluginlib_LTLIBRARIES=libudbserviceplugin.la
libudbserviceplugin_la_SOURCES=udb_service.cpp udb_server.cpp udb_client.cpp

udbservicepluginconfigdir = $(sysconfdir)/seeks

SUBDIRS=. tests