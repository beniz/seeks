#automake for websearch plugin.

websearchpluginlibdir = $(libdir)/seeks/plugins/websearch
websearchplugindatadir = $(datadir)/seeks/plugins/websearch

AM_CPPFLAGS=-Wno-deprecated -Wall -g -pipe \
	    -I${srcdir} -I${srcdir}/../../utils -I${srcdir}/../../lsh -I${srcdir}/../../proxy \
	    -I${srcdir}/../../dht 
if HAVE_IMG_WEBSEARCH_PLUGIN
AM_CPPFLAGS += -I${srcdir}/../img_websearch
endif
if HAVE_PROTOBUF
AM_CPPFLAGS += -I${srcdir}/../query_capture -I${srcdir}/../cf -I../../proxy \
               `pkg-config --cflags protobuf`
if HAVE_TC
AM_CPPFLAGS += `pkg-config --cflags tokyocabinet`
endif

if HAVE_TT
AM_CPPFLAGS += `pkg-config --cflags tokyotyrant`
endif
endif

ACLOCAL_AMFLAGS=-I m4
websearchpluginlib_LTLIBRARIES=libseekswebsearchplugin.la
libseekswebsearchplugin_la_SOURCES=websearch.cpp se_handler.cpp search_snippet.cpp static_renderer.cpp \
				   se_parser.cpp se_parser_ggle.cpp se_parser_bing.cpp \
				   se_parser_yahoo.cpp se_parser_exalead.cpp html_txt_parser.cpp \
				   se_parser_twitter.cpp se_parser_osearch.cpp se_parser_youtube.cpp \
				   se_parser_dailymotion.cpp se_parser_doku.cpp se_parser_delicious.cpp se_parser_wordpress.cpp se_parser_redmine.cpp \
			           query_interceptor.cpp websearch_configuration.cpp \
				   se_parser_yauba.cpp se_parser_blekko.cpp se_parser_mediawiki.cpp \
				   sort_rank.cpp query_context.cpp content_handler.cpp \
				   clustering.cpp oskmeans.cpp json_renderer.cpp dynamic_renderer.cpp feeds.cpp \
				   clustering.h content_handler.h html_txt_parser.h json_renderer.h json_renderer_private.h oskmeans.h \
				   query_context.h query_interceptor.h search_snippet.h se_handler.h se_parser_bing.h \
				   se_parser_exalead.h se_parser_ggle.h se_parser.h se_parser_yahoo.h \
	                           se_parser_youtube.h se_parser_dailymotion.h se_parser_yauba.h se_parser_twitter.h se_parser_osearch.h \
				   se_parser_blekko.h se_parser_mediawiki.h se_parser_doku.h se_parser_delicious.h se_parser_wordpress.h se_parser_redmine.h \
	                           se_query.h sort_rank.h static_renderer.h websearch_configuration.h websearch.h \
	                           dynamic_renderer.h wb_err.h feeds.h
libseekswebsearchplugin_la_CXXFLAGS = -Wall -g -I${srcdir}/../../ @PCRE_CFLAGS@ @CURL_CFLAGS@ @XML2_CFLAGS@ @LCOV_CFLAGS@ -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"'

if USE_PERL
libseekswebsearchplugin_la_CXXFLAGS += `perl -MExtUtils::Embed -e ccopts`
endif

SUBDIRS=. tests

websearchconfigdir=$(sysconfdir)/seeks
dist_websearchconfig_DATA=websearch-config

nobase_dist_websearchplugindata_DATA= \
			  templates/clustered_results_template \
			  templates/one_column_results_template  \
                          templates/opensearch.xml \
			  patterns/qi_patterns \
			  patterns/pdf \
			  patterns/audio \
			  patterns/file_doc \
			  patterns/reject \
			  patterns/forum \
			  patterns/video \
	                  templates/js/dyn_ui.js \
			  templates/themes/original/css/seeks_hp_search.css \
	                  templates/themes/original/css/seeks_search.css \
			  templates/themes/original/seeks_result_template.html \
	                  templates/themes/original/seeks_result_template_dyn.html \
	                  templates/themes/original/seeks_ws_hp.html \
			  templates/themes/original/js/render.js \
                          templates/themes/compact/seeks_result_template.html \
                          templates/themes/compact/seeks_result_template_dyn.html \
		          templates/themes/compact/seeks_ws_hp.html \
			  templates/themes/compact/css/seeks_hp_search.css \
                          templates/themes/compact/css/seeks_search.css \
	                  templates/themes/compact/js/render.js \
			  public/images/seeks_croix_light_10x10.png \
			  public/images/search_engines/yahoo.svg \
			  public/images/search_engines/bing.svg \
			  public/images/search_engines/exalead.svg \
			  public/images/search_engines/exalead_12x12.png \
			  public/images/search_engines/google.svg \
			  public/images/search_engines/yahoo_12x12_transparent.png \
			  public/images/search_engines/bing_12x12_transparent.png \
			  public/images/search_engines/google_12x12.png \
			  public/images/search_engines/youtube_12x12.png \
                          public/images/search_engines/dailymotion_12x12.png \
			  public/images/search_engines/yauba_12x12.png \
		          public/images/search_engines/seeks_12x12.png \
			  public/images/search_engines/blekko.png \
	 		  public/images/search_engines/delicious_12x12.png \
			  public/themes/original/css/seeks_hp_search.css \
	                  public/themes/original/images/home_32x30.png \
                          public/themes/original/images/search_cluster_on.png \
                          public/themes/original/images/search_expansion_4.png \
	                  public/themes/original/images/search_expansion_1.png \
			  public/themes/original/images/search_expansion_5.png \
			  public/themes/original/images/search_cluster_inactive.png \
			  public/themes/original/images/search_expansion_2.png \
			  public/themes/original/images/search_form.png \
			  public/themes/original/images/search_form_hp.png \
			  public/themes/original/images/search_expansion_3.png \
			  public/themes/original/images/home.svg \
			  public/themes/original/images/perso_star.png \
			  public/themes/original/images/search_cluster_off.png \
			  public/themes/original/images/search_page_next.png \
			  public/themes/original/images/search_page_prev.png \
			  public/themes/compact/css/seeks_hp_search.css \
			  public/themes/compact/images/perso_star_ico_on.png \
			  public/themes/compact/images/text.png \
			  public/themes/compact/images/cluster_inactive.png \
			  public/themes/compact/images/tweet.png \
			  public/themes/compact/images/perso_star_ico_off.png \
			  public/themes/compact/images/image.png \
			  public/themes/compact/images/exp_3.png \
			  public/themes/compact/images/search_page_next.png \
			  public/themes/compact/images/types.png \
			  public/themes/compact/images/exp_1.png \
			  public/themes/compact/images/search_form_hp.png \
			  public/themes/compact/images/perso_star.png \
			  public/themes/compact/images/cluster_off.png \
			  public/themes/compact/images/cluster_on.png \
			  public/themes/compact/images/exp_2.png \
			  public/themes/compact/images/search_page_end.png \
			  public/themes/compact/images/search_page_prev.png \
			  public/themes/compact/images/pages.png \
			  public/themes/compact/images/help.png \
			  public/themes/compact/images/video.png \
			  public/themes/compact/images/home_ico.png \
			  public/themes/compact/images/search_bar.png

