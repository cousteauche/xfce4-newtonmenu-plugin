/*
 * Copyright (C) 2025 Adam
 * DBusmenu Integration Helper Functions
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <libdbusmenu-glib/client.h>
#include <libdbusmenu-glib/dbusmenu-glib.h>
#include <libwnck/libwnck.h>
#include <gio/gio.h>
#include <pango/pango.h>

#include "newtonmenu.h" // Provides newtonmenuPlugin, and prototypes for on_menu_button_enter, newtonmenu_menu_button_toggled
#include "dbusmenu-integration.h"

// REMOVE all forward declarations for functions that are now in newtonmenu.h
// void newtonmenu_menu_button_toggled(GtkMenuButton *button, newtonmenuPlugin *newtonmenu); // REMOVE
// static gboolean on_menu_button_enter(GtkWidget *widget, GdkEventCrossing *event, newtonmenuPlugin *newtonmenu); // REMOVE

static void on_dbusmenu_root_changed(DbusmenuClient *client, DbusmenuMenuitem *newroot_unsafe, gpointer user_data);
static void append_dbusmenu_items_to_gtk_menu(GtkWidget *gtk_menu_shell, DbusmenuMenuitem *dbusmenu_item_source);
static GtkWidget* create_gtk_menuitem_from_dbusmenu_item(DbusmenuMenuitem *dbusmenu_item);
static void on_dbusmenu_gtk_item_activated(GtkMenuItem *gtk_item, gpointer user_data);


void newtonmenu_fetch_app_menu_enhanced(newtonmenuPlugin *newtonmenu, WnckWindow *window) {
    GError *error = NULL;
    gchar *service_name = NULL;
    gchar *object_path = NULL;
    g_return_if_fail(newtonmenu != NULL);
    
    if (newtonmenu->app_dbusmenu_client) { 
        if (newtonmenu->app_dbusmenu_client_root_changed_id > 0 && g_signal_handler_is_connected(newtonmenu->app_dbusmenu_client, newtonmenu->app_dbusmenu_client_root_changed_id)) {
            g_signal_handler_disconnect(newtonmenu->app_dbusmenu_client, newtonmenu->app_dbusmenu_client_root_changed_id);
        }
        newtonmenu->app_dbusmenu_client_root_changed_id = 0;
        if (newtonmenu->app_dbusmenu_client_layout_updated_id > 0 && g_signal_handler_is_connected(newtonmenu->app_dbusmenu_client, newtonmenu->app_dbusmenu_client_layout_updated_id)) {
            g_signal_handler_disconnect(newtonmenu->app_dbusmenu_client, newtonmenu->app_dbusmenu_client_layout_updated_id);
        }
        newtonmenu->app_dbusmenu_client_layout_updated_id = 0;
        g_object_unref(newtonmenu->app_dbusmenu_client);
        newtonmenu->app_dbusmenu_client = NULL;
    }
    newtonmenu_clear_dynamic_app_menus(newtonmenu); 

    if (!window) return;
    if (!newtonmenu->appmenu_registrar_proxy) {
        g_warning("AppMenu Registrar Proxy not initialized.");
        return;
    }
    GVariant *result = g_dbus_proxy_call_sync(
        newtonmenu->appmenu_registrar_proxy, "GetMenuForWindow",
        g_variant_new("(u)", (guint32)wnck_window_get_xid(window)),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (error) {
        g_debug("NM_DEBUG: Failed to get menu for window XID %lu: %s.", (gulong)wnck_window_get_xid(window), error->message);
        g_clear_error(&error); return;
    }
    g_debug("NM_DEBUG: GetMenuForWindow call successful.");
    g_variant_get(result, "(so)", &service_name, &object_path);
    g_variant_unref(result);
    g_debug("NM_DEBUG: Service: '%s', Path: '%s'", service_name ? service_name : "(null)", object_path ? object_path : "(null)");
    if (service_name && object_path && strlen(service_name) > 0 && g_strcmp0(object_path, "/") != 0) {
        g_debug("NM_DEBUG: Valid service/path found. Creating DbusmenuClient.");
        newtonmenu->app_dbusmenu_client = dbusmenu_client_new(service_name, object_path);
        g_debug("NM_DEBUG: app_dbusmenu_client: %p", newtonmenu->app_dbusmenu_client);
        if (newtonmenu->app_dbusmenu_client) {
            newtonmenu->app_dbusmenu_client_root_changed_id = g_signal_connect(newtonmenu->app_dbusmenu_client, "root-changed", G_CALLBACK(on_dbusmenu_root_changed), newtonmenu);
            newtonmenu->app_dbusmenu_client_layout_updated_id = g_signal_connect(newtonmenu->app_dbusmenu_client, "layout-updated", G_CALLBACK(on_dbusmenu_root_changed), newtonmenu);
            DbusmenuMenuitem *root = dbusmenu_client_get_root(newtonmenu->app_dbusmenu_client);
            g_debug("NM_DEBUG: Initial client_get_root: %p", root);
            if (root) on_dbusmenu_root_changed(newtonmenu->app_dbusmenu_client, root, newtonmenu);
            else g_debug("NM_DEBUG: DBusmenu client created for %s %s, but root is initially NULL.", service_name, object_path);
        } else g_warning("Failed to create DbusmenuClient for %s %s", service_name, object_path);
    } else g_debug("NM_DEBUG: No valid service/path after GetMenuForWindow.");
    g_free(service_name); g_free(object_path);
}

static void on_dbusmenu_root_changed(DbusmenuClient *client, DbusmenuMenuitem *newroot_unsafe_param, gpointer user_data) {
    newtonmenuPlugin *newtonmenu = (newtonmenuPlugin*)user_data;
    DbusmenuMenuitem *root_item_ref = NULL;
    GList *top_level_dbus_children, *iter;
    g_debug("NM_DEBUG: on_dbusmenu_root_changed CALLED. Client: %p, newroot_unsafe_param: %p", client, newroot_unsafe_param);
    g_return_if_fail(newtonmenu != NULL);
    newtonmenu_clear_dynamic_app_menus(newtonmenu);
    if (!newtonmenu->app_dbusmenu_client) {
        g_debug("NM_DEBUG: DBusmenu client not available in on_dbusmenu_root_changed.");
        gtk_widget_hide(newtonmenu->app_menu_bar_container); return;
    }
    root_item_ref = dbusmenu_client_get_root(newtonmenu->app_dbusmenu_client);
    g_debug("NM_DEBUG: root_item_ref from client_get_root in on_dbusmenu_root_changed: %p", root_item_ref);
    if (!root_item_ref) {
        g_debug("NM_DEBUG: root_item_ref is NULL. Hiding container.");
        gtk_widget_hide(newtonmenu->app_menu_bar_container); return;
    }
    g_object_ref(root_item_ref); 
    top_level_dbus_children = dbusmenu_menuitem_get_children(root_item_ref);
    g_debug("NM_DEBUG: top_level_dbus_children: %p, Length: %d", top_level_dbus_children, top_level_dbus_children ? g_list_length(top_level_dbus_children) : 0);
    if (!top_level_dbus_children) {
        g_debug("NM_DEBUG: No top_level children. Hiding container.");
        gtk_widget_hide(newtonmenu->app_menu_bar_container);
        g_object_unref(root_item_ref); return;
    }
    iter = top_level_dbus_children;
    if (newtonmenu->show_app_name_button_prop && iter != NULL) {
        DbusmenuMenuitem *app_main_dbus_item = DBUSMENU_MENUITEM(iter->data);
        const gchar *app_main_label_str = dbusmenu_menuitem_property_get(app_main_dbus_item, DBUSMENU_MENUITEM_PROP_LABEL);
        g_debug("NM_DEBUG: AppNameButton: Label from D-Bus: '%s'", app_main_label_str ? app_main_label_str : "(null)");
        gchar *display_label;
        newtonmenu->app_name_button = GTK_MENU_BUTTON(gtk_menu_button_new());
        if (newtonmenu->hide_application_name_prop && newtonmenu->global_menu_title_prop && strlen(newtonmenu->global_menu_title_prop) > 0) {
            display_label = g_strdup(newtonmenu->global_menu_title_prop);
        } else if (app_main_label_str && strlen(app_main_label_str) > 0) {
            display_label = g_strdup(app_main_label_str);
        } else if (newtonmenu->active_wnck_window) {
            display_label = g_strdup(wnck_window_get_name(newtonmenu->active_wnck_window));
        } else { display_label = g_strdup(_("Application")); }
        newtonmenu->app_name_button_label = gtk_label_new(display_label);
        g_free(display_label);
        GtkWidget *app_name_content_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
        gtk_box_pack_start(GTK_BOX(app_name_content_box), newtonmenu->app_name_button_label, TRUE, TRUE, 0);
        gtk_container_add(GTK_CONTAINER(newtonmenu->app_name_button), app_name_content_box);
        if (newtonmenu->bold_app_name_prop) {
            PangoAttrList *attrs = pango_attr_list_new();
            pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
            gtk_label_set_attributes(GTK_LABEL(newtonmenu->app_name_button_label), attrs);
            pango_attr_list_unref(attrs);
        }
        GtkWidget *app_main_submenu = gtk_menu_new();
        append_dbusmenu_items_to_gtk_menu(app_main_submenu, app_main_dbus_item);
        gtk_menu_button_set_popup(newtonmenu->app_name_button, app_main_submenu);
        gtk_button_set_relief(GTK_BUTTON(newtonmenu->app_name_button), GTK_RELIEF_NONE);
        g_signal_connect(newtonmenu->app_name_button, "toggled", G_CALLBACK(newtonmenu_menu_button_toggled), newtonmenu);
        g_signal_connect(newtonmenu->app_name_button, "enter-notify-event", G_CALLBACK(on_menu_button_enter), newtonmenu);
        gtk_box_pack_start(GTK_BOX(newtonmenu->app_menu_bar_container), GTK_WIDGET(newtonmenu->app_name_button), FALSE, FALSE, 0);
        iter = g_list_next(iter);
    }
    for (; iter != NULL; iter = g_list_next(iter)) {
        DbusmenuMenuitem *menu_bar_item_dbus = DBUSMENU_MENUITEM(iter->data);
        const gchar *label_str = dbusmenu_menuitem_property_get(menu_bar_item_dbus, DBUSMENU_MENUITEM_PROP_LABEL);
        g_debug("NM_DEBUG: DynamicButton: Label from D-Bus: '%s'", label_str ? label_str : "(null)");
        if (!label_str || strlen(label_str) == 0) continue;
        GtkMenuButton *menu_bar_button = GTK_MENU_BUTTON(gtk_menu_button_new());
        GtkWidget *button_label_widget = gtk_label_new(label_str);
        gtk_container_add(GTK_CONTAINER(menu_bar_button), button_label_widget);
        gtk_button_set_relief(GTK_BUTTON(menu_bar_button), GTK_RELIEF_NONE);
        GtkWidget *submenu = gtk_menu_new();
        append_dbusmenu_items_to_gtk_menu(submenu, menu_bar_item_dbus);
        gtk_menu_button_set_popup(menu_bar_button, submenu);
        g_signal_connect(menu_bar_button, "toggled", G_CALLBACK(newtonmenu_menu_button_toggled), newtonmenu);
        g_signal_connect(menu_bar_button, "enter-notify-event", G_CALLBACK(on_menu_button_enter), newtonmenu);
        gtk_box_pack_start(GTK_BOX(newtonmenu->app_menu_bar_container), GTK_WIDGET(menu_bar_button), FALSE, FALSE, 0);
        newtonmenu->dynamic_app_menu_buttons = g_list_append(newtonmenu->dynamic_app_menu_buttons, menu_bar_button);
    }
    if (top_level_dbus_children) g_list_free(top_level_dbus_children);
    if (root_item_ref) g_object_unref(root_item_ref);
    if (gtk_container_get_children(GTK_CONTAINER(newtonmenu->app_menu_bar_container)) != NULL) {
        g_debug("NM_DEBUG: Showing app_menu_bar_container because it has children.");
        gtk_widget_show_all(newtonmenu->app_menu_bar_container);
    } else {
        g_debug("NM_DEBUG: Hiding app_menu_bar_container because it has NO children.");
        gtk_widget_hide(newtonmenu->app_menu_bar_container);
    }
}

static void append_dbusmenu_items_to_gtk_menu(GtkWidget *gtk_menu_shell, DbusmenuMenuitem *dbusmenu_item_source) {
    GList *children, *iter;
    g_return_if_fail(GTK_IS_MENU_SHELL(gtk_menu_shell) && DBUSMENU_IS_MENUITEM(dbusmenu_item_source));
    children = dbusmenu_menuitem_get_children(dbusmenu_item_source);
    for (iter = children; iter != NULL; iter = g_list_next(iter)) {
        DbusmenuMenuitem *child_dbus_item = DBUSMENU_MENUITEM(iter->data);
        GtkWidget *gtk_menu_item_widget = create_gtk_menuitem_from_dbusmenu_item(child_dbus_item);
        if (gtk_menu_item_widget) gtk_menu_shell_append(GTK_MENU_SHELL(gtk_menu_shell), gtk_menu_item_widget);
    }
    if (children) g_list_free_full(children, g_object_unref);
}

static GtkWidget* create_gtk_menuitem_from_dbusmenu_item(DbusmenuMenuitem *dbusmenu_item) {
    GtkWidget *gtk_item = NULL;
    const gchar *label, *item_type_str, *toggle_type_str;
    GList *children_of_dbus_item;
    g_return_val_if_fail(DBUSMENU_IS_MENUITEM(dbusmenu_item), NULL);
    item_type_str = dbusmenu_menuitem_property_get(dbusmenu_item, DBUSMENU_MENUITEM_PROP_TYPE);
    if (item_type_str && g_strcmp0(item_type_str, DBUSMENU_CLIENT_TYPES_SEPARATOR) == 0) { 
        return gtk_separator_menu_item_new();
    }
    label = dbusmenu_menuitem_property_get(dbusmenu_item, DBUSMENU_MENUITEM_PROP_LABEL);
    if (!label || strlen(label) == 0) label = _("(Missing Label)");
    children_of_dbus_item = dbusmenu_menuitem_get_children(dbusmenu_item);
    if (children_of_dbus_item != NULL) {
        gtk_item = gtk_menu_item_new_with_label(label);
        GtkWidget *submenu_shell = gtk_menu_new();
        append_dbusmenu_items_to_gtk_menu(submenu_shell, dbusmenu_item);
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(gtk_item), submenu_shell);
        g_list_free_full(children_of_dbus_item, g_object_unref);
    } else {
        toggle_type_str = dbusmenu_menuitem_property_get(dbusmenu_item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE);
        if (toggle_type_str && (g_strcmp0(toggle_type_str, "checkmark") == 0 || g_strcmp0(toggle_type_str, "radio") == 0)) {
            gtk_item = gtk_check_menu_item_new_with_label(label);
            gint toggle_state = dbusmenu_menuitem_property_get_int(dbusmenu_item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item), toggle_state == DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED);
        } else gtk_item = gtk_menu_item_new_with_label(label);
    }
    if (gtk_item) {
        gtk_widget_set_sensitive(gtk_item, dbusmenu_menuitem_property_get_bool(dbusmenu_item, DBUSMENU_MENUITEM_PROP_ENABLED));
        if (dbusmenu_menuitem_property_get_bool(dbusmenu_item, DBUSMENU_MENUITEM_PROP_VISIBLE)) gtk_widget_show(gtk_item);
        else gtk_widget_hide(gtk_item);
        if (children_of_dbus_item == NULL) {
             g_object_set_data_full(G_OBJECT(gtk_item), "dbusmenu-item", g_object_ref(dbusmenu_item), g_object_unref);
             g_signal_connect(gtk_item, "activate", G_CALLBACK(on_dbusmenu_gtk_item_activated), dbusmenu_item);
        }
    }
    return gtk_item;
}

static void on_dbusmenu_gtk_item_activated(GtkMenuItem *gtk_item, gpointer user_data) {
    DbusmenuMenuitem *dbusmenu_item = DBUSMENU_MENUITEM(user_data);
    // GVariant *data_variant; // Removed as it's unused with dbusmenu_menuitem_handle_event("clicked")
    guint32 timestamp = 0;
    GdkEvent *current_event = gtk_get_current_event();
    g_return_if_fail(DBUSMENU_IS_MENUITEM(dbusmenu_item));
    if (current_event) { timestamp = gdk_event_get_time(current_event); gdk_event_free(current_event); }
    
    // Using dbusmenu_menuitem_handle_event as suggested by compiler for "clicked"
    // The third argument for "clicked" (GVariant* data) can often be NULL.
    dbusmenu_menuitem_handle_event(dbusmenu_item, "clicked", NULL, timestamp);
}

gboolean newtonmenu_window_has_app_menu(newtonmenuPlugin *newtonmenu, WnckWindow *window) {
    GError *error = NULL;
    gchar *service_name = NULL, *object_path = NULL;
    gboolean has_menu = FALSE;
    g_return_val_if_fail(newtonmenu != NULL && window != NULL, FALSE);
    if (!newtonmenu->appmenu_registrar_proxy) return FALSE;
    GVariant *result = g_dbus_proxy_call_sync(
        newtonmenu->appmenu_registrar_proxy, "GetMenuForWindow",
        g_variant_new("(u)", (guint32)wnck_window_get_xid(window)),
        G_DBUS_CALL_FLAGS_NONE, 1000, NULL, &error);
    if (!error && result) {
        g_variant_get(result, "(so)", &service_name, &object_path);
        has_menu = (service_name && strlen(service_name) > 0 && g_strcmp0(object_path, "/") != 0);
        g_variant_unref(result);
    }
    g_free(service_name); g_free(object_path); g_clear_error(&error);
    return has_menu;
}