lib_LTLIBRARIES=libseekslsh.la
AM_CPPFLAGS=-Wall -Wno-deprecated -g -pipe \
	    -I${srcdir} -I${srcdir}/../utils/ -I${srcdir}/../proxy/ -I${srcdir}/../dht/
if HAVE_PROTOBUF
if HAVE_TC
AM_CPPFLAGS += `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet` -I../proxy/
endif
endif
libseekslsh_la_CXXFLAGS=@PCRE_CFLAGS@
libseekslsh_la_SOURCES=mrf.cpp LSHSystemHamming.cpp LSHFunction.cpp \
		       LSHUniformHashTableHamming.cpp Random.cpp \
		       lsh_configuration.cpp stopwordlist.cpp qprocess.cpp \
		       Bucket.h BucketOperations.h lsh_configuration.h \
		       LSHFunction.h LSHSystem.h LSHSystemHamming.h \
		       LSHUniformHashTable.h LSHUniformHashTableHamming.h \
		       mrf.h Random.h stopwordlist.h qprocess.h
