/*
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/
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
#include "TestRunnerQt.h"

#include "DumpRenderTreeQt.h"
#include "DumpRenderTreeSupportQt.h"
#include "NotificationPresenterClientQt.h"
#include "WorkQueue.h"
#include "WorkQueueItemQt.h"
#include <JSStringRefQt.h>
#include <QCoreApplication>
#include <QDir>
#include <QLocale>
#include <qwebsettings.h>

TestRunnerQt::TestRunnerQt(DumpRenderTree* drt)
    : QObject()
    , m_drt(drt)
    , m_shouldTimeout(true)
    , m_timeout(30000)
{
    reset();
}

TestRunner::~TestRunner()
{
}

void TestRunnerQt::reset()
{
    m_hasDumped = false;
    m_loadFinished = false;
    m_audioDump = false;
    m_waitForDone = false;
    m_timeoutTimer.stop();
    m_topLoadingFrame = 0;
    m_waitForPolicy = false;
    m_handleErrorPages = false;
    m_webHistory = 0;
    m_globalFlag = false;
    m_userStyleSheetEnabled = false;
    m_ignoreDesktopNotification = false;
    m_isGeolocationPermissionSet = false;
    m_geolocationPermission = false;
    m_audioData.clear();

    DumpRenderTreeSupportQt::dumpEditingCallbacks(false);
    DumpRenderTreeSupportQt::dumpFrameLoader(false);
    DumpRenderTreeSupportQt::dumpProgressFinishedCallback(false);
    DumpRenderTreeSupportQt::dumpUserGestureInFrameLoader(false);
    DumpRenderTreeSupportQt::dumpResourceLoadCallbacks(false);
    DumpRenderTreeSupportQt::dumpResourceResponseMIMETypes(false);
    DumpRenderTreeSupportQt::dumpWillCacheResponseCallbacks(false);
    DumpRenderTreeSupportQt::setDeferMainResourceDataLoad(true);
    DumpRenderTreeSupportQt::setWillSendRequestReturnsNullOnRedirect(false);
    DumpRenderTreeSupportQt::setWillSendRequestReturnsNull(false);
    DumpRenderTreeSupportQt::setWillSendRequestClearHeaders(QStringList());
    DumpRenderTreeSupportQt::clearScriptWorlds();
    DumpRenderTreeSupportQt::setCustomPolicyDelegate(false, false);
    DumpRenderTreeSupportQt::dumpHistoryCallbacks(false);
    DumpRenderTreeSupportQt::dumpVisitedLinksCallbacks(false);
    DumpRenderTreeSupportQt::resetGeolocationMock(m_drt->pageAdapter());
    DumpRenderTreeSupportQt::dumpNotification(false);
    DumpRenderTreeSupportQt::setShouldUseFontSmoothing(false);
    DumpRenderTreeSupportQt::disableDefaultTypesettingFeatures();
    setIconDatabaseEnabled(false);
    clearAllDatabases();
    removeAllWebNotificationPermissions();
    // The default state for DRT is to block third-party cookies, mimicing the Mac port
    setAlwaysAcceptCookies(false);
    emit hidePage();
}

void TestRunnerQt::dumpNotifications()
{
    DumpRenderTreeSupportQt::dumpNotification(true);
}

void TestRunnerQt::processWork()
{
    // qDebug() << ">>>processWork";

    // if we didn't start a new load, then we finished all the commands, so we're ready to dump state
    if (WorkQueue::shared()->processWork() && !shouldWaitUntilDone()) {
        emit done();
        m_hasDumped = true;
    }
}

// Called on loadFinished on WebPage
void TestRunnerQt::maybeDump(bool /*success*/)
{

    // This can happen on any of the http/tests/security/window-events-*.html tests, where the test opens
    // a new window, calls the unload and load event handlers on the window's page, and then immediately
    // issues a notifyDone. Needs investigation.
    if (!m_topLoadingFrame)
        return;

    // It is possible that we get called by windows created from the main page that have finished
    // loading, so we don't ASSERT here. At the moment we do not gather results from such windows,
    // but may need to in future.
    if (sender() != m_topLoadingFrame->page())
        return;

    m_loadFinished = true;
    // as the function is called on loadFinished, the test might
    // already have dumped and thus no longer be active, thus
    // bail out here.
    if (m_hasDumped)
        return;

    WorkQueue::shared()->setFrozen(true); // first complete load freezes the queue for the rest of this test
    if (WorkQueue::shared()->count())
        QTimer::singleShot(0, this, SLOT(processWork()));
    else if (!shouldWaitUntilDone()) {
        emit done();
        m_hasDumped = true;
    }
}

void TestRunnerQt::waitUntilDone()
{
    //qDebug() << ">>>>waitForDone";
    m_waitForDone = true;

    if (!m_shouldTimeout)
        return;

    m_timeoutTimer.start(m_timeout, this);
}

void TestRunnerQt::setViewModeMediaFeature(const QString& mode)
{
    m_drt->webPage()->setProperty("_q_viewMode", mode);
}

int TestRunnerQt::webHistoryItemCount()
{
    if (!m_webHistory)
        return -1;

    // Subtract one here as our QWebHistory::count() includes the actual page,
    // which is not considered in the DRT tests.
    return m_webHistory->count() - 1;
}

void TestRunnerQt::keepWebHistory()
{
    m_webHistory = m_drt->webPage()->history();
}

void TestRunnerQt::notifyDone()
{
    qDebug() << ">>>>notifyDone";

    if (m_shouldTimeout && !m_timeoutTimer.isActive())
        return;

    m_timeoutTimer.stop();
    m_waitForDone = false;

    // If the page has not finished loading (i.e. loadFinished() has not been emitted) then
    // content created by the likes of document.write() JS methods will not be available yet.
    // When the page has finished loading, maybeDump above will dump the results now that we have
    // just set shouldWaitUntilDone to false.
    if (!m_loadFinished)
        return;

    emit done();

    // FIXME: investigate why always resetting these result in timeouts
    m_hasDumped = true;
    m_waitForPolicy = false;
}

int TestRunnerQt::windowCount()
{
    return m_drt->windowCount();
}

void TestRunnerQt::grantWebNotificationPermission(const QString& origin)
{
    QWebFrame* frame = m_drt->webPage()->mainFrame();
    m_drt->webPage()->setFeaturePermission(frame, QWebPage::Notifications, QWebPage::PermissionGrantedByUser);
}

void TestRunnerQt::ignoreLegacyWebNotificationPermissionRequests()
{
    m_ignoreDesktopNotification = true;
}

void TestRunnerQt::denyWebNotificationPermission(const QString& origin)
{
    QWebFrame* frame = m_drt->webPage()->mainFrame();
    m_drt->webPage()->setFeaturePermission(frame, QWebPage::Notifications, QWebPage::PermissionDeniedByUser);
}

void TestRunnerQt::removeAllWebNotificationPermissions()
{
    DumpRenderTreeSupportQt::clearNotificationPermissions();
}

void TestRunnerQt::simulateLegacyWebNotificationClick(const QString& title)
{
    DumpRenderTreeSupportQt::simulateDesktopNotificationClick(title);
}

void TestRunnerQt::display()
{
    DumpRenderTreeSupportQt::setTrackRepaintRects(m_topLoadingFrame->handle(), true);
    emit showPage();
}

void TestRunnerQt::displayInvalidatedRegion()
{
    display();
}

void TestRunnerQt::clearBackForwardList()
{
    m_drt->webPage()->history()->clear();
}

QString TestRunnerQt::pathToLocalResource(const QString& url)
{
    QString localTmpUrl(QLatin1String("file:///tmp/LayoutTests"));

    // Translate a request for /tmp/LayoutTests to the repository LayoutTests directory.
    // Do not rely on a symlink to be created via the test runner, which will not work on Windows.
    if (url.startsWith(localTmpUrl)) {
        // DumpRenderTree lives in WebKit/WebKitBuild/<build_mode>/bin.
        // Translate from WebKit/WebKitBuild/Release/bin => WebKit/LayoutTests.
        QFileInfo layoutTestsRoot(QCoreApplication::applicationDirPath() + QLatin1String("/../../../LayoutTests/"));
        if (layoutTestsRoot.exists())
            return QLatin1String("file://") + layoutTestsRoot.absolutePath() + url.mid(localTmpUrl.length());
    }

    return url;
}

void TestRunnerQt::dumpEditingCallbacks()
{
    qDebug() << ">>>dumpEditingCallbacks";
    DumpRenderTreeSupportQt::dumpEditingCallbacks(true);
}

void TestRunnerQt::dumpFrameLoadCallbacks()
{
    DumpRenderTreeSupportQt::dumpFrameLoader(true);
}

void TestRunnerQt::dumpProgressFinishedCallback()
{
    DumpRenderTreeSupportQt::dumpProgressFinishedCallback(true);
}

void TestRunnerQt::dumpUserGestureInFrameLoadCallbacks()
{
    DumpRenderTreeSupportQt::dumpUserGestureInFrameLoader(true);
}

void TestRunnerQt::dumpResourceLoadCallbacks()
{
    DumpRenderTreeSupportQt::dumpResourceLoadCallbacks(true);
}

void TestRunnerQt::dumpResourceResponseMIMETypes()
{
    DumpRenderTreeSupportQt::dumpResourceResponseMIMETypes(true);
}

void TestRunnerQt::dumpWillCacheResponse()
{
    DumpRenderTreeSupportQt::dumpWillCacheResponseCallbacks(true);
}

void TestRunnerQt::dumpHistoryCallbacks()
{
    DumpRenderTreeSupportQt::dumpHistoryCallbacks(true);
}

void TestRunnerQt::setWillSendRequestReturnsNullOnRedirect(bool enabled)
{
    DumpRenderTreeSupportQt::setWillSendRequestReturnsNullOnRedirect(enabled);
}

void TestRunnerQt::setWillSendRequestReturnsNull(bool enabled)
{
    DumpRenderTreeSupportQt::setWillSendRequestReturnsNull(enabled);
}

void TestRunnerQt::setWillSendRequestClearHeader(const QStringList& headers)
{
    DumpRenderTreeSupportQt::setWillSendRequestClearHeaders(headers);
}

void TestRunnerQt::setDeferMainResourceDataLoad(bool defer)
{
    DumpRenderTreeSupportQt::setDeferMainResourceDataLoad(defer);
}

void TestRunnerQt::queueBackNavigation(int howFarBackward)
{
    //qDebug() << ">>>queueBackNavigation" << howFarBackward;
    for (int i = 0; i != howFarBackward; ++i)
        WorkQueue::shared()->queue(new BackItem(1));
}

void TestRunnerQt::queueForwardNavigation(int howFarForward)
{
    //qDebug() << ">>>queueForwardNavigation" << howFarForward;
    for (int i = 0; i != howFarForward; ++i)
        WorkQueue::shared()->queue(new ForwardItem(1));
}

void TestRunnerQt::queueLoadHTMLString(const QString& content, const QString& baseURL, const QString& failingURL)
{
    if (failingURL.isEmpty())
        WorkQueue::shared()->queue(new LoadHTMLStringItem(JSStringCreateWithQString(content).get(), JSStringCreateWithQString(baseURL).get()));
    else
        WorkQueue::shared()->queue(new LoadAlternateHTMLStringItem(JSStringCreateWithQString(content), JSStringCreateWithQString(baseURL), JSStringCreateWithQString(failingURL)));
}

void TestRunnerQt::queueReload()
{
    //qDebug() << ">>>queueReload";
    WorkQueue::shared()->queue(new ReloadItem());
}

void TestRunnerQt::queueLoadingScript(const QString& script)
{
    //qDebug() << ">>>queueLoadingScript" << script;
    WorkQueue::shared()->queue(new LoadingScriptItem(JSStringCreateWithQString(script).get()));
}

void TestRunnerQt::queueNonLoadingScript(const QString& script)
{
    //qDebug() << ">>>queueNonLoadingScript" << script;
    WorkQueue::shared()->queue(new NonLoadingScriptItem(JSStringCreateWithQString(script).get()));
}

void TestRunnerQt::provisionalLoad()
{
    QWebFrame* frame = qobject_cast<QWebFrame*>(sender());
    if (!m_topLoadingFrame && !m_hasDumped)
        m_topLoadingFrame = frame;
}

void TestRunnerQt::timerEvent(QTimerEvent *ev)
{
    if (ev->timerId() == m_timeoutTimer.timerId()) {
        const char* message = "FAIL: Timed out waiting for notifyDone to be called\n";
        fprintf(stderr, "%s", message);
        fprintf(stdout, "%s", message);
        notifyDone();
    } else
        QObject::timerEvent(ev);
}

QString TestRunnerQt::encodeHostName(const QString& host)
{
    QString encoded = QString::fromLatin1(QUrl::toAce(host + QLatin1String(".no")));
    encoded.truncate(encoded.length() - 3); // strip .no
    return encoded;
}

QString TestRunnerQt::decodeHostName(const QString& host)
{
    QString decoded = QUrl::fromAce(host.toLatin1() + QByteArray(".no"));
    decoded.truncate(decoded.length() - 3);
    return decoded;
}

void TestRunnerQt::closeWebInspector()
{
    DumpRenderTreeSupportQt::webInspectorClose(m_drt->pageAdapter());
    m_drt->webPage()->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, false);
}

void TestRunnerQt::setDeveloperExtrasEnabled(bool enabled)
{
    m_drt->webPage()->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, enabled);
}

void TestRunnerQt::showWebInspector()
{
    m_drt->webPage()->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    DumpRenderTreeSupportQt::webInspectorShow(m_drt->pageAdapter());
}

void TestRunnerQt::evaluateInWebInspector(long callId, const QString& script)
{
    DumpRenderTreeSupportQt::webInspectorExecuteScript(m_drt->pageAdapter(), callId, script);
}

void TestRunnerQt::setAllowUniversalAccessFromFileURLs(bool enabled)
{
    m_drt->webPage()->settings()->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, enabled);
}

void TestRunnerQt::setAllowFileAccessFromFileURLs(bool enabled)
{
    m_drt->webPage()->settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, enabled);
}

void TestRunnerQt::setAppCacheMaximumSize(unsigned long long quota)
{
    m_drt->webPage()->settings()->setOfflineWebApplicationCacheQuota(quota);
}

void TestRunnerQt::setValueForUser(const QWebElement& element, const QString& value)
{
    DumpRenderTreeSupportQt::setValueForUser(element, value);
}

void TestRunnerQt::setFixedContentsSize(int width, int height)
{
    m_topLoadingFrame->page()->setPreferredContentsSize(QSize(width, height));
}

void TestRunnerQt::setPrivateBrowsingEnabled(bool enable)
{
    m_drt->webPage()->settings()->setAttribute(QWebSettings::PrivateBrowsingEnabled, enable);
}

void TestRunnerQt::setSpatialNavigationEnabled(bool enable)
{
    m_drt->webPage()->settings()->setAttribute(QWebSettings::SpatialNavigationEnabled, enable);
}

void TestRunnerQt::setPopupBlockingEnabled(bool enable)
{
    m_drt->webPage()->settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, !enable);
}

void TestRunnerQt::setPOSIXLocale(const QString& locale)
{
    QLocale qlocale(locale);
    QLocale::setDefault(qlocale);
} 

void TestRunnerQt::setWindowIsKey(bool isKey)
{
    m_drt->switchFocus(isKey);
}

void TestRunnerQt::setJavaScriptCanAccessClipboard(bool enable)
{
    m_drt->webPage()->settings()->setAttribute(QWebSettings::JavascriptCanAccessClipboard, enable);
}

void TestRunnerQt::setXSSAuditorEnabled(bool enable)
{
    // Set XSSAuditingEnabled globally so that windows created by the test inherit it too.
    // resetSettings() will call this to reset the page and global setting to false again.
    // Needed by http/tests/security/xssAuditor/link-opens-new-window.html
    QWebSettings* globalSettings = QWebSettings::globalSettings();
    globalSettings->setAttribute(QWebSettings::XSSAuditingEnabled, enable);
    m_drt->webPage()->settings()->setAttribute(QWebSettings::XSSAuditingEnabled, enable);
}

void TestRunnerQt::clearAllApplicationCaches()
{
    DumpRenderTreeSupportQt::clearAllApplicationCaches();
}

void TestRunnerQt::setApplicationCacheOriginQuota(unsigned long long quota)
{
    if (!m_topLoadingFrame)
        return;
    m_topLoadingFrame->securityOrigin().setApplicationCacheQuota(quota);
}

QStringList TestRunnerQt::originsWithApplicationCache()
{
    // FIXME: Implement to get origins that have application caches.
    return QStringList();
}

void TestRunnerQt::setDatabaseQuota(int size)
{
    if (!m_topLoadingFrame)
        return;
    m_topLoadingFrame->securityOrigin().setDatabaseQuota(size);
}

void TestRunnerQt::clearAllDatabases()
{
    QWebDatabase::removeAllDatabases();
}

void TestRunnerQt::addOriginAccessWhitelistEntry(const QString& sourceOrigin, const QString& destinationProtocol, const QString& destinationHost, bool allowDestinationSubdomains)
{
    DumpRenderTreeSupportQt::whiteListAccessFromOrigin(sourceOrigin, destinationProtocol, destinationHost, allowDestinationSubdomains);
}

void TestRunnerQt::removeOriginAccessWhitelistEntry(const QString& sourceOrigin, const QString& destinationProtocol, const QString& destinationHost, bool allowDestinationSubdomains)
{
    DumpRenderTreeSupportQt::removeWhiteListAccessFromOrigin(sourceOrigin, destinationProtocol, destinationHost, allowDestinationSubdomains);
}

void TestRunnerQt::setCustomPolicyDelegate(bool enabled, bool permissive)
{
    DumpRenderTreeSupportQt::setCustomPolicyDelegate(enabled, permissive);
}

void TestRunnerQt::waitForPolicyDelegate()
{
    setCustomPolicyDelegate(true);
    m_waitForPolicy = true;
    waitUntilDone();
}

void TestRunnerQt::overridePreference(const QString& name, const QVariant& value)
{
    QWebSettings* settings = m_topLoadingFrame->page()->settings();

    if (name == "WebKitJavaScriptEnabled")
        settings->setAttribute(QWebSettings::JavascriptEnabled, value.toBool());
    else if (name == "WebKitTabToLinksPreferenceKey")
        settings->setAttribute(QWebSettings::LinksIncludedInFocusChain, value.toBool());
    else if (name == "WebKitOfflineWebApplicationCacheEnabled")
        settings->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, value.toBool());
    else if (name == "WebKitDefaultFontSize")
        settings->setFontSize(QWebSettings::DefaultFontSize, value.toInt());
    else if (name == "WebKitUsesPageCachePreferenceKey")
        QWebSettings::setMaximumPagesInCache(value.toInt());
    else if (name == "WebKitEnableCaretBrowsing")
        setCaretBrowsingEnabled(value.toBool());
    else if (name == "WebKitPluginsEnabled")
        settings->setAttribute(QWebSettings::PluginsEnabled, value.toBool());
    else if (name == "WebKitWebGLEnabled")
        settings->setAttribute(QWebSettings::WebGLEnabled, value.toBool());
    else if (name == "WebKitCSSRegionsEnabled")
        settings->setAttribute(QWebSettings::CSSRegionsEnabled, value.toBool());
    else if (name == "WebKitCSSGridLayoutEnabled")
        settings->setAttribute(QWebSettings::CSSGridLayoutEnabled, value.toBool());
    else if (name == "WebKitHyperlinkAuditingEnabled")
        settings->setAttribute(QWebSettings::HyperlinkAuditingEnabled, value.toBool());
    else if (name == "WebKitAcceleratedCompositingEnabled")
        settings->setAttribute(QWebSettings::AcceleratedCompositingEnabled, value.toBool());
    else if (name == "WebKitDisplayImagesKey")
        settings->setAttribute(QWebSettings::AutoLoadImages, value.toBool());
    else if (name == "WebKitWebAudioEnabled")
        settings->setAttribute(QWebSettings::WebAudioEnabled, value.toBool());
    else
        printf("ERROR: TestRunner::overridePreference() does not support the '%s' preference\n",
            name.toLatin1().data());
}

void TestRunnerQt::setUserStyleSheetLocation(const QString& url)
{
    QByteArray urlData = pathToLocalResource(url).toLatin1();
    m_userStyleSheetLocation = QUrl::fromEncoded(urlData, QUrl::StrictMode);

    if (m_userStyleSheetEnabled)
        setUserStyleSheetEnabled(true);
}

void TestRunnerQt::setCaretBrowsingEnabled(bool value)
{
    DumpRenderTreeSupportQt::setCaretBrowsingEnabled(m_drt->pageAdapter(), value);
}

void TestRunnerQt::setAuthorAndUserStylesEnabled(bool value)
{
    DumpRenderTreeSupportQt::setAuthorAndUserStylesEnabled(m_drt->pageAdapter(), value);
}

void TestRunnerQt::setUserStyleSheetEnabled(bool enabled)
{
    m_userStyleSheetEnabled = enabled;

    if (enabled)
        m_drt->webPage()->settings()->setUserStyleSheetUrl(m_userStyleSheetLocation);
    else
        m_drt->webPage()->settings()->setUserStyleSheetUrl(QUrl());
}

void TestRunnerQt::setDomainRelaxationForbiddenForURLScheme(bool forbidden, const QString& scheme)
{
    DumpRenderTreeSupportQt::setDomainRelaxationForbiddenForURLScheme(forbidden, scheme);
}

bool TestRunnerQt::callShouldCloseOnWebView()
{
    return DumpRenderTreeSupportQt::shouldClose(m_drt->mainFrameAdapter());
}

void TestRunnerQt::setScrollbarPolicy(const QString& orientation, const QString& policy)
{
    Qt::Orientation o;
    Qt::ScrollBarPolicy p;

    if (orientation == "vertical")
        o = Qt::Vertical;
    else if (orientation == "horizontal")
        o = Qt::Horizontal;
    else
        return;

    if (policy == "on")
        p = Qt::ScrollBarAlwaysOn;
    else if (policy == "auto")
        p = Qt::ScrollBarAsNeeded;
    else if (policy == "off")
        p = Qt::ScrollBarAlwaysOff;
    else
        return;

    m_drt->webPage()->mainFrame()->setScrollBarPolicy(o, p);
}

void TestRunnerQt::execCommand(const QString& name, const QString& value)
{
    DumpRenderTreeSupportQt::executeCoreCommandByName(m_drt->pageAdapter(), name, value);
}

bool TestRunnerQt::isCommandEnabled(const QString& name) const
{
    return DumpRenderTreeSupportQt::isCommandEnabled(m_drt->pageAdapter(), name);
}

bool TestRunner::findString(JSContextRef context, JSStringRef string, JSObjectRef optionsArray)
{
    JSRetainPtr<JSStringRef> lengthPropertyName(Adopt, JSStringCreateWithUTF8CString("length"));
    JSValueRef lengthValue = JSObjectGetProperty(context, optionsArray, lengthPropertyName.get(), 0);
    if (!JSValueIsNumber(context, lengthValue))
        return false;

    QWebPage::FindFlags findFlags = QWebPage::FindCaseSensitively;

    int length = static_cast<int>(JSValueToNumber(context, lengthValue, 0));
    for (int i = 0; i < length; ++i) {
        JSValueRef value = JSObjectGetPropertyAtIndex(context, optionsArray, i, 0);
        if (!JSValueIsString(context, value))
            continue;

        JSRetainPtr<JSStringRef> optionName(Adopt, JSValueToStringCopy(context, value, 0));
        if (JSStringIsEqualToUTF8CString(optionName.get(), "CaseInsensitive"))
            findFlags &= ~QWebPage::FindCaseSensitively;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "AtWordStarts"))
            findFlags |= QWebPage::FindAtWordBeginningsOnly;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "TreatMedialCapitalAsWordStart"))
            findFlags |=  QWebPage::TreatMedialCapitalAsWordBeginning;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "Backwards"))
            findFlags |=  QWebPage::FindBackward;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "WrapAround"))
            findFlags |=  QWebPage::FindWrapsAroundDocument;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "StartInSelection"))
            findFlags |=  QWebPage::FindBeginsInSelection;
    }

    DumpRenderTree* drt = DumpRenderTree::instance();
    return drt->webPage()->findText(JSStringCopyQString(string), findFlags);
}

void TestRunnerQt::setIconDatabaseEnabled(bool enable)
{
    if (enable && !m_drt->persistentStoragePath().isEmpty())
        QWebSettings::setIconDatabasePath(m_drt->persistentStoragePath());
    else
        QWebSettings::setIconDatabasePath(QString());
}

void TestRunnerQt::setMockDeviceOrientation(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma)
{
    QList<WebPage*> pages = m_drt->getAllPages();
    foreach (WebPage* page, pages)
        DumpRenderTreeSupportQt::setMockDeviceOrientation(page->handle(), canProvideAlpha, alpha, canProvideBeta, beta, canProvideGamma, gamma);
}

void TestRunnerQt::setGeolocationPermission(bool allow)
{
    setGeolocationPermissionCommon(allow);
    QList<WebPage*> pages = m_drt->getAllPages();
    foreach (WebPage* page, pages)
        DumpRenderTreeSupportQt::setMockGeolocationPermission(page->handle(), allow);
}

int TestRunnerQt::numberOfPendingGeolocationPermissionRequests()
{
    int pendingPermissionCount = 0;
    QList<WebPage*> pages = m_drt->getAllPages();
    foreach (WebPage* page, pages)
        pendingPermissionCount += DumpRenderTreeSupportQt::numberOfPendingGeolocationPermissionRequests(page->handle());

    return pendingPermissionCount;
}

void TestRunnerQt::setGeolocationPermissionCommon(bool allow)
{
     m_isGeolocationPermissionSet = true;
     m_geolocationPermission = allow;
}

void TestRunnerQt::setMockGeolocationPositionUnavailableError(const QString& message)
{
    QList<WebPage*> pages = m_drt->getAllPages();
    foreach (WebPage* page, pages)
        DumpRenderTreeSupportQt::setMockGeolocationPositionUnavailableError(page->handle(), message);
}

void TestRunnerQt::setMockGeolocationPosition(double latitude, double longitude, double accuracy)
{
    QList<WebPage*> pages = m_drt->getAllPages();
    foreach (WebPage* page, pages)
        DumpRenderTreeSupportQt::setMockGeolocationPosition(page->handle(), latitude, longitude, accuracy);
}

void TestRunnerQt::removeAllVisitedLinks()
{
    QWebHistory* history = m_drt->webPage()->history();
    history->clear();
    DumpRenderTreeSupportQt::dumpVisitedLinksCallbacks(true);
}

void TestRunnerQt::addURLToRedirect(const QString& origin, const QString& destination)
{
    DumpRenderTreeSupportQt::addURLToRedirect(origin, destination);
}

void TestRunnerQt::setAlwaysAcceptCookies(bool accept)
{
    QWebSettings* globalSettings = QWebSettings::globalSettings();
    if (accept)
        globalSettings->setThirdPartyCookiePolicy(QWebSettings::AlwaysAllowThirdPartyCookies);
    else {
        // This matches the Safari third-party cookie blocking policy tested in third-party-cookie-relaxing.html
        globalSettings->setThirdPartyCookiePolicy(QWebSettings::AllowThirdPartyWithExistingCookies);
    }
}

void TestRunnerQt::setAlwaysBlockCookies(bool block)
{
    QWebSettings* globalSettings = QWebSettings::globalSettings();
    if (block)
        globalSettings->setThirdPartyCookiePolicy(QWebSettings::AlwaysBlockThirdPartyCookies);
    else
        globalSettings->setThirdPartyCookiePolicy(QWebSettings::AlwaysAllowThirdPartyCookies);
}

void TestRunnerQt::setAudioResult(const QByteArray& audioData)
{
    m_audioData = audioData;
    m_audioDump = true;
}

// --- JSC C API stubs

void TestRunner::addDisallowedURL(JSStringRef url)
{
}

void TestRunner::queueLoad(JSStringRef url, JSStringRef target)
{
    DumpRenderTree* drt = DumpRenderTree::instance();
    QUrl mainResourceUrl = drt->webPage()->mainFrame()->url();
    QString absoluteUrl = mainResourceUrl.resolved(QUrl(JSStringCopyQString(url))).toEncoded();
    WorkQueue::shared()->queue(new LoadItem(JSStringCreateWithQString(absoluteUrl).get(), target));
}

void TestRunner::removeAllVisitedLinks()
{
}

void TestRunner::setAcceptsEditing(bool)
{
}

void TestRunner::simulateLegacyWebNotificationClick(JSStringRef title)
{
}

void TestRunner::setWindowIsKey(bool)
{
}

void TestRunner::setAlwaysAcceptCookies(bool)
{
}

void TestRunner::addOriginAccessWhitelistEntry(JSStringRef sourceOrigin, JSStringRef destinationProtocol, JSStringRef destinationHost, bool allowDestinationSubdomains)
{
}

void TestRunner::setWebViewEditable(bool)
{
}

void TestRunner::clearAllApplicationCaches()
{
}

void TestRunner::setTextDirection(JSStringRef directionName)
{
    QWebPage* webPage = DumpRenderTree::instance()->webPage();
    if (JSStringIsEqualToUTF8CString(directionName, "auto"))
        webPage->triggerAction(QWebPage::SetTextDirectionDefault);
    else if (JSStringIsEqualToUTF8CString(directionName, "rtl"))
        webPage->triggerAction(QWebPage::SetTextDirectionRightToLeft);
    else if (JSStringIsEqualToUTF8CString(directionName, "ltr"))
        webPage->triggerAction(QWebPage::SetTextDirectionLeftToRight);
}

void TestRunner::notifyDone()
{
}

int TestRunner::numberOfPendingGeolocationPermissionRequests()
{
    return 0;
}

void TestRunner::overridePreference(JSStringRef key, JSStringRef value)
{
}

JSStringRef TestRunner::pathToLocalResource(JSContextRef, JSStringRef url)
{
    return JSStringCreateWithUTF8CString(0); // ### Take impl from WTR
}

void TestRunner::removeAllWebNotificationPermissions()
{
}

void TestRunner::simulateWebNotificationClick(JSValueRef notification)
{
}

void TestRunner::closeIdleLocalStorageDatabases()
{
}

void TestRunner::focusWebView()
{
}

void TestRunner::setBackingScaleFactor(double)
{
}

void TestRunner::removeChromeInputField()
{
}

void TestRunner::addChromeInputField()
{
}

JSValueRef TestRunner::originsWithLocalStorage(JSContextRef context)
{
    return JSValueMakeNull(context);
}

void TestRunner::deleteAllLocalStorage()
{
}

void TestRunner::deleteLocalStorageForOrigin(JSStringRef originIdentifier)
{
}

void TestRunner::observeStorageTrackerNotifications(unsigned number)
{
}

void TestRunner::syncLocalStorage()
{
}

int TestRunner::windowCount()
{
    return 0;
}

void TestRunner::setWaitToDump(bool)
{
}

void TestRunner::waitForPolicyDelegate()
{
}

size_t TestRunner::webHistoryItemCount()
{
    return 0;
}

void TestRunner::showWebInspector()
{
}

void TestRunner::closeWebInspector()
{
}

void TestRunner::evaluateInWebInspector(long callId, JSStringRef script)
{
}

void TestRunner::setSerializeHTTPLoads(bool)
{
}

void TestRunner::apiTestNewWindowDataLoadBaseURL(JSStringRef utf8Data, JSStringRef baseURL)
{
}

void TestRunner::setCustomPolicyDelegate(bool setDelegate, bool permissive)
{
}

void TestRunner::setDatabaseQuota(unsigned long long quota)
{
}

void TestRunner::setDomainRelaxationForbiddenForURLScheme(bool forbidden, JSStringRef scheme)
{
}

void TestRunner::resetPageVisibility()
{
    DumpRenderTreeSupportQt::resetPageVisibility(DumpRenderTree::instance()->pageAdapter());
}

void TestRunner::setPageVisibility(const char* visibility)
{
    QLatin1String newVisibility = QLatin1String(visibility);
    if (newVisibility == QStringLiteral("visible"))
        DumpRenderTree::instance()->webPage()->setVisibilityState(QWebPage::VisibilityStateVisible);
    else if (newVisibility == QStringLiteral("hidden"))
        DumpRenderTree::instance()->webPage()->setVisibilityState(QWebPage::VisibilityStateHidden);
    else if (newVisibility == QStringLiteral("prerender"))
        DumpRenderTree::instance()->webPage()->setVisibilityState(QWebPage::VisibilityStatePrerender);
    else if (newVisibility == QStringLiteral("unloaded"))
        DumpRenderTree::instance()->webPage()->setVisibilityState(QWebPage::VisibilityStateUnloaded);
}

void TestRunner::keepWebHistory()
{
}

void TestRunner::goBack()
{
    DumpRenderTreeSupportQt::goBack(DumpRenderTree::instance()->pageAdapter());
}

JSValueRef TestRunner::originsWithApplicationCache(JSContextRef context)
{
    return JSValueMakeNull(context);
}

long long TestRunner::applicationCacheDiskUsageForOrigin(JSStringRef name)
{
    return 0;
}

void TestRunner::display()
{
}

void TestRunner::dispatchPendingLoadRequests()
{
}

void TestRunner::clearPersistentUserStyleSheet()
{
}

bool TestRunner::callShouldCloseOnWebView()
{
    return false;
}

JSStringRef TestRunner::copyDecodedHostName(JSStringRef name)
{
    return JSStringCreateWithUTF8CString(0);
}

void TestRunner::clearBackForwardList()
{
}

void TestRunner::clearAllDatabases()
{
}

void TestRunner::clearApplicationCacheForOrigin(JSStringRef name)
{
}

void TestRunner::apiTestGoToCurrentBackForwardItem()
{
}

void TestRunner::authenticateSession(JSStringRef url, JSStringRef username, JSStringRef password)
{
}

void TestRunner::abortModal()
{
}

void TestRunner::setStorageDatabaseIdleInterval(double)
{
}

void TestRunner::setXSSAuditorEnabled(bool flag)
{
}

void TestRunner::setSpatialNavigationEnabled(bool)
{
}

void TestRunner::setScrollbarPolicy(JSStringRef orientation, JSStringRef policy)
{
}

void TestRunner::setJavaScriptCanAccessClipboard(bool flag)
{
}

void TestRunner::setAutomaticLinkDetectionEnabled(bool flag)
{
}

void TestRunner::setUserStyleSheetEnabled(bool flag)
{
}

void TestRunner::setUserStyleSheetLocation(JSStringRef path)
{
}

void TestRunner::setUseDashboardCompatibilityMode(bool flag)
{
}

void TestRunner::setTabKeyCyclesThroughElements(bool)
{
}

void TestRunner::setPrivateBrowsingEnabled(bool)
{
}

void TestRunner::setPluginsEnabled(bool)
{
}

void TestRunner::setPopupBlockingEnabled(bool)
{
}

void TestRunner::setMockSpeechInputDumpRect(bool flag)
{
}

void TestRunner::setPersistentUserStyleSheetLocation(JSStringRef path)
{
}

void TestRunner::setMockGeolocationPosition(double latitude, double longitude, double accuracy, bool providesAltitude, double altitude, bool providesAltitudeAccuracy, double altitudeAccuracy, bool providesHeading, double heading, bool providesSpeed, double speed)
{
}

void TestRunner::setMockGeolocationPositionUnavailableError(JSStringRef message)
{
}

void TestRunner::setMockDeviceOrientation(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma)
{
}

void TestRunner::setMainFrameIsFirstResponder(bool flag)
{
}

void TestRunner::setIconDatabaseEnabled(bool)
{
}

void TestRunner::setGeolocationPermission(bool allow)
{
}

void TestRunner::setDefersLoading(bool flag)
{
    DumpRenderTreeSupportQt::setDefersLoading(DumpRenderTree::instance()->pageAdapter(), flag);
}

void TestRunner::setCacheModel(int)
{
}

void TestRunner::setAuthorAndUserStylesEnabled(bool)
{
}

void TestRunner::setAllowFileAccessFromFileURLs(bool)
{
}

void TestRunner::setAppCacheMaximumSize(unsigned long long quota)
{
}

void TestRunner::setAllowUniversalAccessFromFileURLs(bool)
{
}

void TestRunner::setApplicationCacheOriginQuota(unsigned long long)
{
}

void TestRunner::denyWebNotificationPermission(JSStringRef origin)
{
}

void TestRunner::grantWebNotificationPermission(JSStringRef origin)
{
}

void TestRunner::setValueForUser(JSContextRef, JSValueRef nodeObject, JSStringRef value)
{
}

void TestRunner::setViewModeMediaFeature(JSStringRef)
{
}

void TestRunner::addMockSpeechInputResult(JSStringRef result, double confidence, JSStringRef language)
{
}

void TestRunner::removeOriginAccessWhitelistEntry(JSStringRef sourceOrigin, JSStringRef destinationProtocol, JSStringRef destinationHost, bool allowDestinationSubdomains)
{
}

void TestRunner::addUserScript(JSStringRef source, bool runAtStart, bool allFrames)
{
}

bool TestRunner::isCommandEnabled(JSStringRef name)
{
    return false;
}

void TestRunner::evaluateScriptInIsolatedWorld(unsigned worldID, JSObjectRef globalObject, JSStringRef script)
{
    DumpRenderTreeSupportQt::evaluateScriptInIsolatedWorld(DumpRenderTree::instance()->mainFrameAdapter(), worldID, JSStringCopyQString(script));
}

void TestRunner::evaluateScriptInIsolatedWorldAndReturnValue(unsigned worldID, JSObjectRef globalObject, JSStringRef script)
{
}

JSStringRef TestRunner::copyEncodedHostName(JSStringRef name)
{
    return JSStringCreateWithUTF8CString(0);
}

void TestRunner::addUserStyleSheet(JSStringRef source, bool allFrames)
{
    DumpRenderTreeSupportQt::addUserStyleSheet(DumpRenderTree::instance()->pageAdapter(), JSStringCopyQString(source));
}

void TestRunner::execCommand(JSStringRef name, JSStringRef value)
{
}

long long TestRunner::localStorageDiskUsageForOrigin(JSStringRef originIdentifier)
{
    return 0;
}

