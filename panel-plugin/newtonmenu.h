#ifndef __NEWTONMENU_H__
#define __NEWTONMENU_H__

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4util/libxfce4util.h>
#include <libwnck/libwnck.h> // Wnck is still included, as MenuWidget might internally use it
#include <gio/gio.h>       // GIO is still included, as MenuWidget will use it

G_BEGIN_DECLS

// Forward declarations for Vala-generated C functions
GtkWidget* appmenu_menu_widget_new(void);
void appmenu_menu_widget_set_compact_mode(GtkWidget* widget, gboolean compact_mode);
void appmenu_menu_widget_set_bold_application_name(GtkWidget* widget, gboolean bold_application_name);

typedef struct _newtonmenuPlugin newtonmenuPlugin;

struct _newtonmenuPlugin
{
    XfcePanelPlugin *plugin;

    GtkWidget       *main_box;
    GtkMenuButton   *newton_button;
    GtkWidget       *newton_button_box;
    GtkWidget       *newton_icon_image;
    GtkWidget       *newton_label_widget;
    GtkWidget       *static_newton_menu;

    GtkWidget       *app_menu_bar_container;
    GtkWidget       *appmenu_widget; // NEW: Holds the Vala-generated Appmenu.MenuWidget

    gboolean        display_icon_prop;
    gchar          *icon_name_prop;
    gchar          *label_text_prop;
    
    gboolean        hide_application_name_prop; // This maps to MenuWidget's "compact-mode"
    gboolean        bold_app_name_prop;       // This maps to MenuWidget's "bold-application-name"
    
    gboolean        confirm_logout_prop;
    gboolean        confirm_restart_prop;
    gboolean        confirm_shutdown_prop;
    gboolean        confirm_force_quit_prop;
    
    GtkMenuButton   *currently_open_button;
};

void newtonmenu_save(XfcePanelPlugin *plugin, newtonmenuPlugin *newtonmenu);
void newtonmenu_update_display(newtonmenuPlugin *newtonmenu);
void newtonmenu_apply_appmenu_properties(newtonmenuPlugin *newtonmenu); // NEW

void newtonmenu_menu_button_toggled(GtkMenuButton *button, newtonmenuPlugin *newtonmenu);
gboolean on_menu_button_enter(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data);

void newtonmenu_configure (XfcePanelPlugin *plugin, newtonmenuPlugin *newtonmenu);
void newtonmenu_about (XfcePanelPlugin *plugin);

G_END_DECLS

#endif /* !__NEWTONMENU_H__ */