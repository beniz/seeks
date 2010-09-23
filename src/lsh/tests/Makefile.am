# automake for lsh

bin_PROGRAMS=test_mrf_query_160 test_qprocess

test_mrf_query_160_SOURCES=test-mrf-query-160.cpp
test_qprocess_SOURCES=test-qprocess.cpp

AM_CXXFLAGS=
AM_CPPFLAGS=-I../ -I../../utils/ -I../../proxy -I../../dht \
	       `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet`
AM_LDFLAGS=-L../ -L../../utils -L../../proxy -L../../dht `pkg-config --libs protobuf` `pkg-config --libs tokyocabinet`
LDADD=-lseekslsh -lseeksutils -lseeksproxy -lseeksuserdb -lseeksplugins -lseeksdht -lrmd160 @CURL_LDADD@ -lpcre -lpcreposix
