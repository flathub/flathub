project('io.github.Pavle012.Skakavi-krompir',
  version: '2.0.2',
  meson_version: '>= 0.62.0'
)

python = import('python').find_installation('python3')
pkgdata_dir = get_option('datadir') / 'skakavi-krompir'

# List of all python files to install
python_files = [
  'main.py',
  'achievements.py',
  'dependencies.py',
  'fake_player.py',
  'gui.py',
  'modloader.py',
  'multiplayer.py',
  'namecheck.py',
  'options.py',
  'particles.py',
  'powerups.py',
  'pygame_ui.py',
  'replays.py',
  'scores.py',
  'updater.py',
]

install_data(python_files, install_dir: pkgdata_dir)
install_subdir('assets', install_dir: pkgdata_dir)

# Install desktop file, metainfo, and icon
install_data('io.github.Pavle012.Skakavi-krompir.desktop', install_dir: get_option('datadir') / 'applications')
install_data('io.github.Pavle012.Skakavi-krompir.metainfo.xml', install_dir: get_option('datadir') / 'metainfo')

# Icon
install_data('assets/io.github.Pavle012.Skakavi-krompir.png',
  install_dir: get_option('datadir') / 'icons/hicolor/256x256/apps/',
  rename: 'io.github.Pavle012.Skakavi-krompir.png'
)

# Launch script header
conf = configuration_data()
conf.set('PYTHON', python.full_path())
conf.set('PKG_DATA_DIR', pkgdata_dir)

configure_file(
  input: 'skakavi-krompir.sh.in',
  output: 'skakavi-krompir',
  configuration: conf,
  install_dir: get_option('bindir')
)
