#automake for image websearch plugin.

imgwebsearchpluginlibdir = $(libdir)/seeks/plugins/img_websearch
imgwebsearchplugindatadir = $(datadir)/seeks/plugins/img_websearch

AM_CPPFLAGS=-Wno-deprecated -Wall -g -pipe \
            -I${srcdir} -I${srcdir}/../../utils -I${srcdir}/../../lsh -I${srcdir}/../../proxy -I${srcdir}/../websearch `pkg-config opencv --cflags`
ACLOCAL_AMFLAGS=-I m4

imgwebsearchpluginlib_LTLIBRARIES=libseeksimgwebsearchplugin.la
libseeksimgwebsearchplugin_la_SOURCES=img_websearch.cpp se_handler_img.cpp img_query_context.cpp img_websearch_configuration.cpp \
                                      img_search_snippet.cpp se_parser_bing_img.cpp se_parser_ggle_img.cpp se_parser_flickr.cpp \
				      se_parser_yahoo_img.cpp img_sort_rank.cpp \
				      img_query_context.h img_search_snippet.h img_sort_rank.h \
				      img_websearch_configuration.h img_websearch.h se_handler_img.h \
				      se_parser_bing_img.h se_parser_ggle_img.h se_parser_flickr.h se_parser_yahoo_img.h
if HAVE_OPENCV2
libseeksimgwebsearchplugin_la_SOURCES += img_content_handler.cpp img_content_handler.h ocvsurf.cpp ocvsurf.h
endif 
libseeksimgwebsearchplugin_la_CXXFLAGS=-Wall -g -I${srcdir}/../../ @CURL_CFLAGS@ @XML2_CFLAGS@ -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"'

if USE_PERL
libseeksimgwebsearchplugin_la_CXXFLAGS += `perl -MExtUtils::Embed -e ccopts`
endif

SUBDIRS=. tests

EXTRA_DIST = templates
