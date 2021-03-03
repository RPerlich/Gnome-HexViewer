ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = $(GTK_CFLAGS)
bin_PROGRAMS = hexviewer
hexviewer_SOURCES = \
	resources.c \
	rphexview.c \
	rphexview.h \
	rphexfile.c \
	rphexfile.h \
	hexviewer_win.c \
	hexviewer_win.h \
	hexviewer_app.c \
	hexviewer_app.h \
	hexviewer_prefs.c \
	hexviewer_prefs.h
hexviewer_LDADD= @GTK_LIBS@

resources.c: hexviewer_app.gresource.xml \
	$(shell glib-compile-resources --generate-dependencies --sourcedir=. hexviewer_app.gresource.xml)
	$(AM_V_GEN)glib-compile-resources --target=$@ --generate-source --sourcedir=. $<

EXTRA_DIST = \
	app_sys_menu.ui \
	hexviewer_win.ui \
	hexviewer_prefs.ui \
	hexviewer_app.gresource.xml
