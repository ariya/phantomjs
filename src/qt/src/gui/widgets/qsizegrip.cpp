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

#include "qsizegrip.h"

#ifndef QT_NO_SIZEGRIP

#include "qapplication.h"
#include "qevent.h"
#include "qpainter.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qlayout.h"
#include "qdebug.h"
#include <QDesktopWidget>

#if defined(Q_WS_X11)
#include <private/qt_x11_p.h>
#elif defined (Q_WS_WIN)
#include "qt_windows.h"
#endif
#ifdef Q_WS_MAC
#include <private/qt_mac_p.h>
#endif

#include <private/qwidget_p.h>
#include <QtGui/qabstractscrollarea.h>

#define SZ_SIZEBOTTOMRIGHT  0xf008
#define SZ_SIZEBOTTOMLEFT   0xf007
#define SZ_SIZETOPLEFT      0xf004
#define SZ_SIZETOPRIGHT     0xf005

QT_BEGIN_NAMESPACE

static QWidget *qt_sizegrip_topLevelWidget(QWidget* w)
{
    while (w && !w->isWindow() && w->windowType() != Qt::SubWindow)
        w = w->parentWidget();
    return w;
}

class QSizeGripPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QSizeGrip)
public:
    void init();
    QPoint p;
    QRect r;
    int d;
    int dxMax;
    int dyMax;
    Qt::Corner m_corner;
    bool gotMousePress;
    QWidget *tlw;
#ifdef Q_WS_MAC
    void updateMacSizer(bool hide) const;
#endif
    Qt::Corner corner() const;
    inline bool atBottom() const
    {
        return m_corner == Qt::BottomRightCorner || m_corner == Qt::BottomLeftCorner;
    }

    inline bool atLeft() const
    {
        return m_corner == Qt::BottomLeftCorner || m_corner == Qt::TopLeftCorner;
    }

    void updateTopLevelWidget()
    {
        Q_Q(QSizeGrip);
        QWidget *w = qt_sizegrip_topLevelWidget(q);
        if (tlw == w)
            return;
        if (tlw)
            tlw->removeEventFilter(q);
        tlw = w;
        if (tlw)
            tlw->installEventFilter(q);
    }

    // This slot is invoked by QLayout when the size grip is added to
    // a layout or reparented after the tlw is shown. This re-implementation is basically
    // the same as QWidgetPrivate::_q_showIfNotHidden except that it checks
    // for Qt::WindowFullScreen and Qt::WindowMaximized as well.
    void _q_showIfNotHidden()
    {
        Q_Q(QSizeGrip);
        bool showSizeGrip = !(q->isHidden() && q->testAttribute(Qt::WA_WState_ExplicitShowHide));
        updateTopLevelWidget();
        if (tlw && showSizeGrip) {
            Qt::WindowStates sizeGripNotVisibleState = Qt::WindowFullScreen;
#ifndef Q_WS_MAC
            sizeGripNotVisibleState |= Qt::WindowMaximized;
#endif
            // Don't show the size grip if the tlw is maximized or in full screen mode.
            showSizeGrip = !(tlw->windowState() & sizeGripNotVisibleState);
        }
        if (showSizeGrip)
            q->setVisible(true);
    }
};

#ifdef Q_WS_MAC
void QSizeGripPrivate::updateMacSizer(bool hide) const
{
    Q_Q(const QSizeGrip);
    if (QApplication::closingDown() || !parent)
        return;
    QWidget *topLevelWindow = qt_sizegrip_topLevelWidget(const_cast<QSizeGrip *>(q));
    if(topLevelWindow && topLevelWindow->isWindow())
        QWidgetPrivate::qt_mac_update_sizer(topLevelWindow, hide ? -1 : 1);
}
#endif

Qt::Corner QSizeGripPrivate::corner() const
{
    Q_Q(const QSizeGrip);
    QWidget *tlw = qt_sizegrip_topLevelWidget(const_cast<QSizeGrip *>(q));
    const QPoint sizeGripPos = q->mapTo(tlw, QPoint(0, 0));
    bool isAtBottom = sizeGripPos.y() >= tlw->height() / 2;
    bool isAtLeft = sizeGripPos.x() <= tlw->width() / 2;
    if (isAtLeft)
        return isAtBottom ? Qt::BottomLeftCorner : Qt::TopLeftCorner;
    else
        return isAtBottom ? Qt::BottomRightCorner : Qt::TopRightCorner;
}

/*!
    \class QSizeGrip

    \brief The QSizeGrip class provides a resize handle for resizing top-level windows.

    \ingroup mainwindow-classes
    \ingroup basicwidgets

    This widget works like the standard Windows resize handle. In the
    X11 version this resize handle generally works differently from
    the one provided by the system if the X11 window manager does not
    support necessary modern post-ICCCM specifications.

    Put this widget anywhere in a widget tree and the user can use it
    to resize the top-level window or any widget with the Qt::SubWindow
    flag set. Generally, this should be in the lower right-hand corner.
    Note that QStatusBar already uses this widget, so if you have a
    status bar (e.g., you are using QMainWindow), then you don't need
    to use this widget explicitly.

    On some platforms the size grip automatically hides itself when the
    window is shown full screen or maximised.

    \table 50%
    \row \o \inlineimage plastique-sizegrip.png Screenshot of a Plastique style size grip
    \o A size grip widget at the bottom-right corner of a main window, shown in the
    \l{Plastique Style Widget Gallery}{Plastique widget style}.
    \endtable

    The QSizeGrip class inherits QWidget and reimplements the \l
    {QWidget::mousePressEvent()}{mousePressEvent()} and \l
    {QWidget::mouseMoveEvent()}{mouseMoveEvent()} functions to feature
    the resize functionality, and the \l
    {QWidget::paintEvent()}{paintEvent()} function to render the
    size grip widget.

    \sa QStatusBar QWidget::windowState()
*/


/*!
    Constructs a resize corner as a child widget of  the given \a
    parent.
*/
QSizeGrip::QSizeGrip(QWidget * parent)
    : QWidget(*new QSizeGripPrivate, parent, 0)
{
    Q_D(QSizeGrip);
    d->init();
}

#ifdef QT3_SUPPORT
/*!
    \obsolete

    Constructs a resize corner with the given \a name, as a child
    widget of the given \a parent.
*/
QSizeGrip::QSizeGrip(QWidget * parent, const char* name)
    : QWidget(*new QSizeGripPrivate, parent, 0)
{
    Q_D(QSizeGrip);
    setObjectName(QString::fromAscii(name));
    d->init();
}
#endif

void QSizeGripPrivate::init()
{
    Q_Q(QSizeGrip);
    dxMax = 0;
    dyMax = 0;
    tlw = 0;
    m_corner = q->isLeftToRight() ? Qt::BottomRightCorner : Qt::BottomLeftCorner;
    gotMousePress = false;

#if !defined(QT_NO_CURSOR) && !defined(Q_WS_MAC)
    q->setCursor(m_corner == Qt::TopLeftCorner || m_corner == Qt::BottomRightCorner
                 ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor);
#endif
    q->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
    updateTopLevelWidget();
}


/*!
    Destroys this size grip.
*/
QSizeGrip::~QSizeGrip()
{
}

/*!
  \reimp
*/
QSize QSizeGrip::sizeHint() const
{
    QStyleOption opt(0);
    opt.init(this);
    return (style()->sizeFromContents(QStyle::CT_SizeGrip, &opt, QSize(13, 13), this).
            expandedTo(QApplication::globalStrut()));
}

/*!
    Paints the resize grip.

    Resize grips are usually rendered as small diagonal textured lines
    in the lower-right corner. The paint event is passed in the \a
    event parameter.
*/
void QSizeGrip::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    Q_D(QSizeGrip);
    QPainter painter(this);
    QStyleOptionSizeGrip opt;
    opt.init(this);
    opt.corner = d->m_corner;
    style()->drawControl(QStyle::CE_SizeGrip, &opt, &painter, this);
}

/*!
    \fn void QSizeGrip::mousePressEvent(QMouseEvent * event)

    Receives the mouse press events for the widget, and primes the
    resize operation. The mouse press event is passed in the \a event
    parameter.
*/
void QSizeGrip::mousePressEvent(QMouseEvent * e)
{
    if (e->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(e);
        return;
    }

    Q_D(QSizeGrip);
    QWidget *tlw = qt_sizegrip_topLevelWidget(this);
    d->p = e->globalPos();
    d->gotMousePress = true;
    d->r = tlw->geometry();

#ifdef Q_WS_X11
    // Use a native X11 sizegrip for "real" top-level windows if supported.
    if (tlw->isWindow() && X11->isSupportedByWM(ATOM(_NET_WM_MOVERESIZE))
        && !(tlw->windowFlags() & Qt::X11BypassWindowManagerHint)
        && !tlw->testAttribute(Qt::WA_DontShowOnScreen) && !qt_widget_private(tlw)->hasHeightForWidth()) {
        XEvent xev;
        xev.xclient.type = ClientMessage;
        xev.xclient.message_type = ATOM(_NET_WM_MOVERESIZE);
        xev.xclient.display = X11->display;
        xev.xclient.window = tlw->winId();
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = e->globalPos().x();
        xev.xclient.data.l[1] = e->globalPos().y();
        if (d->atBottom())
            xev.xclient.data.l[2] = d->atLeft() ? 6 : 4; // bottomleft/bottomright
        else
            xev.xclient.data.l[2] = d->atLeft() ? 0 : 2; // topleft/topright
        xev.xclient.data.l[3] = Button1;
        xev.xclient.data.l[4] = 0;
        XUngrabPointer(X11->display, X11->time);
        XSendEvent(X11->display, QX11Info::appRootWindow(x11Info().screen()), False,
                   SubstructureRedirectMask | SubstructureNotifyMask, &xev);
        return;
    }
#endif // Q_WS_X11
#ifdef Q_WS_WIN
    if (tlw->isWindow() && !tlw->testAttribute(Qt::WA_DontShowOnScreen) && !qt_widget_private(tlw)->hasHeightForWidth()) {
        uint orientation = 0;
        if (d->atBottom())
            orientation = d->atLeft() ? SZ_SIZEBOTTOMLEFT : SZ_SIZEBOTTOMRIGHT;
        else
            orientation = d->atLeft() ? SZ_SIZETOPLEFT : SZ_SIZETOPRIGHT;

        ReleaseCapture();
        PostMessage(tlw->winId(), WM_SYSCOMMAND, orientation, 0);
        return;
    }
#endif // Q_WS_WIN

    // Find available desktop/workspace geometry.
    QRect availableGeometry;
    bool hasVerticalSizeConstraint = true;
    bool hasHorizontalSizeConstraint = true;
    if (tlw->isWindow())
        availableGeometry = QApplication::desktop()->availableGeometry(tlw);
    else {
        const QWidget *tlwParent = tlw->parentWidget();
        // Check if tlw is inside QAbstractScrollArea/QScrollArea.
        // If that's the case tlw->parentWidget() will return the viewport
        // and tlw->parentWidget()->parentWidget() will return the scroll area.
#ifndef QT_NO_SCROLLAREA
        QAbstractScrollArea *scrollArea = qobject_cast<QAbstractScrollArea *>(tlwParent->parentWidget());
        if (scrollArea) {
            hasHorizontalSizeConstraint = scrollArea->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff;
            hasVerticalSizeConstraint = scrollArea->verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff;
        }
#endif // QT_NO_SCROLLAREA
        availableGeometry = tlwParent->contentsRect();
    }

    // Find frame geometries, title bar height, and decoration sizes.
    const QRect frameGeometry = tlw->frameGeometry();
    const int titleBarHeight = qMax(tlw->geometry().y() - frameGeometry.y(), 0);
    const int bottomDecoration = qMax(frameGeometry.height() - tlw->height() - titleBarHeight, 0);
    const int leftRightDecoration = qMax((frameGeometry.width() - tlw->width()) / 2, 0);

    // Determine dyMax depending on whether the sizegrip is at the bottom
    // of the widget or not.
    if (d->atBottom()) {
        if (hasVerticalSizeConstraint)
            d->dyMax = availableGeometry.bottom() - d->r.bottom() - bottomDecoration;
        else
            d->dyMax = INT_MAX;
    } else {
        if (hasVerticalSizeConstraint)
            d->dyMax = availableGeometry.y() - d->r.y() + titleBarHeight;
        else
            d->dyMax = -INT_MAX;
    }

    // In RTL mode, the size grip is to the left; find dxMax from the desktop/workspace
    // geometry, the size grip geometry and the width of the decoration.
    if (d->atLeft()) {
        if (hasHorizontalSizeConstraint)
            d->dxMax = availableGeometry.x() - d->r.x() + leftRightDecoration;
        else
            d->dxMax = -INT_MAX;
    } else {
        if (hasHorizontalSizeConstraint)
            d->dxMax = availableGeometry.right() - d->r.right() - leftRightDecoration;
        else
            d->dxMax = INT_MAX;
    }
}


/*!
    \fn void QSizeGrip::mouseMoveEvent(QMouseEvent * event)
    Resizes the top-level widget containing this widget. The mouse
    move event is passed in the \a event parameter.
*/
void QSizeGrip::mouseMoveEvent(QMouseEvent * e)
{
    if (e->buttons() != Qt::LeftButton) {
        QWidget::mouseMoveEvent(e);
        return;
    }

    Q_D(QSizeGrip);
    QWidget* tlw = qt_sizegrip_topLevelWidget(this);
    if (!d->gotMousePress || tlw->testAttribute(Qt::WA_WState_ConfigPending))
        return;

#ifdef Q_WS_X11
    if (tlw->isWindow() && X11->isSupportedByWM(ATOM(_NET_WM_MOVERESIZE))
        && tlw->isTopLevel() && !(tlw->windowFlags() & Qt::X11BypassWindowManagerHint)
        && !tlw->testAttribute(Qt::WA_DontShowOnScreen) && !qt_widget_private(tlw)->hasHeightForWidth())
        return;
#endif
#ifdef Q_WS_WIN
    if (tlw->isWindow() && GetSystemMenu(tlw->winId(), FALSE) != 0 && internalWinId()
        && !tlw->testAttribute(Qt::WA_DontShowOnScreen) && !qt_widget_private(tlw)->hasHeightForWidth()) {
        MSG msg;
        while(PeekMessage(&msg, winId(), WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE));
        return;
    }
#endif

    QPoint np(e->globalPos());

    // Don't extend beyond the available geometry; bound to dyMax and dxMax.
    QSize ns;
    if (d->atBottom())
        ns.rheight() = d->r.height() + qMin(np.y() - d->p.y(), d->dyMax);
    else
        ns.rheight() = d->r.height() - qMax(np.y() - d->p.y(), d->dyMax);

    if (d->atLeft())
        ns.rwidth() = d->r.width() - qMax(np.x() - d->p.x(), d->dxMax);
    else
        ns.rwidth() = d->r.width() + qMin(np.x() - d->p.x(), d->dxMax);

    ns = QLayout::closestAcceptableSize(tlw, ns);

    QPoint p;
    QRect nr(p, ns);
    if (d->atBottom()) {
        if (d->atLeft())
            nr.moveTopRight(d->r.topRight());
        else
            nr.moveTopLeft(d->r.topLeft());
    } else {
        if (d->atLeft())
            nr.moveBottomRight(d->r.bottomRight());
        else
            nr.moveBottomLeft(d->r.bottomLeft());
    }

    tlw->setGeometry(nr);
}

/*!
  \reimp
*/
void QSizeGrip::mouseReleaseEvent(QMouseEvent *mouseEvent)
{
    if (mouseEvent->button() == Qt::LeftButton) {
        Q_D(QSizeGrip);
        d->gotMousePress = false;
        d->p = QPoint();
    } else {
        QWidget::mouseReleaseEvent(mouseEvent);
    }
}

/*!
  \reimp
*/
void QSizeGrip::moveEvent(QMoveEvent * /*moveEvent*/)
{
    Q_D(QSizeGrip);
    // We're inside a resize operation; no update necessary.
    if (!d->p.isNull())
        return;

    d->m_corner = d->corner();
#if !defined(QT_NO_CURSOR) && !defined(Q_WS_MAC)
    setCursor(d->m_corner == Qt::TopLeftCorner || d->m_corner == Qt::BottomRightCorner
              ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor);
#endif
}

/*!
  \reimp
*/
void QSizeGrip::showEvent(QShowEvent *showEvent)
{
#ifdef Q_WS_MAC
    d_func()->updateMacSizer(false);
#endif
    QWidget::showEvent(showEvent);
}

/*!
  \reimp
*/
void QSizeGrip::hideEvent(QHideEvent *hideEvent)
{
#ifdef Q_WS_MAC
    d_func()->updateMacSizer(true);
#endif
    QWidget::hideEvent(hideEvent);
}

/*!
    \reimp
*/
void QSizeGrip::setVisible(bool visible)
{
    QWidget::setVisible(visible);
}

/*! \reimp */
bool QSizeGrip::eventFilter(QObject *o, QEvent *e)
{
    Q_D(QSizeGrip);
    if ((isHidden() && testAttribute(Qt::WA_WState_ExplicitShowHide))
        || e->type() != QEvent::WindowStateChange
		|| o != d->tlw) {
        return QWidget::eventFilter(o, e);
    }
    Qt::WindowStates sizeGripNotVisibleState = Qt::WindowFullScreen;
#ifndef Q_WS_MAC
    sizeGripNotVisibleState |= Qt::WindowMaximized;
#endif
    // Don't show the size grip if the tlw is maximized or in full screen mode.
    setVisible(!(d->tlw->windowState() & sizeGripNotVisibleState));
    setAttribute(Qt::WA_WState_ExplicitShowHide, false);
    return QWidget::eventFilter(o, e);
}

/*!
    \reimp
*/
bool QSizeGrip::event(QEvent *event)
{
    return QWidget::event(event);
}

#ifdef Q_WS_WIN
/*! \reimp */
bool QSizeGrip::winEvent( MSG *m, long *result )
{
    return QWidget::winEvent(m, result);
}
#endif

QT_END_NAMESPACE

#include "moc_qsizegrip.cpp"

#endif //QT_NO_SIZEGRIP
