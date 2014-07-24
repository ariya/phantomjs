/*
 * Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
#ifndef QWebPageAdapter_h
#define QWebPageAdapter_h

#include "PlatformExportMacros.h"
#include "QWebPageClient.h"
#include "ViewportArguments.h"
#include "qwebhistory.h"

#include <qbasictimer.h>
#include <qevent.h>
#include <qnetworkrequest.h>
#include <qrect.h>
#include <qscopedpointer.h>
#include <qsharedpointer.h>
#include <qstring.h>
#include <qurl.h>
#include <wtf/ExportMacros.h>

QT_BEGIN_NAMESPACE
class QBitArray;
class QKeyEvent;
class QMimeData;
class QMouseEvent;
class QNetworkAccessManager;
class QWheelEvent;
class QInputMethodEvent;
QT_END_NAMESPACE

namespace WebCore {
class ChromeClientQt;
class DeviceOrientationClient;
class DeviceMotionClient;
class GeolocationClientQt;
class Page;
class UndoStep;
}

class QtPluginWidgetAdapter;
class QWebFrameAdapter;
class QWebHistoryItem;
class QWebHitTestResultPrivate;
class QWebPageClient;
class QWebPluginFactory;
class QWebSecurityOrigin;
class QWebSelectMethod;
class QWebSettings;
class QWebFullScreenVideoHandler;
class UndoStepQt;

class WEBKIT_EXPORTDATA QWebPageAdapter {
public:

#define FOR_EACH_MAPPED_MENU_ACTION(F, SEPARATOR) \
    F(OpenLink, WebCore::ContextMenuItemTagOpenLink) SEPARATOR \
    F(OpenLinkInNewWindow, WebCore::ContextMenuItemTagOpenLinkInNewWindow) SEPARATOR \
    F(OpenLinkInThisWindow, WebCore::ContextMenuItemTagOpenLinkInThisWindow) SEPARATOR \
    F(DownloadLinkToDisk, WebCore::ContextMenuItemTagDownloadLinkToDisk) SEPARATOR \
    F(CopyLinkToClipboard, WebCore::ContextMenuItemTagCopyLinkToClipboard) SEPARATOR \
    F(OpenImageInNewWindow, WebCore::ContextMenuItemTagOpenImageInNewWindow) SEPARATOR \
    F(DownloadImageToDisk, WebCore::ContextMenuItemTagDownloadImageToDisk) SEPARATOR \
    F(CopyImageToClipboard, WebCore::ContextMenuItemTagCopyImageToClipboard) SEPARATOR \
    F(CopyImageUrlToClipboard, WebCore::ContextMenuItemTagCopyImageUrlToClipboard) SEPARATOR \
    F(OpenFrameInNewWindow, WebCore::ContextMenuItemTagOpenFrameInNewWindow) SEPARATOR \
    F(Copy, WebCore::ContextMenuItemTagCopy) SEPARATOR \
    F(Back, WebCore::ContextMenuItemTagGoBack) SEPARATOR \
    F(Forward, WebCore::ContextMenuItemTagGoForward) SEPARATOR \
    F(Stop, WebCore::ContextMenuItemTagStop) SEPARATOR \
    F(Reload, WebCore::ContextMenuItemTagReload) SEPARATOR \
    F(Cut, WebCore::ContextMenuItemTagCut) SEPARATOR \
    F(Paste, WebCore::ContextMenuItemTagPaste) SEPARATOR \
    F(SetTextDirectionDefault, WebCore::ContextMenuItemTagDefaultDirection) SEPARATOR \
    F(SetTextDirectionLeftToRight, WebCore::ContextMenuItemTagLeftToRight) SEPARATOR \
    F(SetTextDirectionRightToLeft, WebCore::ContextMenuItemTagRightToLeft) SEPARATOR \
    F(ToggleBold, WebCore::ContextMenuItemTagBold) SEPARATOR \
    F(ToggleItalic, WebCore::ContextMenuItemTagItalic) SEPARATOR \
    F(ToggleUnderline, WebCore::ContextMenuItemTagUnderline) SEPARATOR \
    F(SelectAll, WebCore::ContextMenuItemTagSelectAll) SEPARATOR \
    F(DownloadMediaToDisk, WebCore::ContextMenuItemTagDownloadMediaToDisk) SEPARATOR \
    F(CopyMediaUrlToClipboard, WebCore::ContextMenuItemTagCopyMediaLinkToClipboard) SEPARATOR \
    F(ToggleMediaControls, WebCore::ContextMenuItemTagToggleMediaControls) SEPARATOR \
    F(ToggleMediaLoop, WebCore::ContextMenuItemTagToggleMediaLoop) SEPARATOR \
    F(ToggleMediaPlayPause, WebCore::ContextMenuItemTagMediaPlayPause) SEPARATOR \
    F(ToggleMediaMute, WebCore::ContextMenuItemTagMediaMute) SEPARATOR \
    F(ToggleVideoFullscreen, WebCore::ContextMenuItemTagToggleVideoFullscreen)
#define COMMA_SEPARATOR ,
#define SEMICOLON_SEPARATOR ;
#define DEFINE_ACTION(Name, Value) \
    Name

    enum MenuAction {
        NoAction = - 1,
        FOR_EACH_MAPPED_MENU_ACTION(DEFINE_ACTION, COMMA_SEPARATOR)
#if ENABLE(INSPECTOR)
        , InspectElement
#endif
        , ActionCount
    };

    // Duplicated from qwebpage.h
    enum FindFlag {
        FindBackward = 1,
        FindCaseSensitively = 2,
        FindWrapsAroundDocument = 4,
        HighlightAllOccurrences = 8,
        FindAtWordBeginningsOnly = 16,
        TreatMedialCapitalAsWordBeginning = 32,
        FindBeginsInSelection = 64
    };

    // valid values matching those from ScrollTypes.h
    enum ScrollDirection {
        InvalidScrollDirection = -1,
        ScrollUp,
        ScrollDown,
        ScrollLeft,
        ScrollRight
    };
    // same here
    enum ScrollGranularity {
        InvalidScrollGranularity = -1,
        ScrollByLine,
        ScrollByPage,
        ScrollByDocument
    };

    // Must match with values of QWebPage::VisibilityState enum.
    enum VisibilityState {
        VisibilityStateVisible,
        VisibilityStateHidden,
        VisibilityStatePrerender,
        VisibilityStateUnloaded
    };

    QWebPageAdapter();
    virtual ~QWebPageAdapter();

    // Called manually from ~QWebPage destructor to ensure that
    // the QWebPageAdapter and the QWebPagePrivate are intact when
    // various destruction callbacks from WebCore::Page::~Page() hit us.
    void deletePage();
    // For similar reasons, we don't want to create the WebCore Page before
    // we properly initialized the style factory callbacks.
    void initializeWebCorePage();

    virtual void show() = 0;
    virtual void setFocus() = 0;
    virtual void unfocus() = 0;
    virtual void setWindowRect(const QRect&) = 0;
    virtual QSize viewportSize() const = 0;
    virtual QWebPageAdapter* createWindow(bool /*dialog*/) = 0;
    virtual QObject* handle() = 0;
    virtual void javaScriptError(const QString& message, int lineNumber, const QString& sourceID, const QString& stack) = 0;
    virtual void javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID) = 0;
    virtual void javaScriptAlert(QWebFrameAdapter*, const QString& msg) = 0;
    virtual bool javaScriptConfirm(QWebFrameAdapter*, const QString& msg) = 0;
    virtual bool javaScriptPrompt(QWebFrameAdapter*, const QString& msg, const QString& defaultValue, QString* result) = 0;
    virtual bool shouldInterruptJavaScript() = 0;
    virtual void printRequested(QWebFrameAdapter*) = 0;
    virtual void databaseQuotaExceeded(QWebFrameAdapter*, const QString& databaseName) = 0;
    virtual void applicationCacheQuotaExceeded(QWebSecurityOrigin*, quint64 defaultOriginQuota, quint64 totalSpaceNeeded) = 0;
    virtual void setToolTip(const QString&) = 0;
    virtual QStringList chooseFiles(QWebFrameAdapter*, bool allowMultiple, const QStringList& suggestedFileNames) = 0;
    virtual QColor colorSelectionRequested(const QColor& selectedColor) = 0;
    virtual QWebSelectMethod* createSelectPopup() = 0;
    virtual QRect viewRectRelativeToWindow() = 0;

#if USE(QT_MULTIMEDIA)
    virtual QWebFullScreenVideoHandler* createFullScreenVideoHandler() = 0;
#endif
    virtual void geolocationPermissionRequested(QWebFrameAdapter*) = 0;
    virtual void geolocationPermissionRequestCancelled(QWebFrameAdapter*) = 0;
    virtual void notificationsPermissionRequested(QWebFrameAdapter*) = 0;
    virtual void notificationsPermissionRequestCancelled(QWebFrameAdapter*) = 0;

    virtual void respondToChangedContents() = 0;
    virtual void respondToChangedSelection() = 0;
    virtual void microFocusChanged() = 0;
    virtual void triggerCopyAction() = 0;
    virtual void triggerActionForKeyEvent(QKeyEvent*) = 0;
    virtual void clearUndoStack() = 0;
    virtual bool canUndo() const = 0;
    virtual bool canRedo() const = 0;
    virtual void undo() = 0;
    virtual void redo() = 0;
    virtual const char* editorCommandForKeyEvent(QKeyEvent*) = 0;
    virtual void createUndoStep(QSharedPointer<UndoStepQt>) = 0;

    virtual void updateNavigationActions() = 0;

    virtual QWebFrameAdapter* mainFrameAdapter() = 0;

    virtual QObject* inspectorHandle() = 0;
    virtual void setInspectorFrontend(QObject*) = 0;
    virtual void setInspectorWindowTitle(const QString&) = 0;
    virtual void createWebInspector(QObject** inspectorView, QWebPageAdapter** inspectorPage) = 0;
    virtual QStringList menuActionsAsText() = 0;
    virtual void emitViewportChangeRequested() = 0;
    virtual bool acceptNavigationRequest(QWebFrameAdapter*, const QNetworkRequest&, int type) = 0;
    virtual void emitRestoreFrameStateRequested(QWebFrameAdapter *) = 0;
    virtual void emitSaveFrameStateRequested(QWebFrameAdapter *, QWebHistoryItem*) = 0;
    virtual void emitDownloadRequested(const QNetworkRequest&) = 0;
    virtual void emitFrameCreated(QWebFrameAdapter*) = 0;
    virtual QString userAgentForUrl(const QUrl&) const = 0;
    virtual bool supportsErrorPageExtension() const = 0;
    struct ErrorPageOption {
        QUrl url;
        QWebFrameAdapter* frame;
        QString domain;
        int error;
        QString errorString;
    };
    struct ErrorPageReturn {
        QString contentType;
        QString encoding;
        QUrl baseUrl;
        QByteArray content;
    };
    virtual bool errorPageExtension(ErrorPageOption*, ErrorPageReturn*) = 0;
    virtual QtPluginWidgetAdapter* createPlugin(const QString&, const QUrl&, const QStringList&, const QStringList&) = 0;
    virtual QtPluginWidgetAdapter* adapterForWidget(QObject*) const = 0;
    virtual bool requestSoftwareInputPanel() const = 0;
    struct MenuItemDescription {
        MenuItemDescription()
            : type(NoType)
            , action(NoAction)
            , traits(None)
        { }
        enum Type {
            NoType,
            Action,
            Separator,
            SubMenu
        } type;
        MenuAction action;
        enum Trait {
            None = 0,
            Enabled = 1,
            Checkable = 2,
            Checked = 4
        };
        Q_DECLARE_FLAGS(Traits, Trait);
        Traits traits;
        QList<MenuItemDescription> subMenu;
        QString title;
    };
    virtual void createAndSetCurrentContextMenu(const QList<MenuItemDescription>&, QBitArray*) = 0;
    virtual bool handleScrollbarContextMenuEvent(QContextMenuEvent*, bool, ScrollDirection*, ScrollGranularity*) = 0;

    void setVisibilityState(VisibilityState);
    VisibilityState visibilityState() const;

    static QWebPageAdapter* kit(WebCore::Page*);
    WebCore::ViewportArguments viewportArguments() const;
    void registerUndoStep(WTF::PassRefPtr<WebCore::UndoStep>);

    void setNetworkAccessManager(QNetworkAccessManager*);
    QNetworkAccessManager* networkAccessManager();

    bool hasSelection() const;
    QString selectedText() const;
    QString selectedHtml() const;

    bool isContentEditable() const;
    void setContentEditable(bool);

    bool findText(const QString& subString, FindFlag options);

    void adjustPointForClicking(QMouseEvent*);

    void mouseMoveEvent(QMouseEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseDoubleClickEvent(QMouseEvent*);
    void mouseTripleClickEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void handleSoftwareInputPanel(Qt::MouseButton, const QPoint&);
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent*, int wheelScrollLines);
#endif
#ifndef QT_NO_DRAGANDDROP
    Qt::DropAction dragEntered(const QMimeData*, const QPoint&, Qt::DropActions);
    void dragLeaveEvent();
    Qt::DropAction dragUpdated(const QMimeData*, const QPoint&, Qt::DropActions);
    bool performDrag(const QMimeData*, const QPoint&, Qt::DropActions);
#endif
    void inputMethodEvent(QInputMethodEvent*);
    QVariant inputMethodQuery(Qt::InputMethodQuery property) const;
    void dynamicPropertyChangeEvent(QObject*, QDynamicPropertyChangeEvent*);
    bool handleKeyEvent(QKeyEvent*);
    bool handleScrolling(QKeyEvent*);
    void focusInEvent(QFocusEvent*);
    void focusOutEvent(QFocusEvent*);
    bool handleShortcutOverrideEvent(QKeyEvent*);
    // Returns whether the default action was cancelled in the JS event handler
    bool touchEvent(QTouchEvent*);
    bool swallowContextMenuEvent(QContextMenuEvent *, QWebFrameAdapter*);

    QWebHitTestResultPrivate* updatePositionDependentMenuActions(const QPoint&, QBitArray*);
    void updateActionInternal(MenuAction, const char* commandName, bool* enabled, bool* checked);
    void triggerAction(MenuAction, QWebHitTestResultPrivate*, const char* commandName, bool endToEndReload);
    QString contextMenuItemTagForAction(MenuAction, bool* checkable) const;

    QStringList supportedContentTypes() const;
#if ENABLE(GEOLOCATION) && HAVE(QTPOSITIONING)
    void setGeolocationEnabledForFrame(QWebFrameAdapter*, bool);
#endif
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    void setNotificationsAllowedForFrame(QWebFrameAdapter*, bool allowed);
    void addNotificationPresenterClient();
#ifndef QT_NO_SYSTEMTRAYICON
    bool hasSystemTrayIcon() const;
    void setSystemTrayIcon(QObject*);
#endif // QT_NO_SYSTEMTRAYICON
#endif // ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)

    // Called from QWebPage as private slots.
    void _q_cleanupLeakMessages();
    void _q_onLoadProgressChanged(int);

    bool supportsContentType(const QString& mimeType) const;

    void didShowInspector();
    void didCloseInspector();

    static QString defaultUserAgentString();
    static bool treatSchemeAsLocal(const QString&);

    QObject* currentFrame() const;
    bool hasFocusedNode() const;
    struct ViewportAttributes {
        qreal initialScaleFactor;
        qreal minimumScaleFactor;
        qreal maximumScaleFactor;
        qreal devicePixelRatio;
        bool isUserScalable;
        QSizeF size;
    };

    ViewportAttributes viewportAttributesForSize(const QSize& availableSize, const QSize& deviceSize) const;
    void setDevicePixelRatio(float devicePixelRatio);

    QWebSettings *settings;

    WebCore::Page *page;
    QScopedPointer<QWebPageClient> client;

    QWebPluginFactory *pluginFactory;

    bool forwardUnsupportedContent;
    bool insideOpenCall;
    QPoint tripleClick;
    QBasicTimer tripleClickTimer;

    bool clickCausedFocus;
    bool m_useNativeVirtualKeyAsDOMKey;
    quint64 m_totalBytes;
    quint64 m_bytesReceived;
    QWebHistory history;

private:
    QNetworkAccessManager *networkManager;
    WebCore::DeviceOrientationClient* m_deviceOrientationClient;
    WebCore::DeviceMotionClient* m_deviceMotionClient;

public:
    static bool drtRun;

    friend class WebCore::ChromeClientQt;
    friend class WebCore::GeolocationClientQt;
};

#endif // QWebPageAdapter_h
