lib_LTLIBRARIES=libseekslsh.la
AM_CPPFLAGS=-Wno-deprecated -g -pipe \
	    -I${srcdir} -I${srcdir}/../utils/ -I${srcdir}/../proxy/
libseekslsh_la_CXXFLAGS=-Wall -g @PCRE_CFLAGS@
libseekslsh_la_SOURCES=mrf.cpp LSHSystemHamming.cpp LSHFunction.cpp \
		       LSHUniformHashTableHamming.cpp Random.cpp \
		       lsh_configuration.cpp stopwordlist.cpp \
		       Bucket.h BucketOperations.h lsh_configuration.h \
		       LSHFunction.h LSHSystem.h LSHSystemHamming.h \
		       LSHUniformHashTable.h LSHUniformHashTableHamming.h \
		       mrf.h Random.h stopwordlist.h
