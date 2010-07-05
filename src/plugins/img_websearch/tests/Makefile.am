noinst_PROGRAMS=test_bing_img_parser test_ggle_img_parser test_surf_sim

test_bing_img_parser_SOURCES=test-bing-img-parser.cpp
test_ggle_img_parser_SOURCES=test-ggle-img-parser.cpp
test_surf_sim_SOURCES=test-surf-sim.cpp

AM_CPPFLAGS=-Wall -g -Wno-deprecated -pipe -I../../websearch \
             -I../ -I../../../utils -I../../../lsh -I../../../proxy @PCRE_CFLAGS@ @XML2_CFLAGS@ \
	     `pkg-config opencv --cflags`
AM_LDFLAGS=-L../ -L../../../utils -L../../../lsh -L../../../proxy -L../../websearch `pkg-config opencv --libs`
LDADD=-lseeksimgwebsearchplugin -lseekswebsearchplugin -lseekslsh -lseeksutils -lseeksproxy -lseeksplugins @XML2_LDADD@ @CURL_LDADD@ -lpthread -lpcreposix -lpcre -lz