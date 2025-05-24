#ifndef __DBUSMENU_INTEGRATION_H__
#define __DBUSMENU_INTEGRATION_H__

#include "newtonmenu.h"
#include <libdbusmenu-glib/client.h>

G_BEGIN_DECLS

/* Enhanced functions that can replace the basic ones in newtonmenu.c */
void newtonmenu_fetch_app_menu_enhanced(newtonmenuPlugin *newtonmenu, WnckWindow *window);
void newtonmenu_update_combined_menu_enhanced(newtonmenuPlugin *newtonmenu);
gboolean newtonmenu_window_has_app_menu(newtonmenuPlugin *newtonmenu, WnckWindow *window);

G_END_DECLS

#endif /* !__DBUSMENU_INTEGRATION_H__ */
