/*
 * Copyright (C) 2025 Adam
 * Xfce4 Newton Menu Plugin with Global Menu Integration
 */

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

/* Plugin structure */
typedef struct
{
    XfcePanelPlugin *plugin;

    /* Main UI components */
    GtkWidget       *event_box;
    GtkMenuButton   *button;          // Main menu button (changed to GtkMenuButton)
    GtkWidget       *button_box;      // Container for icon/label
    GtkWidget       *icon_image;      // Icon display
    GtkWidget       *label_widget;    // Label display

    /* Menu components */
    GtkWidget       *static_newton_menu;    // Static system menu (About, Settings, etc.)
    GMenuModel      *app_menu_model;        // Current application's menu  
    DbusmenuClient  *app_dbusmenu_client;   // DBusmenu client for app menus

    /* D-Bus and window tracking */
    GDBusConnection *dbus_session_bus;
    GDBusProxy      *appmenu_registrar_proxy;
    WnckScreen      *wnck_screen;
    WnckWindow      *active_wnck_window;

    /* Signal handler IDs */
    gulong          wnck_active_window_changed_handler_id;
    gulong          appmenu_registrar_registered_handler_id;
    gulong          appmenu_registrar_unregistered_handler_id;

    /* Configuration properties */
    gboolean        display_icon_prop;
    gchar          *icon_name_prop;
    gchar          *label_text_prop;
    
    /* Global menu properties */
    gboolean        hide_application_name_prop;
    gchar          *global_menu_title_prop;
    
    /* Confirmation properties */
    gboolean        confirm_logout_prop;
    gboolean        confirm_restart_prop;
    gboolean        confirm_shutdown_prop;
    gboolean        confirm_force_quit_prop;
}
newtonmenuPlugin;

/* Function declarations */

/* Core plugin functions */
void newtonmenu_save(XfcePanelPlugin *plugin, newtonmenuPlugin *newtonmenu);
void newtonmenu_update_display(newtonmenuPlugin *newtonmenu);

/* Global menu functions */
void newtonmenu_update_combined_menu(newtonmenuPlugin *newtonmenu);

G_END_DECLS

#endif /* !__NEWTONMENU_H__ */