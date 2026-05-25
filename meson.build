project('imageviewer',
          version: '0.1.0',
    meson_version: '>= 1.0.0',
  default_options: [ 'warning_level=2', 'werror=false', ],
)

gettext_package = 'jp.co.masatn.ImageViewer'

i18n = import('i18n')
gnome = import('gnome')

install_data(
  'data/icons/hicolor/scalable/apps/jp.co.masatn.ImageViewer.svg',
  install_dir: join_paths(get_option('datadir'),
                          'icons/hicolor/scalable/apps')
)

subdir('data')
subdir('src')
subdir('po')

gnome.post_install(
     glib_compile_schemas: true,
    gtk_update_icon_cache: true,
  update_desktop_database: true,
)
