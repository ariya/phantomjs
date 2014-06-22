/*
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WorkQueueItem.h"

#include "DumpRenderTree.h"

#include <JavaScriptCore/JSStringRef.h>
#include <string.h>
#include <webkit/webkit.h>
#include <wtf/gobject/GOwnPtr.h>

// Returns a newly allocated UTF-8 character buffer which must be freed with g_free()
gchar* JSStringCopyUTF8CString(JSStringRef jsString)
{
    size_t dataSize = JSStringGetMaximumUTF8CStringSize(jsString);
    gchar* utf8 = (gchar*)g_malloc(dataSize);
    JSStringGetUTF8CString(jsString, utf8, dataSize);

    return utf8;
}

bool LoadItem::invoke() const
{
    gchar* targetString = JSStringCopyUTF8CString(m_target.get());

    WebKitWebFrame* targetFrame;
    if (!strlen(targetString))
        targetFrame = mainFrame;
    else
        targetFrame = webkit_web_frame_find_frame(mainFrame, targetString);
    g_free(targetString);

    gchar* urlString = JSStringCopyUTF8CString(m_url.get());
    WebKitNetworkRequest* request = webkit_network_request_new(urlString);
    g_free(urlString);
    webkit_web_frame_load_request(targetFrame, request);
    g_object_unref(request);

    return true;
}

bool LoadHTMLStringItem::invoke() const
{
    GOwnPtr<gchar> content(JSStringCopyUTF8CString(m_content.get()));
    GOwnPtr<gchar> baseURL(JSStringCopyUTF8CString(m_baseURL.get()));

    if (m_unreachableURL) {
        GOwnPtr<gchar> unreachableURL(JSStringCopyUTF8CString(m_unreachableURL.get()));
        webkit_web_frame_load_alternate_string(mainFrame, content.get(), baseURL.get(), unreachableURL.get());
        return true;
    }
    webkit_web_frame_load_string(mainFrame, content.get(), 0, 0, baseURL.get());
    return true;
}

bool ReloadItem::invoke() const
{
    webkit_web_frame_reload(mainFrame);
    return true;
}

bool ScriptItem::invoke() const
{
    WebKitWebView* webView = webkit_web_frame_get_web_view(mainFrame);
    gchar* scriptString = JSStringCopyUTF8CString(m_script.get());
    webkit_web_view_execute_script(webView, scriptString);
    g_free(scriptString);
    return true;
}

bool BackForwardItem::invoke() const
{
    WebKitWebView* webView = webkit_web_frame_get_web_view(mainFrame);
    if (m_howFar == 1)
        webkit_web_view_go_forward(webView);
    else if (m_howFar == -1)
        webkit_web_view_go_back(webView);
    else {
        WebKitWebBackForwardList* webBackForwardList = webkit_web_view_get_back_forward_list(webView);
        WebKitWebHistoryItem* item = webkit_web_back_forward_list_get_nth_item(webBackForwardList, m_howFar);
        webkit_web_view_go_to_back_forward_item(webView, item);
    }
    return true;
}
