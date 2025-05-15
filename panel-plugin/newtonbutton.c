/*  $Id$
 *
 *  Copyright (C) 2019 John Doo <john@foo.org> // Zmień na swoje dane!
 *  (Adapted for Newtonbutton by Adam & AI)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h> // Dla xfce_panel_plugin_get_icon_size

#include "newtonbutton.h"
#include "newtonbutton-dialogs.h"

/* default settings */
#define DEFAULT_DISPLAY_ICON TRUE
#define DEFAULT_ICON_NAME "xfce4-newtonbutton-plugin"
#define DEFAULT_LABEL_TEXT N_("Newton")

/* prototypes */
static void newtonbutton_construct (XfcePanelPlugin *plugin);
static void newtonbutton_update_display (NewtonbuttonPlugin *newtonbutton);
static void newtonbutton_read (NewtonbuttonPlugin *newtonbutton);

/* register the plugin */
XFCE_PANEL_PLUGIN_REGISTER (newtonbutton_construct);


void
newtonbutton_save (XfcePanelPlugin *plugin,
             NewtonbuttonPlugin    *newtonbutton)
{
  XfceRc *rc;
  gchar  *file;

  g_return_if_fail(newtonbutton != NULL);
  g_return_if_fail(XFCE_IS_PANEL_PLUGIN(plugin));


  /* get the config file location */
  file = xfce_panel_plugin_save_location (plugin, TRUE);

  if (G_UNLIKELY (file == NULL))
    {
       DBG ("Failed to get config file location");
       return;
    }

  /* open the config file, read/write */
  rc = xfce_rc_simple_open (file, FALSE);
  g_free (file);

  if (G_LIKELY (rc != NULL))
    {
      /* save the settings */
      DBG("Saving settings.");
      xfce_rc_write_bool_entry (rc, "DisplayIcon", newtonbutton->display_icon_prop);

      if (newtonbutton->icon_name_prop)
        xfce_rc_write_entry (rc, "IconName", newtonbutton->icon_name_prop);
      else
        xfce_rc_write_entry (rc, "IconName", "");

      if (newtonbutton->label_text_prop)
        xfce_rc_write_entry (rc, "LabelText", newtonbutton->label_text_prop);
      else
        xfce_rc_write_entry (rc, "LabelText", "");

      /* close the rc file */
      xfce_rc_close (rc);
    }
  else
    {
      DBG("Failed to open rc file for writing.");
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


  /* get the plugin config file location */
  file = xfce_panel_plugin_save_location (newtonbutton->plugin, TRUE);

  if (G_LIKELY (file != NULL))
    {
      /* open the config file, readonly */
      rc = xfce_rc_simple_open (file, TRUE);
      g_free (file);

      if (G_LIKELY (rc != NULL))
        {
          /* read the settings */
          newtonbutton->display_icon_prop = xfce_rc_read_bool_entry (rc, "DisplayIcon", DEFAULT_DISPLAY_ICON);

          value = xfce_rc_read_entry (rc, "IconName", DEFAULT_ICON_NAME);
          g_free(newtonbutton->icon_name_prop);
          newtonbutton->icon_name_prop = g_strdup (value);

          value = xfce_rc_read_entry (rc, "LabelText", DEFAULT_LABEL_TEXT);
          g_free(newtonbutton->label_text_prop);
          newtonbutton->label_text_prop = g_strdup (value);
          
          /* cleanup */
          xfce_rc_close (rc);
          return;
        }
    }

  /* something went wrong, apply default values */
  DBG ("Applying default settings");

  newtonbutton->display_icon_prop = DEFAULT_DISPLAY_ICON;
  g_free(newtonbutton->icon_name_prop);
  newtonbutton->icon_name_prop = g_strdup (DEFAULT_ICON_NAME);
  g_free(newtonbutton->label_text_prop);
  newtonbutton->label_text_prop = g_strdup (DEFAULT_LABEL_TEXT);
}


/* Function to update the button's appearance based on current settings */
static void
newtonbutton_update_display (NewtonbuttonPlugin *newtonbutton)
{
    GtkIconTheme *icon_theme = NULL; // Initialize to NULL
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

        // Get icon theme only if needed
        icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (newtonbutton->plugin)));
        panel_icon_size = xfce_panel_plugin_get_icon_size (newtonbutton->plugin);
        
        // SEED:HandleCustomFileIconPath - Add logic here to check if icon_name_prop is a file path vs themed icon name
        if (newtonbutton->icon_name_prop && strlen(newtonbutton->icon_name_prop) > 0) {
            // Attempt to load as a themed icon first
            if (gtk_icon_theme_has_icon(icon_theme, newtonbutton->icon_name_prop)) {
                 gtk_image_set_from_icon_name (GTK_IMAGE (newtonbutton->icon_image),
                                          newtonbutton->icon_name_prop,
                                          GTK_ICON_SIZE_BUTTON);
                 gtk_image_set_pixel_size(GTK_IMAGE(newtonbutton->icon_image), panel_icon_size);
            } else if (g_file_test(newtonbutton->icon_name_prop, G_FILE_TEST_IS_REGULAR)) {
                // If not a themed icon, try as a file path
                GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size(newtonbutton->icon_name_prop, panel_icon_size, panel_icon_size, NULL);
                if (pixbuf) {
                    gtk_image_set_from_pixbuf(GTK_IMAGE(newtonbutton->icon_image), pixbuf);
                    g_object_unref(pixbuf);
                } else {
                    // Fallback if file loading fails
                    gtk_image_set_from_icon_name (GTK_IMAGE (newtonbutton->icon_image), "image-missing", GTK_ICON_SIZE_BUTTON);
                    gtk_image_set_pixel_size(GTK_IMAGE(newtonbutton->icon_image), panel_icon_size);
                }
            } else {
                 // Fallback if neither themed nor valid file
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

  newtonbutton->button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (newtonbutton->button), GTK_RELIEF_NONE);
  gtk_widget_show (newtonbutton->button);
  gtk_container_add (GTK_CONTAINER (newtonbutton->event_box), newtonbutton->button);

  newtonbutton->button_box = gtk_box_new (orientation, 0);
  gtk_widget_show (newtonbutton->button_box);
  gtk_container_add (GTK_CONTAINER (newtonbutton->button), newtonbutton->button_box);

  newtonbutton->icon_image = gtk_image_new ();
  gtk_box_pack_start (GTK_BOX (newtonbutton->button_box), newtonbutton->icon_image, FALSE, FALSE, 0);

  newtonbutton->label_widget = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (newtonbutton->button_box), newtonbutton->label_widget, TRUE, TRUE, 0);
  
  newtonbutton_update_display (newtonbutton);

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
  xfce_panel_plugin_menu_show_configure (plugin);
  g_signal_connect (G_OBJECT (plugin), "configure-plugin", G_CALLBACK (newtonbutton_configure), newtonbutton);
  xfce_panel_plugin_menu_show_about (plugin);
  g_signal_connect (G_OBJECT (plugin), "about", G_CALLBACK (newtonbutton_about), NULL);
}