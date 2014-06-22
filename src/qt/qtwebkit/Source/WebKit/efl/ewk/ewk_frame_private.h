/*
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2012 Samsung Electronics

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

#ifndef ewk_frame_private_h
#define ewk_frame_private_h

#include "ewk_frame.h"
#include <Evas.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
class HistoryItem;
class HTMLPlugInElement;
class KURL;
class Frame;
class IntSize;
class Widget;
}

Evas_Object* ewk_frame_add(Evas* canvas);
bool ewk_frame_init(Evas_Object* ewkFrame, Evas_Object* view, WebCore::Frame* frame);
bool ewk_frame_child_add(Evas_Object* ewkFrame, WTF::PassRefPtr<WebCore::Frame> child, const WTF::String& name, const WebCore::KURL& url, const WTF::String& referrer);
void ewk_frame_view_set(Evas_Object* ewkFrame, Evas_Object* newParent);

void ewk_frame_core_gone(Evas_Object* ewkFrame);

void ewk_frame_load_committed(Evas_Object* ewkFrame);
void ewk_frame_load_started(Evas_Object* ewkFrame);
void ewk_frame_load_provisional(Evas_Object* ewkFrame);
void ewk_frame_load_provisional_failed(Evas_Object* ewkFrame, const Ewk_Frame_Load_Error* error);
void ewk_frame_load_firstlayout_finished(Evas_Object* ewkFrame);
void ewk_frame_load_firstlayout_nonempty_finished(Evas_Object* ewkFrame);
void ewk_frame_load_document_finished(Evas_Object* ewkFrame);
void ewk_frame_load_finished(Evas_Object* ewkFrame, const char* errorDomain, int errorCode, bool isCancellation, const char* errorDescription, const char* failingUrl);
void ewk_frame_load_resource_finished(Evas_Object* ewkFrame, unsigned long identifier);
void ewk_frame_load_resource_failed(Evas_Object* ewkFrame, Ewk_Frame_Load_Error* error);
void ewk_frame_load_error(Evas_Object* ewkFrame, const char* errorDomain, int errorCode, bool isCancellation, const char* errorDescription, const char* failingUrl);
void ewk_frame_load_progress_changed(Evas_Object* ewkFrame);

void ewk_frame_redirect_cancelled(Evas_Object* ewkFrame);
void ewk_frame_redirect_provisional_load(Evas_Object* ewkFrame);
void ewk_frame_redirect_requested(Evas_Object* ewkFrame, const char* url);
void ewk_frame_request_will_send(Evas_Object* ewkFrame, Ewk_Frame_Resource_Messages* messages);
void ewk_frame_request_assign_identifier(Evas_Object* ewkFrame, const Ewk_Frame_Resource_Request* request);
void ewk_frame_response_received(Evas_Object* ewkFrame, Ewk_Frame_Resource_Response* response);
void ewk_frame_view_state_save(Evas_Object* ewkFrame, WebCore::HistoryItem* item);

void ewk_frame_did_perform_first_navigation(Evas_Object* ewkFrame);

void ewk_frame_contents_size_changed(Evas_Object* ewkFrame, Evas_Coord width, Evas_Coord height);
void ewk_frame_title_set(Evas_Object* ewkFrame, const Ewk_Text_With_Direction* title);

void ewk_frame_view_create_for_view(Evas_Object* ewkFrame, Evas_Object* view);
bool ewk_frame_uri_changed(Evas_Object* ewkFrame);
void ewk_frame_force_layout(Evas_Object* ewkFrame);
void ewk_frame_icon_changed(Evas_Object* ewkFrame);

WTF::PassRefPtr<WebCore::Widget> ewk_frame_plugin_create(Evas_Object* ewkFrame, const WebCore::IntSize& pluginSize, WebCore::HTMLPlugInElement* element, const WebCore::KURL& url, const WTF::Vector<WTF::String>& paramNames, const WTF::Vector<WTF::String>& paramValues, const WTF::String& mimeType, bool loadManually);

void ewk_frame_editor_client_contents_changed(Evas_Object* ewkFrame);
void ewk_frame_editor_client_selection_changed(Evas_Object* ewkFrame);

void ewk_frame_mixed_content_displayed_set(Evas_Object* ewkFrame, bool hasDisplayed);
void ewk_frame_mixed_content_run_set(Evas_Object* ewkFrame, bool hasRun);

void ewk_frame_xss_detected(Evas_Object* ewkFrame, const Ewk_Frame_Xss_Notification* xssInfo);

namespace EWKPrivate {
WebCore::Frame *coreFrame(const Evas_Object *ewkFrame);
Evas_Object* kitFrame(const WebCore::Frame* coreFrame);
} // namespace EWKPrivate

#endif // ewk_frame_private_h
