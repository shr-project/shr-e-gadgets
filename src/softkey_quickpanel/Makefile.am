MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = shr_elm_softkey

ELMSOFTKEYHEADERS = \
              elm_softkey.h
AM_CFLAGS = "-DPACKAGE_DATA_DIR=\"$(datadir)/shr_elm_softkey\" 
            @ELM_SOFTKEY_CFLAGS@"
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

xsessiondir = $(sysconfdir)/X11/Xsession.d
xsession_SCRIPTS = data/90shr_elm_softkey

