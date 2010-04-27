MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I$(top_srcdir) \
         -I$(top_srcdir)/src/bin \
         -I$(includedir) \
         @ELM_SOFTKEY_CFLAGS@

bin_PROGRAMS = shr_elm_softkey

ELMSOFTKEYHEADERS = \
              elm_softkey.h
AM_CFLAGS = -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\"
shr_elm_softkey_SOURCES = \
            $(ELMSOFTKEYHEADERS) \
            main.c

shr_elm_softkey_LDFLAGS = -export-dynamic
shr_elm_softkey_LDADD = @ELM_SOFTKEY_LIBS@

filesdir = $(datadir)/shr_elm_softkey/images
files_DATA = \
	     data/close.png \
	     data/back.png

	     EXTRA_DIST = $(files_DATA)

