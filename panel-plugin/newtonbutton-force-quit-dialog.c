#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <libwnck/libwnck.h>
#include <signal.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include "newtonbutton.h"
#include "newtonbutton-force-quit-dialog.h"

enum
{
    COL_ICON,
    COL_APP_NAME,
    COL_PID,
    NUM_COLS
};

typedef struct {
    GtkDialog *dialog;
    GtkTreeView *tree_view;
    GtkListStore *list_store;
    GtkButton *force_quit_button;
    NewtonbuttonPlugin *plugin_data;
    WnckScreen *screen;
    guint refresh_id;
} ForceQuitDialogData;

static void populate_app_list(ForceQuitDialogData *data);
static void on_fq_dialog_force_quit_button_clicked(GtkButton *button, gpointer user_data);
static void on_fq_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data);
static void on_fq_dialog_app_selection_changed(GtkTreeSelection *selection, gpointer user_data);
static gboolean on_refresh_app_list(gpointer user_data);

static void
force_quit_process(pid_t pid)
{
    if (pid > 0) {
        kill(pid, SIGKILL);
    }
}

static void
confirm_force_quit_response_cb(GtkDialog *confirm_dialog, gint response_id, gpointer user_data)
{
    if (response_id == GTK_RESPONSE_YES) {
        pid_t pid = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(confirm_dialog), "pid_to_kill"));
        force_quit_process(pid);
    }
    gtk_widget_destroy(GTK_WIDGET(confirm_dialog));
}

static void
confirm_force_quit(GtkWindow *parent, const gchar *app_name, pid_t pid, ForceQuitDialogData *fq_data)
{
    GtkWidget *dialog;
    gchar *message;
    
    message = g_strdup_printf(_("Are you sure you want to force quit \"%s\"?"), app_name);

    dialog = gtk_message_dialog_new(parent,
                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_QUESTION,
                                  GTK_BUTTONS_NONE,
                                  "%s", message);
    g_free(message);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), 
                                           _("You will lose any unsaved changes."));

    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                         _("Force _Quit"), GTK_RESPONSE_YES,
                         NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
    
    g_object_set_data(G_OBJECT(dialog), "pid_to_kill", GINT_TO_POINTER(pid));
    g_signal_connect(dialog, "response", G_CALLBACK(confirm_force_quit_response_cb), NULL);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_widget_show_all(dialog);
}

static void
add_window_to_list(ForceQuitDialogData *data, WnckWindow *window)
{
    GtkTreeIter iter;
    GdkPixbuf *icon = NULL, *scaled_icon = NULL;
    GtkIconTheme *icon_theme;
    const gchar *app_name = NULL;
    WnckApplication *app = NULL;
    pid_t pid = 0;
    
    if (!window || !WNCK_IS_WINDOW(window))
        return;
    
    // Skip windows that shouldn't be shown
    if (wnck_window_is_skip_tasklist(window) ||
        wnck_window_get_window_type(window) == WNCK_WINDOW_DESKTOP ||
        wnck_window_get_window_type(window) == WNCK_WINDOW_DOCK) {
        return;
    }
    
    app = wnck_window_get_application(window);
    if (!app || !WNCK_IS_APPLICATION(app))
        return;
        
    app_name = wnck_application_get_name(app);
    if (!app_name || !*app_name)
        app_name = _("(Unknown Application)");
    
    pid = wnck_application_get_pid(app);
    if (pid <= 0)
        return;  // Skip items with invalid PIDs
    
    // Check if we already have this PID in our list
    GtkTreeIter existing_iter;
    gboolean valid, found = FALSE;
    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(data->list_store), &existing_iter);
    while (valid) {
        pid_t existing_pid;
        gtk_tree_model_get(GTK_TREE_MODEL(data->list_store), &existing_iter, COL_PID, &existing_pid, -1);
        if (existing_pid == pid) {
            found = TRUE;
            break;
        }
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(data->list_store), &existing_iter);
    }
    
    // Skip if already in list
    if (found)
        return;
    
    // Get icon
    icon_theme = gtk_icon_theme_get_default();
    icon = wnck_application_get_icon(app);
    
    if (icon) {
        scaled_icon = gdk_pixbuf_scale_simple(icon, 16, 16, GDK_INTERP_BILINEAR);
    } else {
        // Use fallback icon
        GError *error = NULL;
        scaled_icon = gtk_icon_theme_load_icon(icon_theme, 
                                               "application-x-executable", 
                                               16, 
                                               GTK_ICON_LOOKUP_USE_BUILTIN, 
                                               &error);
        if (error) {
            g_error_free(error);
        }
    }
    
    // Add to list store
    gtk_list_store_append(data->list_store, &iter);
    gtk_list_store_set(data->list_store, &iter,
                     COL_ICON, scaled_icon,
                     COL_APP_NAME, app_name,
                     COL_PID, pid,
                     -1);
    
    if (scaled_icon)
        g_object_unref(scaled_icon);
}

static gboolean
on_refresh_app_list(gpointer user_data)
{
    ForceQuitDialogData *data = (ForceQuitDialogData *)user_data;
    
    if (!data || !data->dialog || !gtk_widget_is_visible(GTK_WIDGET(data->dialog)))
        return G_SOURCE_REMOVE;
    
    populate_app_list(data);
    return G_SOURCE_CONTINUE;
}

static void
populate_app_list(ForceQuitDialogData *data)
{
    GList *windows, *l;
    GtkTreeSelection *selection;
    pid_t selected_pid = 0;
    gboolean had_selection = FALSE;
    
    // Remember the selected app if any
    selection = gtk_tree_view_get_selection(data->tree_view);
    if (gtk_tree_selection_get_selected(selection, NULL, NULL)) {
        GtkTreeModel *model;
        GtkTreeIter iter;
        had_selection = gtk_tree_selection_get_selected(selection, &model, &iter);
        if (had_selection) {
            gtk_tree_model_get(model, &iter, COL_PID, &selected_pid, -1);
        }
    }
    
    // Clear the list
    gtk_list_store_clear(data->list_store);
    
    // Force screen update to get latest data
    if (data->screen) {
        wnck_screen_force_update(data->screen);
        windows = wnck_screen_get_windows(data->screen);
        
        for (l = windows; l != NULL; l = l->next) {
            WnckWindow *window = WNCK_WINDOW(l->data);
            add_window_to_list(data, window);
        }
    }
    
    // Reselect previously selected app if possible
    if (had_selection && selected_pid > 0) {
        GtkTreeIter iter;
        gboolean valid;
        
        valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(data->list_store), &iter);
        while (valid) {
            pid_t pid;
            gtk_tree_model_get(GTK_TREE_MODEL(data->list_store), &iter, COL_PID, &pid, -1);
            if (pid == selected_pid) {
                gtk_tree_selection_select_iter(selection, &iter);
                break;
            }
            valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(data->list_store), &iter);
        }
    }
}

static void
on_fq_dialog_app_selection_changed(GtkTreeSelection *selection, gpointer user_data)
{
    ForceQuitDialogData *data = (ForceQuitDialogData*)user_data;
    gboolean something_selected;
    
    something_selected = gtk_tree_selection_get_selected(selection, NULL, NULL);
    gtk_widget_set_sensitive(GTK_WIDGET(data->force_quit_button), something_selected);
}

static void
on_fq_dialog_force_quit_button_clicked(GtkButton *button, gpointer user_data)
{
    ForceQuitDialogData *data = (ForceQuitDialogData*)user_data;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    pid_t pid_to_kill = 0;
    gchar *app_name_to_kill = NULL;
    
    selection = gtk_tree_view_get_selection(data->tree_view);
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(model, &iter,
                         COL_PID, &pid_to_kill,
                         COL_APP_NAME, &app_name_to_kill,
                         -1);
        
        if (pid_to_kill > 0 && app_name_to_kill) {
            GtkWindow *parent_window = GTK_WINDOW(data->dialog);
            confirm_force_quit(parent_window, app_name_to_kill, pid_to_kill, data);
        }
        g_free(app_name_to_kill);
    }
}

static void
on_fq_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
    ForceQuitDialogData *data = (ForceQuitDialogData*)user_data;
    
    if (!data)
        return;
    
    // If Apply (Force Quit) clicked, handle it and return
    if (response_id == GTK_RESPONSE_APPLY) {
        on_fq_dialog_force_quit_button_clicked(data->force_quit_button, data);
        return;
    }
    
    // For all other responses, clean up and destroy dialog
    if (data->refresh_id > 0) {
        g_source_remove(data->refresh_id);
        data->refresh_id = 0;
    }
    
    gtk_widget_destroy(GTK_WIDGET(dialog));
    g_slice_free(ForceQuitDialogData, data);
}

void
newtonbutton_show_force_quit_applications_dialog(GtkWindow *parent, NewtonbuttonPlugin *plugin_data)
{
    GtkBuilder *builder;
    GObject *dialog_obj;
    ForceQuitDialogData *data;
    const gchar *ui_resource_path = "/org/xfce/panel/plugins/newtonbutton/newtonbutton-force-quit-dialog.ui";
    
    // Initialize data structure
    data = g_slice_new0(ForceQuitDialogData);
    data->plugin_data = plugin_data;
    
    // Handle WNCK lib with deprecation notice 
    /* Using deprecated API for compatibility, will be updated in future versions */
    data->screen = wnck_screen_get_default();
    
    // Load UI
    builder = gtk_builder_new_from_resource(ui_resource_path);
    if (G_UNLIKELY(builder == NULL)) {
        g_warning("Failed to load UI for Force Quit dialog from resource: %s", ui_resource_path);
        g_slice_free(ForceQuitDialogData, data);
        return;
    }
    
    dialog_obj = gtk_builder_get_object(builder, "force_quit_applications_dialog");
    if (G_UNLIKELY(dialog_obj == NULL || !GTK_IS_DIALOG(dialog_obj))) {
        g_warning("UI loaded, but toplevel widget 'force_quit_applications_dialog' is not a GtkDialog.");
        g_object_unref(builder);
        g_slice_free(ForceQuitDialogData, data);
        return;
    }
    
    data->dialog = GTK_DIALOG(dialog_obj);
    data->tree_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "app_list_treeview"));
    data->force_quit_button = GTK_BUTTON(gtk_builder_get_object(builder, "force_quit_button"));
    
    g_object_unref(builder);
    
    // Setup window
    gtk_window_set_transient_for(GTK_WINDOW(data->dialog), parent);
    
    // Setup tree view columns
    GtkCellRenderer *renderer_pixbuf = gtk_cell_renderer_pixbuf_new();
    GtkCellRenderer *renderer_text = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col;
    
    col = gtk_tree_view_column_new_with_attributes("", renderer_pixbuf, "pixbuf", COL_ICON, NULL);
    gtk_tree_view_append_column(data->tree_view, col);
    
    col = gtk_tree_view_column_new_with_attributes("", renderer_text, "text", COL_APP_NAME, NULL);
    gtk_tree_view_column_set_expand(col, TRUE);
    gtk_tree_view_append_column(data->tree_view, col);
    
    // Create list store and connect to tree view
    data->list_store = gtk_list_store_new(NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT);
    gtk_tree_view_set_model(data->tree_view, GTK_TREE_MODEL(data->list_store));
    g_object_unref(data->list_store);
    
    // Connect signals
    GtkTreeSelection *selection = gtk_tree_view_get_selection(data->tree_view);
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
    g_signal_connect(selection, "changed", G_CALLBACK(on_fq_dialog_app_selection_changed), data);
    g_signal_connect(data->force_quit_button, "clicked", G_CALLBACK(on_fq_dialog_force_quit_button_clicked), data);
    g_signal_connect(data->dialog, "response", G_CALLBACK(on_fq_dialog_response), data);
    
    // Populate initial app list
    populate_app_list(data);
    
    // Add periodic refresh timer (every 2 seconds)
    data->refresh_id = g_timeout_add_seconds(2, on_refresh_app_list, data);
    
    // Force quit button starts disabled until selection made
    gtk_widget_set_sensitive(GTK_WIDGET(data->force_quit_button), FALSE);
    
    // Show dialog
    gtk_widget_show_all(GTK_WIDGET(data->dialog));
}
