/*
 * Copyright (C) 2009, 2010, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "WorkQueueItem.h"

#include "DumpRenderTreeBlackBerry.h"
#include "Frame.h"
#include "FrameLoadRequest.h"
#include "KURL.h"
#include "WebPage.h"
#include <wtf/OwnArrayPtr.h>

using namespace WebCore;

bool LoadItem::invoke() const
{
    size_t targetArrSize = JSStringGetMaximumUTF8CStringSize(m_target.get());
    size_t urlArrSize = JSStringGetMaximumUTF8CStringSize(m_url.get());
    OwnArrayPtr<char> target = adoptArrayPtr(new char[targetArrSize]);
    OwnArrayPtr<char> url = adoptArrayPtr(new char[urlArrSize]);
    size_t targetLen = JSStringGetUTF8CString(m_target.get(), target.get(), targetArrSize) - 1;
    JSStringGetUTF8CString(m_url.get(), url.get(), urlArrSize);

    Frame* frame;
    if (target && targetLen)
        frame = mainFrame->tree()->find(target.get());
    else
        frame = mainFrame;

    if (!frame)
        return false;

    KURL kurl = KURL(KURL(), url.get());
    frame->loader()->load(FrameLoadRequest(frame, ResourceRequest(kurl)));
    return true;
}

bool LoadHTMLStringItem::invoke() const
{
    size_t contentSize = JSStringGetMaximumUTF8CStringSize(m_content.get());
    size_t baseURLSize = JSStringGetMaximumUTF8CStringSize(m_baseURL.get());
    size_t unreachableURLSize = JSStringGetMaximumUTF8CStringSize(m_unreachableURL.get());
    OwnArrayPtr<char> content = adoptArrayPtr(new char[contentSize]);
    OwnArrayPtr<char> baseURL = adoptArrayPtr(new char[baseURLSize]);
    OwnArrayPtr<char> unreachableURL = adoptArrayPtr(new char[unreachableURLSize]);
    JSStringGetUTF8CString(m_content.get(), content.get(), contentSize);
    JSStringGetUTF8CString(m_baseURL.get(), baseURL.get(), baseURLSize);
    JSStringGetUTF8CString(m_unreachableURL.get(), unreachableURL.get(), unreachableURLSize);
    STATIC_LOCAL_STRING(s_textHtml, "text/html");
    BlackBerry::WebKit::DumpRenderTree::currentInstance()->page()->loadString(BlackBerry::Platform::String::fromUtf8(content.get()), BlackBerry::Platform::String::fromUtf8(baseURL.get()), s_textHtml, unreachableURLSize ? BlackBerry::Platform::String::fromUtf8(unreachableURL.get()) : BlackBerry::Platform::String::emptyString());
    return true;
}

bool ReloadItem::invoke() const
{
    mainFrame->loader()->reload(true);
    return true;
}

bool ScriptItem::invoke() const
{
    BlackBerry::WebKit::JavaScriptDataType type;
    BlackBerry::Platform::String result;
    size_t scriptArrSize = JSStringGetMaximumUTF8CStringSize(m_script.get());
    OwnArrayPtr<char> script = adoptArrayPtr(new char[scriptArrSize]);
    JSStringGetUTF8CString(m_script.get(), script.get(), scriptArrSize);
    BlackBerry::WebKit::DumpRenderTree::currentInstance()->page()->executeJavaScript(BlackBerry::Platform::String::fromRawData(script.get(), scriptArrSize), type, result);
    return true;
}

bool BackForwardItem::invoke() const
{
    return BlackBerry::WebKit::DumpRenderTree::currentInstance()->page()->goBackOrForward(m_howFar);
}

