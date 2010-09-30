# automake for lsh

bin_PROGRAMS=test_mrf_query_160 gen_mrf_query_160

test_mrf_query_160_SOURCES=test-mrf-query-160.cpp
gen_mrf_query_160_SOURCES=gen-mrf-query-160.cpp

AM_CXXFLAGS=
AM_CPPFLAGS=-I../ -I../../utils/ -I../../proxy -I../../dht
if HAVE_PROTOBUF
if HAVE_TC
AM_CPPFLAGS += `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet`
endif
endif
AM_LDFLAGS=-L../ -L../../utils -L../../proxy -L../../dht 
if HAVE_PROTOBUF
if HAVE_TC
AM_LDFLAGS += `pkg-config --libs protobuf` `pkg-config --libs tokyocabinet`
endif
endif
LDADD=-lseekslsh -lseeksutils -lseeksproxy 
if HAVE_PROTOBUF
if HAVE_TC
LDADD += -lseeksuserdb 
endif
endif
LDADD += -lseeksplugins -lseeksdht -lrmd160 @CURL_LDADD@ -lpcre -lpcreposix
