nem_sources = [
  'main.c',
  'main-window.c',
  'message-item.c',
  'register-window.c',
  'user-item.c',
  'cert.c'
]

nem_deps = [
  dependency('gio-2.0', version: '>= 2.50'),
  dependency('gtk4', version: '>= 4.2.1'),
  dependency('openssl'),
  dependency('json-glib-1.0'),
  dependency('gstreamer-1.0'),

]

gnome = import('gnome')

nem_sources += gnome.compile_resources('nem-resources',
  'nem.gresource.xml',
  c_name: 'nem'
)

executable('nem', nem_sources,
  dependencies: nem_deps,
  install: true,
)
