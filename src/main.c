/*
 * Wii Meta Editor - entry point
 *
 * Initialises GTK and libxml2, builds the window and optionally opens a
 * file passed on the command line (e.g. by the file manager or an app
 * launcher), then runs the main loop.
 */

#include "app.h"

int main(int argc, char **argv)
{
    xmlInitParser();

    if (!gui_init(&argc, &argv))
        return 1;

    App app;
    memset(&app, 0, sizeof(app));
    gui_build(&app);

    /* Optional file to open from the command line / file manager. */
    if (argc >= 2) {
        GError *err = NULL;
        if (!app_load_file(&app, argv[1], &err)) {
            if (err) {
                gui_show_message(&app, GTK_MESSAGE_WARNING, "%s", err->message);
                g_error_free(err);
            }
        }
    }

    gui_run();

    g_free(app.current_file);
    xmlCleanupParser();
    return 0;
}
