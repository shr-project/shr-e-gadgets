AUTOMAKE_OPTIONS     = 1.4 foreign
MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = @edje_cc@
EDJE_FLAGS = -v -id $(top_srcdir)/src/softkey_quickpanel/data/themes -fd $(top_srcdir)/src/softkey_quickpanel/data/themes

filesdir = $(datadir)/shr_elm_softkey/themes

files_DATA = default.edj

EXTRA_DIST = \
	default.edc	\
	back.png	\
	close.png	\
	down.png	\
	forward.png	\
	up.png

default.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/src/softkey_quickpanel/data/themes/default.edc \
	$(top_builddir)/src/softkey_quickpanel/data/themes/default.edj

clean-local:
	rm -f *.edj
