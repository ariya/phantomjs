/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Matt Lilek <webkit@mattlilek.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InspectorFrontendHost.h"

#if ENABLE(INSPECTOR)

#include "ContextMenu.h"
#include "ContextMenuItem.h"
#include "ContextMenuController.h"
#include "ContextMenuProvider.h"
#include "Element.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "HitTestResult.h"
#include "HTMLFrameOwnerElement.h"
#include "InspectorAgent.h"
#include "InspectorController.h"
#include "InspectorFrontendClient.h"
#include "Page.h"
#include "Pasteboard.h"
#include "ScriptFunctionCall.h"

#include <wtf/RefPtr.h>
#include <wtf/StdLibExtras.h>

using namespace std;

namespace WebCore {

#if ENABLE(CONTEXT_MENUS)
class FrontendMenuProvider : public ContextMenuProvider {
public:
    static PassRefPtr<FrontendMenuProvider> create(InspectorFrontendHost* frontendHost, ScriptObject webInspector, const Vector<ContextMenuItem*>& items)
    {
        return adoptRef(new FrontendMenuProvider(frontendHost, webInspector, items));
    }
    
    void disconnect()
    {
        m_webInspector = ScriptObject();
        m_frontendHost = 0;
    }
    
private:
    FrontendMenuProvider(InspectorFrontendHost* frontendHost, ScriptObject webInspector,  const Vector<ContextMenuItem*>& items)
        : m_frontendHost(frontendHost)
        , m_webInspector(webInspector)
        , m_items(items)
    {
    }

    virtual ~FrontendMenuProvider()
    {
        contextMenuCleared();
    }
    
    virtual void populateContextMenu(ContextMenu* menu)
    {
        for (size_t i = 0; i < m_items.size(); ++i)
            menu->appendItem(*m_items[i]);
    }
    
    virtual void contextMenuItemSelected(ContextMenuItem* item)
    {
        if (m_frontendHost) {
            int itemNumber = item->action() - ContextMenuItemBaseCustomTag;

            ScriptFunctionCall function(m_webInspector, "contextMenuItemSelected");
            function.appendArgument(itemNumber);
            function.call();
        }
    }
    
    virtual void contextMenuCleared()
    {
        if (m_frontendHost) {
            ScriptFunctionCall function(m_webInspector, "contextMenuCleared");
            function.call();

            m_frontendHost->m_menuProvider = 0;
        }
        deleteAllValues(m_items);
        m_items.clear();
    }

    InspectorFrontendHost* m_frontendHost;
    ScriptObject m_webInspector;
    Vector<ContextMenuItem*> m_items;
};
#endif

InspectorFrontendHost::InspectorFrontendHost(InspectorFrontendClient* client, Page* frontendPage)
    : m_client(client)
    , m_frontendPage(frontendPage)
#if ENABLE(CONTEXT_MENUS)
    , m_menuProvider(0)
#endif
{
}

InspectorFrontendHost::~InspectorFrontendHost()
{
    ASSERT(!m_client);
}

void InspectorFrontendHost::disconnectClient()
{
    m_client = 0;
#if ENABLE(CONTEXT_MENUS)
    if (m_menuProvider)
        m_menuProvider->disconnect();
#endif
    m_frontendPage = 0;
}

void InspectorFrontendHost::loaded()
{
    if (m_client)
        m_client->frontendLoaded();
}

void InspectorFrontendHost::requestAttachWindow()
{
    if (m_client)
        m_client->requestAttachWindow();
}

void InspectorFrontendHost::requestDetachWindow()
{
    if (m_client)
        m_client->requestDetachWindow();
}

void InspectorFrontendHost::closeWindow()
{
    if (m_client) {
        m_client->closeWindow();
        disconnectClient(); // Disconnect from client.
    }
}

void InspectorFrontendHost::disconnectFromBackend()
{
    if (m_client) {
        m_client->disconnectFromBackend();
        disconnectClient(); // Disconnect from client.
    }
}

void InspectorFrontendHost::bringToFront()
{
    if (m_client)
        m_client->bringToFront();
}

void InspectorFrontendHost::inspectedURLChanged(const String& newURL)
{
    if (m_client)
        m_client->inspectedURLChanged(newURL);
}

void InspectorFrontendHost::setAttachedWindowHeight(unsigned height)
{
    if (m_client)
        m_client->changeAttachedWindowHeight(height);
}

void InspectorFrontendHost::moveWindowBy(float x, float y) const
{
    if (m_client)
        m_client->moveWindowBy(x, y);
}

void InspectorFrontendHost::setExtensionAPI(const String& script)
{
    ASSERT(m_frontendPage->inspectorController());
    m_frontendPage->inspectorController()->setInspectorExtensionAPI(script);
}

String InspectorFrontendHost::localizedStringsURL()
{
    return m_client->localizedStringsURL();
}

String InspectorFrontendHost::hiddenPanels()
{
    return m_client->hiddenPanels();
}

void InspectorFrontendHost::copyText(const String& text)
{
    Pasteboard::generalPasteboard()->writePlainText(text);
}

void InspectorFrontendHost::saveAs(const String& fileName, const String& content)
{
    if (m_client)
        m_client->saveAs(fileName, content);
}

void InspectorFrontendHost::saveSessionSetting(const String& key, const String& value)
{
    if (m_client)
        m_client->saveSessionSetting(key, value);
}

String InspectorFrontendHost::loadSessionSetting(const String& key)
{
    String value;
    if (m_client)
        m_client->loadSessionSetting(key, &value);
    return value;
}

void InspectorFrontendHost::sendMessageToBackend(const String& message)
{
    m_client->sendMessageToBackend(message);
}

#if ENABLE(CONTEXT_MENUS)
void InspectorFrontendHost::showContextMenu(Event* event, const Vector<ContextMenuItem*>& items)
{
    ASSERT(m_frontendPage);
    ScriptState* frontendScriptState = scriptStateFromPage(debuggerWorld(), m_frontendPage);
    ScriptObject webInspectorObj;
    if (!ScriptGlobalObject::get(frontendScriptState, "WebInspector", webInspectorObj)) {
        ASSERT_NOT_REACHED();
        return;
    }
    RefPtr<FrontendMenuProvider> menuProvider = FrontendMenuProvider::create(this, webInspectorObj, items);
    ContextMenuController* menuController = m_frontendPage->contextMenuController();
    menuController->showContextMenu(event, menuProvider);
    m_menuProvider = menuProvider.get();
}
#endif

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
