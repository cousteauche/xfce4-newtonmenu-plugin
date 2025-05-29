#include <gtk/gtk.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libwnck/libwnck.h>
#include <dbusmenu-glib/dbusmenu-glib.h>

#define NEWTONMENU_PLUGIN_PRIV(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), newtonmenu_plugin_get_type(), NewtonMenuPluginPriv))

/* Plugin instance structure */
typedef struct {
    XfcePanelPlugin       parent_instance;
    GtkToggleButton      *apple_button;    // The Apple-style button
    GtkWidget            *menu_bar;        // Menu bar for global application menu
    WnckScreen           *wnck_screen;     // Wnck screen for tracking active window
    DbusmenuClient       *dbus_client;     // DBus menu client for current app
} NewtonMenuPlugin;

typedef struct {
    XfcePanelPluginClass parent_class;
} NewtonMenuPluginClass;

static void newtonmenu_plugin_construct(XfcePanelPlugin *plugin);
static void newtonmenu_plugin_destruct(XfcePanelPlugin *plugin);
static void on_active_window_changed(WnckScreen *screen, WnckWindow *previous, WnckWindow *active, NewtonMenuPlugin *plugin);

/* Define GObject type */
G_DEFINE_TYPE(NewtonMenuPlugin, newtonmenu_plugin, XFCE_TYPE_PANEL_PLUGIN)

static void newtonmenu_plugin_class_init(NewtonMenuPluginClass *klass)
{
    XfcePanelPluginClass *plugin_class = XFCE_PANEL_PLUGIN_CLASS(klass);
    plugin_class->construct = newtonmenu_plugin_construct;
    plugin_class->free_data = newtonmenu_plugin_destruct;
    /* Register private struct */
    g_type_class_add_private(klass, sizeof(NewtonMenuPlugin));
}

static void newtonmenu_plugin_init(NewtonMenuPlugin *self)
{
    /* Initialization is done in construct */
}

/* Helper to build the session/system menu for the Apple button */
static GtkWidget* create_session_menu(NewtonMenuPlugin *plugin)
{
    GtkWidget *menu = gtk_menu_new();
    // (Code to populate the menu with About, Settings, Sleep, Restart, Shutdown, etc.)
    // ... (omitted for brevity, assume existing implementation populates the menu)
    return menu;
}

/* Update the global menu bar when the active window changes */
static void on_active_window_changed(WnckScreen *screen,
                                     WnckWindow *previous_active,
                                     WnckWindow *active,
                                     NewtonMenuPlugin *plugin)
{
    // Remove any existing menu items from the menu bar
    GList *children = gtk_container_get_children(GTK_CONTAINER(plugin->menu_bar));
    for (GList *l = children; l != NULL; l = l->next) {
        gtk_container_remove(GTK_CONTAINER(plugin->menu_bar), GTK_WIDGET(l->data));
    }
    g_list_free(children);

    // If no active window or no global menu available, nothing to do
    if (active == NULL) {
        return;
    }

    // Get X11 window ID of active window
    guint32 xid = wnck_window_get_xid(active);
    if (xid == 0) {
        return;
    }

    // Call the AppMenu registrar to get menu for this window
    GError *error = NULL;
    GDBusConnection *bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (!bus || error != NULL) {
        g_clear_error(&error);
        return;
    }
    GVariant *result = g_dbus_connection_call_sync(
        bus,
        "com.canonical.AppMenu.Registrar",                    // name
        "/com/canonical/AppMenu/Registrar",                   // object path
        "com.canonical.AppMenu.Registrar",                    // interface
        "GetMenuForWindow",                                   // method
        g_variant_new("(u)", xid),                            // parameters
        G_VARIANT_TYPE("(so)"),                               // expected return type: (s,o)
        G_DBUS_CALL_FLAGS_NONE,
        -1, NULL, &error);
    if (!result || error != NULL) {
        if (error) g_clear_error(&error);
        return;
    }

    const gchar *bus_name = NULL;
    const gchar *object_path = NULL;
    g_variant_get(result, "(&s&o)", &bus_name, &object_path);
    if (bus_name == NULL || bus_name[0] == '\0') {
        // No global menu available for this window
        g_variant_unref(result);
        return;
    }

    // Clean up previous DbusmenuClient if any
    if (plugin->dbus_client) {
        g_object_unref(plugin->dbus_client);
        plugin->dbus_client = NULL;
    }

    // Create a new DBusMenu client for the application’s menu
    plugin->dbus_client = dbusmenu_client_new(bus_name, object_path);
    if (plugin->dbus_client == NULL) {
        g_variant_unref(result);
        return;
    }

    // Get the root menuitem and iterate top-level children (menu bar items)
    DbusmenuMenuitem *root = dbusmenu_client_get_root(plugin->dbus_client);
    GList *items = dbusmenu_menuitem_get_children(root);
    for (GList *li = items; li != NULL; li = li->next) {
        DbusmenuMenuitem *item = DBUSMENU_MENUITEM(li->data);
        const gchar *label = dbusmenu_menuitem_property_get(item, "label");
        if (!label) label = "";  // ensure non-null

        // Create a menu item for the menu bar
        GtkWidget *menu_item = gtk_menu_item_new_with_label(label);
        gtk_widget_set_halign(menu_item, GTK_ALIGN_START);

        // If this item has submenu children, create a submenu for it
        if (dbusmenu_menuitem_get_children(item) != NULL) {
            // Use libdbusmenu to create a GtkMenu for the submenu
            // (This will fetch the submenu entries on demand)
            GtkWidget *submenu = dbusmenu_gtkmenu_new(bus_name, object_path);
            // Associate the submenu with our specific menu item ID
            // (Filter to this item's submenu by ID)
            guint32 item_id = dbusmenu_menuitem_get_id(item);
            dbusmenu_menuitem_property_set(item, "visible", "true");  // ensure visible
            dbusmenu_menuitem_property_set(item, "children-display", "submenu");
            // Attach the submenu to the menu item
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), submenu);
        }

        // Add the menu item to the menu bar and show it
        gtk_menu_shell_append(GTK_MENU_SHELL(plugin->menu_bar), menu_item);
        gtk_widget_show(menu_item);
    }
    // Show the menu bar (if not already visible)
    gtk_widget_show(plugin->menu_bar);

    g_variant_unref(result);
}

/* Plugin construct function: set up UI and callbacks */
static void newtonmenu_plugin_construct(XfcePanelPlugin *panel_plugin)
{
    NewtonMenuPlugin *plugin = NEWTONMENU_PLUGIN(panel_plugin);

    // Create horizontal box to hold Apple button and global menu bar
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    /* Initialize the Apple-style menu button */
    plugin->apple_button = GTK_TOGGLE_BUTTON(xfce_panel_create_toggle_button());
    // Set icon or label on the button based on user preference (icon by default)
    GtkWidget *image = gtk_image_new_from_icon_name("distributor-logo", XFCE_PANEL_IMAGE_SIZE_LARGE);
    gtk_container_add(GTK_CONTAINER(plugin->apple_button), image);
    gtk_widget_show(image);

    // Create the static system menu for Apple button and attach to the toggle button
    GtkWidget *session_menu = create_session_menu(plugin);
    xfce_panel_plugin_add_action_widget(panel_plugin, GTK_WIDGET(plugin->apple_button));
    gtk_widget_show(GTK_WIDGET(plugin->apple_button));
    gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(plugin->apple_button), session_menu);  // if using a MenuToolButton
    // If using a ToggleButton, connect its "toggled" to show the menu manually:
    // g_signal_connect(plugin->apple_button, "toggled", G_CALLBACK(toggle_menu_cb), session_menu);

    // Pack the Apple button at the start of hbox
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(plugin->apple_button), FALSE, FALSE, 0);

    /* Initialize the global menu bar widget */
    plugin->menu_bar = gtk_menu_bar_new();
    gtk_menu_bar_set_pack_direction(GTK_MENU_BAR(plugin->menu_bar), GTK_PACK_DIRECTION_LTR);
    gtk_menu_bar_set_child_pack_direction(GTK_MENU_BAR(plugin->menu_bar), GTK_PACK_DIRECTION_LTR);
    gtk_widget_set_hexpand(plugin->menu_bar, TRUE);
    gtk_widget_show(plugin->menu_bar);

    // Pack the menu bar next to the Apple button, expanding to fill remaining space
    gtk_box_pack_start(GTK_BOX(hbox), plugin->menu_bar, TRUE, TRUE, 0);

    gtk_widget_show(hbox);
    // Add the hbox to the panel plugin container
    gtk_container_add(GTK_CONTAINER(panel_plugin), hbox);

    // Allow the plugin to expand (so the menu bar can grow)
    xfce_panel_plugin_set_expand(panel_plugin, TRUE);

    // Track active window changes using libwnck
    plugin->wnck_screen = wnck_screen_get_default();
    if (plugin->wnck_screen) {
        // Initial population of menu (for current active window if any)
        WnckWindow *win = wnck_screen_get_active_window(plugin->wnck_screen);
        on_active_window_changed(plugin->wnck_screen, NULL, win, plugin);
        // Connect signal for future changes
        g_signal_connect(plugin->wnck_screen, "active-window-changed",
                         G_CALLBACK(on_active_window_changed), plugin);
    }
}

/* Plugin destructor to clean up */
static void newtonmenu_plugin_destruct(XfcePanelPlugin *panel_plugin)
{
    NewtonMenuPlugin *plugin = NEWTONMENU_PLUGIN(panel_plugin);
    if (plugin->dbus_client) {
        g_object_unref(plugin->dbus_client);
        plugin->dbus_client = NULL;
    }
    if (plugin->wnck_screen) {
        g_signal_handlers_disconnect_by_func(plugin->wnck_screen,
                                             G_CALLBACK(on_active_window_changed), plugin);
        plugin->wnck_screen = NULL;
    }
}

/* Entry point for the panel plugin (external mode) */
XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL(newtonmenu_plugin_construct);
