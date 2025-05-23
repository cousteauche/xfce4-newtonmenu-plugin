#ifndef __newtonmenu_DIALOGS_H__
#define __newtonmenu_DIALOGS_H__

#include <gtk/gtk.h>
#include "newtonmenu.h" // Include to use newtonmenuPlugin type

G_BEGIN_DECLS

void newtonmenu_configure (XfcePanelPlugin *plugin, newtonmenuPlugin *newtonmenu);
void newtonmenu_about (XfcePanelPlugin *plugin);
void newtonmenu_show_generic_confirmation (GtkWindow *parent, const gchar *action_name_translated, const gchar *action_verb_translated, const gchar *command_to_run);

G_END_DECLS

#endif /* !__newtonmenu_DIALOGS_H__ */