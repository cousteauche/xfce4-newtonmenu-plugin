#ifndef __NEWTONBUTTON_DIALOGS_H__
#define __NEWTONBUTTON_DIALOGS_H__

#include <gtk/gtk.h>
#include "newtonbutton.h" // Include to use NewtonbuttonPlugin type

G_BEGIN_DECLS

void newtonbutton_configure (XfcePanelPlugin *plugin, NewtonbuttonPlugin *newtonbutton);
void newtonbutton_about (XfcePanelPlugin *plugin);
void newtonbutton_show_force_quit_confirmation (GtkWindow *parent);

G_END_DECLS

#endif /* !__NEWTONBUTTON_DIALOGS_H__ */