/*
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/
 * Copyright (C) 2012 Apple Inc. All Rights Reserved.
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

#ifndef TestRunnerQt_h
#define TestRunnerQt_h

#include <QBasicTimer>
#include <QObject>
#include <QSize>
#include <QString>
#include <QTimer>
#include <QTimerEvent>
#include <QVariant>
#include <QtDebug>

#include <qwebdatabase.h>
#include <qwebelement.h>
#include <qwebframe.h>
#include <qwebhistory.h>
#include <qwebpage.h>
#include <qwebsecurityorigin.h>

class QWebFrame;
class DumpRenderTreeSupportQt;
class DumpRenderTree;

class TestRunnerQt : public QObject {
    Q_OBJECT
    Q_PROPERTY(int webHistoryItemCount READ webHistoryItemCount)
    Q_PROPERTY(bool globalFlag READ globalFlag WRITE setGlobalFlag)
public:
    TestRunnerQt(DumpRenderTree*);

    bool shouldDumpAsAudio() const { return m_audioDump; }
    bool shouldWaitUntilDone() const { return m_waitForDone; }
    bool shouldHandleErrorPages() const { return m_handleErrorPages; }
    bool waitForPolicy() const { return m_waitForPolicy; }
    bool ignoreReqestForPermission() const { return m_ignoreDesktopNotification; }

    const QByteArray& audioData() const { return m_audioData; }

    void reset();

    void setTimeout(int timeout) { m_timeout = timeout; }
    void setShouldTimeout(bool flag) { m_shouldTimeout = flag; }

protected:
    void timerEvent(QTimerEvent*);

Q_SIGNALS:
    void done();

    void showPage();
    void hidePage();
    void geolocationPermissionSet();

public Q_SLOTS:
    void maybeDump(bool ok);
    void dumpNotifications();
    void waitUntilDone();
    int webHistoryItemCount();
    void keepWebHistory();
    void notifyDone();
    bool globalFlag() const { return m_globalFlag; }
    void setGlobalFlag(bool flag) { m_globalFlag = flag; }
    void handleErrorPages() { m_handleErrorPages = true; }
    void dumpEditingCallbacks();
    void dumpFrameLoadCallbacks();
    void dumpProgressFinishedCallback();
    void dumpUserGestureInFrameLoadCallbacks();
    void dumpResourceLoadCallbacks();
    void dumpResourceResponseMIMETypes();
    void dumpWillCacheResponse();
    void dumpHistoryCallbacks();
    void setWillSendRequestReturnsNullOnRedirect(bool enabled);
    void setWillSendRequestReturnsNull(bool enabled);
    void setWillSendRequestClearHeader(const QStringList& headers);
    void queueBackNavigation(int howFarBackward);
    void queueForwardNavigation(int howFarForward);
    void queueLoadHTMLString(const QString& content, const QString& baseURL = QString(), const QString& failingURL = QString());
    void queueReload();
    void queueLoadingScript(const QString& script);
    void queueNonLoadingScript(const QString& script);
    void provisionalLoad();
    int windowCount();
    void ignoreLegacyWebNotificationPermissionRequests();
    void simulateLegacyWebNotificationClick(const QString& title);
    void grantWebNotificationPermission(const QString& origin);
    void denyWebNotificationPermission(const QString& origin);
    void removeAllWebNotificationPermissions();
    void display();
    void displayInvalidatedRegion();
    void clearBackForwardList();
    QString pathToLocalResource(const QString& url);
    QString encodeHostName(const QString& host);
    QString decodeHostName(const QString& host);
    void dumpSelectionRect() const { }
    void setDeveloperExtrasEnabled(bool);
    void showWebInspector();
    void closeWebInspector();
    void evaluateInWebInspector(long callId, const QString& script);
    void removeAllVisitedLinks();
    void setAllowUniversalAccessFromFileURLs(bool enable);
    void setAllowFileAccessFromFileURLs(bool enable);
    void setAppCacheMaximumSize(unsigned long long quota);
    void setValueForUser(const QWebElement&, const QString& value);
    void setFixedContentsSize(int width, int height);
    void setPrivateBrowsingEnabled(bool);
    void setSpatialNavigationEnabled(bool);
    void setPopupBlockingEnabled(bool);
    void setPOSIXLocale(const QString& locale);
    void resetLoadFinished() { m_loadFinished = false; }
    void setWindowIsKey(bool);
    void setDeferMainResourceDataLoad(bool);
    void setJavaScriptCanAccessClipboard(bool enable);
    void setXSSAuditorEnabled(bool);
    void setCaretBrowsingEnabled(bool);
    void setAuthorAndUserStylesEnabled(bool);
    void setViewModeMediaFeature(const QString& mode);
    void execCommand(const QString& name, const QString& value = QString());
    bool isCommandEnabled(const QString& name) const;

    void addOriginAccessWhitelistEntry(const QString& sourceOrigin, const QString& destinationProtocol, const QString& destinationHost, bool allowDestinationSubdomains);
    void removeOriginAccessWhitelistEntry(const QString& sourceOrigin, const QString& destinationProtocol, const QString& destinationHost, bool allowDestinationSubdomains);

    void clearAllApplicationCaches();
    void setApplicationCacheOriginQuota(unsigned long long);
    QStringList originsWithApplicationCache();

    void setDatabaseQuota(int size);
    void clearAllDatabases();
    void setIconDatabaseEnabled(bool);

    void setCustomPolicyDelegate(bool enabled, bool permissive = false);
    void waitForPolicyDelegate();

    void overridePreference(const QString& name, const QVariant& value);
    void setUserStyleSheetLocation(const QString& url);
    void setUserStyleSheetEnabled(bool);
    void setDomainRelaxationForbiddenForURLScheme(bool forbidden, const QString& scheme);
    bool callShouldCloseOnWebView();

    void setMockDeviceOrientation(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma);

    void setMockGeolocationPositionUnavailableError(const QString& message);
    void setMockGeolocationPosition(double latitude, double longitude, double accuracy);
    void setGeolocationPermission(bool allow);
    int numberOfPendingGeolocationPermissionRequests();
    bool isGeolocationPermissionSet() const { return m_isGeolocationPermissionSet; }
    bool geolocationPermission() const { return m_geolocationPermission; }

    void addURLToRedirect(const QString& origin, const QString& destination);

    /*
        Policy values: 'on', 'auto' or 'off'.
        Orientation values: 'vertical' or 'horizontal'.
    */
    void setScrollbarPolicy(const QString& orientation, const QString& policy);

    void setAlwaysAcceptCookies(bool);
    void setAlwaysBlockCookies(bool);

    void setAudioResult(const QByteArray&);

private Q_SLOTS:
    void processWork();

private:
    void setGeolocationPermissionCommon(bool allow);

private:
    bool m_hasDumped;
    bool m_audioDump;
    bool m_disallowIncreaseForApplicationCacheQuota;
    bool m_canOpenWindows;
    bool m_waitForDone;
    bool m_waitForPolicy;
    bool m_handleErrorPages;
    bool m_loadFinished;
    bool m_globalFlag;
    bool m_userStyleSheetEnabled;
    bool m_isGeolocationPermissionSet;
    bool m_geolocationPermission;

    QUrl m_userStyleSheetLocation;
    QBasicTimer m_timeoutTimer;
    QWebFrame* m_topLoadingFrame;
    DumpRenderTree* m_drt;
    QWebHistory* m_webHistory;
    bool m_ignoreDesktopNotification;

    QByteArray m_audioData;

    bool m_shouldTimeout;
    int m_timeout;
};

#endif // TestRunnerQt_h
