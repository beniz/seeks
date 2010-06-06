#automake for websearch plugin.

websearchpluginlibdir = $(libdir)/seeks/plugins/websearch
websearchplugindatadir = $(datadir)/seeks/plugins/websearch

AM_CPPFLAGS=-Wno-deprecated -Wall -g -pipe \
	    -I../../utils -I../../lsh -I../../proxy
ACLOCAL_AMFLAGS=-I m4
websearchpluginlib_LTLIBRARIES=libseekswebsearchplugin.la
libseekswebsearchplugin_la_SOURCES=websearch.cpp se_handler.cpp search_snippet.cpp static_renderer.cpp \
	                           se_parser.cpp se_parser_ggle.cpp se_parser_bing.cpp se_parser_cuil.cpp \
				   se_parser_yahoo.cpp se_parser_exalead.cpp html_txt_parser.cpp \
	                           query_interceptor.cpp websearch_configuration.cpp \
				   sort_rank.cpp query_context.cpp content_handler.cpp \
				   clustering.cpp oskmeans.cpp json_renderer.cpp
libseekswebsearchplugin_la_CXXFLAGS = -Wall -g -I../../ @CURL_CFLAGS@ @XML2_CFLAGS@ -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"'

if USE_PERL
libseekswebsearchplugin_la_CXXFLAGS += `perl -MExtUtils::Embed -e ccopts`
endif

SUBDIRS=. tests

websearchconfigdir=$(sysconfdir)/seeks
websearchconfig_DATA=websearch-config

nobase_websearchplugindata_DATA= \
	 	          templates/clustered_results_template \
                 	  templates/one_column_results_template  \
                 	  templates/seeks_result_template.html \
                 	  templates/seeks_ws_hp.html \
                 	  templates/opensearch.xml \
			  templates/css/seeks_hp_search.css \
                 	  templates/css/seeks_search.css \
                 	  patterns/* \
                 	  public/images/*.* \
                 	  public/images/search_engines/*.*
