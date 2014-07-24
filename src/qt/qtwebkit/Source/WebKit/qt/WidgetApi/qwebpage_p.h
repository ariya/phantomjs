/*
    Copyright (C) 2008, 2009 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2008 Holger Hans Peter Freyther

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

#ifndef qwebpage_p_h
#define qwebpage_p_h

#include "QWebPageAdapter.h"

#include "qwebframe.h"
#include "qwebpage.h"

#include <QPointer>
#include <qevent.h>
#include <qgesture.h>
#include <qgraphicssceneevent.h>
#include <qgraphicswidget.h>
#include <qnetworkproxy.h>


namespace WebCore {
class ContextMenuClientQt;
class ContextMenuItem;
class ContextMenu;
class Document;
class EditorClientQt;
class Element;
class IntRect;
class Node;
class NodeList;
class Frame;
}

QT_BEGIN_NAMESPACE
class QBitArray;
class QMenu;
class QScreen;
class QUndoStack;
class QWindow;
QT_END_NAMESPACE

class QtPluginWidgetAdapter;
class QWebInspector;
class QWebFrameAdapter;
class UndoStepQt;

class QtViewportAttributesPrivate : public QSharedData {
public:
    QtViewportAttributesPrivate(QWebPage::ViewportAttributes* qq)
        : q(qq)
    { }

    QWebPage::ViewportAttributes* q;
};

class QWebPagePrivate : public QWebPageAdapter {
public:
    QWebPagePrivate(QWebPage*);
    ~QWebPagePrivate();

    static WebCore::Page* core(const QWebPage*);

    // Adapter implementation
    virtual void show() OVERRIDE;
    virtual void setFocus() OVERRIDE;
    virtual void unfocus() OVERRIDE;
    virtual void setWindowRect(const QRect &) OVERRIDE;
    virtual QSize viewportSize() const OVERRIDE;
    virtual QWebPageAdapter* createWindow(bool /*dialog*/) OVERRIDE;
    virtual QObject* handle() OVERRIDE { return q; }
    virtual void javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID) OVERRIDE;
    virtual void javaScriptAlert(QWebFrameAdapter*, const QString& msg) OVERRIDE;
    virtual bool javaScriptConfirm(QWebFrameAdapter*, const QString& msg) OVERRIDE;
    virtual bool javaScriptPrompt(QWebFrameAdapter*, const QString& msg, const QString& defaultValue, QString* result) OVERRIDE;
    virtual void javaScriptError(const QString& message, int lineNumber, const QString& sourceID, const QString& stack) OVERRIDE;

    virtual bool shouldInterruptJavaScript() OVERRIDE;
    virtual void printRequested(QWebFrameAdapter*) OVERRIDE;
    virtual void databaseQuotaExceeded(QWebFrameAdapter*, const QString& databaseName) OVERRIDE;
    virtual void applicationCacheQuotaExceeded(QWebSecurityOrigin*, quint64 defaultOriginQuota, quint64 totalSpaceNeeded) OVERRIDE;
    virtual void setToolTip(const QString&) OVERRIDE;
#if USE(QT_MULTIMEDIA)
    virtual QWebFullScreenVideoHandler* createFullScreenVideoHandler() OVERRIDE;
#endif
    virtual QWebFrameAdapter* mainFrameAdapter() OVERRIDE;
    virtual QStringList chooseFiles(QWebFrameAdapter*, bool allowMultiple, const QStringList& suggestedFileNames) OVERRIDE;
    virtual QColor colorSelectionRequested(const QColor& selectedColor) OVERRIDE;
    virtual QWebSelectMethod* createSelectPopup() OVERRIDE;
    virtual QRect viewRectRelativeToWindow() OVERRIDE;
    virtual void geolocationPermissionRequested(QWebFrameAdapter*) OVERRIDE;
    virtual void geolocationPermissionRequestCancelled(QWebFrameAdapter*) OVERRIDE;
    virtual void notificationsPermissionRequested(QWebFrameAdapter*) OVERRIDE;
    virtual void notificationsPermissionRequestCancelled(QWebFrameAdapter*) OVERRIDE;

    virtual void respondToChangedContents() OVERRIDE;
    virtual void respondToChangedSelection() OVERRIDE;
    virtual void microFocusChanged() OVERRIDE;
    virtual void triggerCopyAction() OVERRIDE;
    virtual void triggerActionForKeyEvent(QKeyEvent*) OVERRIDE;
    virtual void clearUndoStack() OVERRIDE;
    virtual bool canUndo() const OVERRIDE;
    virtual bool canRedo() const OVERRIDE;
    virtual void undo() OVERRIDE;
    virtual void redo() OVERRIDE;
    virtual void createUndoStep(QSharedPointer<UndoStepQt>) OVERRIDE;
    virtual const char* editorCommandForKeyEvent(QKeyEvent*) OVERRIDE;

    void updateNavigationActions() OVERRIDE;

    virtual QObject* inspectorHandle() OVERRIDE;
    virtual void setInspectorFrontend(QObject*) OVERRIDE;
    virtual void setInspectorWindowTitle(const QString&) OVERRIDE;
    virtual void createWebInspector(QObject** inspectorView, QWebPageAdapter** inspectorPage) OVERRIDE;
    virtual QStringList menuActionsAsText() OVERRIDE;
    virtual void emitViewportChangeRequested() OVERRIDE;
    virtual bool acceptNavigationRequest(QWebFrameAdapter*, const QNetworkRequest&, int type) OVERRIDE;
    virtual void emitRestoreFrameStateRequested(QWebFrameAdapter*) OVERRIDE;
    virtual void emitSaveFrameStateRequested(QWebFrameAdapter*, QWebHistoryItem*) OVERRIDE;
    virtual void emitDownloadRequested(const QNetworkRequest&) OVERRIDE;
    virtual void emitFrameCreated(QWebFrameAdapter*) OVERRIDE;
    virtual QString userAgentForUrl(const QUrl &url) const OVERRIDE { return q->userAgentForUrl(url); }
    virtual bool supportsErrorPageExtension() const OVERRIDE { return q->supportsExtension(QWebPage::ErrorPageExtension); }
    virtual bool errorPageExtension(ErrorPageOption *, ErrorPageReturn *) OVERRIDE;
    virtual QtPluginWidgetAdapter* createPlugin(const QString &, const QUrl &, const QStringList &, const QStringList &) OVERRIDE;
    virtual QtPluginWidgetAdapter* adapterForWidget(QObject *) const OVERRIDE;
    virtual bool requestSoftwareInputPanel() const OVERRIDE;
    virtual void createAndSetCurrentContextMenu(const QList<MenuItemDescription>&, QBitArray*) OVERRIDE;
    virtual bool handleScrollbarContextMenuEvent(QContextMenuEvent*, bool, ScrollDirection*, ScrollGranularity*) OVERRIDE;


    void createMainFrame();

    void _q_webActionTriggered(bool checked);
    void updateAction(QWebPage::WebAction);
    void updateEditorActions();

    void timerEvent(QTimerEvent*);

#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(const QPoint& globalPos);
#endif
    void keyPressEvent(QKeyEvent*);
    void keyReleaseEvent(QKeyEvent*);

    template<class T> void dragEnterEvent(T*);
    template<class T> void dragMoveEvent(T*);
    template<class T> void dropEvent(T*);

    void shortcutOverrideEvent(QKeyEvent*);
    void leaveEvent(QEvent*);

    bool gestureEvent(QGestureEvent*);

    void updateWindow();
    void _q_updateScreen(QScreen*);

    void setInspector(QWebInspector*);
    QWebInspector* getOrCreateInspector();

#ifndef QT_NO_SHORTCUT
    static QWebPage::WebAction editorActionForKeyEvent(QKeyEvent*);
#endif
    static const char* editorCommandForWebActions(QWebPage::WebAction);

    QWebPage *q;
    QPointer<QWebFrame> mainFrame;

#ifndef QT_NO_UNDOSTACK
    QUndoStack *undoStack;
#endif

    QPointer<QWidget> view;

    QWebPage::LinkDelegationPolicy linkPolicy;

    QSize m_viewportSize;
    QSize fixedLayoutSize;

    QWebHitTestResult hitTestResult;
#ifndef QT_NO_CONTEXTMENU
    QPointer<QMenu> currentContextMenu;
#endif
    QPalette palette;
    bool useFixedLayout;

    QAction *actions[QWebPage::WebActionCount];

    QWindow* window;
    QWidget* inspectorFrontend;
    QWebInspector* inspector;
    bool inspectorIsInternalOnly; // True if created through the Inspect context menu action
    Qt::DropAction m_lastDropAction;
};

#endif
