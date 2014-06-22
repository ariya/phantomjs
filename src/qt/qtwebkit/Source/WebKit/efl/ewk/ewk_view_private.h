/*
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2012 Samsung Electronics
    Copyright (C) 2012 Intel Corporation

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef ewk_view_private_h
#define ewk_view_private_h

#include "Frame.h"
#include "NetworkStorageSession.h"
#include "Page.h"
#include "Widget.h"
#include "ewk_paint_context_private.h"
#include "ewk_view.h"

namespace WebCore {
#if ENABLE(INPUT_TYPE_COLOR)
class Color;
class ColorChooserClient;
#endif

class Cursor;
#if USE(ACCELERATED_COMPOSITING)
class GraphicsContext3D;
class GraphicsLayer;
#endif
class HTMLPlugInElement;
class IntRect;
class IntSize;
class PopupMenuClient;
}

// Defines the names for initializing ewk_view_smart_class
const char ewkViewTiledName[] = "Ewk_View_Tiled";
const char ewkViewSingleName[] = "Ewk_View_Single";

// Define to prevent an application using different view type from calling the function.
#define EWK_VIEW_TYPE_CHECK_OR_RETURN(ewkView, viewName, ...) \
    if (!evas_object_smart_type_check(ewkView, viewName)) { \
        INFO("ewkView isn't an instance of %s", viewName); \
        return __VA_ARGS__; \
    }

void ewk_view_cursor_set(Evas_Object* ewkView, const WebCore::Cursor& cursor);
void ewk_view_ready(Evas_Object* ewkView);
void ewk_view_input_method_state_set(Evas_Object* ewkView, bool active);
void ewk_view_title_set(Evas_Object* ewkView, const Ewk_Text_With_Direction* title);
void ewk_view_uri_changed(Evas_Object* ewkView);
void ewk_view_load_document_finished(Evas_Object* ewkView, Evas_Object* frame);
void ewk_view_load_started(Evas_Object* ewkView, Evas_Object* ewkFrame);
void ewk_view_load_provisional(Evas_Object* ewkView);
void ewk_view_load_provisional_failed(Evas_Object* ewkView, const Ewk_Frame_Load_Error* error);
void ewk_view_frame_main_load_started(Evas_Object* ewkView);
void ewk_view_frame_main_cleared(Evas_Object* ewkView);
void ewk_view_frame_main_icon_received(Evas_Object* ewkView);
void ewk_view_load_finished(Evas_Object* ewkView, const Ewk_Frame_Load_Error* error);
void ewk_view_load_error(Evas_Object* ewkView, const Ewk_Frame_Load_Error* error);
void ewk_view_load_progress_changed(Evas_Object* ewkView);
void ewk_view_load_show(Evas_Object* ewkView);
void ewk_view_onload_event(Evas_Object* ewkView, Evas_Object* frame);
void ewk_view_restore_state(Evas_Object* ewkView, Evas_Object* frame);
Evas_Object* ewk_view_window_create(Evas_Object* ewkView, bool javascript, const WebCore::WindowFeatures* coreFeatures);
void ewk_view_window_close(Evas_Object* ewkView);

void ewk_view_mouse_link_hover_in(Evas_Object* ewkView, void* data);
void ewk_view_mouse_link_hover_out(Evas_Object* ewkView);

void ewk_view_toolbars_visible_set(Evas_Object* ewkView, bool visible);
void ewk_view_toolbars_visible_get(Evas_Object* ewkView, bool* visible);

void ewk_view_statusbar_visible_set(Evas_Object* ewkView, bool visible);
void ewk_view_statusbar_visible_get(Evas_Object* ewkView, bool* visible);
void ewk_view_statusbar_text_set(Evas_Object* ewkView, const char* text);

void ewk_view_scrollbars_visible_set(Evas_Object* ewkView, bool visible);
void ewk_view_scrollbars_visible_get(Evas_Object* ewkView, bool* visible);

void ewk_view_menubar_visible_set(Evas_Object* ewkView, bool visible);
void ewk_view_menubar_visible_get(Evas_Object* ewkView, bool* visible);

void ewk_view_tooltip_text_set(Evas_Object* ewkView, const char* text);

void ewk_view_add_console_message(Evas_Object* ewkView, const char* message, unsigned int lineNumber, const char* sourceID);

void ewk_view_run_javascript_alert(Evas_Object* ewkView, Evas_Object* frame, const char* message);
bool ewk_view_run_javascript_confirm(Evas_Object* ewkView, Evas_Object* frame, const char* message);
bool ewk_view_run_before_unload_confirm(Evas_Object* ewkView, Evas_Object* frame, const char* message);
bool ewk_view_run_javascript_prompt(Evas_Object* ewkView, Evas_Object* frame, const char* message, const char* defaultValue, const char** value);
bool ewk_view_should_interrupt_javascript(Evas_Object* ewkView);
int64_t ewk_view_exceeded_application_cache_quota(Evas_Object* ewkView, Ewk_Security_Origin *origin, int64_t defaultOriginQuota, int64_t totalSpaceNeeded);
uint64_t ewk_view_exceeded_database_quota(Evas_Object* ewkView, Evas_Object* frame, const char* databaseName, uint64_t currentSize, uint64_t expectedSize);

bool ewk_view_run_open_panel(Evas_Object* ewkView, Evas_Object* frame, Ewk_File_Chooser* fileChooser, Eina_List** selectedFilenames);

void ewk_view_repaint(Evas_Object* ewkView, Evas_Coord x, Evas_Coord y, Evas_Coord width, Evas_Coord height);
void ewk_view_scroll(Evas_Object*, const WebCore::IntSize& delta, const WebCore::IntRect& rectToScroll, const WebCore::IntRect& clipRect);
WebCore::Page* ewk_view_core_page_get(const Evas_Object* ewkView);

WTF::PassRefPtr<WebCore::Frame> ewk_view_frame_create(Evas_Object* ewkView, Evas_Object* frame, const WTF::String& name, WebCore::HTMLFrameOwnerElement* ownerElement, const WebCore::KURL& url, const WTF::String& referrer);

WTF::PassRefPtr<WebCore::Widget> ewk_view_plugin_create(Evas_Object* ewkView, Evas_Object* frame, const WebCore::IntSize& pluginSize, WebCore::HTMLPlugInElement* element, const WebCore::KURL& url, const WTF::Vector<WTF::String>& paramNames, const WTF::Vector<WTF::String>& paramValues, const WTF::String& mimeType, bool loadManually);

#if ENABLE(INPUT_TYPE_COLOR)
void ewk_view_color_chooser_new(Evas_Object* ewkView, WebCore::ColorChooserClient* client, const WebCore::Color& initialColor);
void ewk_view_color_chooser_changed(Evas_Object* ewkView, const WebCore::Color& newColor);
#endif

void ewk_view_popup_new(Evas_Object* ewkView, WebCore::PopupMenuClient* client, int selected, const WebCore::IntRect& rect);
void ewk_view_viewport_attributes_set(Evas_Object* ewkView, const WebCore::ViewportArguments& arguments);

void ewk_view_download_request(Evas_Object* ewkView, Ewk_Download* download);

void ewk_view_editor_client_contents_changed(Evas_Object* ewkView);
void ewk_view_editor_client_selection_changed(Evas_Object* ewkView);

bool ewk_view_focus_can_cycle(Evas_Object* ewkView, Ewk_Focus_Direction direction);
void ewk_view_frame_view_creation_notify(Evas_Object* ewkView);

Eina_Bool ewk_view_paint(Ewk_View_Private_Data* priv, Ewk_Paint_Context* context, const Eina_Rectangle* area);
Eina_Bool ewk_view_paint_contents(Ewk_View_Private_Data* priv, Ewk_Paint_Context* context, const Eina_Rectangle* area);

#if ENABLE(NETSCAPE_PLUGIN_API)
void ewk_view_js_window_object_clear(Evas_Object* ewkView, Evas_Object* frame);
#endif

#if USE(TILED_BACKING_STORE)
void ewk_view_tiled_backing_store_invalidate(Evas_Object* ewkView, const WebCore::IntRect& area);
#endif

#if ENABLE(TOUCH_EVENTS)
void ewk_view_need_touch_events_set(Evas_Object*, bool needed);
bool ewk_view_need_touch_events_get(const Evas_Object*);
#endif

const Eina_Rectangle* ewk_view_repaints_pop(Ewk_View_Private_Data* priv, size_t* count);
const Vector<WebCore::IntSize>& ewk_view_scroll_offsets_get(const Ewk_View_Private_Data*);
const Vector<WebCore::IntRect>& ewk_view_scroll_rects_get(const Ewk_View_Private_Data*);

void ewk_view_repaint_add(Ewk_View_Private_Data* priv, Evas_Coord x, Evas_Coord y, Evas_Coord width, Evas_Coord height);

void ewk_view_layout_if_needed_recursive(Ewk_View_Private_Data* priv);

bool ewk_view_navigation_policy_decision(Evas_Object* ewkView, Ewk_Frame_Resource_Request* request, Ewk_Navigation_Type navigationType);

void ewk_view_contents_size_changed(Evas_Object* ewkView, Evas_Coord width, Evas_Coord height);

WebCore::FloatRect ewk_view_page_rect_get(const Evas_Object* ewkView);

void ewk_view_mixed_content_displayed_set(Evas_Object* ewkView, bool hasDisplayed);
void ewk_view_mixed_content_run_set(Evas_Object* ewkView, bool hasRun);

#if USE(ACCELERATED_COMPOSITING)
bool ewk_view_accelerated_compositing_object_create(Evas_Object* ewkView, Evas_Native_Surface* nativeSurface, const WebCore::IntRect& rect);
WebCore::GraphicsContext3D* ewk_view_accelerated_compositing_context_get(Evas_Object* ewkView);
void ewk_view_root_graphics_layer_set(Evas_Object* ewkView, WebCore::GraphicsLayer* rootLayer);
void ewk_view_mark_for_sync(Evas_Object* ewkView);
#endif

#if ENABLE(FULLSCREEN_API)
void ewk_view_fullscreen_enter(const Evas_Object* ewkView);
void ewk_view_fullscreen_exit(const Evas_Object* ewkView);
#endif

namespace EWKPrivate {
WebCore::Page *corePage(const Evas_Object *ewkView);
PlatformPageClient corePageClient(Evas_Object* ewkView);
WebCore::NetworkStorageSession* storageSession(const Evas_Object* ewkView);
} // namespace EWKPrivate

#endif // ewk_view_private_h
