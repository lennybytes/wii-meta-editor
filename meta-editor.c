/* Wii Meta Editor - GUI editor for the meta.xml of the Wii Homebrew Channel
 * GTK3 + libxml2
 *
 * Lets you open a meta.xml, edit the important tags (name, version,
 * release_date, coder, short/long description, ahb_access) and save it
 * back as clean, properly formatted XML.
 *
 * The GUI uses a plain gtk_window + gtk_main() (no GtkApplication), so
 * it starts reliably on every environment, including from the GNOME app
 * menu. Optional file to open is taken from argv[1].
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define APP_ICON "io.homebrew.WiiMetaEditor"

typedef struct {
    GtkWidget *name_entry;
    GtkWidget *version_entry;
    GtkWidget *coder_entry;
    GtkWidget *date_entry;
    GtkWidget *short_desc_entry;
    GtkWidget *long_desc_view;
    GtkWidget *ahb_check;
    GtkWidget *statusbar_label;
    gchar *current_file;
} AppWidgets;

static AppWidgets widgets;
static GtkWidget *main_window = NULL;

/* ---- Helpers ---- */

static xmlNodePtr find_child(xmlNodePtr parent, const char *name)
{
    for (xmlNodePtr n = parent->children; n; n = n->next)
        if (n->type == XML_ELEMENT_NODE && xmlStrcmp(n->name, (xmlChar *)name) == 0)
            return n;
    return NULL;
}

static gchar *node_text(xmlNodePtr node)
{
    xmlChar *s = xmlNodeGetContent(node);
    gchar *out = s ? g_strdup((const char *)s) : g_strdup("");
    if (s) xmlFree(s);
    g_strstrip(out);
    return out;
}

static void show_message(GtkMessageType type, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    gchar *msg = g_strdup_vprintf(fmt, ap);
    va_end(ap);

    GtkWidget *md = gtk_message_dialog_new(
        GTK_WINDOW(main_window), GTK_DIALOG_MODAL, type,
        GTK_BUTTONS_OK, "%s", msg);
    gtk_dialog_run(GTK_DIALOG(md));
    gtk_widget_destroy(md);
    g_free(msg);
}

/* ---- Date handling ---- */

/* Accepted input forms:
 *   20231209000000  (Wii format)
 *   2023-12-09
 *   09.12.2023
 *   09.12.23
 */
static gchar *normalize_date(const char *in)
{
    int y = 0, m = 0, d = 0;
    if (sscanf(in, "%4d%2d%2d", &y, &m, &d) >= 3 && y >= 2001)
        return g_strdup_printf("%04d%02d%02d000000", y, m, d);
    if (sscanf(in, "%d-%d-%d", &y, &m, &d) == 3 && y >= 2001)
        return g_strdup_printf("%04d%02d%02d000000", y, m, d);
    if (sscanf(in, "%d.%d.%d", &d, &m, &y) == 3) {
        if (y < 100) y += 2000;
        if (y >= 2001)
            return g_strdup_printf("%04d%02d%02d000000", y, m, d);
    }
    return NULL;
}

static gchar *pretty_date(const char *xmldate)
{
    int y = 0, m = 0, d = 0;
    if (sscanf(xmldate, "%4d%2d%2d", &y, &m, &d) >= 3 && y >= 2001)
        return g_strdup_printf("%04d-%02d-%02d", y, m, d);
    return g_strdup(xmldate);
}

static gboolean file_is_empty(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return TRUE; /* cannot open -> treat as empty */
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fclose(f);
    return sz <= 0;
}

/* ---- Writing ---- */

static gboolean write_meta_xml(const char *path, GError **err)
{
    const char *name     = gtk_entry_get_text(GTK_ENTRY(widgets.name_entry));
    const char *version  = gtk_entry_get_text(GTK_ENTRY(widgets.version_entry));
    const char *coder    = gtk_entry_get_text(GTK_ENTRY(widgets.coder_entry));
    const char *date     = gtk_entry_get_text(GTK_ENTRY(widgets.date_entry));

    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets.long_desc_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buf, &start, &end);
    gchar *long_desc = gtk_text_buffer_get_text(buf, &start, &end, FALSE);

    const char *short_desc = gtk_entry_get_text(GTK_ENTRY(widgets.short_desc_entry));
    gboolean ahb = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets.ahb_check));

    gchar *norm_date = normalize_date(date);
    if (!norm_date) {
        g_set_error(err, G_IO_ERROR, G_IO_ERROR_FAILED,
                    "Invalid date: '%s'.\nExpected: YYYY-MM-DD, DD.MM.YYYY or YYYYMMDDHHMMSS.", date);
        g_free(long_desc);
        return FALSE;
    }

    xmlDocPtr doc = xmlNewDoc((xmlChar *)"1.0");
    xmlNodePtr app = xmlNewNode(NULL, (xmlChar *)"app");
    xmlSetProp(app, (xmlChar *)"version", (xmlChar *)"1");
    xmlDocSetRootElement(doc, app);

    xmlNewTextChild(app, NULL, (xmlChar *)"name", (xmlChar *)name);
    xmlNewTextChild(app, NULL, (xmlChar *)"version", (xmlChar *)version);
    xmlNewTextChild(app, NULL, (xmlChar *)"release_date", (xmlChar *)norm_date);
    xmlNewTextChild(app, NULL, (xmlChar *)"coder", (xmlChar *)coder);
    xmlNewTextChild(app, NULL, (xmlChar *)"short_description", (xmlChar *)short_desc);

    xmlNodePtr ld = xmlNewChild(app, NULL, (xmlChar *)"long_description", NULL);
    xmlNodeSetContent(ld, (xmlChar *)long_desc);

    if (ahb)
        xmlNewTextChild(app, NULL, (xmlChar *)"ahb_access", (xmlChar *)"");

    int wrote = xmlSaveFormatFileEnc(path, doc, "UTF-8", 1);
    xmlFreeDoc(doc);
    g_free(norm_date);
    g_free(long_desc);

    if (wrote < 0) {
        g_set_error(err, G_IO_ERROR, G_IO_ERROR_FAILED, "Could not write the file.");
        return FALSE;
    }
    return TRUE;
}

/* ---- Loading ---- */

static void clear_form(void)
{
    gtk_entry_set_text(GTK_ENTRY(widgets.name_entry), "");
    gtk_entry_set_text(GTK_ENTRY(widgets.version_entry), "");
    gtk_entry_set_text(GTK_ENTRY(widgets.coder_entry), "");
    gtk_entry_set_text(GTK_ENTRY(widgets.date_entry), "");
    gtk_entry_set_text(GTK_ENTRY(widgets.short_desc_entry), "");
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets.long_desc_view));
    gtk_text_buffer_set_text(buf, "", -1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.ahb_check), FALSE);
}

static gboolean load_meta_xml(const char *path, GError **err)
{
    /* Empty or unreadable files are offered as a fresh blank template
     * instead of failing, so the user can build a meta.xml from scratch. */
    if (file_is_empty(path)) {
        clear_form();
        gchar *msg = g_strdup_printf("Blank template loaded: %s", path);
        gtk_label_set_text(GTK_LABEL(widgets.statusbar_label), msg);
        g_free(msg);
        g_free(widgets.current_file);
        widgets.current_file = g_strdup(path);
        return TRUE;
    }

    xmlDocPtr doc = xmlReadFile(path, NULL, XML_PARSE_NONET | XML_PARSE_NOBLANKS);
    if (!doc) {
        g_set_error(err, G_IO_ERROR, G_IO_ERROR_FAILED,
                    "Could not read '%s' as XML.\nIt may be empty or corrupted.", path);
        return FALSE;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    xmlNodePtr app = NULL;
    if (root && xmlStrcmp(root->name, (xmlChar *)"app") == 0)
        app = root;

    gchar *name     = app ? node_text(find_child(app, "name")) : g_strdup("");
    gchar *version  = app ? node_text(find_child(app, "version")) : g_strdup("");
    gchar *cdn      = app ? node_text(find_child(app, "coder")) : g_strdup("");
    gchar *rawdate  = app ? node_text(find_child(app, "release_date")) : g_strdup("");
    gchar *shdesc   = app ? node_text(find_child(app, "short_description")) : g_strdup("");
    gchar *londesc  = app ? node_text(find_child(app, "long_description")) : g_strdup("");

    xmlNodePtr ahb = app ? find_child(app, "ahb_access") : NULL;

    gchar *date = pretty_date(rawdate);

    gtk_entry_set_text(GTK_ENTRY(widgets.name_entry), name);
    gtk_entry_set_text(GTK_ENTRY(widgets.version_entry), version);
    gtk_entry_set_text(GTK_ENTRY(widgets.coder_entry), cdn);
    gtk_entry_set_text(GTK_ENTRY(widgets.date_entry), date);
    gtk_entry_set_text(GTK_ENTRY(widgets.short_desc_entry), shdesc);

    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets.long_desc_view));
    gtk_text_buffer_set_text(buf, londesc, -1);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.ahb_check), ahb != NULL);

    gchar *msg = g_strdup_printf("Loaded: %s", path);
    gtk_label_set_text(GTK_LABEL(widgets.statusbar_label), msg);
    g_free(msg);

    g_free(widgets.current_file);
    widgets.current_file = g_strdup(path);

    g_free(name); g_free(version); g_free(cdn); g_free(rawdate);
    g_free(shdesc); g_free(londesc); g_free(date);

    xmlFreeDoc(doc);
    return TRUE;
}

/* ---- Callbacks ---- */

static void on_open_clicked(GtkWidget *w, gpointer data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new(
        "Select meta.xml", GTK_WINDOW(data),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT, NULL);

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "meta.xml (XML files)");
    gtk_file_filter_add_pattern(filter, "meta.xml");
    gtk_file_filter_add_pattern(filter, "*.xml");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filter);
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dlg), filter);

    if (widgets.current_file)
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), widgets.current_file);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        gchar *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
        GError *err = NULL;
        if (!load_meta_xml(path, &err)) {
            if (err) { show_message(GTK_MESSAGE_ERROR, "%s", err->message); g_error_free(err); }
        }
        g_free(path);
    }
    gtk_widget_destroy(dlg);
}

static void on_save_clicked(GtkWidget *w, gpointer data)
{
    if (!widgets.current_file) {
        show_message(GTK_MESSAGE_INFO, "Please open a meta.xml first.");
        return;
    }

    GError *err = NULL;
    if (write_meta_xml(widgets.current_file, &err)) {
        show_message(GTK_MESSAGE_INFO, "meta.xml saved successfully.");
        gchar *msg = g_strdup_printf("Saved: %s", widgets.current_file);
        gtk_label_set_text(GTK_LABEL(widgets.statusbar_label), msg);
        g_free(msg);
    } else {
        if (err) { show_message(GTK_MESSAGE_ERROR, "%s", err->message); g_error_free(err); }
    }
}

static void on_save_as_clicked(GtkWidget *w, gpointer data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new(
        "Save meta.xml as", GTK_WINDOW(data),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_ACCEPT, NULL);

    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dlg), "meta.xml");
    if (widgets.current_file)
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), widgets.current_file);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        gchar *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
        gchar *saved_path = g_strdup(path);
        GError *err = NULL;
        if (!write_meta_xml(saved_path, &err)) {
            if (err) { show_message(GTK_MESSAGE_ERROR, "%s", err->message); g_error_free(err); }
        } else {
            g_free(widgets.current_file);
            widgets.current_file = saved_path;
            gchar *msg = g_strdup_printf("Saved: %s", saved_path);
            gtk_label_set_text(GTK_LABEL(widgets.statusbar_label), msg);
            g_free(msg);
        }
        g_free(path);
    }
    gtk_widget_destroy(dlg);
}

static void on_quit_clicked(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

/* ---- GUI construction ---- */

static GtkWidget *entry_row(GtkWidget *grid, int row, const char *label, const char *placeholder)
{
    GtkWidget *l = gtk_label_new(label);
    gtk_widget_set_halign(l, GTK_ALIGN_START);
    GtkWidget *e = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(e), placeholder);
    gtk_grid_attach(GTK_GRID(grid), l, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), e, 1, row, 1, 1);
    return e;
}

static void build_window(void)
{
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), "Wii Meta Editor");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 580, 640);
    gtk_window_set_icon_name(GTK_WINDOW(main_window), APP_ICON);
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /*** Header bar (modern look) ***/
    GtkWidget *header = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(header), "Wii Meta Editor");
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(header), "Wii Homebrew Channel meta.xml");
    gtk_window_set_titlebar(GTK_WINDOW(main_window), header);

    GtkWidget *btn_open = gtk_button_new_with_label("Open");
    GtkWidget *btn_save = gtk_button_new_with_label("Save");
    GtkWidget *btn_saveas = gtk_button_new_with_label("Save As");
    gtk_widget_set_tooltip_text(btn_open, "Select a meta.xml file");
    gtk_widget_set_tooltip_text(btn_save, "Save to the currently open file");
    gtk_widget_set_tooltip_text(btn_saveas, "Save to a new location");
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), btn_open);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), btn_save);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), btn_saveas);
    g_signal_connect(btn_open, "clicked", G_CALLBACK(on_open_clicked), main_window);
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_clicked), main_window);
    g_signal_connect(btn_saveas, "clicked", G_CALLBACK(on_save_as_clicked), main_window);

    GtkAccelGroup *accel = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(main_window), accel);
    gtk_widget_add_accelerator(btn_open, "clicked", accel, GDK_KEY_o,
                               GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(btn_save, "clicked", accel, GDK_KEY_s,
                               GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(btn_saveas, "clicked", accel, GDK_KEY_s,
                               GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
    gtk_container_add(GTK_CONTAINER(main_window), vbox);

    /* Fields */
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), grid, FALSE, FALSE, 0);

    widgets.name_entry = entry_row(grid, 0, "App Name:", "e.g. CavEX");
    widgets.version_entry = entry_row(grid, 1, "Version:", "e.g. 1.0");
    widgets.coder_entry = entry_row(grid, 2, "Coder:", "Author / Team");
    widgets.date_entry = entry_row(grid, 3, "Release Date:", "YYYY-MM-DD");
    widgets.short_desc_entry = entry_row(grid, 4, "Short Description:", "One line");

    /* ahb_access checkbox */
    widgets.ahb_check = gtk_check_button_new_with_label(
        "ahb_access (direct access to Wii storage / hardware bypass)");
    gtk_box_pack_start(GTK_BOX(vbox), widgets.ahb_check, FALSE, FALSE, 0);

    /* Long description */
    GtkWidget *ldlab = gtk_label_new("Long Description:");
    gtk_widget_set_halign(ldlab, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), ldlab, FALSE, FALSE, 0);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled, -1, 220);
    widgets.long_desc_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(widgets.long_desc_view), GTK_WRAP_WORD);
    gtk_container_add(GTK_CONTAINER(scrolled), widgets.long_desc_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    /* Status bar */
    widgets.statusbar_label = gtk_label_new("No file loaded. Use 'Open' to select a meta.xml.");
    gtk_widget_set_halign(widgets.statusbar_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), widgets.statusbar_label, FALSE, FALSE, 0);

    gtk_widget_show_all(main_window);
}

int main(int argc, char **argv)
{
    xmlInitParser();

    gtk_init(&argc, &argv);

    memset(&widgets, 0, sizeof(widgets));

    build_window();

    /* Optional file to open from the command line / file manager. */
    if (argc >= 2) {
        gchar *path = g_strdup(argv[1]);
        GError *err = NULL;
        if (!load_meta_xml(path, &err)) {
            if (err) { show_message(GTK_MESSAGE_WARNING, "%s", err->message); g_error_free(err); }
        }
        g_free(path);
    }

    gtk_main();

    g_free(widgets.current_file);
    xmlCleanupParser();
    return 0;
}
