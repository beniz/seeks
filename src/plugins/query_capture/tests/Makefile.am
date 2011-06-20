TESTS = $(check_PROGRAMS)

check_PROGRAMS = ut_query_capture
ut_query_capture_SOURCES = ut-query-capture.cpp

bin_PROGRAMS = test_dbqr_compression
test_dbqr_compression_SOURCES = test-dbqr-compression.cpp

include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS += -I../../../proxy/ -I../../websearch