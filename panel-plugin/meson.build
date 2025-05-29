# Import necessary modules
gnome = import('gnome')
i18n = import('i18n')

# Generate appmenu_config.vala from config.vala.in
appmenu_config_data = configuration_data()
appmenu_config_data.set('GETTEXT_PACKAGE', 'vala-panel-appmenu')
appmenu_config_data.set('LOCALE_DIR', join_paths(get_option('prefix'), get_option('localedir')))

appmenu_config_vala = configure_file(
  input: '../subprojects/vala-panel-appmenu/vapi/config.vala.in',
  output: 'appmenu_config.vala',
  configuration: appmenu_config_data
)

# Define plugin sources
plugin_sources = files(
  'newtonmenu.c',
  'newtonmenu-dialogs.c',
  'newtonmenu-force-quit-dialog.c',
  'newtonmenu-appmenu-bridge.vala',
  appmenu_config_vala
)

# Compile GResource
newtonmenu_gresource = gnome.compile_resources(
  'newtonmenu-gresource',
  'newtonmenu.gresource.xml',
  source_dir: '.',
  c_name: 'newtonmenu'
)

# Retrieve dependencies
libxfce4panel = dependency('libxfce4panel-2.0')
libxfce4ui = dependency('libxfce4ui-2')
libxfce4util = dependency('libxfce4util')
libwnck = dependency('libwnck-3.0')
glib = dependency('glib-2.0')
gtk = dependency('gtk+-3.0')

# Retrieve subproject dependency
valapanel_proj = subproject('vala-panel-appmenu')
valapanel_dep = valapanel_proj.get_variable('appmenu_dep')

# Additional optional dependencies
libdbusmenu_gtk_dep = dependency('libdbusmenu-gtk3', required: false)
exo_dep = dependency('exo-2', required: false)

# Assemble plugin dependencies
plugin_deps = [
  glib,
  gtk,
  libxfce4panel,
  libxfce4ui,
  libxfce4util,
  libwnck,
  valapanel_dep
]

if libdbusmenu_gtk_dep.found()
  plugin_deps += libdbusmenu_gtk_dep
endif

if exo_dep.found()
  plugin_deps += exo_dep
endif

# Determine plugin installation directory
plugin_install_dir = join_paths(get_option('prefix'), get_option('libdir'), 'xfce4', 'panel', 'plugins')
message('Plugin will be installed to: ' + plugin_install_dir)

# Build the shared module
shared_module('newtonmenu',
  sources: [plugin_sources, newtonmenu_gresource],
  dependencies: plugin_deps,
  install: true,
  install_dir: plugin_install_dir
)

# Generate and install the desktop file
desktop_file = i18n.merge_file(
  input: 'newtonmenu.desktop.in',
  output: 'newtonmenu.desktop',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'xfce4', 'panel', 'plugins')
)
