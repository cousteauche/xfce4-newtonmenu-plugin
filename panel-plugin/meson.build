plugin_sources = files(
  'newtonmenu.c',
  'newtonmenu-dialogs.c',
  'newtonmenu-force-quit-dialog.c',
  'dbusmenu-integration.c'
)

newtonmenu_gresource = gnome.compile_resources(
  'newtonmenu-gresource',
  'newtonmenu.gresource.xml',
  source_dir: '.',
  c_name: 'newtonmenu'
)

plugin_c_args = [
  '-DG_LOG_DOMAIN="xfce4-newtonmenu-plugin"',
  '-DWNCK_I_KNOW_THIS_IS_UNSTABLE'
]

if appmenu_glib_translator_dep.found()
  plugin_c_args += '-DHAVE_APPMENU_GLIB_TRANSLATOR=1'
endif

plugin_deps = [
  glib,
  gtk,
  libxfce4panel,
  libxfce4ui,
  libxfce4util,
  libwnck_dep,
  libdbusmenu_glib_dep,
  appmenu_glib_translator_dep,
]

if libdbusmenu_gtk_dep.found()
  plugin_deps += libdbusmenu_gtk_dep
endif

if exo_dep.found()
  plugin_deps += exo_dep
endif

# Determine the correct libdir based on the 'libdir' option from Meson
# This will be 'lib' or 'lib64' etc. as determined by Meson for your system.
plugin_install_dir = get_option('prefix') / get_option('libdir') / 'xfce4' / 'panel' / 'plugins'
message('Plugin will be installed to: ' + plugin_install_dir) # For verification

shared_module('newtonmenu',
  sources: [plugin_sources, newtonmenu_gresource, xfce_revision_h],
  dependencies: plugin_deps,
  c_args: plugin_c_args,
  include_directories: ['.'],
  install: true,
  install_dir: plugin_install_dir 
)

desktop_file = i18n.merge_file(
  input: 'newtonmenu.desktop.in',
  output: 'newtonmenu.desktop',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: get_option('datadir') / 'xfce4' / 'panel' / 'plugins'
)