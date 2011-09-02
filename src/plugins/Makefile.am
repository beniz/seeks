SUBDIRS=
if HAVE_TC
if HAVE_PROTOBUF
SUBDIRS += query_capture uri_capture cf udb_service
endif
endif
SUBDIRS += websearch websearch_api_compat blocker no_tracking
if HAVE_PROTOBUF
SUBDIRS += query_capture/tests
SUBDIRS += udb_service/tests
endif
if HAVE_IMG_WEBSEARCH_PLUGIN
SUBDIRS+=img_websearch
endif
if HAVE_HTTPSERV_PLUGIN
SUBDIRS+=httpserv
endif
