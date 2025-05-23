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

#include "newtonmenu.h"
#include "newtonmenu-dialogs.h" 
#include "newtonmenu-force-quit-dialog.h"

#define DEFAULT_DISPLAY_ICON TRUE
#define DEFAULT_ICON_NAME "xfce4-newtonmenu-plugin"
#define DEFAULT_LABEL_TEXT N_("Menu")

#define DEFAULT_CONFIRM_LOGOUT FALSE
#define DEFAULT_CONFIRM_RESTART TRUE
#define DEFAULT_CONFIRM_SHUTDOWN TRUE
#define DEFAULT_CONFIRM_FORCE_QUIT FALSE 

static void newtonmenu_construct (XfcePanelPlugin *plugin);
static void newtonmenu_read (newtonmenuPlugin *newtonmenu);
static void newtonmenu_free (XfcePanelPlugin *plugin, newtonmenuPlugin *newtonmenu);
static void newtonmenu_orientation_changed (XfcePanelPlugin *plugin, GtkOrientation orientation, newtonmenuPlugin *newtonmenu);
static gboolean newtonmenu_size_changed (XfcePanelPlugin *plugin, gint size, newtonmenuPlugin *newtonmenu);
static newtonmenuPlugin* newtonmenu_new (XfcePanelPlugin *plugin);
static void newtonmenu_popup_menu_on_toggle (GtkToggleButton *toggle_button, newtonmenuPlugin *newtonmenu);
static void newtonmenu_menu_deactivate_cb (GtkMenu *menu, newtonmenuPlugin *newtonmenu);

static void on_about_this_pc_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_system_settings_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_run_command_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_force_quit_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_sleep_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_restart_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_shutdown_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_lock_screen_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_log_out_activate(GtkMenuItem *menuitem, gpointer user_data);

static void execute_command(const gchar *command);


XFCE_PANEL_PLUGIN_REGISTER (newtonmenu_construct);


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
newtonmenu_save (XfcePanelPlugin *plugin,
             newtonmenuPlugin    *newtonmenu)
{
  XfceRc *rc;
  gchar  *file;

  g_return_if_fail(newtonmenu != NULL);
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
      xfce_rc_write_bool_entry (rc, "DisplayIcon", newtonmenu->display_icon_prop);

      if (newtonmenu->icon_name_prop)
        xfce_rc_write_entry (rc, "IconName", newtonmenu->icon_name_prop);
      else
        xfce_rc_write_entry (rc, "IconName", "");

      if (newtonmenu->label_text_prop)
        xfce_rc_write_entry (rc, "LabelText", newtonmenu->label_text_prop);
      else
        xfce_rc_write_entry (rc, "LabelText", "");
      
      xfce_rc_write_bool_entry(rc, "ConfirmLogout", newtonmenu->confirm_logout_prop);
      xfce_rc_write_bool_entry(rc, "ConfirmRestart", newtonmenu->confirm_restart_prop);
      xfce_rc_write_bool_entry(rc, "ConfirmShutdown", newtonmenu->confirm_shutdown_prop);
      xfce_rc_write_bool_entry(rc, "ConfirmForceQuit", newtonmenu->confirm_force_quit_prop);

      xfce_rc_close (rc);
    }
}

static void
newtonmenu_read (newtonmenuPlugin *newtonmenu)
{
  XfceRc      *rc;
  gchar       *file;
  const gchar *value;

  g_return_if_fail(newtonmenu != NULL);
  g_return_if_fail(newtonmenu->plugin != NULL);

  file = xfce_panel_plugin_save_location (newtonmenu->plugin, TRUE);

  if (G_LIKELY (file != NULL))
    {
      rc = xfce_rc_simple_open (file, TRUE);
      g_free (file);

      if (G_LIKELY (rc != NULL))
        {
          newtonmenu->display_icon_prop = xfce_rc_read_bool_entry (rc, "DisplayIcon", DEFAULT_DISPLAY_ICON);

          value = xfce_rc_read_entry (rc, "IconName", DEFAULT_ICON_NAME);
          g_free(newtonmenu->icon_name_prop);
          newtonmenu->icon_name_prop = g_strdup (value);

          value = xfce_rc_read_entry (rc, "LabelText", _(DEFAULT_LABEL_TEXT));
          g_free(newtonmenu->label_text_prop);
          newtonmenu->label_text_prop = g_strdup (value);

          newtonmenu->confirm_logout_prop = xfce_rc_read_bool_entry(rc, "ConfirmLogout", DEFAULT_CONFIRM_LOGOUT);
          newtonmenu->confirm_restart_prop = xfce_rc_read_bool_entry(rc, "ConfirmRestart", DEFAULT_CONFIRM_RESTART);
          newtonmenu->confirm_shutdown_prop = xfce_rc_read_bool_entry(rc, "ConfirmShutdown", DEFAULT_CONFIRM_SHUTDOWN);
          newtonmenu->confirm_force_quit_prop = xfce_rc_read_bool_entry(rc, "ConfirmForceQuit", DEFAULT_CONFIRM_FORCE_QUIT);
          
          xfce_rc_close (rc);
          return;
        }
    }

  newtonmenu->display_icon_prop = DEFAULT_DISPLAY_ICON;
  g_free(newtonmenu->icon_name_prop);
  newtonmenu->icon_name_prop = g_strdup (DEFAULT_ICON_NAME);
  g_free(newtonmenu->label_text_prop);
  newtonmenu->label_text_prop = g_strdup (_(DEFAULT_LABEL_TEXT));

  newtonmenu->confirm_logout_prop = DEFAULT_CONFIRM_LOGOUT;
  newtonmenu->confirm_restart_prop = DEFAULT_CONFIRM_RESTART;
  newtonmenu->confirm_shutdown_prop = DEFAULT_CONFIRM_SHUTDOWN;
  newtonmenu->confirm_force_quit_prop = DEFAULT_CONFIRM_FORCE_QUIT;
}

void
newtonmenu_update_display (newtonmenuPlugin *newtonmenu)
{
    GtkIconTheme *icon_theme = NULL;
    gint panel_icon_size;

    g_return_if_fail (newtonmenu != NULL);
    g_return_if_fail (newtonmenu->plugin != NULL);
    g_return_if_fail (GTK_IS_BUTTON (newtonmenu->button));
    g_return_if_fail (GTK_IS_BOX (newtonmenu->button_box));
    g_return_if_fail (GTK_IS_IMAGE (newtonmenu->icon_image));
    g_return_if_fail (GTK_IS_LABEL (newtonmenu->label_widget));

    if (newtonmenu->display_icon_prop)
    {
        gtk_widget_show (newtonmenu->icon_image);
        gtk_widget_hide (newtonmenu->label_widget);

        icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (newtonmenu->plugin)));
        panel_icon_size = xfce_panel_plugin_get_icon_size (newtonmenu->plugin);
        
        if (newtonmenu->icon_name_prop && strlen(newtonmenu->icon_name_prop) > 0) {
            if (gtk_icon_theme_has_icon(icon_theme, newtonmenu->icon_name_prop)) {
                 gtk_image_set_from_icon_name (GTK_IMAGE (newtonmenu->icon_image),
                                          newtonmenu->icon_name_prop,
                                          GTK_ICON_SIZE_BUTTON);
                 gtk_image_set_pixel_size(GTK_IMAGE(newtonmenu->icon_image), panel_icon_size);
            } else if (g_file_test(newtonmenu->icon_name_prop, G_FILE_TEST_IS_REGULAR)) {
                GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size(newtonmenu->icon_name_prop, panel_icon_size, panel_icon_size, NULL);
                if (pixbuf) {
                    gtk_image_set_from_pixbuf(GTK_IMAGE(newtonmenu->icon_image), pixbuf);
                    g_object_unref(pixbuf);
                } else {
                    gtk_image_set_from_icon_name (GTK_IMAGE (newtonmenu->icon_image), "image-missing", GTK_ICON_SIZE_BUTTON);
                    gtk_image_set_pixel_size(GTK_IMAGE(newtonmenu->icon_image), panel_icon_size);
                }
            } else {
                 gtk_image_set_from_icon_name (GTK_IMAGE (newtonmenu->icon_image), "image-missing", GTK_ICON_SIZE_BUTTON);
                 gtk_image_set_pixel_size(GTK_IMAGE(newtonmenu->icon_image), panel_icon_size);
            }
        } else {
             gtk_image_set_from_icon_name (GTK_IMAGE (newtonmenu->icon_image), "image-missing", GTK_ICON_SIZE_BUTTON);
             gtk_image_set_pixel_size(GTK_IMAGE(newtonmenu->icon_image), panel_icon_size);
        }
    }
    else
    {
        gtk_widget_hide (newtonmenu->icon_image);
        gtk_widget_show (newtonmenu->label_widget);
        gtk_label_set_text (GTK_LABEL (newtonmenu->label_widget), 
                            newtonmenu->label_text_prop ? _(newtonmenu->label_text_prop) : "");
    }

    gtk_widget_queue_resize (GTK_WIDGET(newtonmenu->plugin));
}

static newtonmenuPlugin *
newtonmenu_new (XfcePanelPlugin *plugin)
{
  newtonmenuPlugin   *newtonmenu;
  GtkOrientation  orientation;

  newtonmenu = g_slice_new0 (newtonmenuPlugin);
  newtonmenu->plugin = plugin;
  newtonmenu_read (newtonmenu);

  orientation = xfce_panel_plugin_get_orientation (plugin);

  newtonmenu->event_box = gtk_event_box_new ();
  gtk_widget_show (newtonmenu->event_box);

  newtonmenu->button = gtk_toggle_button_new (); 
  gtk_button_set_relief (GTK_BUTTON (newtonmenu->button), GTK_RELIEF_NONE);
  gtk_widget_show (newtonmenu->button);
  gtk_container_add (GTK_CONTAINER (newtonmenu->event_box), newtonmenu->button);

  newtonmenu->button_box = gtk_box_new (orientation, 2);
  gtk_container_set_border_width(GTK_CONTAINER(newtonmenu->button_box), 2);
  gtk_widget_show (newtonmenu->button_box);
  gtk_container_add (GTK_CONTAINER (newtonmenu->button), newtonmenu->button_box);

  newtonmenu->icon_image = gtk_image_new ();
  gtk_box_pack_start (GTK_BOX (newtonmenu->button_box), newtonmenu->icon_image, FALSE, FALSE, 0);

  newtonmenu->label_widget = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (newtonmenu->button_box), newtonmenu->label_widget, TRUE, TRUE, 0);
  
  newtonmenu_update_display (newtonmenu);
  newtonmenu->main_menu = NULL;

  return newtonmenu;
}

static void
newtonmenu_free (XfcePanelPlugin *plugin,
             newtonmenuPlugin    *newtonmenu)
{
  GtkWidget *dialog;

  g_return_if_fail(newtonmenu != NULL);

  dialog = g_object_get_data (G_OBJECT (plugin), "dialog");
  if (G_UNLIKELY (dialog != NULL))
    gtk_widget_destroy (dialog);

  g_free (newtonmenu->icon_name_prop);
  newtonmenu->icon_name_prop = NULL;
  g_free (newtonmenu->label_text_prop);
  newtonmenu->label_text_prop = NULL;

  if (newtonmenu->main_menu)
  {
      newtonmenu->main_menu = NULL; 
  }
  
  g_slice_free (newtonmenuPlugin, newtonmenu);
}

static void
newtonmenu_orientation_changed (XfcePanelPlugin *plugin,
                            GtkOrientation   orientation,
                            newtonmenuPlugin    *newtonmenu)
{
  g_return_if_fail(newtonmenu != NULL);
  g_return_if_fail(GTK_IS_BOX(newtonmenu->button_box));

  gtk_orientable_set_orientation(GTK_ORIENTABLE(newtonmenu->button_box), orientation);
  newtonmenu_update_display(newtonmenu);
}

static gboolean
newtonmenu_size_changed (XfcePanelPlugin *plugin,
                     gint             size,
                     newtonmenuPlugin    *newtonmenu)
{
  GtkOrientation orientation;

  g_return_val_if_fail(newtonmenu != NULL, TRUE);

  orientation = xfce_panel_plugin_get_orientation (plugin);

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
  else
    gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);


  newtonmenu_update_display(newtonmenu);
  return TRUE;
}

static void
on_about_this_pc_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    execute_command("xfce4-about");
}

static void
on_system_settings_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    execute_command("xfce4-settings-manager");
}

static void
on_run_command_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    execute_command("xfce4-appfinder --collapsed");
}

static void
on_force_quit_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    newtonmenuPlugin *newtonmenu = (newtonmenuPlugin*)user_data;
    GtkWindow *parent_window = NULL;

    g_return_if_fail(newtonmenu != NULL);

    if (newtonmenu->plugin && gtk_widget_get_toplevel(GTK_WIDGET(newtonmenu->plugin))) {
        parent_window = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(newtonmenu->plugin)));
    }

    newtonmenu_show_force_quit_applications_dialog(parent_window, newtonmenu);
}

static void
on_sleep_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    execute_command("xfce4-session-logout --suspend");
}

static void
on_restart_activate(GtkMenuItem *menuitem, gpointer user_data)
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

static void
on_shutdown_activate(GtkMenuItem *menuitem, gpointer user_data)
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

static void
on_lock_screen_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    execute_command("xflock4");
}

static void
on_log_out_activate(GtkMenuItem *menuitem, gpointer user_data)
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
        action_name = g_strdup_printf(_("log out %s"), username);
    } else {
        action_name = g_strdup(_("log out")); 
    }

    if (newtonmenu->confirm_logout_prop) {
        newtonmenu_show_generic_confirmation(parent_window, action_name, _("Log Out"), "xfce4-session-logout --logout");
    } else {
        execute_command("xfce4-session-logout --logout");
    }
    g_free(action_name);
}


static void
newtonmenu_menu_deactivate_cb (GtkMenu *menu, newtonmenuPlugin *newtonmenu)
{
    gulong handler_id;
    g_return_if_fail (newtonmenu != NULL);
    g_return_if_fail (GTK_IS_TOGGLE_BUTTON (newtonmenu->button));

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(newtonmenu->button)))
    {
        handler_id = g_signal_handler_find(newtonmenu->button, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, G_CALLBACK(newtonmenu_popup_menu_on_toggle), NULL);
        if (handler_id > 0) {
            g_signal_handler_block(newtonmenu->button, handler_id);
        }

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(newtonmenu->button), FALSE);

        if (handler_id > 0) {
            g_signal_handler_unblock(newtonmenu->button, handler_id);
        }
    }
    xfce_panel_plugin_block_autohide (newtonmenu->plugin, FALSE);
}

static void
newtonmenu_popup_menu_on_toggle (GtkToggleButton *toggle_button, newtonmenuPlugin *newtonmenu)
{
    GtkWidget *menu_item;
    GtkAccelGroup *accel_group;

    g_return_if_fail (newtonmenu != NULL);
    g_return_if_fail (GTK_IS_TOGGLE_BUTTON (toggle_button));

    if (!gtk_toggle_button_get_active(toggle_button))
    {
        if (newtonmenu->main_menu && gtk_widget_get_visible(newtonmenu->main_menu)) {
            gtk_menu_popdown(GTK_MENU(newtonmenu->main_menu));
        }
        xfce_panel_plugin_block_autohide (newtonmenu->plugin, FALSE);
        return;
    }

    xfce_panel_plugin_block_autohide (newtonmenu->plugin, TRUE);

    if (newtonmenu->main_menu == NULL)
    {
        newtonmenu->main_menu = gtk_menu_new();
        accel_group = gtk_accel_group_new ();
        gtk_menu_set_accel_group (GTK_MENU (newtonmenu->main_menu), accel_group);
        g_object_unref(accel_group);
        
        menu_item = gtk_menu_item_new_with_mnemonic (_("About This PC"));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_about_this_pc_activate), newtonmenu->plugin);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonmenu->main_menu), menu_item);
        
        gtk_menu_shell_append(GTK_MENU_SHELL(newtonmenu->main_menu), gtk_separator_menu_item_new());

        menu_item = gtk_menu_item_new_with_mnemonic (_("System Settings..."));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_system_settings_activate), newtonmenu);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonmenu->main_menu), menu_item);

        menu_item = gtk_menu_item_new_with_mnemonic (_("Run Command..."));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_run_command_activate), newtonmenu);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonmenu->main_menu), menu_item);
        
        gtk_menu_shell_append(GTK_MENU_SHELL(newtonmenu->main_menu), gtk_separator_menu_item_new());
        
        menu_item = gtk_menu_item_new_with_mnemonic (_("Force Quit..."));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_force_quit_activate), newtonmenu); 
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonmenu->main_menu), menu_item);

        gtk_menu_shell_append(GTK_MENU_SHELL(newtonmenu->main_menu), gtk_separator_menu_item_new());

        menu_item = gtk_menu_item_new_with_mnemonic (_("Sleep"));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_sleep_activate), newtonmenu);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonmenu->main_menu), menu_item);
        
        menu_item = gtk_menu_item_new_with_mnemonic (_("Restart..."));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_restart_activate), newtonmenu);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonmenu->main_menu), menu_item);

        menu_item = gtk_menu_item_new_with_mnemonic (_("Shut Down..."));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_shutdown_activate), newtonmenu);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonmenu->main_menu), menu_item);
        
        gtk_menu_shell_append(GTK_MENU_SHELL(newtonmenu->main_menu), gtk_separator_menu_item_new());

        menu_item = gtk_menu_item_new_with_mnemonic (_("Lock Screen"));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_lock_screen_activate), newtonmenu);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonmenu->main_menu), menu_item);

        const gchar* username = g_get_user_name();
        gchar* logout_label;
        if (username) {
            logout_label = g_strdup_printf(_("Log Out %s..."), username);
        } else {
            logout_label = g_strdup(_("Log Out..."));
        }
        menu_item = gtk_menu_item_new_with_mnemonic (logout_label);
        g_free(logout_label);
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_log_out_activate), newtonmenu);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonmenu->main_menu), menu_item);
        
        g_signal_connect (newtonmenu->main_menu, "deactivate",
                          G_CALLBACK (newtonmenu_menu_deactivate_cb), newtonmenu);
        
        g_object_add_weak_pointer (G_OBJECT(newtonmenu->main_menu), (gpointer *) &(newtonmenu->main_menu));
    }
    
    gtk_widget_show_all(newtonmenu->main_menu);

    xfce_panel_plugin_popup_menu(newtonmenu->plugin,
                                 GTK_MENU(newtonmenu->main_menu),
                                 GTK_WIDGET(toggle_button),
                                 NULL);
}

static void
newtonmenu_construct (XfcePanelPlugin *plugin)
{
  newtonmenuPlugin *newtonmenu;

  xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
  newtonmenu = newtonmenu_new (plugin);
  g_return_if_fail(newtonmenu != NULL);

  gtk_container_add (GTK_CONTAINER (plugin), newtonmenu->event_box);
  xfce_panel_plugin_add_action_widget (plugin, newtonmenu->event_box);
  
  g_signal_connect (G_OBJECT (plugin), "free-data", G_CALLBACK (newtonmenu_free), newtonmenu);
  g_signal_connect (G_OBJECT (plugin), "save", G_CALLBACK (newtonmenu_save), newtonmenu);
  g_signal_connect (G_OBJECT (plugin), "size-changed", G_CALLBACK (newtonmenu_size_changed), newtonmenu);
  g_signal_connect (G_OBJECT (plugin), "orientation-changed", G_CALLBACK (newtonmenu_orientation_changed), newtonmenu);
  
  g_return_if_fail(newtonmenu->button != NULL);
  g_signal_connect (G_OBJECT (newtonmenu->button), "toggled", G_CALLBACK (newtonmenu_popup_menu_on_toggle), newtonmenu);

  xfce_panel_plugin_menu_show_configure (plugin);
  g_signal_connect (G_OBJECT (plugin), "configure-plugin", G_CALLBACK (newtonmenu_configure), newtonmenu);
  xfce_panel_plugin_menu_show_about (plugin);
  g_signal_connect (G_OBJECT (plugin), "about", G_CALLBACK (newtonmenu_about), plugin);
}