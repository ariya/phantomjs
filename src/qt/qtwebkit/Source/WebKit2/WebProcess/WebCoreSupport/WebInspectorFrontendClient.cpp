/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebInspectorFrontendClient.h"

#if ENABLE(INSPECTOR)

#include "WebInspector.h"
#include "WebPage.h"
#include <WebCore/InspectorController.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/Page.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

WebInspectorFrontendClient::WebInspectorFrontendClient(WebPage* page, WebPage* inspectorPage)
    : InspectorFrontendClientLocal(page->corePage()->inspectorController(), inspectorPage->corePage(), adoptPtr(new Settings()))
    , m_page(page)
{
}

String WebInspectorFrontendClient::localizedStringsURL()
{
    return m_page->inspector()->localizedStringsURL();
}

void WebInspectorFrontendClient::bringToFront()
{
    m_page->inspector()->bringToFront();
}

void WebInspectorFrontendClient::closeWindow()
{
    m_page->corePage()->inspectorController()->disconnectFrontend();
    m_page->inspector()->didClose();
}

bool WebInspectorFrontendClient::canSave()
{
    return m_page->inspector()->canSave();
}

void WebInspectorFrontendClient::save(const String& filename, const String& content, bool forceSaveAs)
{
    m_page->inspector()->save(filename, content, forceSaveAs);
}

void WebInspectorFrontendClient::append(const String& filename, const String& content)
{
    m_page->inspector()->append(filename, content);
}

void WebInspectorFrontendClient::attachWindow(DockSide dockSide)
{
    switch (dockSide) {
    case InspectorFrontendClient::UNDOCKED:
        ASSERT_NOT_REACHED();
        break;
    case InspectorFrontendClient::DOCKED_TO_BOTTOM:
        m_page->inspector()->attachBottom();
        break;
    case InspectorFrontendClient::DOCKED_TO_RIGHT:
        m_page->inspector()->attachRight();
        break;
    }
}

void WebInspectorFrontendClient::detachWindow()
{
    m_page->inspector()->detach();
}

void WebInspectorFrontendClient::setAttachedWindowHeight(unsigned height)
{
    m_page->inspector()->setAttachedWindowHeight(height);
}

void WebInspectorFrontendClient::setAttachedWindowWidth(unsigned width)
{
    m_page->inspector()->setAttachedWindowWidth(width);
}

void WebInspectorFrontendClient::setToolbarHeight(unsigned height)
{
    m_page->inspector()->setToolbarHeight(height);
}

void WebInspectorFrontendClient::inspectedURLChanged(const String& urlString)
{
    m_page->inspector()->inspectedURLChanged(urlString);
}

} // namespace WebKit

#endif // ENABLE(INSPECTOR)
