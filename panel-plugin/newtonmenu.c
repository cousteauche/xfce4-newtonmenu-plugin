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

#include <libwnck/libwnck.h>
#include <gio/gio.h>
#include <libdbusmenu-glib/client.h>

#include "newtonmenu.h"
#include "newtonmenu-dialogs.h" 
#include "newtonmenu-force-quit-dialog.h"
#include "dbusmenu-integration.h" 

#define DEFAULT_DISPLAY_ICON TRUE
#define DEFAULT_ICON_NAME "xfce4-newtonmenu-plugin" 
#define DEFAULT_LABEL_TEXT N_("Menu")

#define DEFAULT_CONFIRM_LOGOUT FALSE
#define DEFAULT_CONFIRM_RESTART TRUE
#define DEFAULT_CONFIRM_SHUTDOWN TRUE
#define DEFAULT_CONFIRM_FORCE_QUIT FALSE 

#define DEFAULT_HIDE_APPLICATION_NAME FALSE 
#define DEFAULT_GLOBAL_MENU_TITLE N_("Application") 
#define DEFAULT_SHOW_APP_NAME_BUTTON TRUE 
#define DEFAULT_BOLD_APP_NAME FALSE       

#define APPMENU_REGISTRAR_BUS_NAME "com.canonical.AppMenu.Registrar"
#define APPMENU_REGISTRAR_OBJECT_PATH "/com/canonical/AppMenu/Registrar"
#define APPMENU_REGISTRAR_INTERFACE_NAME "com.canonical.AppMenu.Registrar"
#define NEWTONMENU_DBUS_WELL_KNOWN_NAME "org.xfce.NewtonMenuService" // Example D-Bus name

static void newtonmenu_construct(XfcePanelPlugin *plugin);
static void newtonmenu_read_config(newtonmenuPlugin *newtonmenu); 
static void newtonmenu_free(XfcePanelPlugin *plugin, newtonmenuPlugin *newtonmenu);
static void newtonmenu_orientation_changed(XfcePanelPlugin *plugin, GtkOrientation orientation, newtonmenuPlugin *newtonmenu);
static gboolean newtonmenu_size_changed(XfcePanelPlugin *plugin, gint size, newtonmenuPlugin *newtonmenu);
static newtonmenuPlugin* newtonmenu_new(XfcePanelPlugin *plugin);

static void on_about_this_pc_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_system_settings_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_run_command_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_force_quit_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_sleep_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_restart_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_shutdown_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_lock_screen_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_log_out_activate(GtkMenuItem *menuitem, gpointer user_data);

static void newtonmenu_init_dbus_and_wnck(newtonmenuPlugin *newtonmenu);
static void newtonmenu_teardown_dbus_and_wnck(newtonmenuPlugin *newtonmenu);
static void on_wnck_active_window_changed(WnckScreen *screen, WnckWindow *prev_active_window, newtonmenuPlugin *newtonmenu);
static void on_appmenu_registrar_signal(GDBusProxy *proxy, const gchar *sender_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
static GtkWidget* newtonmenu_create_static_newton_menu(newtonmenuPlugin *newtonmenu);

static void execute_command(const gchar *command);

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
    if (G_UNLIKELY(file == NULL)) return;

    rc = xfce_rc_simple_open(file, FALSE);
    g_free(file);

    if (G_LIKELY(rc != NULL)) {
        xfce_rc_write_bool_entry(rc, "DisplayIcon", newtonmenu->display_icon_prop);
        xfce_rc_write_entry(rc, "IconName", newtonmenu->icon_name_prop ? newtonmenu->icon_name_prop : "");
        xfce_rc_write_entry(rc, "LabelText", newtonmenu->label_text_prop ? newtonmenu->label_text_prop : "");
        
        xfce_rc_write_bool_entry(rc, "HideApplicationName", newtonmenu->hide_application_name_prop);
        xfce_rc_write_entry(rc, "GlobalMenuTitle", newtonmenu->global_menu_title_prop ? newtonmenu->global_menu_title_prop : "");
        xfce_rc_write_bool_entry(rc, "ShowAppNameButton", newtonmenu->show_app_name_button_prop);
        xfce_rc_write_bool_entry(rc, "BoldAppName", newtonmenu->bold_app_name_prop);

        xfce_rc_write_bool_entry(rc, "ConfirmLogout", newtonmenu->confirm_logout_prop);
        xfce_rc_write_bool_entry(rc, "ConfirmRestart", newtonmenu->confirm_restart_prop);
        xfce_rc_write_bool_entry(rc, "ConfirmShutdown", newtonmenu->confirm_shutdown_prop);
        xfce_rc_write_bool_entry(rc, "ConfirmForceQuit", newtonmenu->confirm_force_quit_prop);
        xfce_rc_close(rc);
    }
}

static void newtonmenu_read_config(newtonmenuPlugin *newtonmenu)
{
    XfceRc *rc;
    gchar *file;

    g_return_if_fail(newtonmenu != NULL);
    g_return_if_fail(newtonmenu->plugin != NULL);

    file = xfce_panel_plugin_save_location(newtonmenu->plugin, TRUE);

    if (G_LIKELY(file != NULL)) {
        rc = xfce_rc_simple_open(file, TRUE);
        g_free(file);
        if (G_LIKELY(rc != NULL)) {
            newtonmenu->display_icon_prop = xfce_rc_read_bool_entry(rc, "DisplayIcon", DEFAULT_DISPLAY_ICON);
            newtonmenu->icon_name_prop = g_strdup(xfce_rc_read_entry(rc, "IconName", DEFAULT_ICON_NAME));
            newtonmenu->label_text_prop = g_strdup(xfce_rc_read_entry(rc, "LabelText", _(DEFAULT_LABEL_TEXT)));
            
            newtonmenu->hide_application_name_prop = xfce_rc_read_bool_entry(rc, "HideApplicationName", DEFAULT_HIDE_APPLICATION_NAME);
            newtonmenu->global_menu_title_prop = g_strdup(xfce_rc_read_entry(rc, "GlobalMenuTitle", _(DEFAULT_GLOBAL_MENU_TITLE)));
            newtonmenu->show_app_name_button_prop = xfce_rc_read_bool_entry(rc, "ShowAppNameButton", DEFAULT_SHOW_APP_NAME_BUTTON);
            newtonmenu->bold_app_name_prop = xfce_rc_read_bool_entry(rc, "BoldAppName", DEFAULT_BOLD_APP_NAME);

            newtonmenu->confirm_logout_prop = xfce_rc_read_bool_entry(rc, "ConfirmLogout", DEFAULT_CONFIRM_LOGOUT);
            newtonmenu->confirm_restart_prop = xfce_rc_read_bool_entry(rc, "ConfirmRestart", DEFAULT_CONFIRM_RESTART);
            newtonmenu->confirm_shutdown_prop = xfce_rc_read_bool_entry(rc, "ConfirmShutdown", DEFAULT_CONFIRM_SHUTDOWN);
            newtonmenu->confirm_force_quit_prop = xfce_rc_read_bool_entry(rc, "ConfirmForceQuit", DEFAULT_CONFIRM_FORCE_QUIT);
            xfce_rc_close(rc);
            return;
        }
    }

    newtonmenu->display_icon_prop = DEFAULT_DISPLAY_ICON;
    newtonmenu->icon_name_prop = g_strdup(DEFAULT_ICON_NAME);
    newtonmenu->label_text_prop = g_strdup(_(DEFAULT_LABEL_TEXT));
    newtonmenu->hide_application_name_prop = DEFAULT_HIDE_APPLICATION_NAME;
    newtonmenu->global_menu_title_prop = g_strdup(_(DEFAULT_GLOBAL_MENU_TITLE));
    newtonmenu->show_app_name_button_prop = DEFAULT_SHOW_APP_NAME_BUTTON;
    newtonmenu->bold_app_name_prop = DEFAULT_BOLD_APP_NAME;
    newtonmenu->confirm_logout_prop = DEFAULT_CONFIRM_LOGOUT;
    newtonmenu->confirm_restart_prop = DEFAULT_CONFIRM_RESTART;
    newtonmenu->confirm_shutdown_prop = DEFAULT_CONFIRM_SHUTDOWN;
    newtonmenu->confirm_force_quit_prop = DEFAULT_CONFIRM_FORCE_QUIT;
}

void newtonmenu_update_display(newtonmenuPlugin *newtonmenu)
{
    GtkIconTheme *icon_theme;
    gint panel_icon_size;

    g_return_if_fail(newtonmenu != NULL);
    g_return_if_fail(newtonmenu->plugin != NULL);
    g_return_if_fail(GTK_IS_MENU_BUTTON(newtonmenu->newton_button));
    g_return_if_fail(GTK_IS_BOX(newtonmenu->newton_button_box));
    g_return_if_fail(GTK_IS_IMAGE(newtonmenu->newton_icon_image));
    g_return_if_fail(GTK_IS_LABEL(newtonmenu->newton_label_widget));

    if (newtonmenu->display_icon_prop) {
        gtk_widget_show(newtonmenu->newton_icon_image);
        gtk_widget_hide(newtonmenu->newton_label_widget);
        icon_theme = gtk_icon_theme_get_for_screen(gtk_widget_get_screen(GTK_WIDGET(newtonmenu->plugin)));
        panel_icon_size = xfce_panel_plugin_get_icon_size(newtonmenu->plugin);
        if (newtonmenu->icon_name_prop && strlen(newtonmenu->icon_name_prop) > 0) {
            if (gtk_icon_theme_has_icon(icon_theme, newtonmenu->icon_name_prop)) {
                gtk_image_set_from_icon_name(GTK_IMAGE(newtonmenu->newton_icon_image), newtonmenu->icon_name_prop, GTK_ICON_SIZE_BUTTON);
                gtk_image_set_pixel_size(GTK_IMAGE(newtonmenu->newton_icon_image), panel_icon_size);
            } else if (g_file_test(newtonmenu->icon_name_prop, G_FILE_TEST_IS_REGULAR)) {
                GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size(newtonmenu->icon_name_prop, panel_icon_size, panel_icon_size, NULL);
                if (pixbuf) { gtk_image_set_from_pixbuf(GTK_IMAGE(newtonmenu->newton_icon_image), pixbuf); g_object_unref(pixbuf); }
                else { gtk_image_set_from_icon_name(GTK_IMAGE(newtonmenu->newton_icon_image), "image-missing", GTK_ICON_SIZE_BUTTON); gtk_image_set_pixel_size(GTK_IMAGE(newtonmenu->newton_icon_image), panel_icon_size); }
            } else { gtk_image_set_from_icon_name(GTK_IMAGE(newtonmenu->newton_icon_image), "image-missing", GTK_ICON_SIZE_BUTTON); gtk_image_set_pixel_size(GTK_IMAGE(newtonmenu->newton_icon_image), panel_icon_size); }
        } else { gtk_image_set_from_icon_name(GTK_IMAGE(newtonmenu->newton_icon_image), "image-missing", GTK_ICON_SIZE_BUTTON); gtk_image_set_pixel_size(GTK_IMAGE(newtonmenu->newton_icon_image), panel_icon_size); }
    } else {
        gtk_widget_hide(newtonmenu->newton_icon_image);
        gtk_widget_show(newtonmenu->newton_label_widget);
        gtk_label_set_text(GTK_LABEL(newtonmenu->newton_label_widget), newtonmenu->label_text_prop ? _(newtonmenu->label_text_prop) : "");
    }
    gtk_widget_queue_resize(GTK_WIDGET(newtonmenu->plugin));
}

void newtonmenu_clear_dynamic_app_menus(newtonmenuPlugin *newtonmenu) {
    g_return_if_fail(newtonmenu != NULL);
    if (newtonmenu->app_name_button) {
        gtk_widget_destroy(GTK_WIDGET(newtonmenu->app_name_button));
        newtonmenu->app_name_button = NULL;
        newtonmenu->app_name_button_label = NULL;
    }
    if (newtonmenu->dynamic_app_menu_buttons) {
        g_list_free_full(newtonmenu->dynamic_app_menu_buttons, (GDestroyNotify)gtk_widget_destroy);
        newtonmenu->dynamic_app_menu_buttons = NULL;
    }
    GList *children = gtk_container_get_children(GTK_CONTAINER(newtonmenu->app_menu_bar_container));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
    gtk_widget_hide(newtonmenu->app_menu_bar_container);

    if (newtonmenu->app_dbusmenu_client) {
        if (newtonmenu->app_dbusmenu_client_root_changed_id > 0) {
            if (g_signal_handler_is_connected(newtonmenu->app_dbusmenu_client, newtonmenu->app_dbusmenu_client_root_changed_id)) {
                g_signal_handler_disconnect(newtonmenu->app_dbusmenu_client, newtonmenu->app_dbusmenu_client_root_changed_id);
            }
            newtonmenu->app_dbusmenu_client_root_changed_id = 0;
        }
        if (newtonmenu->app_dbusmenu_client_layout_updated_id > 0) {
            if (g_signal_handler_is_connected(newtonmenu->app_dbusmenu_client, newtonmenu->app_dbusmenu_client_layout_updated_id)) {
                g_signal_handler_disconnect(newtonmenu->app_dbusmenu_client, newtonmenu->app_dbusmenu_client_layout_updated_id);
            }
            newtonmenu->app_dbusmenu_client_layout_updated_id = 0;
        }
    }
}

gboolean on_menu_button_enter(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data) {
    newtonmenuPlugin *newtonmenu = (newtonmenuPlugin*)user_data;
    GtkMenuButton *button_entered = GTK_MENU_BUTTON(widget);
    if (newtonmenu->currently_open_button && 
        newtonmenu->currently_open_button != button_entered &&
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(newtonmenu->currently_open_button))) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(newtonmenu->currently_open_button), FALSE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_entered), TRUE);
    }
    return GDK_EVENT_PROPAGATE;
}

void newtonmenu_menu_button_toggled(GtkMenuButton *button, newtonmenuPlugin *newtonmenu)
{
    g_return_if_fail(newtonmenu != NULL);
    
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) {
        xfce_panel_plugin_block_autohide(newtonmenu->plugin, TRUE);
        newtonmenu->currently_open_button = button;
    } else {
        xfce_panel_plugin_block_autohide(newtonmenu->plugin, FALSE);
        if (newtonmenu->currently_open_button == button) {
            newtonmenu->currently_open_button = NULL;
        }
    }
}

static newtonmenuPlugin* newtonmenu_new(XfcePanelPlugin *plugin) {
    newtonmenuPlugin *newtonmenu = g_slice_new0(newtonmenuPlugin);
    newtonmenu->plugin = plugin;
    newtonmenu_read_config(newtonmenu);
    GtkOrientation orientation = xfce_panel_plugin_get_orientation(plugin);

    newtonmenu->main_box = gtk_box_new(orientation, 0);
    gtk_widget_show(newtonmenu->main_box);

    newtonmenu->newton_button = GTK_MENU_BUTTON(gtk_menu_button_new());
    gtk_button_set_relief(GTK_BUTTON(newtonmenu->newton_button), GTK_RELIEF_NONE);
    newtonmenu->newton_button_box = gtk_box_new(orientation, 2);
    gtk_container_set_border_width(GTK_CONTAINER(newtonmenu->newton_button_box), 2);
    gtk_container_add(GTK_CONTAINER(newtonmenu->newton_button), newtonmenu->newton_button_box);
    newtonmenu->newton_icon_image = gtk_image_new();
    gtk_box_pack_start(GTK_BOX(newtonmenu->newton_button_box), newtonmenu->newton_icon_image, FALSE, FALSE, 0);
    newtonmenu->newton_label_widget = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(newtonmenu->newton_button_box), newtonmenu->newton_label_widget, TRUE, TRUE, 0);
    gtk_widget_show_all(GTK_WIDGET(newtonmenu->newton_button));
    gtk_box_pack_start(GTK_BOX(newtonmenu->main_box), GTK_WIDGET(newtonmenu->newton_button), FALSE, FALSE, 0);

    newtonmenu->app_menu_bar_container = gtk_box_new(orientation, 0);
    gtk_box_pack_start(GTK_BOX(newtonmenu->main_box), newtonmenu->app_menu_bar_container, TRUE, TRUE, 0);
    gtk_widget_show(newtonmenu->app_menu_bar_container); 
    gtk_widget_hide(newtonmenu->app_menu_bar_container); 

    newtonmenu->app_name_button = NULL;
    newtonmenu->app_name_button_label = NULL;
    newtonmenu->dynamic_app_menu_buttons = NULL;
    newtonmenu->currently_open_button = NULL;
    newtonmenu->app_dbusmenu_client = NULL;
    newtonmenu->app_dbusmenu_client_root_changed_id = 0;
    newtonmenu->app_dbusmenu_client_layout_updated_id = 0;
    newtonmenu->dbus_name_owner_id = 0;

    newtonmenu->static_newton_menu = newtonmenu_create_static_newton_menu(newtonmenu);
    gtk_menu_button_set_popup(newtonmenu->newton_button, newtonmenu->static_newton_menu);
    newtonmenu_update_display(newtonmenu);
    
    g_signal_connect(newtonmenu->newton_button, "toggled", G_CALLBACK(newtonmenu_menu_button_toggled), newtonmenu);
    g_signal_connect(newtonmenu->newton_button, "enter-notify-event", G_CALLBACK(on_menu_button_enter), newtonmenu);

    newtonmenu_init_dbus_and_wnck(newtonmenu);
    return newtonmenu;
}

static void newtonmenu_free(XfcePanelPlugin *plugin, newtonmenuPlugin *newtonmenu) {
    GtkWidget *dialog = g_object_get_data(G_OBJECT(plugin), "dialog");
    if (G_UNLIKELY(dialog != NULL)) gtk_widget_destroy(dialog);

    newtonmenu_clear_dynamic_app_menus(newtonmenu); 

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

    g_free(newtonmenu->icon_name_prop);
    g_free(newtonmenu->label_text_prop);
    g_free(newtonmenu->global_menu_title_prop);
    if (newtonmenu->static_newton_menu) gtk_widget_destroy(newtonmenu->static_newton_menu);
    newtonmenu_teardown_dbus_and_wnck(newtonmenu);
    g_slice_free(newtonmenuPlugin, newtonmenu);
}

static void newtonmenu_orientation_changed(XfcePanelPlugin *plugin, GtkOrientation orientation, newtonmenuPlugin *newtonmenu) {
    g_return_if_fail(newtonmenu != NULL);
    gtk_orientable_set_orientation(GTK_ORIENTABLE(newtonmenu->main_box), orientation);
    gtk_orientable_set_orientation(GTK_ORIENTABLE(newtonmenu->newton_button_box), orientation);
    gtk_orientable_set_orientation(GTK_ORIENTABLE(newtonmenu->app_menu_bar_container), orientation);
    newtonmenu_update_display(newtonmenu);
}

static gboolean newtonmenu_size_changed(XfcePanelPlugin *plugin, gint size, newtonmenuPlugin *newtonmenu) {
    GtkOrientation orientation = xfce_panel_plugin_get_orientation(plugin);
    g_return_val_if_fail(newtonmenu != NULL, TRUE);
    if (orientation == GTK_ORIENTATION_HORIZONTAL) gtk_widget_set_size_request(GTK_WIDGET(plugin), -1, size);
    else gtk_widget_set_size_request(GTK_WIDGET(plugin), size, -1);
    newtonmenu_update_display(newtonmenu);
    return TRUE;
}

static void newtonmenu_init_dbus_and_wnck(newtonmenuPlugin *newtonmenu) {
    GError *error = NULL;
    const gchar *current_gtk_modules = g_getenv("GTK_MODULES");
    if (!current_gtk_modules || !g_strrstr(current_gtk_modules, "appmenu-gtk-module")) {
        gchar *new_modules = current_gtk_modules ? 
            g_strdup_printf("%s:appmenu-gtk-module", current_gtk_modules) : 
            g_strdup("appmenu-gtk-module");
        g_setenv("GTK_MODULES", new_modules, TRUE);
        g_free(new_modules);
        g_debug("NM_DEBUG: GTK_MODULES set to: %s", g_getenv("GTK_MODULES"));
    }
    
    newtonmenu->wnck_screen = wnck_screen_get_default();
    if (newtonmenu->wnck_screen) {
         newtonmenu->wnck_active_window_changed_handler_id = g_signal_connect(
            newtonmenu->wnck_screen, "active-window-changed", G_CALLBACK(on_wnck_active_window_changed), newtonmenu);
    } else { g_warning("Failed to get WnckScreen."); }
    newtonmenu->dbus_session_bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (!newtonmenu->dbus_session_bus) {
        g_warning("Failed to connect to D-Bus session bus: %s", error ? error->message : "Unknown error");
        g_clear_error(&error); return;
    }

    newtonmenu->dbus_name_owner_id = g_bus_own_name(
        G_BUS_TYPE_SESSION, 
        NEWTONMENU_DBUS_WELL_KNOWN_NAME, 
        G_BUS_NAME_OWNER_FLAGS_NONE,
        NULL, NULL, NULL, NULL, NULL); 
    if (newtonmenu->dbus_name_owner_id == 0) g_warning("NM_WARNING: Failed to own D-Bus name: %s", NEWTONMENU_DBUS_WELL_KNOWN_NAME);
    else g_debug("NM_DEBUG: Attempted to own D-Bus name: %s (owner_id: %u)", NEWTONMENU_DBUS_WELL_KNOWN_NAME, newtonmenu->dbus_name_owner_id);
    
    newtonmenu->appmenu_registrar_proxy = g_dbus_proxy_new_sync(
        newtonmenu->dbus_session_bus, G_DBUS_PROXY_FLAGS_NONE, NULL, APPMENU_REGISTRAR_BUS_NAME,
        APPMENU_REGISTRAR_OBJECT_PATH, APPMENU_REGISTRAR_INTERFACE_NAME, NULL, &error);
    if (!newtonmenu->appmenu_registrar_proxy) {
        g_warning("Failed to create AppMenu Registrar D-Bus proxy: %s", error ? error->message : "Unknown error");
        g_clear_error(&error); return;
    }
    newtonmenu->appmenu_registrar_registered_handler_id = g_signal_connect(
        newtonmenu->appmenu_registrar_proxy, "g-signal", G_CALLBACK(on_appmenu_registrar_signal), newtonmenu);
    on_wnck_active_window_changed(newtonmenu->wnck_screen, NULL, newtonmenu);
}

static void newtonmenu_teardown_dbus_and_wnck(newtonmenuPlugin *newtonmenu) {
    if (newtonmenu->dbus_name_owner_id > 0) {
        g_bus_unown_name(newtonmenu->dbus_name_owner_id);
        newtonmenu->dbus_name_owner_id = 0;
    }
    if (newtonmenu->wnck_screen && newtonmenu->wnck_active_window_changed_handler_id > 0) {
        if (g_signal_handler_is_connected(newtonmenu->wnck_screen, newtonmenu->wnck_active_window_changed_handler_id)) {
             g_signal_handler_disconnect(newtonmenu->wnck_screen, newtonmenu->wnck_active_window_changed_handler_id);
        }
    }
    newtonmenu->wnck_active_window_changed_handler_id = 0;
    if (newtonmenu->active_wnck_window) { g_object_unref(newtonmenu->active_wnck_window); newtonmenu->active_wnck_window = NULL; }
    if (newtonmenu->appmenu_registrar_proxy && newtonmenu->appmenu_registrar_registered_handler_id > 0) {
         if (g_signal_handler_is_connected(newtonmenu->appmenu_registrar_proxy, newtonmenu->appmenu_registrar_registered_handler_id)) {
            g_signal_handler_disconnect(newtonmenu->appmenu_registrar_proxy, newtonmenu->appmenu_registrar_registered_handler_id);
        }
    }
    newtonmenu->appmenu_registrar_registered_handler_id = 0;
    if (newtonmenu->appmenu_registrar_proxy) { g_object_unref(newtonmenu->appmenu_registrar_proxy); newtonmenu->appmenu_registrar_proxy = NULL; }
    if (newtonmenu->dbus_session_bus) { g_object_unref(newtonmenu->dbus_session_bus); newtonmenu->dbus_session_bus = NULL; }
}

static void on_wnck_active_window_changed(WnckScreen *screen, WnckWindow *prev_active_window, newtonmenuPlugin *newtonmenu) {
    WnckWindow *current_active_window = NULL;
    g_debug("NM_DEBUG: on_wnck_active_window_changed CALLED");
    g_return_if_fail(newtonmenu != NULL);
    if (screen) current_active_window = wnck_screen_get_active_window(screen);
    if (newtonmenu->active_wnck_window) { 
        g_object_unref(newtonmenu->active_wnck_window); 
        newtonmenu->active_wnck_window = NULL; 
    }
    
    if (newtonmenu->app_dbusmenu_client) { // Disconnect signals and unref old client *before* clearing UI
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
    newtonmenu_clear_dynamic_app_menus(newtonmenu); // Now clear UI

    if (current_active_window) {
        newtonmenu->active_wnck_window = g_object_ref(current_active_window);
        g_debug("NM_DEBUG: Active window changed to '%s' (XID: %lu)", wnck_window_get_name(current_active_window), (gulong)wnck_window_get_xid(current_active_window));
        newtonmenu_fetch_app_menu_enhanced(newtonmenu, newtonmenu->active_wnck_window);
    } else {
        g_debug("NM_DEBUG: No active window.");
    }
}

static void on_appmenu_registrar_signal(GDBusProxy *proxy, const gchar *sender_name, const gchar *signal_name, GVariant *parameters, gpointer user_data) {
    newtonmenuPlugin *newtonmenu = (newtonmenuPlugin*)user_data;
    guint32 window_id;
    g_debug("NM_DEBUG: on_appmenu_registrar_signal received: %s from %s", signal_name, sender_name);
    if (g_strcmp0(signal_name, "WindowRegistered") == 0) {
        gchar *service_name, *object_path;
        g_variant_get(parameters, "(uso)", &window_id, &service_name, &object_path);
        g_debug("NM_DEBUG: WindowRegistered: XID=%u, service=%s, path=%s", window_id, service_name ? service_name : "(null)", object_path ? object_path : "(null)");
        if (newtonmenu->active_wnck_window && wnck_window_get_xid(newtonmenu->active_wnck_window) == window_id) {
            g_debug("NM_DEBUG: AppMenu registered for current active window %lu. Re-fetching.", (gulong)window_id);
            newtonmenu_fetch_app_menu_enhanced(newtonmenu, newtonmenu->active_wnck_window);
        }
        g_free(service_name); g_free(object_path);
    } else if (g_strcmp0(signal_name, "WindowUnregistered") == 0) {
        g_variant_get(parameters, "(u)", &window_id);
        g_debug("NM_DEBUG: WindowUnregistered: XID=%u", window_id);
        if (newtonmenu->active_wnck_window && wnck_window_get_xid(newtonmenu->active_wnck_window) == window_id) {
            g_debug("NM_DEBUG: AppMenu unregistered for current active window %lu", (gulong)window_id);
            newtonmenu_clear_dynamic_app_menus(newtonmenu);
            if (newtonmenu->app_dbusmenu_client) {
                g_object_unref(newtonmenu->app_dbusmenu_client);
                newtonmenu->app_dbusmenu_client = NULL;
            }
        }
    }
}

static GtkWidget* newtonmenu_create_static_newton_menu(newtonmenuPlugin *newtonmenu) {
    GtkWidget *menu = gtk_menu_new(); GtkWidget *menu_item;
    const gchar* username = g_get_user_name(); gchar* logout_label;
    menu_item = gtk_menu_item_new_with_label(_("About This PC")); g_signal_connect(menu_item, "activate", G_CALLBACK(on_about_this_pc_activate), newtonmenu); gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    menu_item = gtk_menu_item_new_with_label(_("System Settings...")); g_signal_connect(menu_item, "activate", G_CALLBACK(on_system_settings_activate), newtonmenu); gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    menu_item = gtk_menu_item_new_with_label(_("Run Command...")); g_signal_connect(menu_item, "activate", G_CALLBACK(on_run_command_activate), newtonmenu); gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    menu_item = gtk_menu_item_new_with_label(_("Force Quit...")); g_signal_connect(menu_item, "activate", G_CALLBACK(on_force_quit_activate), newtonmenu); gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    menu_item = gtk_menu_item_new_with_label(_("Sleep")); g_signal_connect(menu_item, "activate", G_CALLBACK(on_sleep_activate), newtonmenu); gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    menu_item = gtk_menu_item_new_with_label(_("Restart...")); g_signal_connect(menu_item, "activate", G_CALLBACK(on_restart_activate), newtonmenu); gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    menu_item = gtk_menu_item_new_with_label(_("Shut Down...")); g_signal_connect(menu_item, "activate", G_CALLBACK(on_shutdown_activate), newtonmenu); gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    menu_item = gtk_menu_item_new_with_label(_("Lock Screen")); g_signal_connect(menu_item, "activate", G_CALLBACK(on_lock_screen_activate), newtonmenu); gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    if (username) logout_label = g_strdup_printf(_("Log Out %s..."), username); else logout_label = g_strdup(_("Log Out..."));
    menu_item = gtk_menu_item_new_with_label(logout_label); g_signal_connect(menu_item, "activate", G_CALLBACK(on_log_out_activate), newtonmenu); gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item); g_free(logout_label);
    gtk_widget_show_all(menu); return menu;
}

static void newtonmenu_construct(XfcePanelPlugin *plugin) {
    newtonmenuPlugin *newtonmenu;
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
    newtonmenu = newtonmenu_new(plugin);
    g_return_if_fail(newtonmenu != NULL);
    gtk_container_add(GTK_CONTAINER(plugin), newtonmenu->main_box);
    g_signal_connect(G_OBJECT(plugin), "free-data", G_CALLBACK(newtonmenu_free), newtonmenu);
    g_signal_connect(G_OBJECT(plugin), "save", G_CALLBACK(newtonmenu_save), newtonmenu);
    g_signal_connect(G_OBJECT(plugin), "size-changed", G_CALLBACK(newtonmenu_size_changed), newtonmenu);
    g_signal_connect(G_OBJECT(plugin), "orientation-changed", G_CALLBACK(newtonmenu_orientation_changed), newtonmenu);
    xfce_panel_plugin_menu_show_configure(plugin);
    g_signal_connect(G_OBJECT(plugin), "configure-plugin", G_CALLBACK(newtonmenu_configure), newtonmenu);
    xfce_panel_plugin_menu_show_about(plugin);
    g_signal_connect(G_OBJECT(plugin), "about", G_CALLBACK(newtonmenu_about), plugin);
}

static void on_about_this_pc_activate(GtkMenuItem *menuitem, gpointer user_data) { execute_command("xfce4-about"); }
static void on_system_settings_activate(GtkMenuItem *menuitem, gpointer user_data) { execute_command("xfce4-settings-manager"); }
static void on_run_command_activate(GtkMenuItem *menuitem, gpointer user_data) { execute_command("xfce4-appfinder --collapsed"); }
static void on_force_quit_activate(GtkMenuItem *menuitem, gpointer user_data) { 
    newtonmenuPlugin *nm = (newtonmenuPlugin*)user_data;
    newtonmenu_show_force_quit_applications_dialog(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(nm->plugin))), nm);
}
static void on_sleep_activate(GtkMenuItem *menuitem, gpointer user_data) { execute_command("xfce4-session-logout --suspend"); }
static void on_restart_activate(GtkMenuItem *menuitem, gpointer user_data) {
    newtonmenuPlugin *nm = (newtonmenuPlugin*)user_data;
    if (nm->confirm_restart_prop) newtonmenu_show_generic_confirmation(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(nm->plugin))), _("restart"), _("Restart"), "xfce4-session-logout --reboot");
    else execute_command("xfce4-session-logout --reboot");
}
static void on_shutdown_activate(GtkMenuItem *menuitem, gpointer user_data) {
    newtonmenuPlugin *nm = (newtonmenuPlugin*)user_data;
    if (nm->confirm_shutdown_prop) newtonmenu_show_generic_confirmation(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(nm->plugin))), _("shut down"), _("Shut Down"), "xfce4-session-logout --halt");
    else execute_command("xfce4-session-logout --halt");
}
static void on_lock_screen_activate(GtkMenuItem *menuitem, gpointer user_data) { execute_command("xflock4"); }
static void on_log_out_activate(GtkMenuItem *menuitem, gpointer user_data) {
    newtonmenuPlugin *nm = (newtonmenuPlugin*)user_data;
    const gchar *un = g_get_user_name();
    gchar *an = un ? g_strdup_printf(_("Log Out %s..."), un) : g_strdup(_("Log Out..."));
    if (nm->confirm_logout_prop) newtonmenu_show_generic_confirmation(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(nm->plugin))), an, _("Log Out"), "xfce4-session-logout --logout");
    else execute_command("xfce4-session-logout --logout");
    g_free(an);
}