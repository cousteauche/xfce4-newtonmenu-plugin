/*
 * Copyright (C) 2025 Adam
 * DBusmenu Integration Helper Functions
 * 
 * This file demonstrates how to properly integrate DBusmenu
 * with traditional GTK menus for the Newton Menu plugin.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <libdbusmenu-glib/client.h>
#include <libdbusmenu-gtk/menu.h>
#include <libwnck/libwnck.h>
#include <gio/gio.h>

#include "newtonmenu.h"
#include "dbusmenu-integration.h"

// Forward declarations
static void on_dbusmenu_root_changed(DbusmenuClient *client, DbusmenuMenuitem *newroot, gpointer user_data);
static void append_dbusmenu_items_to_gtk_menu(GtkWidget *gtk_menu, DbusmenuMenuitem *dbusmenu_item);
static GtkWidget* create_gtk_menuitem_from_dbusmenu_item(DbusmenuMenuitem *dbusmenu_item);
static void on_dbusmenu_gtk_item_activated(GtkMenuItem *gtk_item, gpointer user_data);

/**
 * Enhanced fetch_app_menu function that properly integrates DBusmenu
 * This replaces the placeholder implementation in newtonmenu.c
 */
void newtonmenu_fetch_app_menu_enhanced(newtonmenuPlugin *newtonmenu, WnckWindow *window)
{
    GError *error = NULL;
    gchar *service_name = NULL;
    gchar *object_path = NULL;

    g_return_if_fail(newtonmenu != NULL);
    g_return_if_fail(window != NULL);
    g_return_if_fail(newtonmenu->appmenu_registrar_proxy != NULL);

    // Call GetMenuForWindow on the AppMenu Registrar
    GVariant *result = g_dbus_proxy_call_sync(
        newtonmenu->appmenu_registrar_proxy,
        "GetMenuForWindow",
        g_variant_new("(u)", (guint32)wnck_window_get_xid(window)),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);

    if (error) {
        g_warning("Failed to get menu for window %lu: %s", 
                 (gulong)wnck_window_get_xid(window), error->message);
        g_clear_error(&error);
        
        // Clear old app menu and update
        if (newtonmenu->app_dbusmenu_client) {
            g_object_unref(newtonmenu->app_dbusmenu_client);
            newtonmenu->app_dbusmenu_client = NULL;
        }
        newtonmenu_update_combined_menu(newtonmenu);
        return;
    }

    g_variant_get(result, "(so)", &service_name, &object_path);
    g_variant_unref(result);

    // Check if we have a valid menu
    if (service_name && object_path && strlen(service_name) > 0 && g_strcmp0(object_path, "/") != 0) {
        g_debug("Found menu for window %lu: service=%s, path=%s", 
                (gulong)wnck_window_get_xid(window), service_name, object_path);
        
        // Clean up old client
        if (newtonmenu->app_dbusmenu_client) {
            g_object_unref(newtonmenu->app_dbusmenu_client);
            newtonmenu->app_dbusmenu_client = NULL;
        }
        
        // Create new DBusmenu client
        newtonmenu->app_dbusmenu_client = dbusmenu_client_new(service_name, object_path);
        
        if (newtonmenu->app_dbusmenu_client) {
            // Connect to root changes
            g_signal_connect(newtonmenu->app_dbusmenu_client, "root-changed",
                           G_CALLBACK(on_dbusmenu_root_changed), newtonmenu);
            
            // Force initial layout update
            DbusmenuMenuitem *root = dbusmenu_client_get_root(newtonmenu->app_dbusmenu_client);
            if (root) {
                on_dbusmenu_root_changed(newtonmenu->app_dbusmenu_client, root, newtonmenu);
            }
        }
    } else {
        g_debug("No menu available for window %lu", (gulong)wnck_window_get_xid(window));
        
        // Clear old client
        if (newtonmenu->app_dbusmenu_client) {
            g_object_unref(newtonmenu->app_dbusmenu_client);
            newtonmenu->app_dbusmenu_client = NULL;
        }
        newtonmenu_update_combined_menu(newtonmenu);
    }

    g_free(service_name);
    g_free(object_path);
}

/**
 * Callback for when the DBusmenu root changes
 */
static void on_dbusmenu_root_changed(DbusmenuClient *client, DbusmenuMenuitem *newroot, gpointer user_data)
{
    newtonmenuPlugin *newtonmenu = (newtonmenuPlugin*)user_data;
    
    g_return_if_fail(newtonmenu != NULL);
    
    g_debug("DBusmenu root changed, updating application menu");
    
    // Update the combined menu with the new app menu
    newtonmenu_update_combined_menu(newtonmenu);
}

/**
 * Enhanced update_combined_menu function with proper DBusmenu integration
 */
void newtonmenu_update_combined_menu_enhanced(newtonmenuPlugin *newtonmenu)
{
    GtkWidget *new_menu;
    gchar *app_name = NULL;
    
    g_return_if_fail(newtonmenu != NULL);

    // Create a new menu combining system menu and app menu
    new_menu = gtk_menu_new();
    
    // First, add all items from the static Newton menu
    if (newtonmenu->static_newton_menu) {
        GList *children = gtk_container_get_children(GTK_CONTAINER(newtonmenu->static_newton_menu));
        GList *iter;
        
        for (iter = children; iter != NULL; iter = iter->next) {
            GtkWidget *original_item = GTK_WIDGET(iter->data);
            GtkWidget *new_item;
            
            if (GTK_IS_SEPARATOR_MENU_ITEM(original_item)) {
                new_item = gtk_separator_menu_item_new();
            } else {
                const gchar *label = gtk_menu_item_get_label(GTK_MENU_ITEM(original_item));
                new_item = gtk_menu_item_new_with_label(label);
                
                // Copy the signal handlers
                g_object_set_data(G_OBJECT(new_item), "original-item", original_item);
                g_signal_connect_swapped(new_item, "activate", 
                                       G_CALLBACK(gtk_menu_item_activate), original_item);
            }
            
            gtk_menu_shell_append(GTK_MENU_SHELL(new_menu), new_item);
            gtk_widget_show(new_item);
        }
        g_list_free(children);
    }
    
    // Add application menu if available
    if (newtonmenu->app_dbusmenu_client && newtonmenu->active_wnck_window) {
        DbusmenuMenuitem *root = dbusmenu_client_get_root(newtonmenu->app_dbusmenu_client);
        
        if (root) {
            // Get application name
            if (!newtonmenu->hide_application_name_prop) {
                app_name = g_strdup(wnck_window_get_name(newtonmenu->active_wnck_window));
            } else {
                app_name = g_strdup(newtonmenu->global_menu_title_prop);
            }
            
            if (app_name && strlen(app_name) > 0) {
                // Add separator before app menu
                GtkWidget *separator = gtk_separator_menu_item_new();
                gtk_menu_shell_append(GTK_MENU_SHELL(new_menu), separator);
                gtk_widget_show(separator);
                
                // Add app name as a header (disabled menu item)
                GtkWidget *app_header = gtk_menu_item_new_with_label(app_name);
                gtk_widget_set_sensitive(app_header, FALSE);
                gtk_menu_shell_append(GTK_MENU_SHELL(new_menu), app_header);
                gtk_widget_show(app_header);
                
                // Add actual application menu items
                append_dbusmenu_items_to_gtk_menu(new_menu, root);
            }
        }
    }

    // Set the new menu on the button
    gtk_menu_button_set_popup(newtonmenu->button, new_menu);

    g_free(app_name);
}

/**
 * Convert DBusmenu items to GTK menu items and append them
 */
static void append_dbusmenu_items_to_gtk_menu(GtkWidget *gtk_menu, DbusmenuMenuitem *dbusmenu_item)
{
    GList *children, *iter;
    
    g_return_if_fail(GTK_IS_MENU(gtk_menu));
    g_return_if_fail(DBUSMENU_IS_MENUITEM(dbusmenu_item));
    
    children = dbusmenu_menuitem_get_children(dbusmenu_item);
    
    for (iter = children; iter != NULL; iter = iter->next) {
        DbusmenuMenuitem *child = DBUSMENU_MENUITEM(iter->data);
        GtkWidget *gtk_item = create_gtk_menuitem_from_dbusmenu_item(child);
        
        if (gtk_item) {
            gtk_menu_shell_append(GTK_MENU_SHELL(gtk_menu), gtk_item);
            gtk_widget_show(gtk_item);
        }
    }
}

/**
 * Create a GTK menu item from a DBusmenu item
 */
static GtkWidget* create_gtk_menuitem_from_dbusmenu_item(DbusmenuMenuitem *dbusmenu_item)
{
    GtkWidget *gtk_item = NULL;
    const gchar *label;
    const gchar *item_type;
    
    g_return_val_if_fail(DBUSMENU_IS_MENUITEM(dbusmenu_item), NULL);
    
    item_type = dbusmenu_menuitem_property_get(dbusmenu_item, DBUSMENU_MENUITEM_PROP_TYPE);
    
    // Handle separators
    if (g_strcmp0(item_type, DBUSMENU_CLIENT_TYPES_SEPARATOR) == 0) {
        gtk_item = gtk_separator_menu_item_new();
        return gtk_item;
    }
    
    // Get label
    label = dbusmenu_menuitem_property_get(dbusmenu_item, DBUSMENU_MENUITEM_PROP_LABEL);
    if (!label || strlen(label) == 0) {
        label = _("(No Label)");
    }
    
    // Create appropriate menu item type
    if (g_strcmp0(item_type, DBUSMENU_CLIENT_TYPES_DEFAULT) == 0 || item_type == NULL) {
        // Check if it has children (submenu)
        if (dbusmenu_menuitem_get_children(dbusmenu_item) != NULL) {
            gtk_item = gtk_menu_item_new_with_label(label);
            
            // Create submenu
            GtkWidget *submenu = gtk_menu_new();
            append_dbusmenu_items_to_gtk_menu(submenu, dbusmenu_item);
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(gtk_item), submenu);
        } else {
            gtk_item = gtk_menu_item_new_with_label(label);
        }
    } else if (g_strcmp0(item_type, "x-canonical-toggle") == 0) {
        gtk_item = gtk_check_menu_item_new_with_label(label);
        
        // Set toggle state
        const gchar *toggle_state = dbusmenu_menuitem_property_get(dbusmenu_item, 
                                                                   DBUSMENU_MENUITEM_PROP_TOGGLE_STATE);
        if (g_strcmp0(toggle_state, "1") == 0) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item), TRUE);
        }
    } else {
        // Default to regular menu item
        gtk_item = gtk_menu_item_new_with_label(label);
    }
    
    if (gtk_item) {
        // Check if item is enabled
        const gchar *enabled = dbusmenu_menuitem_property_get(dbusmenu_item, DBUSMENU_MENUITEM_PROP_ENABLED);
        if (g_strcmp0(enabled, "false") == 0) {
            gtk_widget_set_sensitive(gtk_item, FALSE);
        }
        
        // Connect activation signal
        g_object_set_data(G_OBJECT(gtk_item), "dbusmenu-item", dbusmenu_item);
        g_signal_connect(gtk_item, "activate", G_CALLBACK(on_dbusmenu_gtk_item_activated), dbusmenu_item);
    }
    
    return gtk_item;
}

/**
 * Handle activation of GTK menu items backed by DBusmenu items
 */
static void on_dbusmenu_gtk_item_activated(GtkMenuItem *gtk_item, gpointer user_data)
{
    DbusmenuMenuitem *dbusmenu_item = DBUSMENU_MENUITEM(user_data);
    
    g_return_if_fail(DBUSMENU_IS_MENUITEM(dbusmenu_item));
    
    // Send event to the DBusmenu item
    dbusmenu_menuitem_handle_event(dbusmenu_item, DBUSMENU_MENUITEM_EVENT_ACTIVATED, 
                                   NULL, gtk_get_current_event_time());
}


/**
 * Utility function to check if a window has an application menu
 */
gboolean newtonmenu_window_has_app_menu(newtonmenuPlugin *newtonmenu, WnckWindow *window)
{
    GError *error = NULL;
    gchar *service_name = NULL;
    gchar *object_path = NULL;
    gboolean has_menu = FALSE;

    g_return_val_if_fail(newtonmenu != NULL, FALSE);
    g_return_val_if_fail(window != NULL, FALSE);
    g_return_val_if_fail(newtonmenu->appmenu_registrar_proxy != NULL, FALSE);

    GVariant *result = g_dbus_proxy_call_sync(
        newtonmenu->appmenu_registrar_proxy,
        "GetMenuForWindow",
        g_variant_new("(u)", (guint32)wnck_window_get_xid(window)),
        G_DBUS_CALL_FLAGS_NONE,
        1000, // 1 second timeout for quick check
        NULL,
        &error);

    if (!error && result) {
        g_variant_get(result, "(so)", &service_name, &object_path);
        has_menu = (service_name && strlen(service_name) > 0 && 
                   g_strcmp0(object_path, "/") != 0);
        g_variant_unref(result);
    }

    g_free(service_name);
    g_free(object_path);
    g_clear_error(&error);

    return has_menu;
}