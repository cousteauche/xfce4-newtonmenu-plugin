# Plugin sources
plugin_sources = files(
  'newtonmenu.c',
  'newtonmenu-dialogs.c',
  'newtonmenu-force-quit-dialog.c',
  'dbusmenu-integration.c'
)

# Create GResource for UI files
newtonmenu_gresource = gnome.compile_resources(
  'newtonmenu-gresource',
  'newtonmenu.gresource.xml',
  source_dir: '.',
  c_name: 'newtonmenu'
)

# Collect all dependencies
plugin_deps = [
  glib,
  gtk,
  libxfce4panel,
  libxfce4ui,
  libxfce4util,
  libwnck_dep,
  libdbusmenu_glib_dep,
]

# Add libdbusmenu-gtk if available (for enhanced integration)
if libdbusmenu_gtk_dep.found()
  plugin_deps += libdbusmenu_gtk_dep
  message('Found libdbusmenu-gtk - enhanced global menu integration enabled')
else
  warning('libdbusmenu-gtk not found - using basic integration only')
endif

# Add optional dependencies
if exo_dep.found()
  plugin_deps += exo_dep
endif

# Try to get appmenu-glib-translator dependency
appmenu_glib_translator_dep = dependency('appmenu-glib-translator', required: false)
if appmenu_glib_translator_dep.found()
  plugin_deps += appmenu_glib_translator_dep
  message('Found appmenu-glib-translator dependency')
else
  warning('appmenu-glib-translator not found - global menu functionality will be limited')
endif

# Build the plugin
shared_module('newtonmenu',
  sources: [plugin_sources, newtonmenu_gresource, xfce_revision_h],
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