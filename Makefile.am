SUBDIRS=src doc

if LCOV_ENABLED
all-local:
	rm -fr coverage ; mkdir coverage ; $(LCOV) -c -i -d ${srcdir} -o coverage/initial.info

check-local:
	$(LCOV) -c -d ${srcdir} -o coverage/check.info
	$(LCOV) -a coverage/initial.info -a coverage/check.info -o coverage/total.info
	$(GENHTML) -s -o coverage coverage/total.info

clean-local:
	find . \( -name '*.gcno' -o -name '*.gcda' \) | xargs rm -f
	rm -fr coverage
endif
