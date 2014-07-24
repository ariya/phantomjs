/*
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2011 ProFUSION Embedded Systems
 * Copyright (C) 2011 Samsung Electronics
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
#include "DumpRenderTreeChrome.h"

#include <EWebKit.h>
#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/OpaqueJSString.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

bool LoadItem::invoke() const
{
    Evas_Object* targetFrame;

    if (!m_target->length())
        targetFrame = browser->mainFrame();
    else
        targetFrame = ewk_frame_child_find(browser->mainFrame(), m_target->string().utf8().data());

    ewk_frame_uri_set(targetFrame, m_url->string().utf8().data());

    return true;
}

bool LoadHTMLStringItem::invoke() const
{
    if (!m_unreachableURL->length())
        ewk_frame_contents_set(browser->mainFrame(), m_content->string().utf8().data(), 0, 0, 0, m_baseURL->string().utf8().data());
    else
        ewk_frame_contents_alternate_set(browser->mainFrame(), m_content->string().utf8().data(), 0, 0, 0, m_baseURL->string().utf8().data(), m_unreachableURL->string().utf8().data());

    return true;
}

bool ReloadItem::invoke() const
{
    ewk_view_reload(browser->mainView());
    return true;
}

bool ScriptItem::invoke() const
{
    return ewk_frame_script_execute(browser->mainFrame(), m_script->string().utf8().data());
}

bool BackForwardItem::invoke() const
{
    if (m_howFar == 1)
        ewk_view_forward(browser->mainView());
    else if (m_howFar == -1)
        ewk_view_back(browser->mainView());
    else
        ewk_view_navigate(browser->mainView(), m_howFar);

    return true;
}
