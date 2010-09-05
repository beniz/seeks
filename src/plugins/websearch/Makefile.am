#automake for websearch plugin.

websearchpluginlibdir = $(libdir)/seeks/plugins/websearch
websearchplugindatadir = $(datadir)/seeks/plugins/websearch

AM_CPPFLAGS=-Wno-deprecated -Wall -g -pipe \
	    -I${srcdir} -I${srcdir}/../../utils -I${srcdir}/../../lsh -I${srcdir}/../../proxy
ACLOCAL_AMFLAGS=-I m4
websearchpluginlib_LTLIBRARIES=libseekswebsearchplugin.la
libseekswebsearchplugin_la_SOURCES=websearch.cpp se_handler.cpp search_snippet.cpp static_renderer.cpp \
				   se_parser.cpp se_parser_ggle.cpp se_parser_bing.cpp se_parser_cuil.cpp \
				   se_parser_yahoo.cpp se_parser_exalead.cpp html_txt_parser.cpp \
				   se_parser_mediawiki.cpp se_parser_twitter.cpp \
				   se_parser_identica.cpp se_parser_youtube.cpp\
				   se_parser_dailymotion.cpp \
				   query_interceptor.cpp websearch_configuration.cpp \
				   sort_rank.cpp query_context.cpp content_handler.cpp \
				   clustering.cpp oskmeans.cpp json_renderer.cpp \
				   clustering.h content_handler.h html_txt_parser.h json_renderer.h oskmeans.h \
				   query_context.h query_interceptor.h search_snippet.h se_handler.h se_parser_bing.h \
				   se_parser_cuil.h se_parser_exalead.h se_parser_ggle.h se_parser.h se_parser_yahoo.h \
				   se_query.h sort_rank.h static_renderer.h websearch_configuration.h websearch.h
libseekswebsearchplugin_la_CXXFLAGS = -Wall -g -I${srcdir}/../../ @CURL_CFLAGS@ @XML2_CFLAGS@ -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"'

if USE_PERL
libseekswebsearchplugin_la_CXXFLAGS += `perl -MExtUtils::Embed -e ccopts`
endif

SUBDIRS=. tests

websearchconfigdir=$(sysconfdir)/seeks
dist_websearchconfig_DATA=websearch-config

nobase_dist_websearchplugindata_DATA= \
			  templates/clustered_results_template \
			  templates/one_column_results_template  \
			  templates/seeks_result_template.html \
			  templates/seeks_ws_hp.html \
			  templates/opensearch.xml \
			  templates/css/seeks_hp_search.css \
			  templates/css/seeks_search.css \
			  patterns/qi_patterns \
			  patterns/pdf \
			  patterns/audio \
			  patterns/file_doc \
			  patterns/reject \
			  patterns/forum \
			  patterns/video \
			  public/images/home_32x30.png \
			  public/images/search_cluster_on.png \
			  public/images/search_expansion_4.png \
			  public/images/search_engines/yahoo.svg \
			  public/images/search_engines/bing.svg \
			  public/images/search_engines/cuil_12x12_transparent.png \
			  public/images/search_engines/exalead.svg \
			  public/images/search_engines/exalead_12x12.png \
			  public/images/search_engines/google.svg \
			  public/images/search_engines/yahoo_12x12_transparent.png \
			  public/images/search_engines/bing_12x12_transparent.png \
			  public/images/search_engines/google_12x12.png \
			  public/images/search_engines/cuil.svg \
			  public/images/search_expansion_1.png \
			  public/images/search_expansion_5.png \
			  public/images/search_cluster_inactive.png \
			  public/images/search_expansion_2.png \
			  public/images/search_form.png \
			  public/images/search_form_hp.png \
			  public/images/search_expansion_3.png \
			  public/images/home.svg \
			  public/images/search_cluster_off.png \
			  public/images/search_page_next.png \
			  public/images/search_page_prev.png \
			  public/css/seeks_hp_search.css


