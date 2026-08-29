/*
 * Wii Meta Editor - XML layer
 *
 * Reads and writes Wii Homebrew Channel meta.xml files using libxml2.
 * This module has no GUI dependencies; it works on a plain MetaData
 * struct so it stays easy to unit-test and reuse.
 */

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdio.h>
#include <string.h>

#include "app.h"

/* ---- Helpers ------------------------------------------------------------ */

static xmlNodePtr find_child(xmlNodePtr parent, const char *name)
{
    for (xmlNodePtr n = parent->children; n; n = n->next)
        if (n->type == XML_ELEMENT_NODE &&
            xmlStrcmp(n->name, (const xmlChar *)name) == 0)
            return n;
    return NULL;
}

static gchar *node_text(xmlNodePtr node)
{
    xmlChar *raw = node ? xmlNodeGetContent(node) : NULL;
    gchar *text = raw ? g_strdup((const char *)raw) : g_strdup("");
    if (raw)
        xmlFree(raw);
    g_strstrip(text);
    return text;
}

static gboolean file_is_empty(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return TRUE; /* Cannot open -> treat as an empty template. */
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);
    return size <= 0;
}

/* ---- Date handling ------------------------------------------------------ */

/*
 * Accepted input forms:
 *   20231209000000  (native Wii format)
 *   2023-12-09
 *   09.12.2023
 *   09.12.23
 *
 * Returns a compact static buffer, or NULL when the input is not a date.
 */
gchar *meta_normalize_date(const char *in)
{
    int y = 0, m = 0, d = 0;

    if (sscanf(in, "%4d%2d%2d", &y, &m, &d) >= 3 && y >= 2001)
        return g_strdup_printf("%04d%02d%02d000000", y, m, d);
    if (sscanf(in, "%d-%d-%d", &y, &m, &d) == 3 && y >= 2001)
        return g_strdup_printf("%04d%02d%02d000000", y, m, d);
    if (sscanf(in, "%d.%d.%d", &d, &m, &y) == 3) {
        if (y < 100)
            y += 2000;
        if (y >= 2001)
            return g_strdup_printf("%04d%02d%02d000000", y, m, d);
    }
    return NULL;
}

gchar *meta_pretty_date(const char *xmldate)
{
    int y = 0, m = 0, d = 0;
    if (sscanf(xmldate, "%4d%2d%2d", &y, &m, &d) >= 3 && y >= 2001)
        return g_strdup_printf("%04d-%02d-%02d", y, m, d);
    return g_strdup(xmldate);
}

/* ---- Lifecycle ---------------------------------------------------------- */

MetaData *meta_new(void)
{
    return g_new0(MetaData, 1);
}

void meta_free(MetaData *meta)
{
    if (!meta)
        return;
    g_free(meta->app_name);
    g_free(meta->version);
    g_free(meta->coder);
    g_free(meta->release_date);
    g_free(meta->short_description);
    g_free(meta->long_description);
    g_free(meta);
}

/* ---- Reading ------------------------------------------------------------ */

/*
 * Loads the given path into `meta` (must be a fresh meta_new() instance).
 * Empty or already-consumed content is reported through `was_empty`.
 */
MetaData *meta_load_from_path(const char *path, gboolean *was_empty, GError **err)
{
    MetaData *meta = meta_new();

    if (was_empty)
        *was_empty = file_is_empty(path);

    /* Empty files are offered as a blank template instead of failing, so a
     * user can build a meta.xml from scratch. */
    if (*was_empty)
        return meta;

    xmlDocPtr doc = xmlReadFile(path, NULL, XML_PARSE_NONET | XML_PARSE_NOBLANKS);
    if (!doc) {
        g_set_error(err, G_IO_ERROR, G_IO_ERROR_FAILED,
                    "Could not read '%s' as XML.\n"
                    "It may be empty or corrupted.",
                    path);
        meta_free(meta);
        return NULL;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    xmlNodePtr app =
        (root && xmlStrcmp(root->name, (const xmlChar *)"app") == 0) ? root : NULL;

    meta->app_name = app ? node_text(find_child(app, "name")) : g_strdup("");
    meta->version  = app ? node_text(find_child(app, "version")) : g_strdup("");
    meta->coder    = app ? node_text(find_child(app, "coder")) : g_strdup("");

    gchar *raw_date      = app ? node_text(find_child(app, "release_date")) : g_strdup("");
    meta->release_date   = meta_pretty_date(raw_date);
    g_free(raw_date);

    meta->short_description =
        app ? node_text(find_child(app, "short_description")) : g_strdup("");
    meta->long_description =
        app ? node_text(find_child(app, "long_description")) : g_strdup("");
    meta->ahb_access = app ? (find_child(app, "ahb_access") != NULL) : 0;

    xmlFreeDoc(doc);
    return meta;
}

/* ---- Writing ------------------------------------------------------------ */

gboolean meta_save_to_path(const MetaData *meta, const char *path, GError **err)
{
    gchar *norm_date = meta_normalize_date(meta->release_date);
    if (!norm_date) {
        g_set_error(err, G_IO_ERROR, G_IO_ERROR_FAILED,
                    "Invalid date: '%s'.\n"
                    "Expected: YYYY-MM-DD, DD.MM.YYYY or YYYYMMDDHHMMSS.",
                    meta->release_date);
        return FALSE;
    }

    xmlDocPtr doc = xmlNewDoc((const xmlChar *)"1.0");
    xmlNodePtr app = xmlNewNode(NULL, (const xmlChar *)"app");
    xmlSetProp(app, (const xmlChar *)"version", (const xmlChar *)"1");
    xmlDocSetRootElement(doc, app);

    xmlNewTextChild(app, NULL, (const xmlChar *)"name", (const xmlChar *)meta->app_name);
    xmlNewTextChild(app, NULL, (const xmlChar *)"version", (const xmlChar *)meta->version);
    xmlNewTextChild(app, NULL, (const xmlChar *)"release_date", (const xmlChar *)norm_date);
    xmlNewTextChild(app, NULL, (const xmlChar *)"coder", (const xmlChar *)meta->coder);
    xmlNewTextChild(app, NULL, (const xmlChar *)"short_description",
                    (const xmlChar *)meta->short_description);

    xmlNodePtr ld = xmlNewChild(app, NULL, (const xmlChar *)"long_description", NULL);
    xmlNodeSetContent(ld, (const xmlChar *)meta->long_description);

    if (meta->ahb_access)
        xmlNewTextChild(app, NULL, (const xmlChar *)"ahb_access", (const xmlChar *)"");

    int wrote = xmlSaveFormatFileEnc(path, doc, "UTF-8", 1);

    xmlFreeDoc(doc);
    g_free(norm_date);

    if (wrote < 0) {
        g_set_error(err, G_IO_ERROR, G_IO_ERROR_FAILED,
                    "Could not write the file '%s'.", path);
        return FALSE;
    }
    return TRUE;
}
