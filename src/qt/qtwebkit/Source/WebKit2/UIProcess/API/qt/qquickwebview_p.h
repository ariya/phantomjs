/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (c) 2012 Hewlett-Packard Development Company, L.P.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef qquickwebview_p_h
#define qquickwebview_p_h

#include "qquickurlschemedelegate_p.h"
#include "qwebkitglobal.h"
#include <QtQml/qqmllist.h>
#include <QtQuick/qquickitem.h>
#include <private/qquickflickable_p.h>

QT_BEGIN_NAMESPACE
class QQmlComponent;
QT_END_NAMESPACE
class QWebNavigationRequest;
class QQuickWebPage;
class QQuickWebViewAttached;
class QWebLoadRequest;
class QQuickWebViewPrivate;
class QQuickWebViewExperimental;
class QWebDownloadItem;
class QWebNavigationHistory;
class QWebPreferences;
class QWebPermissionRequest;
class QWebKitTest;
class QQuickNetworkReply;

namespace WTR {
class PlatformWebView;
}

namespace TestWebKitAPI {
class PlatformWebView;
}

namespace WebKit {
struct QtRefCountedNetworkRequestData;
class PageViewportControllerClientQt;
class QtWebPagePolicyClient;
class QtWebPageUIClient;
}

namespace WTF {
template<class T> class PassRefPtr;
}

typedef const struct OpaqueWKContext* WKContextRef;
typedef const struct OpaqueWKPageGroup* WKPageGroupRef;
typedef const struct OpaqueWKPage* WKPageRef;

QT_BEGIN_NAMESPACE
class QPainter;
class QUrl;
class QQuickFlickable;
class QJSValue;
QT_END_NAMESPACE


// Instantiating the WebView in C++ is only possible by creating
// a QQmlComponent as the initialization depends on the
// componentComplete method being called.
class QWEBKIT_EXPORT QQuickWebView : public QQuickFlickable {
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(QUrl icon READ icon NOTIFY iconChanged FINAL)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY navigationHistoryChanged FINAL)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY navigationHistoryChanged FINAL)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged FINAL)
    Q_PROPERTY(int loadProgress READ loadProgress NOTIFY loadProgressChanged)
    Q_ENUMS(NavigationRequestAction)
    Q_ENUMS(LoadStatus)
    Q_ENUMS(ErrorDomain)
    Q_ENUMS(NavigationType)

public:
    enum NavigationRequestAction {
        AcceptRequest,
        // Make room in the valid range of the enum for extra actions exposed in Experimental.
        IgnoreRequest = 0xFF
    };
    enum LoadStatus {
        LoadStartedStatus,
        LoadStoppedStatus,
        LoadSucceededStatus,
        LoadFailedStatus
    };
    enum ErrorDomain {
        NoErrorDomain,
        InternalErrorDomain,
        NetworkErrorDomain,
        HttpErrorDomain,
        DownloadErrorDomain
    };

    enum NavigationType {
        LinkClickedNavigation,
        FormSubmittedNavigation,
        BackForwardNavigation,
        ReloadNavigation,
        FormResubmittedNavigation,
        OtherNavigation
    };

    QQuickWebView(QQuickItem* parent = 0);
    virtual ~QQuickWebView();

    QUrl url() const;
    void setUrl(const QUrl&);
    QUrl icon() const;
    QString title() const;
    int loadProgress() const;

    bool canGoBack() const;
    bool canGoForward() const;
    bool loading() const;

    virtual QVariant inputMethodQuery(Qt::InputMethodQuery property) const;

    QPointF mapToWebContent(const QPointF&) const;
    QRectF mapRectToWebContent(const QRectF&) const;
    QPointF mapFromWebContent(const QPointF&) const;
    QRectF mapRectFromWebContent(const QRectF&) const;

    QQuickWebPage* page();

    QQuickWebViewExperimental* experimental() const;
    static QQuickWebViewAttached* qmlAttachedProperties(QObject*);

    static void platformInitialize(); // Only needed by WTR.

    // Private C++-only API.
    qreal zoomFactor() const;
    void setZoomFactor(qreal);
    void runJavaScriptInMainFrame(const QString& script, QObject* receiver, const char* method);
    // Used to automatically accept the HTTPS certificate in WTR. No other use intended.
    bool allowAnyHTTPSCertificateForLocalHost() const;
    void setAllowAnyHTTPSCertificateForLocalHost(bool allow);

public Q_SLOTS:
    void loadHtml(const QString& html, const QUrl& baseUrl = QUrl(), const QUrl& unreachableUrl = QUrl());

    void goBack();
    void goForward();
    void stop();
    void reload();

Q_SIGNALS:
    void titleChanged();
    void navigationHistoryChanged();
    void loadingChanged(QWebLoadRequest* loadRequest);
    void loadProgressChanged();
    void urlChanged();
    void iconChanged();
    void linkHovered(const QUrl& hoveredUrl, const QString& hoveredTitle);
    void navigationRequested(QWebNavigationRequest* request);

protected:
    virtual bool childMouseEventFilter(QQuickItem*, QEvent*);
    virtual void geometryChanged(const QRectF&, const QRectF&);
    virtual void componentComplete();
    virtual void keyPressEvent(QKeyEvent*);
    virtual void keyReleaseEvent(QKeyEvent*);
    virtual void inputMethodEvent(QInputMethodEvent*);
    virtual void focusInEvent(QFocusEvent*);
    virtual void itemChange(ItemChange change, const ItemChangeData &value);
    virtual void touchEvent(QTouchEvent*);
    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void mouseDoubleClickEvent(QMouseEvent*);
    virtual void wheelEvent(QWheelEvent*);
    virtual void hoverEnterEvent(QHoverEvent*);
    virtual void hoverMoveEvent(QHoverEvent*);
    virtual void hoverLeaveEvent(QHoverEvent*);
    virtual void dragMoveEvent(QDragMoveEvent*);
    virtual void dragEnterEvent(QDragEnterEvent*);
    virtual void dragLeaveEvent(QDragLeaveEvent*);
    virtual void dropEvent(QDropEvent*);
    virtual bool event(QEvent*);

private:
    Q_DECLARE_PRIVATE(QQuickWebView)

    void handleFlickableMousePress(const QPointF& position, qint64 eventTimestampMillis);
    void handleFlickableMouseMove(const QPointF& position, qint64 eventTimestampMillis);
    void handleFlickableMouseRelease(const QPointF& position, qint64 eventTimestampMillis);

    QPointF contentPos() const;
    void setContentPos(const QPointF&);

    QQuickWebView(WKContextRef, WKPageGroupRef, QQuickItem* parent = 0);
    WKPageRef pageRef() const;

    void emitUrlChangeIfNeeded();

    Q_PRIVATE_SLOT(d_func(), void _q_onVisibleChanged());
    Q_PRIVATE_SLOT(d_func(), void _q_onUrlChanged());
    Q_PRIVATE_SLOT(d_func(), void _q_onReceivedResponseFromDownload(QWebDownloadItem*));
    Q_PRIVATE_SLOT(d_func(), void _q_onIconChangedForPageURL(const QString&));

    // Hides QObject::d_ptr allowing us to use the convenience macros.
    QScopedPointer<QQuickWebViewPrivate> d_ptr;

    friend class QWebKitTest;
    friend class WebKit::PageViewportControllerClientQt;
    friend class WebKit::QtWebPagePolicyClient;
    friend class WebKit::QtWebPageUIClient;
    friend class WTR::PlatformWebView;
    friend class TestWebKitAPI::PlatformWebView;
    friend class QQuickWebViewExperimental;
};

QML_DECLARE_TYPE(QQuickWebView)

class QWEBKIT_EXPORT QQuickWebViewAttached : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQuickWebView* view READ view NOTIFY viewChanged FINAL)

public:
    QQuickWebViewAttached(QObject* object);
    QQuickWebView* view() const { return m_view; }
    void setView(QQuickWebView*);

Q_SIGNALS:
    void viewChanged();

private:
    QQuickWebView* m_view;
};

QML_DECLARE_TYPEINFO(QQuickWebView, QML_HAS_ATTACHED_PROPERTIES)

class QWEBKIT_EXPORT QQuickWebViewExperimental : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQuickWebPage* page READ page CONSTANT FINAL)

    Q_PROPERTY(bool transparentBackground WRITE setTransparentBackground READ transparentBackground)
    Q_PROPERTY(bool useDefaultContentItemSize WRITE setUseDefaultContentItemSize READ useDefaultContentItemSize)

    Q_PROPERTY(int preferredMinimumContentsWidth WRITE setPreferredMinimumContentsWidth READ preferredMinimumContentsWidth NOTIFY preferredMinimumContentsWidthChanged)
    Q_PROPERTY(int deviceWidth WRITE setDeviceWidth READ deviceWidth NOTIFY deviceWidthChanged)
    Q_PROPERTY(int deviceHeight WRITE setDeviceHeight READ deviceHeight NOTIFY deviceHeightChanged)

    Q_PROPERTY(QWebNavigationHistory* navigationHistory READ navigationHistory CONSTANT FINAL)

    Q_PROPERTY(QQmlComponent* alertDialog READ alertDialog WRITE setAlertDialog NOTIFY alertDialogChanged)
    Q_PROPERTY(QQmlComponent* confirmDialog READ confirmDialog WRITE setConfirmDialog NOTIFY confirmDialogChanged)
    Q_PROPERTY(QQmlComponent* promptDialog READ promptDialog WRITE setPromptDialog NOTIFY promptDialogChanged)
    Q_PROPERTY(QQmlComponent* authenticationDialog READ authenticationDialog WRITE setAuthenticationDialog NOTIFY authenticationDialogChanged)
    Q_PROPERTY(QQmlComponent* proxyAuthenticationDialog READ proxyAuthenticationDialog WRITE setProxyAuthenticationDialog NOTIFY proxyAuthenticationDialogChanged)
    Q_PROPERTY(QQmlComponent* certificateVerificationDialog READ certificateVerificationDialog WRITE setCertificateVerificationDialog NOTIFY certificateVerificationDialogChanged)
    Q_PROPERTY(QQmlComponent* itemSelector READ itemSelector WRITE setItemSelector NOTIFY itemSelectorChanged)
    Q_PROPERTY(QQmlComponent* filePicker READ filePicker WRITE setFilePicker NOTIFY filePickerChanged)
    Q_PROPERTY(QQmlComponent* databaseQuotaDialog READ databaseQuotaDialog WRITE setDatabaseQuotaDialog NOTIFY databaseQuotaDialogChanged)
    Q_PROPERTY(QQmlComponent* colorChooser READ colorChooser WRITE setColorChooser NOTIFY colorChooserChanged)

    Q_PROPERTY(QWebPreferences* preferences READ preferences CONSTANT FINAL)
    Q_PROPERTY(QWebKitTest* test READ test CONSTANT FINAL)
    Q_PROPERTY(QQmlListProperty<QQuickUrlSchemeDelegate> urlSchemeDelegates READ schemeDelegates)
    Q_PROPERTY(QString userAgent READ userAgent WRITE setUserAgent NOTIFY userAgentChanged)
    Q_PROPERTY(QList<QUrl> userScripts READ userScripts WRITE setUserScripts NOTIFY userScriptsChanged)
    Q_PROPERTY(QUrl remoteInspectorUrl READ remoteInspectorUrl NOTIFY remoteInspectorUrlChanged FINAL)
    Q_ENUMS(NavigationRequestActionExperimental)
    Q_FLAGS(FindFlags)

public:
    enum NavigationRequestActionExperimental {
        DownloadRequest = QQuickWebView::IgnoreRequest - 1
    };

    enum FindFlag {
        FindCaseSensitively = 1 << 0,
        FindBackward = 1 << 1,
        FindWrapsAroundDocument = 1 << 2,
        FindHighlightAllOccurrences = 1 << 3
    };
    Q_DECLARE_FLAGS(FindFlags, FindFlag)

    virtual ~QQuickWebViewExperimental();

    QQmlComponent* alertDialog() const;
    void setAlertDialog(QQmlComponent*);
    QQmlComponent* confirmDialog() const;
    void setConfirmDialog(QQmlComponent*);
    QQmlComponent* promptDialog() const;
    void setPromptDialog(QQmlComponent*);
    QQmlComponent* authenticationDialog() const;
    void setAuthenticationDialog(QQmlComponent*);
    QQmlComponent* certificateVerificationDialog() const;
    void setCertificateVerificationDialog(QQmlComponent*);
    QQmlComponent* itemSelector() const;
    void setItemSelector(QQmlComponent*);
    QQmlComponent* proxyAuthenticationDialog() const;
    void setProxyAuthenticationDialog(QQmlComponent*);
    QQmlComponent* filePicker() const;
    void setFilePicker(QQmlComponent*);
    QQmlComponent* databaseQuotaDialog() const;
    void setDatabaseQuotaDialog(QQmlComponent*);
    QQmlComponent* colorChooser() const;
    void setColorChooser(QQmlComponent*);
    QString userAgent() const;
    void setUserAgent(const QString& userAgent);
    int deviceWidth() const;
    void setDeviceWidth(int);
    int deviceHeight() const;
    void setDeviceHeight(int);
    QList<QUrl> userScripts() const;
    void setUserScripts(const QList<QUrl>& userScripts);
    QUrl remoteInspectorUrl() const;

    QWebKitTest* test();

    QWebPreferences* preferences() const;
    QWebNavigationHistory* navigationHistory() const;
    QQuickWebPage* page();

    static QQuickUrlSchemeDelegate* schemeDelegates_At(QQmlListProperty<QQuickUrlSchemeDelegate>*, int index);
    static void schemeDelegates_Append(QQmlListProperty<QQuickUrlSchemeDelegate>*, QQuickUrlSchemeDelegate*);
    static int schemeDelegates_Count(QQmlListProperty<QQuickUrlSchemeDelegate>*);
    static void schemeDelegates_Clear(QQmlListProperty<QQuickUrlSchemeDelegate>*);
    QQmlListProperty<QQuickUrlSchemeDelegate> schemeDelegates();
    void invokeApplicationSchemeHandler(WTF::PassRefPtr<WebKit::QtRefCountedNetworkRequestData>);
    void sendApplicationSchemeReply(QQuickNetworkReply*);

    bool transparentBackground() const;
    void setTransparentBackground(bool);

    bool useDefaultContentItemSize() const;
    void setUseDefaultContentItemSize(bool enable);

    int preferredMinimumContentsWidth() const;
    void setPreferredMinimumContentsWidth(int);

    // C++ only
    bool renderToOffscreenBuffer() const;
    void setRenderToOffscreenBuffer(bool enable);
    static void setFlickableViewportEnabled(bool enable);
    static bool flickableViewportEnabled();

public Q_SLOTS:
    void goBackTo(int index);
    void goForwardTo(int index);
    void postMessage(const QString&);
    void evaluateJavaScript(const QString& script, const QJSValue& value = QJSValue());
    void findText(const QString& string, FindFlags options = 0);

Q_SIGNALS:
    void loadVisuallyCommitted();
    void alertDialogChanged();
    void confirmDialogChanged();
    void promptDialogChanged();
    void authenticationDialogChanged();
    void certificateVerificationDialogChanged();
    void itemSelectorChanged();
    void filePickerChanged();
    void databaseQuotaDialogChanged();
    void colorChooserChanged();
    void downloadRequested(QWebDownloadItem* downloadItem);
    void permissionRequested(QWebPermissionRequest* permission);
    void messageReceived(const QVariantMap& message);
    void proxyAuthenticationDialogChanged();
    void userAgentChanged();
    void deviceWidthChanged();
    void deviceHeightChanged();
    void enterFullScreenRequested();
    void exitFullScreenRequested();
    void userScriptsChanged();
    void preferredMinimumContentsWidthChanged();
    void remoteInspectorUrlChanged();
    void textFound(int matchCount);

    void processDidCrash();
    void didRelaunchProcess();
    void processDidBecomeUnresponsive();
    void processDidBecomeResponsive();

private:
    QQuickWebViewExperimental(QQuickWebView* webView, QQuickWebViewPrivate* webViewPrivate);
    QQuickWebView* q_ptr;
    QQuickWebViewPrivate* d_ptr;
    QObject* schemeParent;
    QWebKitTest* m_test;

    friend class WebKit::QtWebPageUIClient;

    Q_DECLARE_PRIVATE(QQuickWebView)
    Q_DECLARE_PUBLIC(QQuickWebView)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickWebViewExperimental::FindFlags)

#endif // qquickwebview_p_h
