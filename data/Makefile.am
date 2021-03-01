BUILT_SOURCES = gschemas.compiled

gsettings_SCHEMAS = org.gnome.hexviewer.gschema.xml
@GSETTINGS_RULES@

gschemas.compiled: org.gnome.hexviewer.gschema.xml
	$(GLIB_COMPILE_SCHEMAS) .

EXTRA_DIST = $(gsettings_SCHEMAS)

desktopdir = $(datadir)/applications
dist_desktop_DATA = hexviewer.desktop

CLEANFILES = 					\
	gschemas.compiled
