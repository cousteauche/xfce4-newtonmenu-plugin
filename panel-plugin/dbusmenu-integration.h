#ifndef __DBUSMENU_INTEGRATION_H__
#define __DBUSMENU_INTEGRATION_H__

#include "newtonmenu.h"
#include <libdbusmenu-glib/client.h>
#include <libwnck/libwnck.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

void newtonmenu_fetch_app_menu_enhanced(newtonmenuPlugin *newtonmenu, WnckWindow *window);
gboolean newtonmenu_window_has_app_menu(newtonmenuPlugin *newtonmenu, WnckWindow *window);

G_END_DECLS

#endif