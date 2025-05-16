/*
 * Newton Button Plugin - Header File
 * Contains type definitions and public function declarations for the plugin module.
 */
#ifndef __NEWTONBUTTON_H__
#define __NEWTONBUTTON_H__

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>

G_BEGIN_DECLS

typedef struct _NewtonbuttonPlugin NewtonbuttonPlugin;

struct _NewtonbuttonPlugin
{
    XfcePanelPlugin *plugin;

    GtkWidget       *event_box;
    GtkWidget       *button;
    GtkWidget       *button_box;
    GtkWidget       *icon_image;
    GtkWidget       *label_widget;

    gboolean         display_icon_prop;
    gchar           *icon_name_prop;
    gchar           *label_text_prop;

    gboolean         confirm_logout_prop;
    gboolean         confirm_restart_prop;
    gboolean         confirm_shutdown_prop;
    gboolean         confirm_force_quit_prop;

    GtkWidget       *main_menu;
};

void newtonbutton_save (XfcePanelPlugin *plugin, NewtonbuttonPlugin *newtonbutton);
void newtonbutton_update_display (NewtonbuttonPlugin *newtonbutton);
void newtonbutton_configure (XfcePanelPlugin *plugin, NewtonbuttonPlugin *newtonbutton); // Declared in newtonbutton-dialogs.h as well
void newtonbutton_about (XfcePanelPlugin *plugin); // Declared in newtonbutton-dialogs.h as well

G_END_DECLS

#endif /* !__NEWTONBUTTON_H__ */