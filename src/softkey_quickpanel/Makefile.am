MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I$(top_srcdir) \
         -I$(top_srcdir)/src/bin \
         -I$(includedir) \
         @ELM_SOFTKEY_CFLAGS@

bin_PROGRAMS = shr_elm_softkey

ELMSOFTKEYHEADERS = \
              elm_softkey.h

shr_elm_softkey_SOURCES = \
            $(ELMSOFTKEYHEADERS) \
            main.c

shr_elm_softkey_LDFLAGS = -export-dynamic
shr_elm_softkey_LDADD = @ELM_SOFTKEY_LIBS@
