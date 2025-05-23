project(
  'xfce4-newtonmenu-plugin',
  'c',
  version: '0.0.4-dev',
  license: 'GPL-2.0-or-later',
  meson_version: '>= 0.54.0',
  default_options: [
    'c_std=gnu11',
    'buildtype=debugoptimized',
    'warning_level=2',
  ]
)

project_namespace = 'panel-plugins'
pkgdatadir = get_option('prefix') / get_option('datadir') / meson.project_name()
copyright_year = '2025'

cc = meson.get_compiler('c')
pkgconfig = import('pkgconfig')
gnome = import('gnome')
i18n = import('i18n')

dependency_versions = {
  'glib': '>= 2.66.0',
  'gtk': '>= 3.24.0',
  'xfce4': '>= 4.16.0',
  'libwnck': '>= 3.4.8',
  'dbusmenu-glib': '>= 0.6.0',
}

# Core dependencies
glib = dependency('glib-2.0', version: dependency_versions['glib'])
gtk = dependency('gtk+-3.0', version: dependency_versions['gtk'])
libxfce4panel = dependency('libxfce4panel-2.0', version: dependency_versions['xfce4'])
libxfce4ui = dependency('libxfce4ui-2', version: dependency_versions['xfce4'])
libxfce4util = dependency('libxfce4util-1.0', version: dependency_versions['xfce4'])
libwnck_dep = dependency('libwnck-3.0', version: dependency_versions['libwnck'])

# DBusMenu dependency - this provides the global menu headers
libdbusmenu_glib_dep = dependency('dbusmenu-glib-0.4', version: dependency_versions['dbusmenu-glib'])
libdbusmenu_gtk_dep = dependency('dbusmenu-gtk3-0.4', required: false)

# Optional dependencies
exo_dep = dependency('exo-2', version: '>= 0.12.0', required: false)

feature_cflags = []
if cc.check_header('string.h')
  feature_cflags += '-DHAVE_STRING_H=1'
endif

extra_cflags = []
extra_cflags_check = [
  '-Wmissing-declarations',
  '-Wmissing-noreturn',
  '-Wold-style-definition',
  '-Wredundant-decls',
  '-Wpointer-arith',
  '-Wcast-align',
  '-Winit-self',
  '-Wshadow',
  '-Wmissing-include-dirs',
  '-Wundef',
  '-Wformat',
  '-Wformat-security',
  '-Wnested-externs',
  '-Wno-unused-parameter',
  '-Wno-declaration-after-statement',
  '-Wno-missing-field-initializers',
  '-Werror=implicit-function-declaration',
  '-Wno-error=deprecated-declarations',
]

# Only add '-Wformat-y2k' if supported
if cc.has_argument('-Wformat-y2k')
  extra_cflags_check += '-Wformat-y2k'
endif

optimization = get_option('optimization')
if get_option('debug') and optimization in ['0', 'g']
  extra_cflags_check += '-fstack-protector-strong'
  extra_cflags += [
    '-DDEBUG=1',
    '-DDEBUG_TRACE=1',
    '-DG_ENABLE_DEBUG',
  ]
elif optimization in ['3', 'minsize']
  extra_cflags += [
    '-DNDEBUG',
    '-DG_DISABLE_CAST_CHECKS',
    '-DG_DISABLE_ASSERT',
  ]
endif

if dependency_versions.has_key('glib')
  glib_version_parts = dependency_versions['glib'].split(' ')
  glib_min_version_parts = glib_version_parts[1].split('.')
  glib_min_version_define = 'GLIB_VERSION_@0@_@1@'.format(glib_min_version_parts[0], glib_min_version_parts[1])
  extra_cflags += [
    '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_min_version_define),
    '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_min_version_define),
    '-DG_LOG_USE_STRUCTURED=1',
  ]
endif

version_parts = meson.project_version().split('-dev')[0].split('.')
version_short = '@0@.@1@'.format(version_parts[0], version_parts[1])

extra_cflags += [
  '-DPACKAGE="@0@"'.format(meson.project_name()),
  '-DPACKAGE_NAME="@0@"'.format(meson.project_name()),
  '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
  '-DVERSION="@0@"'.format(meson.project_version()),
  '-DVERSION_SHORT="@0@"'.format(version_short),
  '-DPACKAGE_STRING="@0@ @1@"'.format(meson.project_name(), meson.project_version()),
  '-DPACKAGE_DATADIR="@0@"'.format(pkgdatadir),
  '-DCOPYRIGHT_YEAR="@0@"'.format(copyright_year),
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DPACKAGE_URL="https://docs.xfce.org/@0@/@1@/start"'.format(project_namespace, meson.project_name()),
  '-DPACKAGE_BUGREPORT="https://gitlab.xfce.org/@0@/@1@/-/issues"'.format(project_namespace, meson.project_name()),
  '-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name()),
  '-DPREFIX="@0@"'.format(get_option('prefix')),
  '-DBINDIR="@0@"'.format(get_option('prefix') / get_option('bindir')),
  '-DDATADIR="@0@"'.format(get_option('prefix') / get_option('datadir')),
  '-DDOCDIR="@0@"'.format(get_option('prefix') / 'doc' / meson.project_name()),
  '-DINCLUDEDIR="@0@"'.format(get_option('prefix') / get_option('includedir')),
  '-DLIBDIR="@0@"'.format(get_option('prefix') / get_option('libdir')),
  '-DLIBEXECDIR="@0@"'.format(get_option('prefix') / get_option('libexecdir')),
  '-DLOCALEDIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DLOCALSTATEDIR="@0@"'.format(get_option('prefix') / get_option('localstatedir')),
  '-DSBINDIR="@0@"'.format(get_option('prefix') / get_option('sbindir')),
  '-DSYSCONFDIR="@0@"'.format(get_option('prefix') / get_option('sysconfdir')),
  '-DHAVE_XFCE_REVISION_H=1',
]

add_project_arguments(cc.get_supported_arguments(extra_cflags_check), language: 'c')
add_project_arguments(feature_cflags, language: 'c')
add_project_arguments(extra_cflags, language: 'c')

xfce_revision_h = vcs_tag(
  command: ['git', 'rev-parse', '--short', 'HEAD'],
  fallback: 'UNKNOWN',
  input: 'xfce-revision.h.in',
  output: 'xfce-revision.h',
  replace_string: '@REVISION@',
)

# Try to find vala-panel-appmenu as a subproject if it exists
vala_panel_appmenu_found = false
appmenu_glib_translator_dep = dependency('', required: false)

# Check if subprojects/vala-panel-appmenu/meson.build exists
subproject_file = 'subprojects/vala-panel-appmenu/meson.build'
if run_command('test', '-f', subproject_file, check: false).returncode() == 0
  message('Found vala-panel-appmenu subproject directory')
  
  # Try to add as subproject
  valapanel_appmenu_proj = subproject('vala-panel-appmenu',
      required: false,
      default_options: [
          'xfce=enabled',
          'appmenu-gtk-module=enabled',
          'registrar=enabled',
          'jayatana=disabled',
          'budgie=disabled',
          'mate=disabled',
          'valapanel=disabled',
      ]
  )
  
  if valapanel_appmenu_proj.found()
    message('Successfully loaded vala-panel-appmenu subproject')
    vala_panel_appmenu_found = true
    
    # Try to get the appmenu-glib-translator dependency
    appmenu_glib_translator_dep = dependency('appmenu-glib-translator', 
      fallback: ['vala-panel-appmenu', 'appmenu_glib_translator_dep'],
      required: false
    )
    
    if not appmenu_glib_translator_dep.found()
      # Alternative fallback names
      appmenu_glib_translator_dep = dependency('appmenu-glib-translator',
        required: false
      )
    endif
  else
    warning('vala-panel-appmenu subproject found but failed to load')
  endif
else
  message('No vala-panel-appmenu subproject found, trying system dependency')
endif

# Fallback to system dependency if subproject not found
if not vala_panel_appmenu_found
  appmenu_glib_translator_dep = dependency('appmenu-glib-translator', required: false)
endif

if appmenu_glib_translator_dep.found()
  message('Found appmenu-glib-translator dependency - full global menu support enabled')
  extra_cflags += '-DHAVE_APPMENU_GLIB_TRANSLATOR=1'
else
  warning('appmenu-glib-translator not found - global menu functionality will be limited')
  warning('To get full global menu support:')
  warning('  1. Install vala-panel-appmenu system package, or')
  warning('  2. Add vala-panel-appmenu as a git submodule in subprojects/')
endif

subdir('icons')
subdir('panel-plugin')
subdir('po')