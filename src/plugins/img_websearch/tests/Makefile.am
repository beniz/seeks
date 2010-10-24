noinst_PROGRAMS=test_bing_img_parser test_ggle_img_parser test_flickr_parser test_yahoo_img_parser \
                test_wcommons_parser
if HAVE_OPENCV2
noinst_PROGRAMS += test_surf_sim
endif

test_bing_img_parser_SOURCES=test-bing-img-parser.cpp
test_ggle_img_parser_SOURCES=test-ggle-img-parser.cpp
test_flickr_parser_SOURCES=test-flickr-parser.cpp
test_yahoo_img_parser_SOURCES=test-yahoo-img-parser.cpp
test_wcommons_parser_SOURCES=test-wcommons-parser.cpp
if HAVE_OPENCV2
test_surf_sim_SOURCES=test-surf-sim.cpp
endif

AM_CPPFLAGS=-Wall -g -Wno-deprecated -pipe -I${srcdir}/../../websearch \
             -I${srcdir}/../ -I${srcdir}/../../../utils -I${srcdir}/../../../lsh -I${srcdir}/../../../proxy @PCRE_CFLAGS@ @XML2_CFLAGS@ \
	     -I${srcdir}/../../../dht 
if HAVE_PROTOBUF
if HAVE_TC
AM_CPPFLAGS += -I${srcdir}/../../query_capture -I${srcdir}/../../cf \
             `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet` -I../../../proxy
endif
endif
if HAVE_OPENCV2
AM_CPPFLAGS += `pkg-config opencv --cflags`
endif
AM_LDFLAGS=-L../ -L../../../utils -L../../../lsh -L../../../proxy -L../../websearch -L../../../dht 
if HAVE_PROTOBUF
if HAVE_TC
AM_LDFLAGS += -L../../query_capture -L../../cf `pkg-config --libs protobuf` `pkg-config --libs tokyocabinet`
endif
endif
if HAVE_OPENCV2
AM_LDFLAGS += `pkg-config opencv --libs`
endif
LDADD=-lseeksimgwebsearchplugin -lseekswebsearchplugin 
if HAVE_PROTOBUF
if HAVE_TC
LDADD += -lquerycaptureplugin -lseeksuserdb -lcfplugin 
endif
endif
LDADD += -lseekslsh -lseeksutils -lseeksproxy -lseeksplugins -lseeksdht -lrmd160 @XML2_LDADD@ @CURL_LDADD@ -lpthread -lpcreposix -lpcre -lz
