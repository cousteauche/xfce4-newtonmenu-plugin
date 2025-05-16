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

#include "newtonbutton.h"
#include "newtonbutton-dialogs.h" // For newtonbutton_show_force_quit_confirmation

#define DEFAULT_DISPLAY_ICON TRUE
#define DEFAULT_ICON_NAME "/usr/share/icons/hicolor/scalable/apps/xfce4-newtonbutton-plugin.svg"
#define DEFAULT_LABEL_TEXT N_("Menu")

// Default confirmation states
#define DEFAULT_CONFIRM_LOGOUT FALSE
#define DEFAULT_CONFIRM_RESTART TRUE
#define DEFAULT_CONFIRM_SHUTDOWN TRUE
#define DEFAULT_CONFIRM_FORCE_QUIT FALSE // Changed to FALSE

static void newtonbutton_construct (XfcePanelPlugin *plugin);
static void newtonbutton_read (NewtonbuttonPlugin *newtonbutton);
static void newtonbutton_free (XfcePanelPlugin *plugin, NewtonbuttonPlugin *newtonbutton);
static void newtonbutton_orientation_changed (XfcePanelPlugin *plugin, GtkOrientation orientation, NewtonbuttonPlugin *newtonbutton);
static gboolean newtonbutton_size_changed (XfcePanelPlugin *plugin, gint size, NewtonbuttonPlugin *newtonbutton);
static NewtonbuttonPlugin* newtonbutton_new (XfcePanelPlugin *plugin);
static void newtonbutton_popup_menu_on_toggle (GtkToggleButton *toggle_button, NewtonbuttonPlugin *newtonbutton);
static void newtonbutton_menu_deactivate_cb (GtkMenu *menu, NewtonbuttonPlugin *newtonbutton);

static void on_about_this_pc_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_system_settings_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_app_store_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_force_quit_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_sleep_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_restart_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_shutdown_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_lock_screen_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_log_out_activate(GtkMenuItem *menuitem, gpointer user_data);

static void execute_command(const gchar *command);


XFCE_PANEL_PLUGIN_REGISTER (newtonbutton_construct);


static void execute_command(const gchar *command)
{
    GError *error = NULL;
    if (!g_spawn_command_line_async (command, &error))
    {
        g_warning ("Failed to execute command '%s': %s", command, error ? error->message : "Unknown error");
        if (error) g_error_free (error);
    }
}

void
newtonbutton_save (XfcePanelPlugin *plugin,
             NewtonbuttonPlugin    *newtonbutton)
{
  XfceRc *rc;
  gchar  *file;

  g_return_if_fail(newtonbutton != NULL);
  g_return_if_fail(XFCE_IS_PANEL_PLUGIN(plugin));

  file = xfce_panel_plugin_save_location (plugin, TRUE);

  if (G_UNLIKELY (file == NULL))
    {
       return;
    }

  rc = xfce_rc_simple_open (file, FALSE);
  g_free (file);

  if (G_LIKELY (rc != NULL))
    {
      xfce_rc_write_bool_entry (rc, "DisplayIcon", newtonbutton->display_icon_prop);

      if (newtonbutton->icon_name_prop)
        xfce_rc_write_entry (rc, "IconName", newtonbutton->icon_name_prop);
      else
        xfce_rc_write_entry (rc, "IconName", "");

      if (newtonbutton->label_text_prop)
        xfce_rc_write_entry (rc, "LabelText", newtonbutton->label_text_prop);
      else
        xfce_rc_write_entry (rc, "LabelText", "");
      
      xfce_rc_write_bool_entry(rc, "ConfirmLogout", newtonbutton->confirm_logout_prop);
      xfce_rc_write_bool_entry(rc, "ConfirmRestart", newtonbutton->confirm_restart_prop);
      xfce_rc_write_bool_entry(rc, "ConfirmShutdown", newtonbutton->confirm_shutdown_prop);
      xfce_rc_write_bool_entry(rc, "ConfirmForceQuit", newtonbutton->confirm_force_quit_prop);

      xfce_rc_close (rc);
    }
}

static void
newtonbutton_read (NewtonbuttonPlugin *newtonbutton)
{
  XfceRc      *rc;
  gchar       *file;
  const gchar *value;

  g_return_if_fail(newtonbutton != NULL);
  g_return_if_fail(newtonbutton->plugin != NULL);

  file = xfce_panel_plugin_save_location (newtonbutton->plugin, TRUE);

  if (G_LIKELY (file != NULL))
    {
      rc = xfce_rc_simple_open (file, TRUE);
      g_free (file);

      if (G_LIKELY (rc != NULL))
        {
          newtonbutton->display_icon_prop = xfce_rc_read_bool_entry (rc, "DisplayIcon", DEFAULT_DISPLAY_ICON);

          value = xfce_rc_read_entry (rc, "IconName", DEFAULT_ICON_NAME);
          g_free(newtonbutton->icon_name_prop);
          newtonbutton->icon_name_prop = g_strdup (value);

          value = xfce_rc_read_entry (rc, "LabelText", _(DEFAULT_LABEL_TEXT));
          g_free(newtonbutton->label_text_prop);
          newtonbutton->label_text_prop = g_strdup (value);

          newtonbutton->confirm_logout_prop = xfce_rc_read_bool_entry(rc, "ConfirmLogout", DEFAULT_CONFIRM_LOGOUT);
          newtonbutton->confirm_restart_prop = xfce_rc_read_bool_entry(rc, "ConfirmRestart", DEFAULT_CONFIRM_RESTART);
          newtonbutton->confirm_shutdown_prop = xfce_rc_read_bool_entry(rc, "ConfirmShutdown", DEFAULT_CONFIRM_SHUTDOWN);
          newtonbutton->confirm_force_quit_prop = xfce_rc_read_bool_entry(rc, "ConfirmForceQuit", DEFAULT_CONFIRM_FORCE_QUIT);
          
          xfce_rc_close (rc);
          return;
        }
    }

  newtonbutton->display_icon_prop = DEFAULT_DISPLAY_ICON;
  g_free(newtonbutton->icon_name_prop);
  newtonbutton->icon_name_prop = g_strdup (DEFAULT_ICON_NAME);
  g_free(newtonbutton->label_text_prop);
  newtonbutton->label_text_prop = g_strdup (_(DEFAULT_LABEL_TEXT));

  newtonbutton->confirm_logout_prop = DEFAULT_CONFIRM_LOGOUT;
  newtonbutton->confirm_restart_prop = DEFAULT_CONFIRM_RESTART;
  newtonbutton->confirm_shutdown_prop = DEFAULT_CONFIRM_SHUTDOWN;
  newtonbutton->confirm_force_quit_prop = DEFAULT_CONFIRM_FORCE_QUIT;
}

void
newtonbutton_update_display (NewtonbuttonPlugin *newtonbutton)
{
    GtkIconTheme *icon_theme = NULL;
    gint panel_icon_size;

    g_return_if_fail (newtonbutton != NULL);
    g_return_if_fail (newtonbutton->plugin != NULL);
    g_return_if_fail (GTK_IS_BUTTON (newtonbutton->button));
    g_return_if_fail (GTK_IS_BOX (newtonbutton->button_box));
    g_return_if_fail (GTK_IS_IMAGE (newtonbutton->icon_image));
    g_return_if_fail (GTK_IS_LABEL (newtonbutton->label_widget));

    if (newtonbutton->display_icon_prop)
    {
        gtk_widget_show (newtonbutton->icon_image);
        gtk_widget_hide (newtonbutton->label_widget);

        icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (newtonbutton->plugin)));
        panel_icon_size = xfce_panel_plugin_get_icon_size (newtonbutton->plugin);
        
        if (newtonbutton->icon_name_prop && strlen(newtonbutton->icon_name_prop) > 0) {
            if (gtk_icon_theme_has_icon(icon_theme, newtonbutton->icon_name_prop)) {
                 gtk_image_set_from_icon_name (GTK_IMAGE (newtonbutton->icon_image),
                                          newtonbutton->icon_name_prop,
                                          GTK_ICON_SIZE_BUTTON);
                 gtk_image_set_pixel_size(GTK_IMAGE(newtonbutton->icon_image), panel_icon_size);
            } else if (g_file_test(newtonbutton->icon_name_prop, G_FILE_TEST_IS_REGULAR)) {
                GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size(newtonbutton->icon_name_prop, panel_icon_size, panel_icon_size, NULL);
                if (pixbuf) {
                    gtk_image_set_from_pixbuf(GTK_IMAGE(newtonbutton->icon_image), pixbuf);
                    g_object_unref(pixbuf);
                } else {
                    gtk_image_set_from_icon_name (GTK_IMAGE (newtonbutton->icon_image), "image-missing", GTK_ICON_SIZE_BUTTON);
                    gtk_image_set_pixel_size(GTK_IMAGE(newtonbutton->icon_image), panel_icon_size);
                }
            } else {
                 gtk_image_set_from_icon_name (GTK_IMAGE (newtonbutton->icon_image), "image-missing", GTK_ICON_SIZE_BUTTON);
                 gtk_image_set_pixel_size(GTK_IMAGE(newtonbutton->icon_image), panel_icon_size);
            }
        } else {
             gtk_image_set_from_icon_name (GTK_IMAGE (newtonbutton->icon_image), "image-missing", GTK_ICON_SIZE_BUTTON);
             gtk_image_set_pixel_size(GTK_IMAGE(newtonbutton->icon_image), panel_icon_size);
        }
    }
    else
    {
        gtk_widget_hide (newtonbutton->icon_image);
        gtk_widget_show (newtonbutton->label_widget);
        gtk_label_set_text (GTK_LABEL (newtonbutton->label_widget), 
                            newtonbutton->label_text_prop ? _(newtonbutton->label_text_prop) : "");
    }

    gtk_widget_queue_resize (GTK_WIDGET(newtonbutton->plugin));
}

static NewtonbuttonPlugin *
newtonbutton_new (XfcePanelPlugin *plugin)
{
  NewtonbuttonPlugin   *newtonbutton;
  GtkOrientation  orientation;

  newtonbutton = g_slice_new0 (NewtonbuttonPlugin);
  newtonbutton->plugin = plugin;
  newtonbutton_read (newtonbutton);

  orientation = xfce_panel_plugin_get_orientation (plugin);

  newtonbutton->event_box = gtk_event_box_new ();
  gtk_widget_show (newtonbutton->event_box);

  newtonbutton->button = gtk_toggle_button_new (); 
  gtk_button_set_relief (GTK_BUTTON (newtonbutton->button), GTK_RELIEF_NONE);
  gtk_widget_show (newtonbutton->button);
  gtk_container_add (GTK_CONTAINER (newtonbutton->event_box), newtonbutton->button);

  newtonbutton->button_box = gtk_box_new (orientation, 2);
  gtk_container_set_border_width(GTK_CONTAINER(newtonbutton->button_box), 2);
  gtk_widget_show (newtonbutton->button_box);
  gtk_container_add (GTK_CONTAINER (newtonbutton->button), newtonbutton->button_box);

  newtonbutton->icon_image = gtk_image_new ();
  gtk_box_pack_start (GTK_BOX (newtonbutton->button_box), newtonbutton->icon_image, FALSE, FALSE, 0);

  newtonbutton->label_widget = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (newtonbutton->button_box), newtonbutton->label_widget, TRUE, TRUE, 0);
  
  newtonbutton_update_display (newtonbutton);
  newtonbutton->main_menu = NULL;

  return newtonbutton;
}

static void
newtonbutton_free (XfcePanelPlugin *plugin,
             NewtonbuttonPlugin    *newtonbutton)
{
  GtkWidget *dialog;

  g_return_if_fail(newtonbutton != NULL);

  dialog = g_object_get_data (G_OBJECT (plugin), "dialog");
  if (G_UNLIKELY (dialog != NULL))
    gtk_widget_destroy (dialog);

  g_free (newtonbutton->icon_name_prop);
  newtonbutton->icon_name_prop = NULL;
  g_free (newtonbutton->label_text_prop);
  newtonbutton->label_text_prop = NULL;

  if (newtonbutton->main_menu)
  {
      newtonbutton->main_menu = NULL; 
  }
  
  g_slice_free (NewtonbuttonPlugin, newtonbutton);
}

static void
newtonbutton_orientation_changed (XfcePanelPlugin *plugin,
                            GtkOrientation   orientation,
                            NewtonbuttonPlugin    *newtonbutton)
{
  g_return_if_fail(newtonbutton != NULL);
  g_return_if_fail(GTK_IS_BOX(newtonbutton->button_box));

  gtk_orientable_set_orientation(GTK_ORIENTABLE(newtonbutton->button_box), orientation);
  newtonbutton_update_display(newtonbutton);
}

static gboolean
newtonbutton_size_changed (XfcePanelPlugin *plugin,
                     gint             size,
                     NewtonbuttonPlugin    *newtonbutton)
{
  GtkOrientation orientation;

  g_return_val_if_fail(newtonbutton != NULL, TRUE);

  orientation = xfce_panel_plugin_get_orientation (plugin);

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
  else
    gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);


  newtonbutton_update_display(newtonbutton);
  return TRUE;
}

static void
on_about_this_pc_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    XfcePanelPlugin *plugin = XFCE_PANEL_PLUGIN(user_data);
    g_return_if_fail(plugin != NULL);
    newtonbutton_about(plugin);
}

static void
on_system_settings_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    execute_command("xfce4-settings-manager");
}

static void
on_app_store_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    g_debug("App Store activated - placeholder: No standard XFCE app store command.");
}

static void
on_force_quit_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    NewtonbuttonPlugin *newtonbutton = (NewtonbuttonPlugin*)user_data;
    GtkWindow *parent_window = NULL;

    g_return_if_fail(newtonbutton != NULL);

    if (newtonbutton->plugin && gtk_widget_get_toplevel(GTK_WIDGET(newtonbutton->plugin))) {
        parent_window = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(newtonbutton->plugin)));
    }

    if (newtonbutton->confirm_force_quit_prop) {
        newtonbutton_show_force_quit_confirmation(parent_window);
    } else {
        execute_command("xkill");
    }
}

static void
on_sleep_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    // Sleep action typically does not need confirmation by default from most OSes.
    // If needed, it would use newtonbutton_show_generic_confirmation.
    execute_command("xfce4-session-logout --suspend");
}

static void
on_restart_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    NewtonbuttonPlugin *newtonbutton = (NewtonbuttonPlugin*)user_data;
    GtkWindow *parent_window = NULL;

    g_return_if_fail(newtonbutton != NULL);

    if (newtonbutton->plugin && gtk_widget_get_toplevel(GTK_WIDGET(newtonbutton->plugin))) {
        parent_window = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(newtonbutton->plugin)));
    }

    if (newtonbutton->confirm_restart_prop) {
        newtonbutton_show_generic_confirmation(parent_window, _("restart"), _("Restart"), "xfce4-session-logout --reboot");
    } else {
        execute_command("xfce4-session-logout --reboot");
    }
}

static void
on_shutdown_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    NewtonbuttonPlugin *newtonbutton = (NewtonbuttonPlugin*)user_data;
    GtkWindow *parent_window = NULL;

    g_return_if_fail(newtonbutton != NULL);

    if (newtonbutton->plugin && gtk_widget_get_toplevel(GTK_WIDGET(newtonbutton->plugin))) {
        parent_window = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(newtonbutton->plugin)));
    }
    
    if (newtonbutton->confirm_shutdown_prop) {
        newtonbutton_show_generic_confirmation(parent_window, _("shut down"), _("Shut Down"), "xfce4-session-logout --halt");
    } else {
        execute_command("xfce4-session-logout --halt");
    }
}

static void
on_lock_screen_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    // Lock screen action typically does not need confirmation.
    execute_command("xflock4");
}

static void
on_log_out_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    NewtonbuttonPlugin *newtonbutton = (NewtonbuttonPlugin*)user_data;
    GtkWindow *parent_window = NULL;
    const gchar *username = g_get_user_name();
    gchar *action_name; // For "log out Adam" vs "log out"

    g_return_if_fail(newtonbutton != NULL);

    if (newtonbutton->plugin && gtk_widget_get_toplevel(GTK_WIDGET(newtonbutton->plugin))) {
        parent_window = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(newtonbutton->plugin)));
    }

    if (username) {
        // Construct "log out <username>"
        action_name = g_strdup_printf(_("log out %s"), username);
    } else {
        action_name = g_strdup(_("log out")); // Fallback
    }

    if (newtonbutton->confirm_logout_prop) {
        newtonbutton_show_generic_confirmation(parent_window, action_name, _("Log Out"), "xfce4-session-logout --logout");
    } else {
        execute_command("xfce4-session-logout --logout");
    }
    g_free(action_name);
}


static void
newtonbutton_menu_deactivate_cb (GtkMenu *menu, NewtonbuttonPlugin *newtonbutton)
{
    gulong handler_id;
    g_return_if_fail (newtonbutton != NULL);
    g_return_if_fail (GTK_IS_TOGGLE_BUTTON (newtonbutton->button));

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(newtonbutton->button)))
    {
        handler_id = g_signal_handler_find(newtonbutton->button, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, G_CALLBACK(newtonbutton_popup_menu_on_toggle), NULL);
        if (handler_id > 0) {
            g_signal_handler_block(newtonbutton->button, handler_id);
        }

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(newtonbutton->button), FALSE);

        if (handler_id > 0) {
            g_signal_handler_unblock(newtonbutton->button, handler_id);
        }
    }
    xfce_panel_plugin_block_autohide (newtonbutton->plugin, FALSE);
}

static void
newtonbutton_popup_menu_on_toggle (GtkToggleButton *toggle_button, NewtonbuttonPlugin *newtonbutton)
{
    GtkWidget *menu_item;
    GtkAccelGroup *accel_group;

    g_return_if_fail (newtonbutton != NULL);
    g_return_if_fail (GTK_IS_TOGGLE_BUTTON (toggle_button));

    if (!gtk_toggle_button_get_active(toggle_button))
    {
        if (newtonbutton->main_menu && gtk_widget_get_visible(newtonbutton->main_menu)) {
            gtk_menu_popdown(GTK_MENU(newtonbutton->main_menu));
        }
        xfce_panel_plugin_block_autohide (newtonbutton->plugin, FALSE);
        return;
    }

    xfce_panel_plugin_block_autohide (newtonbutton->plugin, TRUE);

    if (newtonbutton->main_menu == NULL)
    {
        newtonbutton->main_menu = gtk_menu_new();
        accel_group = gtk_accel_group_new ();
        gtk_menu_set_accel_group (GTK_MENU (newtonbutton->main_menu), accel_group);
        g_object_unref(accel_group);
        
        menu_item = gtk_menu_item_new_with_mnemonic (_("About This PC"));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_about_this_pc_activate), newtonbutton->plugin);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonbutton->main_menu), menu_item);
        
        gtk_menu_shell_append(GTK_MENU_SHELL(newtonbutton->main_menu), gtk_separator_menu_item_new());

        menu_item = gtk_menu_item_new_with_mnemonic (_("System Settings..."));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_system_settings_activate), newtonbutton);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonbutton->main_menu), menu_item);

        menu_item = gtk_menu_item_new_with_mnemonic (_("App Store..."));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_app_store_activate), newtonbutton);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonbutton->main_menu), menu_item);
        
        gtk_menu_shell_append(GTK_MENU_SHELL(newtonbutton->main_menu), gtk_separator_menu_item_new());
        
        menu_item = gtk_menu_item_new_with_mnemonic (_("Force Quit..."));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_force_quit_activate), newtonbutton);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonbutton->main_menu), menu_item);

        gtk_menu_shell_append(GTK_MENU_SHELL(newtonbutton->main_menu), gtk_separator_menu_item_new());

        menu_item = gtk_menu_item_new_with_mnemonic (_("Sleep"));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_sleep_activate), newtonbutton);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonbutton->main_menu), menu_item);
        
        menu_item = gtk_menu_item_new_with_mnemonic (_("Restart..."));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_restart_activate), newtonbutton);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonbutton->main_menu), menu_item);

        menu_item = gtk_menu_item_new_with_mnemonic (_("Shut Down..."));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_shutdown_activate), newtonbutton);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonbutton->main_menu), menu_item);
        
        gtk_menu_shell_append(GTK_MENU_SHELL(newtonbutton->main_menu), gtk_separator_menu_item_new());

        menu_item = gtk_menu_item_new_with_mnemonic (_("Lock Screen"));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_lock_screen_activate), newtonbutton);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonbutton->main_menu), menu_item);

        const gchar* username = g_get_user_name();
        gchar* logout_label;
        if (username) {
            logout_label = g_strdup_printf(_("Log Out %s..."), username);
        } else {
            logout_label = g_strdup(_("Log Out..."));
        }
        menu_item = gtk_menu_item_new_with_mnemonic (logout_label);
        g_free(logout_label);
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_log_out_activate), newtonbutton);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonbutton->main_menu), menu_item);
        
        g_signal_connect (newtonbutton->main_menu, "deactivate",
                          G_CALLBACK (newtonbutton_menu_deactivate_cb), newtonbutton);
        
        g_object_add_weak_pointer (G_OBJECT(newtonbutton->main_menu), (gpointer *) &(newtonbutton->main_menu));
    }
    
    gtk_widget_show_all(newtonbutton->main_menu);

    xfce_panel_plugin_popup_menu(newtonbutton->plugin,
                                 GTK_MENU(newtonbutton->main_menu),
                                 GTK_WIDGET(toggle_button),
                                 NULL);
}

static void
newtonbutton_construct (XfcePanelPlugin *plugin)
{
  NewtonbuttonPlugin *newtonbutton;

  xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
  newtonbutton = newtonbutton_new (plugin);
  g_return_if_fail(newtonbutton != NULL);

  gtk_container_add (GTK_CONTAINER (plugin), newtonbutton->event_box);
  xfce_panel_plugin_add_action_widget (plugin, newtonbutton->event_box);
  
  g_signal_connect (G_OBJECT (plugin), "free-data", G_CALLBACK (newtonbutton_free), newtonbutton);
  g_signal_connect (G_OBJECT (plugin), "save", G_CALLBACK (newtonbutton_save), newtonbutton);
  g_signal_connect (G_OBJECT (plugin), "size-changed", G_CALLBACK (newtonbutton_size_changed), newtonbutton);
  g_signal_connect (G_OBJECT (plugin), "orientation-changed", G_CALLBACK (newtonbutton_orientation_changed), newtonbutton);
  
  g_return_if_fail(newtonbutton->button != NULL);
  g_signal_connect (G_OBJECT (newtonbutton->button), "toggled", G_CALLBACK (newtonbutton_popup_menu_on_toggle), newtonbutton);

  xfce_panel_plugin_menu_show_configure (plugin);
  g_signal_connect (G_OBJECT (plugin), "configure-plugin", G_CALLBACK (newtonbutton_configure), newtonbutton);
  xfce_panel_plugin_menu_show_about (plugin);
  g_signal_connect (G_OBJECT (plugin), "about", G_CALLBACK (newtonbutton_about), plugin);
}