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

#include "QtWidgets/qwidget.h"
#include "QtGui/qevent.h"
#include "QtWidgets/qapplication.h"
#include "private/qwidgetbackingstore_p.h"
#include "private/qwidget_p.h"
#include "private/qwidgetwindow_qpa_p.h"
#include "private/qapplication_p.h"
#include "QtWidgets/qdesktopwidget.h"
#include <qpa/qplatformwindow.h>
#include "QtGui/qsurfaceformat.h"
#include <QtGui/qopenglcontext.h>
#include <qpa/qplatformopenglcontext.h>
#include <qpa/qplatformintegration.h>
#include "QtGui/private/qwindow_p.h"
#include "QtGui/private/qguiapplication_p.h"
#include <private/qwindowcontainer_p.h>

#include <qpa/qplatformcursor.h>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtCore/QMargins>

QT_BEGIN_NAMESPACE

void q_createNativeChildrenAndSetParent(const QWidget *parentWidget)
{
    QObjectList children = parentWidget->children();
    for (int i = 0; i < children.size(); i++) {
        if (children.at(i)->isWidgetType()) {
            const QWidget *childWidget = qobject_cast<const QWidget *>(children.at(i));
            if (childWidget) { // should not be necessary
                if (childWidget->testAttribute(Qt::WA_NativeWindow)) {
                    if (!childWidget->internalWinId())
                        childWidget->winId();
                    if (childWidget->windowHandle()) {
                        QWindow *parentWindow = childWidget->nativeParentWidget()->windowHandle();
                        if (childWidget->isWindow())
                            childWidget->windowHandle()->setTransientParent(parentWindow);
                        else
                            childWidget->windowHandle()->setParent(parentWindow);
                    }
                } else {
                    q_createNativeChildrenAndSetParent(childWidget);
                }
            }
        }
    }

}

void QWidgetPrivate::create_sys(WId window, bool initializeWindow, bool destroyOldWindow)
{
    Q_Q(QWidget);

    Q_UNUSED(window);
    Q_UNUSED(initializeWindow);
    Q_UNUSED(destroyOldWindow);

    Qt::WindowFlags flags = data.window_flags;

    if (!q->testAttribute(Qt::WA_NativeWindow) && !q->isWindow())
        return; // we only care about real toplevels

    QWindow *win = topData()->window;
    // topData() ensures the extra is created but does not ensure 'window' is non-null
    // in case the extra was already valid.
    if (!win) {
        createTLSysExtra();
        win = topData()->window;
    }

    foreach (const QByteArray &propertyName, q->dynamicPropertyNames()) {
        if (!qstrncmp(propertyName, "_q_platform_", 12))
            win->setProperty(propertyName, q->property(propertyName));
    }

    win->setFlags(data.window_flags);
    fixPosIncludesFrame();
    if (q->testAttribute(Qt::WA_Moved)
        || !QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowManagement))
        win->setGeometry(q->geometry());
    else
        win->resize(q->size());
    win->setScreen(QGuiApplication::screens().value(topData()->screenIndex, 0));

    QSurfaceFormat format = win->requestedFormat();
    if ((flags & Qt::Window) && win->surfaceType() != QSurface::OpenGLSurface
            && q->testAttribute(Qt::WA_TranslucentBackground)) {
        format.setAlphaBufferSize(8);
    }
    win->setFormat(format);

    if (QWidget *nativeParent = q->nativeParentWidget()) {
        if (nativeParent->windowHandle()) {
            if (flags & Qt::Window) {
                win->setTransientParent(nativeParent->windowHandle());
                win->setParent(0);
            } else {
                win->setTransientParent(0);
                win->setParent(nativeParent->windowHandle());
            }
        }
    }

    qt_window_private(win)->positionPolicy = topData()->posIncludesFrame ?
        QWindowPrivate::WindowFrameInclusive : QWindowPrivate::WindowFrameExclusive;
    win->create();
    // Enable nonclient-area events for QDockWidget and other NonClientArea-mouse event processing.
    if ((flags & Qt::Desktop) == Qt::Window)
        win->handle()->setFrameStrutEventsEnabled(true);

    data.window_flags = win->flags();

    QBackingStore *store = q->backingStore();

    if (!store) {
        if (win && q->windowType() != Qt::Desktop) {
            if (q->isTopLevel())
                q->setBackingStore(new QBackingStore(win));
        } else {
            q->setAttribute(Qt::WA_PaintOnScreen, true);
        }
    }

    setWindowModified_helper();
    WId id = win->winId();
    // See the QPlatformWindow::winId() documentation
    Q_ASSERT(id != WId(0));
    setWinId(id);

    // Check children and create windows for them if necessary
    q_createNativeChildrenAndSetParent(q);

    if (extra && !extra->mask.isEmpty())
        setMask_sys(extra->mask);

    // If widget is already shown, set window visible, too
    if (q->isVisible())
        win->setVisible(true);
}

void QWidget::destroy(bool destroyWindow, bool destroySubWindows)
{
    Q_D(QWidget);

    d->aboutToDestroy();
    if (!isWindow() && parentWidget())
        parentWidget()->d_func()->invalidateBuffer(d->effectiveRectFor(geometry()));
    d->deactivateWidgetCleanup();

    if ((windowType() == Qt::Popup) && qApp)
        qApp->d_func()->closePopup(this);

    if (this == QApplicationPrivate::active_window)
        QApplication::setActiveWindow(0);
    if (QWidget::mouseGrabber() == this)
        releaseMouse();
    if (QWidget::keyboardGrabber() == this)
        releaseKeyboard();

    setAttribute(Qt::WA_WState_Created, false);

    if (windowType() != Qt::Desktop) {
        if (destroySubWindows) {
            QObjectList childList(children());
            for (int i = 0; i < childList.size(); i++) {
                QWidget *widget = qobject_cast<QWidget *>(childList.at(i));
                if (widget && widget->testAttribute(Qt::WA_NativeWindow)) {
                    if (widget->windowHandle()) {
                        widget->destroy();
                    }
                }
            }
        }
        if (destroyWindow) {
            d->deleteTLSysExtra();
        } else {
            if (parentWidget() && parentWidget()->testAttribute(Qt::WA_WState_Created)) {
                d->hide_sys();
            }
        }

        d->setWinId(0);
    }
}

void QWidgetPrivate::setParent_sys(QWidget *newparent, Qt::WindowFlags f)
{
    Q_Q(QWidget);

    Qt::WindowFlags oldFlags = data.window_flags;
    bool wasCreated = q->testAttribute(Qt::WA_WState_Created);

    int targetScreen = -1;
    // Handle a request to move the widget to a particular screen
    if (newparent && newparent->windowType() == Qt::Desktop) {
        // make sure the widget is created on the same screen as the
        // programmer specified desktop widget

        // get the desktop's screen number
        targetScreen = newparent->window()->d_func()->topData()->screenIndex;
        newparent = 0;
    }

    setWinId(0);

    if (parent != newparent) {
        QObjectPrivate::setParent_helper(newparent); //### why does this have to be done in the _sys function???
        if (q->windowHandle()) {
            q->windowHandle()->setFlags(f);
            QWidget *parentWithWindow =
                newparent ? (newparent->windowHandle() ? newparent : newparent->nativeParentWidget()) : 0;
            if (parentWithWindow) {
                if (f & Qt::Window) {
                    q->windowHandle()->setTransientParent(parentWithWindow->windowHandle());
                    q->windowHandle()->setParent(0);
                } else {
                    q->windowHandle()->setTransientParent(0);
                    q->windowHandle()->setParent(parentWithWindow->windowHandle());
                }
            } else {
                q->windowHandle()->setTransientParent(0);
                q->windowHandle()->setParent(0);
            }
        }
    }

    if (!newparent) {
        f |= Qt::Window;
        if (targetScreen == -1) {
            if (parent)
                targetScreen = q->parentWidget()->window()->d_func()->topData()->screenIndex;
        }
    }

    bool explicitlyHidden = q->testAttribute(Qt::WA_WState_Hidden) && q->testAttribute(Qt::WA_WState_ExplicitShowHide);

    // Reparenting toplevel to child
    if (wasCreated && !(f & Qt::Window) && (oldFlags & Qt::Window) && !q->testAttribute(Qt::WA_NativeWindow)) {
        if (extra && extra->hasWindowContainer)
            QWindowContainer::toplevelAboutToBeDestroyed(q);
        q->destroy();
    }

    adjustFlags(f, q);
    data.window_flags = f;
    q->setAttribute(Qt::WA_WState_Created, false);
    q->setAttribute(Qt::WA_WState_Visible, false);
    q->setAttribute(Qt::WA_WState_Hidden, false);

    if (newparent && wasCreated && (q->testAttribute(Qt::WA_NativeWindow) || (f & Qt::Window)))
        q->createWinId();

    if (q->isWindow() || (!newparent || newparent->isVisible()) || explicitlyHidden)
        q->setAttribute(Qt::WA_WState_Hidden);
    q->setAttribute(Qt::WA_WState_ExplicitShowHide, explicitlyHidden);

    // move the window to the selected screen
    if (!newparent && targetScreen != -1) {
        if (maybeTopData())
            maybeTopData()->screenIndex = targetScreen;
        // only if it is already created
        if (q->testAttribute(Qt::WA_WState_Created)) {
            q->windowHandle()->setScreen(QGuiApplication::screens().value(targetScreen, 0));
        }
    }
}

QPoint QWidget::mapToGlobal(const QPoint &pos) const
{
    int x = pos.x(), y = pos.y();
    const QWidget *w = this;
    while (w) {
        QWindow *window = w->windowHandle();
        if (window && window->handle())
            return window->mapToGlobal(QPoint(x, y));

        x += w->data->crect.x();
        y += w->data->crect.y();
        w = w->isWindow() ? 0 : w->parentWidget();
    }
    return QPoint(x, y);
}

QPoint QWidget::mapFromGlobal(const QPoint &pos) const
{
    int x = pos.x(), y = pos.y();
    const QWidget *w = this;
    while (w) {
        QWindow *window = w->windowHandle();
        if (window && window->handle())
            return window->mapFromGlobal(QPoint(x, y));

        x -= w->data->crect.x();
        y -= w->data->crect.y();
        w = w->isWindow() ? 0 : w->parentWidget();
    }
    return QPoint(x, y);
}

void QWidgetPrivate::updateSystemBackground() {}

#ifndef QT_NO_CURSOR
void QWidgetPrivate::setCursor_sys(const QCursor &cursor)
{
    Q_UNUSED(cursor);
    Q_Q(QWidget);
    qt_qpa_set_cursor(q, false);
}

void QWidgetPrivate::unsetCursor_sys()
{
    Q_Q(QWidget);
    qt_qpa_set_cursor(q, false);
}

#endif //QT_NO_CURSOR

void QWidgetPrivate::setWindowTitle_sys(const QString &caption)
{
    Q_Q(QWidget);
    if (!q->isWindow())
        return;

    if (QWindow *window = q->windowHandle())
        window->setTitle(caption);

}

void QWidgetPrivate::setWindowFilePath_sys(const QString &filePath)
{
    Q_Q(QWidget);
    if (!q->isWindow())
        return;

    if (QWindow *window = q->windowHandle())
        window->setFilePath(filePath);
}

void QWidgetPrivate::setWindowIcon_sys()
{
    Q_Q(QWidget);
    if (QWindow *window = q->windowHandle())
        window->setIcon(q->windowIcon());
}

void QWidgetPrivate::setWindowIconText_sys(const QString &iconText)
{
    Q_UNUSED(iconText);
}

QWidget *qt_pressGrab = 0;
QWidget *qt_mouseGrb = 0;
static QWidget *keyboardGrb = 0;

static inline QWindow *grabberWindow(const QWidget *w)
{
    QWindow *window = w->windowHandle();
    if (!window)
        if (const QWidget *nativeParent = w->nativeParentWidget())
            window = nativeParent->windowHandle();
    return window;
}

void QWidget::grabMouse()
{
    if (qt_mouseGrb)
        qt_mouseGrb->releaseMouse();

    if (QWindow *window = grabberWindow(this))
        window->setMouseGrabEnabled(true);

    qt_mouseGrb = this;
    qt_pressGrab = 0;
}

#ifndef QT_NO_CURSOR
void QWidget::grabMouse(const QCursor &cursor)
{
    Q_UNUSED(cursor);
    grabMouse();
}
#endif

bool QWidgetPrivate::stealMouseGrab(bool grab)
{
    // This is like a combination of grab/releaseMouse() but with error checking
    // and it has no effect on the result of mouseGrabber().
    Q_Q(QWidget);
    QWindow *window = grabberWindow(q);
    return window ? window->setMouseGrabEnabled(grab) : false;
}

void QWidget::releaseMouse()
{
    if (qt_mouseGrb == this) {
        if (QWindow *window = grabberWindow(this))
            window->setMouseGrabEnabled(false);
        qt_mouseGrb = 0;
    }
}

void QWidget::grabKeyboard()
{
    if (keyboardGrb)
        keyboardGrb->releaseKeyboard();
    if (QWindow *window = grabberWindow(this))
        window->setKeyboardGrabEnabled(true);
    keyboardGrb = this;
}

bool QWidgetPrivate::stealKeyboardGrab(bool grab)
{
    // This is like a combination of grab/releaseKeyboard() but with error
    // checking and it has no effect on the result of keyboardGrabber().
    Q_Q(QWidget);
    QWindow *window = grabberWindow(q);
    return window ? window->setKeyboardGrabEnabled(grab) : false;
}

void QWidget::releaseKeyboard()
{
    if (keyboardGrb == this) {
        if (QWindow *window = grabberWindow(this))
            window->setKeyboardGrabEnabled(false);
        keyboardGrb = 0;
    }
}

QWidget *QWidget::mouseGrabber()
{
    if (qt_mouseGrb)
        return qt_mouseGrb;
    return qt_pressGrab;
}

QWidget *QWidget::keyboardGrabber()
{
    return keyboardGrb;
}

void QWidget::activateWindow()
{
    QWindow *const wnd = window()->windowHandle();

    if (wnd)
        wnd->requestActivate();
}

// move() was invoked with Qt::WA_WState_Created not set (frame geometry
// unknown), that is, crect has a position including the frame.
// If we can determine the frame strut, fix that and clear the flag.
void QWidgetPrivate::fixPosIncludesFrame()
{
    Q_Q(QWidget);
    if (QTLWExtra *te = maybeTopData()) {
        if (te->posIncludesFrame) {
            // For Qt::WA_DontShowOnScreen, assume a frame of 0 (for
            // example, in QGraphicsProxyWidget).
            if (q->testAttribute(Qt::WA_DontShowOnScreen)) {
                te->posIncludesFrame = 0;
            } else {
                if (q->windowHandle()) {
                    updateFrameStrut();
                    if (!q->data->fstrut_dirty) {
                        data.crect.translate(te->frameStrut.x(), te->frameStrut.y());
                        te->posIncludesFrame = 0;
                    }
                } // windowHandle()
            } // !WA_DontShowOnScreen
        } // posIncludesFrame
    } // QTLWExtra
}

void QWidgetPrivate::show_sys()
{
    Q_Q(QWidget);

    QWindow *window = q->windowHandle();

    if (q->testAttribute(Qt::WA_DontShowOnScreen)) {
        invalidateBuffer(q->rect());
        q->setAttribute(Qt::WA_Mapped);
        if (q->isWindow() && q->windowModality() != Qt::NonModal && window) {
            // add our window to the modal window list
            QGuiApplicationPrivate::showModalWindow(window);
        }
        return;
    }

    if (renderToTexture && !q->isWindow())
        QApplication::postEvent(q->parentWidget(), new QUpdateLaterEvent(q->geometry()));
    else
        QApplication::postEvent(q, new QUpdateLaterEvent(q->rect()));

    if (!q->isWindow() && !q->testAttribute(Qt::WA_NativeWindow))
        return;

    if (window) {
        if (q->isWindow())
            fixPosIncludesFrame();
        QRect geomRect = q->geometry();
        if (!q->isWindow()) {
            QPoint topLeftOfWindow = q->mapTo(q->nativeParentWidget(),QPoint());
            geomRect.moveTopLeft(topLeftOfWindow);
        }
        const QRect windowRect = window->geometry();
        if (windowRect != geomRect) {
            if (q->testAttribute(Qt::WA_Moved)
                || !QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowManagement))
                window->setGeometry(geomRect);
            else
                window->resize(geomRect.size());
        }

#ifndef QT_NO_CURSOR
        qt_qpa_set_cursor(q, false); // Needed in case cursor was set before show
#endif
        invalidateBuffer(q->rect());
        window->setVisible(true);
        // Was the window moved by the Window system or QPlatformWindow::initialGeometry() ?
        if (window->isTopLevel()) {
            const QPoint crectTopLeft = q->data->crect.topLeft();
            const QPoint windowTopLeft = window->geometry().topLeft();
            if (crectTopLeft == QPoint(0, 0) && windowTopLeft != crectTopLeft)
                q->data->crect.moveTopLeft(windowTopLeft);
        }
    }
}


void QWidgetPrivate::hide_sys()
{
    Q_Q(QWidget);

    QWindow *window = q->windowHandle();

    if (q->testAttribute(Qt::WA_DontShowOnScreen)) {
        q->setAttribute(Qt::WA_Mapped, false);
        if (q->isWindow() && q->windowModality() != Qt::NonModal && window) {
            // remove our window from the modal window list
            QGuiApplicationPrivate::hideModalWindow(window);
        }
        // do not return here, if window non-zero, we must hide it
    }

    deactivateWidgetCleanup();

    if (!q->isWindow()) {
        QWidget *p = q->parentWidget();
        if (p &&p->isVisible()) {
            if (renderToTexture)
                p->d_func()->invalidateBuffer(q->geometry());
            else
                invalidateBuffer(q->rect());
        }
    } else {
        invalidateBuffer(q->rect());
    }

    if (window)
        window->setVisible(false);
}

Qt::WindowState effectiveState(Qt::WindowStates state)
 {
     if (state & Qt::WindowMinimized)
         return Qt::WindowMinimized;
     else if (state & Qt::WindowFullScreen)
         return Qt::WindowFullScreen;
     else if (state & Qt::WindowMaximized)
         return Qt::WindowMaximized;
     return Qt::WindowNoState;
 }

void QWidget::setWindowState(Qt::WindowStates newstate)
{
    Q_D(QWidget);
    Qt::WindowStates oldstate = windowState();
    if (oldstate == newstate)
        return;
    if (isWindow() && !testAttribute(Qt::WA_WState_Created))
        create();

    data->window_state = newstate;
    data->in_set_window_state = 1;
    Qt::WindowState newEffectiveState = effectiveState(newstate);
    Qt::WindowState oldEffectiveState = effectiveState(oldstate);
    if (isWindow() && newEffectiveState != oldEffectiveState) {
        // Ensure the initial size is valid, since we store it as normalGeometry below.
        if (!testAttribute(Qt::WA_Resized) && !isVisible())
            adjustSize();

        d->createTLExtra();
        if (oldEffectiveState == Qt::WindowNoState)
            d->topData()->normalGeometry = geometry();

        Q_ASSERT(windowHandle());
        windowHandle()->setWindowState(newEffectiveState);
    }
    data->in_set_window_state = 0;

    if (newstate & Qt::WindowActive)
        activateWindow();

    QWindowStateChangeEvent e(oldstate);
    QApplication::sendEvent(this, &e);
}

void QWidgetPrivate::setFocus_sys()
{
    Q_Q(QWidget);
    // Embedded native widget may have taken the focus; get it back to toplevel if that is the case
    const QWidget *topLevel = q->window();
    if (topLevel->windowType() != Qt::Popup) {
        if (QWindow *nativeWindow = q->window()->windowHandle()) {
            if (nativeWindow != QGuiApplication::focusWindow()
                && q->testAttribute(Qt::WA_WState_Created)) {
                nativeWindow->requestActivate();
            }
        }
    }
}

void QWidgetPrivate::raise_sys()
{
    Q_Q(QWidget);
    if (q->isWindow() || q->testAttribute(Qt::WA_NativeWindow)) {
        q->windowHandle()->raise();
    } else if (renderToTexture) {
        if (QWidget *p = q->parentWidget()) {
            setDirtyOpaqueRegion();
            p->d_func()->invalidateBuffer(effectiveRectFor(q->geometry()));
        }
    }
}

void QWidgetPrivate::lower_sys()
{
    Q_Q(QWidget);
    if (q->isWindow() || q->testAttribute(Qt::WA_NativeWindow)) {
        Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
        q->windowHandle()->lower();
    } else if (QWidget *p = q->parentWidget()) {
        setDirtyOpaqueRegion();
        p->d_func()->invalidateBuffer(effectiveRectFor(q->geometry()));
    }
}

void QWidgetPrivate::stackUnder_sys(QWidget*)
{
    Q_Q(QWidget);
    if (QWidget *p = q->parentWidget()) {
        setDirtyOpaqueRegion();
        p->d_func()->invalidateBuffer(effectiveRectFor(q->geometry()));
    }
}

void QWidgetPrivate::setGeometry_sys(int x, int y, int w, int h, bool isMove)
{
    Q_Q(QWidget);
    if (extra) {                                // any size restrictions?
        w = qMin(w,extra->maxw);
        h = qMin(h,extra->maxh);
        w = qMax(w,extra->minw);
        h = qMax(h,extra->minh);
    }

    if (q->isWindow() && q->windowHandle()) {
        QPlatformIntegration *integration = QGuiApplicationPrivate::platformIntegration();
        if (!integration->hasCapability(QPlatformIntegration::NonFullScreenWindows)) {
            x = 0;
            y = 0;
            w = q->windowHandle()->width();
            h = q->windowHandle()->height();
        }
    }

    QPoint oldp = q->geometry().topLeft();
    QSize olds = q->size();
    QRect r(x, y, w, h);

    bool isResize = olds != r.size();
    isMove = oldp != r.topLeft(); //### why do we have isMove as a parameter?


    // We only care about stuff that changes the geometry, or may
    // cause the window manager to change its state
    if (r.size() == olds && oldp == r.topLeft())
        return;

    if (!data.in_set_window_state) {
        q->data->window_state &= ~Qt::WindowMaximized;
        q->data->window_state &= ~Qt::WindowFullScreen;
        if (q->isWindow())
            topData()->normalGeometry = QRect(0, 0, -1, -1);
    }

    QPoint oldPos = q->pos();
    data.crect = r;

    bool needsShow = false;

    if (!(data.window_state & Qt::WindowFullScreen) && (w == 0 || h == 0)) {
        q->setAttribute(Qt::WA_OutsideWSRange, true);
        if (q->isVisible() && q->testAttribute(Qt::WA_Mapped))
            hide_sys();
        data.crect = QRect(x, y, w, h);
    } else if (q->isVisible() && q->testAttribute(Qt::WA_OutsideWSRange)) {
        q->setAttribute(Qt::WA_OutsideWSRange, false);
        needsShow = true;
    }

    if (q->isVisible()) {
        if (!q->testAttribute(Qt::WA_DontShowOnScreen) && !q->testAttribute(Qt::WA_OutsideWSRange)) {
            if (q->windowHandle()) {
                if (q->isWindow()) {
                    q->windowHandle()->setGeometry(q->geometry());
                } else {
                    QPoint posInNativeParent =  q->mapTo(q->nativeParentWidget(),QPoint());
                    q->windowHandle()->setGeometry(QRect(posInNativeParent,r.size()));
                }

                if (needsShow)
                    show_sys();
            }

            if (!q->isWindow()) {
                if (renderToTexture) {
                    QRegion updateRegion(q->geometry());
                    updateRegion += QRect(oldPos, olds);
                    q->parentWidget()->d_func()->invalidateBuffer(updateRegion);
                } else if (isMove && !isResize) {
                    moveRect(QRect(oldPos, olds), x - oldPos.x(), y - oldPos.y());
                } else {
                    invalidateBuffer_resizeHelper(oldPos, olds);
                }
            }
        }

        // generate a move event for QWidgets without window handles. QWidgets with native
        // window handles already receive a move event from
        // QGuiApplicationPrivate::processGeometryChangeEvent.
        if (isMove && (!q->windowHandle() || q->testAttribute(Qt::WA_DontShowOnScreen))) {
            QMoveEvent e(q->pos(), oldPos);
            QApplication::sendEvent(q, &e);
        }
        if (isResize) {
            QResizeEvent e(r.size(), olds);
            QApplication::sendEvent(q, &e);
            if (q->windowHandle())
                q->update();
        }
    } else { // not visible
        if (isMove && q->pos() != oldPos)
            q->setAttribute(Qt::WA_PendingMoveEvent, true);
        if (isResize)
            q->setAttribute(Qt::WA_PendingResizeEvent, true);
    }

}

void QWidgetPrivate::setConstraints_sys()
{
    Q_Q(QWidget);
    if (extra && q->windowHandle()) {
        QWindow *win = q->windowHandle();
        QWindowPrivate *winp = qt_window_private(win);

        winp->minimumSize = QSize(extra->minw, extra->minh);
        winp->maximumSize = QSize(extra->maxw, extra->maxh);

        if (extra->topextra) {
            winp->baseSize = QSize(extra->topextra->basew, extra->topextra->baseh);
            winp->sizeIncrement = QSize(extra->topextra->incw, extra->topextra->inch);
        }

        if (winp->platformWindow) {
            fixPosIncludesFrame();
            winp->platformWindow->propagateSizeHints();
        }
    }
}

void QWidgetPrivate::scroll_sys(int dx, int dy)
{
    Q_Q(QWidget);
    scrollChildren(dx, dy);
    scrollRect(q->rect(), dx, dy);
}

void QWidgetPrivate::scroll_sys(int dx, int dy, const QRect &r)
{
    scrollRect(r, dx, dy);
}

int QWidget::metric(PaintDeviceMetric m) const
{
    Q_D(const QWidget);

    QWindow *topLevelWindow = 0;
    QScreen *screen = 0;
    if (QWidget *topLevel = window())
        topLevelWindow = topLevel->windowHandle();

    if (topLevelWindow) {
        QPlatformScreen *platformScreen = QPlatformScreen::platformScreenForWindow(topLevelWindow);
        if (platformScreen)
            screen = platformScreen->screen();
    }
    if (!screen && QGuiApplication::primaryScreen())
        screen = QGuiApplication::primaryScreen();

    if (!screen) {
        if (m == PdmDpiX || m == PdmDpiY)
              return 72;
        return QPaintDevice::metric(m);
    }
    int val;
    if (m == PdmWidth) {
        val = data->crect.width();
    } else if (m == PdmWidthMM) {
        val = data->crect.width() * screen->physicalSize().width() / screen->geometry().width();
    } else if (m == PdmHeight) {
        val = data->crect.height();
    } else if (m == PdmHeightMM) {
        val = data->crect.height() * screen->physicalSize().height() / screen->geometry().height();
    } else if (m == PdmDepth) {
        return screen->depth();
    } else if (m == PdmDpiX) {
        if (d->extra && d->extra->customDpiX)
            return d->extra->customDpiX;
        else if (d->parent)
            return static_cast<QWidget *>(d->parent)->metric(m);
        return qRound(screen->logicalDotsPerInchX());
    } else if (m == PdmDpiY) {
        if (d->extra && d->extra->customDpiY)
            return d->extra->customDpiY;
        else if (d->parent)
            return static_cast<QWidget *>(d->parent)->metric(m);
        return qRound(screen->logicalDotsPerInchY());
    } else if (m == PdmPhysicalDpiX) {
        return qRound(screen->physicalDotsPerInchX());
    } else if (m == PdmPhysicalDpiY) {
        return qRound(screen->physicalDotsPerInchY());
    } else if (m == PdmDevicePixelRatio) {
        return topLevelWindow ? topLevelWindow->devicePixelRatio() : qApp->devicePixelRatio();
    } else {
        val = QPaintDevice::metric(m);// XXX
    }
    return val;
}

/*!
    If this is a native widget, return the associated QWindow.
    Otherwise return null.

    Native widgets include toplevel widgets, QGLWidget, and child widgets
    on which winId() was called.

    \since 5.0

    \sa winId()
*/
QWindow *QWidget::windowHandle() const
{
    Q_D(const QWidget);
    QTLWExtra *extra = d->maybeTopData();
    if (extra)
        return extra->window;

    return 0;
}

void QWidgetPrivate::createSysExtra()
{
}

void QWidgetPrivate::deleteSysExtra()
{

}

#ifdef Q_OS_WIN
static const char activeXNativeParentHandleProperty[] = "_q_embedded_native_parent_handle";
#endif

void QWidgetPrivate::createTLSysExtra()
{
    Q_Q(QWidget);
    if (!extra->topextra->window && (q->testAttribute(Qt::WA_NativeWindow) || q->isWindow())) {
        extra->topextra->window = new QWidgetWindow(q);
        if (extra->minw || extra->minh)
            extra->topextra->window->setMinimumSize(QSize(extra->minw, extra->minh));
        if (extra->maxw != QWIDGETSIZE_MAX || extra->maxh != QWIDGETSIZE_MAX)
            extra->topextra->window->setMaximumSize(QSize(extra->maxw, extra->maxh));
        if (extra->topextra->opacity != 255 && q->isWindow())
            extra->topextra->window->setOpacity(qreal(extra->topextra->opacity) / qreal(255));
#ifdef Q_OS_WIN
        // Pass on native parent handle for Widget embedded into Active X.
        const QVariant activeXNativeParentHandle = q->property(activeXNativeParentHandleProperty);
        if (activeXNativeParentHandle.isValid())
            extra->topextra->window->setProperty(activeXNativeParentHandleProperty, activeXNativeParentHandle);
        if (q->inherits("QTipLabel") || q->inherits("QAlphaWidget"))
            extra->topextra->window->setProperty("_q_windowsDropShadow", QVariant(true));
#endif
    }

}

void QWidgetPrivate::deleteTLSysExtra()
{
    if (extra && extra->topextra) {
        //the qplatformbackingstore may hold a reference to the window, so the backingstore
        //needs to be deleted first. If the backingstore holds GL resources, we need to
        // make the context current here, since the platform bs does not have a reference
        // to the widget.

#ifndef QT_NO_OPENGL
        if (textureChildSeen && extra->topextra->shareContext)
            extra->topextra->shareContext->makeCurrent(extra->topextra->window);
#endif
        extra->topextra->backingStoreTracker.destroy();
        delete extra->topextra->backingStore;
        extra->topextra->backingStore = 0;
#ifndef QT_NO_OPENGL
        if (textureChildSeen && extra->topextra->shareContext)
            extra->topextra->shareContext->doneCurrent();
        delete extra->topextra->shareContext;
        extra->topextra->shareContext = 0;
#endif

        //the toplevel might have a context with a "qglcontext associated with it. We need to
        //delete the qglcontext before we delete the qplatformopenglcontext.
        //One unfortunate thing about this is that we potentially create a glContext just to
        //delete it straight afterwards.
        if (extra->topextra->window) {
            extra->topextra->window->destroy();
        }
        setWinId(0);
        delete extra->topextra->window;
        extra->topextra->window = 0;

    }
}

void QWidgetPrivate::registerDropSite(bool on)
{
    Q_UNUSED(on);
}

void QWidgetPrivate::setMask_sys(const QRegion &region)
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowMasks)) {
        qWarning("%s: Not supported on %s.", Q_FUNC_INFO, qPrintable(QGuiApplication::platformName()));
        return;
    }
    Q_Q(QWidget);
    if (const QWindow *window = q->windowHandle())
        if (QPlatformWindow *platformWindow = window->handle())
            platformWindow->setMask(region);
}

void QWidgetPrivate::updateFrameStrut()
{
    Q_Q(QWidget);
    if (q->data->fstrut_dirty) {
        if (QTLWExtra *te = maybeTopData()) {
            if (te->window) {
                if (const QPlatformWindow *pw = te->window->handle()) {
                    const QMargins margins = pw->frameMargins();
                    if (!margins.isNull()) {
                        te->frameStrut.setCoords(margins.left(), margins.top(), margins.right(), margins.bottom());
                        q->data->fstrut_dirty = false;
                    }
                }
            }
        }
    }
}

void QWidgetPrivate::setWindowOpacity_sys(qreal level)
{
    Q_Q(QWidget);
    if (q->windowHandle())
        q->windowHandle()->setOpacity(level);
}

void QWidgetPrivate::setWSGeometry(bool dontShow, const QRect &oldRect)
{
    Q_UNUSED(dontShow);
    Q_UNUSED(oldRect);
    // XXX
}

QPaintEngine *QWidget::paintEngine() const
{
    qWarning("QWidget::paintEngine: Should no longer be called");

#ifdef Q_OS_WIN
    // We set this bit which is checked in setAttribute for
    // Qt::WA_PaintOnScreen. We do this to allow these two scenarios:
    //
    // 1. Users accidentally set Qt::WA_PaintOnScreen on X and port to
    // Windows which would mean suddenly their widgets stop working.
    //
    // 2. Users set paint on screen and subclass paintEngine() to
    // return 0, in which case we have a "hole" in the backingstore
    // allowing use of GDI or DirectX directly.
    //
    // 1 is WRONG, but to minimize silent failures, we have set this
    // bit to ignore the setAttribute call. 2. needs to be
    // supported because its our only means of embedding native
    // graphics stuff.
    const_cast<QWidgetPrivate *>(d_func())->noPaintOnScreen = 1;
#endif

    return 0; //##### @@@
}

void QWidgetPrivate::setModal_sys()
{
    Q_Q(QWidget);
    if (q->windowHandle())
        q->windowHandle()->setModality(q->windowModality());
}

#ifndef QT_NO_CURSOR
static inline void applyCursor(QWidget *w, QCursor c)
{
    if (QWindow *window = w->windowHandle())
        window->setCursor(c);
}

static inline void unsetCursor(QWidget *w)
{
    if (QWindow *window = w->windowHandle())
        window->unsetCursor();
}

void qt_qpa_set_cursor(QWidget *w, bool force)
{
    if (!w->testAttribute(Qt::WA_WState_Created))
        return;

    static QPointer<QWidget> lastUnderMouse = 0;
    if (force) {
        lastUnderMouse = w;
    } else if (lastUnderMouse) {
        const WId lastWinId = lastUnderMouse->effectiveWinId();
        const WId winId = w->effectiveWinId();
        if (lastWinId && lastWinId == winId)
            w = lastUnderMouse;
    } else if (!w->internalWinId()) {
        return; // The mouse is not under this widget, and it's not native, so don't change it.
    }

    while (!w->internalWinId() && w->parentWidget() && !w->isWindow()
           && !w->testAttribute(Qt::WA_SetCursor))
        w = w->parentWidget();

    QWidget *nativeParent = w;
    if (!w->internalWinId())
        nativeParent = w->nativeParentWidget();
    if (!nativeParent || !nativeParent->internalWinId())
        return;

    if (w->isWindow() || w->testAttribute(Qt::WA_SetCursor)) {
        if (w->isEnabled())
            applyCursor(nativeParent, w->cursor());
        else
            // Enforce the windows behavior of clearing the cursor on
            // disabled widgets.
            unsetCursor(nativeParent);
    } else {
        unsetCursor(nativeParent);
    }
}
#endif //QT_NO_CURSOR

QT_END_NAMESPACE
