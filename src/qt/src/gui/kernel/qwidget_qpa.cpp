/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "QtGui/qwidget.h"
#include "QtGui/qevent.h"
#include "QtGui/qapplication.h"
#include "QtGui/private/qbackingstore_p.h"
#include "QtGui/private/qwidget_p.h"
#include "QtGui/private/qgraphicssystem_p.h"
#include "QtGui/private/qapplication_p.h"
#include "QtGui/qdesktopwidget.h"
#include "QtGui/qplatformwindow_qpa.h"
#include "QtGui/qplatformglcontext_qpa.h"

#include <QtGui/QPlatformCursor>

QT_BEGIN_NAMESPACE

void q_createNativeChildrenAndSetParent(QPlatformWindow *parentWindow, const QWidget *parentWidget)
{
    QObjectList children = parentWidget->children();
    for (int i = 0; i < children.size(); i++) {
        if (children.at(i)->isWidgetType()) {
            const QWidget *childWidget = qobject_cast<const QWidget *>(children.at(i));
            if (childWidget) { // should not be necessary
                if (childWidget->testAttribute(Qt::WA_NativeWindow)) {
                    if (!childWidget->platformWindow())
                        childWidget->winId();
                }
                if (childWidget->platformWindow()) {
                    childWidget->platformWindow()->setParent(parentWindow);
                } else {
                    q_createNativeChildrenAndSetParent(parentWindow,childWidget);
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

    if ((!q->testAttribute(Qt::WA_NativeWindow) && !q->isWindow()) || q->windowType() == Qt::Desktop )
        return; // we only care about real toplevels

    QWindowSurface *surface = q->windowSurface();
    QPlatformWindow *platformWindow = q->platformWindow();

    if (!platformWindow) {
        platformWindow = QApplicationPrivate::platformIntegration()->createPlatformWindow(q);
    }
    Q_ASSERT(platformWindow);

    if (!surface ) {
        if (platformWindow && q->platformWindowFormat().hasWindowSurface()) {
            surface = QApplicationPrivate::platformIntegration()->createWindowSurface(q,platformWindow->winId());
        } else {
            q->setAttribute(Qt::WA_PaintOnScreen,true);
        }
    }

    data.window_flags = q->platformWindow()->setWindowFlags(data.window_flags);

    setWinId(q->platformWindow()->winId());

    //first check children. and create them if necessary
    q_createNativeChildrenAndSetParent(q->platformWindow(),q);

    //if we we have a parent, then set correct parent;
    if (!q->isWindow()) {
        if (QWidget *nativeParent = q->nativeParentWidget()) {
            if (nativeParent->platformWindow()) {
                platformWindow->setParent(nativeParent->platformWindow());
            }
        }
    }

    QApplicationPrivate::platformIntegration()->moveToScreen(q, topData()->screenIndex);
//    qDebug() << "create_sys" << q << q->internalWinId();
}

void QWidget::destroy(bool destroyWindow, bool destroySubWindows)
{
    Q_D(QWidget);

    d->aboutToDestroy();
    if (!isWindow() && parentWidget())
        parentWidget()->d_func()->invalidateBuffer(d->effectiveRectFor(geometry()));
    d->deactivateWidgetCleanup();

    if ((windowType() == Qt::Popup))
        qApp->d_func()->closePopup(this);

    //### we don't have proper focus event handling yet
    if (this == QApplicationPrivate::active_window)
        QApplication::setActiveWindow(0);

    setAttribute(Qt::WA_WState_Created, false);

    if (windowType() != Qt::Desktop) {
        if (destroySubWindows) {
            QObjectList childList(children());
            for (int i = 0; i < childList.size(); i++) {
                QWidget *widget = qobject_cast<QWidget *>(childList.at(i));
                if (widget && widget->testAttribute(Qt::WA_NativeWindow)) {
                    if (widget->platformWindow()) {
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

    int targetScreen = -1;
    // Handle a request to move the widget to a particular screen
    if (newparent && newparent->windowType() == Qt::Desktop) {
        // make sure the widget is created on the same screen as the
        // programmer specified desktop widget

        // get the desktop's screen number
        targetScreen = newparent->window()->d_func()->topData()->screenIndex;
        newparent = 0;
    }

    if (parent != newparent) {
        QObjectPrivate::setParent_helper(newparent); //### why does this have to be done in the _sys function???
        if (q->platformWindow() && newparent) {
            QWidget * parentWithWindow = newparent->platformWindow()? newparent : newparent->nativeParentWidget();
            if (parentWithWindow && parentWithWindow->platformWindow()) {
                q->platformWindow()->setParent(parentWithWindow->platformWindow());
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
    if (!(f&Qt::Window) && (oldFlags&Qt::Window) && !q->testAttribute(Qt::WA_NativeWindow)) {
        //qDebug() << "setParent_sys() change from toplevel";
        q->destroy();
    }

    data.window_flags = f;
    q->setAttribute(Qt::WA_WState_Created, false);
    q->setAttribute(Qt::WA_WState_Visible, false);
    q->setAttribute(Qt::WA_WState_Hidden, false);

    if (f & Qt::Window) {
        //qDebug() << "setParent_sys" << q << newparent << hex << f;
        if (QPlatformWindow *window = q->platformWindow())
            data.window_flags = window->setWindowFlags(data.window_flags);
    }
    
    if (q->isWindow() || (!newparent || newparent->isVisible()) || explicitlyHidden)
        q->setAttribute(Qt::WA_WState_Hidden);
    q->setAttribute(Qt::WA_WState_ExplicitShowHide, explicitlyHidden);

    // move the window to the selected screen
    if (!newparent && targetScreen != -1) {
        if (maybeTopData())
            maybeTopData()->screenIndex = targetScreen;
        // only if it is already created
        if (q->testAttribute(Qt::WA_WState_Created)) {
            QPlatformIntegration *platform = QApplicationPrivate::platformIntegration();
            platform->moveToScreen(q, targetScreen);
        }
    }
}

QPoint QWidget::mapToGlobal(const QPoint &pos) const
{
    int           x=pos.x(), y=pos.y();
    const QWidget* w = this;
    while (w) {
        x += w->data->crect.x();
        y += w->data->crect.y();
        w = w->isWindow() ? 0 : w->parentWidget();
    }
    return QPoint(x, y);
}

QPoint QWidget::mapFromGlobal(const QPoint &pos) const
{
    int           x=pos.x(), y=pos.y();
    const QWidget* w = this;
    while (w) {
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
    if (q->isVisible())
        qt_qpa_set_cursor(q, false);
}

void QWidgetPrivate::unsetCursor_sys()
{
    Q_Q(QWidget);
    if (q->isVisible())
        qt_qpa_set_cursor(q, false);
}

void QWidgetPrivate::updateCursor() const
{
    // XXX
}

#endif //QT_NO_CURSOR

void QWidgetPrivate::setWindowTitle_sys(const QString &caption)
{
    Q_Q(QWidget);
    if (!q->isWindow())
        return;

    if (QPlatformWindow *window = q->platformWindow())
        window->setWindowTitle(caption);

}

void QWidgetPrivate::setWindowIcon_sys(bool /*forceReset*/)
{
}

void QWidgetPrivate::setWindowIconText_sys(const QString &iconText)
{
    Q_UNUSED(iconText);
}

QWidget *qt_pressGrab = 0;
QWidget *qt_mouseGrb = 0;
static QWidget *keyboardGrb = 0;

void QWidget::grabMouse()
{
    if (qt_mouseGrb)
        qt_mouseGrb->releaseMouse();

    // XXX
    //qwsDisplay()->grabMouse(this,true);

    qt_mouseGrb = this;
    qt_pressGrab = 0;
}

#ifndef QT_NO_CURSOR
void QWidget::grabMouse(const QCursor &cursor)
{
    Q_UNUSED(cursor);

    if (qt_mouseGrb)
        qt_mouseGrb->releaseMouse();

    // XXX
    //qwsDisplay()->grabMouse(this,true);
    //qwsDisplay()->selectCursor(this, cursor.handle());
    qt_mouseGrb = this;
    qt_pressGrab = 0;
}
#endif

void QWidget::releaseMouse()
{
    if (qt_mouseGrb == this) {
        // XXX
        //qwsDisplay()->grabMouse(this,false);
        qt_mouseGrb = 0;
    }
}

void QWidget::grabKeyboard()
{
    if (keyboardGrb)
        keyboardGrb->releaseKeyboard();
    // XXX
    //qwsDisplay()->grabKeyboard(this, true);
    keyboardGrb = this;
}

void QWidget::releaseKeyboard()
{
    if (keyboardGrb == this) {
        // XXX
        //qwsDisplay()->grabKeyboard(this, false);
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
    if (platformWindow())
        platformWindow()->requestActivateWindow();
}

void QWidgetPrivate::show_sys()
{
    Q_Q(QWidget);
    q->setAttribute(Qt::WA_Mapped);
    if (q->testAttribute(Qt::WA_DontShowOnScreen)) {
        invalidateBuffer(q->rect());
        return;
    }

    QApplication::postEvent(q, new QUpdateLaterEvent(q->rect()));

    QPlatformWindow *window = q->platformWindow();
    if (window) {
        QRect geomRect = q->geometry();
        if (!q->isWindow()) {
            QPoint topLeftOfWindow = q->mapTo(q->nativeParentWidget(),QPoint());
            geomRect.moveTopLeft(topLeftOfWindow);
        }
        const QRect windowRect = window->geometry();
        if (windowRect != geomRect) {
            window->setGeometry(geomRect);
        }
        if (QWindowSurface *surface = q->windowSurface()) {
            if (windowRect.size() != geomRect.size()) {
                surface->resize(geomRect.size());
            }
        }
        if (window)
            window->setVisible(true);
    }
}


void QWidgetPrivate::hide_sys()
{
    Q_Q(QWidget);
    q->setAttribute(Qt::WA_Mapped, false);
    deactivateWidgetCleanup();
    if (!q->isWindow()) {
        QWidget *p = q->parentWidget();
        if (p &&p->isVisible()) {
            invalidateBuffer(q->rect());
        }
        return;
    }
    if (QPlatformWindow *window = q->platformWindow()) {
         window->setVisible(false);
     }

    //### we don't yet have proper focus event handling
    if (q == QApplicationPrivate::active_window)
        QApplication::setActiveWindow(0);

}

void QWidgetPrivate::setMaxWindowState_helper()
{
    Q_Q(QWidget);

    const uint old_state = data.in_set_window_state;
    data.in_set_window_state = 1;

    const QRect desktop = qApp->desktop()->availableGeometry(qApp->desktop()->screenNumber(q));
    q->setGeometry(desktop);

    data.in_set_window_state = old_state;
}

void QWidgetPrivate::setFullScreenSize_helper()
{
    Q_Q(QWidget);

    const uint old_state = data.in_set_window_state;
    data.in_set_window_state = 1;

    const QRect screen = qApp->desktop()->screenGeometry(qApp->desktop()->screenNumber(q));
    q->move(screen.topLeft());
    q->resize(screen.size());

    data.in_set_window_state = old_state;
}

static Qt::WindowStates effectiveState(Qt::WindowStates state)
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
    bool needShow = false;
    Qt::WindowStates newEffectiveState = effectiveState(newstate);
    Qt::WindowStates oldEffectiveState = effectiveState(oldstate);
    if (isWindow() && newEffectiveState != oldEffectiveState) {
        d->createTLExtra();
        if (oldEffectiveState == Qt::WindowNoState) { //normal
            d->topData()->normalGeometry = geometry();
        } else if (oldEffectiveState == Qt::WindowFullScreen) {
            setParent(0, d->topData()->savedFlags);
            needShow = true;
        } else if (oldEffectiveState == Qt::WindowMinimized) {
            needShow = true;
        }

        if (newEffectiveState == Qt::WindowMinimized) {
            //### not ideal...
            hide();
            needShow = false;
        } else if (newEffectiveState == Qt::WindowFullScreen) {
            d->topData()->savedFlags = windowFlags();
            setParent(0, Qt::FramelessWindowHint | (windowFlags() & Qt::WindowStaysOnTopHint));
            d->setFullScreenSize_helper();
            raise();
            needShow = true;
        } else if (newEffectiveState == Qt::WindowMaximized) {
            createWinId();
            d->setMaxWindowState_helper();
        } else { //normal
            QRect r = d->topData()->normalGeometry;
            if (r.width() >= 0) {
                d->topData()->normalGeometry = QRect(0,0,-1,-1);
                setGeometry(r);
            }
        }
    }
    data->in_set_window_state = 0;

    if (needShow)
        show();

    if (newstate & Qt::WindowActive)
        activateWindow();

    QWindowStateChangeEvent e(oldstate);
    QApplication::sendEvent(this, &e);
}

void QWidgetPrivate::setFocus_sys()
{

}

void QWidgetPrivate::raise_sys()
{
    Q_Q(QWidget);
    if (q->isWindow()) {
        q->platformWindow()->raise();
    }
}

void QWidgetPrivate::lower_sys()
{
    Q_Q(QWidget);
    if (q->isWindow()) {
        Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
        q->platformWindow()->lower();
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

    if (q->isVisible()) {
        if (q->platformWindow()) {
            if (q->isWindow()) {
                q->platformWindow()->setGeometry(q->geometry());
            } else {
                QPoint posInNativeParent =  q->mapTo(q->nativeParentWidget(),QPoint());
                q->platformWindow()->setGeometry(QRect(posInNativeParent,r.size()));
            }
            const QWidgetBackingStore *bs = maybeBackingStore();
            if (bs->windowSurface) {
                if (isResize)
                    bs->windowSurface->resize(r.size());
            }
        } else {
            if (isMove && !isResize)
                moveRect(QRect(oldPos, olds), x - oldPos.x(), y - oldPos.y());
            else
                invalidateBuffer_resizeHelper(oldPos, olds);
        }

        if (isMove) {
            QMoveEvent e(q->pos(), oldPos);
            QApplication::sendEvent(q, &e);
        }
        if (isResize) {
            QResizeEvent e(r.size(), olds);
            QApplication::sendEvent(q, &e);
            if (q->platformWindow())
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

    QPlatformScreen *screen = QPlatformScreen::platformScreenForWidget(this);
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
    } else if (m == PdmDpiX || m == PdmPhysicalDpiX) {
        if (d->extra && d->extra->customDpiX)
            return d->extra->customDpiX;
        else if (d->parent)
            return static_cast<QWidget *>(d->parent)->metric(m);
        return qRound(screen->geometry().width() / double(screen->physicalSize().width() / 25.4));
    } else if (m == PdmDpiY || m == PdmPhysicalDpiY) {
        if (d->extra && d->extra->customDpiY)
            return d->extra->customDpiY;
        else if (d->parent)
            return static_cast<QWidget *>(d->parent)->metric(m);
        return qRound(screen->geometry().height() / double(screen->physicalSize().height() / 25.4));
    } else {
        val = QPaintDevice::metric(m);// XXX
    }
    return val;
}

/*!
    \preliminary
    \since 4.8

    Sets the window to be the platform \a window specified.

    The widget takes ownership of the \a window. Any platform window
    previously set on the widget will be destroyed.
*/
void QWidget::setPlatformWindow(QPlatformWindow *window)
{
    Q_D(QWidget);

    QTLWExtra *topData = d->topData();
    if (topData->platformWindow == window)
        return;

    delete topData->platformWindow;
    topData->platformWindow = window;
}

/*!
    \preliminary
    \since 4.8

    Returns the QPlatformWindow this widget will be drawn into.
*/
QPlatformWindow *QWidget::platformWindow() const
{
    Q_D(const QWidget);
    QTLWExtra *extra = d->maybeTopData();
    if (extra && extra->platformWindow)
        return extra->platformWindow;

    return 0;
}

/*!
    \since 4.8

    Sets the platform window format for the widget to the \a format specified.
*/
void QWidget::setPlatformWindowFormat(const QPlatformWindowFormat &format)
{
    if (isWindow() || testAttribute(Qt::WA_NativeWindow)) {
        Q_D(QWidget);
        QTLWExtra *topData = d->topData();
        topData->platformWindowFormat = format;
        if (testAttribute(Qt::WA_WState_Created)) {
            bool wasVisible = testAttribute(Qt::WA_WState_Visible);
            destroy();
            d->create_sys(0,true,true);
            if (wasVisible)
                topData->platformWindow->setVisible(true);
        }
    }
}

/*!
    \since 4.8

    Returns the platform window format for the widget.
*/
QPlatformWindowFormat QWidget::platformWindowFormat() const
{
    Q_D(const QWidget);

    QPlatformWindowFormat format;

    QTLWExtra *extra = d->maybeTopData();
    if (extra){
        format = extra->platformWindowFormat;
    } else {
        format = QPlatformWindowFormat::defaultFormat();
    }

    if (testAttribute(Qt::WA_TranslucentBackground))
        format.setAlpha(true);

    return format;
}

void QWidgetPrivate::createSysExtra()
{
}

void QWidgetPrivate::deleteSysExtra()
{

}

void QWidgetPrivate::createTLSysExtra()
{
}

void QWidgetPrivate::deleteTLSysExtra()
{
    if (extra && extra->topextra) {
        //the toplevel might have a context with a "qglcontext associated with it. We need to
        //delete the qglcontext before we delete the qplatformglcontext.
        //One unfortunate thing about this is that we potentially create a glContext just to
        //delete it straight afterwards.
        if (extra->topextra->platformWindow) {
            if (QPlatformGLContext *context = extra->topextra->platformWindow->glContext()) {
                context->deleteQGLContext();
            }
        }
        setWinId(0);
        delete extra->topextra->platformWindow;
        extra->topextra->platformWindow = 0;
    }
}

void QWidgetPrivate::registerDropSite(bool on)
{
    Q_UNUSED(on);
}

void QWidgetPrivate::setMask_sys(const QRegion &region)
{
    Q_UNUSED(region);
    // XXX
}

void QWidgetPrivate::updateFrameStrut()
{
    // XXX
}

void QWidgetPrivate::setWindowOpacity_sys(qreal level)
{
    Q_Q(QWidget);
    q->platformWindow()->setOpacity(level);
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
    return 0; //##### @@@
}

QWindowSurface *QWidgetPrivate::createDefaultWindowSurface_sys()
{
    Q_Q(QWidget);
    if (q->platformWindowFormat().hasWindowSurface())
        return QApplicationPrivate::platformIntegration()->createWindowSurface(q,0);
    else
        return 0;
}

void QWidgetPrivate::setModal_sys()
{
}

#ifndef QT_NO_CURSOR
void qt_qpa_set_cursor(QWidget * w, bool force)
{
    static QCursor arrowCursor(Qt::ArrowCursor);
    static QPointer<QWidget> lastUnderMouse = 0;

    QCursor * override = QApplication::overrideCursor();

    if (override && w != 0)
        return;

    QWidget *cursorWidget;
    QCursor cursorCursor;

    do {
        if (w == 0) {
            if (override) {
                cursorCursor = *override;
                cursorWidget = QApplication::topLevelAt(QCursor::pos());
                break;
            }
            w = QApplication::widgetAt(QCursor::pos());
            if (w == 0) // clear the override cursor while over empty space
                w = QApplication::desktop();
        } else if (force) {
            lastUnderMouse = w;
        } else if (w->testAttribute(Qt::WA_WState_Created) && lastUnderMouse
                   && lastUnderMouse->effectiveWinId() == w->effectiveWinId()) {
            w = lastUnderMouse;
        }
        if (w == QApplication::desktop() && !override) {
            cursorCursor = arrowCursor;
            cursorWidget = w;
            break;
        }

        QWidget * curWin = QApplication::activeWindow();
        if (!curWin && w && w->internalWinId())
            return;
        QWidget* cW = w && !w->internalWinId() ? w : curWin;

        if (!cW || cW->window() != w->window() ||
            !cW->isVisible() || !cW->underMouse() || override)
            return;

        cursorCursor = w->cursor();
        cursorWidget = w;
    } while (0);
    foreach (QWeakPointer<QPlatformCursor> cursor, QPlatformCursorPrivate::getInstances())
        if (cursor)
            cursor.data()->changeCursor(&cursorCursor, cursorWidget);
}
#endif //QT_NO_CURSOR 

QT_END_NAMESPACE
