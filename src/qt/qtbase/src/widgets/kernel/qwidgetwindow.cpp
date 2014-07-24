/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "private/qwindow_p.h"
#include "qwidgetwindow_qpa_p.h"

#include "private/qwidget_p.h"
#include "private/qapplication_p.h"
#ifndef QT_NO_ACCESSIBILITY
#include <QtGui/qaccessible.h>
#endif
#include <private/qwidgetbackingstore_p.h>
#include <qpa/qwindowsysteminterface_p.h>
#include <qpa/qplatformtheme.h>
#include <qpa/qplatformwindow.h>
#include <private/qgesturemanager_p.h>

QT_BEGIN_NAMESPACE

Q_WIDGETS_EXPORT extern bool qt_tab_all_widgets();

QWidget *qt_button_down = 0; // widget got last button-down
static QPointer<QWidget> qt_tablet_target = 0;

// popup control
QWidget *qt_popup_down = 0; // popup that contains the pressed widget
extern int openPopupCount;
bool qt_replay_popup_mouse_event = false;
extern bool qt_try_modal(QWidget *widget, QEvent::Type type);

class QWidgetWindowPrivate : public QWindowPrivate
{
    Q_DECLARE_PUBLIC(QWidgetWindow)
public:
    QWindow *eventReceiver() {
        Q_Q(QWidgetWindow);
        QWindow *w = q;
        while (w->parent() && qobject_cast<QWidgetWindow *>(w) && qobject_cast<QWidgetWindow *>(w->parent())) {
            w = w->parent();
        }
        return w;
    }

    void clearFocusObject()
    {
        if (QApplicationPrivate::focus_widget)
            QApplicationPrivate::focus_widget->clearFocus();
    }

};

QWidgetWindow::QWidgetWindow(QWidget *widget)
    : QWindow(*new QWidgetWindowPrivate(), 0)
    , m_widget(widget)
{
    updateObjectName();
    // Enable QOpenGLWidget/QQuickWidget children if the platform plugin supports it,
    // and the application developer has not explicitly disabled it.
    if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::RasterGLSurface)
        && !QApplication::testAttribute(Qt::AA_ForceRasterWidgets)) {
        setSurfaceType(QSurface::RasterGLSurface);
    }
    connect(m_widget, &QObject::objectNameChanged, this, &QWidgetWindow::updateObjectName);
    connect(this, SIGNAL(screenChanged(QScreen*)), this, SLOT(repaintWindow()));
}

QWidgetWindow::~QWidgetWindow()
{
}

#ifndef QT_NO_ACCESSIBILITY
QAccessibleInterface *QWidgetWindow::accessibleRoot() const
{
    if (m_widget)
        return QAccessible::queryAccessibleInterface(m_widget);
    return 0;
}
#endif

QObject *QWidgetWindow::focusObject() const
{
    QWidget *widget = m_widget->focusWidget();

    if (!widget)
        widget = m_widget;

    return widget;
}

bool QWidgetWindow::event(QEvent *event)
{
    if (m_widget->testAttribute(Qt::WA_DontShowOnScreen)) {
        // \a event is uninteresting for QWidgetWindow, the event was probably
        // generated before WA_DontShowOnScreen was set
        return m_widget->event(event);
    }

    switch (event->type()) {
    case QEvent::Close:
        handleCloseEvent(static_cast<QCloseEvent *>(event));
        return true;

    case QEvent::Enter:
    case QEvent::Leave:
        handleEnterLeaveEvent(event);
        return true;

    // these should not be sent to QWidget, the corresponding events
    // are sent by QApplicationPrivate::notifyActiveWindowChange()
    case QEvent::FocusIn:
        handleFocusInEvent(static_cast<QFocusEvent *>(event));
        // Fallthrough
    case QEvent::FocusOut: {
#ifndef QT_NO_ACCESSIBILITY
        QAccessible::State state;
        state.active = true;
        QAccessibleStateChangeEvent ev(widget(), state);
        QAccessible::updateAccessibility(&ev);
#endif
        return false; }

    case QEvent::FocusAboutToChange:
        if (QApplicationPrivate::focus_widget) {
            if (QApplicationPrivate::focus_widget->testAttribute(Qt::WA_InputMethodEnabled))
                qApp->inputMethod()->commit();

            QGuiApplication::sendSpontaneousEvent(QApplicationPrivate::focus_widget, event);
        }
        return true;

    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::ShortcutOverride:
        handleKeyEvent(static_cast<QKeyEvent *>(event));
        return true;

    case QEvent::MouseMove:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
        handleMouseEvent(static_cast<QMouseEvent *>(event));
        return true;

    case QEvent::NonClientAreaMouseMove:
    case QEvent::NonClientAreaMouseButtonPress:
    case QEvent::NonClientAreaMouseButtonRelease:
    case QEvent::NonClientAreaMouseButtonDblClick:
        handleNonClientAreaMouseEvent(static_cast<QMouseEvent *>(event));
        return true;

    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        handleTouchEvent(static_cast<QTouchEvent *>(event));
        return true;

    case QEvent::Move:
        handleMoveEvent(static_cast<QMoveEvent *>(event));
        return true;

    case QEvent::Resize:
        handleResizeEvent(static_cast<QResizeEvent *>(event));
        return true;

#ifndef QT_NO_WHEELEVENT
    case QEvent::Wheel:
        handleWheelEvent(static_cast<QWheelEvent *>(event));
        return true;
#endif

#ifndef QT_NO_DRAGANDDROP
    case QEvent::DragEnter:
    case QEvent::DragMove:
        handleDragEnterMoveEvent(static_cast<QDragMoveEvent *>(event));
        return true;
    case QEvent::DragLeave:
        handleDragLeaveEvent(static_cast<QDragLeaveEvent *>(event));
        return true;
    case QEvent::Drop:
        handleDropEvent(static_cast<QDropEvent *>(event));
        return true;
#endif

    case QEvent::Expose:
        handleExposeEvent(static_cast<QExposeEvent *>(event));
        return true;

    case QEvent::WindowStateChange:
        handleWindowStateChangedEvent(static_cast<QWindowStateChangeEvent *>(event));
        return true;

    case QEvent::ThemeChange: {
        QEvent widgetEvent(QEvent::ThemeChange);
        QGuiApplication::sendSpontaneousEvent(m_widget, &widgetEvent);
    }
        return true;

#ifndef QT_NO_TABLETEVENT
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease:
        handleTabletEvent(static_cast<QTabletEvent *>(event));
        return true;
#endif

#ifndef QT_NO_GESTURES
    case QEvent::NativeGesture:
        handleGestureEvent(static_cast<QNativeGestureEvent *>(event));
        return true;
#endif

#ifndef QT_NO_CONTEXTMENU
    case QEvent::ContextMenu:
        handleContextMenuEvent(static_cast<QContextMenuEvent *>(event));
        return true;
#endif

    // Handing show events to widgets (see below) here would cause them to be triggered twice
    case QEvent::Show:
    case QEvent::Hide:
        return QWindow::event(event);
    case QEvent::WindowBlocked:
        qt_button_down = 0;
        break;
    default:
        break;
    }

    return m_widget->event(event) || QWindow::event(event);
}

QPointer<QWidget> qt_last_mouse_receiver = 0;

void QWidgetWindow::handleEnterLeaveEvent(QEvent *event)
{
    if (event->type() == QEvent::Leave) {
        QWidget *enter = 0;
        // Check from window system event queue if the next queued enter targets a window
        // in the same window hierarchy (e.g. enter a child of this window). If so,
        // remove the enter event from queue and handle both in single dispatch.
        QWindowSystemInterfacePrivate::EnterEvent *systemEvent =
            static_cast<QWindowSystemInterfacePrivate::EnterEvent *>
            (QWindowSystemInterfacePrivate::peekWindowSystemEvent(QWindowSystemInterfacePrivate::Enter));
        const QPointF globalPosF = systemEvent ? systemEvent->globalPos : QGuiApplicationPrivate::lastCursorPosition;
        if (systemEvent) {
            if (QWidgetWindow *enterWindow = qobject_cast<QWidgetWindow *>(systemEvent->enter))
            {
                QWindow *thisParent = this;
                QWindow *enterParent = enterWindow;
                while (thisParent->parent())
                    thisParent = thisParent->parent();
                while (enterParent->parent())
                    enterParent = enterParent->parent();
                if (thisParent == enterParent) {
                    QGuiApplicationPrivate::currentMouseWindow = enterWindow;
                    enter = enterWindow->widget();
                    QWindowSystemInterfacePrivate::removeWindowSystemEvent(systemEvent);
                }
            }
        }
        // Enter-leave between sibling widgets is ignored when there is a mousegrabber - this makes
        // both native and non-native widgets work similarly.
        // When mousegrabbing, leaves are only generated if leaving the parent window.
        if (!enter || !QWidget::mouseGrabber()) {
            // Preferred leave target is the last mouse receiver, unless it has native window,
            // in which case it is assumed to receive it's own leave event when relevant.
            QWidget *leave = m_widget;
            if (qt_last_mouse_receiver && !qt_last_mouse_receiver->internalWinId())
                leave = qt_last_mouse_receiver.data();
            QApplicationPrivate::dispatchEnterLeave(enter, leave, globalPosF);
            qt_last_mouse_receiver = enter;
        }
    } else {
        const QEnterEvent *ee = static_cast<QEnterEvent *>(event);
        QWidget *child = m_widget->childAt(ee->pos());
        QWidget *receiver = child ? child : m_widget;
        QApplicationPrivate::dispatchEnterLeave(receiver, 0, ee->screenPos());
        qt_last_mouse_receiver = receiver;
    }
}

QWidget *QWidgetWindow::getFocusWidget(FocusWidgets fw)
{
    QWidget *tlw = m_widget;
    QWidget *w = tlw->nextInFocusChain();

    QWidget *last = tlw;

    uint focus_flag = qt_tab_all_widgets() ? Qt::TabFocus : Qt::StrongFocus;

    while (w != tlw)
    {
        if (((w->focusPolicy() & focus_flag) == focus_flag)
            && w->isVisibleTo(m_widget) && w->isEnabled())
        {
            last = w;
            if (fw == FirstFocusWidget)
                break;
        }
        w = w->nextInFocusChain();
    }

    return last;
}

void QWidgetWindow::handleFocusInEvent(QFocusEvent *e)
{
    QWidget *focusWidget = 0;
    if (e->reason() == Qt::BacktabFocusReason)
        focusWidget = getFocusWidget(LastFocusWidget);
    else if (e->reason() == Qt::TabFocusReason)
        focusWidget = getFocusWidget(FirstFocusWidget);

    if (focusWidget != 0)
        focusWidget->setFocus();
}

void QWidgetWindow::handleNonClientAreaMouseEvent(QMouseEvent *e)
{
    QApplication::sendSpontaneousEvent(m_widget, e);
}

void QWidgetWindow::handleMouseEvent(QMouseEvent *event)
{
    static const QEvent::Type contextMenuTrigger =
        QGuiApplicationPrivate::platformTheme()->themeHint(QPlatformTheme::ContextMenuOnMouseRelease).toBool() ?
        QEvent::MouseButtonRelease : QEvent::MouseButtonPress;
    if (qApp->d_func()->inPopupMode()) {
        QWidget *activePopupWidget = qApp->activePopupWidget();
        QWidget *popup = activePopupWidget;
        QPoint mapped = event->pos();
        if (popup != m_widget)
            mapped = popup->mapFromGlobal(event->globalPos());
        bool releaseAfter = false;
        QWidget *popupChild  = popup->childAt(mapped);

        if (popup != qt_popup_down) {
            qt_button_down = 0;
            qt_popup_down = 0;
        }

        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
            qt_button_down = popupChild;
            qt_popup_down = popup;
            break;
        case QEvent::MouseButtonRelease:
            releaseAfter = true;
            break;
        default:
            break; // nothing for mouse move
        }

        int oldOpenPopupCount = openPopupCount;

        if (popup->isEnabled()) {
            // deliver event
            qt_replay_popup_mouse_event = false;
            QWidget *receiver = popup;
            QPoint widgetPos = mapped;
            if (qt_button_down)
                receiver = qt_button_down;
            else if (popupChild)
                receiver = popupChild;
            if (receiver != popup)
                widgetPos = receiver->mapFromGlobal(event->globalPos());
            QWidget *alien = m_widget->childAt(m_widget->mapFromGlobal(event->globalPos()));
            QMouseEvent e(event->type(), widgetPos, event->windowPos(), event->screenPos(), event->button(), event->buttons(), event->modifiers());
            QGuiApplicationPrivate::setMouseEventSource(&e, QGuiApplicationPrivate::mouseEventSource(event));
            e.setTimestamp(event->timestamp());
            QApplicationPrivate::sendMouseEvent(receiver, &e, alien, m_widget, &qt_button_down, qt_last_mouse_receiver);
        } else {
            // close disabled popups when a mouse button is pressed or released
            switch (event->type()) {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonDblClick:
            case QEvent::MouseButtonRelease:
                popup->close();
                break;
            default:
                break;
            }
        }

        if (qApp->activePopupWidget() != activePopupWidget
            && qt_replay_popup_mouse_event) {
            if (m_widget->windowType() != Qt::Popup)
                qt_button_down = 0;
            if (event->type() == QEvent::MouseButtonPress) {
                // the popup disappeared, replay the mouse press event
                QWidget *w = QApplication::widgetAt(event->globalPos());
                if (w && !QApplicationPrivate::isBlockedByModal(w)) {
                    // activate window of the widget under mouse pointer
                    if (!w->isActiveWindow()) {
                        w->activateWindow();
                        w->raise();
                    }

                    QWindow *win = w->windowHandle();
                    if (!win)
                        win = w->nativeParentWidget()->windowHandle();
                    if (win && win->geometry().contains(event->globalPos())) {
                        const QPoint localPos = win->mapFromGlobal(event->globalPos());
                        QMouseEvent e(QEvent::MouseButtonPress, localPos, localPos, event->globalPos(), event->button(), event->buttons(), event->modifiers());
                        QGuiApplicationPrivate::setMouseEventSource(&e, QGuiApplicationPrivate::mouseEventSource(event));
                        e.setTimestamp(event->timestamp());
                        QApplication::sendSpontaneousEvent(win, &e);
                    }
                }
            }
            qt_replay_popup_mouse_event = false;
#ifndef QT_NO_CONTEXTMENU
        } else if (event->type() == contextMenuTrigger
                   && event->button() == Qt::RightButton
                   && (openPopupCount == oldOpenPopupCount)) {
            QWidget *popupEvent = popup;
            if (qt_button_down)
                popupEvent = qt_button_down;
            else if(popupChild)
                popupEvent = popupChild;
            QContextMenuEvent e(QContextMenuEvent::Mouse, mapped, event->globalPos(), event->modifiers());
            QApplication::sendSpontaneousEvent(popupEvent, &e);
#endif
        }

        if (releaseAfter) {
            qt_button_down = 0;
            qt_popup_down = 0;
        }
        return;
    }

    // modal event handling
    if (QApplicationPrivate::instance()->modalState() && !qt_try_modal(m_widget, event->type()))
        return;

    // which child should have it?
    QWidget *widget = m_widget->childAt(event->pos());
    QPoint mapped = event->pos();

    if (!widget)
        widget = m_widget;

    if (event->type() == QEvent::MouseButtonPress)
        qt_button_down = widget;

    QWidget *receiver = QApplicationPrivate::pickMouseReceiver(m_widget, event->windowPos().toPoint(), &mapped, event->type(), event->buttons(),
                                                               qt_button_down, widget);

    if (!receiver) {
        if (event->type() == QEvent::MouseButtonRelease)
            QApplicationPrivate::mouse_buttons &= ~event->button();
        return;
    }
    if ((event->type() != QEvent::MouseButtonPress)
        || !(event->flags().testFlag(Qt::MouseEventCreatedDoubleClick))) {

        // The preceding statement excludes MouseButtonPress events which caused
        // creation of a MouseButtonDblClick event. QTBUG-25831
        QMouseEvent translated(event->type(), mapped, event->windowPos(), event->screenPos(),
                               event->button(), event->buttons(), event->modifiers());
        QGuiApplicationPrivate::setMouseEventSource(&translated, QGuiApplicationPrivate::mouseEventSource(event));
        translated.setTimestamp(event->timestamp());
        QApplicationPrivate::sendMouseEvent(receiver, &translated, widget, m_widget,
                                            &qt_button_down, qt_last_mouse_receiver);
    }
#ifndef QT_NO_CONTEXTMENU
    if (event->type() == contextMenuTrigger && event->button() == Qt::RightButton) {
        QContextMenuEvent e(QContextMenuEvent::Mouse, mapped, event->globalPos(), event->modifiers());
        QGuiApplication::sendSpontaneousEvent(receiver, &e);
    }
#endif
}

void QWidgetWindow::handleTouchEvent(QTouchEvent *event)
{
    if (event->type() == QEvent::TouchCancel) {
        QApplicationPrivate::translateTouchCancel(event->device(), event->timestamp());
        event->accept();
    } else if (qApp->d_func()->inPopupMode()) {
        // Ignore touch events for popups. This will cause QGuiApplication to synthesise mouse
        // events instead, which QWidgetWindow::handleMouseEvent will forward correctly:
        event->ignore();
    } else {
        event->setAccepted(QApplicationPrivate::translateRawTouchEvent(m_widget, event->device(), event->touchPoints(), event->timestamp()));
    }
}

void QWidgetWindow::handleKeyEvent(QKeyEvent *event)
{
    if (QApplicationPrivate::instance()->modalState() && !qt_try_modal(m_widget, event->type()))
        return;

    QObject *receiver = QWidget::keyboardGrabber();
    if (!receiver && QApplicationPrivate::inPopupMode()) {
        QWidget *popup = QApplication::activePopupWidget();
        QWidget *popupFocusWidget = popup->focusWidget();
        receiver = popupFocusWidget ? popupFocusWidget : popup;
    }
    if (!receiver)
        receiver = focusObject();
    QGuiApplication::sendSpontaneousEvent(receiver, event);
}

void QWidgetWindow::updateGeometry()
{
    if (m_widget->testAttribute(Qt::WA_OutsideWSRange))
        return;

    const QMargins margins = frameMargins();

    m_widget->data->crect = geometry();
    QTLWExtra *te = m_widget->d_func()->topData();
    te->posIncludesFrame= false;
    te->frameStrut.setCoords(margins.left(), margins.top(), margins.right(), margins.bottom());
    m_widget->data->fstrut_dirty = false;
}

// Invalidates the backing store buffer and repaints immediately.
// ### Qt 5.4: replace with QUpdateWindowRequestEvent.
void QWidgetWindow::repaintWindow()
{
    if (!m_widget->isVisible() || !m_widget->updatesEnabled())
        return;

    QTLWExtra *tlwExtra = m_widget->window()->d_func()->maybeTopData();
    if (tlwExtra && !tlwExtra->inTopLevelResize && tlwExtra->backingStore)
        tlwExtra->backingStoreTracker->markDirty(m_widget->rect(), m_widget,
                                                 QWidgetBackingStore::UpdateNow, QWidgetBackingStore::BufferInvalid);
}

Qt::WindowState effectiveState(Qt::WindowStates state);

// Store normal geometry used for saving application settings.
void QWidgetWindow::updateNormalGeometry()
{
    QTLWExtra *tle = m_widget->d_func()->maybeTopData();
    if (!tle)
        return;
     // Ask platform window, default to widget geometry.
    QRect normalGeometry;
    if (const QPlatformWindow *pw = handle())
        normalGeometry = pw->normalGeometry();
    if (!normalGeometry.isValid() && effectiveState(m_widget->windowState()) == Qt::WindowNoState)
        normalGeometry = m_widget->geometry();
    if (normalGeometry.isValid())
        tle->normalGeometry = normalGeometry;
}

void QWidgetWindow::handleMoveEvent(QMoveEvent *event)
{
    updateGeometry();
    QGuiApplication::sendSpontaneousEvent(m_widget, event);
}

void QWidgetWindow::handleResizeEvent(QResizeEvent *event)
{
    QSize oldSize = m_widget->data->crect.size();

    updateGeometry();
    QGuiApplication::sendSpontaneousEvent(m_widget, event);

    if (m_widget->d_func()->paintOnScreen()) {
        QRegion updateRegion(geometry());
        if (m_widget->testAttribute(Qt::WA_StaticContents))
            updateRegion -= QRect(0, 0, oldSize.width(), oldSize.height());
        m_widget->d_func()->syncBackingStore(updateRegion);
    } else {
        m_widget->d_func()->syncBackingStore();
    }
}

void QWidgetWindow::handleCloseEvent(QCloseEvent *event)
{
    bool is_closing = m_widget->d_func()->close_helper(QWidgetPrivate::CloseWithSpontaneousEvent);
    event->setAccepted(is_closing);
}

#ifndef QT_NO_WHEELEVENT

void QWidgetWindow::handleWheelEvent(QWheelEvent *event)
{
    if (QApplicationPrivate::instance()->modalState() && !qt_try_modal(m_widget, event->type()))
        return;

    // which child should have it?
    QWidget *widget = m_widget->childAt(event->pos());

    if (!widget)
        widget = m_widget;

    QPoint mapped = widget->mapFrom(m_widget, event->pos());

    QWheelEvent translated(mapped, event->globalPos(), event->pixelDelta(), event->angleDelta(), event->delta(), event->orientation(), event->buttons(), event->modifiers(), event->phase());
    QGuiApplication::sendSpontaneousEvent(widget, &translated);
}

#endif // QT_NO_WHEELEVENT

#ifndef QT_NO_DRAGANDDROP

void QWidgetWindow::handleDragEnterMoveEvent(QDragMoveEvent *event)
{
     Q_ASSERT(event->type() ==QEvent::DragMove || !m_dragTarget);
    // Find a target widget under mouse that accepts drops (QTBUG-22987).
    QWidget *widget = m_widget->childAt(event->pos());
    if (!widget)
        widget = m_widget;
    for ( ; widget && !widget->isWindow() && !widget->acceptDrops(); widget = widget->parentWidget()) ;
    if (widget && !widget->acceptDrops())
        widget = 0;
    // Target widget unchanged: DragMove
    if (widget && widget == m_dragTarget.data()) {
        Q_ASSERT(event->type() == QEvent::DragMove);
        const QPoint mapped = widget->mapFromGlobal(m_widget->mapToGlobal(event->pos()));
        QDragMoveEvent translated(mapped, event->possibleActions(), event->mimeData(), event->mouseButtons(), event->keyboardModifiers());
        translated.setDropAction(event->dropAction());
        if (event->isAccepted()) { // Handling 'DragEnter' should suffice for the application.
            translated.accept();
            translated.setDropAction(event->dropAction());
        }
        QGuiApplication::sendSpontaneousEvent(widget, &translated);
        if (translated.isAccepted()) {
            event->accept();
        } else {
            event->ignore();
        }
        event->setDropAction(translated.dropAction());
        return;
    }
    // Target widget changed: Send DragLeave to previous, DragEnter to new if there is any
    if (m_dragTarget.data()) {
        QDragLeaveEvent le;
        QGuiApplication::sendSpontaneousEvent(m_dragTarget.data(), &le);
        m_dragTarget = 0;
    }
    if (!widget) {
         event->ignore();
         return;
    }
    m_dragTarget = widget;
    const QPoint mapped = widget->mapFromGlobal(m_widget->mapToGlobal(event->pos()));
    QDragEnterEvent translated(mapped, event->possibleActions(), event->mimeData(), event->mouseButtons(), event->keyboardModifiers());
    QGuiApplication::sendSpontaneousEvent(widget, &translated);
    if (translated.isAccepted()) {
        event->accept();
    } else {
        event->ignore();
    }
    event->setDropAction(translated.dropAction());
}

void QWidgetWindow::handleDragLeaveEvent(QDragLeaveEvent *event)
{
    if (m_dragTarget)
        QGuiApplication::sendSpontaneousEvent(m_dragTarget.data(), event);
    m_dragTarget = 0;
}

void QWidgetWindow::handleDropEvent(QDropEvent *event)
{
    if (m_dragTarget.isNull()) {
        qWarning() << Q_FUNC_INFO << m_widget << ": No drag target set.";
        event->ignore();
        return;
    }
    const QPoint mapped = m_dragTarget.data()->mapFromGlobal(m_widget->mapToGlobal(event->pos()));
    QDropEvent translated(mapped, event->possibleActions(), event->mimeData(), event->mouseButtons(), event->keyboardModifiers());
    QGuiApplication::sendSpontaneousEvent(m_dragTarget.data(), &translated);
    if (translated.isAccepted())
        event->accept();
    event->setDropAction(translated.dropAction());
    m_dragTarget = 0;
}

#endif // QT_NO_DRAGANDDROP

void QWidgetWindow::handleExposeEvent(QExposeEvent *event)
{
    if (isExposed()) {
        m_widget->setAttribute(Qt::WA_Mapped);
        if (!event->region().isNull()) {
            // Exposed native widgets need to be marked dirty to get them repainted correctly.
            if (m_widget->internalWinId() && !m_widget->isWindow() && m_widget->isVisible() && m_widget->updatesEnabled()) {
                if (QWidgetBackingStore *bs = m_widget->d_func()->maybeBackingStore())
                    bs->markDirty(event->region(), m_widget);
            }
            m_widget->d_func()->syncBackingStore(event->region());
        }
    } else {
        m_widget->setAttribute(Qt::WA_Mapped, false);
    }
}

void QWidgetWindow::handleWindowStateChangedEvent(QWindowStateChangeEvent *event)
{
    // QWindow does currently not know 'active'.
    Qt::WindowStates eventState = event->oldState();
    Qt::WindowStates widgetState = m_widget->windowState();
    if (widgetState & Qt::WindowActive)
        eventState |= Qt::WindowActive;

    // Determine the new widget state, remember maximized/full screen
    // during minimized.
    switch (windowState()) {
    case Qt::WindowNoState:
        widgetState &= ~(Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowFullScreen);
        break;
    case Qt::WindowMinimized:
        widgetState |= Qt::WindowMinimized;
        break;
    case Qt::WindowMaximized:
        updateNormalGeometry();
        widgetState |= Qt::WindowMaximized;
        widgetState &= ~(Qt::WindowMinimized | Qt::WindowFullScreen);
        break;
    case Qt::WindowFullScreen:
        updateNormalGeometry();
        widgetState |= Qt::WindowFullScreen;
        widgetState &= ~(Qt::WindowMinimized);
        break;
    case Qt::WindowActive: // Not handled by QWindow
        break;
    }

    // Sent event if the state changed (that is, it is not triggered by
    // QWidget::setWindowState(), which also sends an event to the widget).
    if (widgetState != int(m_widget->data->window_state)) {
        m_widget->data->window_state = widgetState;
        QWindowStateChangeEvent widgetEvent(eventState);
        QGuiApplication::sendSpontaneousEvent(m_widget, &widgetEvent);
    }
}

bool QWidgetWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    return m_widget->nativeEvent(eventType, message, result);
}

#ifndef QT_NO_TABLETEVENT
void QWidgetWindow::handleTabletEvent(QTabletEvent *event)
{
    if (event->type() == QEvent::TabletPress) {
        QWidget *widget = m_widget->childAt(event->pos());
        if (!widget)
            widget = m_widget;

        qt_tablet_target = widget;
    }

    if (qt_tablet_target) {
        QPointF delta = event->globalPosF() - event->globalPos();
        QPointF mapped = qt_tablet_target->mapFromGlobal(event->globalPos()) + delta;
        QTabletEvent ev(event->type(), mapped, event->globalPosF(), event->device(), event->pointerType(),
                        event->pressure(), event->xTilt(), event->yTilt(), event->tangentialPressure(),
                        event->rotation(), event->z(), event->modifiers(), event->uniqueId());
        ev.setTimestamp(event->timestamp());
        QGuiApplication::sendSpontaneousEvent(qt_tablet_target, &ev);
    }

    if (event->type() == QEvent::TabletRelease)
        qt_tablet_target = 0;
}
#endif // QT_NO_TABLETEVENT

#ifndef QT_NO_GESTURES
void QWidgetWindow::handleGestureEvent(QNativeGestureEvent *e)
{
    // copy-pasted code to find correct widget follows:
    QObject *receiver = 0;
    if (QApplicationPrivate::inPopupMode()) {
        QWidget *popup = QApplication::activePopupWidget();
        QWidget *popupFocusWidget = popup->focusWidget();
        receiver = popupFocusWidget ? popupFocusWidget : popup;
    }
    if (!receiver)
        receiver = QApplication::widgetAt(e->globalPos());
    if (!receiver)
        receiver = m_widget; // last resort

    QApplication::sendSpontaneousEvent(receiver, e);
}
#endif // QT_NO_GESTURES

#ifndef QT_NO_CONTEXTMENU
void QWidgetWindow::handleContextMenuEvent(QContextMenuEvent *e)
{
    // We are only interested in keyboard originating context menu events here,
    // mouse originated context menu events for widgets are generated in mouse handling methods.
    if (e->reason() != QContextMenuEvent::Keyboard)
        return;

    QWidget *fw = QWidget::keyboardGrabber();
    if (!fw) {
        if (QApplication::activePopupWidget()) {
            fw = (QApplication::activePopupWidget()->focusWidget()
                  ? QApplication::activePopupWidget()->focusWidget()
                  : QApplication::activePopupWidget());
        } else if (QApplication::focusWidget()) {
            fw = QApplication::focusWidget();
        } else {
            fw = m_widget;
        }
    }
    if (fw && fw->isEnabled()) {
        QPoint pos = fw->inputMethodQuery(Qt::ImMicroFocus).toRect().center();
        QContextMenuEvent widgetEvent(QContextMenuEvent::Keyboard, pos, fw->mapToGlobal(pos),
                                      e->modifiers());
        QGuiApplication::sendSpontaneousEvent(fw, &widgetEvent);
    }
}
#endif // QT_NO_CONTEXTMENU

void QWidgetWindow::updateObjectName()
{
    QString name = m_widget->objectName();
    if (name.isEmpty())
        name = QString::fromUtf8(m_widget->metaObject()->className()) + QStringLiteral("Class");
    name += QStringLiteral("Window");
    setObjectName(name);
}

QT_END_NAMESPACE
