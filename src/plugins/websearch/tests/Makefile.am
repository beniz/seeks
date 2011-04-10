check_PROGRAMS = ut_json_renderer ut_feeds ut_snippet ut_parser ut_se_handler ut_qc ut_websearch ut_content_handler
ut_json_renderer_SOURCES = ut-json-renderer.cpp
ut_feeds_SOURCES = ut-feeds.cpp
ut_snippet_SOURCES = ut-snippet.cpp
ut_parser_SOURCES = ut-parser.cpp
ut_se_handler_SOURCES = ut-se-handler.cpp
ut_qc_SOURCES = ut-qc.cpp
ut_websearch_SOURCES = ut-websearch.cpp
ut_content_handler_SOURCES = ut-content-handler.cpp

TESTS = $(check_PROGRAMS)

noinst_PROGRAMS=test_ggle_parser test_bing_parser test_yahoo_parser test_exalead_parser \
	        test_html_txt_parser test_twitter_parser test_youtube_parser test_dailymotion_parser \
			test_yauba_parser test_blekko_parser test_osearch_parser

test_ggle_parser_SOURCES=test-ggle-parser.cpp
test_blekko_parser_SOURCES=test-blekko-parser.cpp
test_twitter_parser_SOURCES=test-twitter-parser.cpp
test_youtube_parser_SOURCES=test-youtube-parser.cpp
test_bing_parser_SOURCES=test-bing-parser.cpp
test_yauba_parser_SOURCES=test-yauba-parser.cpp
test_dailymotion_parser_SOURCES=test-dailymotion-parser.cpp
test_yahoo_parser_SOURCES=test-yahoo-parser.cpp
test_exalead_parser_SOURCES=test-exalead-parser.cpp
test_osearch_parser_SOURCES=test-osearch-parser.cpp
test_html_txt_parser_SOURCES=test-html-text-parser.cpp

include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS += -I../../../proxy/

#LDADD += \
		 -lseekswebsearchplugin
