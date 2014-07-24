/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Holger Hans Peter Freyther
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
#include "InspectorClientQt.h"

#include "Frame.h"
#include "FrameView.h"
#include "InspectorController.h"
#include "InspectorFrontend.h"
#include "InspectorServerQt.h"
#include "NotImplemented.h"
#include "Page.h"
#include "QWebFrameAdapter.h"
#include "QWebPageAdapter.h"
#include "ScriptDebugServer.h"
#include <QCoreApplication>
#include <QFile>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>
#include <QVariant>
#include <wtf/text/CString.h>

namespace WebCore {

static const QLatin1String settingStoragePrefix("Qt/QtWebKit/QWebInspector/");
static const QLatin1String settingStorageTypeSuffix(".type");

namespace {

#if ENABLE(INSPECTOR)
class InspectorFrontendSettingsQt : public InspectorFrontendClientLocal::Settings {
public:
    virtual ~InspectorFrontendSettingsQt() { }
    virtual String getProperty(const String& name)
    {
#ifdef QT_NO_SETTINGS
        Q_UNUSED(name)
        Q_UNUSED(value)
        qWarning("QWebInspector: QSettings is not supported by Qt.");
        return String();
#else
        QSettings qsettings;
        if (qsettings.status() == QSettings::AccessError) {
            // QCoreApplication::setOrganizationName and QCoreApplication::setApplicationName haven't been called
            qWarning("QWebInspector: QSettings couldn't read configuration setting [%s].",
                qPrintable(static_cast<QString>(name)));
            return String();
        }

        QString settingKey(settingStoragePrefix + QString(name));
        QString storedValueType = qsettings.value(settingKey + settingStorageTypeSuffix).toString();
        QVariant storedValue = qsettings.value(settingKey);
        storedValue.convert(QVariant::nameToType(storedValueType.toLatin1().data()));
        return variantToSetting(storedValue);
#endif // QT_NO_SETTINGS
    }

    virtual void setProperty(const String& name, const String& value)
    {
#ifdef QT_NO_SETTINGS
        Q_UNUSED(name)
        Q_UNUSED(value)
        qWarning("QWebInspector: QSettings is not supported by Qt.");
#else
        QSettings qsettings;
        if (qsettings.status() == QSettings::AccessError) {
            qWarning("QWebInspector: QSettings couldn't persist configuration setting [%s].",
                qPrintable(static_cast<QString>(name)));
            return;
        }

        QVariant valueToStore = settingToVariant(value);
        QString settingKey(settingStoragePrefix + QString(name));
        qsettings.setValue(settingKey, valueToStore);
        qsettings.setValue(settingKey + settingStorageTypeSuffix, QLatin1String(QVariant::typeToName(valueToStore.type())));
#endif // QT_NO_SETTINGS
    }

private:
    static String variantToSetting(const QVariant& qvariant)
    {
        String retVal;

        switch (qvariant.type()) {
        case QVariant::Bool:
            retVal = qvariant.toBool() ? "true" : "false";
            break;
        case QVariant::String:
            retVal = qvariant.toString();
            break;
        default:
            break;
        }

        return retVal;
    }

    static QVariant settingToVariant(const String& setting)
    {
        QVariant retVal;
        retVal.setValue(static_cast<QString>(setting));
        return retVal;
    }
};
#endif // ENABLE(INSPECTOR)

}

InspectorClientQt::InspectorClientQt(QWebPageAdapter* page)
    : m_inspectedWebPage(page)
    , m_frontendWebPage(0)
    , m_frontendClient(0)
    , m_remoteFrontEndChannel(0)
{
    InspectorServerQt* webInspectorServer = InspectorServerQt::server();
    if (webInspectorServer)
        webInspectorServer->registerClient(this);
}

void InspectorClientQt::inspectorDestroyed()
{
#if ENABLE(INSPECTOR)
    closeInspectorFrontend();

    InspectorServerQt* webInspectorServer = InspectorServerQt::server();
    if (webInspectorServer)
        webInspectorServer->unregisterClient(this);

    delete this;
#endif
}

    
WebCore::InspectorFrontendChannel* InspectorClientQt::openInspectorFrontend(WebCore::InspectorController* inspectorController)
{
    WebCore::InspectorFrontendChannel* frontendChannel = 0;
#if ENABLE(INSPECTOR)
    QObject* view = 0;
    QWebPageAdapter* inspectorPage = 0;
    m_inspectedWebPage->createWebInspector(&view, &inspectorPage);
    OwnPtr<QObject> inspectorView = adoptPtr(view);

    QObject* inspector = m_inspectedWebPage->inspectorHandle();
    // Remote frontend was attached.
    if (m_remoteFrontEndChannel)
        return 0;

    // This is a known hook that allows changing the default URL for the
    // Web inspector. This is used for SDK purposes. Please keep this hook
    // around and don't remove it.
    // https://bugs.webkit.org/show_bug.cgi?id=35340
    QUrl inspectorUrl;
#ifndef QT_NO_PROPERTIES
    inspectorUrl = inspector->property("_q_inspectorUrl").toUrl();
#endif
    if (!inspectorUrl.isValid())
        inspectorUrl = QUrl(QLatin1String("qrc:/webkit/inspector/inspector.html"));

#ifndef QT_NO_PROPERTIES
    QVariant inspectorJavaScriptWindowObjects = inspector->property("_q_inspectorJavaScriptWindowObjects");
    if (inspectorJavaScriptWindowObjects.isValid())
        inspectorPage->handle()->setProperty("_q_inspectorJavaScriptWindowObjects", inspectorJavaScriptWindowObjects);
#endif
    inspectorPage->mainFrameAdapter()->load(QNetworkRequest(inspectorUrl));
    m_inspectedWebPage->setInspectorFrontend(inspectorView.get());

    // Is 'controller' the same object as 'inspectorController' (which appears to be unused)?
    InspectorController* controller = inspectorPage->page->inspectorController();
    OwnPtr<InspectorFrontendClientQt> frontendClient = adoptPtr(new InspectorFrontendClientQt(m_inspectedWebPage, inspectorView.release(), inspectorPage->page, this));
    m_frontendClient = frontendClient.get();
    controller->setInspectorFrontendClient(frontendClient.release());
    m_frontendWebPage = inspectorPage;

    // Web Inspector should not belong to any other page groups since it is a specialized debugger window.
    m_frontendWebPage->page->setGroupName("__WebInspectorPageGroup__");
    frontendChannel = this;
#endif
    return frontendChannel;
}

void InspectorClientQt::closeInspectorFrontend()
{
#if ENABLE(INSPECTOR)
    if (m_frontendClient)
        m_frontendClient->inspectorClientDestroyed();
#endif
}

void InspectorClientQt::bringFrontendToFront()
{
#if ENABLE(INSPECTOR)
    m_frontendClient->bringToFront();
#endif
}

void InspectorClientQt::releaseFrontendPage()
{
    m_frontendWebPage = 0;
    m_frontendClient = 0;
}

void InspectorClientQt::attachAndReplaceRemoteFrontend(InspectorServerRequestHandlerQt* channel)
{
#if ENABLE(INSPECTOR)
    m_remoteFrontEndChannel = channel;
    m_inspectedWebPage->page->inspectorController()->connectFrontend(this);
#endif
}

void InspectorClientQt::detachRemoteFrontend()
{
#if ENABLE(INSPECTOR)
    m_remoteFrontEndChannel = 0;
    m_inspectedWebPage->page->inspectorController()->disconnectFrontend();
#endif
}

void InspectorClientQt::highlight()
{
    hideHighlight();
}

void InspectorClientQt::hideHighlight()
{
    WebCore::Frame* frame = m_inspectedWebPage->page->mainFrame();
    if (frame) {
        QRect rect = m_inspectedWebPage->mainFrameAdapter()->frameRect();
        if (!rect.isEmpty())
            frame->view()->invalidateRect(rect);
    }
}

bool InspectorClientQt::sendMessageToFrontend(const String& message)
{
#if ENABLE(INSPECTOR)
    if (m_remoteFrontEndChannel) {
        WTF::CString msg = message.utf8();
        m_remoteFrontEndChannel->webSocketSend(msg.data(), msg.length());
        return true;
    }
    if (!m_frontendWebPage)
        return false;

    Page* frontendPage = m_frontendWebPage->page;
    return doDispatchMessageOnFrontendPage(frontendPage, message);
#else
    return false;
#endif
}

#if ENABLE(INSPECTOR)
InspectorFrontendClientQt::InspectorFrontendClientQt(QWebPageAdapter* inspectedWebPage, PassOwnPtr<QObject> inspectorView, WebCore::Page* inspectorPage, InspectorClientQt* inspectorClient)
    : InspectorFrontendClientLocal(inspectedWebPage->page->inspectorController(), inspectorPage, adoptPtr(new InspectorFrontendSettingsQt()))
    , m_inspectedWebPage(inspectedWebPage)
    , m_inspectorView(inspectorView)
    , m_destroyingInspectorView(false)
    , m_inspectorClient(inspectorClient)
{
}

InspectorFrontendClientQt::~InspectorFrontendClientQt()
{
    ASSERT(m_destroyingInspectorView);
    if (m_inspectorClient)
        m_inspectorClient->releaseFrontendPage();
}

void InspectorFrontendClientQt::frontendLoaded()
{
    InspectorFrontendClientLocal::frontendLoaded();
    setAttachedWindow(DOCKED_TO_BOTTOM);
}

String InspectorFrontendClientQt::localizedStringsURL()
{
    notImplemented();
    return String();
}

void InspectorFrontendClientQt::bringToFront()
{
    updateWindowTitle();
}

void InspectorFrontendClientQt::closeWindow()
{
    destroyInspectorView(true);
}

void InspectorFrontendClientQt::attachWindow(DockSide)
{
    notImplemented();
}

void InspectorFrontendClientQt::detachWindow()
{
    notImplemented();
}

void InspectorFrontendClientQt::setAttachedWindowHeight(unsigned)
{
    notImplemented();
}

void InspectorFrontendClientQt::setAttachedWindowWidth(unsigned)
{
    notImplemented();
}

void InspectorFrontendClientQt::setToolbarHeight(unsigned)
{
    notImplemented();
}

void InspectorFrontendClientQt::inspectedURLChanged(const String& newURL)
{
    m_inspectedURL = newURL;
    updateWindowTitle();
}

void InspectorFrontendClientQt::updateWindowTitle()
{
    QString caption = QCoreApplication::translate("QWebPage", "Web Inspector - %2").arg(m_inspectedURL);
    m_inspectedWebPage->setInspectorWindowTitle(caption);
}

void InspectorFrontendClientQt::destroyInspectorView(bool notifyInspectorController)
{
    if (m_destroyingInspectorView)
        return;
    m_destroyingInspectorView = true;

    // Inspected page may have already been destroyed.
    if (m_inspectedWebPage) {
        // Clear reference from QWebInspector to the frontend view.
        m_inspectedWebPage->setInspectorFrontend(0);
    }

#if ENABLE(INSPECTOR)
    if (notifyInspectorController)
        m_inspectedWebPage->page->inspectorController()->disconnectFrontend();
#endif
    if (m_inspectorClient)
        m_inspectorClient->releaseFrontendPage();

    // Clear pointer before deleting WebView to avoid recursive calls to its destructor.
    OwnPtr<QObject> inspectorView = m_inspectorView.release();
}

void InspectorFrontendClientQt::inspectorClientDestroyed()
{
    destroyInspectorView(false);
    m_inspectorClient = 0;
    m_inspectedWebPage = 0;
}
#endif
}

