gnome = import('gnome')
resource_files = files('hexviewer_app.gresource.xml')
resources = gnome.compile_resources('resources', resource_files)

main_source = files(
	'rphexview.c',
	'rphexview.h',
	'rphexfile.c',
	'rphexfile.h',
	'hexviewer_win.c',
	'hexviewer_win.h',
	'hexviewer_app.c',
	'hexviewer_app.h',
	'hexviewer_prefs.c',
	'hexviewer_prefs.h'
	)
project_sources += main_source
