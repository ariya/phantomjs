/*
 *  Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
 *  Copyright (C) 2009-2010 ProFUSION embedded systems
 *  Copyright (C) 2009-2010 Samsung Electronics
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "InspectorClientEfl.h"

#if ENABLE(INSPECTOR)
#include "EflInspectorUtilities.h"
#include "InspectorController.h"
#include "NotImplemented.h"
#include "ewk_view_private.h"
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

static void notifyInspectorDestroy(void* userData, Evas_Object* /*webview*/, void* /*eventInfo*/)
{
    InspectorFrontendClientEfl* inspectorFrontendClient = static_cast<InspectorFrontendClientEfl*>(userData);
    if (inspectorFrontendClient)
        inspectorFrontendClient->destroyInspectorWindow(true);
}

static void invalidateView(Evas_Object* webView)
{
    Evas_Coord width, height;
    Evas_Object* mainFrame = ewk_view_frame_main_get(webView);
    if (mainFrame && ewk_frame_contents_size_get(mainFrame, &width, &height)) {
        WebCore::Page* page = EWKPrivate::corePage(webView);
        if (page)
            page->mainFrame()->view()->invalidateRect(WebCore::IntRect(0, 0, width, height));
    }
}

class InspectorFrontendSettingsEfl : public InspectorFrontendClientLocal::Settings {
public:
    virtual String getProperty(const String& /*name*/)
    {
        notImplemented();
        return String();
    }

    virtual void setProperty(const String& /*name*/, const String& /*value*/)
    {
        notImplemented();
    }
};

InspectorClientEfl::InspectorClientEfl(Evas_Object* webView)
    : m_inspectedView(webView)
    , m_inspectorView(0)
    , m_frontendClient(0)
{
}

InspectorClientEfl::~InspectorClientEfl()
{
    if (m_frontendClient) {
        m_frontendClient->disconnectInspectorClient();
        m_frontendClient = 0;
    }
}

void InspectorClientEfl::inspectorDestroyed()
{
    closeInspectorFrontend();
    delete this;
}

InspectorFrontendChannel* InspectorClientEfl::openInspectorFrontend(InspectorController*)
{
    evas_object_smart_callback_call(m_inspectedView, "inspector,view,create", 0);

    Evas_Object* inspectorView = ewk_view_inspector_view_get(m_inspectedView);
    if (!inspectorView)
        return 0;

    m_inspectorView = inspectorView;

    String inspectorUri = inspectorFilesPath() + "/inspector.html";
    ewk_view_uri_set(m_inspectorView, inspectorUri.utf8().data());

    OwnPtr<InspectorFrontendClientEfl> frontendClient = adoptPtr(new InspectorFrontendClientEfl(m_inspectedView, m_inspectorView, this));
    m_frontendClient = frontendClient.get();

    InspectorController* controller = EWKPrivate::corePage(m_inspectorView)->inspectorController();
    controller->setInspectorFrontendClient(frontendClient.release());
    
    return this;
}

void InspectorClientEfl::closeInspectorFrontend()
{
    if (m_frontendClient)
        m_frontendClient->destroyInspectorWindow(false);
}

void InspectorClientEfl::bringFrontendToFront()
{
    m_frontendClient->bringToFront();
}

void InspectorClientEfl::highlight()
{
    invalidateView(m_inspectedView);
}

void InspectorClientEfl::hideHighlight()
{
    invalidateView(m_inspectedView);
}

bool InspectorClientEfl::sendMessageToFrontend(const String& message)
{
    Page* frontendPage = EWKPrivate::corePage(m_inspectorView);
    return doDispatchMessageOnFrontendPage(frontendPage, message);
}

void InspectorClientEfl::releaseFrontendPage()
{
    m_inspectorView = 0;
    m_frontendClient = 0;
}

String InspectorClientEfl::inspectorFilesPath()
{
    return "file://" + inspectorResourcePath();
}

InspectorFrontendClientEfl::InspectorFrontendClientEfl(Evas_Object* inspectedView, Evas_Object* inspectorView, InspectorClientEfl* inspectorClient)
    : InspectorFrontendClientLocal(EWKPrivate::corePage(inspectedView)->inspectorController(), EWKPrivate::corePage(inspectorView), adoptPtr(new InspectorFrontendSettingsEfl()))
    , m_inspectedView(inspectedView)
    , m_inspectorView(inspectorView)
    , m_inspectorClient(inspectorClient)
{
    evas_object_smart_callback_add(m_inspectorView, "inspector,view,destroy", notifyInspectorDestroy, this);
}

InspectorFrontendClientEfl::~InspectorFrontendClientEfl()
{
    evas_object_smart_callback_del(m_inspectorView, "inspector,view,destroy", notifyInspectorDestroy);

    if (m_inspectorClient) {
        m_inspectorClient->releaseFrontendPage();
        m_inspectorClient = 0;
    }
}

String InspectorFrontendClientEfl::localizedStringsURL()
{
    return m_inspectorClient->inspectorFilesPath() + "/localizedStrings.js";
}

void InspectorFrontendClientEfl::bringToFront()
{
    evas_object_focus_set(m_inspectorView, true);
}

void InspectorFrontendClientEfl::closeWindow()
{
    destroyInspectorWindow(true);
}

void InspectorFrontendClientEfl::inspectedURLChanged(const String&)
{
    notImplemented();
}

void InspectorFrontendClientEfl::attachWindow(DockSide)
{
    notImplemented();
}

void InspectorFrontendClientEfl::detachWindow()
{
    notImplemented();
}

void InspectorFrontendClientEfl::setAttachedWindowHeight(unsigned)
{
    notImplemented();
}

void InspectorFrontendClientEfl::setAttachedWindowWidth(unsigned)
{
    notImplemented();
}

void InspectorFrontendClientEfl::setToolbarHeight(unsigned)
{
    notImplemented();
}

void InspectorFrontendClientEfl::destroyInspectorWindow(bool notifyInspectorController)
{
    if (notifyInspectorController)
        EWKPrivate::corePage(m_inspectedView)->inspectorController()->disconnectFrontend();

    if (m_inspectorClient)
        m_inspectorClient->releaseFrontendPage();

    evas_object_smart_callback_call(m_inspectedView, "inspector,view,close", m_inspectorView);
}

}
#endif
