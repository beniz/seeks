#automake for websearch plugin.

AM_CPPFLAGS=-Wno-deprecated -Wall -g -march=native -mtune=native -pipe \
	    -I../../utils -I../../lsh -I../../proxy
ACLOCAL_AMFLAGS=-I m4
lib_LTLIBRARIES=libseekswebsearchplugin.la
libseekswebsearchplugin_la_SOURCES=websearch.cpp se_handler.cpp search_snippet.cpp static_renderer.cpp \
	                           se_parser.cpp se_parser_ggle.cpp se_parser_bing.cpp se_parser_cuil.cpp \
				   html_txt_parser.cpp \
	                           query_interceptor.cpp websearch_configuration.cpp \
				   sort_rank.cpp query_context.cpp content_handler.cpp \
				   clustering.cpp oskmeans.cpp
libseekswebsearchplugin_la_CXXFLAGS = -Wall -g -I../../ -I../../../lsh/osb-lsh/ @CURL_CFLAGS@ @XML2_CFLAGS@ 

if USE_PERL
libseekswebsearchplugin_la_CXXFLAGS += `perl -MExtUtils::Embed -e ccopts`
endif

SUBDIRS=. tests
