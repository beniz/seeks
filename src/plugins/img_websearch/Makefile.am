#automake for image websearch plugin.

imgwebsearchpluginlibdir = $(libdir)/seeks/plugins/img_websearch
imgwebsearchplugindatadir = $(datadir)/seeks/plugins/img_websearch

AM_CPPFLAGS=-Wno-deprecated -Wall -g -pipe \
            -I${srcdir} -I${srcdir}/../../utils -I${srcdir}/../../lsh -I${srcdir}/../../proxy -I${srcdir}/../websearch \
	    -I${srcdir}/../../dht 
if HAVE_PROTOBUF
AM_CPPFLAGS += -I${srcdir}/../query_capture -I../../proxy \
	       `pkg-config --cflags protobuf`

if HAVE_TC
AM_CPPFLAGS += `pkg-config --cflags tokyocabinet`
endif

if HAVE_TT
AM_CPPFLAGS += `pkg-config --cflags tokyotyrant`
endif
endif

if HAVE_OPENCV2
AM_CPPFLAGS += `pkg-config opencv --cflags`
endif
ACLOCAL_AMFLAGS=-I m4

imgwebsearchpluginlib_LTLIBRARIES=libseeksimgwebsearchplugin.la
libseeksimgwebsearchplugin_la_SOURCES=img_websearch.cpp se_handler_img.cpp img_query_context.cpp img_websearch_configuration.cpp \
                                      img_search_snippet.cpp se_parser_bing_img.cpp se_parser_ggle_img.cpp se_parser_flickr.cpp \
				      se_parser_yahoo_img.cpp se_parser_wcommons.cpp img_sort_rank.cpp \
				      img_query_context.h img_search_snippet.h img_sort_rank.h \
				      img_websearch_configuration.h img_websearch.h se_handler_img.h se_parser_wcommons.h \
				      se_parser_bing_img.h se_parser_ggle_img.h se_parser_flickr.h se_parser_yahoo_img.h
if HAVE_OPENCV2
libseeksimgwebsearchplugin_la_SOURCES += img_content_handler.cpp img_content_handler.h ocvsurf.cpp ocvsurf.h
endif 
libseeksimgwebsearchplugin_la_CXXFLAGS=-Wall -g -I${srcdir}/../../ @PCRE_CFLAGS@ @CURL_CFLAGS@ @XML2_CFLAGS@ @ICU_CFLAGS@ -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"'

if USE_PERL
libseeksimgwebsearchplugin_la_CXXFLAGS += `perl -MExtUtils::Embed -e ccopts`
endif

SUBDIRS=. tests

imgwebsearchpluginconfigdir=$(sysconfdir)/seeks
dist_imgwebsearchpluginconfig_DATA=img-websearch-config

nobase_dist_imgwebsearchplugindata_DATA= \
	templates/themes/original/css/seeks_img_search.css \
	templates/themes/original/seeks_result_template.html \
	templates/themes/compact/css/seeks_img_search.css \
	templates/themes/compact/seeks_result_template.html \
	public/images/search_engines/wcommons_12x12.png \
	public/images/search_engines/flickr_12x12.png
