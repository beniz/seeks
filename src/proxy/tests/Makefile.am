TESTS = $(check_PROGRAMS)

bin_PROGRAMS=user_db_ops
noinst_PROGRAMS=test_curl_mget shash
check_PROGRAMS=ut_plugin_manager
if HAVE_PROTOBUF
if HAVE_TC
noinst_PROGRAMS += user_db_print user_db_clear user_db_remove user_db_find_key user_db_export
check_PROGRAMS += ut_user_db ut_urlmatch
endif
endif

ut_plugin_manager_SOURCES=ut-plugin-manager.cpp
test_curl_mget_SOURCES=test-curl-mget.cpp
shash_SOURCES=shash.cpp
ut_urlmatch_SOURCES=ut-urlmatch.cpp
if HAVE_PROTOBUF
if HAVE_TC
user_db_print_SOURCES=user-db-print.cpp
user_db_clear_SOURCES=user-db-clear.cpp
user_db_remove_SOURCES=user-db-remove.cpp
user_db_export_SOURCES=user-db-export.cpp
user_db_find_key_SOURCES=user-db-find-key.cpp
user_db_ops_SOURCES=user-db-ops.cpp
ut_user_db_SOURCES=ut-user-db.cpp
endif
endif

include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS += -I../