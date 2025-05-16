#ifndef __NEWTONBUTTON_FORCE_QUIT_DIALOG_H__
#define __NEWTONBUTTON_FORCE_QUIT_DIALOG_H__

#include <gtk/gtk.h>
#include "newtonbutton.h"

G_BEGIN_DECLS

void newtonbutton_show_force_quit_applications_dialog(GtkWindow *parent, NewtonbuttonPlugin *plugin_data);

G_END_DECLS

#endif /* !__NEWTONBUTTON_FORCE_QUIT_DIALOG_H__ */