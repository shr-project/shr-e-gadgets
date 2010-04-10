AC_PREREQ(2.53)
AC_INIT(shr-e-gadgets, 0.1, http://www.openmoko.org/)
AM_INIT_AUTOMAKE()
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_GNU_SOURCE
AC_ISC_POSIX
AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(ENLIGHTENMENT, enlightenment)
AC_SUBST(DATADIR, $datadir)
AC_SUBST(BINDIR, $bindir)
AC_SUBST(E_VERSION, `pkg-config --variable=release enlightenment`)

AC_OUTPUT([
Makefile
src/Makefile
src/module.desktop
src/illume-rightclick-toggle/Makefile
src/illume-rightclick-toggle/module.desktop
])

