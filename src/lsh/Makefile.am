lib_LTLIBRARIES=libseekslsh.la
AM_CPPFLAGS=-Wall -Wno-deprecated -g -pipe \
	    -I${srcdir} -I${srcdir}/../utils/ -I${srcdir}/../proxy/
libseekslsh_la_CXXFLAGS=@PCRE_CFLAGS@
if HAVE_DHT
AM_CPPFLAGS += -I../dht
endif
libseekslsh_la_SOURCES=mrf.cpp LSHSystemHamming.cpp LSHFunction.cpp \
		       LSHUniformHashTableHamming.cpp Random.cpp \
		       lsh_configuration.cpp stopwordlist.cpp qprocess.cpp \
                       Bucket.h BucketOperations.h lsh_configuration.h \
                       LSHFunction.h LSHSystem.h LSHSystemHamming.h \
                       LSHUniformHashTable.h LSHUniformHashTableHamming.h \
                       mrf.h Random.h stopwordlist.h qprocess.h
