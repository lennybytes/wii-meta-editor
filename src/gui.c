/*
 * Wii Meta Editor - GTK3 GUI
 *
 * Builds the window, header bar, form fields and status bar. The GUI is
 * deliberately kept thin: it only renders/collects MetaData values and
 * forwards user actions to the controller layer in app.c.
 */

#include <gdk/gdkkeysyms.h>

#include "app.h"

/* ---- Lifecycle ---------------------------------------------------------- */

int gui_init(int *argc, char ***argv)
{
    return gtk_init_check(argc, argv);
}

void gui_run(void)
{
    gtk_main();
}

void gui_quit(void)
{
    gtk_main_quit();
}

/* ---- Internal helpers --------------------------------------------------- */

static GtkWidget *entry_row(GtkWidget *grid, int row,
                            const char *label, const char *placeholder)
{
    GtkWidget *l = gtk_label_new(label);
    gtk_widget_set_halign(l, GTK_ALIGN_START);
    GtkWidget *e = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(e), placeholder);
    gtk_grid_attach(GTK_GRID(grid), l, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), e, 1, row, 1, 1);
    return e;
}

static GtkTextBuffer *text_buffer(App *app)
{
    return gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->long_desc_view));
}

/* ---- Form <-> data ------------------------------------------------------ */

void gui_populate(App *app, const MetaData *meta)
{
    gtk_entry_set_text(GTK_ENTRY(app->name_entry), meta->app_name);
    gtk_entry_set_text(GTK_ENTRY(app->version_entry), meta->version);
    gtk_entry_set_text(GTK_ENTRY(app->coder_entry), meta->coder);
    gtk_entry_set_text(GTK_ENTRY(app->date_entry), meta->release_date);
    gtk_entry_set_text(GTK_ENTRY(app->short_desc_entry), meta->short_description);
    gtk_text_buffer_set_text(text_buffer(app), meta->long_description, -1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->ahb_check),
                                 meta->ahb_access != 0);
}

void gui_collect(App *app, MetaData *meta)
{
    g_free(meta->app_name);
    g_free(meta->version);
    g_free(meta->coder);
    g_free(meta->release_date);
    g_free(meta->short_description);
    g_free(meta->long_description);

    meta->app_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(app->name_entry)));
    meta->version = g_strdup(gtk_entry_get_text(GTK_ENTRY(app->version_entry)));
    meta->coder = g_strdup(gtk_entry_get_text(GTK_ENTRY(app->coder_entry)));
    meta->release_date = g_strdup(gtk_entry_get_text(GTK_ENTRY(app->date_entry)));
    meta->short_description =
        g_strdup(gtk_entry_get_text(GTK_ENTRY(app->short_desc_entry)));

    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(text_buffer(app), &start, &end);
    meta->long_description =
        gtk_text_buffer_get_text(text_buffer(app), &start, &end, FALSE);

    meta->ahb_access =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(app->ahb_check)) ? 1 : 0;
}

void gui_clear(App *app)
{
    MetaData blank;
    memset(&blank, 0, sizeof(blank));
    gui_populate(app, &blank);
}

/* ---- Feedback ----------------------------------------------------------- */

void gui_set_status(App *app, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    gchar *msg = g_strdup_vprintf(fmt, ap);
    va_end(ap);
    gtk_label_set_text(GTK_LABEL(app->statusbar_label), msg);
    g_free(msg);
}

void gui_show_message(App *app, GtkMessageType type, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    gchar *msg = g_strdup_vprintf(fmt, ap);
    va_end(ap);

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(app->window), GTK_DIALOG_MODAL, type,
        GTK_BUTTONS_OK, "%s", msg);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_free(msg);
}

/* ---- Window construction ------------------------------------------------ */

void gui_build(App *app)
{
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), APP_NAME);
    gtk_window_set_default_size(GTK_WINDOW(app->window), 580, 640);
    gtk_window_set_icon_name(GTK_WINDOW(app->window), APP_ICON);
    g_signal_connect(app->window, "destroy", G_CALLBACK(gui_quit), NULL);

    /* Header bar (modern look). */
    GtkWidget *header = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(header), APP_NAME);
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(header),
                                "Wii Homebrew Channel meta.xml");
    gtk_window_set_titlebar(GTK_WINDOW(app->window), header);

    GtkWidget *btn_open = gtk_button_new_with_label("Open");
    GtkWidget *btn_save = gtk_button_new_with_label("Save");
    GtkWidget *btn_save_as = gtk_button_new_with_label("Save As");
    gtk_widget_set_tooltip_text(btn_open, "Select a meta.xml file");
    gtk_widget_set_tooltip_text(btn_save, "Save to the currently open file");
    gtk_widget_set_tooltip_text(btn_save_as, "Save to a new location");
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), btn_open);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), btn_save);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), btn_save_as);

    GtkAccelGroup *accel = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(app->window), accel);
    gtk_widget_add_accelerator(btn_open, "clicked", accel, GDK_KEY_o,
                               GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(btn_save, "clicked", accel, GDK_KEY_s,
                               GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(btn_save_as, "clicked", accel, GDK_KEY_s,
                               GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);

    g_signal_connect_swapped(btn_open, "clicked", G_CALLBACK(app_open_dialog), app);
    g_signal_connect_swapped(btn_save, "clicked", G_CALLBACK(app_save_current), app);
    g_signal_connect_swapped(btn_save_as, "clicked", G_CALLBACK(app_save_as), app);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
    gtk_container_add(GTK_CONTAINER(app->window), vbox);

    /* Form fields. */
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), grid, FALSE, FALSE, 0);

    app->name_entry = entry_row(grid, 0, "App Name:", "e.g. CavEX");
    app->version_entry = entry_row(grid, 1, "Version:", "e.g. 1.0");
    app->coder_entry = entry_row(grid, 2, "Coder:", "Author / Team");
    app->date_entry = entry_row(grid, 3, "Release Date:", "YYYY-MM-DD");
    app->short_desc_entry = entry_row(grid, 4, "Short Description:", "One line");

    /* ahb_access checkbox. */
    app->ahb_check = gtk_check_button_new_with_label(
        "ahb_access (direct access to Wii storage / hardware bypass)");
    gtk_box_pack_start(GTK_BOX(vbox), app->ahb_check, FALSE, FALSE, 0);

    /* Long description. */
    GtkWidget *ld_label = gtk_label_new("Long Description:");
    gtk_widget_set_halign(ld_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), ld_label, FALSE, FALSE, 0);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled, -1, 220);
    app->long_desc_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(app->long_desc_view), GTK_WRAP_WORD);
    gtk_container_add(GTK_CONTAINER(scrolled), app->long_desc_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    /* Status bar. */
    app->statusbar_label =
        gtk_label_new("No file loaded. Use 'Open' to select a meta.xml.");
    gtk_widget_set_halign(app->statusbar_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), app->statusbar_label, FALSE, FALSE, 0);

    gtk_widget_show_all(app->window);
}
