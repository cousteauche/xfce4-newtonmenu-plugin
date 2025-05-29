project('xfce4-newtonmenu-plugin', 'c', 'vala',
  version: '1.0.0',
  default_options: ['warning_level=1'])

# Dependencies
xfce_panel_dep = dependency('libxfce4panel-2.0', version: '>= 4.16')
xfce_ui_dep    = dependency('libxfce4ui-2', version: '>= 4.16')
xfce_util_dep  = dependency('libxfce4util-1.0', version: '>= 4.16')
exo_dep        = dependency('exo-2', version: '>= 0.12')
gtk_dep        = dependency('gtk+-3.0', version: '>= 3.24')
glib_dep       = dependency('glib-2.0', version: '>= 2.66')
wnck_dep       = dependency('libwnck-3.0', version: '>= 3.4.0')
dbusmenu_dep   = dependency('dbusmenu-glib-0.4', required: true)

# Plugin sources (C and Vala)
sources_c = [
  'src/newtonmenu.c',
]
sources_vala = [
  'src/newtonmenu-dialog.vala',  # (example: maybe a settings dialog implemented in Vala)
]

# Compile Vala sources to C (if any)
vala_features = [ '--target-glib=2.0' ]
vala_c = vala.compile('newtonmenu_vala',
  sources_vala,
  dependencies: [gtk_dep, glib_dep, xfce_ui_dep, xfce_panel_dep],
  args: vala_features,
  install: false  # We compile into the .so, not a separate prog
)

# Combine generated C from Vala with hand-written C
plugin_sources = sources_c + vala_c

# Build the plugin as a shared object for Xfce panel
plugin = shared_library(
  'newtonmenu',
  plugin_sources,
  include_directories: include_directories('src'),
  dependencies: [xfce_panel_dep, xfce_ui_dep, xfce_util_dep, exo_dep, gtk_dep, glib_dep, wnck_dep, dbusmenu_dep],
  install: true,
  install_dir: xfce_panel_dep.get_variable('plugindir')  # installs to XFCE's panel plugin directory
)

# Install the panel .desktop file for the plugin (to show in Add New Items dialog)
install_data('panel-plugin/xfce4-newtonmenu-plugin.desktop',
  install_dir: xfce_panel_dep.get_variable('datadir') + '/applications')
