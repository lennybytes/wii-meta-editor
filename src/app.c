/*
 * Wii Meta Editor - controller layer
 *
 * Glues the GUI (gui.c) to the XML layer (meta.c): wires up the open /
 * save file dialogs, tracks the current file and reports results to the
 * user through the status bar.
 */

#include "app.h"

/* ---- Loading ------------------------------------------------------------ */

/*
 * Loads the given path into the form. On an empty file a blank template is
 * offered instead of failing, so a fresh meta.xml can be built from scratch.
 */
gboolean app_load_file(App *app, const char *path, GError **err)
{
    gboolean was_empty = FALSE;
    MetaData *meta = meta_load_from_path(path, &was_empty, err);
    if (!meta)
        return FALSE;

    gui_populate(app, meta);

    g_free(app->current_file);
    app->current_file = g_strdup(path);

    if (was_empty)
        gui_set_status(app, "Blank template loaded: %s", path);
    else
        gui_set_status(app, "Loaded: %s", path);

    meta_free(meta);
    return TRUE;
}

/* ---- File dialogs ------------------------------------------------------- */

void app_open_dialog(App *app)
{
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select meta.xml", GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT, NULL);

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "meta.xml (XML files)");
    gtk_file_filter_add_pattern(filter, "meta.xml");
    gtk_file_filter_add_pattern(filter, "*.xml");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (app->current_file)
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), app->current_file);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        GError *err = NULL;
        if (!app_load_file(app, path, &err)) {
            if (err) {
                gui_show_message(app, GTK_MESSAGE_ERROR, "%s", err->message);
                g_error_free(err);
            }
        }
        g_free(path);
    }
    gtk_widget_destroy(dialog);
}

/* ---- Saving ------------------------------------------------------------- */

static void save_to(App *app, const char *path)
{
    MetaData meta;
    memset(&meta, 0, sizeof(meta));
    gui_collect(app, &meta);

    GError *err = NULL;
    if (meta_save_to_path(&meta, path, &err)) {
        gui_set_status(app, "Saved: %s", path);
    } else if (err) {
        gui_show_message(app, GTK_MESSAGE_ERROR, "%s", err->message);
        g_error_free(err);
    }

    meta_free(&meta);
}

void app_save_current(App *app)
{
    if (!app->current_file) {
        gui_show_message(app, GTK_MESSAGE_INFO, "Please open a meta.xml first.");
        return;
    }
    save_to(app, app->current_file);
}

void app_save_as(App *app)
{
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Save meta.xml as", GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_ACCEPT, NULL);

    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "meta.xml");
    if (app->current_file)
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), app->current_file);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        save_to(app, path);

        g_free(app->current_file);
        app->current_file = g_strdup(path);
        g_free(path);
    }
    gtk_widget_destroy(dialog);
}

/* ---- Templates ---------------------------------------------------------- */

void app_new_template(App *app)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "New Template", GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Create", GTK_RESPONSE_ACCEPT, NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *label = gtk_label_new("Select a template:");
    gtk_container_add(GTK_CONTAINER(content), label);

    GtkWidget *combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Basic Homebrew App");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Game with ahb_access");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Utility Tool");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    gtk_container_add(GTK_CONTAINER(content), combo);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gint active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
        gui_apply_template(app, active);
    }
    gtk_widget_destroy(dialog);
}

/* ---- Validation --------------------------------------------------------- */

void app_validate(App *app)
{
    MetaData meta;
    memset(&meta, 0, sizeof(meta));
    gui_collect(app, &meta);

    GError *err = NULL;
    if (meta_validate(&meta, &err)) {
        gui_show_message(app, GTK_MESSAGE_INFO, "All fields are valid!");
    } else {
        gui_show_message(app, GTK_MESSAGE_WARNING, "%s", err->message);
        g_error_free(err);
    }

    meta_free(&meta);
}
