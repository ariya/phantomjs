/*
 * Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
 * Copyright (C) 2009, 2010 ProFUSION embedded systems
 * Copyright (C) 2009, 2010, 2011 Samsung Electronics
 * Copyright (C) 2012 Intel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "EWebKit.h"

#include "url_bar.h"
#include "url_utils.h"
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_File.h>
#include <Ecore_Getopt.h>
#include <Ecore_X.h>
#include <Edje.h>
#include <Evas.h>
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_WIDTH      800
#define DEFAULT_HEIGHT     600
#define DEFAULT_ZOOM_INIT  1.0

#define info(format, args...)       \
    do {                            \
        if (verbose)                \
            printf(format"\n", ##args); \
    } while (0)

#define MIN_ZOOM_LEVEL 0
#define DEFAULT_ZOOM_LEVEL 5
#define MAX_ZOOM_LEVEL 13

static int currentZoomLevel = DEFAULT_ZOOM_LEVEL;
static float currentZoom = 1.0;

// the zoom values are chosen to be like in Mozilla Firefox 3
static int zoomLevels[] = {30, 50, 67, 80, 90,
                            100,
                            110, 120, 133, 150, 170, 200, 240, 300};

static int verbose = 0;

static Eina_List *windows = NULL;

static char *themePath = NULL;

static const char *backingStores[] = {
    "tiled",
    "single",
    NULL
};

typedef struct _Window_Properties {
    Eina_Bool toolbarsVisible:1;
    Eina_Bool statusbarVisible:1;
    Eina_Bool scrollbarsVisible:1;
    Eina_Bool menubarVisible:1;
} Window_Properties;

Window_Properties windowProperties = { /* Pretend we have them and they are initially visible */
    EINA_TRUE,
    EINA_TRUE,
    EINA_TRUE,
    EINA_TRUE
};

static const Ecore_Getopt options = {
    "EWebLauncher",
    "%prog [options] [url]",
    "0.0.1",
    "(C)2008 INdT (The Nokia Technology Institute)\n"
    "(C)2009, 2010 ProFUSION embedded systems\n"
    "(C)2009, 2010, 2011 Samsung Electronics\n"
    "(C)2012 Intel Corporation\n",
    "GPL",
    "Test Web Browser using the Enlightenment Foundation Libraries of WebKit",
    EINA_TRUE, {
        ECORE_GETOPT_STORE_STR
            ('e', "engine", "ecore-evas engine to use."),
        ECORE_GETOPT_CALLBACK_NOARGS
            ('E', "list-engines", "list ecore-evas engines.",
             ecore_getopt_callback_ecore_evas_list_engines, NULL),
        ECORE_GETOPT_CHOICE
            ('b', "backing-store", "choose backing store to use.", backingStores),
        ECORE_GETOPT_STORE_DOUBLE
            ('r', "device-pixel-ratio", "Ratio between the CSS units and device pixels."),
        ECORE_GETOPT_STORE_DEF_BOOL
            ('c', "encoding-detector", "enable/disable encoding detector", 0),
        ECORE_GETOPT_STORE_DEF_BOOL
            ('f', "flattening", "frame flattening.", 0),
        ECORE_GETOPT_STORE_DEF_BOOL
            ('F', "fullscreen", "fullscreen mode.", 0),
        ECORE_GETOPT_CALLBACK_ARGS
            ('g', "geometry", "geometry to use in x:y:w:h form.", "X:Y:W:H",
             ecore_getopt_callback_geometry_parse, NULL),
        ECORE_GETOPT_STORE_STR
            ('t', "theme", "path to read the theme file from."),
        ECORE_GETOPT_STORE_DEF_BOOL
            ('T', "tiled-backing-store", "enable/disable WebCore's tiled backingstore(ewk_view_single only)", 0),
        ECORE_GETOPT_STORE_STR
            ('U', "user-agent", "custom user agent string to use."),
        ECORE_GETOPT_COUNT
            ('v', "verbose", "be more verbose."),
        ECORE_GETOPT_VERSION
            ('V', "version"),
        ECORE_GETOPT_COPYRIGHT
            ('R', "copyright"),
        ECORE_GETOPT_LICENSE
            ('L', "license"),
        ECORE_GETOPT_HELP
            ('h', "help"),
        ECORE_GETOPT_SENTINEL
    }
};

typedef struct _User_Arguments {
    char *engine;
    Eina_Bool quitOption;
    char *backingStore;
    float device_pixel_ratio;
    Eina_Bool enableEncodingDetector;
    Eina_Bool enableTiledBackingStore;
    Eina_Bool isFlattening;
    Eina_Bool isFullscreen;
    Eina_Rectangle geometry;
    char *theme;
    char *userAgent;
    char *databasePath;
} User_Arguments;

typedef struct _ELauncher {
    Ecore_Evas *ee;
    Evas *evas;
    Evas_Object *browser;
    Url_Bar *url_bar;
    User_Arguments *userArgs;
} ELauncher;

static void windowDestroy(Ecore_Evas *ee);
static void closeWindow(Ecore_Evas *ee);
static int browserCreate(const char *url, User_Arguments *userArgs);
static int webInspectorCreate(ELauncher *appBrowser);
static ELauncher *windowCreate(User_Arguments *userArgs);

static ELauncher *
find_app_from_ee(Ecore_Evas *ee)
{
    Eina_List *l;
    void *data;

    EINA_LIST_FOREACH(windows, l, data)
    {
        ELauncher *app = (ELauncher *) data;
        if (app->ee == ee)
            return app;
    }
    return NULL;
}

static void
print_history(Eina_List *list)
{
    Eina_List *l;
    void *d;

    if (!verbose)
       return;

    printf("Session history contains:\n");

    EINA_LIST_FOREACH(list, l, d) {
       Ewk_History_Item *item = (Ewk_History_Item*)d;
       cairo_surface_t *cs = ewk_history_item_icon_surface_get(item);
       char buf[PATH_MAX];
       int s = snprintf(buf, sizeof(buf), "/tmp/favicon-%s.png", ewk_history_item_uri_original_get(item));
       for (s--; s >= (int)sizeof("/tmp/favicon-"); s--) {
           if (!isalnum(buf[s]) && buf[s] != '.')
               buf[s] = '_';
       }
       cs = ewk_history_item_icon_surface_get(item);

       if (cs && cairo_surface_status(cs) == CAIRO_STATUS_SUCCESS)
           cairo_surface_write_to_png(cs, buf);
       else
           buf[0] = '\0';

       printf("* '%s' title='%s' icon='%s'\n",
              ewk_history_item_uri_original_get(item),
              ewk_history_item_title_get(item), buf);
    }
}

static int
nearest_zoom_level_get(float factor)
{
    int i, intFactor = (int)(factor * 100.0);
    for (i = 0; zoomLevels[i] <= intFactor; i++) { }
    printf("factor=%f, intFactor=%d, zoomLevels[%d]=%d, zoomLevels[%d]=%d\n",
           factor, intFactor, i-1, zoomLevels[i-1], i, zoomLevels[i]);
    if (intFactor - zoomLevels[i-1] < zoomLevels[i] - intFactor)
        return i - 1;
    return i;
}

static Eina_Bool
zoom_level_set(Evas_Object *webview, int level)
{
    float factor = ((float) zoomLevels[level]) / 100.0;
    Evas_Coord ox, oy, mx, my, cx, cy;
    evas_pointer_canvas_xy_get(evas_object_evas_get(webview), &mx, &my);
    evas_object_geometry_get(webview, &ox, &oy, NULL, NULL);
    cx = mx - ox;
    cy = my - oy;
    return ewk_view_zoom_animated_set(webview, factor, 0.5, cx, cy);
}

static void
on_browser_ecore_evas_resize(Ecore_Evas *ee)
{
    ELauncher *app;
    Evas_Object *webview;
    int w, h;

    ecore_evas_geometry_get(ee, NULL, NULL, &w, &h);

    /* Resize URL bar */
    app = find_app_from_ee(ee);
    url_bar_width_set(app->url_bar, w);

    webview = evas_object_name_find(ecore_evas_get(ee), "browser");
    evas_object_move(webview, 0, URL_BAR_HEIGHT);
    evas_object_resize(webview, w, h - URL_BAR_HEIGHT);
}

static void
on_inspector_ecore_evas_resize(Ecore_Evas *ee)
{
    Evas_Object *webview;
    int w, h;

    ecore_evas_geometry_get(ee, NULL, NULL, &w, &h);

    webview = evas_object_name_find(ecore_evas_get(ee), "inspector");
    evas_object_move(webview, 0, 0);
    evas_object_resize(webview, w, h);
}

static void
title_set(Ecore_Evas *ee, const Ewk_Text_With_Direction *title, int progress)
{
    const char *appname = "EFL Test Launcher";
    const char *separator = " - ";
    char label[4096];
    int size;

    if (!title || !title->string || !strcmp(title->string, "")) {
        ecore_evas_title_set(ee, appname);
        return;
    }

    if (progress < 100)
        size = snprintf(label, sizeof(label), "%s (%d%%)%s%s", title->string, progress, separator, appname);
    else
        size = snprintf(label, sizeof(label), "%s %s%s", title->string, separator, appname);

    if (size >= (int)sizeof(label))
        return;

    ecore_evas_title_set(ee, label);
}

static void
on_title_changed(void *user_data, Evas_Object *webview, void *event_info)
{
    ELauncher *app = (ELauncher *)user_data;
    const Ewk_Text_With_Direction *title = (const Ewk_Text_With_Direction *)event_info;

    title_set(app->ee, title, 100);
}

static void
on_progress(void *user_data, Evas_Object *webview, void *event_info)
{
    ELauncher *app = (ELauncher *)user_data;
    double *progress = (double *)event_info;

    title_set(app->ee, ewk_view_title_get(app->browser), *progress * 100);
}

static void
on_load_finished(void *user_data, Evas_Object *webview, void *event_info)
{
    const Ewk_Frame_Load_Error *err = (const Ewk_Frame_Load_Error *)event_info;

    if (!err)
        info("Succeeded loading page.");
    else if (err->is_cancellation)
        info("Load was cancelled.");
    else
        info("Failed loading page: %d %s \"%s\", url=%s",
             err->code, err->domain, err->description, err->failing_url);

    currentZoom = ewk_view_zoom_get(webview);
    currentZoomLevel = nearest_zoom_level_get(currentZoom);
    info("WebCore Zoom=%f, currentZoomLevel=%d", currentZoom, currentZoomLevel);
}

static void
on_load_error(void *user_data, Evas_Object *webview, void *event_info)
{
    const Ewk_Frame_Load_Error *err = (const Ewk_Frame_Load_Error *)event_info;
    char message[1024];
    snprintf(message, 1024, "<html><body><div style=\"color:#ff0000\">ERROR!</div><br><div>Code: %d<br>Domain: %s<br>Description: %s<br>URL: %s</div></body</html>",
             err->code, err->domain, err->description, err->failing_url);
    ewk_frame_contents_set(err->frame, message, 0, "text/html", "UTF-8", err->failing_url);
}

static void
on_toolbars_visible_set(void* user_data, Evas_Object* webview, void* event_info)
{
    Eina_Bool *visible = (Eina_Bool *)event_info;
    if (*visible) {
        info("Toolbars visible changed: show");
        windowProperties.toolbarsVisible = EINA_TRUE;
    } else {
        info("Toolbars visible changed: hide");
        windowProperties.toolbarsVisible = EINA_FALSE;
    }
}

static void
on_toolbars_visible_get(void* user_data, Evas_Object* webview, void* event_info)
{
    Eina_Bool *visible = (Eina_Bool *)event_info;
    *visible = windowProperties.toolbarsVisible;
}

static void
on_statusbar_visible_set(void* user_data, Evas_Object* webview, void* event_info)
{
    Eina_Bool *visible = (Eina_Bool *)event_info;
    if (*visible) {
        info("Statusbar visible changed: show");
        windowProperties.statusbarVisible = EINA_TRUE;
    } else {
        info("Statusbar visible changed: hide");
        windowProperties.statusbarVisible = EINA_FALSE;
    }
}

static void
on_statusbar_visible_get(void* user_data, Evas_Object* webview, void* event_info)
{
    Eina_Bool *visible = (Eina_Bool *)event_info;
    *visible = windowProperties.statusbarVisible;
}

static void
on_scrollbars_visible_set(void* user_data, Evas_Object* webview, void* event_info)
{
    Eina_Bool *visible = (Eina_Bool *)event_info;
    if (*visible) {
        info("Scrollbars visible changed: show");
        windowProperties.scrollbarsVisible = EINA_TRUE;
    } else {
        info("Scrollbars visible changed: hide");
        windowProperties.scrollbarsVisible = EINA_FALSE;
    }
}

static void
on_scrollbars_visible_get(void* user_data, Evas_Object* webview, void* event_info)
{
    Eina_Bool *visible = (Eina_Bool *)event_info;
    *visible = windowProperties.scrollbarsVisible;
}

static void
on_menubar_visible_set(void* user_data, Evas_Object* webview, void* event_info)
{
    Eina_Bool *visible = (Eina_Bool *)event_info;
    if (*visible) {
        info("Menubar visible changed: show");
        windowProperties.menubarVisible = EINA_TRUE;
    } else {
        info("Menubar visible changed: hide");
        windowProperties.menubarVisible = EINA_FALSE;
    }
}

static void
on_menubar_visible_get(void* user_data, Evas_Object* webview, void* event_info)
{
    Eina_Bool *visible = (Eina_Bool *)event_info;
    *visible = windowProperties.menubarVisible;
}

static void
on_tooltip_text_set(void* user_data, Evas_Object* webview, void* event_info)
{
    const char *text = (const char *)event_info;
    info("Tooltip is set: %s", text);
}

static void
on_tooltip_text_unset(void* user_data, Evas_Object* webview, void* event_info)
{
    info("Tooltip is unset");
}

static void
on_inputmethod_changed(void* user_data, Evas_Object* webview, void* event_info)
{
    Eina_Bool active = (Eina_Bool)(long)event_info;
    unsigned int imh;
    info("Keyboard changed: %d", active);

    if (!active)
        return;

    imh = ewk_view_imh_get(webview);
    info("    Keyboard flags: %#.2x", imh);

}

static void
on_url_changed(void* user_data, Evas_Object* webview, void* event_info)
{
    ELauncher *app = (ELauncher *)user_data;
    url_bar_url_set(app->url_bar, ewk_view_uri_get(app->browser));
}

static void
on_mouse_down(void* data, Evas* e, Evas_Object* webview, void* event_info)
{
    Evas_Event_Mouse_Down *ev = (Evas_Event_Mouse_Down *)event_info;

    if (ev->button == 1)
        evas_object_focus_set(webview, EINA_TRUE);
    else if (ev->button == 2)
        evas_object_focus_set(webview, !evas_object_focus_get(webview));
}

static void
on_focus_out(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    info("the webview lost keyboard focus");
}

static void
on_focus_in(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    info("the webview gained keyboard focus");
}

static void
on_key_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Evas_Event_Key_Down *ev = (Evas_Event_Key_Down*) event_info;
    ELauncher *app = data;
    static const char *encodings[] = {
        "ISO-8859-1",
        "UTF-8",
        NULL
    };
    static int currentEncoding = -1;
    const Evas_Modifier *mod = evas_key_modifier_get(e);
    Eina_Bool ctrlPressed = evas_key_modifier_is_set(mod, "Control");
    Eina_Bool altPressed = evas_key_modifier_is_set(mod, "Alt");

    if (!strcmp(ev->key, "Escape")) {
        closeWindow(app->ee);
    } else if (!strcmp(ev->key, "Left") && altPressed) {
        info("Back (Alt+Left) was pressed");
        if (ewk_view_back_possible(obj)) {
            Ewk_History *history = ewk_view_history_get(obj);
            Eina_List *list = ewk_history_back_list_get(history);
            print_history(list);
            ewk_history_item_list_free(list);
            ewk_view_back(obj);
        } else
            info("Back ignored: No back history");
    } else if (!strcmp(ev->key, "Right") && altPressed) {
        info("Forward (Alt+Right) was pressed");
        if (ewk_view_forward_possible(obj)) {
            Ewk_History *history = ewk_view_history_get(obj);
            Eina_List *list = ewk_history_forward_list_get(history);
            print_history(list);
            ewk_history_item_list_free(list);
            ewk_view_forward(obj);
        } else
            info("Forward ignored: No forward history");
    } else if (!strcmp(ev->key, "F3")) {
        currentEncoding++;
        currentEncoding %= (sizeof(encodings) / sizeof(encodings[0]));
        info("Set encoding (F3) pressed. New encoding to %s", encodings[currentEncoding]);
        ewk_view_setting_encoding_custom_set(obj, encodings[currentEncoding]);
    } else if (!strcmp(ev->key, "F4")) {
        Evas_Object *frame = ewk_view_frame_main_get(obj);
        Evas_Coord x, y;
        Ewk_Hit_Test *ht;

        evas_pointer_canvas_xy_get(evas_object_evas_get(obj), &x, &y);
        ht = ewk_frame_hit_test_new(frame, x, y);
        if (!ht)
            printf("No hit test returned for point %d,%d\n", x, y);
        else {
            printf("Hit test for point %d,%d\n"
                   "  pos=%3d,%3d\n"
                   "  bounding_box=%d,%d + %dx%d\n"
                   "  title='%s'\n"
                   "  alternate_text='%s'\n"
                   "  frame=%p (%s)\n"
                   "  link {\n"
                   "    text='%s'\n"
                   "    url='%s'\n"
                   "    title='%s'\n"
                   "    target frame=%p (%s)\n"
                   "  }\n"
                   "context:\n"
                   "%s"
                   "%s"
                   "%s"
                   "%s"
                   "%s\n",
                   x, y,
                   ht->x, ht->y,
                   ht->bounding_box.x, ht->bounding_box.y, ht->bounding_box.w, ht->bounding_box.h,
                   ht->title.string,
                   ht->alternate_text,
                   ht->frame, evas_object_name_get(ht->frame),
                   ht->link.text,
                   ht->link.url,
                   ht->link.title,
                   ht->link.target_frame, evas_object_name_get(ht->link.target_frame),
                   ht->context & EWK_HIT_TEST_RESULT_CONTEXT_LINK ? "  LINK\n" : "",
                   ht->context & EWK_HIT_TEST_RESULT_CONTEXT_IMAGE ? "  IMAGE\n" : "",
                   ht->context & EWK_HIT_TEST_RESULT_CONTEXT_MEDIA ? "   MEDIA\n" : "",
                   ht->context & EWK_HIT_TEST_RESULT_CONTEXT_SELECTION ? "  SELECTION\n" : "",
                   ht->context & EWK_HIT_TEST_RESULT_CONTEXT_EDITABLE ? "  EDITABLE" : "");
            ewk_frame_hit_test_free(ht);
        }

    } else if (!strcmp(ev->key, "F5")) {
        info("Reload (F5) was pressed, reloading.");
        ewk_view_reload(obj);
    } else if (!strcmp(ev->key, "F6")) {
        info("Stop (F6) was pressed, stop loading.");
        ewk_view_stop(obj);
    } else if (!strcmp(ev->key, "F12")) {
        Eina_Bool status = ewk_view_setting_spatial_navigation_get(obj);
        ewk_view_setting_spatial_navigation_set(obj, !status);
        info("Command::keyboard navigation toggle");
    } else if ((!strcmp(ev->key, "minus") || !strcmp(ev->key, "KP_Subtract")) && ctrlPressed) {
        if (currentZoomLevel > MIN_ZOOM_LEVEL && zoom_level_set(obj, currentZoomLevel - 1))
            currentZoomLevel--;
        info("Zoom out (Ctrl + '-') was pressed, zoom level became %.2f", zoomLevels[currentZoomLevel] / 100.0);
    } else if ((!strcmp(ev->key, "equal") || !strcmp(ev->key, "KP_Add")) && ctrlPressed) {
        if (currentZoomLevel < MAX_ZOOM_LEVEL && zoom_level_set(obj, currentZoomLevel + 1))
            currentZoomLevel++;
        info("Zoom in (Ctrl + '+') was pressed, zoom level became %.2f", zoomLevels[currentZoomLevel] / 100.0);
    } else if (!strcmp(ev->key, "0") && ctrlPressed) {
        if (zoom_level_set(obj, DEFAULT_ZOOM_LEVEL))
            currentZoomLevel = DEFAULT_ZOOM_LEVEL;
        info("Zoom to default (Ctrl + '0') was pressed, zoom level became %.2f", zoomLevels[currentZoomLevel] / 100.0);
    } else if (!strcmp(ev->key, "n") && ctrlPressed) {
        info("Create new window (Ctrl+n) was pressed.");
        browserCreate("http://www.google.com", app->userArgs);
    } else if (!strcmp(ev->key, "g") && ctrlPressed ) {
        Evas_Coord x, y, w, h;
        Evas_Object *frame = ewk_view_frame_main_get(obj);
        float zoom = zoomLevels[currentZoomLevel] / 100.0;

        ewk_frame_visible_content_geometry_get(frame, EINA_FALSE, &x, &y, &w, &h);
        x -= w;
        y -= h;
        w *= 4;
        h *= 4;
        info("Pre-render %d,%d + %dx%d", x, y, w, h);
        ewk_view_pre_render_region(obj, x, y, w, h, zoom);
    } else if (!strcmp(ev->key, "r") && ctrlPressed) {
        info("Pre-render 1 extra column/row with current zoom");
        ewk_view_pre_render_relative_radius(obj, 1);
    } else if (!strcmp(ev->key, "p") && ctrlPressed) {
        info("Pre-rendering start");
        ewk_view_pre_render_start(obj);
    } else if (!strcmp(ev->key, "d") && ctrlPressed) {
        info("Render suspended");
        ewk_view_disable_render(obj);
    } else if (!strcmp(ev->key, "e") && ctrlPressed) {
        info("Render resumed");
        ewk_view_enable_render(obj);
    } else if (!strcmp(ev->key, "s") && ctrlPressed) {
        Evas_Object *frame = ewk_view_frame_main_get(obj);
        Ewk_Security_Origin *origin = ewk_frame_security_origin_get(frame);
        printf("Security origin information:\n"
               "  protocol=%s\n"
               "  host=%s\n"
               "  port=%d\n"
               "  web database quota=%" PRIu64 "\n",
               ewk_security_origin_protocol_get(origin),
               ewk_security_origin_host_get(origin),
               ewk_security_origin_port_get(origin),
               ewk_security_origin_web_database_quota_get(origin));

        Eina_List *databaseList = ewk_security_origin_web_database_get_all(origin);
        Eina_List *listIterator = 0;
        Ewk_Web_Database *database;
        EINA_LIST_FOREACH(databaseList, listIterator, database)
            printf("Database information:\n"
                   "  name=%s\n"
                   "  display name=%s\n"
                   "  filename=%s\n"
                   "  expected size=%" PRIu64 "\n"
                   "  size=%" PRIu64 "\n",
                   ewk_web_database_name_get(database),
                   ewk_web_database_display_name_get(database),
                   ewk_web_database_filename_get(database),
                   ewk_web_database_expected_size_get(database),
                   ewk_web_database_size_get(database));

        ewk_security_origin_free(origin);
        ewk_web_database_list_free(databaseList);
    } else if (!strcmp(ev->key, "i") && ctrlPressed) {
        Evas_Object *inspector_view = ewk_view_inspector_view_get(obj);
        if (inspector_view) {
            info("Web Inspector close");
            ewk_view_inspector_close(obj);
        } else {
            info("Web Inspector show");
            ewk_view_inspector_show(obj);
        }
    }
}

static void
on_browser_del(void *data, Evas *evas, Evas_Object *browser, void *event)
{
    ELauncher *app = (ELauncher*) data;

    evas_object_event_callback_del(app->browser, EVAS_CALLBACK_KEY_DOWN, on_key_down);
    evas_object_event_callback_del(app->browser, EVAS_CALLBACK_MOUSE_DOWN, on_mouse_down);
    evas_object_event_callback_del(app->browser, EVAS_CALLBACK_FOCUS_IN, on_focus_in);
    evas_object_event_callback_del(app->browser, EVAS_CALLBACK_FOCUS_OUT, on_focus_out);
    evas_object_event_callback_del(app->browser, EVAS_CALLBACK_DEL, on_browser_del);
}

static void
on_inspector_view_create(void *user_data, Evas_Object *webview, void *event_info)
{
    ELauncher *app_browser = (ELauncher *)user_data;

    webInspectorCreate(app_browser);
}

static void
on_inspector_view_close(void *user_data, Evas_Object *webview, void *event_info)
{
    Eina_List *l;
    void *app;
    ELauncher *app_browser = (ELauncher *)user_data;
    Evas_Object *inspector_view = (Evas_Object *)event_info;

    ewk_view_inspector_view_set(app_browser->browser, NULL);

    EINA_LIST_FOREACH(windows, l, app)
        if (((ELauncher *)app)->browser == inspector_view)
            break;

    windows = eina_list_remove(windows, app);
    windowDestroy(((ELauncher *)app)->ee);
    free(app);
}

static void
on_inspector_view_destroyed(Ecore_Evas *ee)
{
    ELauncher *app;

    app = find_app_from_ee(ee);
    evas_object_smart_callback_call(app->browser, "inspector,view,destroy", NULL);
}

static int
quit(Eina_Bool success, const char *msg)
{
    edje_shutdown();
    ecore_evas_shutdown();
    ecore_file_shutdown();

    if (msg)
        fputs(msg, (success) ? stdout : stderr);

    if (themePath) {
        free(themePath);
        themePath = NULL;
    }

    if (!success)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

static int
browserCreate(const char *url, User_Arguments *userArgs)
{
    ELauncher *appBrowser = windowCreate(userArgs);
    if (!appBrowser)
        return quit(EINA_FALSE, "ERROR: could not create a browser window\n");

    ecore_evas_title_set(appBrowser->ee, "EFL Test Launcher");
    ecore_evas_callback_resize_set(appBrowser->ee, on_browser_ecore_evas_resize);
    ecore_evas_callback_delete_request_set(appBrowser->ee, closeWindow);

    evas_object_name_set(appBrowser->browser, "browser");

    evas_object_smart_callback_add(appBrowser->browser, "inputmethod,changed", on_inputmethod_changed, appBrowser);
    evas_object_smart_callback_add(appBrowser->browser, "inspector,view,close", on_inspector_view_close, appBrowser);
    evas_object_smart_callback_add(appBrowser->browser, "inspector,view,create", on_inspector_view_create, appBrowser);
    evas_object_smart_callback_add(appBrowser->browser, "load,error", on_load_error, appBrowser);
    evas_object_smart_callback_add(appBrowser->browser, "load,finished", on_load_finished, appBrowser);
    evas_object_smart_callback_add(appBrowser->browser, "load,progress", on_progress, appBrowser);
    evas_object_smart_callback_add(appBrowser->browser, "menubar,visible,get", on_menubar_visible_get, appBrowser);
    evas_object_smart_callback_add(appBrowser->browser, "menubar,visible,set", on_menubar_visible_set, appBrowser);
    evas_object_smart_callback_add(appBrowser->browser, "scrollbars,visible,get", on_scrollbars_visible_get, appBrowser);
    evas_object_smart_callback_add(appBrowser->browser, "scrollbars,visible,set", on_scrollbars_visible_set, appBrowser);
    evas_object_smart_callback_add(appBrowser->browser, "statusbar,visible,get", on_statusbar_visible_get, appBrowser);
    evas_object_smart_callback_add(appBrowser->browser, "statusbar,visible,set", on_statusbar_visible_set, appBrowser);
    evas_object_smart_callback_add(appBrowser->browser, "title,changed", on_title_changed, appBrowser);
    evas_object_smart_callback_add(appBrowser->browser, "toolbars,visible,get", on_toolbars_visible_get, appBrowser);
    evas_object_smart_callback_add(appBrowser->browser, "toolbars,visible,set", on_toolbars_visible_set, appBrowser);
    evas_object_smart_callback_add(appBrowser->browser, "tooltip,text,set", on_tooltip_text_set, appBrowser);
    evas_object_smart_callback_add(appBrowser->browser, "tooltip,text,unset", on_tooltip_text_unset, appBrowser);
    evas_object_smart_callback_add(appBrowser->browser, "uri,changed", on_url_changed, appBrowser);

    evas_object_event_callback_add(appBrowser->browser, EVAS_CALLBACK_DEL, on_browser_del, appBrowser);
    evas_object_event_callback_add(appBrowser->browser, EVAS_CALLBACK_FOCUS_IN, on_focus_in, appBrowser);
    evas_object_event_callback_add(appBrowser->browser, EVAS_CALLBACK_FOCUS_OUT, on_focus_out, appBrowser);
    evas_object_event_callback_add(appBrowser->browser, EVAS_CALLBACK_KEY_DOWN, on_key_down, appBrowser);
    evas_object_event_callback_add(appBrowser->browser, EVAS_CALLBACK_MOUSE_DOWN, on_mouse_down, appBrowser);

    ewk_view_setting_enable_developer_extras_set(appBrowser->browser, EINA_TRUE);

    appBrowser->url_bar = url_bar_add(appBrowser->browser, DEFAULT_WIDTH);

    evas_object_move(appBrowser->browser, 0, URL_BAR_HEIGHT);
    evas_object_resize(appBrowser->browser, userArgs->geometry.w, userArgs->geometry.h - URL_BAR_HEIGHT);

    ewk_view_uri_set(appBrowser->browser, url);

    evas_object_show(appBrowser->browser);
    ecore_evas_show(appBrowser->ee);

    evas_object_focus_set(appBrowser->browser, EINA_TRUE);

    return 1;
}

static int
webInspectorCreate(ELauncher *appBrowser)
{
    ELauncher *appInspector = windowCreate(appBrowser->userArgs);
    if (!appInspector)
        return quit(EINA_FALSE, "ERROR: could not create an inspector window\n");

    ecore_evas_title_set(appInspector->ee, "Web Inspector");
    ecore_evas_callback_resize_set(appInspector->ee, on_inspector_ecore_evas_resize);
    ecore_evas_callback_delete_request_set(appInspector->ee, on_inspector_view_destroyed);

    evas_object_name_set(appInspector->browser, "inspector");

    evas_object_move(appInspector->browser, 0, 0);
    evas_object_resize(appInspector->browser, appInspector->userArgs->geometry.w, appInspector->userArgs->geometry.h);

    evas_object_show(appInspector->browser);
    ecore_evas_show(appInspector->ee);

    evas_object_focus_set(appInspector->browser, EINA_TRUE);

    ewk_view_inspector_view_set(appBrowser->browser, appInspector->browser);

    return 1;
}

static ELauncher *
windowCreate(User_Arguments *userArgs)
{
    ELauncher *app = (ELauncher *)malloc(sizeof(ELauncher));
    if (!app) {
        quit(EINA_FALSE, "ERROR: could not create an ELauncher\n");
        return NULL;
    }

#if defined(WTF_USE_ACCELERATED_COMPOSITING) && defined(HAVE_ECORE_X)
    if (userArgs->engine)
#endif
        app->ee = ecore_evas_new(userArgs->engine, 0, 0, userArgs->geometry.w, userArgs->geometry.h, NULL);
#if defined(WTF_USE_ACCELERATED_COMPOSITING) && defined(HAVE_ECORE_X)
    else {
        const char* engine = "opengl_x11";
        app->ee = ecore_evas_new(engine, 0, 0, userArgs->geometry.w, userArgs->geometry.h, NULL);
    }
#endif
    if (!app->ee) {
        quit(EINA_FALSE, "ERROR: could not construct evas-ecore\n");
        return NULL;
    }

    if (userArgs->isFullscreen)
        ecore_evas_fullscreen_set(app->ee, EINA_TRUE);

    app->evas = ecore_evas_get(app->ee);
    if (!app->evas) {
        quit(EINA_FALSE, "ERROR: could not get evas from evas-ecore\n");
        return NULL;
    }

    if (userArgs->backingStore && !strcasecmp(userArgs->backingStore, "tiled")) {
        app->browser = ewk_view_tiled_add(app->evas);
        info("backing store: tiled");
    } else {
        app->browser = ewk_view_single_add(app->evas);
        info("backing store: single");

        ewk_view_setting_tiled_backing_store_enabled_set(app->browser, userArgs->enableTiledBackingStore);
    }

    ewk_view_theme_set(app->browser, themePath);
    if (userArgs->userAgent)
        ewk_view_setting_user_agent_set(app->browser, userArgs->userAgent);

    ewk_view_setting_local_storage_database_path_set(app->browser, userArgs->databasePath);
    ewk_view_setting_enable_frame_flattening_set(app->browser, userArgs->isFlattening);
    ewk_view_setting_encoding_detector_set(app->browser, userArgs->enableEncodingDetector);
    ewk_view_device_pixel_ratio_set(app->browser, userArgs->device_pixel_ratio);

    app->userArgs = userArgs;
    app->url_bar = NULL;

    windows = eina_list_append(windows, app);

    return app;
}

static void
windowDestroy(Ecore_Evas *ee)
{
    ecore_evas_free(ee);
    if (!eina_list_count(windows))
        ecore_main_loop_quit();
}

static void
closeWindow(Ecore_Evas *ee)
{
    ELauncher *app;

    app = find_app_from_ee(ee);
    ewk_view_inspector_close(app->browser);

    windows = eina_list_remove(windows, app);
    url_bar_del(app->url_bar);
    windowDestroy(ee);
    free(app);
}

static Eina_Bool
main_signal_exit(void *data, int ev_type, void *ev)
{
    ELauncher *app;
    while (windows) {
        app = (ELauncher*) eina_list_data_get(windows);
        ewk_view_inspector_close(app->browser);

        ecore_evas_free(app->ee);
        windows = eina_list_remove(windows, app);
    }
    if (!eina_list_count(windows))
        ecore_main_loop_quit();
    return EINA_TRUE;
}

static char *
findThemePath(const char *theme)
{
    const char *default_theme = TEST_THEME_DIR "/default.edj";
    char *rpath;
    struct stat st;

    if (!theme)
        theme = default_theme;

    rpath = ecore_file_realpath(theme);

    if (!strlen(rpath) || stat(rpath, &st)) {
        free(rpath);
        return NULL;
    }

    return rpath;
}

int
parseUserArguments(int argc, char *argv[], User_Arguments *userArgs)
{
    int args;

    userArgs->engine = NULL;
    userArgs->quitOption = EINA_FALSE;
    userArgs->backingStore = (char *)backingStores[1];
    userArgs->device_pixel_ratio = 1.0;
    userArgs->enableEncodingDetector = EINA_FALSE;
    userArgs->enableTiledBackingStore = EINA_FALSE;
    userArgs->isFlattening = EINA_FALSE;
    userArgs->isFullscreen = EINA_FALSE;
    userArgs->geometry.x = 0;
    userArgs->geometry.y = 0;
    userArgs->geometry.w = 0;
    userArgs->geometry.h = 0;
    userArgs->theme = NULL;
    userArgs->userAgent = NULL;

    Ecore_Getopt_Value values[] = {
        ECORE_GETOPT_VALUE_STR(userArgs->engine),
        ECORE_GETOPT_VALUE_BOOL(userArgs->quitOption),
        ECORE_GETOPT_VALUE_STR(userArgs->backingStore),
        ECORE_GETOPT_VALUE_DOUBLE(userArgs->device_pixel_ratio),
        ECORE_GETOPT_VALUE_BOOL(userArgs->enableEncodingDetector),
        ECORE_GETOPT_VALUE_BOOL(userArgs->isFlattening),
        ECORE_GETOPT_VALUE_BOOL(userArgs->isFullscreen),
        ECORE_GETOPT_VALUE_PTR_CAST(userArgs->geometry),
        ECORE_GETOPT_VALUE_STR(userArgs->theme),
        ECORE_GETOPT_VALUE_BOOL(userArgs->enableTiledBackingStore),
        ECORE_GETOPT_VALUE_STR(userArgs->userAgent),
        ECORE_GETOPT_VALUE_INT(verbose),
        ECORE_GETOPT_VALUE_BOOL(userArgs->quitOption),
        ECORE_GETOPT_VALUE_BOOL(userArgs->quitOption),
        ECORE_GETOPT_VALUE_BOOL(userArgs->quitOption),
        ECORE_GETOPT_VALUE_BOOL(userArgs->quitOption),
        ECORE_GETOPT_VALUE_NONE
    };

    ecore_app_args_set(argc, (const char**) argv);
    args = ecore_getopt_parse(&options, values, argc, argv);

    themePath = findThemePath(userArgs->theme);

    if ((userArgs->geometry.w <= 0) || (userArgs->geometry.h <= 0)) {
        userArgs->geometry.w = DEFAULT_WIDTH;
        userArgs->geometry.h = DEFAULT_HEIGHT;
    }

    return args;
}

int
main(int argc, char *argv[])
{
    const char *default_url = "http://www.google.com/";
    const char *tmp;
    const char *proxyUri;
    char path[PATH_MAX];
    int args;

    User_Arguments userArgs;

    if (!ewk_init())
        return EXIT_FAILURE;

    if (!ecore_file_init()) {
        ewk_shutdown();
        return EXIT_FAILURE;
    }

    args = parseUserArguments(argc, argv, &userArgs);
    if (args < 0)
       return quit(EINA_FALSE, "ERROR: could not parse options.\n");

    if (userArgs.quitOption)
        return quit(EINA_TRUE, NULL);

    if (!themePath)
        return quit(EINA_FALSE, "ERROR: could not find theme.\n");

    tmp = getenv("TMPDIR");
    if (!tmp)
        tmp = "/tmp";
    snprintf(path, sizeof(path), "%s/.ewebkit-%u", tmp, getuid());
    if (!ecore_file_mkpath(path))
        return quit(EINA_FALSE, "ERROR: could not create settings database directory.\n");

    userArgs.databasePath = path;

    ewk_settings_icon_database_path_set(path);
    ewk_settings_web_database_path_set(path);

    proxyUri = getenv("http_proxy");
    if (proxyUri)
        ewk_network_proxy_uri_set(proxyUri);

    if (args < argc) {
        char *url = url_from_user_input(argv[args]);
        browserCreate(url, &userArgs);
        free(url);
    } else
        browserCreate(default_url, &userArgs);

    ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, main_signal_exit, &windows);

    ecore_main_loop_begin();

    ecore_file_shutdown();
    ewk_shutdown();

    return quit(EINA_TRUE, NULL);
}
