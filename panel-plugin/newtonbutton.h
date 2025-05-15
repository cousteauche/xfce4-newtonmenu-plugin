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
};

void newtonbutton_save (XfcePanelPlugin *plugin, NewtonbuttonPlugin *newtonbutton);
void newtonbutton_update_display (NewtonbuttonPlugin *newtonbutton);
/* Functions from newtonbutton-dialogs.c, declared here if needed by newtonbutton.c (though usually not)
   or just for completeness if newtonbutton.h is the main header for the plugin module.
   These are typically called via signals from XfcePanelPlugin. */
void newtonbutton_configure (XfcePanelPlugin *plugin, NewtonbuttonPlugin *newtonbutton);
void newtonbutton_about (XfcePanelPlugin *plugin);


G_END_DECLS

#endif /* !__NEWTONBUTTON_H__ */