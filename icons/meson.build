publicIcons = [
  'hicolor/16x16/apps/hexviewer.png',
  'hicolor/22x22/apps/hexviewer.png',
  'hicolor/24x24/apps/hexviewer.png',
  'hicolor/32x32/apps/hexviewer.png',
  'hicolor/48x48/apps/hexviewer.png',
  'hicolor/128x128/apps/hexviewer.png'
]

foreach icon : publicIcons
  nameParts = icon.split('/')
  theme   = nameParts[0]
  size    = nameParts[1]
  context = nameParts[2]
  file    = nameParts[3]
  install_data(icon,
    install_dir: join_paths(get_option('datadir'), 'icons', theme, size, context)
  )
endforeach
