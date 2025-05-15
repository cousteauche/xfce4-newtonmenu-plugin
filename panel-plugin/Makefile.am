AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfce4-newtonbutton-plugin\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

#
# Newtonbutton plugin
#
plugin_LTLIBRARIES = \
	libnewtonbutton.la

plugindir = \
	$(libdir)/xfce4/panel/plugins

libnewtonbutton_la_SOURCES = \
	newtonbutton.c \
	newtonbutton.h \
	newtonbutton-dialogs.c \
	newtonbutton-dialogs.h

libnewtonbutton_la_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(LIBXFCE4PANEL_CFLAGS) \
	$(PLATFORM_CFLAGS)

libnewtonbutton_la_LDFLAGS = \
       -avoid-version \
       -module \
       -no-undefined \
       -export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
       $(PLATFORM_LDFLAGS)

libnewtonbutton_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GTK_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(LIBXFCE4PANEL_LIBS)

#
# Desktop file
#
desktopdir =								\
	$(datadir)/xfce4/panel/plugins

newtonbutton.desktop: newtonbutton.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

desktop_DATA =								\
	newtonbutton.desktop

EXTRA_DIST =								\
	meson.build							\
	newtonbutton.desktop.in

CLEANFILES =								\
	$(desktop_DATA)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
