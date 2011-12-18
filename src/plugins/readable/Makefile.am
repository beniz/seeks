#automake for readable plugin.

readablepluginlibdir = $(libdir)/seeks/plugins/readable
readableplugindatadir = $(datadir)/seeks/plugins/readable

AM_CFLAGS=-std=c99
AM_CXXFLAGS=-Wall -g -pipe
AM_CPPFLAGS=-I${srcdir} -I${srcdir}/../../utils -I${srcdir}/../../proxy -I${srcdir}/../../lsh -I${srcdir}/../../dht \
            -I${srcdir}/../websearch -I${srcdir}/../query_capture -I${srcdir}/../uri_capture -I${srcdir}/../cf \
            -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet` \
            @PCRE_CFLAGS@ @XML2_CFLAGS@ -I../../proxy/

ACLOCAL_AMFLAGS=-I m4

readablepluginlib_LTLIBRARIES=libreadableplugin.la
libreadableplugin_la_SOURCES=rdbl_pl.cpp readable.c rd_list.c rdbl_pl.h rd_list.h khash.h

#readablepluginconfigdir = $(sysconfdir)/seeks
#dist_readablepluginconfig_DATA=readable-config

SUBDIRS=. tests