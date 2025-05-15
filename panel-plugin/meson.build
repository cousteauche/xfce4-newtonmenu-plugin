plugin_sources = [
  'newtonbutton-dialogs.c',
  'newtonbutton-dialogs.h',
  'newtonbutton.c',
  'newtonbutton.h',
  xfce_revision_h,
]

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

plugin_lib = shared_module(
  'newtonbutton',
  plugin_sources,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfce4-newtonbutton-plugin'),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    glib,
    gtk,
    libxfce4panel,
    libxfce4ui,
    libxfce4util,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('libdir') / plugin_install_subdir,
)

i18n.merge_file(
  input: 'newtonbutton.desktop.in',
  output: 'newtonbutton.desktop',
  po_dir: '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)
