project('hexviewer', 'c',
    version : '0.6',
    license : ['GNU General Public License'],
    meson_version : '>= 0.50.0',
    default_options : [ 'warning_level=3', 'buildtype=debugoptimized', 'prefix=/usr']
)

gtkdep = dependency('gtk+-3.0')

project_sources = []

subdir('data')
subdir('icons')
subdir('src')

hexviewer_bin = executable('hexviewer', 
  main_source, 
  resources,
  install:true, 
  dependencies : gtkdep)

