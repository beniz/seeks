# automake file for dht.

check_PROGRAMS=ut_dhtkey
ut_dhtkey_SOURCES=ut-dhtkey.cpp

TESTS = $(check_PROGRAMS)

AM_CPPFLAGS=-Wall -g -Wno-deprecated -pipe \
            -I${srcdir}/../ -I${srcdir}/../../utils -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' \
	    $(GTEST_CPPFLAGS)
AM_LDFLAGS=-L../ -L../../utils $(GTEST_LDFLAGS)
LDADD = -lseeksdht -lrmd160 $(GTEST_LIBS)
