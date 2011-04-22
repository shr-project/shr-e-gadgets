MAINTAINERCLEANFILES = Makefile.in module.desktop
MODULE = battery-fso
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
                         e_mod_fso.c

module_la_LIBADD       = @ENLIGHTENMENT_LIBS@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
