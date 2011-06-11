TESTS = $(check_PROGRAMS)

bin_PROGRAMS = test_bqc
test_bqc_SOURCES = test-bqc.cpp

check_PROGRAMS = ut_udb_server
ut_udb_server_SOURCES = ut-udb-server.cpp

include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS += -I../../../proxy/