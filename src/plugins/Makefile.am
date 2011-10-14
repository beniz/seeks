SUBDIRS=
if HAVE_TC
if HAVE_PROTOBUF
SUBDIRS += query_capture uri_capture cf udb_service
endif
endif
if HAVE_XSLSERIALIZER_PLUGIN
SUBDIRS+=xsl_serializer
endif
SUBDIRS += websearch_api_compat websearch blocker no_tracking
if HAVE_IMG_WEBSEARCH_PLUGIN
SUBDIRS+=img_websearch
endif
if HAVE_HTTPSERV_PLUGIN
SUBDIRS+=httpserv
endif
SUBDIRS += websearch/tests
if HAVE_IMG_WEBSEARCH_PLUGIN
SUBDIRS += img_websearch/tests
endif
if HAVE_PROTOBUF
SUBDIRS += query_capture/tests
SUBDIRS += udb_service/tests
endif

