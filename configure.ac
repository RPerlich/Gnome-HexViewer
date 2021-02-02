AC_INIT([HexViewer], 0.4, [rp990@mailbox.org], [hexviewer])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS(src/config.h)
AM_INIT_AUTOMAKE
AC_PROG_CC
AC_PROG_INSTALL
PKG_CHECK_MODULES(GTK, gtk+-3.0)
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

cat << EOF

Your build configuration:

  Prefix         : $prefix
  Package        : $PACKAGE_NAME
  Version        : $VERSION
  Compiler flags : $CFLAGS
  Linker flags   : $LIBS $LDFLAGS
  UTF-8 support  : $utf8
  Bugs           : $PACKAGE_BUGREPORT

EOF
