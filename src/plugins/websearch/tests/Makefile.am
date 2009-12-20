bin_PROGRAMS=test_ggle_parser test_bing_parser test_cuil_parser test_html_txt_parser \
	     test_cleanup_query
test_ggle_parser_SOURCES=test-ggle-parser.cpp
test_bing_parser_SOURCES=test-bing-parser.cpp
test_cuil_parser_SOURCES=test-cuil-parser.cpp
test_html_txt_parser_SOURCES=test-html-text-parser.cpp
test_cleanup_query_SOURCES=test-cleanup-query.cpp

AM_CPPFLAGS=-Wall -g -Wno-deprecated -march=native -mtune=native -pipe \
	     -I../ -I../../../utils -I../../../lsh -I../../../proxy @PCRE_CFLAGS@ @XML2_CFLAGS@
AM_LDFLAGS=-L../ -L../../../utils -L../../../lsh -L../../../proxy
if USE_PERL
 AM_LDFLAGS += `perl -MExtUtils::Embed -e ldopts`
endif
LDADD=-lseekswebsearchplugin -lseekslsh -lseeksutils -lseeksproxy -lseeksplugins @XML2_LDADD@ @CURL_LDADD@ -lpthread -lpcreposix -lpcre -lnsl -lz
