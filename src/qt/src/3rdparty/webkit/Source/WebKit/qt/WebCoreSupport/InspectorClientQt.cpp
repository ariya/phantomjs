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
#include "PlatformString.h"
#include "ScriptDebugServer.h"
#include "qwebinspector.h"
#include "qwebinspector_p.h"
#include "qwebpage.h"
#include "qwebpage_p.h"
#include "qwebview.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtCore/QVariant>

namespace WebCore {

static const QLatin1String settingStoragePrefix("Qt/QtWebKit/QWebInspector/");
static const QLatin1String settingStorageTypeSuffix(".type");

class InspectorClientWebPage : public QWebPage {
    Q_OBJECT
    friend class InspectorClientQt;
public:
    InspectorClientWebPage(QObject* parent = 0)
        : QWebPage(parent)
    {
        connect(mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), SLOT(javaScriptWindowObjectCleared()));
    }

    QWebPage* createWindow(QWebPage::WebWindowType)
    {
        QWebView* view = new QWebView;
        QWebPage* page = new QWebPage;
        view->setPage(page);
        view->setAttribute(Qt::WA_DeleteOnClose);
        return page;
    }

public slots:
    void javaScriptWindowObjectCleared() 
    {
#ifndef QT_NO_PROPERTIES
        QVariant inspectorJavaScriptWindowObjects = property("_q_inspectorJavaScriptWindowObjects");
        if (!inspectorJavaScriptWindowObjects.isValid())
            return;
        QMap<QString, QVariant> javaScriptNameObjectMap = inspectorJavaScriptWindowObjects.toMap();
        QWebFrame* frame = mainFrame();
        QMap<QString, QVariant>::const_iterator it = javaScriptNameObjectMap.constBegin();
        for ( ; it != javaScriptNameObjectMap.constEnd(); ++it) {
            QString name = it.key();
            QVariant value = it.value();
            QObject* obj = value.value<QObject*>();
            frame->addToJavaScriptWindowObject(name, obj);
        }
#endif
    }
};

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
        storedValue.convert(QVariant::nameToType(storedValueType.toAscii().data()));
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

InspectorClientQt::InspectorClientQt(QWebPage* page)
    : m_inspectedWebPage(page)
    , m_frontendWebPage(0)
    , m_frontendClient(0)
{
    InspectorServerQt* webInspectorServer = InspectorServerQt::server();
    if (webInspectorServer)
        webInspectorServer->registerClient(this);
}

void InspectorClientQt::inspectorDestroyed()
{
#if ENABLE(INSPECTOR)
    if (m_frontendClient)
        m_frontendClient->inspectorClientDestroyed();

    InspectorServerQt* webInspectorServer = InspectorServerQt::server();
    if (webInspectorServer)
        webInspectorServer->unregisterClient(this);

    delete this;
#endif
}

    
void InspectorClientQt::openInspectorFrontend(WebCore::InspectorController* inspectorController)
{
#if ENABLE(INSPECTOR)
    QWebView* inspectorView = new QWebView;
    InspectorClientWebPage* inspectorPage = new InspectorClientWebPage(inspectorView);
    inspectorView->setPage(inspectorPage);

    QWebInspector* inspector = m_inspectedWebPage->d->getOrCreateInspector();
    // Remote frontend was attached.
    if (m_inspectedWebPage->d->inspector->d->remoteFrontend)
        return;

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
        inspectorPage->setProperty("_q_inspectorJavaScriptWindowObjects", inspectorJavaScriptWindowObjects);
#endif
    inspectorView->page()->mainFrame()->load(inspectorUrl);
    m_inspectedWebPage->d->inspectorFrontend = inspectorView;
    inspector->d->setFrontend(inspectorView);

    m_frontendClient = new InspectorFrontendClientQt(m_inspectedWebPage, inspectorView, this);
    inspectorView->page()->d->page->inspectorController()->setInspectorFrontendClient(m_frontendClient);
    m_frontendWebPage = inspectorPage;
#endif
}

void InspectorClientQt::releaseFrontendPage()
{
    m_frontendWebPage = 0;
    m_frontendClient = 0;
}

void InspectorClientQt::attachAndReplaceRemoteFrontend(RemoteFrontendChannel* channel)
{
#if ENABLE(INSPECTOR)
    // Channel was allocated by InspectorServerQt. Here we transfer ownership to inspector.
    m_inspectedWebPage->d->inspector->d->attachAndReplaceRemoteFrontend(channel);
    m_inspectedWebPage->d->inspectorController()->connectFrontend();
#endif
}

void InspectorClientQt::detachRemoteFrontend()
{
#if ENABLE(INSPECTOR)
    m_inspectedWebPage->d->inspector->d->detachRemoteFrontend();
    m_inspectedWebPage->d->inspectorController()->disconnectFrontend();
#endif
}

void InspectorClientQt::highlight(Node*)
{
    hideHighlight();
}

void InspectorClientQt::hideHighlight()
{
    WebCore::Frame* frame = m_inspectedWebPage->d->page->mainFrame();
    if (frame) {
        QRect rect = m_inspectedWebPage->mainFrame()->geometry();
        if (!rect.isEmpty())
            frame->view()->invalidateRect(rect);
    }
}

bool InspectorClientQt::sendMessageToFrontend(const String& message)
{
#if ENABLE(INSPECTOR)
    if (m_inspectedWebPage->d->inspector->d->remoteFrontend) {
        RemoteFrontendChannel* session = qobject_cast<RemoteFrontendChannel*>(m_inspectedWebPage->d->inspector->d->remoteFrontend);
        if (session)
            session->sendMessageToFrontend(message);
        return true;
    }
    if (!m_frontendWebPage)
        return false;

    Page* frontendPage = QWebPagePrivate::core(m_frontendWebPage);
    return doDispatchMessageOnFrontendPage(frontendPage, message);
#else
    return false;
#endif
}

#if ENABLE(INSPECTOR)
InspectorFrontendClientQt::InspectorFrontendClientQt(QWebPage* inspectedWebPage, PassOwnPtr<QWebView> inspectorView, InspectorClientQt* inspectorClient)
    : InspectorFrontendClientLocal(inspectedWebPage->d->page->inspectorController(), inspectorView->page()->d->page, new InspectorFrontendSettingsQt())
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
    setAttachedWindow(true);
}

String InspectorFrontendClientQt::localizedStringsURL()
{
    notImplemented();
    return String();
}

String InspectorFrontendClientQt::hiddenPanels()
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

void InspectorFrontendClientQt::disconnectFromBackend()
{
    destroyInspectorView(false);
}

void InspectorFrontendClientQt::attachWindow()
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

void InspectorFrontendClientQt::inspectedURLChanged(const String& newURL)
{
    m_inspectedURL = newURL;
    updateWindowTitle();
}

void InspectorFrontendClientQt::updateWindowTitle()
{
    if (m_inspectedWebPage->d->inspector) {
        QString caption = QCoreApplication::translate("QWebPage", "Web Inspector - %2").arg(m_inspectedURL);
        m_inspectedWebPage->d->inspector->setWindowTitle(caption);
    }
}

void InspectorFrontendClientQt::destroyInspectorView(bool notifyInspectorController)
{
    if (m_destroyingInspectorView)
        return;
    m_destroyingInspectorView = true;

    // Inspected page may have already been destroyed.
    if (m_inspectedWebPage) {
        // Clear reference from QWebInspector to the frontend view.
        m_inspectedWebPage->d->getOrCreateInspector()->d->setFrontend(0);
    }

#if ENABLE(INSPECTOR)
    if (notifyInspectorController)
        m_inspectedWebPage->d->inspectorController()->disconnectFrontend();
#endif
    if (m_inspectorClient)
        m_inspectorClient->releaseFrontendPage();

    // Clear pointer before deleting WebView to avoid recursive calls to its destructor.
    OwnPtr<QWebView> inspectorView = m_inspectorView.release();
}

void InspectorFrontendClientQt::inspectorClientDestroyed()
{
    m_inspectorClient = 0;
    m_inspectedWebPage = 0;
}
#endif
}

#include "InspectorClientQt.moc"
