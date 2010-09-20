noinst_PROGRAMS=test_ggle_parser test_bing_parser test_cuil_parser test_yahoo_parser test_exalead_parser \
	        test_html_txt_parser test_twitter_parser test_youtube_parser test_dailymotion_parser test_yauba_parser

test_ggle_parser_SOURCES=test-ggle-parser.cpp
test_mediawiki_parser_SOURCES=test-mediawiki-parser.cpp
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
	     -I${srcdir}/../../../dht @PCRE_CFLAGS@ @XML2_CFLAGS@ `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet`
AM_LDFLAGS=-L../ -L../../../utils -L../../../lsh -L../../../proxy -L../../../dht \
	   `pkg-config --libs protobuf` `pkg-config --libs tokyocabinet`
if USE_PERL
 AM_LDFLAGS += `perl -MExtUtils::Embed -e ldopts`
endif
LDADD=-lseekswebsearchplugin -lseekslsh -lseeksutils -lseeksproxy -lseeksuserdb -lseeksplugins -lseeksdht -lrmd160 @XML2_LDADD@ @CURL_LDADD@ -lpthread -lpcreposix -lpcre -lz
