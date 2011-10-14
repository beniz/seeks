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
AM_CPPFLAGS += -I${srcdir}/../../query_capture -I${srcdir}/../../cf -I${srcdir}/../../uri_capture -I${srcdir}/../../udb_service \
             -I../../../proxy `pkg-config --cflags protobuf`

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

if HAVE_XSLSERIALIZER_PLUGIN
AM_CPPFLAGS += @XSLT_CFLAGS@
endif

AM_LDFLAGS=-L../ -L../../../utils -L../../../lsh -L../../../proxy -L../../websearch -L../../../dht 
if HAVE_PROTOBUF
AM_LDFLAGS += -L../../query_capture -L../../uri_capture -L../../cf -L../../udb_service \
	      `pkg-config --libs protobuf`

if HAVE_TC
AM_LDFLAGS += `pkg-config --libs tokyocabinet`
endif

if HAVE_TT
AM_LDFLAGS += `pkg-config --libs tokyotyrant`
endif
endif

if HAVE_OPENCV2
AM_LDFLAGS += `pkg-config opencv --libs`
endif

if HAVE_XSLSERIALIZER_PLUGIN
AM_LDFLAGS += -L../../xsl_serializer
endif

LDADD=-lseeksimgwebsearchplugin -lseekswebsearchplugin 
if HAVE_PROTOBUF
if HAVE_TC
LDADD += -lquerycaptureplugin -luricaptureplugin -lseeksuserdb -lcfplugin -ludbserviceplugin
endif
endif
if HAVE_XSLSERIALIZER_PLUGIN
LDADD += -lseeksxslserializerplugin @XSLT_LDADD@
endif
LDADD += -lseekslsh -lseeksutils -lseeksproxy -lseeksplugins -lseeksdht -lrmd160 @XML2_LDADD@ @CURL_LDADD@ @ICU_LDADD@ -lpthread -lpcreposix -lpcre -lz
