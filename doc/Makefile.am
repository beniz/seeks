# automake for seeks doc

dist_man_MANS=

if HAVE_DOCBOOK2X
dist_man_MANS += seeks.1
endif

EXTRA_DIST=seeks.1.docbook
seeks.1:
	docbook2x-man ${srcdir}/seeks.1.docbook
