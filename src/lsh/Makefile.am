lib_LTLIBRARIES=libseekslsh.la
AM_CPPFLAGS=-Wall -Wno-deprecated -g -pipe \
	    -I../utils/ -I../proxy/ 
if HAVE_DHT
AM_CPPFLAGS += -I../dht
endif
libseekslsh_la_SOURCES=mrf.cpp LSHSystemHamming.cpp LSHFunction.cpp \
		       LSHUniformHashTableHamming.cpp Random.cpp \
		       lsh_configuration.cpp stopwordlist.cpp qprocess.cpp

SUBDIRS= . tests