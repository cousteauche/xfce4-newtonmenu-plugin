#ifndef __newtonmenu_FORCE_QUIT_DIALOG_H__
#define __newtonmenu_FORCE_QUIT_DIALOG_H__

#include <gtk/gtk.h>
#include "newtonmenu.h"

G_BEGIN_DECLS

void newtonmenu_show_force_quit_applications_dialog(GtkWindow *parent, newtonmenuPlugin *plugin_data);

G_END_DECLS

#endif /* !__newtonmenu_FORCE_QUIT_DIALOG_H__ */