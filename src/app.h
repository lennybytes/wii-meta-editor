/*
 * Wii Meta Editor
 *
 * Shared application state and module interfaces.
 *
 * This editor lets you open a Wii Homebrew Channel meta.xml, edit the
 * important tags (name, version, release_date, coder, short/long
 * description, ahb_access) and save it back as clean, formatted XML.
 */

#ifndef WII_META_EDITOR_APP_H
#define WII_META_EDITOR_APP_H

#include <gtk/gtk.h>
#include <libxml/parser.h>

#define APP_ID      "io.homebrew.WiiMetaEditor"
#define APP_NAME    "Wii Meta Editor"
#define APP_ICON    APP_ID

/* The editable fields, as exposed by the GUI and consumed by the XML layer. */
typedef struct {
    char *app_name;
    char *version;
    char *coder;
    char *release_date;
    char *short_description;
    char *long_description;
    int   ahb_access; /* 1 to write an <ahb_access/> element */
} MetaData;

/* Central application state shared by the GUI and controller layers. */
typedef struct {
    GtkWidget *window;
    GtkWidget *name_entry;
    GtkWidget *version_entry;
    GtkWidget *coder_entry;
    GtkWidget *date_entry;
    GtkWidget *short_desc_entry;
    GtkWidget *long_desc_view;
    GtkWidget *ahb_check;
    GtkWidget *statusbar_label;
    gchar     *current_file;
} App;

/* gui.c */
int  gui_init(int *argc, char ***argv);
void gui_build(App *app);
void gui_run(void);
void gui_quit(void);
void gui_show_message(App *app, GtkMessageType type, const char *fmt, ...)
        G_GNUC_PRINTF(3, 4);
void gui_set_status(App *app, const char *fmt, ...) G_GNUC_PRINTF(2, 3);
void gui_populate(App *app, const MetaData *meta);
void gui_collect(App *app, MetaData *meta);
void gui_clear(App *app);

/* app.c: controller glue between the GUI and the XML layer. */
void  app_open_dialog(App *app);
void  app_save_current(App *app);
void  app_save_as(App *app);
gboolean app_load_file(App *app, const char *path, GError **err);

/* meta.c: pure XML reading/writing (no GUI dependencies). */
MetaData *meta_load_from_path(const char *path, gboolean *was_empty, GError **err);
gboolean  meta_save_to_path(const MetaData *meta, const char *path, GError **err);
MetaData *meta_new(void);
void      meta_free(MetaData *meta);
gchar    *meta_normalize_date(const char *in);
gchar    *meta_pretty_date(const char *xmldate);

#endif /* WII_META_EDITOR_APP_H */
