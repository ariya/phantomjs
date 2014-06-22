/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "url_bar.h"
#include "url_utils.h"

#include <Edje.h>
#include <Ecore_Evas.h>

#define PADDING_SIZE 5

static char *
_url_bar_url_get_with_protocol(Url_Bar *urlBar)
{
    const char *url = edje_object_part_text_get(urlBar->entry, "url.text");

    return url_from_user_input(url);
}

static void
on_urlbar_key_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Evas_Event_Key_Down *ev = event_info;
    Url_Bar *urlBar = (Url_Bar *)data;

    if (!ev->key || strcmp(ev->key, "Return"))
        return;

    char *url = _url_bar_url_get_with_protocol(urlBar);
    if (url) {
        ewk_view_uri_set(urlBar->webView, url);
        free(url);
    }
    evas_object_focus_set(urlBar->webView, EINA_TRUE);
}

static void
on_urlbar_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Evas_Event_Mouse_Down *ev = event_info;
    Url_Bar *urlBar = (Url_Bar *)data;

    if (ev->button == 1) {
        evas_object_focus_set(urlBar->entry, EINA_TRUE);
        edje_object_signal_emit(urlBar->entry, "entry,action,focus", "entry");
    }
}

static void
on_urlbar_focus_out(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Url_Bar *urlBar = (Url_Bar *)data;

    edje_object_signal_emit(urlBar->entry, "entry,action,unfocus", "entry");
}

void
url_bar_width_set(Url_Bar *urlBar, int width)
{
    evas_object_move(urlBar->area, 0, 0);
    evas_object_resize(urlBar->area, width, URL_BAR_HEIGHT);

    evas_object_move(urlBar->entry, PADDING_SIZE, PADDING_SIZE);
    evas_object_resize(urlBar->entry, width - PADDING_SIZE * 2, URL_BAR_HEIGHT - PADDING_SIZE * 2);
}

Url_Bar *
url_bar_add(Evas_Object *webView, int width)
{
    Evas *evas;
    Url_Bar *url_bar;
    if (!webView)
        return NULL;
    evas = evas_object_evas_get(webView);

    url_bar = (Url_Bar *)malloc(sizeof(Url_Bar));
    url_bar->webView = webView;

    url_bar->area = evas_object_rectangle_add(evas);
    evas_object_name_set(url_bar->area, "url_barArea");
    evas_object_color_set(url_bar->area, 255, 255, 255, 255);

    url_bar->entry = edje_object_add(evas);
    Eina_Bool ret = edje_object_file_set(url_bar->entry, TEST_THEME_DIR "/entry.edj", "control/entry/base/default");
    if (!ret) {
        evas_object_del(url_bar->area);

        free(url_bar);
        return NULL;
    }

    edje_object_part_text_set(url_bar->entry, "url.text", "");

    /* Set URL bar dimensions and show it */
    url_bar_width_set(url_bar, width);
    evas_object_show(url_bar->area);
    evas_object_show(url_bar->entry);

    evas_object_event_callback_add(url_bar->entry, EVAS_CALLBACK_MOUSE_DOWN, on_urlbar_mouse_down, url_bar);
    evas_object_event_callback_add(url_bar->entry, EVAS_CALLBACK_KEY_DOWN, on_urlbar_key_down, url_bar);
    evas_object_event_callback_add(url_bar->entry, EVAS_CALLBACK_FOCUS_OUT, on_urlbar_focus_out, url_bar);

    return url_bar;
}

void
url_bar_del(Url_Bar *urlBar)
{
    if (!urlBar)
        return;

    evas_object_event_callback_del(urlBar->entry, EVAS_CALLBACK_KEY_DOWN, on_urlbar_key_down);
    evas_object_event_callback_del(urlBar->entry, EVAS_CALLBACK_MOUSE_DOWN, on_urlbar_mouse_down);
    evas_object_event_callback_del(urlBar->entry, EVAS_CALLBACK_FOCUS_OUT, on_urlbar_focus_out);

    evas_object_del(urlBar->area);
    evas_object_del(urlBar->entry);
    free(urlBar);
}

void
url_bar_url_set(Url_Bar *urlBar, const char *url)
{
    if (!urlBar || !url)
        return;

    edje_object_part_text_set(urlBar->entry, "url.text", url);
}
