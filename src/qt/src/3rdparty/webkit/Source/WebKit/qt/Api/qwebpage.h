/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2007 Staikos Computing Services Inc.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef QWEBPAGE_H
#define QWEBPAGE_H

#include "qwebsettings.h"
#include "qwebkitglobal.h"

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include <QtGui/qwidget.h>

QT_BEGIN_NAMESPACE
class QNetworkProxy;
class QUndoStack;
class QMenu;
class QNetworkRequest;
class QNetworkReply;
class QNetworkAccessManager;
QT_END_NAMESPACE

class QWebElement;
class QWebFrame;
class QWebNetworkRequest;
class QWebHistory;

class QWebFrameData;
class QWebHistoryItem;
class QWebHitTestResult;
class QWebNetworkInterface;
class QWebPagePrivate;
class QWebPluginFactory;
class QWebSecurityOrigin;
class QtViewportAttributesPrivate;

namespace WebCore {
    class ChromeClientQt;
    class EditorClientQt;
    class FrameLoaderClientQt;
    class InspectorClientQt;
    class InspectorServerRequestHandlerQt;
    class InspectorFrontendClientQt;
    class NotificationPresenterClientQt;
    class GeolocationPermissionClientQt;
    class ResourceHandle;
    class HitTestResult;
    class QNetworkReplyHandler;

    struct FrameLoadRequest;
}

class QWEBKIT_EXPORT QWebPage : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool modified READ isModified)
    Q_PROPERTY(QString selectedText READ selectedText)
    Q_PROPERTY(QString selectedHtml READ selectedHtml)
    Q_PROPERTY(bool hasSelection READ hasSelection)
    Q_PROPERTY(QSize viewportSize READ viewportSize WRITE setViewportSize)
    Q_PROPERTY(QSize preferredContentsSize READ preferredContentsSize WRITE setPreferredContentsSize)
    Q_PROPERTY(bool forwardUnsupportedContent READ forwardUnsupportedContent WRITE setForwardUnsupportedContent)
    Q_PROPERTY(LinkDelegationPolicy linkDelegationPolicy READ linkDelegationPolicy WRITE setLinkDelegationPolicy)
    Q_PROPERTY(QPalette palette READ palette WRITE setPalette)
    Q_PROPERTY(bool contentEditable READ isContentEditable WRITE setContentEditable)
    Q_ENUMS(LinkDelegationPolicy NavigationType WebAction)
public:
    enum NavigationType {
        NavigationTypeLinkClicked,
        NavigationTypeFormSubmitted,
        NavigationTypeBackOrForward,
        NavigationTypeReload,
        NavigationTypeFormResubmitted,
        NavigationTypeOther
    };

    enum WebAction {
        NoWebAction = - 1,

        OpenLink,

        OpenLinkInNewWindow,
        OpenFrameInNewWindow,

        DownloadLinkToDisk,
        CopyLinkToClipboard,

        OpenImageInNewWindow,
        DownloadImageToDisk,
        CopyImageToClipboard,

        Back,
        Forward,
        Stop,
        Reload,

        Cut,
        Copy,
        Paste,

        Undo,
        Redo,
        MoveToNextChar,
        MoveToPreviousChar,
        MoveToNextWord,
        MoveToPreviousWord,
        MoveToNextLine,
        MoveToPreviousLine,
        MoveToStartOfLine,
        MoveToEndOfLine,
        MoveToStartOfBlock,
        MoveToEndOfBlock,
        MoveToStartOfDocument,
        MoveToEndOfDocument,
        SelectNextChar,
        SelectPreviousChar,
        SelectNextWord,
        SelectPreviousWord,
        SelectNextLine,
        SelectPreviousLine,
        SelectStartOfLine,
        SelectEndOfLine,
        SelectStartOfBlock,
        SelectEndOfBlock,
        SelectStartOfDocument,
        SelectEndOfDocument,
        DeleteStartOfWord,
        DeleteEndOfWord,

        SetTextDirectionDefault,
        SetTextDirectionLeftToRight,
        SetTextDirectionRightToLeft,

        ToggleBold,
        ToggleItalic,
        ToggleUnderline,

        InspectElement,

        InsertParagraphSeparator,
        InsertLineSeparator,

        SelectAll,
        ReloadAndBypassCache,

        PasteAndMatchStyle,
        RemoveFormat,

        ToggleStrikethrough,
        ToggleSubscript,
        ToggleSuperscript,
        InsertUnorderedList,
        InsertOrderedList,
        Indent,
        Outdent,

        AlignCenter,
        AlignJustified,
        AlignLeft,
        AlignRight,

        StopScheduledPageRefresh,

        CopyImageUrlToClipboard,

        WebActionCount
    };

    enum FindFlag {
        FindBackward = 1,
        FindCaseSensitively = 2,
        FindWrapsAroundDocument = 4,
        HighlightAllOccurrences = 8
    };
    Q_DECLARE_FLAGS(FindFlags, FindFlag)

    enum LinkDelegationPolicy {
        DontDelegateLinks,
        DelegateExternalLinks,
        DelegateAllLinks
    };

    enum WebWindowType {
        WebBrowserWindow,
        WebModalDialog
    };

    enum PermissionPolicy {
        PermissionUnknown,
        PermissionGrantedByUser,
        PermissionDeniedByUser
    };

    enum Feature {
        Notifications,
        Geolocation
    };

    class QWEBKIT_EXPORT ViewportAttributes {
    public:
        ViewportAttributes();
        ViewportAttributes(const QWebPage::ViewportAttributes& other);

        ~ViewportAttributes();

        QWebPage::ViewportAttributes& operator=(const QWebPage::ViewportAttributes& other);

        inline qreal initialScaleFactor() const { return m_initialScaleFactor; }
        inline qreal minimumScaleFactor() const { return m_minimumScaleFactor; }
        inline qreal maximumScaleFactor() const { return m_maximumScaleFactor; }
        inline qreal devicePixelRatio() const { return m_devicePixelRatio; }
        inline bool isUserScalable() const { return m_isUserScalable; }
        inline bool isValid() const { return m_isValid; }
        inline QSize size() const { return m_size; }

    private:
        QSharedDataPointer<QtViewportAttributesPrivate> d;
        qreal m_initialScaleFactor;
        qreal m_minimumScaleFactor;
        qreal m_maximumScaleFactor;
        qreal m_devicePixelRatio;
        bool m_isUserScalable;
        bool m_isValid;
        QSize m_size;

        friend class WebCore::ChromeClientQt;
        friend class QWebPage;
    };

    explicit QWebPage(QObject *parent = 0);
    ~QWebPage();

    QWebFrame *mainFrame() const;
    QWebFrame *currentFrame() const;
    QWebFrame* frameAt(const QPoint& pos) const;

    QWebHistory *history() const;
    QWebSettings *settings() const;

    void setView(QWidget *view);
    QWidget *view() const;

    bool isModified() const;
#ifndef QT_NO_UNDOSTACK
    QUndoStack *undoStack() const;
#endif

    void setNetworkAccessManager(QNetworkAccessManager *manager);
    QNetworkAccessManager *networkAccessManager() const;

    void setPluginFactory(QWebPluginFactory *factory);
    QWebPluginFactory *pluginFactory() const;

    quint64 totalBytes() const;
    quint64 bytesReceived() const;

    bool hasSelection() const;
    QString selectedText() const;
    QString selectedHtml() const;

#ifndef QT_NO_ACTION
    QAction *action(WebAction action) const;
#endif
    virtual void triggerAction(WebAction action, bool checked = false);

    QSize viewportSize() const;
    void setViewportSize(const QSize &size) const;
    ViewportAttributes viewportAttributesForSize(const QSize& availableSize) const;

    QSize preferredContentsSize() const;
    void setPreferredContentsSize(const QSize &size) const;
    void setActualVisibleContentRect(const QRect& rect) const;

    virtual bool event(QEvent*);
    bool focusNextPrevChild(bool next);

    QVariant inputMethodQuery(Qt::InputMethodQuery property) const;

    bool findText(const QString &subString, FindFlags options = 0);

    void setForwardUnsupportedContent(bool forward);
    bool forwardUnsupportedContent() const;

    void setLinkDelegationPolicy(LinkDelegationPolicy policy);
    LinkDelegationPolicy linkDelegationPolicy() const;

    void setPalette(const QPalette &palette);
    QPalette palette() const;

    void setContentEditable(bool editable);
    bool isContentEditable() const;

#ifndef QT_NO_CONTEXTMENU
    bool swallowContextMenuEvent(QContextMenuEvent *event);
#endif
    void updatePositionDependentActions(const QPoint &pos);

    QMenu *createStandardContextMenu();

    void setFeaturePermission(QWebFrame* frame, Feature feature, PermissionPolicy policy);

    QStringList supportedContentTypes() const;
    bool supportsContentType(const QString& mimeType) const;

    enum Extension {
        ChooseMultipleFilesExtension,
        ErrorPageExtension
    };
    class ExtensionOption
    {};
    class ExtensionReturn
    {};

    class ChooseMultipleFilesExtensionOption : public ExtensionOption {
    public:
        QWebFrame *parentFrame;
        QStringList suggestedFileNames;
    };

    class ChooseMultipleFilesExtensionReturn : public ExtensionReturn {
    public:
        QStringList fileNames;
    };

    enum ErrorDomain { QtNetwork, Http, WebKit };
    class ErrorPageExtensionOption : public ExtensionOption {
    public:
        QUrl url;
        QWebFrame* frame;
        ErrorDomain domain;
        int error;
        QString errorString;
    };

    class ErrorPageExtensionReturn : public ExtensionReturn {
    public:
        ErrorPageExtensionReturn() : contentType(QLatin1String("text/html")), encoding(QLatin1String("utf-8")) {};
        QString contentType;
        QString encoding;
        QUrl baseUrl;
        QByteArray content;
    };


    virtual bool extension(Extension extension, const ExtensionOption *option = 0, ExtensionReturn *output = 0);
    virtual bool supportsExtension(Extension extension) const;

    inline QWebPagePrivate* handle() const { return d; }

public Q_SLOTS:
    bool shouldInterruptJavaScript();

Q_SIGNALS:
    void loadStarted();
    void loadProgress(int progress);
    void loadFinished(bool ok);

    void linkHovered(const QString &link, const QString &title, const QString &textContent);
    void statusBarMessage(const QString& text);
    void selectionChanged();
    void frameCreated(QWebFrame *frame);
    void geometryChangeRequested(const QRect& geom);
    void repaintRequested(const QRect& dirtyRect);
    void scrollRequested(int dx, int dy, const QRect& scrollViewRect);
    void windowCloseRequested();
    void printRequested(QWebFrame *frame);
    void linkClicked(const QUrl &url);

    void toolBarVisibilityChangeRequested(bool visible);
    void statusBarVisibilityChangeRequested(bool visible);
    void menuBarVisibilityChangeRequested(bool visible);

    void unsupportedContent(QNetworkReply *reply);
    void downloadRequested(const QNetworkRequest &request);

    void microFocusChanged();
    void contentsChanged();
    void databaseQuotaExceeded(QWebFrame* frame, QString databaseName);
    void applicationCacheQuotaExceeded(QWebSecurityOrigin* origin, quint64 defaultOriginQuota);

    void saveFrameStateRequested(QWebFrame* frame, QWebHistoryItem* item);
    void restoreFrameStateRequested(QWebFrame* frame);

    void viewportChangeRequested();

    void featurePermissionRequested(QWebFrame* frame, QWebPage::Feature feature);
    void featurePermissionRequestCanceled(QWebFrame* frame, QWebPage::Feature feature);

protected:
    virtual QWebPage *createWindow(WebWindowType type);
    virtual QObject *createPlugin(const QString &classid, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues);

    virtual bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type);
    virtual QString chooseFile(QWebFrame *originatingFrame, const QString& oldFile);
    virtual void javaScriptAlert(QWebFrame *originatingFrame, const QString& msg);
    virtual bool javaScriptConfirm(QWebFrame *originatingFrame, const QString& msg);
    virtual bool javaScriptPrompt(QWebFrame *originatingFrame, const QString& msg, const QString& defaultValue, QString* result);
    virtual void javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID);
    virtual void javaScriptError(const QString& message, int lineNumber, const QString& sourceID, const QString& stack);

    virtual QString userAgentForUrl(const QUrl& url) const;

private:
    Q_PRIVATE_SLOT(d, void _q_onLoadProgressChanged(int))
#ifndef QT_NO_ACTION
    Q_PRIVATE_SLOT(d, void _q_webActionTriggered(bool checked))
#endif
    Q_PRIVATE_SLOT(d, void _q_cleanupLeakMessages())

    QWebPagePrivate *d;

    friend class QWebFrame;
    friend class QWebPagePrivate;
    friend class QWebView;
    friend class QWebViewPrivate;
    friend class QGraphicsWebView;
    friend class QGraphicsWebViewPrivate;
    friend class QWebInspector;
    friend class WebCore::ChromeClientQt;
    friend class WebCore::EditorClientQt;
    friend class WebCore::FrameLoaderClientQt;
    friend class WebCore::InspectorClientQt;
    friend class WebCore::InspectorServerRequestHandlerQt;
    friend class WebCore::InspectorFrontendClientQt;
    friend class WebCore::NotificationPresenterClientQt;
    friend class WebCore::GeolocationPermissionClientQt;
    friend class WebCore::ResourceHandle;
    friend class WebCore::QNetworkReplyHandler;
    friend class DumpRenderTreeSupportQt;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QWebPage::FindFlags)

#endif
