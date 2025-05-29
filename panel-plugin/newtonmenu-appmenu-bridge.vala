// panel-plugin/newtonmenu-appmenu-bridge.vala

// This file serves as a bridge for C code to instantiate Appmenu.MenuWidget
// and provides C-callable functions to configure its properties.

using Gtk; // Required for Gtk.Widget type

// C-callable function to create a new instance of Appmenu.MenuWidget
[CCode (export="appmenu_menu_widget_new")]
public Gtk.Widget appmenu_menu_widget_new() {
    return new Appmenu.MenuWidget();
}

// C-callable function to set the 'compact-mode' property on the MenuWidget.
// This corresponds to Newton Menu's 'hide_application_name_prop'.
[CCode (export="appmenu_menu_widget_set_compact_mode")]
public void appmenu_menu_widget_set_compact_mode(Gtk.Widget widget, bool compact_mode) {
    if (widget is Appmenu.MenuWidget) {
        (widget as Appmenu.MenuWidget).compact_mode = compact_mode;
    } else {
        // Log an error if the widget type is incorrect (should not happen in proper use)
        GLib.stderr.printf("ERROR: appmenu_menu_widget_set_compact_mode called on non-MenuWidget object.\n");
    }
}

// C-callable function to set the 'bold-application-name' property on the MenuWidget.
// This corresponds to Newton Menu's 'bold_app_name_prop'.
[CCode (export="appmenu_menu_widget_set_bold_application_name")]
public void appmenu_menu_widget_set_bold_application_name(Gtk.Widget widget, bool bold_application_name) {
    if (widget is Appmenu.MenuWidget) {
        (widget as Appmenu.MenuWidget).bold_application_name = bold_application_name;
    } else {
        // Log an error if the widget type is incorrect (should not happen in proper use)
        GLib.stderr.printf("ERROR: appmenu_menu_widget_set_bold_application_name called on non-MenuWidget object.\n");
    }
}

// Define the Config namespace required by vala-panel-appmenu's internal dgettext calls.
// We explicitly set GETTEXT_PACKAGE to 'vala-panel-appmenu' so its strings are translated
// under its own domain, not Newton Menu's. This prevents conflicts.
namespace Config {
    // This value will be set by the 'configure_file' step in meson.build
    public const string GETTEXT_PACKAGE = "vala-panel-appmenu";
    // This value will be set by the 'configure_file' step in meson.build
    public const string LOCALE_DIR = "/usr/share/locale";
}
