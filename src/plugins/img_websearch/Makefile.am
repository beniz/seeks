#automake for image websearch plugin.

imgwebsearchpluginlibdir = $(libdir)/seeks/plugins/img_websearch
imgwebsearchplugindatadir = $(datadir)/seeks/plugins/img_websearch

AM_CPPFLAGS=-Wno-deprecated -Wall -g -pipe \
            -I../../utils -I../../lsh -I../../proxy -I../websearch `pkg-config opencv --cflags`
ACLOCAL_AMFLAGS=-I m4

imgwebsearchpluginlib_LTLIBRARIES=libseeksimgwebsearchplugin.la
libseeksimgwebsearchplugin_la_SOURCES=img_websearch.cpp se_handler_img.cpp img_query_context.cpp img_websearch_configuration.cpp \
                                      img_search_snippet.cpp se_parser_bing_img.cpp se_parser_ggle_img.cpp img_sort_rank.cpp \
				      img_content_handler.cpp ocvsurf.cpp
libseeksimgwebsearchplugin_la_CXXFLAGS=-Wall -g -I../../ @CURL_CFLAGS@ @XML2_CFLAGS@ -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"'

if USE_PERL
libseeksimgwebsearchplugin_la_CXXFLAGS += `perl -MExtUtils::Embed -e ccopts`
endif

SUBDIRS=. tests