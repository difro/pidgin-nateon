EXTRA_DIST = \
		Makefile.mingw

pkgdir = $(libdir)/purple-$(PURPLE_MAJOR_VERSION)

NATEONSOURCES = \
	cmdproc.c \
	cmdproc.h \
	command.c \
	command.h \
	dialog.c \
	dialog.h \
	error.c \
	error.h \
	group.c \
	group.h \
	history.c \
	history.h \
	memo.c \
	memo.h \
	msg.c \
	msg.h \
	nateon.c \
	nateon.h \
	nateon-utils.c \
	nateon-utils.h \
	notification.c \
	notification.h \
	servconn.c \
	servconn.h \
	session.c \
	session.h \
	slplink.c \
	slplink.h \
	switchboard.c \
	switchboard.h \
	state.c \
	state.h \
	sync.c \
	sync.h \
	table.c \
	table.h \
	transaction.c \
	transaction.h \
	user.c \
	user.h \
	userlist.c \
	userlist.h

AM_CFLAGS = $(st)

libnateon_la_LDFLAGS = -module -avoid-version

if STATIC_NATEON

st = -DPURPLE_STATIC_PRPL
noinst_LIBRARIES = libnateon.a
libnateon_a_SOURCES = $(NATEONSOURCES)
libnateon_a_CFLAGS  = $(AM_CFLAGS)

else

st =
pkg_LTLIBRARIES   = libnateon.la
libnateon_la_SOURCES = $(NATEONSOURCES)
libnateon_la_LIBADD  = $(GLIB_LIBS)

endif

AM_CPPFLAGS = \
	-I$(top_srcdir)/libpurple \
	-I$(top_builddir)/libpurple \
	$(GLIB_CFLAGS) \
	$(DEBUG_CFLAGS)
