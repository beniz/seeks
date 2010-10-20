check_PROGRAMS = 

check_PROGRAMS += test_json_renderer
test_json_renderer_SOURCES = test-json-renderer.cpp

TESTS = $(check_PROGRAMS)

noinst_PROGRAMS=test_ggle_parser test_bing_parser test_cuil_parser test_yahoo_parser test_exalead_parser \
	        test_html_txt_parser test_twitter_parser test_youtube_parser test_dailymotion_parser test_yauba_parser

test_ggle_parser_SOURCES=test-ggle-parser.cpp
test_twitter_parser_SOURCES=test-twitter-parser.cpp
test_youtube_parser_SOURCES=test-youtube-parser.cpp
test_bing_parser_SOURCES=test-bing-parser.cpp
test_cuil_parser_SOURCES=test-cuil-parser.cpp
test_yauba_parser_SOURCES=test-yauba-parser.cpp
test_dailymotion_parser_SOURCES=test-dailymotion-parser.cpp
test_yahoo_parser_SOURCES=test-yahoo-parser.cpp
test_exalead_parser_SOURCES=test-exalead-parser.cpp
test_html_txt_parser_SOURCES=test-html-text-parser.cpp

AM_CPPFLAGS=-Wall -g -Wno-deprecated -pipe \
	     -I${srcdir} -I${srcdir}/../ -I${srcdir}/../../../utils -I${srcdir}/../../../lsh -I${srcdir}/../../../proxy \
	     -I${srcdir}/../../../dht -I${srcdir}/../../query_capture -I${srcdir}/../../cf @PCRE_CFLAGS@ @XML2_CFLAGS@ \
		$(GTEST_CPPFLAGS)
if HAVE_PROTOBUF
if HAVE_TC
AM_CPPFLAGS += `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet`
endif
endif
AM_LDFLAGS=-L../ -L../../../utils -L../../../lsh -L../../../proxy -L../../../dht $(GTEST_LDFLAGS)
if HAVE_PROTOBUF
if HAVE_TC
AM_LDFLAGS += -L../../query_capture -L../../cf `pkg-config --libs protobuf` `pkg-config --libs tokyocabinet`
endif
endif
if USE_PERL
 AM_LDFLAGS += `perl -MExtUtils::Embed -e ldopts`
endif
LDADD=-lseekswebsearchplugin 
if HAVE_PROTOBUF
if HAVE_TC
LDADD += -lquerycaptureplugin -lcfplugin -lseeksuserdb
endif
endif
LDADD += -lseekslsh -lseeksutils -lseeksproxy -lseeksplugins -lseeksdht -lrmd160 @XML2_LDADD@ @CURL_LDADD@ -lpthread -lpcreposix -lpcre -lz @LCOV_LIBS@ $(GTEST_LIBS)
