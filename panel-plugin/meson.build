newtonbutton_resources_xml = 'newtonbutton.gresource.xml'
newtonbutton_config_ui_file = 'newtonbutton-dialog.ui'
newtonbutton_force_quit_ui_file = 'newtonbutton-force-quit-dialog.ui'

newtonbutton_resources_compiled = gnome.compile_resources('newtonbutton-ui-resources',
  newtonbutton_resources_xml,
  source_dir: meson.current_source_dir(),
  c_name: 'newtonbutton_ui',
  dependencies: [
    files(newtonbutton_config_ui_file),
    files(newtonbutton_force_quit_ui_file) 
  ]
)

plugin_sources = [
  'newtonbutton-dialogs.c',
  'newtonbutton-dialogs.h',
  'newtonbutton-force-quit-dialog.c',
  'newtonbutton-force-quit-dialog.h',
  'newtonbutton.c',
  'newtonbutton.h',
  xfce_revision_h,
  newtonbutton_resources_compiled
]

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

exo_dep = dependency('exo-2', version: '>=0.12.0', required: true)
libwnck_dep = dependency('libwnck-3.0', version: '>=3.0', required: true)

plugin_lib = shared_module(
  'newtonbutton',
  plugin_sources,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfce4-newtonbutton-plugin'),
    '-DWNCK_I_KNOW_THIS_IS_UNSTABLE'
  ],
  include_directories: [
    include_directories('..')
  ],
  dependencies: [
    glib,
    gtk,
    libxfce4panel,
    libxfce4ui,
    libxfce4util,
    exo_dep,
    libwnck_dep 
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('libdir') / plugin_install_subdir
)

i18n.merge_file(
  input: 'newtonbutton.desktop.in',
  output: 'newtonbutton.desktop',
  po_dir: '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir
)