AC_PREREQ(2.53)
AC_INIT(shr-e-gadgets, 0.1, http://www.openmoko.org/)
AC_CONFIG_SRCDIR(src/)
AM_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE()

AC_PROG_CC
AC_GNU_SOURCE
AC_PROG_LIBTOOL

AC_ISC_POSIX
AC_STDC_HEADERS

PKG_CHECK_MODULES(ENLIGHTENMENT, enlightenment)
EFL_WITH_BIN([edje], [edje-cc], [edje_cc])
PKG_CHECK_MODULES(ELM_SOFTKEY, [
  ecore-x
  efreet
  elementary
])

AC_SUBST(DATADIR, $datadir)
AC_SUBST(BINDIR, $bindir)
AC_SUBST(E_VERSION, `pkg-config --variable=release enlightenment`)

AC_OUTPUT([
Makefile
src/Makefile
src/softkey_quickpanel/Makefile
src/softkey_quickpanel/data/Makefile
src/softkey_quickpanel/data/themes/Makefile
src/shelf-gadgets/Makefile
src/shelf-gadgets/module.desktop
src/illume-rightclick-toggle/Makefile
src/illume-rightclick-toggle/module.desktop
])

