dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.50])

m4_define([purple_lt_current], [2])
m4_define([purple_major_version], [2])
m4_define([purple_minor_version], [2])
m4_define([purple_micro_version], [0])
m4_define([purple_version_suffix], [])
m4_define([purple_version],
          [purple_major_version.purple_minor_version.purple_micro_version])
m4_define([purple_display_version], purple_version[]m4_ifdef([purple_version_suffix],[purple_version_suffix]))

AC_INIT(pidgin, [purple_display_version], [hansun.lee@gmail.com])

AC_CANONICAL_SYSTEM
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

PURPLE_MAJOR_VERSION=purple_major_version
PURPLE_MINOR_VERSION=purple_minor_version
PURPLE_MICRO_VERSION=purple_micro_version
PURPLE_VERSION=[purple_display_version]
AC_SUBST(PURPLE_MAJOR_VERSION)
AC_SUBST(PURPLE_MINOR_VERSION)
AC_SUBST(PURPLE_MICRO_VERSION)
AC_SUBST(PURPLE_VERSION)

PURPLE_LT_VERSION_INFO="purple_lt_current:purple_micro_version:purple_minor_version"
AC_SUBST(PURPLE_LT_VERSION_INFO)

AC_PATH_PROG(sedpath, sed)

dnl Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
LIBTOOL="$LIBTOOL --silent"
AC_PROG_INSTALL
PKG_PROG_PKG_CONFIG

AM_GLIB_GNU_GETTEXT

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(arpa/nameser_compat.h fcntl.h sys/time.h unistd.h locale.h signal.h stdint.h regex.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_STRUCT_TM
AC_CHECK_SIZEOF(time_t, ,[
#include <stdio.h>
#include <time.h>])

AC_C_BIGENDIAN

dnl Checks for library functions.
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_CHECK_FUNCS(strdup strstr atexit setlocale)
dnl Checks for getopt in standard library
AC_CHECK_FUNCS(getopt_long,,
[
        AC_LIBOBJ(getopt)
        AC_LIBOBJ(getopt1)
])
AC_CHECK_LIB(resolv, __res_query)
AC_CHECK_LIB(nsl, gethostent)

dnl #######################################################################
dnl # Check for GLib 2.0 (required)
dnl #######################################################################
PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.0.0 gobject-2.0 gmodule-2.0 gthread-2.0], , [
        AC_MSG_RESULT(no)
        AC_MSG_ERROR([

You must have the GLib 2.0 development headers installed to build.
])])
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

AM_CONDITIONAL(STATIC_NATEON, test "x$static_nateon" = "xyes")

AC_ARG_ENABLE(plugins, [AC_HELP_STRING([--disable-plugins], [compile without plugin support])], , enable_plugins=yes)

##if test "x$enable_debug" = "xyes" ; then
##        AC_DEFINE(DEBUG, 1, [Define if debugging is enabled.])
##fi
##
##if test "x$enable_fatal_asserts" = "xyes" ; then
##        AC_DEFINE(PURPLE_FATAL_ASSERTS, 1, [Define to make assertions fatal (useful for debugging).])
##fi
##
##if test "x$enable_deprecated" = "xno"; then
##        DEBUG_CFLAGS="$DEBUG_CFLAGS -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
##fi

if test "x$enable_plugins" = "xyes" ; then
	AC_DEFINE(PURPLE_PLUGINS, 1, [Define if plugins are enabled.])
	AM_CONDITIONAL(PLUGINS, true)
else
	AM_CONDITIONAL(PLUGINS, false)
fi

##AC_ARG_ENABLE(fortify, [AC_HELP_STRING([--disable-fortify], [compile without FORTIFY_SOURCE support])], , enable_fortify=yes)
				
extern_init=
AC_DEFINE_UNQUOTED(STATIC_PROTO_INIT, $extern_init static void static_proto_init() { $load_proto },
	[Loads static protocol plugin module initialization functions.])

if test "x$GCC" = "xyes"; then
	dnl We enable -Wall later.
	dnl If it's set after the warning CFLAGS in the compiler invocation, it counteracts the -Wno... flags.
	dnl This leads to warnings we don't want.
	CFLAGS=`echo $CFLAGS |$sedpath 's/-Wall//'`

	dnl ENABLE WARNINGS SUPPORTED BY THE VERSION OF GCC IN USE
	dnl
	dnl Future Possibilities
	dnl
	dnl Consider adding -Wbad-function-cast.
	dnl	This leads to spurious warnings using GPOINTER_TO_INT(), et al. directly on a function call.
	dnl		We'd need an intermediate variable.
	dnl
	dnl Consider adding -Wfloat-equal.
	dnl	This leads to warnings with Perl.
	dnl 		Perhaps we could write ugly configure magic and pass -Wno-float-equal down to that subdirectory.
	dnl		On the other hand, it's probably actually broken, so maybe the Perl folks should fix that?
	dnl
	dnl Consider removing -Wno-sign-compare (from the -Wextra set) and fixing all those cases.
	dnl	This is likely non-trivial.
	dnl
	for newflag in \
			"-Waggregate-return" \
			"-Wcast-align" \
			"-Wdeclaration-after-statement" \
			"-Werror-implicit-function-declaration" \
			"-Wextra -Wno-sign-compare -Wno-unused-parameter" \
			"-Winit-self" \
			"-Wmissing-declarations" \
			"-Wmissing-prototypes" \
			"-Wnested-externs" \
			"-Wpointer-arith" \
			"-Wundef" \
	; do
		orig_CFLAGS="$CFLAGS"
		CFLAGS="$CFLAGS $newflag"
		AC_MSG_CHECKING(for $newflag option to gcc)
		AC_TRY_COMPILE([], [
			int main() {return 0;}
		], [
			AC_MSG_RESULT(yes)
			CFLAGS="$orig_CFLAGS"
			DEBUG_CFLAGS="$DEBUG_CFLAGS $newflag"
		], [
			AC_MSG_RESULT(no)
			CFLAGS="$orig_CFLAGS"
		])
	done

	DEBUG_CFLAGS="-Wall $DEBUG_CFLAGS"
	CFLAGS="-g $CFLAGS"
fi
AC_SUBST(CFLAGS)
AC_SUBST(DEBUG_CFLAGS)

AC_PATH_PROG(pidginpath, pidgin)

AC_OUTPUT([Makefile
		pixmaps/Makefile
		pixmaps/protocols/Makefile
		pixmaps/protocols/16/Makefile
		pixmaps/protocols/16/scalable/Makefile
		pixmaps/protocols/22/Makefile
		pixmaps/protocols/22/scalable/Makefile
		pixmaps/protocols/48/Makefile
		pixmaps/protocols/48/scalable/Makefile
		src/Makefile
		])
