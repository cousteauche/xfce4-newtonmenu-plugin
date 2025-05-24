/*
 * Copyright (C) 2025 Adam
 * Xfce4 Newton Menu Plugin with Global Menu Integration
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <gdk/gdkkeysyms.h>
#include <libintl.h>

// Global menu includes
#include <libwnck/libwnck.h>
#include <gio/gio.h>
#include <libdbusmenu-glib/client.h>
#include <libdbusmenu-gtk/menu.h>

#include "newtonmenu.h"
#include "newtonmenu-dialogs.h" 
#include "newtonmenu-force-quit-dialog.h"

// Constants
#define DEFAULT_DISPLAY_ICON TRUE
#define DEFAULT_ICON_NAME "xfce4-newtonmenu-plugin"
#define DEFAULT_LABEL_TEXT N_("Menu")

#define DEFAULT_CONFIRM_LOGOUT FALSE
#define DEFAULT_CONFIRM_RESTART TRUE
#define DEFAULT_CONFIRM_SHUTDOWN TRUE
#define DEFAULT_CONFIRM_FORCE_QUIT FALSE 

// Global menu defaults
#define DEFAULT_HIDE_APPLICATION_NAME FALSE
#define DEFAULT_GLOBAL_MENU_TITLE N_("Applications")

// D-Bus constants for AppMenu Registrar
#define APPMENU_REGISTRAR_BUS_NAME "com.canonical.AppMenu.Registrar"
#define APPMENU_REGISTRAR_OBJECT_PATH "/com/canonical/AppMenu/Registrar"
#define APPMENU_REGISTRAR_INTERFACE_NAME "com.canonical.AppMenu.Registrar"

// Forward declarations
static void newtonmenu_construct(XfcePanelPlugin *plugin);
static void newtonmenu_read(newtonmenuPlugin *newtonmenu);
static void newtonmenu_free(XfcePanelPlugin *plugin, newtonmenuPlugin *newtonmenu);
static void newtonmenu_orientation_changed(XfcePanelPlugin *plugin, GtkOrientation orientation, newtonmenuPlugin *newtonmenu);
static gboolean newtonmenu_size_changed(XfcePanelPlugin *plugin, gint size, newtonmenuPlugin *newtonmenu);
static newtonmenuPlugin* newtonmenu_new(XfcePanelPlugin *plugin);

// Menu action callbacks
static void on_about_this_pc_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_system_settings_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_run_command_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_force_quit_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_sleep_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_restart_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_shutdown_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_lock_screen_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_log_out_activate(GtkMenuItem *menuitem, gpointer user_data);

// Global menu functions
static void newtonmenu_init_dbus_and_wnck(newtonmenuPlugin *newtonmenu);
static void newtonmenu_teardown_dbus_and_wnck(newtonmenuPlugin *newtonmenu);
static void on_wnck_active_window_changed(WnckScreen *screen, WnckWindow *prev_active_window, newtonmenuPlugin *newtonmenu);
static void on_appmenu_registrar_signal(GDBusProxy *proxy, const gchar *sender_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
static void newtonmenu_fetch_app_menu(newtonmenuPlugin *newtonmenu, WnckWindow *window);
static GtkWidget* newtonmenu_create_static_newton_menu(newtonmenuPlugin *newtonmenu);

// Utility functions
static void execute_command(const gchar *command);
static void newtonmenu_menu_button_toggled(GtkMenuButton *button, newtonmenuPlugin *newtonmenu);

XFCE_PANEL_PLUGIN_REGISTER(newtonmenu_construct);

static void execute_command(const gchar *command)
{
    GError *error = NULL;
    if (!g_spawn_command_line_async(command, &error)) {
        g_warning("Failed to execute command '%s': %s", command, error ? error->message : "Unknown error");
        if (error) g_error_free(error);
    }
}

void newtonmenu_save(XfcePanelPlugin *plugin, newtonmenuPlugin *newtonmenu)
{
    XfceRc *rc;
    gchar *file;

    g_return_if_fail(newtonmenu != NULL);
    g_return_if_fail(XFCE_IS_PANEL_PLUGIN(plugin));

    file = xfce_panel_plugin_save_location(plugin, TRUE);
    if (G_UNLIKELY(file == NULL)) {
        return;
    }

    rc = xfce_rc_simple_open(file, FALSE);
    g_free(file);

    if (G_LIKELY(rc != NULL)) {
        xfce_rc_write_bool_entry(rc, "DisplayIcon", newtonmenu->display_icon_prop);
        xfce_rc_write_bool_entry(rc, "HideApplicationName", newtonmenu->hide_application_name_prop);
        
        if (newtonmenu->global_menu_title_prop)
            xfce_rc_write_entry(rc, "GlobalMenuTitle", newtonmenu->global_menu_title_prop);
        else
            xfce_rc_write_entry(rc, "GlobalMenuTitle", "");

        if (newtonmenu->icon_name_prop)
            xfce_rc_write_entry(rc, "IconName", newtonmenu->icon_name_prop);
        else
            xfce_rc_write_entry(rc, "IconName", "");

        if (newtonmenu->label_text_prop)
            xfce_rc_write_entry(rc, "LabelText", newtonmenu->label_text_prop);
        else
            xfce_rc_write_entry(rc, "LabelText", "");
        
        xfce_rc_write_bool_entry(rc, "ConfirmLogout", newtonmenu->confirm_logout_prop);
        xfce_rc_write_bool_entry(rc, "ConfirmRestart", newtonmenu->confirm_restart_prop);
        xfce_rc_write_bool_entry(rc, "ConfirmShutdown", newtonmenu->confirm_shutdown_prop);
        xfce_rc_write_bool_entry(rc, "ConfirmForceQuit", newtonmenu->confirm_force_quit_prop);

        xfce_rc_close(rc);
    }
}

static void newtonmenu_read(newtonmenuPlugin *newtonmenu)
{
    XfceRc *rc;
    gchar *file;
    const gchar *value;

    g_return_if_fail(newtonmenu != NULL);
    g_return_if_fail(newtonmenu->plugin != NULL);

    file = xfce_panel_plugin_save_location(newtonmenu->plugin, TRUE);

    if (G_LIKELY(file != NULL)) {
        rc = xfce_rc_simple_open(file, TRUE);
        g_free(file);

        if (G_LIKELY(rc != NULL)) {
            newtonmenu->display_icon_prop = xfce_rc_read_bool_entry(rc, "DisplayIcon", DEFAULT_DISPLAY_ICON);
            newtonmenu->hide_application_name_prop = xfce_rc_read_bool_entry(rc, "HideApplicationName", DEFAULT_HIDE_APPLICATION_NAME);
            
            value = xfce_rc_read_entry(rc, "GlobalMenuTitle", _(DEFAULT_GLOBAL_MENU_TITLE));
            g_free(newtonmenu->global_menu_title_prop);
            newtonmenu->global_menu_title_prop = g_strdup(value);

            value = xfce_rc_read_entry(rc, "IconName", DEFAULT_ICON_NAME);
            g_free(newtonmenu->icon_name_prop);
            newtonmenu->icon_name_prop = g_strdup(value);

            value = xfce_rc_read_entry(rc, "LabelText", _(DEFAULT_LABEL_TEXT));
            g_free(newtonmenu->label_text_prop);
            newtonmenu->label_text_prop = g_strdup(value);
            
            newtonmenu->confirm_logout_prop = xfce_rc_read_bool_entry(rc, "ConfirmLogout", DEFAULT_CONFIRM_LOGOUT);
            newtonmenu->confirm_restart_prop = xfce_rc_read_bool_entry(rc, "ConfirmRestart", DEFAULT_CONFIRM_RESTART);
            newtonmenu->confirm_shutdown_prop = xfce_rc_read_bool_entry(rc, "ConfirmShutdown", DEFAULT_CONFIRM_SHUTDOWN);
            newtonmenu->confirm_force_quit_prop = xfce_rc_read_bool_entry(rc, "ConfirmForceQuit", DEFAULT_CONFIRM_FORCE_QUIT);
            
            xfce_rc_close(rc);
            return;
        }
    }

    // Set defaults
    newtonmenu->display_icon_prop = DEFAULT_DISPLAY_ICON;
    newtonmenu->hide_application_name_prop = DEFAULT_HIDE_APPLICATION_NAME;
    
    g_free(newtonmenu->icon_name_prop);
    newtonmenu->icon_name_prop = g_strdup(DEFAULT_ICON_NAME);
    g_free(newtonmenu->label_text_prop);
    newtonmenu->label_text_prop = g_strdup(_(DEFAULT_LABEL_TEXT));
    g_free(newtonmenu->global_menu_title_prop);
    newtonmenu->global_menu_title_prop = g_strdup(_(DEFAULT_GLOBAL_MENU_TITLE));

    newtonmenu->confirm_logout_prop = DEFAULT_CONFIRM_LOGOUT;
    newtonmenu->confirm_restart_prop = DEFAULT_CONFIRM_RESTART;
    newtonmenu->confirm_shutdown_prop = DEFAULT_CONFIRM_SHUTDOWN;
    newtonmenu->confirm_force_quit_prop = DEFAULT_CONFIRM_FORCE_QUIT;
}

void newtonmenu_update_display(newtonmenuPlugin *newtonmenu)
{
    GtkIconTheme *icon_theme = NULL;
    gint panel_icon_size;

    g_return_if_fail(newtonmenu != NULL);
    g_return_if_fail(newtonmenu->plugin != NULL);
    g_return_if_fail(GTK_IS_MENU_BUTTON(newtonmenu->button));
    g_return_if_fail(GTK_IS_BOX(newtonmenu->button_box));
    g_return_if_fail(GTK_IS_IMAGE(newtonmenu->icon_image));
    g_return_if_fail(GTK_IS_LABEL(newtonmenu->label_widget));

    if (newtonmenu->display_icon_prop) {
        gtk_widget_show(newtonmenu->icon_image);
        gtk_widget_hide(newtonmenu->label_widget);

        icon_theme = gtk_icon_theme_get_for_screen(gtk_widget_get_screen(GTK_WIDGET(newtonmenu->plugin)));
        panel_icon_size = xfce_panel_plugin_get_icon_size(newtonmenu->plugin);
        
        if (newtonmenu->icon_name_prop && strlen(newtonmenu->icon_name_prop) > 0) {
            if (gtk_icon_theme_has_icon(icon_theme, newtonmenu->icon_name_prop)) {
                gtk_image_set_from_icon_name(GTK_IMAGE(newtonmenu->icon_image),
                                        newtonmenu->icon_name_prop,
                                        GTK_ICON_SIZE_BUTTON);
                gtk_image_set_pixel_size(GTK_IMAGE(newtonmenu->icon_image), panel_icon_size);
            } else if (g_file_test(newtonmenu->icon_name_prop, G_FILE_TEST_IS_REGULAR)) {
                GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size(newtonmenu->icon_name_prop, panel_icon_size, panel_icon_size, NULL);
                if (pixbuf) {
                    gtk_image_set_from_pixbuf(GTK_IMAGE(newtonmenu->icon_image), pixbuf);
                    g_object_unref(pixbuf);
                } else {
                    gtk_image_set_from_icon_name(GTK_IMAGE(newtonmenu->icon_image), "image-missing", GTK_ICON_SIZE_BUTTON);
                    gtk_image_set_pixel_size(GTK_IMAGE(newtonmenu->icon_image), panel_icon_size);
                }
            } else {
                gtk_image_set_from_icon_name(GTK_IMAGE(newtonmenu->icon_image), "image-missing", GTK_ICON_SIZE_BUTTON);
                gtk_image_set_pixel_size(GTK_IMAGE(newtonmenu->icon_image), panel_icon_size);
            }
        } else {
            gtk_image_set_from_icon_name(GTK_IMAGE(newtonmenu->icon_image), "image-missing", GTK_ICON_SIZE_BUTTON);
            gtk_image_set_pixel_size(GTK_IMAGE(newtonmenu->icon_image), panel_icon_size);
        }
    } else {
        gtk_widget_hide(newtonmenu->icon_image);
        gtk_widget_show(newtonmenu->label_widget);
        gtk_label_set_text(GTK_LABEL(newtonmenu->label_widget), 
                          newtonmenu->label_text_prop ? _(newtonmenu->label_text_prop) : "");
    }

    gtk_widget_queue_resize(GTK_WIDGET(newtonmenu->plugin));
}

static newtonmenuPlugin* newtonmenu_new(XfcePanelPlugin *plugin)
{
    newtonmenuPlugin *newtonmenu;
    GtkOrientation orientation;

    newtonmenu = g_slice_new0(newtonmenuPlugin);
    newtonmenu->plugin = plugin;
    newtonmenu_read(newtonmenu);

    orientation = xfce_panel_plugin_get_orientation(plugin);

    newtonmenu->event_box = gtk_event_box_new();
    gtk_widget_show(newtonmenu->event_box);

    // Use GtkMenuButton for dynamic menu model binding
    newtonmenu->button = GTK_MENU_BUTTON(gtk_menu_button_new());
    gtk_button_set_relief(GTK_BUTTON(newtonmenu->button), GTK_RELIEF_NONE);
    gtk_widget_show(GTK_WIDGET(newtonmenu->button));

    // Container for icon/label within the menu button
    newtonmenu->button_box = gtk_box_new(orientation, 2);
    gtk_container_set_border_width(GTK_CONTAINER(newtonmenu->button_box), 2);
    gtk_widget_show(newtonmenu->button_box);
    gtk_container_add(GTK_CONTAINER(newtonmenu->button), newtonmenu->button_box);

    newtonmenu->icon_image = gtk_image_new();
    gtk_box_pack_start(GTK_BOX(newtonmenu->button_box), newtonmenu->icon_image, FALSE, FALSE, 0);

    newtonmenu->label_widget = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(newtonmenu->button_box), newtonmenu->label_widget, TRUE, TRUE, 0);
    
    // Add button to event box
    gtk_container_add(GTK_CONTAINER(newtonmenu->event_box), GTK_WIDGET(newtonmenu->button));
    
    newtonmenu_update_display(newtonmenu);
    
    // Initialize global menu components
    newtonmenu->static_newton_menu = newtonmenu_create_static_newton_menu(newtonmenu);
    newtonmenu->app_menu_model = NULL;
    newtonmenu->app_dbusmenu_client = NULL;
    
    // Initialize DBus and WNCK for global menu functionality
    newtonmenu_init_dbus_and_wnck(newtonmenu);
    
    // Set initial combined menu
    newtonmenu_update_combined_menu(newtonmenu);

    // Connect to menu button signals for panel autohide
    g_signal_connect(newtonmenu->button, "toggled", G_CALLBACK(newtonmenu_menu_button_toggled), newtonmenu);

    return newtonmenu;
}

static void newtonmenu_free(XfcePanelPlugin *plugin, newtonmenuPlugin *newtonmenu)
{
    GtkWidget *dialog;

    g_return_if_fail(newtonmenu != NULL);

    dialog = g_object_get_data(G_OBJECT(plugin), "dialog");
    if (G_UNLIKELY(dialog != NULL))
        gtk_widget_destroy(dialog);

    // Free configuration strings
    g_free(newtonmenu->icon_name_prop);
    g_free(newtonmenu->label_text_prop);
    g_free(newtonmenu->global_menu_title_prop);

    // Release global menu resources
    if (newtonmenu->static_newton_menu)
        gtk_widget_destroy(newtonmenu->static_newton_menu);
    if (newtonmenu->app_menu_model)
        g_object_unref(newtonmenu->app_menu_model);
    if (newtonmenu->app_dbusmenu_client)
        g_object_unref(newtonmenu->app_dbusmenu_client);
    
    // Cleanup DBus and WNCK
    newtonmenu_teardown_dbus_and_wnck(newtonmenu);
    
    g_slice_free(newtonmenuPlugin, newtonmenu);
}

static void newtonmenu_orientation_changed(XfcePanelPlugin *plugin, GtkOrientation orientation, newtonmenuPlugin *newtonmenu)
{
    g_return_if_fail(newtonmenu != NULL);
    g_return_if_fail(GTK_IS_BOX(newtonmenu->button_box));

    gtk_orientable_set_orientation(GTK_ORIENTABLE(newtonmenu->button_box), orientation);
    newtonmenu_update_display(newtonmenu);
}

static gboolean newtonmenu_size_changed(XfcePanelPlugin *plugin, gint size, newtonmenuPlugin *newtonmenu)
{
    GtkOrientation orientation;

    g_return_val_if_fail(newtonmenu != NULL, TRUE);

    orientation = xfce_panel_plugin_get_orientation(plugin);

    if (orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_set_size_request(GTK_WIDGET(plugin), -1, size);
    else
        gtk_widget_set_size_request(GTK_WIDGET(plugin), size, -1);

    newtonmenu_update_display(newtonmenu);
    return TRUE;
}

// Menu action implementations
static void on_about_this_pc_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    execute_command("xfce4-about");
}

static void on_system_settings_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    execute_command("xfce4-settings-manager");
}

static void on_run_command_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    execute_command("xfce4-appfinder --collapsed");
}

static void on_force_quit_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    newtonmenuPlugin *newtonmenu = (newtonmenuPlugin*)user_data;
    GtkWindow *parent_window = NULL;
    
    g_return_if_fail(newtonmenu != NULL);

    if (newtonmenu->plugin && gtk_widget_get_toplevel(GTK_WIDGET(newtonmenu->plugin))) {
        parent_window = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(newtonmenu->plugin)));
    }

    newtonmenu_show_force_quit_applications_dialog(parent_window, newtonmenu);
}

static void on_sleep_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    execute_command("xfce4-session-logout --suspend");
}

static void on_restart_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    newtonmenuPlugin *newtonmenu = (newtonmenuPlugin*)user_data;
    GtkWindow *parent_window = NULL;

    g_return_if_fail(newtonmenu != NULL);

    if (newtonmenu->plugin && gtk_widget_get_toplevel(GTK_WIDGET(newtonmenu->plugin))) {
        parent_window = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(newtonmenu->plugin)));
    }

    if (newtonmenu->confirm_restart_prop) {
        newtonmenu_show_generic_confirmation(parent_window, _("restart"), _("Restart"), "xfce4-session-logout --reboot");
    } else {
        execute_command("xfce4-session-logout --reboot");
    }
}

static void on_shutdown_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    newtonmenuPlugin *newtonmenu = (newtonmenuPlugin*)user_data;
    GtkWindow *parent_window = NULL;

    g_return_if_fail(newtonmenu != NULL);

    if (newtonmenu->plugin && gtk_widget_get_toplevel(GTK_WIDGET(newtonmenu->plugin))) {
        parent_window = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(newtonmenu->plugin)));
    }
    
    if (newtonmenu->confirm_shutdown_prop) {
        newtonmenu_show_generic_confirmation(parent_window, _("shut down"), _("Shut Down"), "xfce4-session-logout --halt");
    } else {
        execute_command("xfce4-session-logout --halt");
    }
}

static void on_lock_screen_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    execute_command("xflock4");
}

static void on_log_out_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    newtonmenuPlugin *newtonmenu = (newtonmenuPlugin*)user_data;
    GtkWindow *parent_window = NULL;
    const gchar *username = g_get_user_name();
    gchar *action_name; 

    g_return_if_fail(newtonmenu != NULL);

    if (newtonmenu->plugin && gtk_widget_get_toplevel(GTK_WIDGET(newtonmenu->plugin))) {
        parent_window = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(newtonmenu->plugin)));
    }

    if (username) {
        action_name = g_strdup_printf(_("Log Out %s..."), username); 
    } else {
        action_name = g_strdup(_("Log Out...")); 
    }

    if (newtonmenu->confirm_logout_prop) {
        newtonmenu_show_generic_confirmation(parent_window, action_name, _("Log Out"), "xfce4-session-logout --logout");
    } else {
        execute_command("xfce4-session-logout --logout");
    }
    g_free(action_name);
}

// Create the static Newton Menu using traditional GtkMenu
static GtkWidget* newtonmenu_create_static_newton_menu(newtonmenuPlugin *newtonmenu)
{
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *menu_item;

    // About This PC
    menu_item = gtk_menu_item_new_with_label(_("About This PC"));
    g_signal_connect(menu_item, "activate", G_CALLBACK(on_about_this_pc_activate), newtonmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);

    // Separator
    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);

    // System Settings
    menu_item = gtk_menu_item_new_with_label(_("System Settings..."));
    g_signal_connect(menu_item, "activate", G_CALLBACK(on_system_settings_activate), newtonmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);

    // Run Command
    menu_item = gtk_menu_item_new_with_label(_("Run Command..."));
    g_signal_connect(menu_item, "activate", G_CALLBACK(on_run_command_activate), newtonmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);
    
    // Separator
    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);

    // Force Quit
    menu_item = gtk_menu_item_new_with_label(_("Force Quit..."));
    g_signal_connect(menu_item, "activate", G_CALLBACK(on_force_quit_activate), newtonmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);

    // Separator
    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);

    // Session Management
    menu_item = gtk_menu_item_new_with_label(_("Sleep"));
    g_signal_connect(menu_item, "activate", G_CALLBACK(on_sleep_activate), newtonmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);
    
    menu_item = gtk_menu_item_new_with_label(_("Restart..."));
    g_signal_connect(menu_item, "activate", G_CALLBACK(on_restart_activate), newtonmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);

    menu_item = gtk_menu_item_new_with_label(_("Shut Down..."));
    g_signal_connect(menu_item, "activate", G_CALLBACK(on_shutdown_activate), newtonmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);
    
    // Separator
    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);

    menu_item = gtk_menu_item_new_with_label(_("Lock Screen"));
    g_signal_connect(menu_item, "activate", G_CALLBACK(on_lock_screen_activate), newtonmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);

    // Logout
    const gchar* username = g_get_user_name();
    gchar* logout_label;
    if (username) {
        logout_label = g_strdup_printf(_("Log Out %s..."), username);
    } else {
        logout_label = g_strdup(_("Log Out..."));
    }
    menu_item = gtk_menu_item_new_with_label(logout_label);
    g_signal_connect(menu_item, "activate", G_CALLBACK(on_log_out_activate), newtonmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);
    g_free(logout_label);
    
    return menu;
}

void newtonmenu_update_combined_menu(newtonmenuPlugin *newtonmenu)
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
                
                // Copy the signal handlers (this is a simplified approach)
                g_object_set_data(G_OBJECT(new_item), "original-item", original_item);
                g_signal_connect_swapped(new_item, "activate", 
                                       G_CALLBACK(gtk_menu_item_activate), original_item);
            }
            
            gtk_menu_shell_append(GTK_MENU_SHELL(new_menu), new_item);
            gtk_widget_show(new_item);
        }
        g_list_free(children);
    }
    
    // Add application-specific items if we have an active window
    if (newtonmenu->active_wnck_window) {
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
            
            // Add app name as a disabled menu item (header)
            GtkWidget *app_header = gtk_menu_item_new_with_label(app_name);
            gtk_widget_set_sensitive(app_header, FALSE);
            gtk_menu_shell_append(GTK_MENU_SHELL(new_menu), app_header);
            gtk_widget_show(app_header);
            
            // Here you would add actual application menu items
            // For now, we'll add a placeholder
            GtkWidget *placeholder = gtk_menu_item_new_with_label(_("(Application menu will appear here)"));
            gtk_widget_set_sensitive(placeholder, FALSE);
            gtk_menu_shell_append(GTK_MENU_SHELL(new_menu), placeholder);
            gtk_widget_show(placeholder);
        }
    }

    // Set the new menu on the button
    gtk_menu_button_set_popup(newtonmenu->button, new_menu);

    g_free(app_name);
}

// Signal handler for menu button toggle (for panel autohide)
static void newtonmenu_menu_button_toggled(GtkMenuButton *button, newtonmenuPlugin *newtonmenu)
{
    g_return_if_fail(newtonmenu != NULL);
    
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) {
        xfce_panel_plugin_block_autohide(newtonmenu->plugin, TRUE);
    } else {
        xfce_panel_plugin_block_autohide(newtonmenu->plugin, FALSE);
    }
}

// Global menu integration functions
static void newtonmenu_init_dbus_and_wnck(newtonmenuPlugin *newtonmenu)
{
    GError *error = NULL;

    // Initialize WnckScreen for window tracking
    newtonmenu->wnck_screen = wnck_screen_get_default();
    newtonmenu->wnck_active_window_changed_handler_id = g_signal_connect(
        newtonmenu->wnck_screen, "active-window-changed",
        G_CALLBACK(on_wnck_active_window_changed), newtonmenu);

    // Initialize D-Bus session bus connection
    newtonmenu->dbus_session_bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (!newtonmenu->dbus_session_bus) {
        g_warning("Failed to connect to D-Bus session bus: %s", error ? error->message : "Unknown error");
        g_clear_error(&error);
        return;
    }

    // Create D-Bus proxy for AppMenu Registrar
    newtonmenu->appmenu_registrar_proxy = g_dbus_proxy_new_sync(
        newtonmenu->dbus_session_bus,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        APPMENU_REGISTRAR_BUS_NAME,
        APPMENU_REGISTRAR_OBJECT_PATH,
        APPMENU_REGISTRAR_INTERFACE_NAME,
        NULL,
        &error);
    
    if (!newtonmenu->appmenu_registrar_proxy) {
        g_warning("Failed to create AppMenu Registrar D-Bus proxy: %s", error ? error->message : "Unknown error");
        g_clear_error(&error);
        g_object_unref(newtonmenu->dbus_session_bus);
        newtonmenu->dbus_session_bus = NULL;
        return;
    }

    // Connect to D-Bus signals
    newtonmenu->appmenu_registrar_registered_handler_id = g_signal_connect(
        newtonmenu->appmenu_registrar_proxy, "g-signal",
        G_CALLBACK(on_appmenu_registrar_signal), newtonmenu);

    // Trigger initial menu update
    on_wnck_active_window_changed(newtonmenu->wnck_screen, NULL, newtonmenu);
}

static void newtonmenu_teardown_dbus_and_wnck(newtonmenuPlugin *newtonmenu)
{
    if (newtonmenu->wnck_active_window_changed_handler_id > 0) {
        g_signal_handler_disconnect(newtonmenu->wnck_screen, newtonmenu->wnck_active_window_changed_handler_id);
        newtonmenu->wnck_active_window_changed_handler_id = 0;
    }
    if (newtonmenu->wnck_screen) {
        g_object_unref(newtonmenu->wnck_screen);
        newtonmenu->wnck_screen = NULL;
    }
    if (newtonmenu->active_wnck_window) {
        g_object_unref(newtonmenu->active_wnck_window);
        newtonmenu->active_wnck_window = NULL;
    }

    if (newtonmenu->appmenu_registrar_registered_handler_id > 0) {
        g_signal_handler_disconnect(newtonmenu->appmenu_registrar_proxy, newtonmenu->appmenu_registrar_registered_handler_id);
        newtonmenu->appmenu_registrar_registered_handler_id = 0;
    }
    if (newtonmenu->appmenu_registrar_proxy) {
        g_object_unref(newtonmenu->appmenu_registrar_proxy);
        newtonmenu->appmenu_registrar_proxy = NULL;
    }
    if (newtonmenu->dbus_session_bus) {
        g_object_unref(newtonmenu->dbus_session_bus);
        newtonmenu->dbus_session_bus = NULL;
    }
}

static void on_wnck_active_window_changed(WnckScreen *screen, WnckWindow *prev_active_window, newtonmenuPlugin *newtonmenu)
{
    WnckWindow *current_active_window = wnck_screen_get_active_window(screen);

    // Release previous active window reference
    if (newtonmenu->active_wnck_window) {
        g_object_unref(newtonmenu->active_wnck_window);
        newtonmenu->active_wnck_window = NULL;
    }

    if (current_active_window) {
        newtonmenu->active_wnck_window = g_object_ref(current_active_window);
        g_debug("Active window changed to %s (XID: %lu)",
                wnck_window_get_name(current_active_window),
                (gulong)wnck_window_get_xid(current_active_window));
        newtonmenu_fetch_app_menu(newtonmenu, newtonmenu->active_wnck_window);
    } else {
        g_debug("No active window. Displaying default menu.");
        newtonmenu_update_combined_menu(newtonmenu);
    }
}

static void on_appmenu_registrar_signal(GDBusProxy *proxy, const gchar *sender_name, const gchar *signal_name, GVariant *parameters, gpointer user_data)
{
    newtonmenuPlugin *newtonmenu = (newtonmenuPlugin*)user_data;

    if (g_strcmp0(signal_name, "WindowRegistered") == 0) {
        guint32 window_id;
        gchar *service_name = NULL;
        gchar *object_path = NULL;

        g_variant_get(parameters, "(uso)", &window_id, &service_name, &object_path);

        if (newtonmenu->active_wnck_window && wnck_window_get_xid(newtonmenu->active_wnck_window) == window_id) {
            g_debug("AppMenu registered for active window %lu", (gulong)window_id);
            newtonmenu_fetch_app_menu(newtonmenu, newtonmenu->active_wnck_window);
        }

        g_free(service_name);
        g_free(object_path);
    } else if (g_strcmp0(signal_name, "WindowUnregistered") == 0) {
        guint32 window_id;
        g_variant_get(parameters, "(u)", &window_id);

        if (newtonmenu->active_wnck_window && wnck_window_get_xid(newtonmenu->active_wnck_window) == window_id) {
            g_debug("AppMenu unregistered for active window %lu", (gulong)window_id);
            if (newtonmenu->app_menu_model) {
                g_object_unref(newtonmenu->app_menu_model);
                newtonmenu->app_menu_model = NULL;
            }
            newtonmenu_update_combined_menu(newtonmenu);
        }
    }
}

static void newtonmenu_fetch_app_menu(newtonmenuPlugin *newtonmenu, WnckWindow *window)
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
        g_warning("Failed to get menu for window %lu: %s", (gulong)wnck_window_get_xid(window), error->message);
        g_clear_error(&error);
        newtonmenu_update_combined_menu(newtonmenu);
        return;
    }

    g_variant_get(result, "(so)", &service_name, &object_path);
    g_variant_unref(result);

    // Check if we have a valid menu
    if (service_name && object_path && strlen(service_name) > 0 && g_strcmp0(object_path, "/") != 0) {
        g_debug("Found menu for window %lu: service=%s, path=%s", 
                (gulong)wnck_window_get_xid(window), service_name, object_path);
        
        // Here you would create a DbusmenuClient and convert it to a GMenuModel
        // For now, we'll just update the combined menu
        newtonmenu_update_combined_menu(newtonmenu);
    } else {
        g_debug("No menu available for window %lu", (gulong)wnck_window_get_xid(window));
        newtonmenu_update_combined_menu(newtonmenu);
    }

    g_free(service_name);
    g_free(object_path);
}

static void newtonmenu_construct(XfcePanelPlugin *plugin)
{
    newtonmenuPlugin *newtonmenu;

    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
    newtonmenu = newtonmenu_new(plugin);
    g_return_if_fail(newtonmenu != NULL);

    gtk_container_add(GTK_CONTAINER(plugin), newtonmenu->event_box);
    xfce_panel_plugin_add_action_widget(plugin, newtonmenu->event_box);
    
    g_signal_connect(G_OBJECT(plugin), "free-data", G_CALLBACK(newtonmenu_free), newtonmenu);
    g_signal_connect(G_OBJECT(plugin), "save", G_CALLBACK(newtonmenu_save), newtonmenu);
    g_signal_connect(G_OBJECT(plugin), "size-changed", G_CALLBACK(newtonmenu_size_changed), newtonmenu);
    g_signal_connect(G_OBJECT(plugin), "orientation-changed", G_CALLBACK(newtonmenu_orientation_changed), newtonmenu);

    xfce_panel_plugin_menu_show_configure(plugin);
    g_signal_connect(G_OBJECT(plugin), "configure-plugin", G_CALLBACK(newtonmenu_configure), newtonmenu);
    xfce_panel_plugin_menu_show_about(plugin);
    g_signal_connect(G_OBJECT(plugin), "about", G_CALLBACK(newtonmenu_about), plugin);
}