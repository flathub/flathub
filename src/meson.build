pkgdatadir = get_option('prefix') / get_option('datadir') / meson.project_name()
moduledir = pkgdatadir / 'imageviewer'
gnome = import('gnome')

gnome.compile_resources('imageviewer',
  'imageviewer.gresource.xml',
  source_dir: '.',
  c_name: 'imageviewer',
  gresource_bundle: true,
  install: true,
  install_dir: pkgdatadir,
)

python = import('python')

conf = configuration_data()
conf.set('PYTHON', python.find_installation('python3').full_path())
conf.set('VERSION', meson.project_version())
conf.set('localedir', get_option('prefix') / get_option('localedir'))
conf.set('pkgdatadir', pkgdatadir)

configure_file(
  input: 'imageviewer.in',
  output: 'imageviewer',
  configuration: conf,
  install: true,
  install_dir: get_option('bindir'),
  install_mode: 'rwxr-xr-x'
)

imageviewer_sources = [
  '__init__.py',
  'main.py',
  'window.py',
  'shortcuts.py',
  'viewer.py',
  'about.py',
]

install_data(imageviewer_sources, install_dir: moduledir)
