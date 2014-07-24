/*
 * Copyright (C) 2008, 2012 Gustavo Noronha Silva
 * Copyright (C) 2010 Collabora Ltd.
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
#include "InspectorClientGtk.h"

#include "FileSystem.h"
#include "Frame.h"
#include "InspectorController.h"
#include "NotImplemented.h"
#include "Page.h"
#include "webkitversion.h"
#include "webkitwebinspector.h"
#include "webkitwebinspectorprivate.h"
#include "webkitwebview.h"
#include "webkitwebviewprivate.h"
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

static void notifyWebViewDestroyed(WebKitWebView* webView, InspectorFrontendClient* inspectorFrontendClient)
{
    inspectorFrontendClient->destroyInspectorWindow(true);
}

namespace {

class InspectorFrontendSettingsGtk : public InspectorFrontendClientLocal::Settings {
public:
    virtual ~InspectorFrontendSettingsGtk() { }

private:
    virtual String getProperty(const String& name)
    {
        notImplemented();
        return String();
    }

    virtual void setProperty(const String& name, const String& value)
    {
        notImplemented();
    }
};

} // namespace

InspectorClient::InspectorClient(WebKitWebView* webView)
    : m_inspectedWebView(webView)
    , m_frontendPage(0)
    , m_frontendClient(0)
{}

InspectorClient::~InspectorClient()
{
    if (m_frontendClient) {
        m_frontendClient->disconnectInspectorClient();
        m_frontendClient = 0;
    }
}

void InspectorClient::inspectorDestroyed()
{
    closeInspectorFrontend();
    delete this;
}

InspectorFrontendChannel* InspectorClient::openInspectorFrontend(InspectorController* controller)
{
    // This g_object_get will ref the inspector. We're not doing an
    // unref if this method succeeds because the inspector object must
    // be alive even after the inspected WebView is destroyed - the
    // close-window and destroy signals still need to be
    // emitted.
    WebKitWebInspector* webInspector = 0;
    g_object_get(m_inspectedWebView, "web-inspector", &webInspector, NULL);
    ASSERT(webInspector);

    WebKitWebView* inspectorWebView = 0;
    g_signal_emit_by_name(webInspector, "inspect-web-view", m_inspectedWebView, &inspectorWebView);

    if (!inspectorWebView) {
        g_object_unref(webInspector);
        return 0;
    }

    webkit_web_inspector_set_web_view(webInspector, inspectorWebView);
 
    GOwnPtr<gchar> inspectorPath(g_build_filename(inspectorFilesPath(), "inspector.html", NULL));
    GOwnPtr<gchar> inspectorURI(g_filename_to_uri(inspectorPath.get(), 0, 0));
    webkit_web_view_load_uri(inspectorWebView, inspectorURI.get());

    gtk_widget_show(GTK_WIDGET(inspectorWebView));

    m_frontendPage = core(inspectorWebView);
    OwnPtr<InspectorFrontendClient> frontendClient = adoptPtr(new InspectorFrontendClient(m_inspectedWebView, inspectorWebView, webInspector, m_frontendPage, this));
    m_frontendClient = frontendClient.get();
    m_frontendPage->inspectorController()->setInspectorFrontendClient(frontendClient.release());

    // The inspector must be in it's own PageGroup to avoid deadlock while debugging.
    m_frontendPage->setGroupName("");
    
    return this;
}

void InspectorClient::closeInspectorFrontend()
{
    if (m_frontendClient)
        m_frontendClient->destroyInspectorWindow(false);
}

void InspectorClient::bringFrontendToFront()
{
    m_frontendClient->bringToFront();
}

void InspectorClient::releaseFrontendPage()
{
    m_frontendPage = 0;
    m_frontendClient = 0;
}

void InspectorClient::highlight()
{
    hideHighlight();
}

void InspectorClient::hideHighlight()
{
    // FIXME: we should be able to only invalidate the actual rects of
    // the new and old nodes. We need to track the nodes, and take the
    // actual highlight size into account when calculating the damage
    // rect.
    gtk_widget_queue_draw(GTK_WIDGET(m_inspectedWebView));
}

bool InspectorClient::sendMessageToFrontend(const String& message)
{
    return doDispatchMessageOnFrontendPage(m_frontendPage, message);
}

const char* InspectorClient::inspectorFilesPath()
{
    if (m_inspectorFilesPath)
        return m_inspectorFilesPath.get();

    const char* environmentPath = getenv("WEBKIT_INSPECTOR_PATH");
    if (environmentPath && g_file_test(environmentPath, G_FILE_TEST_IS_DIR))
        m_inspectorFilesPath.set(g_strdup(environmentPath));
    else
        m_inspectorFilesPath.set(g_build_filename(sharedResourcesPath().data(), "webinspector", NULL));

    return m_inspectorFilesPath.get();
}

InspectorFrontendClient::InspectorFrontendClient(WebKitWebView* inspectedWebView, WebKitWebView* inspectorWebView, WebKitWebInspector* webInspector, Page* inspectorPage, InspectorClient* inspectorClient)
    : InspectorFrontendClientLocal(core(inspectedWebView)->inspectorController(), inspectorPage, adoptPtr(new InspectorFrontendSettingsGtk()))
    , m_inspectorWebView(inspectorWebView)
    , m_inspectedWebView(inspectedWebView)
    , m_webInspector(webInspector)
    , m_inspectorClient(inspectorClient)
{
    g_signal_connect(m_inspectorWebView, "destroy",
                     G_CALLBACK(notifyWebViewDestroyed), (gpointer)this);
}

InspectorFrontendClient::~InspectorFrontendClient()
{
    if (m_inspectorClient) {
        m_inspectorClient->releaseFrontendPage();
        m_inspectorClient = 0;
    }

    ASSERT(!m_webInspector);
}

void InspectorFrontendClient::destroyInspectorWindow(bool notifyInspectorController)
{
    if (!m_webInspector)
        return;

    GRefPtr<WebKitWebInspector> webInspector = adoptGRef(m_webInspector.leakRef());

    if (m_inspectorWebView) {
        g_signal_handlers_disconnect_by_func(m_inspectorWebView, reinterpret_cast<gpointer>(notifyWebViewDestroyed), this);
        m_inspectorWebView = 0;
    }

    if (notifyInspectorController)
        core(m_inspectedWebView)->inspectorController()->disconnectFrontend();

    if (m_inspectorClient)
        m_inspectorClient->releaseFrontendPage();

    gboolean handled = FALSE;
    g_signal_emit_by_name(webInspector.get(), "close-window", &handled);
    ASSERT(handled);

    // Please do not use member variables here because InspectorFrontendClient object pointed by 'this'
    // has been implicitly deleted by "close-window" function.
}

String InspectorFrontendClient::localizedStringsURL()
{
    GOwnPtr<gchar> stringsPath(g_build_filename(m_inspectorClient->inspectorFilesPath(), "localizedStrings.js", NULL));
    GOwnPtr<gchar> stringsURI(g_filename_to_uri(stringsPath.get(), 0, 0));

    // FIXME: support l10n of localizedStrings.js
    return String::fromUTF8(stringsURI.get());
}

void InspectorFrontendClient::bringToFront()
{
    if (!m_inspectorWebView)
        return;

    gboolean handled = FALSE;
    g_signal_emit_by_name(m_webInspector.get(), "show-window", &handled);
}

void InspectorFrontendClient::closeWindow()
{
    destroyInspectorWindow(true);
}

void InspectorFrontendClient::attachWindow(DockSide)
{
    if (!m_inspectorWebView)
        return;

    gboolean handled = FALSE;
    g_signal_emit_by_name(m_webInspector.get(), "attach-window", &handled);
}

void InspectorFrontendClient::detachWindow()
{
    if (!m_inspectorWebView)
        return;

    gboolean handled = FALSE;
    g_signal_emit_by_name(m_webInspector.get(), "detach-window", &handled);
}

void InspectorFrontendClient::setAttachedWindowHeight(unsigned height)
{
    notImplemented();
}

void InspectorFrontendClient::setAttachedWindowWidth(unsigned width)
{
    notImplemented();
}

void InspectorFrontendClient::setToolbarHeight(unsigned height)
{
    notImplemented();
}

void InspectorFrontendClient::inspectedURLChanged(const String& newURL)
{
    if (!m_inspectorWebView)
        return;

    webkit_web_inspector_set_inspected_uri(m_webInspector.get(), newURL.utf8().data());
}

}

