/*
 * Copyright (C) 2025 Adam
 * Xfce4 Newton Menu Plugin - Configuration Dialogs
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_XFCE_REVISION_H
#include "xfce-revision.h"
#endif

#include <string.h>
#include <gtk/gtk.h>

#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/libxfce4panel.h>
#include <exo/exo.h>

#include "newtonmenu.h"
#include "newtonmenu-dialogs.h"

#define PLUGIN_WEBSITE "https://gitlab.xfce.org/panel-plugins/xfce4-newtonmenu-plugin"

// Forward declarations for dialog-specific callbacks
static void on_display_icon_checkbutton_toggled(GtkToggleButton *togglebutton, gpointer user_data);
static void on_hide_app_name_checkbutton_toggled(GtkToggleButton *togglebutton, gpointer user_data);
static void on_icon_choose_button_clicked(GtkButton *button, gpointer user_data);
static void dialog_save_settings_and_update(GtkDialog *dialog, newtonmenuPlugin *newtonmenu, GtkBuilder *builder);
static void newtonmenu_configure_response_cb(GtkWidget *dialog_widget, gint response, newtonmenuPlugin *newtonmenu);
static void generic_action_dialog_response_cb(GtkDialog *dialog, gint response_id, gpointer user_data);

static void on_display_icon_checkbutton_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    GtkBuilder *builder = GTK_BUILDER(user_data);
    gboolean display_icon = gtk_toggle_button_get_active(togglebutton);
    GtkWidget *icon_settings_box, *label_settings_box;

    g_return_if_fail(builder != NULL);

    icon_settings_box = GTK_WIDGET(gtk_builder_get_object(builder, "icon_settings_box"));
    label_settings_box = GTK_WIDGET(gtk_builder_get_object(builder, "label_settings_box"));

    if (icon_settings_box) gtk_widget_set_visible(icon_settings_box, display_icon);
    if (label_settings_box) gtk_widget_set_visible(label_settings_box, !display_icon);
}

static void on_hide_app_name_checkbutton_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    GtkBuilder *builder = GTK_BUILDER(user_data);
    gboolean hide_app_name = gtk_toggle_button_get_active(togglebutton);
    GtkWidget *global_menu_title_box;

    g_return_if_fail(builder != NULL);

    global_menu_title_box = GTK_WIDGET(gtk_builder_get_object(builder, "global_menu_title_box"));

    if (global_menu_title_box) gtk_widget_set_visible(global_menu_title_box, hide_app_name);
}

static void on_icon_choose_button_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *parent_dialog = GTK_WIDGET(user_data);
    GtkBuilder *builder = GTK_BUILDER(g_object_get_data(G_OBJECT(parent_dialog), "builder"));
    GtkWidget *icon_chooser_dialog;
    gchar *selected_icon_name = NULL;
    GtkWindow *parent_window = GTK_WINDOW(parent_dialog);
    GtkWidget *icon_name_entry_widget;

    g_return_if_fail(builder != NULL);
    icon_name_entry_widget = GTK_WIDGET(gtk_builder_get_object(builder, "icon_name_entry"));
    g_return_if_fail(GTK_IS_ENTRY(icon_name_entry_widget));

    icon_chooser_dialog = exo_icon_chooser_dialog_new(
        _("Choose an Icon"),
        parent_window,
        _("_Cancel"), GTK_RESPONSE_CANCEL,
        _("_OK"), GTK_RESPONSE_ACCEPT,
        NULL);
    
    gtk_window_set_modal(GTK_WINDOW(icon_chooser_dialog), TRUE);
    gtk_dialog_set_default_response(GTK_DIALOG(icon_chooser_dialog), GTK_RESPONSE_ACCEPT);

    const gchar *current_icon = gtk_entry_get_text(GTK_ENTRY(icon_name_entry_widget));
    if (current_icon && *current_icon) {
        exo_icon_chooser_dialog_set_icon(EXO_ICON_CHOOSER_DIALOG(icon_chooser_dialog), current_icon);
    }

    if (gtk_dialog_run(GTK_DIALOG(icon_chooser_dialog)) == GTK_RESPONSE_ACCEPT) {
        selected_icon_name = exo_icon_chooser_dialog_get_icon(EXO_ICON_CHOOSER_DIALOG(icon_chooser_dialog));
        if (selected_icon_name) {
            gtk_entry_set_text(GTK_ENTRY(icon_name_entry_widget), selected_icon_name);
            g_free(selected_icon_name);
        }
    }
    gtk_widget_destroy(icon_chooser_dialog);
}

static void dialog_save_settings_and_update(GtkDialog *dialog, newtonmenuPlugin *newtonmenu, GtkBuilder *builder)
{
    GtkWidget *widget;

    g_return_if_fail(newtonmenu != NULL);
    g_return_if_fail(builder != NULL);

    // Save button appearance settings
    widget = GTK_WIDGET(gtk_builder_get_object(builder, "display_icon_checkbutton"));
    if (GTK_IS_TOGGLE_BUTTON(widget))
        newtonmenu->display_icon_prop = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    widget = GTK_WIDGET(gtk_builder_get_object(builder, "icon_name_entry"));
    if (GTK_IS_ENTRY(widget)) {
        g_free(newtonmenu->icon_name_prop);
        newtonmenu->icon_name_prop = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
    }

    widget = GTK_WIDGET(gtk_builder_get_object(builder, "label_text_entry"));
    if (GTK_IS_ENTRY(widget)) {
        g_free(newtonmenu->label_text_prop);
        newtonmenu->label_text_prop = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
    }

    // Save global menu settings
    widget = GTK_WIDGET(gtk_builder_get_object(builder, "hide_app_name_checkbutton"));
    if (GTK_IS_TOGGLE_BUTTON(widget))
        newtonmenu->hide_application_name_prop = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    widget = GTK_WIDGET(gtk_builder_get_object(builder, "global_menu_title_entry"));
    if (GTK_IS_ENTRY(widget)) {
        g_free(newtonmenu->global_menu_title_prop);
        newtonmenu->global_menu_title_prop = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
    }
    
    // Save confirmation settings
    widget = GTK_WIDGET(gtk_builder_get_object(builder, "confirm_logout_checkbutton"));
    if (GTK_IS_TOGGLE_BUTTON(widget))
        newtonmenu->confirm_logout_prop = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    widget = GTK_WIDGET(gtk_builder_get_object(builder, "confirm_restart_checkbutton"));
    if (GTK_IS_TOGGLE_BUTTON(widget))
        newtonmenu->confirm_restart_prop = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
        
    widget = GTK_WIDGET(gtk_builder_get_object(builder, "confirm_shutdown_checkbutton"));
    if (GTK_IS_TOGGLE_BUTTON(widget))
        newtonmenu->confirm_shutdown_prop = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    widget = GTK_WIDGET(gtk_builder_get_object(builder, "confirm_force_quit_checkbutton"));
    if (GTK_IS_TOGGLE_BUTTON(widget))
        newtonmenu->confirm_force_quit_prop = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    
    // Save and update display
    newtonmenu_save(newtonmenu->plugin, newtonmenu);
    newtonmenu_update_display(newtonmenu);
    // REMOVED: newtonmenu_update_combined_menu(newtonmenu); - this function doesn't exist
}

static void newtonmenu_configure_response_cb(GtkWidget *dialog_widget, gint response, newtonmenuPlugin *newtonmenu)
{
    GtkBuilder *builder = GTK_BUILDER(g_object_get_data(G_OBJECT(dialog_widget), "builder"));

    if (response == GTK_RESPONSE_HELP) {
        gboolean result;
        result = g_spawn_command_line_async("exo-open --launch WebBrowser " PLUGIN_WEBSITE, NULL);
        if (G_UNLIKELY(result == FALSE))
            g_warning(_("Unable to open the following url: %s"), PLUGIN_WEBSITE);
        return; 
    }
  
    if (response == GTK_RESPONSE_CLOSE || response == GTK_RESPONSE_DELETE_EVENT || response == GTK_RESPONSE_OK) {
        if (builder && newtonmenu) {
            dialog_save_settings_and_update(GTK_DIALOG(dialog_widget), newtonmenu, builder);
        }
    }

    if (newtonmenu && newtonmenu->plugin) {
        g_object_set_data(G_OBJECT(newtonmenu->plugin), "dialog", NULL);
        xfce_panel_plugin_unblock_menu(newtonmenu->plugin);
    }
    if (builder) {
        g_object_unref(builder);
        g_object_set_data(G_OBJECT(dialog_widget), "builder", NULL);
    }
    gtk_widget_destroy(dialog_widget);
}

void newtonmenu_configure(XfcePanelPlugin *plugin, newtonmenuPlugin *newtonmenu)
{
    GtkBuilder *builder;
    GObject *dialog_obj;
    GtkWidget *dialog_widget;
    GtkWidget *widget;

    g_return_if_fail(plugin != NULL);
    g_return_if_fail(newtonmenu != NULL);

    if (g_object_get_data(G_OBJECT(plugin), "dialog") != NULL) {
        gtk_window_present(GTK_WINDOW(g_object_get_data(G_OBJECT(plugin), "dialog")));
        return;
    }

    xfce_panel_plugin_block_menu(plugin);

    const gchar *ui_resource_path = "/org/xfce/panel/plugins/newtonmenu/newtonmenu-dialog.ui";
    builder = gtk_builder_new_from_resource(ui_resource_path);

    if (G_UNLIKELY(builder == NULL)) {
        g_warning("Failed to load UI for newtonmenu plugin configuration from resource: %s", ui_resource_path);
        xfce_panel_plugin_unblock_menu(plugin);
        return;
    }

    dialog_obj = gtk_builder_get_object(builder, "newtonmenu_config_dialog");
    if (G_UNLIKELY(dialog_obj == NULL || !GTK_IS_DIALOG(dialog_obj))) {
        g_warning("UI loaded, but toplevel widget ('newtonmenu_config_dialog') is not a GtkDialog or has wrong ID.");
        g_object_unref(builder);
        xfce_panel_plugin_unblock_menu(plugin);
        return;
    }
    
    dialog_widget = GTK_WIDGET(dialog_obj);
    gtk_window_set_transient_for(GTK_WINDOW(dialog_widget), GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(plugin))));
    gtk_window_set_position(GTK_WINDOW(dialog_widget), GTK_WIN_POS_CENTER_ON_PARENT);

    g_object_set_data(G_OBJECT(dialog_widget), "builder", builder);
    g_object_set_data(G_OBJECT(dialog_widget), "plugin_data", newtonmenu);

    // Setup button appearance controls
    widget = GTK_WIDGET(gtk_builder_get_object(builder, "display_icon_checkbutton"));
    if (GTK_IS_TOGGLE_BUTTON(widget)) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), newtonmenu->display_icon_prop);
        on_display_icon_checkbutton_toggled(GTK_TOGGLE_BUTTON(widget), builder);
        g_signal_connect(widget, "toggled", G_CALLBACK(on_display_icon_checkbutton_toggled), builder);
    } else {
        g_warning("Widget 'display_icon_checkbutton' not found or not a GtkToggleButton.");
    }

    widget = GTK_WIDGET(gtk_builder_get_object(builder, "icon_name_entry"));
    if (GTK_IS_ENTRY(widget)) {
        gtk_entry_set_text(GTK_ENTRY(widget), newtonmenu->icon_name_prop ? newtonmenu->icon_name_prop : "");
    } else {
        g_warning("Widget 'icon_name_entry' not found or not a GtkEntry.");
    }

    widget = GTK_WIDGET(gtk_builder_get_object(builder, "label_text_entry"));
    if (GTK_IS_ENTRY(widget)) {
        gtk_entry_set_text(GTK_ENTRY(widget), newtonmenu->label_text_prop ? newtonmenu->label_text_prop : "");
    } else {
        g_warning("Widget 'label_text_entry' not found or not a GtkEntry.");
    }

    widget = GTK_WIDGET(gtk_builder_get_object(builder, "icon_choose_button"));
    if (GTK_IS_BUTTON(widget)) {
        g_signal_connect(widget, "clicked", G_CALLBACK(on_icon_choose_button_clicked), dialog_widget);
    } else {
        g_warning("Widget 'icon_choose_button' not found or not a GtkButton.");
    }

    // Setup global menu appearance controls
    widget = GTK_WIDGET(gtk_builder_get_object(builder, "hide_app_name_checkbutton"));
    if (GTK_IS_TOGGLE_BUTTON(widget)) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), newtonmenu->hide_application_name_prop);
        on_hide_app_name_checkbutton_toggled(GTK_TOGGLE_BUTTON(widget), builder);
        g_signal_connect(widget, "toggled", G_CALLBACK(on_hide_app_name_checkbutton_toggled), builder);
    } else {
        g_warning("Widget 'hide_app_name_checkbutton' not found or not a GtkToggleButton.");
    }

    widget = GTK_WIDGET(gtk_builder_get_object(builder, "global_menu_title_entry"));
    if (GTK_IS_ENTRY(widget)) {
        gtk_entry_set_text(GTK_ENTRY(widget), newtonmenu->global_menu_title_prop ? newtonmenu->global_menu_title_prop : "");
    } else {
        g_warning("Widget 'global_menu_title_entry' not found or not a GtkEntry.");
    }

    // Setup confirmation controls
    widget = GTK_WIDGET(gtk_builder_get_object(builder, "confirm_logout_checkbutton"));
    if (GTK_IS_TOGGLE_BUTTON(widget))
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), newtonmenu->confirm_logout_prop);

    widget = GTK_WIDGET(gtk_builder_get_object(builder, "confirm_restart_checkbutton"));
    if (GTK_IS_TOGGLE_BUTTON(widget))
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), newtonmenu->confirm_restart_prop);
        
    widget = GTK_WIDGET(gtk_builder_get_object(builder, "confirm_shutdown_checkbutton"));
    if (GTK_IS_TOGGLE_BUTTON(widget))
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), newtonmenu->confirm_shutdown_prop);

    widget = GTK_WIDGET(gtk_builder_get_object(builder, "confirm_force_quit_checkbutton"));
    if (GTK_IS_TOGGLE_BUTTON(widget))
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), newtonmenu->confirm_force_quit_prop);
    
    g_object_set_data(G_OBJECT(plugin), "dialog", dialog_widget);
    g_signal_connect(G_OBJECT(dialog_widget), "response",
                     G_CALLBACK(newtonmenu_configure_response_cb), newtonmenu);

    gtk_widget_show_all(dialog_widget);
}

void newtonmenu_about(XfcePanelPlugin *plugin)
{
    const gchar *auth[] = {
        "Adam",
        "AI Assistant",
        NULL
    };

    gtk_show_about_dialog(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(plugin))),
                         "logo-icon-name", "xfce4-newtonmenu-plugin",
                         "license-type",   GTK_LICENSE_GPL_2_0,
                         "version",        PACKAGE_VERSION,
                         "program-name",   PACKAGE_NAME,
                         "comments",       _("A macOS-like application and session menu button with global menu integration."),
                         "website",        PLUGIN_WEBSITE,
                         "copyright",      _("Copyright © 2024-2025 Adam"),
                         "authors",        auth,
                         NULL);
}

static void generic_action_dialog_response_cb(GtkDialog *dialog, gint response_id, gpointer command_to_run_gpointer)
{
    gchar *command_to_run = (gchar*)command_to_run_gpointer;

    if (response_id == GTK_RESPONSE_YES) {
        if (command_to_run && *command_to_run) {
            GError *error = NULL;
            if (!g_spawn_command_line_async(command_to_run, &error)) {
                g_warning("Failed to execute command '%s': %s", command_to_run, error ? error->message : "Unknown error");
                if (error) g_error_free(error);
            }
        }
    }
    if (command_to_run) g_free(command_to_run); 
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

void newtonmenu_show_generic_confirmation(GtkWindow *parent, 
                                          const gchar *action_name_translated, 
                                          const gchar *action_verb_translated, 
                                          const gchar *command_to_run)
{
    GtkWidget *dialog;
    gchar *primary_text;
    gchar *secondary_text = NULL; 

    primary_text = g_strdup_printf(_("Are you sure you want to %s?"), action_name_translated);

    dialog = gtk_message_dialog_new(parent,
                                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_QUESTION,
                                   GTK_BUTTONS_NONE,
                                   "%s", 
                                   primary_text);
    g_free(primary_text);

    if (secondary_text) {
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", secondary_text);
        g_free(secondary_text);
    }
    
    gchar *action_button_label = g_strdup_printf("_%s", action_verb_translated);

    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                          _("_Cancel"), GTK_RESPONSE_CANCEL,
                          action_button_label, GTK_RESPONSE_YES,
                          NULL);
    g_free(action_button_label);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
    
    g_signal_connect(dialog, "response", G_CALLBACK(generic_action_dialog_response_cb), g_strdup(command_to_run));
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ALWAYS); 
    gtk_widget_show_all(dialog);
}