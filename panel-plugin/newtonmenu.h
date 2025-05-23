#ifndef __newtonmenu_H__
#define __newtonmenu_H__

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>

G_BEGIN_DECLS

typedef struct _newtonmenuPlugin newtonmenuPlugin;

struct _newtonmenuPlugin
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

void newtonmenu_save (XfcePanelPlugin *plugin, newtonmenuPlugin *newtonmenu);
void newtonmenu_update_display (newtonmenuPlugin *newtonmenu);

G_END_DECLS

#endif /* !__newtonmenu_H__ */