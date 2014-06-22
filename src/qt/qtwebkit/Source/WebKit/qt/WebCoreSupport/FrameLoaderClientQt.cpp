/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Collabora Ltd. All rights reserved.
 * Coypright (C) 2008 Holger Hans Peter Freyther
 * Coypright (C) 2009, 2010 Girish Ramakrishnan <girish@forwardbias.in>
 *
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "FrameLoaderClientQt.h"

#include "CSSComputedStyleDeclaration.h"
#include "CSSPropertyNames.h"
#include "DocumentLoader.h"
#include "EventHandler.h"
#include "FormState.h"
#include "FrameLoadRequest.h"
#include "FrameNetworkingContextQt.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "HTMLAppletElement.h"
#include "HTMLFormElement.h"
#include "HTMLPlugInElement.h"
#include "HTTPParsers.h"
#include "HTTPStatusCodes.h"
#include "HistoryItem.h"
#include "HitTestResult.h"
#if ENABLE(ICONDATABASE)
#include "IconDatabaseClientQt.h"
#endif
#include "JSDOMWindowBase.h"
#include "MIMETypeRegistry.h"
#include "MouseEvent.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PlatformMouseEvent.h"
#include "PluginData.h"
#include "PluginDatabase.h"
#include "PolicyChecker.h"
#include "ProgressTracker.h"
#include "QNetworkReplyHandler.h"
#include "QWebFrameAdapter.h"
#include "QWebPageAdapter.h"
#include "QWebPageClient.h"
#include "QtPluginWidgetAdapter.h"
#include "RenderPart.h"
#include "ResourceHandle.h"
#include "ResourceHandleInternal.h"
#include "ResourceLoader.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "ScriptController.h"
#include "Settings.h"
#include "ViewportArguments.h"
#include "WebEventConversion.h"
#include "qwebhistory.h"
#include "qwebhistory_p.h"
#include "qwebhistoryinterface.h"
#include "qwebpluginfactory.h"
#include "qwebsettings.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QMouseEvent>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <wtf/OwnPtr.h>
#include <wtf/text/StringBuilder.h>

static QMap<unsigned long, QString> dumpAssignedUrls;

// Compare with the file "WebKit/Tools/DumpRenderTree/mac/FrameLoadDelegate.mm".
static QString drtDescriptionSuitableForTestResult(WebCore::Frame* webCoreFrame)
{
    QWebFrameAdapter* frame = QWebFrameAdapter::kit(webCoreFrame);
    QString name = webCoreFrame->tree()->uniqueName();

    bool isMainFrame = frame == frame->pageAdapter->mainFrameAdapter();
    if (isMainFrame) {
        if (!name.isEmpty())
            return QString::fromLatin1("main frame \"%1\"").arg(name);
        return QLatin1String("main frame");
    }
    if (!name.isEmpty())
        return QString::fromLatin1("frame \"%1\"").arg(name);
    return QLatin1String("frame (anonymous)");
}

static QString drtPrintFrameUserGestureStatus(WebCore::Frame* frame)
{
    if (WebCore::ScriptController::processingUserGesture())
        return QString::fromLatin1("Frame with user gesture \"%1\"").arg(QLatin1String("true"));
    return QString::fromLatin1("Frame with user gesture \"%1\"").arg(QLatin1String("false"));
}

static QString drtDescriptionSuitableForTestResult(const WebCore::KURL& kurl)
{
    if (kurl.isEmpty() || !kurl.isLocalFile())
        return kurl.string();
    // Remove the leading path from file urls.
    return QString(kurl.string()).remove(WebCore::FrameLoaderClientQt::dumpResourceLoadCallbacksPath).mid(1);
}

static QString drtDescriptionSuitableForTestResult(const WebCore::ResourceError& error)
{
    QString failingURL = error.failingURL();
    return QString::fromLatin1("<NSError domain NSURLErrorDomain, code %1, failing URL \"%2\">").arg(error.errorCode()).arg(failingURL);
}

static QString drtDescriptionSuitableForTestResult(const WebCore::ResourceRequest& request)
{
    QString url = drtDescriptionSuitableForTestResult(request.url());
    QString httpMethod = request.httpMethod();
    QString mainDocumentUrl = drtDescriptionSuitableForTestResult(request.firstPartyForCookies());
    return QString::fromLatin1("<NSURLRequest URL %1, main document URL %2, http method %3>").arg(url).arg(mainDocumentUrl).arg(httpMethod);
}

static QString drtDescriptionSuitableForTestResult(const WebCore::ResourceResponse& response)
{
    QString url = drtDescriptionSuitableForTestResult(response.url());
    int httpStatusCode = response.httpStatusCode();
    return QString::fromLatin1("<NSURLResponse %1, http status code %2>").arg(url).arg(httpStatusCode);
}

static QString drtDescriptionSuitableForTestResult(const RefPtr<WebCore::Node> node, int exception)
{
    QString result;
    if (exception) {
        result.append(QLatin1String("ERROR"));
        return result;
    }
    if (!node) {
        result.append(QLatin1String("NULL"));
        return result;
    }
    result.append(node->nodeName());
    RefPtr<WebCore::Node> parent = node->parentNode();
    if (parent) {
        result.append(QLatin1String(" > "));
        result.append(drtDescriptionSuitableForTestResult(parent, 0));
    }
    return result;
}

namespace WebCore {

bool FrameLoaderClientQt::dumpFrameLoaderCallbacks = false;
bool FrameLoaderClientQt::dumpProgressFinishedCallback = false;
bool FrameLoaderClientQt::dumpUserGestureInFrameLoaderCallbacks = false;
bool FrameLoaderClientQt::dumpWillCacheResponseCallbacks = false;
bool FrameLoaderClientQt::dumpResourceLoadCallbacks = false;
bool FrameLoaderClientQt::sendRequestReturnsNullOnRedirect = false;
bool FrameLoaderClientQt::sendRequestReturnsNull = false;
bool FrameLoaderClientQt::dumpResourceResponseMIMETypes = false;
bool FrameLoaderClientQt::deferMainResourceDataLoad = true;
bool FrameLoaderClientQt::dumpHistoryCallbacks = false;

QStringList FrameLoaderClientQt::sendRequestClearHeaders;
QString FrameLoaderClientQt::dumpResourceLoadCallbacksPath;
bool FrameLoaderClientQt::policyDelegateEnabled = false;
bool FrameLoaderClientQt::policyDelegatePermissive = false;
QMap<QString, QString> FrameLoaderClientQt::URLsToRedirect = QMap<QString, QString>();

// Taken from the file "WebKit/Tools/DumpRenderTree/chromium/WebViewHost.cpp".
static const char* navigationTypeToString(NavigationType type)
{
    switch (type) {
    case NavigationTypeLinkClicked:
        return "link clicked";
    case NavigationTypeFormSubmitted:
        return "form submitted";
    case NavigationTypeBackForward:
        return "back/forward";
    case NavigationTypeReload:
        return "reload";
    case NavigationTypeFormResubmitted:
        return "form resubmitted";
    case NavigationTypeOther:
        return "other";
    }
    return "illegal value";
}

FrameLoaderClientQt::FrameLoaderClientQt()
    : m_frame(0)
    , m_webFrame(0)
    , m_pluginView(0)
    , m_hasSentResponseToPlugin(false)
    , m_isOriginatingLoad(false)
{
}


FrameLoaderClientQt::~FrameLoaderClientQt()
{
}

void FrameLoaderClientQt::setFrame(QWebFrameAdapter* webFrame, Frame* frame)
{
    m_webFrame = webFrame;
    m_frame = frame;

    if (!m_webFrame || !m_webFrame->pageAdapter) {
        qWarning("FrameLoaderClientQt::setFrame frame without Page!");
        return;
    }

    connect(this, SIGNAL(loadProgress(int)),
        m_webFrame->pageAdapter->handle(), SIGNAL(loadProgress(int)));

    connect(this, SIGNAL(unsupportedContent(QNetworkReply*)),
        m_webFrame->pageAdapter->handle(), SIGNAL(unsupportedContent(QNetworkReply*)));

    connect(this, SIGNAL(titleChanged(QString)),
        m_webFrame->handle(), SIGNAL(titleChanged(QString)));
}

void FrameLoaderClientQt::callPolicyFunction(FramePolicyFunction function, PolicyAction action)
{
    (m_frame->loader()->policyChecker()->*function)(action);
}

bool FrameLoaderClientQt::hasWebView() const
{
    // notImplemented();
    return true;
}

void FrameLoaderClientQt::savePlatformDataToCachedFrame(CachedFrame*) 
{
    notImplemented();
}

void FrameLoaderClientQt::transitionToCommittedFromCachedFrame(CachedFrame*)
{
}

void FrameLoaderClientQt::transitionToCommittedForNewPage()
{
    ASSERT(m_frame);
    ASSERT(m_webFrame);

    QObject* qWebPage = m_webFrame->pageAdapter->handle();
    QBrush brush = qWebPage->property("palette").value<QPalette>().brush(QPalette::Base);
    QColor backgroundColor = brush.style() == Qt::SolidPattern ? brush.color() : QColor();

    const QSize preferredLayoutSize = qWebPage->property("preferredContentsSize").toSize();

    ScrollbarMode hScrollbar = (ScrollbarMode) m_webFrame->scrollBarPolicy(Qt::Horizontal);
    ScrollbarMode vScrollbar = (ScrollbarMode) m_webFrame->scrollBarPolicy(Qt::Vertical);
    bool hLock = hScrollbar != ScrollbarAuto;
    bool vLock = vScrollbar != ScrollbarAuto;

    // The HistoryController will update the scroll position later if needed.
    IntRect currentVisibleContentRect = m_frame->view() ? IntRect(IntPoint::zero(), m_frame->view()->fixedVisibleContentRect().size()) : IntRect();

    m_frame->createView(qWebPage->property("viewportSize").toSize(),
        backgroundColor, !backgroundColor.alpha(),
        preferredLayoutSize.isValid() ? IntSize(preferredLayoutSize) : IntSize(),
        currentVisibleContentRect,
        preferredLayoutSize.isValid(),
        hScrollbar, hLock,
        vScrollbar, vLock);

    bool isMainFrame = m_frame == m_frame->page()->mainFrame();
    if (isMainFrame &&m_webFrame->pageAdapter->client) {
        bool resizesToContents = m_webFrame->pageAdapter->client->viewResizesToContentsEnabled();

        m_frame->view()->setPaintsEntireContents(resizesToContents);
        m_frame->view()->setDelegatesScrolling(resizesToContents);
    }
}

void FrameLoaderClientQt::didSaveToPageCache()
{
}

void FrameLoaderClientQt::didRestoreFromPageCache()
{
}

void FrameLoaderClientQt::dispatchDidBecomeFrameset(bool)
{
}


void FrameLoaderClientQt::forceLayout()
{
    FrameView* view = m_frame->view();
    if (view)
        view->layout(true);
}


void FrameLoaderClientQt::forceLayoutForNonHTML()
{
}


void FrameLoaderClientQt::setCopiesOnScroll()
{
    // Apparently this is mac specific.
}


void FrameLoaderClientQt::detachedFromParent2()
{
}


void FrameLoaderClientQt::detachedFromParent3()
{
}

void FrameLoaderClientQt::dispatchDidHandleOnloadEvents()
{
    // Don't need this one.
    if (dumpFrameLoaderCallbacks)
        printf("%s - didHandleOnloadEventsForFrame\n", qPrintable(drtDescriptionSuitableForTestResult(m_frame)));
}


void FrameLoaderClientQt::dispatchDidReceiveServerRedirectForProvisionalLoad()
{
    if (dumpFrameLoaderCallbacks)
        printf("%s - didReceiveServerRedirectForProvisionalLoadForFrame\n", qPrintable(drtDescriptionSuitableForTestResult(m_frame)));

    notImplemented();
}


void FrameLoaderClientQt::dispatchDidCancelClientRedirect()
{
    if (dumpFrameLoaderCallbacks)
        printf("%s - didCancelClientRedirectForFrame\n", qPrintable(drtDescriptionSuitableForTestResult(m_frame)));

    notImplemented();
}


void FrameLoaderClientQt::dispatchWillPerformClientRedirect(const KURL& url, double, double)
{
    if (dumpFrameLoaderCallbacks)
        printf("%s - willPerformClientRedirectToURL: %s \n", qPrintable(drtDescriptionSuitableForTestResult(m_frame)), qPrintable(drtDescriptionSuitableForTestResult(url)));

    if (dumpUserGestureInFrameLoaderCallbacks)
        printf("%s - in willPerformClientRedirect\n", qPrintable(drtPrintFrameUserGestureStatus(m_frame)));

    notImplemented();
}

void FrameLoaderClientQt::dispatchDidNavigateWithinPage()
{
    if (!m_webFrame)
        return;

    FrameLoader* loader = m_frame->loader();
    bool loaderCompleted = !(loader->activeDocumentLoader() && loader->activeDocumentLoader()->isLoadingInAPISense());

    if (!loaderCompleted)
        return;

    dispatchDidCommitLoad();
    dispatchDidFinishLoad();
}

void FrameLoaderClientQt::dispatchDidChangeLocationWithinPage()
{
    if (dumpFrameLoaderCallbacks)
        printf("%s - didChangeLocationWithinPageForFrame\n", qPrintable(drtDescriptionSuitableForTestResult(m_frame)));

    if (!m_webFrame)
        return;

    m_webFrame->emitUrlChanged();
    m_webFrame->pageAdapter->updateNavigationActions();
}


void FrameLoaderClientQt::dispatchDidPushStateWithinPage()
{
    if (dumpFrameLoaderCallbacks)
        printf("%s - dispatchDidPushStateWithinPage\n", qPrintable(drtDescriptionSuitableForTestResult(m_frame)));

    dispatchDidNavigateWithinPage();
}

void FrameLoaderClientQt::dispatchDidReplaceStateWithinPage()
{
    if (dumpFrameLoaderCallbacks)
        printf("%s - dispatchDidReplaceStateWithinPage\n", qPrintable(drtDescriptionSuitableForTestResult(m_frame)));

    dispatchDidNavigateWithinPage();
}

void FrameLoaderClientQt::dispatchDidPopStateWithinPage()
{
    if (dumpFrameLoaderCallbacks)
        printf("%s - dispatchDidPopStateWithinPage\n", qPrintable(drtDescriptionSuitableForTestResult(m_frame)));

    // No need to call dispatchDidNavigateWithinPage here, it's already been done in loadInSameDocument().
}

void FrameLoaderClientQt::dispatchWillClose()
{
}


void FrameLoaderClientQt::dispatchDidStartProvisionalLoad()
{
    if (dumpFrameLoaderCallbacks)
        printf("%s - didStartProvisionalLoadForFrame\n", qPrintable(drtDescriptionSuitableForTestResult(m_frame)));

    if (dumpUserGestureInFrameLoaderCallbacks)
        printf("%s - in didStartProvisionalLoadForFrame\n", qPrintable(drtPrintFrameUserGestureStatus(m_frame)));

    m_lastRequestedUrl = m_frame->loader()->activeDocumentLoader()->requestURL();

    if (!m_webFrame)
        return;
    emitLoadStarted();
    postProgressEstimateChangedNotification();
    m_webFrame->didStartProvisionalLoad();
}


void FrameLoaderClientQt::dispatchDidReceiveTitle(const StringWithDirection& title)
{
    // FIXME: Use direction of title.
    if (dumpFrameLoaderCallbacks)
        printf("%s - didReceiveTitle: %s\n", qPrintable(drtDescriptionSuitableForTestResult(m_frame)), qPrintable(QString(title.string())));

    if (!m_webFrame)
        return;

    emit titleChanged(title.string());
}


void FrameLoaderClientQt::dispatchDidChangeIcons(WebCore::IconType)
{
    if (dumpFrameLoaderCallbacks)
        printf("%s - didChangeIcons\n", qPrintable(drtDescriptionSuitableForTestResult(m_frame)));

    if (!m_webFrame)
        return;

    // FIXME: In order to get notified of icon URLS' changes, add a notification.
    // emit iconsChanged();
}


void FrameLoaderClientQt::dispatchDidCommitLoad()
{
    if (dumpFrameLoaderCallbacks)
        printf("%s - didCommitLoadForFrame\n", qPrintable(drtDescriptionSuitableForTestResult(m_frame)));

    if (m_frame->tree()->parent() || !m_webFrame)
        return;

    m_webFrame->emitUrlChanged();
    m_webFrame->pageAdapter->updateNavigationActions();

    // We should assume first the frame has no title. If it has, then the above dispatchDidReceiveTitle()
    // will be called very soon with the correct title.
    // This properly resets the title when we navigate to a URI without a title.
    emit titleChanged(QString());

    bool isMainFrame = (m_frame == m_frame->page()->mainFrame());
    if (!isMainFrame)
        return;

    emit m_webFrame->pageAdapter->emitViewportChangeRequested();
}


void FrameLoaderClientQt::dispatchDidFinishDocumentLoad()
{
    if (dumpFrameLoaderCallbacks)
        printf("%s - didFinishDocumentLoadForFrame\n", qPrintable(drtDescriptionSuitableForTestResult(m_frame)));

    if (QWebPageAdapter::drtRun) {
        int unloadEventCount = m_frame->document()->domWindow()->pendingUnloadEventListeners();
        if (unloadEventCount)
            printf("%s - has %u onunload handler(s)\n", qPrintable(drtDescriptionSuitableForTestResult(m_frame)), unloadEventCount);
    }

    if (m_frame->tree()->parent() || !m_webFrame)
        return;

    m_webFrame->pageAdapter->updateNavigationActions();
}


void FrameLoaderClientQt::dispatchDidFinishLoad()
{
    if (dumpFrameLoaderCallbacks)
        printf("%s - didFinishLoadForFrame\n", qPrintable(drtDescriptionSuitableForTestResult(m_frame)));

    if (!m_webFrame)
        return;

    m_webFrame->pageAdapter->updateNavigationActions();
    emitLoadFinished(true);
}

void FrameLoaderClientQt::dispatchDidLayout(LayoutMilestones milestones)
{
    if (!m_webFrame)
        return;

    if (milestones & DidFirstVisuallyNonEmptyLayout)
        m_webFrame->emitInitialLayoutCompleted();
}

void FrameLoaderClientQt::dispatchShow()
{
    notImplemented();
}


void FrameLoaderClientQt::cancelPolicyCheck()
{
    // qDebug() << "FrameLoaderClientQt::cancelPolicyCheck";
}


void FrameLoaderClientQt::dispatchWillSubmitForm(FramePolicyFunction function, PassRefPtr<FormState>)
{
    notImplemented();
    // FIXME: This is surely too simple.
    callPolicyFunction(function, PolicyUse);
}


void FrameLoaderClientQt::postProgressStartedNotification()
{
    if (m_webFrame && m_frame->page())
        m_isOriginatingLoad = true;
    if (m_frame->tree()->parent() || !m_webFrame)
        return;
    m_webFrame->pageAdapter->updateNavigationActions();
}

void FrameLoaderClientQt::postProgressEstimateChangedNotification()
{
    if (m_webFrame && m_frame->page())
        emit loadProgress(qRound(m_frame->page()->progress()->estimatedProgress() * 100));
}

void FrameLoaderClientQt::postProgressFinishedNotification()
{
    if (dumpProgressFinishedCallback)
        printf("postProgressFinishedNotification\n");

    // Send a mousemove event to:
    // (1) update the cursor to change according to whatever is underneath the mouse cursor right now;
    // (2) display the tool tip if the mouse hovers a node which has a tool tip.
    if (m_frame && m_frame->eventHandler() && m_webFrame) {
        QPoint localPos;
        if (m_webFrame->handleProgressFinished(&localPos)) {
            QMouseEvent event(QEvent::MouseMove, localPos, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
            m_frame->eventHandler()->mouseMoved(convertMouseEvent(&event, 0));
        }
    }
}

void FrameLoaderClientQt::setMainFrameDocumentReady(bool)
{
    // This is only interesting once we provide an external API for the DOM.
}


void FrameLoaderClientQt::willChangeTitle(DocumentLoader*)
{
    // No need for, dispatchDidReceiveTitle is the right callback.
}


void FrameLoaderClientQt::didChangeTitle(DocumentLoader*)
{
    // No need for, dispatchDidReceiveTitle is the right callback.
}


void FrameLoaderClientQt::finishedLoading(DocumentLoader*)
{
    if (!m_pluginView)
        return;
    if (m_pluginView->isPluginView())
        m_pluginView->didFinishLoading();
    m_pluginView = 0;
    m_hasSentResponseToPlugin = false;
}

bool FrameLoaderClientQt::canShowMIMETypeAsHTML(const String& MIMEType) const
{
    notImplemented();
    return false;
}
    
bool FrameLoaderClientQt::canShowMIMEType(const String& MIMEType) const
{
    String type = MIMEType;
    type.makeLower();
    if (MIMETypeRegistry::canShowMIMEType(type))
        return true;

    if (m_frame && m_frame->settings()  && m_frame->settings()->arePluginsEnabled()
        && PluginDatabase::installedPlugins()->isMIMETypeRegistered(type))
        return true;

    return false;
}

bool FrameLoaderClientQt::representationExistsForURLScheme(const String&) const
{
    return false;
}


String FrameLoaderClientQt::generatedMIMETypeForURLScheme(const String&) const
{
    notImplemented();
    return String();
}


void FrameLoaderClientQt::frameLoadCompleted()
{
    // Note that this can be called multiple times.
    if (!m_webFrame)
        return;
    m_webFrame->pageAdapter->updateNavigationActions();
}


void FrameLoaderClientQt::restoreViewState()
{
    if (!m_webFrame)
        return;
    m_webFrame->pageAdapter->emitRestoreFrameStateRequested(m_webFrame);
}


void FrameLoaderClientQt::provisionalLoadStarted()
{
    // Don't need to do anything here.
}


void FrameLoaderClientQt::didFinishLoad()
{
    // notImplemented();
}


void FrameLoaderClientQt::prepareForDataSourceReplacement()
{
}

void FrameLoaderClientQt::setTitle(const StringWithDirection& title, const KURL& url)
{
    // Used by Apple WebKit to update the title of an existing history item.
    // QtWebKit doesn't accomodate this on history items. If it ever does,
    // it should be privateBrowsing-aware. For now, we are just passing
    // globalhistory layout tests.
    // FIXME: Use direction of title.
    if (dumpHistoryCallbacks) {
        printf("WebView updated the title for history URL \"%s\" to \"%s\".\n",
            qPrintable(drtDescriptionSuitableForTestResult(url)),
            qPrintable(QString(title.string())));
    }
}


String FrameLoaderClientQt::userAgent(const KURL& url)
{
    if (m_webFrame)
        return m_webFrame->pageAdapter->userAgentForUrl(url).remove(QLatin1Char('\n')).remove(QLatin1Char('\r'));
    return String();
}

void FrameLoaderClientQt::dispatchDidReceiveIcon()
{
    if (m_webFrame)
        m_webFrame->emitIconChanged();
}

void FrameLoaderClientQt::frameLoaderDestroyed()
{
    // Delete QWebFrame (handle()), which owns QWebFramePrivate, which
    // _is_ a QWebFrameAdapter.
    if (m_webFrame)
        delete m_webFrame->handle();
    m_frame = 0;
    m_webFrame = 0;

    delete this;
}

bool FrameLoaderClientQt::canHandleRequest(const WebCore::ResourceRequest&) const
{
    return true;
}

void FrameLoaderClientQt::dispatchDidClearWindowObjectInWorld(DOMWrapperWorld* world)
{
    if (world != mainThreadNormalWorld())
        return;

    if (m_webFrame)
        m_webFrame->didClearWindowObject();
}

void FrameLoaderClientQt::documentElementAvailable()
{
    return;
}

void FrameLoaderClientQt::didPerformFirstNavigation() const
{
    if (m_frame->tree()->parent() || !m_webFrame)
        return;
    m_webFrame->pageAdapter->updateNavigationActions();
}

void FrameLoaderClientQt::registerForIconNotification(bool shouldRegister)
{
#if ENABLE(ICONDATABASE)
    if (shouldRegister)
        connect(IconDatabaseClientQt::instance(), SIGNAL(iconLoadedForPageURL(QString)), this, SLOT(onIconLoadedForPageURL(QString)), Qt::UniqueConnection);
    else
        disconnect(IconDatabaseClientQt::instance(), SIGNAL(iconLoadedForPageURL(QString)), this, SLOT(onIconLoadedForPageURL(QString)));
#endif
}

void FrameLoaderClientQt::onIconLoadedForPageURL(const QString& url)
{
#if ENABLE(ICONDATABASE)
    if (m_webFrame && m_webFrame->url == url)
        m_webFrame->emitIconChanged();
#endif
}


void FrameLoaderClientQt::updateGlobalHistory()
{
    QWebHistoryInterface* history = QWebHistoryInterface::defaultInterface();
    WebCore::DocumentLoader* loader = m_frame->loader()->documentLoader();
    if (history)
        history->addHistoryEntry(loader->urlForHistory().string());

    if (dumpHistoryCallbacks) {
        printf("WebView navigated to url \"%s\" with title \"%s\" with HTTP equivalent method \"%s\".  The navigation was %s and was %s%s.\n",
            qPrintable(drtDescriptionSuitableForTestResult(loader->urlForHistory())),
            qPrintable(QString(loader->title().string())),
            qPrintable(QString(loader->request().httpMethod())),
            ((loader->substituteData().isValid() || (loader->response().httpStatusCode() >= 400)) ? "a failure" : "successful"),
            ((!loader->clientRedirectSourceForHistory().isEmpty()) ? "a client redirect from " : "not a client redirect"),
            (!loader->clientRedirectSourceForHistory().isEmpty()) ? qPrintable(drtDescriptionSuitableForTestResult(loader->clientRedirectSourceForHistory())) : "");
    }
}

void FrameLoaderClientQt::updateGlobalHistoryRedirectLinks()
{
    // Apple WebKit is the only port that makes use of this callback. It calls
    // WebCore::HistoryItem::addRedirectURL() with the contents of
    // loader->[server|client]RedirectDestinationForHistory().
    // WebCore can associate a bunch of redirect URLs with a particular
    // item in the history, presumably this allows Safari to skip the redirections
    // when navigating to that history item. That might be a feature Qt wants to
    // offer through QWebHistoryInterface in the future. For now, we're just
    // passing tests in LayoutTests/http/tests/globalhistory.
    WebCore::DocumentLoader* loader = m_frame->loader()->documentLoader();

    if (!loader->clientRedirectSourceForHistory().isNull()) {
        if (dumpHistoryCallbacks) {
            printf("WebView performed a client redirect from \"%s\" to \"%s\".\n",
                qPrintable(QString(loader->clientRedirectSourceForHistory())),
                qPrintable(QString(loader->clientRedirectDestinationForHistory())));
        }
    }

    if (!loader->serverRedirectSourceForHistory().isNull()) {
        if (dumpHistoryCallbacks) {
            printf("WebView performed a server redirect from \"%s\" to \"%s\".\n",
                qPrintable(QString(loader->serverRedirectSourceForHistory())),
                qPrintable(QString(loader->serverRedirectDestinationForHistory())));
        }
    }
}

bool FrameLoaderClientQt::shouldGoToHistoryItem(WebCore::HistoryItem*) const
{
    return true;
}

bool FrameLoaderClientQt::shouldStopLoadingForHistoryItem(WebCore::HistoryItem*) const
{
    return true;
}

void FrameLoaderClientQt::didDisplayInsecureContent()
{
    if (dumpFrameLoaderCallbacks)
        printf("didDisplayInsecureContent\n");

    notImplemented();
}

void FrameLoaderClientQt::didRunInsecureContent(WebCore::SecurityOrigin*, const KURL&)
{
    if (dumpFrameLoaderCallbacks)
        printf("didRunInsecureContent\n");

    notImplemented();
}

void FrameLoaderClientQt::didDetectXSS(const KURL&, bool)
{
    if (dumpFrameLoaderCallbacks)
        printf("didDetectXSS\n");

    notImplemented();
}

void FrameLoaderClientQt::saveViewStateToItem(WebCore::HistoryItem* item)
{
    QWebHistoryItem historyItem(new QWebHistoryItemPrivate(item));
    m_webFrame->pageAdapter->emitSaveFrameStateRequested(m_webFrame, &historyItem);
}

bool FrameLoaderClientQt::canCachePage() const
{
    return true;
}

void FrameLoaderClientQt::setMainDocumentError(WebCore::DocumentLoader* loader, const WebCore::ResourceError& error)
{
    if (!m_pluginView)
        return;
    if (m_pluginView->isPluginView())
        m_pluginView->didFail(error);
    m_pluginView = 0;
    m_hasSentResponseToPlugin = false;
}

// FIXME: This function should be moved into WebCore.
void FrameLoaderClientQt::committedLoad(WebCore::DocumentLoader* loader, const char* data, int length)
{
    if (!m_pluginView)
        loader->commitData(data, length);

    // If we are sending data to MediaDocument, we should stop here and cancel the request.
    if (m_frame->document()->isMediaDocument())
        loader->cancelMainResourceLoad(pluginWillHandleLoadError(loader->response()));

    // We re-check here as the plugin can have been created.
    if (m_pluginView && m_pluginView->isPluginView()) {
        if (!m_hasSentResponseToPlugin) {
            m_pluginView->didReceiveResponse(loader->response());
            // The function didReceiveResponse sets up a new stream to the plug-in.
            // On a full-page plug-in, a failure in setting up this stream can cause the
            // main document load to be cancelled, setting m_pluginView to null.
            if (!m_pluginView)
                return;
            m_hasSentResponseToPlugin = true;
        }
        m_pluginView->didReceiveData(data, length);
    }
}

WebCore::ResourceError FrameLoaderClientQt::cancelledError(const WebCore::ResourceRequest& request)
{
    ResourceError error = ResourceError("QtNetwork", QNetworkReply::OperationCanceledError, request.url().string(),
        QCoreApplication::translate("QWebFrame", "Request cancelled", 0));
    error.setIsCancellation(true);
    return error;
}

// This was copied from file "WebKit/Source/WebKit/mac/Misc/WebKitErrors.h".
enum {
    WebKitErrorCannotShowMIMEType =                             100,
    WebKitErrorCannotShowURL =                                  101,
    WebKitErrorFrameLoadInterruptedByPolicyChange =             102,
    WebKitErrorCannotUseRestrictedPort =                        103,
    WebKitErrorCannotFindPlugIn =                               200,
    WebKitErrorCannotLoadPlugIn =                               201,
    WebKitErrorJavaUnavailable =                                202,
    WebKitErrorPluginWillHandleLoad =                           203
};

WebCore::ResourceError FrameLoaderClientQt::blockedError(const WebCore::ResourceRequest& request)
{
    return ResourceError("WebKitErrorDomain", WebKitErrorCannotUseRestrictedPort, request.url().string(),
        QCoreApplication::translate("QWebFrame", "Request blocked", 0));
}


WebCore::ResourceError FrameLoaderClientQt::cannotShowURLError(const WebCore::ResourceRequest& request)
{
    return ResourceError("WebKitErrorDomain", WebKitErrorCannotShowURL, request.url().string(),
        QCoreApplication::translate("QWebFrame", "Cannot show URL", 0));
}

WebCore::ResourceError FrameLoaderClientQt::interruptedForPolicyChangeError(const WebCore::ResourceRequest& request)
{
    return ResourceError("WebKitErrorDomain", WebKitErrorFrameLoadInterruptedByPolicyChange, request.url().string(),
        QCoreApplication::translate("QWebFrame", "Frame load interrupted by policy change", 0));
}

WebCore::ResourceError FrameLoaderClientQt::cannotShowMIMETypeError(const WebCore::ResourceResponse& response)
{
    return ResourceError("WebKitErrorDomain", WebKitErrorCannotShowMIMEType, response.url().string(),
        QCoreApplication::translate("QWebFrame", "Cannot show mimetype", 0));
}

WebCore::ResourceError FrameLoaderClientQt::fileDoesNotExistError(const WebCore::ResourceResponse& response)
{
    return ResourceError("QtNetwork", QNetworkReply::ContentNotFoundError, response.url().string(),
        QCoreApplication::translate("QWebFrame", "File does not exist", 0));
}

WebCore::ResourceError FrameLoaderClientQt::pluginWillHandleLoadError(const WebCore::ResourceResponse& response)
{
    return ResourceError("WebKit", WebKitErrorPluginWillHandleLoad, response.url().string(),
        QCoreApplication::translate("QWebFrame", "Loading is handled by the media engine", 0));
}

bool FrameLoaderClientQt::shouldFallBack(const WebCore::ResourceError& error)
{
    DEFINE_STATIC_LOCAL(const ResourceError, cancelledError, (this->cancelledError(ResourceRequest())));
    DEFINE_STATIC_LOCAL(const ResourceError, pluginWillHandleLoadError, (this->pluginWillHandleLoadError(ResourceResponse())));
    DEFINE_STATIC_LOCAL(const ResourceError, errorInterruptedForPolicyChange, (this->interruptedForPolicyChangeError(ResourceRequest())));

    if (error.errorCode() == cancelledError.errorCode() && error.domain() == cancelledError.domain())
        return false;

    if (error.errorCode() == errorInterruptedForPolicyChange.errorCode() && error.domain() == errorInterruptedForPolicyChange.domain())
        return false;

    if (error.errorCode() == pluginWillHandleLoadError.errorCode() && error.domain() == pluginWillHandleLoadError.domain())
        return false;

    return true;
}

WTF::PassRefPtr<WebCore::DocumentLoader> FrameLoaderClientQt::createDocumentLoader(const WebCore::ResourceRequest& request, const SubstituteData& substituteData)
{
    RefPtr<DocumentLoader> loader = DocumentLoader::create(request, substituteData);
    if (!deferMainResourceDataLoad || substituteData.isValid())
        loader->setDeferMainResourceDataLoad(false);
    else
        m_frame->page()->setCustomHTMLTokenizerTimeDelay(-1);
    return loader.release();
}

void FrameLoaderClientQt::convertMainResourceLoadToDownload(WebCore::DocumentLoader* documentLoader, const WebCore::ResourceRequest& request, const WebCore::ResourceResponse&)
{
    if (!m_webFrame)
        return;

    QNetworkReplyHandler* handler = documentLoader->mainResourceLoader()->handle()->getInternal()->m_job;
    if (!handler) {
        qWarning("Attempted to download unsupported URL %s", request.url().string().characters8());
        return;
    }

    QNetworkReply* reply = handler->release();
    if (reply) {
        if (m_webFrame->pageAdapter->forwardUnsupportedContent)
            emit unsupportedContent(reply);
        else
            reply->abort();
    }
}

void FrameLoaderClientQt::assignIdentifierToInitialRequest(unsigned long identifier, WebCore::DocumentLoader*, const WebCore::ResourceRequest& request)
{
    if (dumpResourceLoadCallbacks)
        dumpAssignedUrls[identifier] = drtDescriptionSuitableForTestResult(request.url());
}

static void blockRequest(WebCore::ResourceRequest& request)
{
    request.setURL(QUrl());
}

static bool isLocalhost(const QString& host)
{
    return host == QLatin1String("127.0.0.1") || host == QLatin1String("localhost");
}

static bool hostIsUsedBySomeTestsToGenerateError(const QString& host)
{
    return host == QLatin1String("255.255.255.255");
}

void FrameLoaderClientQt::dispatchWillSendRequest(WebCore::DocumentLoader*, unsigned long identifier, WebCore::ResourceRequest& newRequest, const WebCore::ResourceResponse& redirectResponse)
{
    if (dumpResourceLoadCallbacks)
        printf("%s - willSendRequest %s redirectResponse %s\n",
            qPrintable(dumpAssignedUrls[identifier]),
            qPrintable(drtDescriptionSuitableForTestResult(newRequest)),
            (redirectResponse.isNull()) ? "(null)" : qPrintable(drtDescriptionSuitableForTestResult(redirectResponse)));

    if (sendRequestReturnsNull) {
        blockRequest(newRequest);
        return;
    }

    if (sendRequestReturnsNullOnRedirect && !redirectResponse.isNull()) {
        printf("Returning null for this redirect\n");
        blockRequest(newRequest);
        return;
    }

    QUrl url = newRequest.url();
    QString host = url.host();
    QString urlScheme = url.scheme().toLower();

    if (QWebPageAdapter::drtRun
        && !host.isEmpty()
        && (urlScheme == QLatin1String("http") || urlScheme == QLatin1String("https"))) {

        QUrl testURL = m_webFrame->pageAdapter->mainFrameAdapter()->frameLoaderClient->lastRequestedUrl();
        QString testHost = testURL.host();
        QString testURLScheme = testURL.scheme().toLower();

        if (!isLocalhost(host)
            && !hostIsUsedBySomeTestsToGenerateError(host)
            && ((testURLScheme != QLatin1String("http") && testURLScheme != QLatin1String("https")) || isLocalhost(testHost))) {
            printf("Blocked access to external URL %s\n", qPrintable(drtDescriptionSuitableForTestResult(newRequest.url())));
            blockRequest(newRequest);
            return;
        }
    }

    for (int i = 0; i < sendRequestClearHeaders.size(); ++i)
        newRequest.setHTTPHeaderField(sendRequestClearHeaders.at(i).toLocal8Bit().constData(), QString());

    if (QWebPageAdapter::drtRun) {
        QMap<QString, QString>::const_iterator it = URLsToRedirect.constFind(url.toString());
        if (it != URLsToRedirect.constEnd())
            newRequest.setURL(QUrl(it.value()));
    }
    // Seems like the Mac code doesn't do anything here by default neither.
    // qDebug() << "FrameLoaderClientQt::dispatchWillSendRequest" << request.isNull() << url;
}

bool FrameLoaderClientQt::shouldUseCredentialStorage(DocumentLoader*, unsigned long)
{
    notImplemented();
    return false;
}

void FrameLoaderClientQt::dispatchDidReceiveAuthenticationChallenge(DocumentLoader*, unsigned long, const AuthenticationChallenge&)
{
    notImplemented();
}

void FrameLoaderClientQt::dispatchDidCancelAuthenticationChallenge(DocumentLoader*, unsigned long, const AuthenticationChallenge&)
{
    notImplemented();
}

void FrameLoaderClientQt::dispatchDidReceiveResponse(WebCore::DocumentLoader*, unsigned long identifier, const WebCore::ResourceResponse& response)
{

    m_response = response;
    if (dumpWillCacheResponseCallbacks)
        printf("%s - willCacheResponse: called\n",
            qPrintable(dumpAssignedUrls[identifier]));

    if (dumpResourceLoadCallbacks)
        printf("%s - didReceiveResponse %s\n",
            qPrintable(dumpAssignedUrls[identifier]),
            qPrintable(drtDescriptionSuitableForTestResult(response)));

    if (dumpResourceResponseMIMETypes) {
        printf("%s has MIME type %s\n",
            qPrintable(QString(response.url().lastPathComponent())),
            qPrintable(QString(response.mimeType())));
    }
}

void FrameLoaderClientQt::dispatchDidReceiveContentLength(WebCore::DocumentLoader*, unsigned long, int)
{
}

void FrameLoaderClientQt::dispatchDidFinishLoading(WebCore::DocumentLoader*, unsigned long identifier)
{
    if (dumpResourceLoadCallbacks)
        printf("%s - didFinishLoading\n",
            (dumpAssignedUrls.contains(identifier) ? qPrintable(dumpAssignedUrls[identifier]) : "<unknown>"));
}

void FrameLoaderClientQt::dispatchDidFailLoading(WebCore::DocumentLoader* loader, unsigned long identifier, const WebCore::ResourceError& error)
{
    if (dumpResourceLoadCallbacks)
        printf("%s - didFailLoadingWithError: %s\n",
            (dumpAssignedUrls.contains(identifier) ? qPrintable(dumpAssignedUrls[identifier]) : "<unknown>"),
            qPrintable(drtDescriptionSuitableForTestResult(error)));
}

bool FrameLoaderClientQt::dispatchDidLoadResourceFromMemoryCache(WebCore::DocumentLoader*, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&, int)
{
    notImplemented();
    return false;
}

bool FrameLoaderClientQt::callErrorPageExtension(const WebCore::ResourceError& error)
{
    QWebPageAdapter* page = m_webFrame->pageAdapter;
    if (!page->supportsErrorPageExtension())
        return false;
    QWebPageAdapter::ErrorPageOption option;
    option.url = QUrl(error.failingURL());
    option.frame = m_webFrame;
    option.domain = error.domain();
    option.error = error.errorCode();
    option.errorString = error.localizedDescription();

    QWebPageAdapter::ErrorPageReturn output;

    if (!page->errorPageExtension(&option, &output))
        return false;

    KURL baseUrl(output.baseUrl);
    KURL failingUrl(option.url);

    WebCore::ResourceRequest request(baseUrl);
    WTF::RefPtr<WebCore::SharedBuffer> buffer = WebCore::SharedBuffer::create(output.content.constData(), output.content.length());
    WebCore::SubstituteData substituteData(buffer, output.contentType, output.encoding, failingUrl);
    m_frame->loader()->load(WebCore::FrameLoadRequest(m_frame, request, substituteData));
    return true;
}

void FrameLoaderClientQt::dispatchDidFailProvisionalLoad(const WebCore::ResourceError& error)
{
    if (dumpFrameLoaderCallbacks)
        printf("%s - didFailProvisionalLoadWithError\n", qPrintable(drtDescriptionSuitableForTestResult(m_frame)));

    if (!error.isNull() && !error.isCancellation()) {
        if (callErrorPageExtension(error))
            return;
    }

    if (m_webFrame)
        emitLoadFinished(false);
}

void FrameLoaderClientQt::dispatchDidFailLoad(const WebCore::ResourceError& error)
{
    if (dumpFrameLoaderCallbacks)
        printf("%s - didFailLoadWithError\n", qPrintable(drtDescriptionSuitableForTestResult(m_frame)));

    if (!error.isNull() && !error.isCancellation()) {
        if (callErrorPageExtension(error))
            return;
    }

    if (m_webFrame)
        emitLoadFinished(false);
}

WebCore::Frame* FrameLoaderClientQt::dispatchCreatePage(const WebCore::NavigationAction&)
{
    if (!m_webFrame)
        return 0;
    QWebPageAdapter* newPage = m_webFrame->pageAdapter->createWindow(/* modalDialog = */ false);
    if (!newPage)
        return 0;
    return newPage->mainFrameAdapter()->frame;
}

void FrameLoaderClientQt::dispatchDecidePolicyForResponse(FramePolicyFunction function, const WebCore::ResourceResponse& response, const WebCore::ResourceRequest&)
{
    // We need to call directly here.
    switch (response.httpStatusCode()) {
    case HTTPResetContent:
        // FIXME: a 205 response requires that the requester reset the document view.
        // Fallthrough
    case HTTPNoContent:
        callPolicyFunction(function, PolicyIgnore);
        return;
    }

    if (WebCore::contentDispositionType(response.httpHeaderField("Content-Disposition")) == WebCore::ContentDispositionAttachment)
        callPolicyFunction(function, PolicyDownload);
    else if (canShowMIMEType(response.mimeType()))
        callPolicyFunction(function, PolicyUse);
    else
        callPolicyFunction(function, PolicyDownload);
}

void FrameLoaderClientQt::dispatchDecidePolicyForNewWindowAction(FramePolicyFunction function, const WebCore::NavigationAction& action, const WebCore::ResourceRequest& request, PassRefPtr<WebCore::FormState>, const WTF::String&)
{
    Q_ASSERT(m_webFrame);
    QNetworkRequest r(request.toNetworkRequest(m_frame->loader()->networkingContext()));

    if (!m_webFrame->pageAdapter->acceptNavigationRequest(0, r, (int)action.type())) {
        if (action.type() == NavigationTypeFormSubmitted || action.type() == NavigationTypeFormResubmitted)
            m_frame->loader()->resetMultipleFormSubmissionProtection();

        if (action.type() == NavigationTypeLinkClicked && r.url().hasFragment()) {
            ResourceRequest emptyRequest;
            m_frame->loader()->activeDocumentLoader()->setLastCheckedRequest(emptyRequest);
        }

        callPolicyFunction(function, PolicyIgnore);
        return;
    }
    callPolicyFunction(function, PolicyUse);
}

void FrameLoaderClientQt::dispatchDecidePolicyForNavigationAction(FramePolicyFunction function, const WebCore::NavigationAction& action, const WebCore::ResourceRequest& request, PassRefPtr<WebCore::FormState>)
{
    Q_ASSERT(m_webFrame);
    QNetworkRequest r(request.toNetworkRequest(m_frame->loader()->networkingContext()));
    PolicyAction result;

    // Currently, this is only enabled by DRT.
    if (policyDelegateEnabled) {
        RefPtr<Node> node;
        for (const Event* event = action.event(); event; event = event->underlyingEvent()) {
            if (event->isMouseEvent()) {
                const MouseEvent* mouseEvent =  static_cast<const MouseEvent*>(event);
                node = m_webFrame->frame->eventHandler()->hitTestResultAtPoint(
                    mouseEvent->absoluteLocation()).innerNonSharedNode();
                break;
            }
        }

        printf("Policy delegate: attempt to load %s with navigation type '%s'%s\n",
            qPrintable(drtDescriptionSuitableForTestResult(request.url())), navigationTypeToString(action.type()),
            (node) ? qPrintable(QString::fromLatin1(" originating from ") + drtDescriptionSuitableForTestResult(node, 0)) : "");

        if (policyDelegatePermissive)
            result = PolicyUse;
        else
            result = PolicyIgnore;

        m_webFrame->pageAdapter->acceptNavigationRequest(m_webFrame, r, (int)action.type());
        callPolicyFunction(function, result);
        return;
    }

    if (!m_webFrame->pageAdapter->acceptNavigationRequest(m_webFrame, r, (int)action.type())) {
        if (action.type() == NavigationTypeFormSubmitted || action.type() == NavigationTypeFormResubmitted)
            m_frame->loader()->resetMultipleFormSubmissionProtection();

        if (action.type() == NavigationTypeLinkClicked && r.url().hasFragment()) {
            ResourceRequest emptyRequest;
            m_frame->loader()->activeDocumentLoader()->setLastCheckedRequest(emptyRequest);
        }

        callPolicyFunction(function, PolicyIgnore);
        return;
    }
    callPolicyFunction(function, PolicyUse);
}

void FrameLoaderClientQt::dispatchUnableToImplementPolicy(const WebCore::ResourceError&)
{
    notImplemented();
}

void FrameLoaderClientQt::startDownload(const WebCore::ResourceRequest& request, const String& /* suggestedName */)
{
    if (!m_webFrame)
        return;

    m_webFrame->pageAdapter->emitDownloadRequested(request.toNetworkRequest(m_frame->loader()->networkingContext()));
}

PassRefPtr<Frame> FrameLoaderClientQt::createFrame(const KURL& url, const String& name, HTMLFrameOwnerElement* ownerElement, const String& referrer, bool allowsScrolling, int marginWidth, int marginHeight)
{
    if (!m_webFrame)
        return 0;

    QWebFrameData frameData(m_frame->page(), m_frame, ownerElement, name);

    frameData.referrer = referrer;
    frameData.allowsScrolling = allowsScrolling;
    frameData.marginWidth = marginWidth;
    frameData.marginHeight = marginHeight;

    QWebFrameAdapter* childWebFrame = m_webFrame->createChildFrame(&frameData);
    // The creation of the frame may have run arbitrary JavaScript that removed it from the page already.
    if (!childWebFrame->frame->page()) {
        QPointer<QObject> qWebFrame = childWebFrame->handle();
        frameData.frame.release();
        ASSERT_UNUSED(qWebFrame, !qWebFrame);
        return 0;
    }

    m_webFrame->pageAdapter->emitFrameCreated(childWebFrame);

    // FIXME: Set override encoding if we have one.

    KURL urlToLoad = url;
    if (urlToLoad.isEmpty())
        urlToLoad = blankURL();

    m_frame->loader()->loadURLIntoChildFrame(urlToLoad, frameData.referrer, frameData.frame.get());

    // The frame's onload handler may have removed it from the document.
    if (!frameData.frame->tree()->parent())
        return 0;

    return frameData.frame.release();
}

ObjectContentType FrameLoaderClientQt::objectContentType(const KURL& url, const String& mimeTypeIn, bool shouldPreferPlugInsForImages)
{
    // qDebug()<<" ++++++++++++++++ url is "<<url.string()<<", mime = "<<mimeTypeIn;
    QFileInfo fi(url.path());
    String extension = fi.suffix();
    if (mimeTypeIn == "application/x-qt-plugin" || mimeTypeIn == "application/x-qt-styled-widget")
        return ObjectContentOtherPlugin;

    if (url.isEmpty() && !mimeTypeIn.length())
        return ObjectContentNone;

    String mimeType = mimeTypeIn;
    if (!mimeType.length())
        mimeType = MIMETypeRegistry::getMIMETypeForExtension(extension);

    bool arePluginsEnabled = (m_frame && m_frame->settings() && m_frame->settings()->arePluginsEnabled());
    if (arePluginsEnabled && !mimeType.length())
        mimeType = PluginDatabase::installedPlugins()->MIMETypeForExtension(extension);

    if (!mimeType.length())
        return ObjectContentFrame;

    ObjectContentType plugInType = ObjectContentNone;
    if (arePluginsEnabled && PluginDatabase::installedPlugins()->isMIMETypeRegistered(mimeType))
        plugInType = ObjectContentNetscapePlugin;
    else if (m_frame->page() && m_frame->page()->pluginData()) {
        bool allowPlugins = m_frame->loader()->subframeLoader()->allowPlugins(NotAboutToInstantiatePlugin);
        if ((m_frame->page()->pluginData()->supportsMimeType(mimeType, PluginData::AllPlugins) && allowPlugins)
            || m_frame->page()->pluginData()->supportsMimeType(mimeType, PluginData::OnlyApplicationPlugins))
                plugInType = ObjectContentOtherPlugin;
    }

    if (MIMETypeRegistry::isSupportedImageMIMEType(mimeType))
        return shouldPreferPlugInsForImages && plugInType != ObjectContentNone ? plugInType : ObjectContentImage;
    
    if (plugInType != ObjectContentNone)
        return plugInType;

    if (MIMETypeRegistry::isSupportedNonImageMIMEType(mimeType))
        return ObjectContentFrame;

    if (url.protocol() == "about")
        return ObjectContentFrame;

    return ObjectContentNone;
}

static const CSSPropertyID qstyleSheetProperties[] = {
    CSSPropertyColor,
    CSSPropertyFontFamily,
    CSSPropertyFontSize,
    CSSPropertyFontStyle,
    CSSPropertyFontWeight
};

const unsigned numqStyleSheetProperties = sizeof(qstyleSheetProperties) / sizeof(qstyleSheetProperties[0]);

class QtPluginWidget: public Widget {
public:
    QtPluginWidget(QtPluginWidgetAdapter* w)
        : Widget(w->handle())
        , m_adapter(w)
    {
        setBindingObject(w->handle());
    }

    ~QtPluginWidget()
    {
        delete m_adapter;
    }

    inline QtPluginWidgetAdapter* widgetAdapter() const
    {
        return m_adapter;
    }

    virtual void invalidateRect(const IntRect& r)
    { 
        if (platformWidget())
            widgetAdapter()->update(r);
    }
    virtual void frameRectsChanged()
    {
        QtPluginWidgetAdapter* widget = widgetAdapter();
        if (!widget)
            return;
        QRect windowRect = convertToContainingWindow(IntRect(0, 0, frameRect().width(), frameRect().height()));

        ScrollView* parentScrollView = parent();
        QRect clipRect;
        if (parentScrollView) {
            ASSERT_WITH_SECURITY_IMPLICATION(parentScrollView->isFrameView());
            clipRect = toFrameView(parentScrollView)->windowClipRect();
            clipRect.translate(-windowRect.x(), -windowRect.y());
        }
        widget->setGeometryAndClip(windowRect, clipRect, isVisible());
    }

    virtual void show()
    {
        Widget::show();
        handleVisibility();
    }
    virtual void hide()
    {
        Widget::hide();
        if (platformWidget())
            widgetAdapter()->setVisible(false);
    }

private:
    QtPluginWidgetAdapter* m_adapter;

    void handleVisibility()
    {
        if (!isVisible())
            return;
        widgetAdapter()->setVisible(true);
    }
};


PassRefPtr<Widget> FrameLoaderClientQt::createPlugin(const IntSize& pluginSize, HTMLPlugInElement* element, const KURL& url, const Vector<String>& paramNames, const Vector<String>& paramValues, const String& mimeType, bool loadManually)
{
    // qDebug()<<"------ Creating plugin in FrameLoaderClientQt::createPlugin for "<<url.string() << mimeType;
    // qDebug()<<"------\t url = "<<url.string();

    if (!m_webFrame)
        return 0;

    QStringList params;
    QStringList values;
    QString classid(element->getAttribute("classid"));

    for (unsigned i = 0; i < paramNames.size(); ++i) {
        params.append(paramNames[i]);
        if (paramNames[i] == "classid")
            classid = paramValues[i];
    }
    for (unsigned i = 0; i < paramValues.size(); ++i)
        values.append(paramValues[i]);

    QString urlStr(url.string());
    QUrl qurl = urlStr;

    QObject* pluginAdapter = 0;

    if (mimeType == "application/x-qt-plugin" || mimeType == "application/x-qt-styled-widget") {
        pluginAdapter = m_webFrame->pageAdapter->createPlugin(classid, qurl, params, values);
#ifndef QT_NO_STYLE_STYLESHEET
        QtPluginWidgetAdapter* widget = qobject_cast<QtPluginWidgetAdapter*>(pluginAdapter);
        if (widget && mimeType == "application/x-qt-styled-widget") {

            StringBuilder styleSheet;
            styleSheet.append(element->getAttribute("style"));
            if (!styleSheet.isEmpty())
                styleSheet.append(';');

            for (unsigned i = 0; i < numqStyleSheetProperties; ++i) {
                CSSPropertyID property = qstyleSheetProperties[i];

                styleSheet.append(getPropertyName(property));
                styleSheet.append(':');
                styleSheet.append(CSSComputedStyleDeclaration::create(element)->getPropertyValue(property));
                styleSheet.append(';');
            }

            widget->setStyleSheet(styleSheet.toString());
        }
#endif // QT_NO_STYLE_STYLESHEET
    }

    if (!pluginAdapter) {
        QWebPluginFactory* factory = m_webFrame->pageAdapter->pluginFactory;
        if (factory)
            pluginAdapter = m_webFrame->pageAdapter->adapterForWidget(factory->create(mimeType, qurl, params, values));
    }
    if (pluginAdapter) {
        QtPluginWidgetAdapter* widget = qobject_cast<QtPluginWidgetAdapter*>(pluginAdapter);
        if (widget) {
            QObject* parentWidget = 0;
            if (m_webFrame->pageAdapter->client)
                parentWidget = m_webFrame->pageAdapter->client->pluginParent();
            if (parentWidget) // Don't reparent to nothing (i.e. keep whatever parent QWebPage::createPlugin() chose.
                widget->setWidgetParent(parentWidget);
            widget->setVisible(false);
            RefPtr<QtPluginWidget> w = adoptRef(new QtPluginWidget(widget));
            // Make sure it's invisible until properly placed into the layout.
            w->setFrameRect(IntRect(0, 0, 0, 0));
            return w;
        }

        // FIXME: Make things work for widgetless plugins as well.
        delete pluginAdapter;
    }
#if ENABLE(NETSCAPE_PLUGIN_API)
    else { // NPAPI Plugins
        Vector<String> params = paramNames;
        Vector<String> values = paramValues;
        if (mimeType == "application/x-shockwave-flash") {
            // Inject wmode=opaque when there is no client or the client is not a QWebView.
            size_t wmodeIndex = params.find("wmode");
            if (wmodeIndex == WTF::notFound) {
                params.append("wmode");
                values.append("opaque");
            } else if (equalIgnoringCase(values[wmodeIndex], "window"))
                values[wmodeIndex] = "opaque";
        }

        RefPtr<PluginView> pluginView = PluginView::create(m_frame, pluginSize, element, url,
            params, values, mimeType, loadManually);
        return pluginView;
    }
#endif // ENABLE(NETSCAPE_PLUGIN_API)

    return 0;
}

void FrameLoaderClientQt::redirectDataToPlugin(Widget* pluginWidget)
{
    m_pluginView = toPluginView(pluginWidget);
    if (pluginWidget)
        m_hasSentResponseToPlugin = false;
}

PassRefPtr<Widget> FrameLoaderClientQt::createJavaAppletWidget(const IntSize& pluginSize, HTMLAppletElement* element, const KURL& url, const Vector<String>& paramNames, const Vector<String>& paramValues)
{
    return createPlugin(pluginSize, element, url, paramNames, paramValues, "application/x-java-applet", true);
}

String FrameLoaderClientQt::overrideMediaType() const
{
    if (m_webFrame && m_webFrame->pageAdapter && m_webFrame->pageAdapter->settings)
        return m_webFrame->pageAdapter->settings->cssMediaType();

    return String();
}

QString FrameLoaderClientQt::chooseFile(const QString& oldFile)
{
    QStringList result = m_webFrame->pageAdapter->chooseFiles(m_webFrame, /*allowMulti*/ false, (QStringList() << oldFile));
    return result.isEmpty() ? QString() : result.first();
}

PassRefPtr<FrameNetworkingContext> FrameLoaderClientQt::createNetworkingContext()
{
    QVariant value = m_webFrame->pageAdapter->handle()->property("_q_MIMESniffingDisabled");
    bool MIMESniffingDisabled = value.isValid() && value.toBool();

    return FrameNetworkingContextQt::create(m_frame, m_webFrame->handle(), !MIMESniffingDisabled);
}

QWebFrameAdapter* FrameLoaderClientQt::webFrame() const
{
    return m_webFrame;
}

void FrameLoaderClientQt::emitLoadStarted()
{
    m_webFrame->emitLoadStarted(m_isOriginatingLoad);
}

void FrameLoaderClientQt::emitLoadFinished(bool ok)
{
    // Signal handlers can lead to a new load, that will use the member again.
    const bool wasOriginatingLoad = m_isOriginatingLoad;
    m_isOriginatingLoad = false;

    m_webFrame->emitLoadFinished(wasOriginatingLoad, ok);
}

}

#include "moc_FrameLoaderClientQt.cpp"
