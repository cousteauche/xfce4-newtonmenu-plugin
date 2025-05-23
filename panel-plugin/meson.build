# Plugin sources
plugin_sources = files(
  'newtonmenu.c',
  'newtonmenu-dialogs.c',
  'newtonmenu-force-quit-dialog.c'
)

# Collect all dependencies
plugin_deps = [
  glib,
  gtk,
  libxfce4panel,
  libxfce4ui,
  libxfce4util,
  libwnck_dep,
  libdbusmenu_glib_dep,  # This provides the dbusmenu headers
  appmenu_glib_translator_dep  # This provides the menu translation functionality
]

# Add optional dependencies
if exo_dep.found()
  plugin_deps += exo_dep
endif

# Build the plugin
shared_module('newtonmenu',
  sources: plugin_sources + [xfce_revision_h],
  dependencies: plugin_deps,
  install: true,
  install_dir: libxfce4panel.get_pkgconfig_variable('plugindir'),
  c_args: [
    '-DG_LOG_DOMAIN="xfce4-newtonmenu-plugin"',
    '-DWNCK_I_KNOW_THIS_IS_UNSTABLE'
  ]
)

# Desktop file
desktop_file = i18n.merge_file(
  input: 'newtonmenu.desktop.in',
  output: 'newtonmenu.desktop',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: get_option('datadir') / 'xfce4' / 'panel' / 'plugins'
)

# If you have a glade file for dialogs (optional)
# Uncomment if you have this file
# install_data('newtonmenu-dialog.glade',
#   install_dir: get_option('datadir') / meson.project_name()
# )