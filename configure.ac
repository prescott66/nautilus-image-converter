AC_INIT([nautilus-image-converter], [0.3.0], [j@bitron.ch], [nautilus-image-converter])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE([dist-bzip2])

AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC
AC_DISABLE_STATIC

AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=nautilus-image-converter
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])
AC_PROG_LIBTOOL
AC_PATH_PROG(PKG_CONFIG, pkg-config, no)

AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)

NAUTILUS_REQUIRED=2.12.0
GTK_REQUIRED=2.4.0
LIBGLADE_REQUIRED=2.4.0
GNOMEVFS_REQUIRED=2.6.0

PKG_CHECK_MODULES(NAUTILUS, libnautilus-extension >= $NAUTILUS_REQUIRED gtk+-2.0 >= $GTK_REQUIRED libglade-2.0 >= $LIBGLADE_REQUIRED gnome-vfs-2.0 >= $GNOMEVFS_REQUIRED)
AC_SUBST(NAUTILUS_CFLAGS)
AC_SUBST(NAUTILUS_LIBS)

dnl -----------------------------------------------------------
dnl Use correct nautilus extensions directory
dnl -----------------------------------------------------------
NAUTILUS_EXTENSION_DIR=`$PKG_CONFIG --variable=extensiondir libnautilus-extension`
AC_SUBST(NAUTILUS_EXTENSION_DIR)

AC_CONFIG_FILES([Makefile
           src/Makefile
           po/Makefile.in])

AC_OUTPUT
