SUBDIRS=
if HAVE_TC
if HAVE_PROTOBUF
SUBDIRS += query_capture cf 
endif
endif
SUBDIRS += websearch blocker 
if HAVE_TC
if HAVE_PROTOBUF
SUBDIRS += uri_capture
endif
endif
if HAVE_IMG_WEBSEARCH_PLUGIN
SUBDIRS+=img_websearch
endif
if HAVE_HTTPSERV_PLUGIN
SUBDIRS+=httpserv
endif
