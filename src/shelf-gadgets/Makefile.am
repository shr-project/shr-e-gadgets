MAINTAINERCLEANFILES = Makefile.in
MODULE = shr-e-gadgets
#manual hack until I found out how to take e's release version
release=@E_VERSION@
MODULE_ARCH=$(host_os)-$(host_cpu)-$(release)

AM_CFLAGS = -Wall @ENLIGHTENMENT_CFLAGS@ \
	    -DMODULE_ARCH=\"$(MODULE_ARCH)\" 

# data files for the module
filesdir = $(libdir)/enlightenment/modules/$(MODULE)
files_DATA = \
e-module-$(MODULE).edj module.desktop

EXTRA_DIST = $(files_DATA)

# the module .so file
INCLUDES               = -I. \
                         -I$(top_srcdir) \
                         -I$(top_srcdir)/src/modules/$(MODULE) \
                         -I$(top_srcdir)/src/bin \
                         -I$(top_srcdir)/src/lib \
                         -I$(top_srcdir)/src/modules 

pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la
module_la_SOURCES      = e_mod_main.c \
                         e_mod_main.h \
			 e_mod_gad_wifi.c \
                         e_mod_gad_wifi.h \
                         e_mod_gad_gsm.c \
		         e_mod_gad_gsm.h \
			 e_mod_gad_usb.c \
                         e_mod_gad_usb.h \
                         e_mod_gad_bluetooth.c \
		         e_mod_gad_bluetooth.h 

module_la_LIBADD       = @ENLIGHTENMENT_LIBS@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
