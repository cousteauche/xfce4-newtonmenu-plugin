newtonmenu_resources_xml = 'newtonmenu.gresource.xml'
newtonmenu_config_ui_file = 'newtonmenu-dialog.ui'
newtonmenu_force_quit_ui_file = 'newtonmenu-force-quit-dialog.ui'

newtonmenu_resources_compiled = gnome.compile_resources('newtonmenu-ui-resources',
  newtonmenu_resources_xml,
  source_dir: meson.current_source_dir(),
  c_name: 'newtonmenu_ui',
  dependencies: [
    files(newtonmenu_config_ui_file),
    files(newtonmenu_force_quit_ui_file) 
  ]
)

plugin_sources = [
  'newtonmenu-dialogs.c',
  'newtonmenu-dialogs.h',
  'newtonmenu-force-quit-dialog.c',
  'newtonmenu-force-quit-dialog.h',
  'newtonmenu.c',
  'newtonmenu.h',
  xfce_revision_h,
  newtonmenu_resources_compiled
]

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

exo_dep = dependency('exo-2', version: '>=0.12.0', required: true)
libwnck_dep = dependency('libwnck-3.0', version: '>=3.0', required: true)

plugin_lib = shared_module(
  'newtonmenu',
  plugin_sources,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfce4-newtonmenu-plugin'),
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
  input: 'newtonmenu.desktop.in',
  output: 'newtonmenu.desktop',
  po_dir: '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir
)