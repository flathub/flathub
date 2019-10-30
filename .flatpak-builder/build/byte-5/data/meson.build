icon_sizes = ['32', '48', '64', '128']

foreach i : icon_sizes
    install_data(
        join_paths('icons', i, meson.project_name() + '.svg'),
        install_dir: join_paths(get_option('datadir'), 'icons', 'hicolor', i + 'x' + i, 'apps')
    )
    install_data(
        join_paths('icons', i, meson.project_name() + '.svg'),
        install_dir: join_paths(get_option('datadir'), 'icons', 'hicolor', i + 'x' + i + '@2', 'apps')
    )
endforeach

install_data(
    meson.project_name() + '.gschema.xml',
    install_dir: join_paths(get_option('datadir'), 'glib-2.0', 'schemas')
)

install_data(
    join_paths('resources', 'album-default-cover.svg'),
    install_dir: join_paths(get_option('datadir'), 'com.github.alainm23.byte')
)

install_data(
    join_paths('resources', 'track-default-cover.svg'),
    install_dir: join_paths(get_option('datadir'), 'com.github.alainm23.byte')
)

install_data(
    join_paths('resources', 'radio-default-cover.svg'),
    install_dir: join_paths(get_option('datadir'), 'com.github.alainm23.byte')
)

install_data(
    join_paths('resources', 'playlist-default-cover.svg'),
    install_dir: join_paths(get_option('datadir'), 'com.github.alainm23.byte')
)

install_data(
    join_paths('resources', 'artist-default-cover.svg'),
    install_dir: join_paths(get_option('datadir'), 'com.github.alainm23.byte')
)

install_data(
    join_paths('resources', 'playlist-add.svg'),
    install_dir: join_paths(get_option('datadir'), 'com.github.alainm23.byte')
)

i18n.merge_file(
  input: meson.project_name() + '.desktop.in',
  output: meson.project_name() + '.desktop',
  po_dir: join_paths(meson.source_root(), 'po', 'extra'),
  type: 'desktop',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications')
)

i18n.merge_file(
  input: meson.project_name() + '.appdata.xml.in',
  output: meson.project_name() + '.appdata.xml',
  po_dir: join_paths(meson.source_root(), 'po', 'extra'),
  install: true,
  install_dir: join_paths(get_option('datadir'), 'metainfo')
)

test (
    'Validate desktop file',
    find_program('desktop-file-validate'),
    args: join_paths(meson.current_build_dir (), meson.project_name() + '.desktop')
)

test (
    'Validate appdata file',
    find_program('appstreamcli'),
    args: ['validate', join_paths(meson.current_build_dir (), meson.project_name() + '.appdata.xml')]
)
