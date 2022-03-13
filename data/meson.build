schemas_dir = get_option('datadir') / 'glib-2.0' / 'schemas'
settings_schemas = ['org.gnome.hexviewer.gschema.xml']

install_data(settings_schemas, install_dir: schemas_dir)

# Compile the schemas in the current directory; this is only
# useful for testing
gnome = import('gnome')
gnome.compile_schemas(depend_files: files(settings_schemas))

# Re-compile gsettings
meson.add_install_script('meson_install_schema.py', schemas_dir)

# Update the Gtk icon cache
meson.add_install_script('meson_update_icon_cache.py')

# Install desktop file
install_data('hexviewer.desktop',
    install_dir: join_paths(get_option('datadir'), 'applications')
)
