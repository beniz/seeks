#automake for adfilter plugin.

adfilterpluginlibdir = $(libdir)/seeks/plugins/adfilter
adfilterplugindatadir = $(datadir)/seeks/plugins/adfilter

AM_CXXFLAGS=-Wall -g -pipe
AM_CPPFLAGS= -I${srcdir} -I${srcdir}/../../utils -I${srcdir}/../../proxy -I${srcdir}/../../lsh \
	    -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet` \
	    -I../../proxy
ACLOCAL_AMFLAGS=-I m4

adfilterpluginlib_LTLIBRARIES=libadfilterplugin.la
libadfilterplugin_la_SOURCES=adfilter.cpp adfilter.h adfilter_configuration.cpp adfilter_configuration.h adblock_parser.cpp adblock_parser.h adfilter_element.cpp adfilter_element.h adblocker_element.cpp adblocker_element.h adblock_downloader.cpp adblock_downloader.h
libadfilterplugin_la_CXXFLAGS=-Wall -g -I${srcdir}/../../ @XML2_CFLAGS@ @PCRE_CFLAGS@ -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"'
libadfilterplugin_la_LDFLAGS=@XML2_LDADD@ @PCRE_LDADD@

nobase_dist_adfilterplugindata_DATA=adfilter-config
