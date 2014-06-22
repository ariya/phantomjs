/*
    Copyright (C) 2008, 2009, 2012 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2007 Staikos Computing Services Inc.
    Copyright (C) 2007 Apple Inc.

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

#include "config.h"
#include "qwebpage.h"

#include "DefaultFullScreenVideoHandler.h"
#include "InitWebKitQt.h"
#include "InspectorClientQt.h"
#include "InspectorClientWebPage.h"
#include "InspectorServerQt.h"
#include "PageClientQt.h"
#include "QGraphicsWidgetPluginImpl.h"
#include "QWebUndoCommand.h"
#include "QWidgetPluginImpl.h"
#include "QtFallbackWebPopup.h"
#include "QtPlatformPlugin.h"
#include "UndoStepQt.h"
#include "WebEventConversion.h"

#include "qwebframe.h"
#include "qwebframe_p.h"
#include "qwebhistory.h"
#include "qwebinspector.h"
#include "qwebinspector_p.h"
#include "qwebkitplatformplugin.h"
#include "qwebpage_p.h"
#include "qwebsettings.h"
#include "qwebview.h"

#include <QAction>
#include <QApplication>
#include <QBasicTimer>
#include <QBitArray>
#include <QClipboard>
#include <QColorDialog>
#include <QDebug>
#include <QDesktopWidget>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QGestureEvent>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QPainter>
#include <QScreen>
#include <QSslSocket>
#include <QStyle>
#include <QSysInfo>
#if USE(QT_MOBILITY_SYSTEMINFO)
#include <qsysteminfo.h>
#endif
#include <QSystemTrayIcon>
#include <QTextCharFormat>
#include <QToolTip>
#include <QTouchEvent>
#include <QUndoStack>
#include <QUrl>
#include <QWindow>
#if defined(Q_WS_X11)
#include <QX11Info>
#endif

using namespace WebCore;

// Lookup table mapping QWebPage::WebActions to the associated Editor commands
static const char* editorCommandWebActions[] =
{
    0, // OpenLink,

    0, // OpenLinkInNewWindow,
    0, // OpenFrameInNewWindow,

    0, // DownloadLinkToDisk,
    0, // CopyLinkToClipboard,

    0, // OpenImageInNewWindow,
    0, // DownloadImageToDisk,
    0, // CopyImageToClipboard,

    0, // Back,
    0, // Forward,
    0, // Stop,
    0, // Reload,

    "Cut", // Cut,
    "Copy", // Copy,
    "Paste", // Paste,

    "Undo", // Undo,
    "Redo", // Redo,
    "MoveForward", // MoveToNextChar,
    "MoveBackward", // MoveToPreviousChar,
    "MoveWordForward", // MoveToNextWord,
    "MoveWordBackward", // MoveToPreviousWord,
    "MoveDown", // MoveToNextLine,
    "MoveUp", // MoveToPreviousLine,
    "MoveToBeginningOfLine", // MoveToStartOfLine,
    "MoveToEndOfLine", // MoveToEndOfLine,
    "MoveToBeginningOfParagraph", // MoveToStartOfBlock,
    "MoveToEndOfParagraph", // MoveToEndOfBlock,
    "MoveToBeginningOfDocument", // MoveToStartOfDocument,
    "MoveToEndOfDocument", // MoveToEndOfDocument,
    "MoveForwardAndModifySelection", // SelectNextChar,
    "MoveBackwardAndModifySelection", // SelectPreviousChar,
    "MoveWordForwardAndModifySelection", // SelectNextWord,
    "MoveWordBackwardAndModifySelection", // SelectPreviousWord,
    "MoveDownAndModifySelection", // SelectNextLine,
    "MoveUpAndModifySelection", // SelectPreviousLine,
    "MoveToBeginningOfLineAndModifySelection", // SelectStartOfLine,
    "MoveToEndOfLineAndModifySelection", // SelectEndOfLine,
    "MoveToBeginningOfParagraphAndModifySelection", // SelectStartOfBlock,
    "MoveToEndOfParagraphAndModifySelection", // SelectEndOfBlock,
    "MoveToBeginningOfDocumentAndModifySelection", // SelectStartOfDocument,
    "MoveToEndOfDocumentAndModifySelection", // SelectEndOfDocument,
    "DeleteWordBackward", // DeleteStartOfWord,
    "DeleteWordForward", // DeleteEndOfWord,

    0, // SetTextDirectionDefault,
    0, // SetTextDirectionLeftToRight,
    0, // SetTextDirectionRightToLeft,

    "ToggleBold", // ToggleBold,
    "ToggleItalic", // ToggleItalic,
    "ToggleUnderline", // ToggleUnderline,

    0, // InspectElement,

    "InsertNewline", // InsertParagraphSeparator
    "InsertLineBreak", // InsertLineSeparator

    "SelectAll", // SelectAll
    0, // ReloadAndBypassCache,

    "PasteAndMatchStyle", // PasteAndMatchStyle
    "RemoveFormat", // RemoveFormat
    "Strikethrough", // ToggleStrikethrough,
    "Subscript", // ToggleSubscript
    "Superscript", // ToggleSuperscript
    "InsertUnorderedList", // InsertUnorderedList
    "InsertOrderedList", // InsertOrderedList
    "Indent", // Indent
    "Outdent", // Outdent,

    "AlignCenter", // AlignCenter,
    "AlignJustified", // AlignJustified,
    "AlignLeft", // AlignLeft,
    "AlignRight", // AlignRight,

    0, // StopScheduledPageRefresh,

    0, // CopyImageUrlToClipboard,

    0, // OpenLinkInThisWindow,

    0 // WebActionCount
};

// Lookup the appropriate editor command to use for WebAction \a action
const char* QWebPagePrivate::editorCommandForWebActions(QWebPage::WebAction action)
{
    if ((action > QWebPage::NoWebAction) && (action < int(sizeof(editorCommandWebActions) / sizeof(const char*))))
        return editorCommandWebActions[action];
    return 0;
}

QWebPagePrivate::QWebPagePrivate(QWebPage *qq)
    : q(qq)
#ifndef QT_NO_UNDOSTACK
    , undoStack(0)
#endif
    , linkPolicy(QWebPage::DontDelegateLinks)
    , m_viewportSize(QSize(0, 0))
    , useFixedLayout(false)
    , window(0)
    , inspectorFrontend(0)
    , inspector(0)
    , inspectorIsInternalOnly(false)
    , m_lastDropAction(Qt::IgnoreAction)
{
    WebKit::initializeWebKitWidgets();
    initializeWebCorePage();
    memset(actions, 0, sizeof(actions));

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    addNotificationPresenterClient();
#ifndef QT_NO_SYSTEMTRAYICON
    if (!hasSystemTrayIcon())
        setSystemTrayIcon(new QSystemTrayIcon);
#endif // QT_NO_SYSTEMTRAYICON
#endif // ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
}

QWebPagePrivate::~QWebPagePrivate()
{
#ifndef QT_NO_CONTEXTMENU
    delete currentContextMenu.data();
#endif
#ifndef QT_NO_UNDOSTACK
    delete undoStack;
    undoStack = 0;
#endif
    
    if (inspector) {
        // If the inspector is ours, delete it, otherwise just detach from it.
        if (inspectorIsInternalOnly)
            delete inspector;
        else
            inspector->setPage(0);
    }
    // Explicitly destruct the WebCore page at this point when the
    // QWebPagePrivate / QWebPageAdapater vtables are still intact,
    // in order for various destruction callbacks out of WebCore to
    // work.
    deletePage();
}

void QWebPagePrivate::show()
{
    if (!view)
        return;
    view->window()->show();
}

void QWebPagePrivate::setFocus()
{
    if (!view)
        return;
    view->setFocus();
}

void QWebPagePrivate::unfocus()
{
    if (!view)
        return;
    view->clearFocus();
}

void QWebPagePrivate::setWindowRect(const QRect &rect)
{
    emit q->geometryChangeRequested(rect);
}

QSize QWebPagePrivate::viewportSize() const
{
    return q->viewportSize();
}

QWebPageAdapter *QWebPagePrivate::createWindow(bool dialog)
{
    QWebPage *newPage = q->createWindow(dialog ? QWebPage::WebModalDialog : QWebPage::WebBrowserWindow);
    if (!newPage)
        return 0;
    // Make sure the main frame exists, as WebCore expects it when returning from this ChromeClient::createWindow()
    newPage->d->createMainFrame();
    return newPage->d;
}

void QWebPagePrivate::javaScriptError(const QString& message, int lineNumber, const QString& sourceID, const QString& stack)
{
    q->javaScriptError(message, lineNumber, sourceID, stack);
}

void QWebPagePrivate::javaScriptConsoleMessage(const QString &message, int lineNumber, const QString &sourceID)
{
    q->javaScriptConsoleMessage(message, lineNumber, sourceID);
}

void QWebPagePrivate::javaScriptAlert(QWebFrameAdapter* frame, const QString& msg)
{
    q->javaScriptAlert(QWebFramePrivate::kit(frame), msg);
}

bool QWebPagePrivate::javaScriptConfirm(QWebFrameAdapter* frame, const QString& msg)
{
    return q->javaScriptConfirm(QWebFramePrivate::kit(frame), msg);
}

bool QWebPagePrivate::javaScriptPrompt(QWebFrameAdapter *frame, const QString &msg, const QString &defaultValue, QString *result)
{
    return q->javaScriptPrompt(QWebFramePrivate::kit(frame), msg, defaultValue, result);
}

bool QWebPagePrivate::shouldInterruptJavaScript()
{
    return q->shouldInterruptJavaScript();
}

void QWebPagePrivate::printRequested(QWebFrameAdapter *frame)
{
    emit q->printRequested(QWebFramePrivate::kit(frame));
}

void QWebPagePrivate::databaseQuotaExceeded(QWebFrameAdapter* frame, const QString& databaseName)
{
    emit q->databaseQuotaExceeded(QWebFramePrivate::kit(frame), databaseName);
}

void QWebPagePrivate::applicationCacheQuotaExceeded(QWebSecurityOrigin *origin, quint64 defaultOriginQuota, quint64 c)
{
    emit q->applicationCacheQuotaExceeded(origin, defaultOriginQuota, c);
}

void QWebPagePrivate::setToolTip(const QString &tip)
{
#ifndef QT_NO_TOOLTIP
    if (!view)
        return;

    if (tip.isEmpty()) {
        view->setToolTip(QString());
        QToolTip::hideText();
    } else {
        QString dtip = QLatin1String("<p>") + QString(tip).toHtmlEscaped() + QLatin1String("</p>");
        view->setToolTip(dtip);
    }
#else
    Q_UNUSED(tip);
#endif
}

#if USE(QT_MULTIMEDIA)
QWebFullScreenVideoHandler *QWebPagePrivate::createFullScreenVideoHandler()
{
    return new WebKit::DefaultFullScreenVideoHandler;
}
#endif

QWebFrameAdapter *QWebPagePrivate::mainFrameAdapter()
{
    return q->mainFrame()->d;
}

QStringList QWebPagePrivate::chooseFiles(QWebFrameAdapter *frame, bool allowMultiple, const QStringList &suggestedFileNames)
{
    if (allowMultiple && q->supportsExtension(QWebPage::ChooseMultipleFilesExtension)) {
        QWebPage::ChooseMultipleFilesExtensionOption option;
        option.parentFrame = QWebFramePrivate::kit(frame);
        option.suggestedFileNames = suggestedFileNames;

        QWebPage::ChooseMultipleFilesExtensionReturn output;
        q->extension(QWebPage::ChooseMultipleFilesExtension, &option, &output);

        return output.fileNames;
    }
    // Single file
    QStringList result;
    QString suggestedFile;
    if (!suggestedFileNames.isEmpty())
        suggestedFile = suggestedFileNames.first();
    QString file = q->chooseFile(QWebFramePrivate::kit(frame), suggestedFile);
    if (!file.isEmpty())
        result << file;
    return result;
}

bool QWebPagePrivate::acceptNavigationRequest(QWebFrameAdapter *frameAdapter, const QNetworkRequest &request, int type)
{
    QWebFrame *frame = frameAdapter ? QWebFramePrivate::kit(frameAdapter): 0;
    if (insideOpenCall
        && frame == mainFrame.data())
        return true;
    return q->acceptNavigationRequest(frame, request, QWebPage::NavigationType(type));
}

void QWebPagePrivate::emitRestoreFrameStateRequested(QWebFrameAdapter *frame)
{
    emit q->restoreFrameStateRequested(QWebFramePrivate::kit(frame));
}

void QWebPagePrivate::emitSaveFrameStateRequested(QWebFrameAdapter *frame, QWebHistoryItem *item)
{
    emit q->saveFrameStateRequested(QWebFramePrivate::kit(frame), item);
}

void QWebPagePrivate::emitDownloadRequested(const QNetworkRequest &request)
{
    emit q->downloadRequested(request);
}

void QWebPagePrivate::emitFrameCreated(QWebFrameAdapter *frame)
{
    emit q->frameCreated(QWebFramePrivate::kit(frame));
}

bool QWebPagePrivate::errorPageExtension(QWebPageAdapter::ErrorPageOption *opt, QWebPageAdapter::ErrorPageReturn *out)
{
    QWebPage::ErrorPageExtensionOption option;
    if (opt->domain == QLatin1String("QtNetwork"))
        option.domain = QWebPage::QtNetwork;
    else if (opt->domain == QLatin1String("HTTP"))
        option.domain = QWebPage::Http;
    else if (opt->domain == QLatin1String("WebKit"))
        option.domain = QWebPage::WebKit;
    else
        return false;
    option.url = opt->url;
    option.frame = QWebFramePrivate::kit(opt->frame);
    option.error = opt->error;
    option.errorString = opt->errorString;
    QWebPage::ErrorPageExtensionReturn output;
    if (!q->extension(QWebPage::ErrorPageExtension, &option, &output))
        return false;
    out->baseUrl = output.baseUrl;
    out->content = output.content;
    out->contentType = output.contentType;
    out->encoding = output.encoding;
    return true;
}

QtPluginWidgetAdapter *QWebPagePrivate::createPlugin(const QString &classid, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues)
{
    QObject *widget = q->createPlugin(classid, url, paramNames, paramValues);
    return adapterForWidget(widget);
}

QtPluginWidgetAdapter *QWebPagePrivate::adapterForWidget(QObject *object) const
{
    if (QWidget *widget = qobject_cast<QWidget*>(object))
        return new QWidgetPluginImpl(widget);
#ifndef QT_NO_GRAPHICSVIEW
    if (QGraphicsWidget *widget = qobject_cast<QGraphicsWidget*>(object))
        return new QGraphicsWidgetPluginImpl(widget);
#endif
    return 0;
}

void QWebPagePrivate::createMainFrame()
{
    if (!mainFrame) {
        mainFrame = new QWebFrame(q);
        emit q->frameCreated(mainFrame.data());
    }
}

#define MAP_WEB_ACTION_FROM_ADAPTER_EQUIVALENT(Name, Value) \
    case QWebPageAdapter::Name: return QWebPage::Name

static QWebPage::WebAction webActionForAdapterMenuAction(QWebPageAdapter::MenuAction action)
{
    switch (action) {
        FOR_EACH_MAPPED_MENU_ACTION(MAP_WEB_ACTION_FROM_ADAPTER_EQUIVALENT, SEMICOLON_SEPARATOR);
#if ENABLE(INSPECTOR)
    case QWebPageAdapter::InspectElement: return QWebPage::InspectElement;
#endif
    default:
        ASSERT_NOT_REACHED();
        break;
    }
    return QWebPage::NoWebAction;
}

#define MAP_ADAPTER_ACTION_FROM_WEBACTION_EQUIVALENT(Name, Value) \
    case QWebPage::Name: return QWebPageAdapter::Name

static QWebPageAdapter::MenuAction adapterMenuActionForWebAction(QWebPage::WebAction action)
{
    switch (action) {
        FOR_EACH_MAPPED_MENU_ACTION(MAP_ADAPTER_ACTION_FROM_WEBACTION_EQUIVALENT, SEMICOLON_SEPARATOR);
#if ENABLE(INSPECTOR)
    case QWebPage::InspectElement: return QWebPageAdapter::InspectElement;
#endif
    default:
        ASSERT_NOT_REACHED();
        break;
    }
    return QWebPageAdapter::NoAction;
}

#ifndef QT_NO_CONTEXTMENU
typedef QWebPageAdapter::MenuItemDescription MenuItem;
QMenu *createContextMenu(QWebPage* page, const QList<MenuItem>& items, QBitArray *visitedWebActions)
{
    if (items.isEmpty())
        return 0;

    QMenu* menu = new QMenu(page->view());
    for (int i = 0; i < items.count(); ++i) {
        const MenuItem &item = items.at(i);
        switch (item.type) {
        case MenuItem::Action: {
            QWebPage::WebAction action = webActionForAdapterMenuAction(item.action);
            QAction *a = page->action(action);
            if (a) {
                a->setText(item.title);
                a->setEnabled(item.traits & MenuItem::Enabled);
                a->setCheckable(item.traits & MenuItem::Checkable);
                a->setChecked(item.traits & MenuItem::Checked);

                menu->addAction(a);
                visitedWebActions->setBit(action);
            }
            break;
        }
        case MenuItem::Separator:
            menu->addSeparator();
            break;
        case MenuItem::SubMenu: {
            QMenu *subMenu = createContextMenu(page, item.subMenu, visitedWebActions);
            Q_ASSERT(subMenu);

            bool anyEnabledAction = false;

            QList<QAction *> actions = subMenu->actions();
            for (int i = 0; i < actions.count(); ++i) {
                if (actions.at(i)->isVisible())
                    anyEnabledAction |= actions.at(i)->isEnabled();
            }

            // don't show sub-menus with just disabled actions
            if (anyEnabledAction) {
                subMenu->setTitle(item.title);
                menu->addAction(subMenu->menuAction());
            } else
                delete subMenu;
            break;
        }
        }
    }
    return menu;
}
#endif // QT_NO_CONTEXTMENU

void QWebPagePrivate::createAndSetCurrentContextMenu(const QList<MenuItemDescription>& items, QBitArray* visitedWebActions)
{
#ifndef QT_NO_CONTEXTMENU
    delete currentContextMenu.data();

    currentContextMenu = createContextMenu(q, items, visitedWebActions);
#else
    Q_UNUSED(menuDescription);
    Q_UNUSED(visitedWebActions);
#endif // QT_NO_CONTEXTMENU
}

#ifndef QT_NO_ACTION
void QWebPagePrivate::_q_webActionTriggered(bool checked)
{
    QAction *a = qobject_cast<QAction *>(q->sender());
    if (!a)
        return;
    QWebPage::WebAction action = static_cast<QWebPage::WebAction>(a->data().toInt());
    q->triggerAction(action, checked);
}
#endif // QT_NO_ACTION

void QWebPagePrivate::updateAction(QWebPage::WebAction action)
{
#ifdef QT_NO_ACTION
    Q_UNUSED(action)
#else
    QAction *a = actions[action];
    if (!a || !mainFrame)
        return;

    bool enabled = a->isEnabled();
    bool checked = a->isChecked();

    QWebPageAdapter::MenuAction mappedAction = QWebPageAdapter::NoAction;
    const char* commandName = 0;

    switch (action) {
    case QWebPage::Back:
    case QWebPage::Forward:
    case QWebPage::Stop:
    case QWebPage::Reload:
    case QWebPage::SetTextDirectionDefault:
    case QWebPage::SetTextDirectionLeftToRight:
    case QWebPage::SetTextDirectionRightToLeft:
        mappedAction = adapterMenuActionForWebAction(action);
        break;
    case QWebPage::ReloadAndBypassCache: // Manual mapping
        mappedAction = QWebPageAdapter::Reload;
        break;
#ifndef QT_NO_UNDOSTACK
    case QWebPage::Undo:
    case QWebPage::Redo:
        // those two are handled by QUndoStack
        break;
#endif // QT_NO_UNDOSTACK
    case QWebPage::SelectAll: // editor command is always enabled
        break;
    default: {
        // see if it's an editor command
        commandName = editorCommandForWebActions(action);
        break;
    }
    }
    if (mappedAction != QWebPageAdapter::NoAction || commandName)
        updateActionInternal(mappedAction, commandName, &enabled, &checked);

    a->setEnabled(enabled);

    if (a->isCheckable())
        a->setChecked(checked);
#endif // QT_NO_ACTION
}

void QWebPagePrivate::updateNavigationActions()
{
    updateAction(QWebPage::Back);
    updateAction(QWebPage::Forward);
    updateAction(QWebPage::Stop);
    updateAction(QWebPage::Reload);
    updateAction(QWebPage::ReloadAndBypassCache);
}

QObject *QWebPagePrivate::inspectorHandle()
{
    return getOrCreateInspector();
}

void QWebPagePrivate::setInspectorFrontend(QObject* frontend)
{
    inspectorFrontend = qobject_cast<QWidget*>(frontend);
    if (inspector)
        inspector->d->setFrontend(frontend);
}

void QWebPagePrivate::setInspectorWindowTitle(const QString& title)
{
    if (inspector)
        inspector->setWindowTitle(title);
}

void QWebPagePrivate::createWebInspector(QObject** inspectorView, QWebPageAdapter** inspectorPage)
{
    QWebPage* page = new WebKit::InspectorClientWebPage;
    *inspectorView = page->view();
    *inspectorPage = page->d;
}

#ifndef QT_NO_MENU
static QStringList iterateContextMenu(QMenu* menu)
{
    if (!menu)
        return QStringList();

    QStringList items;
    QList<QAction *> actions = menu->actions();
    for (int i = 0; i < actions.count(); ++i) {
        if (actions.at(i)->isSeparator())
            items << QLatin1String("<separator>");
        else
            items << actions.at(i)->text();
        if (actions.at(i)->menu())
            items << iterateContextMenu(actions.at(i)->menu());
    }
    return items;
}
#endif

QStringList QWebPagePrivate::menuActionsAsText()
{
#ifndef QT_NO_MENU
    return iterateContextMenu(currentContextMenu.data());
#else
    return QStringList();
#endif
}

void QWebPagePrivate::emitViewportChangeRequested()
{
    emit q->viewportChangeRequested();
}

void QWebPagePrivate::updateEditorActions()
{
    updateAction(QWebPage::Cut);
    updateAction(QWebPage::Copy);
    updateAction(QWebPage::Paste);
    updateAction(QWebPage::MoveToNextChar);
    updateAction(QWebPage::MoveToPreviousChar);
    updateAction(QWebPage::MoveToNextWord);
    updateAction(QWebPage::MoveToPreviousWord);
    updateAction(QWebPage::MoveToNextLine);
    updateAction(QWebPage::MoveToPreviousLine);
    updateAction(QWebPage::MoveToStartOfLine);
    updateAction(QWebPage::MoveToEndOfLine);
    updateAction(QWebPage::MoveToStartOfBlock);
    updateAction(QWebPage::MoveToEndOfBlock);
    updateAction(QWebPage::MoveToStartOfDocument);
    updateAction(QWebPage::MoveToEndOfDocument);
    updateAction(QWebPage::SelectNextChar);
    updateAction(QWebPage::SelectPreviousChar);
    updateAction(QWebPage::SelectNextWord);
    updateAction(QWebPage::SelectPreviousWord);
    updateAction(QWebPage::SelectNextLine);
    updateAction(QWebPage::SelectPreviousLine);
    updateAction(QWebPage::SelectStartOfLine);
    updateAction(QWebPage::SelectEndOfLine);
    updateAction(QWebPage::SelectStartOfBlock);
    updateAction(QWebPage::SelectEndOfBlock);
    updateAction(QWebPage::SelectStartOfDocument);
    updateAction(QWebPage::SelectEndOfDocument);
    updateAction(QWebPage::DeleteStartOfWord);
    updateAction(QWebPage::DeleteEndOfWord);
    updateAction(QWebPage::SetTextDirectionDefault);
    updateAction(QWebPage::SetTextDirectionLeftToRight);
    updateAction(QWebPage::SetTextDirectionRightToLeft);
    updateAction(QWebPage::ToggleBold);
    updateAction(QWebPage::ToggleItalic);
    updateAction(QWebPage::ToggleUnderline);
    updateAction(QWebPage::InsertParagraphSeparator);
    updateAction(QWebPage::InsertLineSeparator);
    updateAction(QWebPage::PasteAndMatchStyle);
    updateAction(QWebPage::RemoveFormat);
    updateAction(QWebPage::ToggleStrikethrough);
    updateAction(QWebPage::ToggleSubscript);
    updateAction(QWebPage::ToggleSuperscript);
    updateAction(QWebPage::InsertUnorderedList);
    updateAction(QWebPage::InsertOrderedList);
    updateAction(QWebPage::Indent);
    updateAction(QWebPage::Outdent);
    updateAction(QWebPage::AlignCenter);
    updateAction(QWebPage::AlignJustified);
    updateAction(QWebPage::AlignLeft);
    updateAction(QWebPage::AlignRight);
}

void QWebPagePrivate::timerEvent(QTimerEvent *ev)
{
    int timerId = ev->timerId();
    if (timerId == tripleClickTimer.timerId())
        tripleClickTimer.stop();
    else
        q->timerEvent(ev);
}

bool QWebPagePrivate::requestSoftwareInputPanel() const
{
    return QStyle::RequestSoftwareInputPanel(client->style()->styleHint(QStyle::SH_RequestSoftwareInputPanel)) == QStyle::RSIP_OnMouseClick;
}

#ifndef QT_NO_CONTEXTMENU
void QWebPagePrivate::contextMenuEvent(const QPoint& globalPos)
{
    QMenu *menu = q->createStandardContextMenu();
    if (menu) {
        menu->exec(globalPos);
        delete menu;
    }
}
#endif // QT_NO_CONTEXTMENU

/*!
    \since 4.5
    This function creates the standard context menu which is shown when
    the user clicks on the web page with the right mouse button. It is
    called from the default contextMenuEvent() handler. The popup menu's
    ownership is transferred to the caller.
 */
QMenu *QWebPage::createStandardContextMenu()
{
#ifndef QT_NO_CONTEXTMENU
    QMenu* menu = d->currentContextMenu.data();
    d->currentContextMenu = 0;
    return menu;
#else
    return 0;
#endif
}

#ifndef QT_NO_SHORTCUT
QWebPage::WebAction QWebPagePrivate::editorActionForKeyEvent(QKeyEvent* event)
{
    static struct {
        QKeySequence::StandardKey standardKey;
        QWebPage::WebAction action;
    } editorActions[] = {
        { QKeySequence::Cut, QWebPage::Cut },
        { QKeySequence::Copy, QWebPage::Copy },
        { QKeySequence::Paste, QWebPage::Paste },
        { QKeySequence::Undo, QWebPage::Undo },
        { QKeySequence::Redo, QWebPage::Redo },
        { QKeySequence::MoveToNextChar, QWebPage::MoveToNextChar },
        { QKeySequence::MoveToPreviousChar, QWebPage::MoveToPreviousChar },
        { QKeySequence::MoveToNextWord, QWebPage::MoveToNextWord },
        { QKeySequence::MoveToPreviousWord, QWebPage::MoveToPreviousWord },
        { QKeySequence::MoveToNextLine, QWebPage::MoveToNextLine },
        { QKeySequence::MoveToPreviousLine, QWebPage::MoveToPreviousLine },
        { QKeySequence::MoveToStartOfLine, QWebPage::MoveToStartOfLine },
        { QKeySequence::MoveToEndOfLine, QWebPage::MoveToEndOfLine },
        { QKeySequence::MoveToStartOfBlock, QWebPage::MoveToStartOfBlock },
        { QKeySequence::MoveToEndOfBlock, QWebPage::MoveToEndOfBlock },
        { QKeySequence::MoveToStartOfDocument, QWebPage::MoveToStartOfDocument },
        { QKeySequence::MoveToEndOfDocument, QWebPage::MoveToEndOfDocument },
        { QKeySequence::SelectNextChar, QWebPage::SelectNextChar },
        { QKeySequence::SelectPreviousChar, QWebPage::SelectPreviousChar },
        { QKeySequence::SelectNextWord, QWebPage::SelectNextWord },
        { QKeySequence::SelectPreviousWord, QWebPage::SelectPreviousWord },
        { QKeySequence::SelectNextLine, QWebPage::SelectNextLine },
        { QKeySequence::SelectPreviousLine, QWebPage::SelectPreviousLine },
        { QKeySequence::SelectStartOfLine, QWebPage::SelectStartOfLine },
        { QKeySequence::SelectEndOfLine, QWebPage::SelectEndOfLine },
        { QKeySequence::SelectStartOfBlock, QWebPage::SelectStartOfBlock },
        { QKeySequence::SelectEndOfBlock,  QWebPage::SelectEndOfBlock },
        { QKeySequence::SelectStartOfDocument, QWebPage::SelectStartOfDocument },
        { QKeySequence::SelectEndOfDocument, QWebPage::SelectEndOfDocument },
        { QKeySequence::DeleteStartOfWord, QWebPage::DeleteStartOfWord },
        { QKeySequence::DeleteEndOfWord, QWebPage::DeleteEndOfWord },
        { QKeySequence::InsertParagraphSeparator, QWebPage::InsertParagraphSeparator },
        { QKeySequence::InsertLineSeparator, QWebPage::InsertLineSeparator },
        { QKeySequence::SelectAll, QWebPage::SelectAll },
        { QKeySequence::UnknownKey, QWebPage::NoWebAction }
    };

    for (int i = 0; editorActions[i].standardKey != QKeySequence::UnknownKey; ++i)
        if (event == editorActions[i].standardKey)
            return editorActions[i].action;

    return QWebPage::NoWebAction;
}
#endif // QT_NO_SHORTCUT

void QWebPagePrivate::keyPressEvent(QKeyEvent *ev)
{
    // we forward the key event to WebCore first to handle potential DOM
    // defined event handlers and later on end up in EditorClientQt::handleKeyboardEvent
    // to trigger editor commands via triggerAction().
    bool handled = handleKeyEvent(ev);

    if (!handled)
        handled = handleScrolling(ev);

    if (!handled) {
        handled = true;
        switch (ev->key()) {
        case Qt::Key_Back:
            q->triggerAction(QWebPage::Back);
            break;
        case Qt::Key_Forward:
            q->triggerAction(QWebPage::Forward);
            break;
        case Qt::Key_Stop:
            q->triggerAction(QWebPage::Stop);
            break;
        case Qt::Key_Refresh:
            q->triggerAction(QWebPage::Reload);
            break;
        default:
            handled = false;
            break;
        }
    }

    ev->setAccepted(handled);
}

void QWebPagePrivate::keyReleaseEvent(QKeyEvent *ev)
{
    if (ev->isAutoRepeat()) {
        ev->setAccepted(true);
        return;
    }

    bool handled = handleKeyEvent(ev);
    ev->setAccepted(handled);
}

template<class T>
void QWebPagePrivate::dragEnterEvent(T* ev)
{
#ifndef QT_NO_DRAGANDDROP
    Qt::DropAction action = dragEntered(ev->mimeData(), QPointF(ev->pos()).toPoint(), ev->possibleActions());
    ev->setDropAction(action);
    ev->acceptProposedAction();
#endif
}

template<class T>
void QWebPagePrivate::dragMoveEvent(T *ev)
{
#ifndef QT_NO_DRAGANDDROP
    m_lastDropAction = dragUpdated(ev->mimeData(), QPointF(ev->pos()).toPoint(), ev->possibleActions());
    ev->setDropAction(m_lastDropAction);
    if (m_lastDropAction != Qt::IgnoreAction)
        ev->accept();
#endif
}

template<class T>
void QWebPagePrivate::dropEvent(T *ev)
{
#ifndef QT_NO_DRAGANDDROP
    if (performDrag(ev->mimeData(), QPointF(ev->pos()).toPoint(), ev->possibleActions())) {
        ev->setDropAction(m_lastDropAction);
        ev->accept();
    }
#endif
}

void QWebPagePrivate::leaveEvent(QEvent*)
{
    // Fake a mouse move event just outside of the widget, since all
    // the interesting mouse-out behavior like invalidating scrollbars
    // is handled by the WebKit event handler's mouseMoved function.
    QMouseEvent fakeEvent(QEvent::MouseMove, QCursor::pos(), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    mouseMoveEvent(&fakeEvent);
}

/*!
    \property QWebPage::palette
    \brief the page's palette

    The base brush of the palette is used to draw the background of the main frame.

    By default, this property contains the application's default palette.
*/
void QWebPage::setPalette(const QPalette &pal)
{
    d->palette = pal;
    if (!d->mainFrame || !d->mainFrame.data()->d->hasView())
        return;

    QBrush brush = pal.brush(QPalette::Base);
    QColor backgroundColor = brush.style() == Qt::SolidPattern ? brush.color() : QColor();
    d->mainFrame.data()->d->updateBackgroundRecursively(backgroundColor);
}

QPalette QWebPage::palette() const
{
    return d->palette;
}

void QWebPagePrivate::shortcutOverrideEvent(QKeyEvent* event)
{
    if (handleShortcutOverrideEvent(event))
        return;
#ifndef QT_NO_SHORTCUT
    if (editorActionForKeyEvent(event) != QWebPage::NoWebAction)
        event->accept();
#endif

}

bool QWebPagePrivate::gestureEvent(QGestureEvent* event)
{
    QWebFrameAdapter* frame = mainFrame.data()->d;
    if (!frame->hasView())
        return false;
    // QGestureEvents can contain updates for multiple gestures.
    bool handled = false;
#if ENABLE(GESTURE_EVENTS)
    // QGestureEvent lives in Widgets, we'll need a dummy struct to mule the info it contains to the "other side"
    QGestureEventFacade gestureFacade;

    QGesture* gesture = event->gesture(Qt::TapGesture);
    // Beware that gestures send by DumpRenderTree will have state Qt::NoGesture,
    // due to not originating from a GestureRecognizer.
    if (gesture && (gesture->state() == Qt::GestureStarted || gesture->state() == Qt::NoGesture)) {
        gestureFacade.type = Qt::TapGesture;
        QPointF globalPos = static_cast<const QTapGesture*>(gesture)->position();
        gestureFacade.globalPos = globalPos.toPoint();
        gestureFacade.pos = event->widget()->mapFromGlobal(globalPos.toPoint());
        frame->handleGestureEvent(&gestureFacade);
        handled = true;
    }
    gesture = event->gesture(Qt::TapAndHoldGesture);
    if (gesture && (gesture->state() == Qt::GestureStarted || gesture->state() == Qt::NoGesture)) {
        gestureFacade.type = Qt::TapAndHoldGesture;
        QPointF globalPos = static_cast<const QTapAndHoldGesture*>(gesture)->position();
        gestureFacade.globalPos = globalPos.toPoint();
        gestureFacade.pos = event->widget()->mapFromGlobal(globalPos.toPoint());
        frame->handleGestureEvent(&gestureFacade);
        handled = true;
    }
#endif // ENABLE(GESTURE_EVENTS)

    event->setAccepted(handled);
    return handled;
}

/*!
  This method is used by the input method to query a set of properties of the page
  to be able to support complex input method operations as support for surrounding
  text and reconversions.

  \a property specifies which property is queried.

  \sa QWidget::inputMethodEvent(), QInputMethodEvent, QInputContext
*/
QVariant QWebPage::inputMethodQuery(Qt::InputMethodQuery property) const
{
    return d->inputMethodQuery(property);
}

/*!
    \internal
*/
void QWebPagePrivate::setInspector(QWebInspector* insp)
{
    if (inspector)
        inspector->d->setFrontend(0);

    inspector = insp;

    // Give inspector frontend web view if previously created
    if (inspector && inspectorFrontend)
        inspector->d->setFrontend(inspectorFrontend);
}

/*!
    \internal
    Returns the inspector and creates it if it wasn't created yet.
    The instance created here will not be available through QWebPage's API.
*/
QWebInspector* QWebPagePrivate::getOrCreateInspector()
{
#if ENABLE(INSPECTOR)
    if (!inspector) {
        QWebInspector* insp = new QWebInspector;
        insp->setPage(q);
        inspectorIsInternalOnly = true;

        Q_ASSERT(inspector); // Associated through QWebInspector::setPage(q)
    }
#endif
    return inspector;
}

/*!
   \enum QWebPage::FindFlag

   This enum describes the options available to the findText() function. The options
   can be OR-ed together from the following list:

   \value FindBackward Searches backwards instead of forwards.
   \value FindCaseSensitively By default findText() works case insensitive. Specifying this option
   changes the behaviour to a case sensitive find operation.
   \value FindWrapsAroundDocument Makes findText() restart from the beginning of the document if the end
   was reached and the text was not found.
   \value HighlightAllOccurrences Highlights all existing occurrences of a specific string.
       (This value was introduced in 4.6.)
   \value FindAtWordBeginningsOnly Searches for the sub-string only at the beginnings of words.
       (This value was introduced in 5.2.)
   \value TreatMedialCapitalAsWordBeginning Treats a capital letter occurring anywhere in the middle of a word
   as the beginning of a new word.
       (This value was introduced in 5.2.)
   \value FindBeginsInSelection Begin searching inside the text selection first.
       (This value was introduced in 5.2.)
*/

/*!
    \enum QWebPage::VisibilityState

    This enum defines visibility states that a webpage can take.

    \value VisibilityStateVisible The webpage is at least partially visible at on at least one screen.
    \value VisibilityStateHidden The webpage is not visible at all on any screen.
    \value VisibilityStatePrerender The webpage is loaded off-screen and is not visible.
    \value VisibilityStateUnloaded The webpage is unloading its content.
    More information about this values can be found at \l{ http://www.w3.org/TR/page-visibility/#dom-document-visibilitystate}{W3C Recommendation: Page Visibility: visibilityState attribute}.

    \sa QWebPage::visibilityState
*/

/*!
    \enum QWebPage::LinkDelegationPolicy

    This enum defines the delegation policies a webpage can have when activating links and emitting
    the linkClicked() signal.

    \value DontDelegateLinks No links are delegated. Instead, QWebPage tries to handle them all.
    \value DelegateExternalLinks When activating links that point to documents not stored on the
    local filesystem or an equivalent - such as the Qt resource system - then linkClicked() is emitted.
    \value DelegateAllLinks Whenever a link is activated the linkClicked() signal is emitted.

    \sa QWebPage::linkDelegationPolicy
*/

/*!
    \enum QWebPage::NavigationType

    This enum describes the types of navigation available when browsing through hyperlinked
    documents.

    \value NavigationTypeLinkClicked The user clicked on a link or pressed return on a focused link.
    \value NavigationTypeFormSubmitted The user activated a submit button for an HTML form.
    \value NavigationTypeBackOrForward Navigation to a previously shown document in the back or forward history is requested.
    \value NavigationTypeReload The user activated the reload action.
    \value NavigationTypeFormResubmitted An HTML form was submitted a second time.
    \value NavigationTypeOther A navigation to another document using a method not listed above.

    \sa acceptNavigationRequest()
*/

/*!
    \enum QWebPage::WebAction

    This enum describes the types of action which can be performed on the web page.

    Actions only have an effect when they are applicable. The availability of
    actions can be be determined by checking \l{QAction::}{isEnabled()} on the
    action returned by action().

    One method of enabling the text editing, cursor movement, and text selection actions
    is by setting \l contentEditable to true.

    \value NoWebAction No action is triggered.
    \value OpenLink Open the current link.
    \value OpenLinkInNewWindow Open the current link in a new window.
    \value OpenLinkInThisWindow Open the current link without opening a new window. Used on links that would default to opening in another frame or a new window. (Added in Qt 5.0)
    \value OpenFrameInNewWindow Replicate the current frame in a new window.
    \value DownloadLinkToDisk Download the current link to the disk.
    \value CopyLinkToClipboard Copy the current link to the clipboard.
    \value OpenImageInNewWindow Open the highlighted image in a new window.
    \value DownloadImageToDisk Download the highlighted image to the disk.
    \value CopyImageToClipboard Copy the highlighted image to the clipboard.  (Added in Qt 4.8)
    \value CopyImageUrlToClipboard Copy the highlighted image's URL to the clipboard.
    \value Back Navigate back in the history of navigated links.
    \value Forward Navigate forward in the history of navigated links.
    \value Stop Stop loading the current page.
    \value StopScheduledPageRefresh Stop all pending page refresh/redirect requests.  (Added in Qt 4.7)
    \value Reload Reload the current page.
    \value ReloadAndBypassCache Reload the current page, but do not use any local cache. (Added in Qt 4.6)
    \value Cut Cut the content currently selected into the clipboard.
    \value Copy Copy the content currently selected into the clipboard.
    \value Paste Paste content from the clipboard.
    \value Undo Undo the last editing action.
    \value Redo Redo the last editing action.
    \value MoveToNextChar Move the cursor to the next character.
    \value MoveToPreviousChar Move the cursor to the previous character.
    \value MoveToNextWord Move the cursor to the next word.
    \value MoveToPreviousWord Move the cursor to the previous word.
    \value MoveToNextLine Move the cursor to the next line.
    \value MoveToPreviousLine Move the cursor to the previous line.
    \value MoveToStartOfLine Move the cursor to the start of the line.
    \value MoveToEndOfLine Move the cursor to the end of the line.
    \value MoveToStartOfBlock Move the cursor to the start of the block.
    \value MoveToEndOfBlock Move the cursor to the end of the block.
    \value MoveToStartOfDocument Move the cursor to the start of the document.
    \value MoveToEndOfDocument Move the cursor to the end of the document.
    \value SelectNextChar Select to the next character.
    \value SelectPreviousChar Select to the previous character.
    \value SelectNextWord Select to the next word.
    \value SelectPreviousWord Select to the previous word.
    \value SelectNextLine Select to the next line.
    \value SelectPreviousLine Select to the previous line.
    \value SelectStartOfLine Select to the start of the line.
    \value SelectEndOfLine Select to the end of the line.
    \value SelectStartOfBlock Select to the start of the block.
    \value SelectEndOfBlock Select to the end of the block.
    \value SelectStartOfDocument Select to the start of the document.
    \value SelectEndOfDocument Select to the end of the document.
    \value DeleteStartOfWord Delete to the start of the word.
    \value DeleteEndOfWord Delete to the end of the word.
    \value SetTextDirectionDefault Set the text direction to the default direction.
    \value SetTextDirectionLeftToRight Set the text direction to left-to-right.
    \value SetTextDirectionRightToLeft Set the text direction to right-to-left.
    \value ToggleBold Toggle the formatting between bold and normal weight.
    \value ToggleItalic Toggle the formatting between italic and normal style.
    \value ToggleUnderline Toggle underlining.
    \value InspectElement Show the Web Inspector with the currently highlighted HTML element.
    \value InsertParagraphSeparator Insert a new paragraph.
    \value InsertLineSeparator Insert a new line.
    \value SelectAll Selects all content.
    \value PasteAndMatchStyle Paste content from the clipboard with current style. (Added in Qt 4.6)
    \value RemoveFormat Removes formatting and style. (Added in Qt 4.6)
    \value ToggleStrikethrough Toggle the formatting between strikethrough and normal style. (Added in Qt 4.6)
    \value ToggleSubscript Toggle the formatting between subscript and baseline. (Added in Qt 4.6)
    \value ToggleSuperscript Toggle the formatting between supercript and baseline. (Added in Qt 4.6)
    \value InsertUnorderedList Toggles the selection between an ordered list and a normal block. (Added in Qt 4.6)
    \value InsertOrderedList Toggles the selection between an ordered list and a normal block. (Added in Qt 4.6)
    \value Indent Increases the indentation of the currently selected format block by one increment. (Added in Qt 4.6)
    \value Outdent Decreases the indentation of the currently selected format block by one increment. (Added in Qt 4.6)
    \value AlignCenter Applies center alignment to content. (Added in Qt 4.6)
    \value AlignJustified Applies full justification to content. (Added in Qt 4.6)
    \value AlignLeft Applies left justification to content. (Added in Qt 4.6)
    \value AlignRight Applies right justification to content. (Added in Qt 4.6)
    \value DownloadMediaToDisk Download the hovered audio or video to the disk. (Added in Qt 5.2)
    \value CopyMediaUrlToClipboard Copy the hovered audio or video's URL to the clipboard. (Added in Qt 5.2)
    \value ToggleMediaControls Toggles between showing and hiding the controls for the hovered audio or video element. (Added in Qt 5.2)
    \value ToggleMediaLoop Toggles whether the hovered audio or video should loop on completetion or not. (Added in Qt 5.2)
    \value ToggleMediaPlayPause Toggles the play/pause state of the hovered audio or video element. (Added in Qt 5.2)
    \value ToggleMediaMute Mutes or unmutes the hovered audio or video element. (Added in Qt 5.2)
    \value ToggleVideoFullscreen Switches the hovered video element into or out of fullscreen mode. (Added in Qt 5.2)

    \omitvalue WebActionCount

*/

/*!
    \enum QWebPage::WebWindowType

    This enum describes the types of window that can be created by the createWindow() function.

    \value WebBrowserWindow The window is a regular web browser window.
    \value WebModalDialog The window acts as modal dialog.
*/


/*!
    \class QWebPage::ViewportAttributes
    \since 4.7
    \brief The QWebPage::ViewportAttributes class describes hints that can be applied to a viewport.

    QWebPage::ViewportAttributes provides a description of a viewport, such as viewport geometry,
    initial scale factor with limits, plus information about whether a user should be able
    to scale the contents in the viewport or not, ie. by zooming.

    ViewportAttributes can be set by a web author using the viewport meta tag extension, documented
    at \l{http://developer.apple.com/safari/library/documentation/appleapplications/reference/safariwebcontent/usingtheviewport/usingtheviewport.html}{Safari Reference Library: Using the Viewport Meta Tag}.

    All values might not be set, as such when dealing with the hints, the developer needs to
    check whether the values are valid. Negative values denote an invalid qreal value.

    \inmodule QtWebKit
*/

/*!
    Constructs an empty QWebPage::ViewportAttributes.
*/
QWebPage::ViewportAttributes::ViewportAttributes()
    : d(0)
    , m_initialScaleFactor(-1.0)
    , m_minimumScaleFactor(-1.0)
    , m_maximumScaleFactor(-1.0)
    , m_devicePixelRatio(-1.0)
    , m_isUserScalable(true)
    , m_isValid(false)
{

}

/*!
    Constructs a QWebPage::ViewportAttributes which is a copy from \a other .
*/
QWebPage::ViewportAttributes::ViewportAttributes(const QWebPage::ViewportAttributes& other)
    : d(other.d)
    , m_initialScaleFactor(other.m_initialScaleFactor)
    , m_minimumScaleFactor(other.m_minimumScaleFactor)
    , m_maximumScaleFactor(other.m_maximumScaleFactor)
    , m_devicePixelRatio(other.m_devicePixelRatio)
    , m_isUserScalable(other.m_isUserScalable)
    , m_isValid(other.m_isValid)
    , m_size(other.m_size)
{

}

/*!
    Destroys the QWebPage::ViewportAttributes.
*/
QWebPage::ViewportAttributes::~ViewportAttributes()
{

}

/*!
    Assigns the given QWebPage::ViewportAttributes to this viewport hints and returns a
    reference to this.
*/
QWebPage::ViewportAttributes& QWebPage::ViewportAttributes::operator=(const QWebPage::ViewportAttributes& other)
{
    if (this != &other) {
        d = other.d;
        m_initialScaleFactor = other.m_initialScaleFactor;
        m_minimumScaleFactor = other.m_minimumScaleFactor;
        m_maximumScaleFactor = other.m_maximumScaleFactor;
        m_isUserScalable = other.m_isUserScalable;
        m_isValid = other.m_isValid;
        m_size = other.m_size;
    }

    return *this;
}

/*! \fn inline bool QWebPage::ViewportAttributes::isValid() const
    Returns whether this is a valid ViewportAttributes or not.

    An invalid ViewportAttributes will have an empty QSize, negative values for scale factors and
    true for the boolean isUserScalable.
*/

/*! \fn inline QSize QWebPage::ViewportAttributes::size() const
    Returns the size of the viewport.
*/

/*! \fn inline qreal QWebPage::ViewportAttributes::initialScaleFactor() const
    Returns the initial scale of the viewport as a multiplier.
*/

/*! \fn inline qreal QWebPage::ViewportAttributes::minimumScaleFactor() const
    Returns the minimum scale value of the viewport as a multiplier.
*/

/*! \fn inline qreal QWebPage::ViewportAttributes::maximumScaleFactor() const
    Returns the maximum scale value of the viewport as a multiplier.
*/

/*! \fn inline bool QWebPage::ViewportAttributes::isUserScalable() const
    Determines whether or not the scale can be modified by the user.
*/


/*!
    \class QWebPage
    \since 4.4
    \brief The QWebPage class provides an object to view and edit web documents.

    \inmodule QtWebKit

    QWebPage holds a main frame responsible for web content, settings, the history
    of navigated links and actions. This class can be used, together with QWebFrame,
    to provide functionality like QWebView in a widget-less environment.

    QWebPage's API is very similar to QWebView, as you are still provided with
    common functions like action() (known as
    \l{QWebView::pageAction()}{pageAction}() in QWebView), triggerAction(),
    findText() and settings(). More QWebView-like functions can be found in the
    main frame of QWebPage, obtained via the mainFrame() function. For example,
    the \l{QWebFrame::load()}{load}(), \l{QWebFrame::setUrl()}{setUrl}() and
    \l{QWebFrame::setHtml()}{setHtml}() functions for QWebPage can be accessed
    using QWebFrame.

    The loadStarted() signal is emitted when the page begins to load.The
    loadProgress() signal, on the other hand, is emitted whenever an element
    of the web page completes loading, such as an embedded image, a script,
    etc. Finally, the loadFinished() signal is emitted when the page contents
    are loaded completely, independent of script execution or page rendering.
    Its argument, either true or false, indicates whether or not the load
    operation succeeded.

    \section1 Using QWebPage in a Widget-less Environment

    Before you begin painting a QWebPage object, you need to set the size of
    the viewport by calling setViewportSize(). Then, you invoke the main
    frame's render function (QWebFrame::render()). An example of this
    is shown in the code snippet below.

    Suppose we have a \c Thumbnail class as follows:

    \snippet webkitsnippets/webpage/main.cpp 0

    The \c Thumbnail's constructor takes in a \a url. We connect our QWebPage
    object's \l{QWebPage::}{loadFinished()} signal to our private slot,
    \c render().

    \snippet webkitsnippets/webpage/main.cpp 1

    The \c render() function shows how we can paint a thumbnail using a
    QWebPage object.

    \snippet webkitsnippets/webpage/main.cpp 2

    We begin by setting the \l{QWebPage::viewportSize()}{viewportSize} and
    then we instantiate a QImage object, \c image, with the same size as our
    \l{QWebPage::viewportSize()}{viewportSize}. This image is then sent
    as a parameter to \c painter. Next, we render the contents of the main
    frame and its subframes into \c painter. Finally, we save the scaled image.

    \sa QWebFrame
*/

/*!
    Constructs an empty QWebPage with parent \a parent.
*/
QWebPage::QWebPage(QObject *parent)
    : QObject(parent)
    , d(new QWebPagePrivate(this))
{
    setView(qobject_cast<QWidget*>(parent));

    connect(this, SIGNAL(loadProgress(int)), this, SLOT(_q_onLoadProgressChanged(int)));
#ifndef NDEBUG
    connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(_q_cleanupLeakMessages()));
#endif
}

/*!
    Destroys the web page.
*/
QWebPage::~QWebPage()
{
    delete d;
}

/*!
    Returns the main frame of the page.

    The main frame provides access to the hierarchy of sub-frames and is also needed if you
    want to explicitly render a web page into a given painter.

    \sa currentFrame()
*/
QWebFrame *QWebPage::mainFrame() const
{
    d->createMainFrame();
    return d->mainFrame.data();
}

/*!
    Returns the frame currently active.

    \sa mainFrame(), frameCreated()
*/
QWebFrame *QWebPage::currentFrame() const
{
    d->createMainFrame();
    return qobject_cast<QWebFrame*>(d->currentFrame());
}


/*!
    \since 4.6

    Returns the frame at the given point \a pos, or 0 if there is no frame at
    that position.

    \sa mainFrame(), currentFrame()
*/
QWebFrame* QWebPage::frameAt(const QPoint& pos) const
{
    QWebFrame* webFrame = mainFrame();
    if (!webFrame->geometry().contains(pos))
        return 0;
    QWebHitTestResult hitTestResult = webFrame->hitTestContent(pos);
    return hitTestResult.frame();
}

/*!
    Returns a pointer to the view's history of navigated web pages.
*/
QWebHistory *QWebPage::history() const
{
    d->createMainFrame();
    return &d->history;
}

/*!
    Sets the \a view that is associated with the web page.

    \sa view()
*/
void QWebPage::setView(QWidget* view)
{
    if (this->view() == view)
        return;

    d->view = view;
    setViewportSize(view ? view->size() : QSize(0, 0));

    // If we have no client, we install a special client delegating
    // the responsibility to the QWidget. This is the code path
    // handling a.o. the "legacy" QWebView.
    //
    // If such a special delegate already exist, we substitute the view.

    if (d->client) {
        if (d->client->isQWidgetClient())
            static_cast<PageClientQWidget*>(d->client.data())->view = view;
        return;
    }

    if (view)
        d->client.reset(new PageClientQWidget(view, this));
}

/*!
    Returns the view widget that is associated with the web page.

    \sa setView()
*/
QWidget *QWebPage::view() const
{
    return d->view.data();
}

/*!
    This function is called whenever a JavaScript program tries to print a \a message to the web browser's console.

    For example in case of evaluation errors the source URL may be provided in \a sourceID as well as the \a lineNumber.

    The default implementation prints nothing.
*/
void QWebPage::javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID)
{
    Q_UNUSED(sourceID);

    // Catch plugin logDestroy message for LayoutTests/plugins/open-and-close-window-with-plugin.html
    // At this point DRT's WebPage has already been destroyed
    if (QWebPageAdapter::drtRun) {
        if (message == QLatin1String("PLUGIN: NPP_Destroy")) {
            fprintf(stdout, "CONSOLE MESSAGE: ");
            if (lineNumber)
                fprintf(stdout, "line %d: ", lineNumber);
            fprintf(stdout, "%s\n", message.toUtf8().constData());
        }
    }
}

/*!
    This function is called whenever a JavaScript program running inside \a frame calls the alert() function with
    the message \a msg.

    The default implementation shows the message, \a msg, with QMessageBox::information.
*/
void QWebPage::javaScriptAlert(QWebFrame *frame, const QString& msg)
{
    Q_UNUSED(frame);
#ifndef QT_NO_MESSAGEBOX
    QMessageBox box(view());
    box.setWindowTitle(tr("JavaScript Alert - %1").arg(mainFrame()->url().host()));
    box.setTextFormat(Qt::PlainText);
    box.setText(msg);
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
#endif
}

/*!
    This function is called whenever a JavaScript program running inside \a frame calls the confirm() function
    with the message, \a msg. Returns true if the user confirms the message; otherwise returns false.

    The default implementation executes the query using QMessageBox::information with QMessageBox::Ok and QMessageBox::Cancel buttons.
*/
bool QWebPage::javaScriptConfirm(QWebFrame *frame, const QString& msg)
{
    Q_UNUSED(frame);
#ifdef QT_NO_MESSAGEBOX
    return true;
#else
    QMessageBox box(view());
    box.setWindowTitle(tr("JavaScript Confirm - %1").arg(mainFrame()->url().host()));
    box.setTextFormat(Qt::PlainText);
    box.setText(msg);
    box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    return QMessageBox::Ok == box.exec();
#endif
}

/*!
    This function is called whenever a JavaScript program running inside \a frame tries to prompt the user for input.
    The program may provide an optional message, \a msg, as well as a default value for the input in \a defaultValue.

    If the prompt was cancelled by the user the implementation should return false; otherwise the
    result should be written to \a result and true should be returned. If the prompt was not cancelled by the
    user, the implementation should return true and the result string must not be null.

    The default implementation uses QInputDialog::getText().
*/
bool QWebPage::javaScriptPrompt(QWebFrame *frame, const QString& msg, const QString& defaultValue, QString* result)
{
    Q_UNUSED(frame);
    bool ok = false;
#ifndef QT_NO_INPUTDIALOG

    QInputDialog dlg(view());
    dlg.setWindowTitle(tr("JavaScript Prompt - %1").arg(mainFrame()->url().host()));

    // Hack to force the dialog's QLabel into plain text mode
    // prevents https://bugs.webkit.org/show_bug.cgi?id=34429
    QLabel* label = dlg.findChild<QLabel*>();
    if (label)
        label->setTextFormat(Qt::PlainText);

    // double the &'s because single & will underline the following character
    // (Accelerator mnemonics)
    QString escMsg(msg);
    escMsg.replace(QChar::fromLatin1('&'), QLatin1String("&&"));
    dlg.setLabelText(escMsg);

    dlg.setTextEchoMode(QLineEdit::Normal);
    dlg.setTextValue(defaultValue);

    ok = !!dlg.exec();

    if (ok && result)
        *result = dlg.textValue();
#endif
    return ok;
}

void QWebPage::javaScriptError(const QString &message, int lineNumber, const QString &sourceID, const QString &stack)
{
    Q_UNUSED(message);
    Q_UNUSED(lineNumber);
    Q_UNUSED(sourceID);
    Q_UNUSED(stack);
}

/*!
    \fn bool QWebPage::shouldInterruptJavaScript()
    \since 4.6
    This function is called when a JavaScript program is running for a long period of time.

    If the user wanted to stop the JavaScript the implementation should return true; otherwise false.

    The default implementation executes the query using QMessageBox::information with QMessageBox::Yes and QMessageBox::No buttons.
*/
bool QWebPage::shouldInterruptJavaScript()
{
#ifdef QT_NO_MESSAGEBOX
    return false;
#else
    return QMessageBox::Yes == QMessageBox::information(view(), tr("JavaScript Problem - %1").arg(mainFrame()->url().host()), tr("The script on this page appears to have a problem. Do you want to stop the script?"), QMessageBox::Yes, QMessageBox::No);
#endif
}

void QWebPage::setFeaturePermission(QWebFrame* frame, Feature feature, PermissionPolicy policy)
{
#if !ENABLE(NOTIFICATIONS) && !ENABLE(LEGACY_NOTIFICATIONS) && !ENABLE(GEOLOCATION)
    Q_UNUSED(frame);
    Q_UNUSED(policy);
#endif
    switch (feature) {
    case Notifications:
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
        if (policy != PermissionUnknown)
            d->setNotificationsAllowedForFrame(frame->d, (policy == PermissionGrantedByUser));
#endif
        break;
    case Geolocation:
#if ENABLE(GEOLOCATION) && HAVE(QTPOSITIONING)
        if (policy != PermissionUnknown)
            d->setGeolocationEnabledForFrame(frame->d, (policy == PermissionGrantedByUser));
#endif
        break;

    default:
        break;
    }
}

/*!
    This function is called whenever WebKit wants to create a new window of the given \a type, for
    example when a JavaScript program requests to open a document in a new window.

    If the new window can be created, the new window's QWebPage is returned; otherwise a null pointer is returned.

    If the view associated with the web page is a QWebView object, then the default implementation forwards
    the request to QWebView's createWindow() function; otherwise it returns a null pointer.

    If \a type is WebModalDialog, the application must call setWindowModality(Qt::ApplicationModal) on the new window.

    \note In the cases when the window creation is being triggered by JavaScript, apart from
    reimplementing this method application must also set the JavaScriptCanOpenWindows attribute
    of QWebSettings to true in order for it to get called.

    \sa acceptNavigationRequest(), QWebView::createWindow()
*/
QWebPage *QWebPage::createWindow(WebWindowType type)
{
    QWebView *webView = qobject_cast<QWebView*>(view());
    if (webView) {
        QWebView *newView = webView->createWindow(type);
        if (newView)
            return newView->page();
    }
    return 0;
}

/*!
    This function is called whenever WebKit encounters a HTML object element with type "application/x-qt-plugin". It is
    called regardless of the value of QWebSettings::PluginsEnabled. The \a classid, \a url, \a paramNames and \a paramValues
    correspond to the HTML object element attributes and child elements to configure the embeddable object.
*/
QObject *QWebPage::createPlugin(const QString &classid, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues)
{
    Q_UNUSED(classid);
    Q_UNUSED(url);
    Q_UNUSED(paramNames);
    Q_UNUSED(paramValues);
    return 0;
}

/*!
 *  Returns the list of all content types supported by QWebPage.
 */
QStringList QWebPage::supportedContentTypes() const
{
    return d->supportedContentTypes();
}

/*!
 *  Returns true if QWebPage can handle the given \a mimeType; otherwise, returns false.
 */
bool QWebPage::supportsContentType(const QString& mimeType) const
{
    return d->supportsContentType(mimeType);
}

static void collectChildFrames(QWebFrame* frame, QList<QWebFrame*>& list)
{
    list << frame->childFrames();
    QListIterator<QWebFrame*> it(frame->childFrames());
    while (it.hasNext())
        collectChildFrames(it.next(), list);
}

/*!
    This function can be called to trigger the specified \a action.
    It is also called by Qt WebKit if the user triggers the action, for example
    through a context menu item.

    If \a action is a checkable action then \a checked specified whether the action
    is toggled or not.

    \sa action()
*/
void QWebPage::triggerAction(WebAction action, bool)
{
    const char *command = 0;
    QWebPageAdapter::MenuAction mappedAction = QWebPageAdapter::NoAction;
    QWebHitTestResultPrivate* hitTestResult = d->hitTestResult.d;

    switch (action) {
    case OpenLink:
    case OpenLinkInNewWindow:
    case OpenLinkInThisWindow:
    case OpenFrameInNewWindow:
    case CopyLinkToClipboard:
    case OpenImageInNewWindow:
    case DownloadImageToDisk:
    case DownloadLinkToDisk:
    case Back:
    case Forward:
    case Stop:
    case Reload:
    case SetTextDirectionDefault:
    case SetTextDirectionLeftToRight:
    case SetTextDirectionRightToLeft:
    case DownloadMediaToDisk:
    case ToggleMediaControls:
    case ToggleMediaLoop:
    case ToggleMediaPlayPause:
    case ToggleMediaMute:
    case ToggleVideoFullscreen:
        mappedAction = adapterMenuActionForWebAction(action);
        break;
    case ReloadAndBypassCache: // Manual mapping
        mappedAction = QWebPageAdapter::Reload;
        break;
#ifndef QT_NO_CLIPBOARD
    case CopyImageToClipboard:
        QApplication::clipboard()->setPixmap(d->hitTestResult.pixmap());
        break;
    case CopyImageUrlToClipboard:
        QApplication::clipboard()->setText(d->hitTestResult.imageUrl().toString());
        break;
    case CopyMediaUrlToClipboard:
        QApplication::clipboard()->setText(d->hitTestResult.mediaUrl().toString());
        break;
#endif
    case InspectElement: {
#if ENABLE(INSPECTOR)
        if (!d->hitTestResult.isNull()) {
            d->getOrCreateInspector(); // Make sure the inspector is created
            d->inspector->show(); // The inspector is expected to be shown on inspection
            mappedAction = QWebPageAdapter::InspectElement;
        }
#endif
        break;
    }
    case StopScheduledPageRefresh: {
        QWebFrame* topFrame = mainFrame();
        topFrame->d->cancelLoad();
        QList<QWebFrame*> childFrames;
        collectChildFrames(topFrame, childFrames);
        QListIterator<QWebFrame*> it(childFrames);
        while (it.hasNext())
            it.next()->d->cancelLoad();
        break;
    }
    default:
        command = QWebPagePrivate::editorCommandForWebActions(action);
        break;
    }
    if (command || mappedAction != QWebPageAdapter::NoAction)
        d->triggerAction(mappedAction, hitTestResult, command, /*endToEndReload*/ action == ReloadAndBypassCache);
}


QColor QWebPagePrivate::colorSelectionRequested(const QColor &selectedColor)
{
    QColor ret = selectedColor;
#ifndef QT_NO_COLORDIALOG
    ret = QColorDialog::getColor(selectedColor, q->view());
    if (!ret.isValid())
        ret = selectedColor;
#endif
    return ret;
}

QWebSelectMethod *QWebPagePrivate::createSelectPopup()
{
    return new QtFallbackWebPopup(this);
}

QRect QWebPagePrivate::viewRectRelativeToWindow()
{

    QWidget* ownerWidget= client.isNull() ? 0 : qobject_cast<QWidget*>(client->ownerWidget());
    if (!ownerWidget)
        return QRect();
    QWidget* topLevelWidget = ownerWidget->window();

    QPoint topLeftCorner = ownerWidget->mapFrom(topLevelWidget, QPoint(0, 0));
    return QRect(topLeftCorner, ownerWidget->size());
}

void QWebPagePrivate::geolocationPermissionRequested(QWebFrameAdapter* frame)
{
    emit q->featurePermissionRequested(QWebFramePrivate::kit(frame), QWebPage::Geolocation);
}

void QWebPagePrivate::geolocationPermissionRequestCancelled(QWebFrameAdapter* frame)
{
    emit q->featurePermissionRequestCanceled(QWebFramePrivate::kit(frame), QWebPage::Geolocation);
}

void QWebPagePrivate::notificationsPermissionRequested(QWebFrameAdapter* frame)
{
    emit q->featurePermissionRequested(QWebFramePrivate::kit(frame), QWebPage::Notifications);
}

void QWebPagePrivate::notificationsPermissionRequestCancelled(QWebFrameAdapter* frame)
{
    emit q->featurePermissionRequestCanceled(QWebFramePrivate::kit(frame), QWebPage::Notifications);
}

void QWebPagePrivate::respondToChangedContents()
{
    updateEditorActions();

    emit q->contentsChanged();
}

void QWebPagePrivate::respondToChangedSelection()
{
    updateEditorActions();
    emit q->selectionChanged();
}

void QWebPagePrivate::microFocusChanged()
{
    emit q->microFocusChanged();
}

void QWebPagePrivate::triggerCopyAction()
{
    q->triggerAction(QWebPage::Copy);
}

void QWebPagePrivate::triggerActionForKeyEvent(QKeyEvent* event)
{
    QWebPage::WebAction action = editorActionForKeyEvent(event);
    q->triggerAction(action);
}

void QWebPagePrivate::clearUndoStack()
{
#ifndef QT_NO_UNDOSTACK
    if (undoStack)
        undoStack->clear();
#endif
}

bool QWebPagePrivate::canUndo() const
{
#ifndef QT_NO_UNDOSTACK
    if (!undoStack)
        return false;
    return undoStack->canUndo();
#else
    return false;
#endif
}

bool QWebPagePrivate::canRedo() const
{
#ifndef QT_NO_UNDOSTACK
    if (!undoStack)
        return false;
    return undoStack->canRedo();
#else
    return false;
#endif
}

void QWebPagePrivate::undo()
{
#ifndef QT_NO_UNDOSTACK
    if (undoStack)
        undoStack->undo();
#endif
}

void QWebPagePrivate::redo()
{
#ifndef QT_NO_UNDOSTACK
    if (undoStack)
        undoStack->redo();
#endif
}

void QWebPagePrivate::createUndoStep(QSharedPointer<UndoStepQt> step)
{
#ifndef QT_NO_UNDOSTACK
    // Call undoStack() getter first to ensure stack is created
    // if it doesn't exist yet.
    q->undoStack()->push(new QWebUndoCommand(step));
#endif
}

const char *QWebPagePrivate::editorCommandForKeyEvent(QKeyEvent* event)
{
    QWebPage::WebAction action = editorActionForKeyEvent(event);
    return editorCommandForWebActions(action);
}

QSize QWebPage::viewportSize() const
{
    if (d->mainFrame && d->mainFrame.data()->d->hasView())
        return d->mainFrame.data()->d->frameRect().size();

    return d->m_viewportSize;
}

/*!
    \property QWebPage::viewportSize
    \brief the size of the viewport

    The size affects for example the visibility of scrollbars
    if the document is larger than the viewport.

    By default, for a newly-created Web page, this property contains a size with
    zero width and height.

    \sa QWebFrame::render(), preferredContentsSize
*/
void QWebPage::setViewportSize(const QSize &size) const
{
    d->m_viewportSize = size;

    d->updateWindow();

    QWebFrameAdapter* mainFrame = d->mainFrameAdapter();
    if (!mainFrame->hasView())
        return;

    mainFrame->setViewportSize(size);
}

void QWebPagePrivate::updateWindow()
{
    QWindow* _window = 0;
    if (view && view->window())
        _window = view->window()->windowHandle();

    if (window == _window)
        return;

    if (window)
        QObject::disconnect(window, SIGNAL(screenChanged(QScreen*)), q, SLOT(_q_updateScreen(QScreen*)));
    window = _window;
    if (window) {
        QObject::connect(window, SIGNAL(screenChanged(QScreen*)), q, SLOT(_q_updateScreen(QScreen*)));
        _q_updateScreen(window->screen());
    }
}

void QWebPagePrivate::_q_updateScreen(QScreen* screen)
{
    if (screen)
        setDevicePixelRatio(screen->devicePixelRatio());
}

static int getintenv(const char* variable)
{
    bool ok;
    int value = qgetenv(variable).toInt(&ok);
    return (ok) ? value : -1;
}

static QSize queryDeviceSizeForScreenContainingWidget(const QWidget* widget)
{
    QDesktopWidget* desktop = QApplication::desktop();
    if (!desktop)
        return QSize();

    QSize size;

    if (widget) {
        // Returns the available geometry of the screen which contains widget.
        // NOTE: this must be the the full screen size including any fixed status areas etc.
        size = desktop->availableGeometry(widget).size();
    } else
        size = desktop->availableGeometry().size();

    // This must be in portrait mode, adjust if not.
    if (size.width() > size.height()) {
        int width = size.width();
        size.setWidth(size.height());
        size.setHeight(width);
    }

    return size;
}

/*!
    Computes the optimal viewport configuration given the \a availableSize, when
    user interface components are disregarded.

    The configuration is also dependent on the device screen size which is obtained
    automatically. For testing purposes the size can be overridden by setting two
    environment variables QTWEBKIT_DEVICE_WIDTH and QTWEBKIT_DEVICE_HEIGHT, which
    both needs to be set.

    The ViewportAttributes includes a pixel density ratio, which will also be exposed to
    the web author though the -webkit-pixel-ratio media feature. This is the ratio
    between 1 density-independent pixel (DPI) and physical pixels.

    A density-independent pixel is equivalent to one physical pixel on a 160 DPI screen,
    so on our platform assumes that as the baseline density.

    The conversion of DIP units to screen pixels is quite simple:

    pixels = DIPs * (density / 160).

    Thus, on a 240 DPI screen, 1 DIPs would equal 1.5 physical pixels.

    An invalid instance will be returned in the case an empty size is passed to the
    method.

    \note The density is automatically obtained from the DPI of the screen where the page
    is being shown, but as many X11 servers are reporting wrong DPI, it is possible to
    override it using QX11Info::setAppDpiY().
*/

QWebPage::ViewportAttributes QWebPage::viewportAttributesForSize(const QSize& availableSize) const
{
    ViewportAttributes result;

    if (availableSize.isEmpty())
        return result; // Returns an invalid instance.

    QSize deviceSize(getintenv("QTWEBKIT_DEVICE_WIDTH"), getintenv("QTWEBKIT_DEVICE_HEIGHT"));

    // Both environment variables need to be set - or they will be ignored.
    if (deviceSize.isNull())
        deviceSize = queryDeviceSizeForScreenContainingWidget(view());
    QWebPageAdapter::ViewportAttributes attr = d->viewportAttributesForSize(availableSize, deviceSize);

    result.m_isValid = true;
    result.m_size = attr.size;
    result.m_initialScaleFactor = attr.initialScaleFactor;
    result.m_minimumScaleFactor = attr.minimumScaleFactor;
    result.m_maximumScaleFactor = attr.maximumScaleFactor;
    result.m_devicePixelRatio = attr.devicePixelRatio;
    result.m_isUserScalable = attr.isUserScalable;

    return result;
}

QSize QWebPage::preferredContentsSize() const
{
    QWebFrameAdapter* mainFrame = d->mainFrame ? d->mainFrame->d : 0;
    QSize customSize;
    if (mainFrame && mainFrame->hasView())
        customSize = mainFrame->customLayoutSize();

    return customSize.isNull() ? d->fixedLayoutSize : customSize;
}

/*!
    \property QWebPage::preferredContentsSize
    \since 4.6
    \brief a custom size used for laying out the page contents.

    By default all pages are laid out using the viewport of the page as the base.

    As pages mostly are designed for desktop usage, they often do not layout properly
    on small devices as the contents require a certain view width. For this reason
    it is common to use a different layout size and then scale the contents to fit
    within the actual view.

    If this property is set to a valid size, this size is used for all layout needs
    instead of the size of the viewport.

    Setting an invalid size, makes the page fall back to using the viewport size for layout.

    \sa viewportSize
*/
void QWebPage::setPreferredContentsSize(const QSize& size) const
{
    // FIXME: Rename this method to setCustomLayoutSize

    d->fixedLayoutSize = size;

    QWebFrameAdapter* mainFrame = d->mainFrameAdapter();
    if (!mainFrame->hasView())
        return;

    mainFrame->setCustomLayoutSize(size);
}

/*
    This function is to be called after any (animated) scroll/pan has ended, in the case the application handles the
    scrolling/panning of the web contents. This is commonly used in combination with tiling where is it common for
    the application to pan the actual view, which then resizes itself to the size of the contents.

    \note Calling this function makes WebKit stop trying to calculate the visibleContentRect. To turn that on
    again, call this method with an empty rect.

    \sa QGraphicsWebView::resizesToContents, QWebSettings::TiledBackingStoreEnabled
*/
void QWebPage::setActualVisibleContentRect(const QRect& rect) const
{
    QWebFrameAdapter* mainFrame = d->mainFrameAdapter();
    if (!mainFrame->hasView())
        return;

    mainFrame->setFixedVisibleContentRect(rect);
}

/*!
    \fn bool QWebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, QWebPage::NavigationType type)

    This function is called whenever WebKit requests to navigate \a frame to the resource specified by \a request by means of
    the specified navigation type \a type.

    If \a frame is a null pointer then navigation to a new window is requested. If the request is
    accepted createWindow() will be called.

    The default implementation interprets the page's linkDelegationPolicy and emits linkClicked accordingly or returns true
    to let QWebPage handle the navigation itself.

    \sa createWindow()
*/
bool QWebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, QWebPage::NavigationType type)
{
    Q_UNUSED(frame);
    if (type == NavigationTypeLinkClicked) {
        switch (d->linkPolicy) {
        case DontDelegateLinks:
            return true;

        case DelegateExternalLinks:
            if (QWebPageAdapter::treatSchemeAsLocal(request.url().scheme()))
                return true;
            emit linkClicked(request.url());
            return false;

        case DelegateAllLinks:
            emit linkClicked(request.url());
            return false;
        }
    }
    return true;
}

/*!
    \property QWebPage::hasSelection
    \brief whether this page contains selected content or not.

    \sa selectionChanged()
*/
bool QWebPage::hasSelection() const
{
    d->createMainFrame();
    return d->hasSelection();
}

/*!
    \property QWebPage::selectedText
    \brief the text currently selected

    By default, this property contains an empty string.

    \sa selectionChanged(), selectedHtml()
*/
QString QWebPage::selectedText() const
{
    d->createMainFrame();
    return d->selectedText();
}

/*!
    \since 4.8
    \property QWebPage::selectedHtml
    \brief the HTML currently selected

    By default, this property contains an empty string.

    \sa selectionChanged(), selectedText()
*/
QString QWebPage::selectedHtml() const
{
    d->createMainFrame();
    return d->selectedHtml();
}

#ifndef QT_NO_ACTION
/*!
   Returns a QAction for the specified WebAction \a action.

   The action is owned by the QWebPage but you can customize the look by
   changing its properties.

   QWebPage also takes care of implementing the action, so that upon
   triggering the corresponding action is performed on the page.

   \sa triggerAction()
*/
QAction *QWebPage::action(WebAction action) const
{
    if (action == QWebPage::NoWebAction)
        return 0;
    if (d->actions[action])
        return d->actions[action];

    QString text;
    QIcon icon;
    QStyle *style = d->client ? d->client->style() : qApp->style();
    bool checkable = false;
    QWebPageAdapter::MenuAction mappedAction = QWebPageAdapter::NoAction;

    switch (action) {
    // to be fetched from LocalizedStringsQt via the page adapter
    case OpenLink:
    case OpenLinkInNewWindow:
    case OpenFrameInNewWindow:
    case OpenLinkInThisWindow:
    case DownloadLinkToDisk:
    case CopyLinkToClipboard:
    case OpenImageInNewWindow:
    case DownloadImageToDisk:
    case CopyImageToClipboard:
    case CopyImageUrlToClipboard:
    case Cut:
    case Copy:
    case Paste:
    case SelectAll:
    case SetTextDirectionDefault:
    case SetTextDirectionLeftToRight:
    case SetTextDirectionRightToLeft:
    case ToggleBold:
    case ToggleItalic:
    case ToggleUnderline:
    case DownloadMediaToDisk:
    case CopyMediaUrlToClipboard:
    case ToggleMediaControls:
    case ToggleMediaLoop:
    case ToggleMediaPlayPause:
    case ToggleMediaMute:
    case ToggleVideoFullscreen:
        mappedAction = adapterMenuActionForWebAction(action);
        break;
    case InspectElement:
#if ENABLE(INSPECTOR)
        mappedAction = QWebPageAdapter::InspectElement;
#endif
        break;

        // icon needed as well, map by hand.
    case Back:
        mappedAction = QWebPageAdapter::Back;
        icon = style->standardIcon(QStyle::SP_ArrowBack);
        break;
    case Forward:
        mappedAction = QWebPageAdapter::Forward;
        icon = style->standardIcon(QStyle::SP_ArrowForward);
        break;
    case Stop:
        mappedAction = QWebPageAdapter::Stop;
        icon = style->standardIcon(QStyle::SP_BrowserStop);
        break;
    case Reload:
        mappedAction = QWebPageAdapter::Reload;
        icon = style->standardIcon(QStyle::SP_BrowserReload);
        break;

#ifndef QT_NO_UNDOSTACK
    case Undo: {
        QAction *a = undoStack()->createUndoAction(d->q);
        d->actions[action] = a;
        return a;
    }
    case Redo: {
        QAction *a = undoStack()->createRedoAction(d->q);
        d->actions[action] = a;
        return a;
    }
#endif // QT_NO_UNDOSTACK
        // in place l10n
        case MoveToNextChar:
            text = tr("Move the cursor to the next character");
            break;
        case MoveToPreviousChar:
            text = tr("Move the cursor to the previous character");
            break;
        case MoveToNextWord:
            text = tr("Move the cursor to the next word");
            break;
        case MoveToPreviousWord:
            text = tr("Move the cursor to the previous word");
            break;
        case MoveToNextLine:
            text = tr("Move the cursor to the next line");
            break;
        case MoveToPreviousLine:
            text = tr("Move the cursor to the previous line");
            break;
        case MoveToStartOfLine:
            text = tr("Move the cursor to the start of the line");
            break;
        case MoveToEndOfLine:
            text = tr("Move the cursor to the end of the line");
            break;
        case MoveToStartOfBlock:
            text = tr("Move the cursor to the start of the block");
            break;
        case MoveToEndOfBlock:
            text = tr("Move the cursor to the end of the block");
            break;
        case MoveToStartOfDocument:
            text = tr("Move the cursor to the start of the document");
            break;
        case MoveToEndOfDocument:
            text = tr("Move the cursor to the end of the document");
            break;
        case SelectNextChar:
            text = tr("Select to the next character");
            break;
        case SelectPreviousChar:
            text = tr("Select to the previous character");
            break;
        case SelectNextWord:
            text = tr("Select to the next word");
            break;
        case SelectPreviousWord:
            text = tr("Select to the previous word");
            break;
        case SelectNextLine:
            text = tr("Select to the next line");
            break;
        case SelectPreviousLine:
            text = tr("Select to the previous line");
            break;
        case SelectStartOfLine:
            text = tr("Select to the start of the line");
            break;
        case SelectEndOfLine:
            text = tr("Select to the end of the line");
            break;
        case SelectStartOfBlock:
            text = tr("Select to the start of the block");
            break;
        case SelectEndOfBlock:
            text = tr("Select to the end of the block");
            break;
        case SelectStartOfDocument:
            text = tr("Select to the start of the document");
            break;
        case SelectEndOfDocument:
            text = tr("Select to the end of the document");
            break;
        case DeleteStartOfWord:
            text = tr("Delete to the start of the word");
            break;
        case DeleteEndOfWord:
            text = tr("Delete to the end of the word");
            break;

        case InsertParagraphSeparator:
            text = tr("Insert a new paragraph");
            break;
        case InsertLineSeparator:
            text = tr("Insert a new line");
            break;

        case PasteAndMatchStyle:
            text = tr("Paste and Match Style");
            break;
        case RemoveFormat:
            text = tr("Remove formatting");
            break;

        case ToggleStrikethrough:
            text = tr("Strikethrough");
            checkable = true;
            break;
        case ToggleSubscript:
            text = tr("Subscript");
            checkable = true;
            break;
        case ToggleSuperscript:
            text = tr("Superscript");
            checkable = true;
            break;
        case InsertUnorderedList:
            text = tr("Insert Bulleted List");
            checkable = true;
            break;
        case InsertOrderedList:
            text = tr("Insert Numbered List");
            checkable = true;
            break;
        case Indent:
            text = tr("Indent");
            break;
        case Outdent:
            text = tr("Outdent");
            break;
        case AlignCenter:
            text = tr("Center");
            break;
        case AlignJustified:
            text = tr("Justify");
            break;
        case AlignLeft:
            text = tr("Align Left");
            break;
        case AlignRight:
            text = tr("Align Right");
            break;
        case NoWebAction:
            return 0;
        default:
            break;
    }
    if (mappedAction != QWebPageAdapter::NoAction)
        text = d->contextMenuItemTagForAction(mappedAction, &checkable);

    if (text.isEmpty())
        return 0;

    QAction *a = new QAction(d->q);
    a->setText(text);
    a->setData(action);
    a->setCheckable(checkable);
    a->setIcon(icon);

    connect(a, SIGNAL(triggered(bool)),
        this, SLOT(_q_webActionTriggered(bool)));

    d->actions[action] = a;
    d->updateAction(action);
    return a;
}
#endif // QT_NO_ACTION

/*!
    \property QWebPage::modified
    \brief whether the page contains unsubmitted form data, or the contents have been changed.

    By default, this property is false.

    \sa contentsChanged(), contentEditable, undoStack()
*/
bool QWebPage::isModified() const
{
#ifdef QT_NO_UNDOSTACK
    return false;
#else
    if (!d->undoStack)
        return false;
    return d->undoStack->canUndo();
#endif // QT_NO_UNDOSTACK
}

#ifndef QT_NO_UNDOSTACK
/*!
    Returns a pointer to the undo stack used for editable content.

    \sa modified
*/
QUndoStack *QWebPage::undoStack() const
{
    if (!d->undoStack)
        d->undoStack = new QUndoStack(const_cast<QWebPage *>(this));

    return d->undoStack;
}
#endif // QT_NO_UNDOSTACK

/*! \reimp
*/
bool QWebPage::event(QEvent *ev)
{
    switch (ev->type()) {
    case QEvent::Timer:
        d->timerEvent(static_cast<QTimerEvent*>(ev));
        break;
    case QEvent::MouseMove:
        d->mouseMoveEvent(static_cast<QMouseEvent*>(ev));
        break;
    case QEvent::MouseButtonPress:
        d->mousePressEvent(static_cast<QMouseEvent*>(ev));
        break;
    case QEvent::MouseButtonDblClick:
        d->mouseDoubleClickEvent(static_cast<QMouseEvent*>(ev));
        break;
    case QEvent::MouseButtonRelease:
        d->mouseReleaseEvent(static_cast<QMouseEvent*>(ev));
        break;
#if !defined(QT_NO_GRAPHICSVIEW)
    case QEvent::GraphicsSceneMouseMove: {
        QGraphicsSceneMouseEvent *gsEv = static_cast<QGraphicsSceneMouseEvent*>(ev);
        QMouseEvent dummyEvent(QEvent::MouseMove, gsEv->pos(), gsEv->screenPos(), gsEv->button(), gsEv->buttons(), gsEv->modifiers());
        d->mouseMoveEvent(&dummyEvent);
        ev->setAccepted(dummyEvent.isAccepted());
        break;
    }
    case QEvent::GraphicsSceneMouseRelease: {
        QGraphicsSceneMouseEvent *gsEv = static_cast<QGraphicsSceneMouseEvent*>(ev);
        QMouseEvent dummyEvent(QEvent::MouseButtonRelease, gsEv->pos(), gsEv->screenPos(), gsEv->button(), gsEv->buttons(), gsEv->modifiers());
        d->adjustPointForClicking(&dummyEvent);
        d->mouseReleaseEvent(&dummyEvent);
        ev->setAccepted(dummyEvent.isAccepted());
        break;
    }
    case QEvent::GraphicsSceneMousePress: {
        QGraphicsSceneMouseEvent *gsEv = static_cast<QGraphicsSceneMouseEvent*>(ev);
        QMouseEvent dummyEvent(QEvent::MouseButtonPress, gsEv->pos(), gsEv->screenPos(), gsEv->button(), gsEv->buttons(), gsEv->modifiers());
        d->adjustPointForClicking(&dummyEvent);
        d->mousePressEvent(&dummyEvent);
        ev->setAccepted(dummyEvent.isAccepted());
        break;
    }
    case QEvent::GraphicsSceneMouseDoubleClick: {
        QGraphicsSceneMouseEvent *gsEv = static_cast<QGraphicsSceneMouseEvent*>(ev);
        QMouseEvent dummyEvent(QEvent::MouseButtonDblClick, gsEv->pos(), gsEv->screenPos(), gsEv->button(), gsEv->buttons(), gsEv->modifiers());
        d->adjustPointForClicking(&dummyEvent);
        d->mouseDoubleClickEvent(&dummyEvent);
        ev->setAccepted(dummyEvent.isAccepted());
        break;
    }
#endif
#ifndef QT_NO_CONTEXTMENU
    case QEvent::ContextMenu:
        d->contextMenuEvent(static_cast<QContextMenuEvent*>(ev)->globalPos());
        break;
#if !defined(QT_NO_GRAPHICSVIEW)
    case QEvent::GraphicsSceneContextMenu:
        d->contextMenuEvent(static_cast<QGraphicsSceneContextMenuEvent*>(ev)->screenPos());
        break;
#endif
#endif
#ifndef QT_NO_WHEELEVENT
    case QEvent::Wheel:
        d->wheelEvent(static_cast<QWheelEvent*>(ev), QApplication::wheelScrollLines());
        break;
#if !defined(QT_NO_GRAPHICSVIEW)
    case QEvent::GraphicsSceneWheel: {
        QGraphicsSceneWheelEvent *gsEv = static_cast<QGraphicsSceneWheelEvent*>(ev);
        QWheelEvent dummyEvent(gsEv->pos(), gsEv->screenPos(), gsEv->delta(), gsEv->buttons(), gsEv->modifiers(), gsEv->orientation());
        d->wheelEvent(&dummyEvent, QApplication::wheelScrollLines());
        ev->setAccepted(dummyEvent.isAccepted());
        break;
    }
#endif
#endif
    case QEvent::KeyPress:
        d->keyPressEvent(static_cast<QKeyEvent*>(ev));
        break;
    case QEvent::KeyRelease:
        d->keyReleaseEvent(static_cast<QKeyEvent*>(ev));
        break;
    case QEvent::FocusIn:
        d->focusInEvent(static_cast<QFocusEvent*>(ev));
        break;
    case QEvent::FocusOut:
        d->focusOutEvent(static_cast<QFocusEvent*>(ev));
        break;
#ifndef QT_NO_DRAGANDDROP
    case QEvent::DragEnter:
        d->dragEnterEvent(static_cast<QDragEnterEvent*>(ev));
        break;
    case QEvent::DragLeave:
        d->dragLeaveEvent();
        ev->accept();
        break;
    case QEvent::DragMove:
        d->dragMoveEvent(static_cast<QDragMoveEvent*>(ev));
        break;
    case QEvent::Drop:
        d->dropEvent(static_cast<QDropEvent*>(ev));
        break;
#if !defined(QT_NO_GRAPHICSVIEW)
    case QEvent::GraphicsSceneDragEnter:
        d->dragEnterEvent(static_cast<QGraphicsSceneDragDropEvent*>(ev));
        break;
    case QEvent::GraphicsSceneDragMove:
        d->dragMoveEvent(static_cast<QGraphicsSceneDragDropEvent*>(ev));
        break;
    case QEvent::GraphicsSceneDragLeave:
        d->dragLeaveEvent();
        ev->accept();
        break;
    case QEvent::GraphicsSceneDrop:
        d->dropEvent(static_cast<QGraphicsSceneDragDropEvent*>(ev));
        break;
#endif

#endif
    case QEvent::InputMethod:
        d->inputMethodEvent(static_cast<QInputMethodEvent*>(ev));
        break;
    case QEvent::ShortcutOverride:
        d->shortcutOverrideEvent(static_cast<QKeyEvent*>(ev));
        break;
    case QEvent::Leave:
        d->leaveEvent(ev);
        break;
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        // Return whether the default action was cancelled in the JS event handler
        return d->touchEvent(static_cast<QTouchEvent*>(ev));
#ifndef QT_NO_GESTURES
    case QEvent::Gesture:
        d->gestureEvent(static_cast<QGestureEvent*>(ev));
        break;
#endif
#ifndef QT_NO_PROPERTIES
    case QEvent::DynamicPropertyChange:
        d->dynamicPropertyChangeEvent(this, static_cast<QDynamicPropertyChangeEvent*>(ev));
        break;
#endif
    default:
        return QObject::event(ev);
    }

    return true;
}

/*!
    Similar to QWidget::focusNextPrevChild() it focuses the next focusable web element
    if \a next is true; otherwise the previous element is focused.

    Returns true if it can find a new focusable element, or false if it can't.
*/
bool QWebPage::focusNextPrevChild(bool next)
{
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_Tab, Qt::KeyboardModifiers(next ? Qt::NoModifier : Qt::ShiftModifier));
    d->keyPressEvent(&ev);
    return d->hasFocusedNode();
}

/*!
    \property QWebPage::contentEditable
    \brief whether the content in this QWebPage is editable or not
    \since 4.5

    If this property is enabled the contents of the page can be edited by the user through a visible
    cursor. If disabled (the default) only HTML elements in the web page with their
    \c{contenteditable} attribute set are editable.

    \sa modified, contentsChanged(), WebAction
*/
void QWebPage::setContentEditable(bool editable)
{
    if (isContentEditable() != editable) {
        d->setContentEditable(editable);
        d->updateEditorActions();
    }
}

bool QWebPage::isContentEditable() const
{
    return d->isContentEditable();
}

/*!
    \property QWebPage::forwardUnsupportedContent
    \brief whether QWebPage should forward unsupported content

    If enabled, the unsupportedContent() signal is emitted with a network reply that
    can be used to read the content.

    If disabled, the download of such content is aborted immediately.

    By default unsupported content is not forwarded.
*/

void QWebPage::setForwardUnsupportedContent(bool forward)
{
    d->forwardUnsupportedContent = forward;
}

bool QWebPage::forwardUnsupportedContent() const
{
    return d->forwardUnsupportedContent;
}

/*!
    \property QWebPage::linkDelegationPolicy
    \brief how QWebPage should delegate the handling of links through the
    linkClicked() signal

    The default is to delegate no links.
*/

void QWebPage::setLinkDelegationPolicy(LinkDelegationPolicy policy)
{
    d->linkPolicy = policy;
}

QWebPage::LinkDelegationPolicy QWebPage::linkDelegationPolicy() const
{
    return d->linkPolicy;
}

#ifndef QT_NO_CONTEXTMENU

bool QWebPagePrivate::handleScrollbarContextMenuEvent(QContextMenuEvent* event, bool horizontal, QWebPageAdapter::ScrollDirection* direction, QWebPageAdapter::ScrollGranularity* granularity)
{
    if (!QApplication::style()->styleHint(QStyle::SH_ScrollBar_ContextMenu))
        return false;

    QMenu menu;
    QAction* actScrollHere = menu.addAction(QCoreApplication::translate("QWebPage", "Scroll here"));
    menu.addSeparator();

    QAction* actScrollTop = menu.addAction(horizontal ? QCoreApplication::translate("QWebPage", "Left edge") : QCoreApplication::translate("QWebPage", "Top"));
    QAction* actScrollBottom = menu.addAction(horizontal ? QCoreApplication::translate("QWebPage", "Right edge") : QCoreApplication::translate("QWebPage", "Bottom"));
    menu.addSeparator();

    QAction* actPageUp = menu.addAction(horizontal ? QCoreApplication::translate("QWebPage", "Page left") : QCoreApplication::translate("QWebPage", "Page up"));
    QAction* actPageDown = menu.addAction(horizontal ? QCoreApplication::translate("QWebPage", "Page right") : QCoreApplication::translate("QWebPage", "Page down"));
    menu.addSeparator();

    QAction* actScrollUp = menu.addAction(horizontal ? QCoreApplication::translate("QWebPage", "Scroll left") : QCoreApplication::translate("QWebPage", "Scroll up"));
    QAction* actScrollDown = menu.addAction(horizontal ? QCoreApplication::translate("QWebPage", "Scroll right") : QCoreApplication::translate("QWebPage", "Scroll down"));

    QAction* actionSelected = menu.exec(event->globalPos());

    if (actionSelected == actScrollHere)
        return true;
    if (actionSelected == actScrollTop) {
        *direction = horizontal ? ScrollLeft : ScrollUp;
        *granularity = ScrollByDocument;
    } else if (actionSelected == actScrollBottom) {
        *direction =horizontal ? ScrollRight : ScrollDown;
        *granularity = ScrollByDocument;
    } else if (actionSelected == actPageUp) {
        *direction = horizontal ? ScrollLeft : ScrollUp;
        *granularity = ScrollByPage;
    } else if (actionSelected == actPageDown) {
        *direction =horizontal ? ScrollRight : ScrollDown;
        *granularity = ScrollByPage;
    } else if (actionSelected == actScrollUp) {
        *direction = horizontal ? ScrollLeft : ScrollUp;
        *granularity = ScrollByLine;
    } else if (actionSelected == actScrollDown) {
        *direction =horizontal ? ScrollRight : ScrollDown;
        *granularity = ScrollByLine;
    }
    return true;
}

/*!
    Filters the context menu event, \a event, through handlers for scrollbars and
    custom event handlers in the web page. Returns true if the event was handled;
    otherwise false.

    A web page may swallow a context menu event through a custom event handler, allowing for context
    menus to be implemented in HTML/JavaScript. This is used by \l{http://maps.google.com/}{Google
    Maps}, for example.
*/
bool QWebPage::swallowContextMenuEvent(QContextMenuEvent *event)
{
    QWebFrame* webFrame = frameAt(event->pos());
    return d->swallowContextMenuEvent(event, webFrame ? webFrame->d : 0);
}
#endif // QT_NO_CONTEXTMENU

/*!
    Updates the page's actions depending on the position \a pos. For example if \a pos is over an image
    element the CopyImageToClipboard action is enabled.
*/
void QWebPage::updatePositionDependentActions(const QPoint &pos)
{
#ifndef QT_NO_ACTION
    // First we disable all actions, but keep track of which ones were originally enabled.
    QBitArray originallyEnabledWebActions(QWebPage::WebActionCount);
    for (int i = QWebPageAdapter::NoAction + 1; i < QWebPageAdapter::ActionCount; ++i) {
        QWebPage::WebAction action = webActionForAdapterMenuAction(QWebPageAdapter::MenuAction(i));
        if (QAction *a = this->action(action)) {
            originallyEnabledWebActions.setBit(action, a->isEnabled());
            a->setEnabled(false);
        }
    }
#endif // QT_NO_ACTION

    QBitArray visitedWebActions(QWebPage::WebActionCount);
    d->createMainFrame();
    // Then we let updatePositionDependantMenuActions() enable the actions that are put into the menu
    QWebHitTestResultPrivate* result = d->updatePositionDependentMenuActions(pos, &visitedWebActions);
    if (!result)
        d->hitTestResult = QWebHitTestResult();
    else
        d->hitTestResult = QWebHitTestResult(result);

#ifndef QT_NO_ACTION
    // Finally, we restore the original enablement for the actions that were not put into the menu.
    originallyEnabledWebActions &= ~visitedWebActions; // Mask out visited actions (they're part of the menu)
    for (int i = 0; i < QWebPage::WebActionCount; ++i) {
        if (originallyEnabledWebActions.at(i)) {
            if (QAction *a = this->action(QWebPage::WebAction(i)))
                a->setEnabled(true);
        }
    }
#endif // QT_NO_ACTION

    // This whole process ensures that any actions put into to the context menu has the right
    // enablement, while also keeping the correct enablement for actions that were left out of
    // the menu.

}



/*!
    \enum QWebPage::Extension

    This enum describes the types of extensions that the page can support. Before using these extensions, you
    should verify that the extension is supported by calling supportsExtension().

    \value ChooseMultipleFilesExtension Whether the web page supports multiple file selection.
    This extension is invoked when the web content requests one or more file names, for example
    as a result of the user clicking on a "file upload" button in a HTML form where multiple
    file selection is allowed.

    \value ErrorPageExtension Whether the web page can provide an error page when loading fails.
    (introduced in Qt 4.6)

    \sa ChooseMultipleFilesExtensionOption, ChooseMultipleFilesExtensionReturn, ErrorPageExtensionOption, ErrorPageExtensionReturn
*/

/*!
    \enum QWebPage::ErrorDomain
    \since 4.6

    This enum describes the domain of an ErrorPageExtensionOption object (i.e. the layer in which the error occurred).

    \value QtNetwork The error occurred in the QtNetwork layer; the error code is of type QNetworkReply::NetworkError.
    \value Http The error occurred in the HTTP layer; the error code is a HTTP status code (see QNetworkRequest::HttpStatusCodeAttribute).
    \value WebKit The error is an internal WebKit error.
*/

/*!
    \class QWebPage::ExtensionOption
    \since 4.4
    \brief The ExtensionOption class provides an extended input argument to QWebPage's extension support.

    \inmodule QtWebKit

    \sa QWebPage::extension(), QWebPage::ExtensionReturn
*/


/*!
    \class QWebPage::ExtensionReturn
    \since 4.4
    \brief The ExtensionReturn class provides an output result from a QWebPage's extension.

    \inmodule QtWebKit

    \sa QWebPage::extension(), QWebPage::ExtensionOption
*/

/*!
    \class QWebPage::ErrorPageExtensionOption
    \since 4.6
    \brief The ErrorPageExtensionOption class describes the option
    for the error page extension.

    \inmodule QtWebKit

    The ErrorPageExtensionOption class holds the \a url for which an error occurred as well as
    the associated \a frame.

    The error itself is reported by an error \a domain, the \a error code as well as \a errorString.

    \sa QWebPage::extension(), QWebPage::ErrorPageExtensionReturn
*/

/*!
    \variable QWebPage::ErrorPageExtensionOption::url
    \brief the url for which an error occurred
*/

/*!
    \variable QWebPage::ErrorPageExtensionOption::frame
    \brief the frame associated with the error
*/

/*!
    \variable QWebPage::ErrorPageExtensionOption::domain
    \brief the domain that reported the error
*/

/*!
    \variable QWebPage::ErrorPageExtensionOption::error
    \brief the error code. Interpretation of the value depends on the \a domain
    \sa QWebPage::ErrorDomain
*/

/*!
    \variable QWebPage::ErrorPageExtensionOption::errorString
    \brief a string that describes the error
*/

/*!
    \class QWebPage::ErrorPageExtensionReturn
    \since 4.6
    \brief The ErrorPageExtensionReturn describes the error page, which will be shown for the
    frame for which the error occured.

    \inmodule QtWebKit

    The ErrorPageExtensionReturn class holds the data needed for creating an error page. Some are
    optional such as \a contentType, which defaults to "text/html", as well as the \a encoding, which
    is assumed to be UTF-8 if not indicated otherwise.

    The error page is stored in the \a content byte array, as HTML content. In order to convert a
    QString to a byte array, the QString::toUtf8() method can be used.

    External objects such as stylesheets or images referenced in the HTML are located relative to
    \a baseUrl.

    \sa QWebPage::extension(), QWebPage::ErrorPageExtensionOption, QString::toUtf8()
*/

/*!
    \fn QWebPage::ErrorPageExtensionReturn::ErrorPageExtensionReturn()

    Constructs a new error page object.
*/


/*!
    \variable QWebPage::ErrorPageExtensionReturn::contentType
    \brief the error page's content type
*/

/*!
    \variable QWebPage::ErrorPageExtensionReturn::encoding
    \brief the error page encoding
*/

/*!
    \variable QWebPage::ErrorPageExtensionReturn::baseUrl
    \brief the base url

    External objects such as stylesheets or images referenced in the HTML are located relative to this url.
*/

/*!
    \variable QWebPage::ErrorPageExtensionReturn::content
    \brief the HTML content of the error page
*/

/*!
    \class QWebPage::ChooseMultipleFilesExtensionOption
    \since 4.5
    \brief The ChooseMultipleFilesExtensionOption class describes the option
    for the multiple files selection extension.

    \inmodule QtWebKit

    The ChooseMultipleFilesExtensionOption class holds the frame originating the request
    and the suggested filenames which might be provided.

    \sa QWebPage::extension(), QWebPage::chooseFile(), QWebPage::ChooseMultipleFilesExtensionReturn
*/

/*!
    \variable QWebPage::ChooseMultipleFilesExtensionOption::parentFrame
    \brief The frame in which the request originated
*/

/*!
    \variable QWebPage::ChooseMultipleFilesExtensionOption::suggestedFileNames
    \brief The suggested filenames
*/

/*!
    \variable QWebPage::ChooseMultipleFilesExtensionReturn::fileNames
    \brief The selected filenames
*/

/*!
    \class QWebPage::ChooseMultipleFilesExtensionReturn
    \since 4.5
    \brief The ChooseMultipleFilesExtensionReturn describes the return value
    for the multiple files selection extension.

    \inmodule QtWebKit

    The ChooseMultipleFilesExtensionReturn class holds the filenames selected by the user
    when the extension is invoked.

    \sa QWebPage::extension(), QWebPage::ChooseMultipleFilesExtensionOption
*/

/*!
    This virtual function can be reimplemented in a QWebPage subclass to provide support for extensions. The \a option
    argument is provided as input to the extension; the output results can be stored in \a output.

    The behavior of this function is determined by \a extension. The \a option
    and \a output values are typically casted to the corresponding types (for
    example, ChooseMultipleFilesExtensionOption and
    ChooseMultipleFilesExtensionReturn for ChooseMultipleFilesExtension).

    You can call supportsExtension() to check if an extension is supported by the page.

    Returns true if the extension was called successfully; otherwise returns false.

    \sa supportsExtension(), Extension
*/
bool QWebPage::extension(Extension extension, const ExtensionOption *option, ExtensionReturn *output)
{
#ifndef QT_NO_FILEDIALOG
    if (extension == ChooseMultipleFilesExtension) {
        // FIXME: do not ignore suggestedFiles
        QStringList suggestedFiles = static_cast<const ChooseMultipleFilesExtensionOption*>(option)->suggestedFileNames;
        QStringList names = QFileDialog::getOpenFileNames(view(), QString::null);
        static_cast<ChooseMultipleFilesExtensionReturn*>(output)->fileNames = names;
        return true;
    }
#endif

    return false;
}

/*!
    This virtual function returns true if the web page supports \a extension; otherwise false is returned.

    \sa extension()
*/
bool QWebPage::supportsExtension(Extension extension) const
{
#ifndef QT_NO_FILEDIALOG
    return extension == ChooseMultipleFilesExtension;
#else
    Q_UNUSED(extension);
    return false;
#endif
}

/*!
 * \internal
 */
QWebPageAdapter *QWebPage::handle() const
{
    return d;
}

/*!
    Finds the specified string, \a subString, in the page, using the given \a options.

    If the HighlightAllOccurrences flag is passed, the function will highlight all occurrences
    that exist in the page. All subsequent calls will extend the highlight, rather than
    replace it, with occurrences of the new string.

    If the HighlightAllOccurrences flag is not passed, the function will select an occurrence
    and all subsequent calls will replace the current occurrence with the next one.

    To clear the selection, just pass an empty string.

    Returns true if \a subString was found; otherwise returns false.
*/
bool QWebPage::findText(const QString &subString, FindFlags options)
{
    return d->findText(subString, static_cast<QWebPageAdapter::FindFlag>(options.operator int()));
}

/*!
    Returns a pointer to the page's settings object.

    \sa QWebSettings::globalSettings()
*/
QWebSettings *QWebPage::settings() const
{
    return d->settings;
}

/*!
    This function is called when the web content requests a file name, for example
    as a result of the user clicking on a "file upload" button in a HTML form.

    A suggested filename may be provided in \a suggestedFile. The frame originating the
    request is provided as \a parentFrame.

    \sa ChooseMultipleFilesExtension
*/
QString QWebPage::chooseFile(QWebFrame *parentFrame, const QString& suggestedFile)
{
    Q_UNUSED(parentFrame);
#ifndef QT_NO_FILEDIALOG
    return QFileDialog::getOpenFileName(view(), QString::null, suggestedFile);
#else
    return QString::null;
#endif
}

/*!
    Sets the QNetworkAccessManager \a manager responsible for serving network requests for this
    QWebPage.

    \note It is currently not supported to change the network access manager after the
    QWebPage has used it. The results of doing this are undefined.

    \sa networkAccessManager()
*/
void QWebPage::setNetworkAccessManager(QNetworkAccessManager *manager)
{
    d->setNetworkAccessManager(manager);
}

/*!
    Returns the QNetworkAccessManager that is responsible for serving network
    requests for this QWebPage.

    \sa setNetworkAccessManager()
*/
QNetworkAccessManager *QWebPage::networkAccessManager() const
{
    return d->networkAccessManager();
}

/*!
    Sets the QWebPluginFactory \a factory responsible for creating plugins embedded into this
    QWebPage.

    Note: The plugin factory is only used if the QWebSettings::PluginsEnabled attribute is enabled.

    \sa pluginFactory()
*/
void QWebPage::setPluginFactory(QWebPluginFactory *factory)
{
    d->pluginFactory = factory;
}

/*!
    Returns the QWebPluginFactory that is responsible for creating plugins embedded into
    this QWebPage. If no plugin factory is installed a null pointer is returned.

    \sa setPluginFactory()
*/
QWebPluginFactory *QWebPage::pluginFactory() const
{
    return d->pluginFactory;
}

/*!
    This function is called when a user agent for HTTP requests is needed. You can reimplement this
    function to dynamically return different user agents for different URLs, based on the \a url parameter.

    The default implementation returns the following value:

    "Mozilla/5.0 (%Platform%%Security%%Subplatform%) AppleWebKit/%WebKitVersion% (KHTML, like Gecko) %AppVersion Safari/%WebKitVersion%"

    In this string the following values are replaced at run-time:
    \list
    \li %Platform% expands to the windowing system followed by "; " if it is not Windows (e.g. "X11; ").
    \li %Security% expands to "N; " if SSL is disabled.
    \li %Subplatform% expands to the operating system version (e.g. "Windows NT 6.1" or "Intel Mac OS X 10.5").
    \li %WebKitVersion% is the version of WebKit the application was compiled against.
    \li %AppVersion% expands to QCoreApplication::applicationName()/QCoreApplication::applicationVersion() if they're set; otherwise defaulting to Qt and the current Qt version.
    \endlist
*/
QString QWebPage::userAgentForUrl(const QUrl&) const
{
    return QWebPageAdapter::defaultUserAgentString();
}


/*!
    Returns the total number of bytes that were received from the network to render the current page,
    including extra content such as embedded images.

    \sa bytesReceived()
*/
quint64 QWebPage::totalBytes() const
{
    return d->m_totalBytes;
}


/*!
    Returns the number of bytes that were received from the network to render the current page.

    \sa totalBytes(), loadProgress()
*/
quint64 QWebPage::bytesReceived() const
{
    return d->m_bytesReceived;
}


/*!
    \property QWebPage::visibilityState
    \brief the page's visibility state

    This property should be changed by Qt applications who want to notify the JavaScript application
    that the visibility state has changed (e.g. by reimplementing QWidget::setVisible).
    The visibility state will be updated with the \a state parameter value only if it's different from the previous set.
    Then, HTML DOM Document Object attributes 'hidden' and 'visibilityState'
    will be updated to the correct value and a 'visiblitychange' event will be fired.
    More information about this HTML5 API can be found at \l{http://www.w3.org/TR/page-visibility/}{W3C Recommendation: Page Visibility}.

    By default, this property is set to VisibilityStateVisible.
*/
void QWebPage::setVisibilityState(VisibilityState state)
{
    d->setVisibilityState(static_cast<QWebPageAdapter::VisibilityState>(state));
}

QWebPage::VisibilityState QWebPage::visibilityState() const
{
    return static_cast<VisibilityState>(d->visibilityState());
}


/*!
    \since 4.8
    \fn void QWebPage::viewportChangeRequested()

    Page authors can provide the supplied values by using the viewport meta tag. More information
    about this can be found at \l{http://developer.apple.com/safari/library/documentation/appleapplications/reference/safariwebcontent/usingtheviewport/usingtheviewport.html}{Safari Reference Library: Using the Viewport Meta Tag}.

    \sa QWebPage::ViewportAttributes, setPreferredContentsSize(), QGraphicsWebView::setScale()
*/

/*!
    \fn void QWebPage::loadStarted()

    This signal is emitted when a page starts loading content.

    \sa loadFinished()
*/

/*!
    \fn void QWebPage::loadProgress(int progress)

    This signal is emitted when the global progress status changes.
    The current value is provided by \a progress and scales from 0 to 100,
    which is the default range of QProgressBar.
    It accumulates changes from all the child frames.

    \sa bytesReceived()
*/

/*!
    \fn void QWebPage::loadFinished(bool ok)

    This signal is emitted when the page finishes loading content. This signal
    is independant of script execution or page rendering.
    \a ok will indicate whether the load was successful or any error occurred.

    \sa loadStarted(), ErrorPageExtension
*/

/*!
    \fn void QWebPage::linkHovered(const QString &link, const QString &title, const QString &textContent)

    This signal is emitted when the mouse hovers over a link.

    \a link contains the link url.
    \a title is the link element's title, if it is specified in the markup.
    \a textContent provides text within the link element, e.g., text inside an HTML anchor tag.

    When the mouse leaves the link element the signal is emitted with empty parameters.

    \sa linkClicked()
*/

/*!
    \fn void QWebPage::statusBarMessage(const QString& text)

    This signal is emitted when the statusbar \a text is changed by the page.
*/

/*!
    \fn void QWebPage::frameCreated(QWebFrame *frame)

    This signal is emitted whenever the page creates a new \a frame.

    \sa currentFrame()
*/

/*!
    \fn void QWebPage::selectionChanged()

    This signal is emitted whenever the selection changes, either interactively
    or programmatically (e.g. by calling triggerAction() with a selection action).

    \sa selectedText()
*/

/*!
    \fn void QWebPage::contentsChanged()
    \since 4.5

    This signal is emitted whenever the text in form elements changes
    as well as other editable content.

    \sa contentEditable, modified, QWebFrame::toHtml(), QWebFrame::toPlainText()
*/

/*!
    \fn void QWebPage::geometryChangeRequested(const QRect& geom)

    This signal is emitted whenever the document wants to change the position and size of the
    page to \a geom. This can happen for example through JavaScript.
*/

/*!
    \fn void QWebPage::repaintRequested(const QRect& dirtyRect)

    This signal is emitted whenever this QWebPage should be updated. It's useful
    when rendering a QWebPage without a QWebView or QGraphicsWebView.
    \a dirtyRect contains the area that needs to be updated. To paint the QWebPage get
    the mainFrame() and call the render(QPainter*, const QRegion&) method with the
    \a dirtyRect as the second parameter.

    \sa mainFrame()
    \sa view()
*/

/*!
    \fn void QWebPage::scrollRequested(int dx, int dy, const QRect& rectToScroll)

    This signal is emitted whenever the content given by \a rectToScroll needs
    to be scrolled \a dx and \a dy downwards and no view was set.

    \sa view()
*/

/*!
    \fn void QWebPage::windowCloseRequested()

    This signal is emitted whenever the page requests the web browser window to be closed,
    for example through the JavaScript \c{window.close()} call.
*/

/*!
    \fn void QWebPage::printRequested(QWebFrame *frame)

    This signal is emitted whenever the page requests the web browser to print \a frame,
    for example through the JavaScript \c{window.print()} call.

    \sa QWebFrame::print(), QPrintPreviewDialog
*/

/*!
    \fn void QWebPage::unsupportedContent(QNetworkReply *reply)

    This signal is emitted when WebKit cannot handle a link the user navigated to or a
    web server's response includes a "Content-Disposition" header with the 'attachment' 
    directive. If "Content-Disposition" is present in \a reply, the web server is indicating
    that the client should prompt the user to save the content regardless of content-type. 
    See RFC 2616 sections 19.5.1 for details about Content-Disposition.

    At signal emission time the meta-data of the QNetworkReply \a reply is available.

    \note The receiving slot is responsible for deleting the QNetworkReply \a reply.

    \note This signal is only emitted if the forwardUnsupportedContent property is set to true.

    \sa downloadRequested()
*/

/*!
    \fn void QWebPage::downloadRequested(const QNetworkRequest &request)

    This signal is emitted when the user decides to download a link. The url of
    the link as well as additional meta-information is contained in \a request.

    \sa unsupportedContent()
*/

/*!
    \fn void QWebPage::microFocusChanged()

    This signal is emitted when for example the position of the cursor in an editable form
    element changes. It is used to inform input methods about the new on-screen position where
    the user is able to enter text. This signal is usually connected to the
    QWidget::updateMicroFocus() slot.
*/

/*!
    \fn void QWebPage::linkClicked(const QUrl &url)

    This signal is emitted whenever the user clicks on a link and the page's linkDelegationPolicy
    property is set to delegate the link handling for the specified \a url.

    By default no links are delegated and are handled by QWebPage instead.

    \note This signal possibly won't be emitted for clicked links which use
    JavaScript to trigger navigation.

    \sa linkHovered()
*/

/*!
    \fn void QWebPage::toolBarVisibilityChangeRequested(bool visible)

    This signal is emitted whenever the visibility of the toolbar in a web browser
    window that hosts QWebPage should be changed to \a visible.
*/

/*!
    \fn void QWebPage::statusBarVisibilityChangeRequested(bool visible)

    This signal is emitted whenever the visibility of the statusbar in a web browser
    window that hosts QWebPage should be changed to \a visible.
*/

/*!
    \fn void QWebPage::menuBarVisibilityChangeRequested(bool visible)

    This signal is emitted whenever the visibility of the menubar in a web browser
    window that hosts QWebPage should be changed to \a visible.
*/

/*!
    \fn void QWebPage::databaseQuotaExceeded(QWebFrame* frame, QString databaseName);
    \since 4.5

    This signal is emitted whenever the web site shown in \a frame is asking to store data
    to the database \a databaseName and the quota allocated to that web site is exceeded.

    \sa QWebDatabase
*/
/*!
    \fn void QWebPage::applicationCacheQuotaExceeded(QWebSecurityOrigin* origin, quint64 defaultOriginQuota, quint64 totalSpaceNeeded);

    This signal is emitted whenever the web site is asking to store data to the application cache
    database databaseName and the quota allocated to that web site is exceeded.

*/

/*!
  \since 4.5
  \fn void QWebPage::saveFrameStateRequested(QWebFrame* frame, QWebHistoryItem* item);

  This signal is emitted shortly before the history of navigated pages
  in \a frame is changed, for example when navigating back in the history.

  The provided QWebHistoryItem, \a item, holds the history entry of the frame before
  the change.

  A potential use-case for this signal is to store custom data in
  the QWebHistoryItem associated to the frame, using QWebHistoryItem::setUserData().
*/

/*!
  \since 4.5
  \fn void QWebPage::restoreFrameStateRequested(QWebFrame* frame);

  This signal is emitted when the load of \a frame is finished and the application may now update its state accordingly.
*/

#include "moc_qwebpage.cpp"
