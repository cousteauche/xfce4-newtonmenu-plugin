#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <gdk/gdkkeysyms.h> // For GDK_KEY_ constants if used with accelerators

#include "newtonbutton.h"
#include "newtonbutton-dialogs.h"

#define DEFAULT_DISPLAY_ICON TRUE
#define DEFAULT_ICON_NAME "xfce4-newtonbutton-plugin" // Default to plugin's own icon
#define DEFAULT_LABEL_TEXT N_("Menu") // Translatable default label

// Forward declarations for static functions
static void newtonbutton_construct (XfcePanelPlugin *plugin);
static void newtonbutton_read (NewtonbuttonPlugin *newtonbutton);
static void newtonbutton_free (XfcePanelPlugin *plugin, NewtonbuttonPlugin *newtonbutton);
static void newtonbutton_orientation_changed (XfcePanelPlugin *plugin, GtkOrientation orientation, NewtonbuttonPlugin *newtonbutton);
static gboolean newtonbutton_size_changed (XfcePanelPlugin *plugin, gint size, NewtonbuttonPlugin *newtonbutton);
static NewtonbuttonPlugin* newtonbutton_new (XfcePanelPlugin *plugin);
static void newtonbutton_popup_menu_on_toggle (GtkToggleButton *toggle_button, NewtonbuttonPlugin *newtonbutton);
static void newtonbutton_menu_deactivate_cb (GtkMenu *menu, NewtonbuttonPlugin *newtonbutton);

// Callbacks for menu items
static void on_about_this_pc_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_system_settings_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_app_store_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_force_quit_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_sleep_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_restart_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_shutdown_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_lock_screen_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_log_out_activate(GtkMenuItem *menuitem, gpointer user_data);


XFCE_PANEL_PLUGIN_REGISTER (newtonbutton_construct);

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
          
          xfce_rc_close (rc);
          return;
        }
    }

  newtonbutton->display_icon_prop = DEFAULT_DISPLAY_ICON;
  g_free(newtonbutton->icon_name_prop);
  newtonbutton->icon_name_prop = g_strdup (DEFAULT_ICON_NAME);
  g_free(newtonbutton->label_text_prop);
  newtonbutton->label_text_prop = g_strdup (_(DEFAULT_LABEL_TEXT));
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
                                          GTK_ICON_SIZE_BUTTON); // Using a symbolic size
                 gtk_image_set_pixel_size(GTK_IMAGE(newtonbutton->icon_image), panel_icon_size);
            } else if (g_file_test(newtonbutton->icon_name_prop, G_FILE_TEST_IS_REGULAR)) {
                GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size(newtonbutton->icon_name_prop, panel_icon_size, panel_icon_size, NULL);
                if (pixbuf) {
                    gtk_image_set_from_pixbuf(GTK_IMAGE(newtonbutton->icon_image), pixbuf);
                    g_object_unref(pixbuf);
                } else { // Fallback if file path is invalid or not an image
                    gtk_image_set_from_icon_name (GTK_IMAGE (newtonbutton->icon_image), "image-missing", GTK_ICON_SIZE_BUTTON);
                    gtk_image_set_pixel_size(GTK_IMAGE(newtonbutton->icon_image), panel_icon_size);
                }
            } else { // Fallback if icon name is not in theme and not a valid file
                 gtk_image_set_from_icon_name (GTK_IMAGE (newtonbutton->icon_image), "image-missing", GTK_ICON_SIZE_BUTTON);
                 gtk_image_set_pixel_size(GTK_IMAGE(newtonbutton->icon_image), panel_icon_size);
            }
        } else { // Fallback if icon_name_prop is empty
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

  // Use a small border/padding for the button content
  newtonbutton->button_box = gtk_box_new (orientation, 2); // Spacing of 2
  gtk_container_set_border_width(GTK_CONTAINER(newtonbutton->button_box), 2); // Border of 2
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
      // gtk_widget_destroy will be called when the GObject weak ref is broken or explicitly
      // No need to destroy if it's already being handled by GTK's window management
      newtonbutton->main_menu = NULL; // The weak pointer will set this to NULL if destroyed elsewhere
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

  // Let the panel manage the plugin's size based on its content (icon/label)
  // and the panel's size. Explicitly setting size_request can interfere.
  // gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, -1); // Reset size request

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
  else
    gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);


  newtonbutton_update_display(newtonbutton);
  return TRUE;
}

// Menu Item Callbacks
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
    g_debug("System Settings activated - placeholder");
    // Actual command: g_spawn_command_line_async ("xfce4-settings-manager", NULL);
}

static void
on_app_store_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    g_debug("App Store activated - placeholder");
    // Actual command: e.g., g_spawn_command_line_async ("pamac-manager", NULL); or ("gnome-software", NULL);
}

static void
on_force_quit_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    g_debug("Force Quit activated - placeholder");
    // Actual command: g_spawn_command_line_async ("xkill", NULL);
}

static void
on_sleep_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    g_debug("Sleep activated - placeholder");
    // Actual command: g_spawn_command_line_async ("xfce4-session-logout --suspend", NULL);
}

static void
on_restart_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    g_debug("Restart activated - placeholder");
    // Actual command: g_spawn_command_line_async ("xfce4-session-logout --reboot", NULL);
}

static void
on_shutdown_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    g_debug("Shut Down activated - placeholder");
    // Actual command: g_spawn_command_line_async ("xfce4-session-logout --halt", NULL);
}

static void
on_lock_screen_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    g_debug("Lock Screen activated - placeholder");
    // Actual command: g_spawn_command_line_async ("xflock4", NULL);
}

static void
on_log_out_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    g_debug("Log Out activated - placeholder");
    // Actual command: g_spawn_command_line_async ("xfce4-session-logout --logout", NULL);
}


static void
newtonbutton_menu_deactivate_cb (GtkMenu *menu, NewtonbuttonPlugin *newtonbutton)
{
    gulong handler_id;
    g_return_if_fail (newtonbutton != NULL);
    g_return_if_fail (GTK_IS_TOGGLE_BUTTON (newtonbutton->button));

    // Ensure the toggle button state is consistent with the menu being closed
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(newtonbutton->button)))
    {
        // Temporarily block the "toggled" signal handler to prevent recursion
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
        // If menu is visible and button is toggled off, hide menu
        if (newtonbutton->main_menu && gtk_widget_get_visible(newtonbutton->main_menu)) {
            gtk_menu_popdown(GTK_MENU(newtonbutton->main_menu));
        }
        xfce_panel_plugin_block_autohide (newtonbutton->plugin, FALSE);
        return;
    }

    // Button is toggled on, show the menu
    xfce_panel_plugin_block_autohide (newtonbutton->plugin, TRUE);

    if (newtonbutton->main_menu == NULL)
    {
        newtonbutton->main_menu = gtk_menu_new();
        // Create an accel group for the menu
        accel_group = gtk_accel_group_new ();
        gtk_menu_set_accel_group (GTK_MENU (newtonbutton->main_menu), accel_group);
        g_object_unref(accel_group); // The menu now owns the accel group
        
        // About This PC
        menu_item = gtk_menu_item_new_with_mnemonic (_("About This PC"));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_about_this_pc_activate), newtonbutton->plugin);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonbutton->main_menu), menu_item);
        
        gtk_menu_shell_append(GTK_MENU_SHELL(newtonbutton->main_menu), gtk_separator_menu_item_new());

        // System Settings...
        menu_item = gtk_menu_item_new_with_mnemonic (_("System Settings..."));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_system_settings_activate), newtonbutton);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonbutton->main_menu), menu_item);

        // App Store...
        menu_item = gtk_menu_item_new_with_mnemonic (_("App Store..."));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_app_store_activate), newtonbutton);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonbutton->main_menu), menu_item);
        
        gtk_menu_shell_append(GTK_MENU_SHELL(newtonbutton->main_menu), gtk_separator_menu_item_new());
        
        // Force Quit...
        menu_item = gtk_menu_item_new_with_mnemonic (_("Force Quit..."));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_force_quit_activate), newtonbutton);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonbutton->main_menu), menu_item);

        gtk_menu_shell_append(GTK_MENU_SHELL(newtonbutton->main_menu), gtk_separator_menu_item_new());

        // Sleep
        menu_item = gtk_menu_item_new_with_mnemonic (_("Sleep"));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_sleep_activate), newtonbutton);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonbutton->main_menu), menu_item);
        
        // Restart...
        menu_item = gtk_menu_item_new_with_mnemonic (_("Restart..."));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_restart_activate), newtonbutton);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonbutton->main_menu), menu_item);

        // Shut Down...
        menu_item = gtk_menu_item_new_with_mnemonic (_("Shut Down..."));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_shutdown_activate), newtonbutton);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonbutton->main_menu), menu_item);
        
        gtk_menu_shell_append(GTK_MENU_SHELL(newtonbutton->main_menu), gtk_separator_menu_item_new());

        // Lock Screen
        menu_item = gtk_menu_item_new_with_mnemonic (_("Lock Screen"));
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_lock_screen_activate), newtonbutton);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonbutton->main_menu), menu_item);
        /* Example of adding an accelerator:
           gtk_widget_add_accelerator (menu_item, "activate", accel_group,
                                    GDK_KEY_L, GDK_CONTROL_MASK | GDK_META_MASK, GTK_ACCEL_VISIBLE);
        */

        // Log Out %s...
        const gchar* username = g_get_user_name(); // Corrected to const gchar*
        gchar* logout_label;
        if (username) {
            logout_label = g_strdup_printf(_("Log Out %s..."), username);
        } else {
            logout_label = g_strdup(_("Log Out...")); // Fallback if username is not available
        }
        menu_item = gtk_menu_item_new_with_mnemonic (logout_label);
        g_free(logout_label);
        g_signal_connect (menu_item, "activate", G_CALLBACK (on_log_out_activate), newtonbutton);
        gtk_menu_shell_append (GTK_MENU_SHELL (newtonbutton->main_menu), menu_item);
        /* Example of adding an accelerator:
           gtk_widget_add_accelerator (menu_item, "activate", accel_group,
                                    GDK_KEY_Delete, GDK_CONTROL_MASK | GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
        */
        
        // Connect deactivate AFTER all items are added and menu is constructed
        g_signal_connect (newtonbutton->main_menu, "deactivate",
                          G_CALLBACK (newtonbutton_menu_deactivate_cb), newtonbutton);
        
        // Add weak pointer so menu can be destroyed if plugin is removed while menu is open
        g_object_add_weak_pointer (G_OBJECT(newtonbutton->main_menu), (gpointer *) &(newtonbutton->main_menu));
    }
    
    gtk_widget_show_all(newtonbutton->main_menu);

    // Use the standard panel plugin function to show the menu relative to the button
    xfce_panel_plugin_popup_menu(newtonbutton->plugin,
                                 GTK_MENU(newtonbutton->main_menu),
                                 GTK_WIDGET(toggle_button),
                                 NULL); // Pass NULL for event, position relative to button
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