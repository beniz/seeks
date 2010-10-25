TESTS = $(check_PROGRAMS)

noinst_PROGRAMS=test_curl_mget shash 
if HAVE_PROTOBUF
if HAVE_TC
noinst_PROGRAMS += user_db_print user_db_clear user_db_remove user_db_find_key
check_PROGRAMS = ut_user_db
endif
endif
test_curl_mget_SOURCES=test-curl-mget.cpp 
shash_SOURCES=shash.cpp
if HAVE_PROTOBUF
if HAVE_TC
user_db_print_SOURCES=user-db-print.cpp
user_db_clear_SOURCES=user-db-clear.cpp
user_db_remove_SOURCES=user-db-remove.cpp
user_db_find_key_SOURCES=user-db-find-key.cpp
ut_user_db_SOURCES=ut-user-db.cpp
endif
endif

include $(top_srcdir)/src/Makefile.include
