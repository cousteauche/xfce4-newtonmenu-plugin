#ifndef __NEWTONMENU_H__
#define __NEWTONMENU_H__

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4util/libxfce4util.h>
#include <libwnck/libwnck.h>
#include <gio/gio.h>
#include <libdbusmenu-glib/client.h>

G_BEGIN_DECLS

typedef struct _newtonmenuPlugin newtonmenuPlugin;

struct _newtonmenuPlugin
{
    XfcePanelPlugin *plugin;

    GtkWidget       *main_box;
    GtkMenuButton   *newton_button;
    GtkWidget       *newton_button_box;
    GtkWidget       *newton_icon_image;
    GtkWidget       *newton_label_widget;
    GtkWidget       *static_newton_menu;

    GtkWidget       *app_menu_bar_container;
    GtkMenuButton   *app_name_button;
    GtkWidget       *app_name_button_label;

    GList           *dynamic_app_menu_buttons;

    DbusmenuClient  *app_dbusmenu_client;
    gulong          app_dbusmenu_client_root_changed_id;    // For DbusmenuClient "root-changed"
    gulong          app_dbusmenu_client_layout_updated_id;  // For DbusmenuClient "layout-updated"

    GDBusConnection *dbus_session_bus;
    GDBusProxy      *appmenu_registrar_proxy;
    guint           dbus_name_owner_id;                     // For g_bus_own_name

    WnckScreen      *wnck_screen;
    WnckWindow      *active_wnck_window;

    gulong          wnck_active_window_changed_handler_id;
    gulong          appmenu_registrar_registered_handler_id;
    gulong          appmenu_registrar_unregistered_handler_id;

    gboolean        display_icon_prop;
    gchar          *icon_name_prop;
    gchar          *label_text_prop;
    
    gboolean        hide_application_name_prop;
    gchar          *global_menu_title_prop;

    gboolean        show_app_name_button_prop;
    gboolean        bold_app_name_prop;
    
    gboolean        confirm_logout_prop;
    gboolean        confirm_restart_prop;
    gboolean        confirm_shutdown_prop;
    gboolean        confirm_force_quit_prop;
    
    GtkMenuButton   *currently_open_button;
};

void newtonmenu_save(XfcePanelPlugin *plugin, newtonmenuPlugin *newtonmenu);
void newtonmenu_update_display(newtonmenuPlugin *newtonmenu);
void newtonmenu_clear_dynamic_app_menus(newtonmenuPlugin *newtonmenu);

void newtonmenu_menu_button_toggled(GtkMenuButton *button, newtonmenuPlugin *newtonmenu);
gboolean on_menu_button_enter(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data);

void newtonmenu_configure (XfcePanelPlugin *plugin, newtonmenuPlugin *newtonmenu);
void newtonmenu_about (XfcePanelPlugin *plugin);

G_END_DECLS

#endif /* !__NEWTONMENU_H__ */