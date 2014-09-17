/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qbitmap.h"
#include "qevent.h"
#include "qstylepainter.h"
#include "qrubberband.h"
#include "qtimer.h"

#ifndef QT_NO_RUBBERBAND

#include "qstyle.h"
#include "qstyleoption.h"
#ifdef Q_WS_MAC
#  include <private/qt_mac_p.h>
#  include <private/qt_cocoa_helpers_mac_p.h>
#endif

#include <qdebug.h>

#include <private/qwidget_p.h>

QT_BEGIN_NAMESPACE

//### a rubberband window type would be a more elegant solution
#define RUBBERBAND_WINDOW_TYPE Qt::ToolTip

class QRubberBandPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QRubberBand)
public:
    QRect rect;
    QRubberBand::Shape shape;
    QRegion clipping;
    void updateMask();
};

/*!
    Initialize \a option with the values from this QRubberBand. This method
    is useful for subclasses when they need a QStyleOptionRubberBand, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QRubberBand::initStyleOption(QStyleOptionRubberBand *option) const
{
    if (!option)
        return;
    option->initFrom(this);
    option->shape = d_func()->shape;
#ifndef Q_WS_MAC
    option->opaque = true;
#else
    option->opaque = windowFlags() & RUBBERBAND_WINDOW_TYPE;
#endif
}

/*!
    \class QRubberBand
    \brief The QRubberBand class provides a rectangle or line that can
    indicate a selection or a boundary.

    A rubber band is often used to show a new bounding area (as in a
    QSplitter or a QDockWidget that is undocking). Historically this has
    been implemented using a QPainter and XOR, but this approach
    doesn't always work properly since rendering can happen in the
    window below the rubber band, but before the rubber band has been
    "erased".

    You can create a QRubberBand whenever you need to render a rubber band
    around a given area (or to represent a single line), then call
    setGeometry(), move() or resize() to position and size it. A common
    pattern is to do this in conjunction with mouse events. For example:

    \snippet doc/src/snippets/code/src_gui_widgets_qrubberband.cpp 0

    If you pass a parent to QRubberBand's constructor, the rubber band will
    display only inside its parent, but stays on top of other child widgets.
    If no parent is passed, QRubberBand will act as a top-level widget.

    Call show() to make the rubber band visible; also when the
    rubber band is not a top-level. Hiding or destroying
    the widget will make the rubber band disappear. The rubber band
    can be a \l Rectangle or a \l Line (vertical or horizontal),
    depending on the shape() it was given when constructed.
*/

// ### DOC: How about some nice convenience constructors?
//QRubberBand::QRubberBand(QRubberBand::Type t, const QRect &rect, QWidget *p)
//QRubberBand::QRubberBand(QRubberBand::Type t, int x, int y, int w, int h, QWidget *p)

/*!
    Constructs a rubber band of shape \a s, with parent \a p.

    By default a rectangular rubber band (\a s is \c Rectangle) will
    use a mask, so that a small border of the rectangle is all
    that is visible. Some styles (e.g., native Mac OS X) will
    change this and call QWidget::setWindowOpacity() to make a
    semi-transparent filled selection rectangle.
*/
QRubberBand::QRubberBand(Shape s, QWidget *p)
    : QWidget(*new QRubberBandPrivate, p, (p && p->windowType() != Qt::Desktop) ? Qt::Widget : RUBBERBAND_WINDOW_TYPE)
{
    Q_D(QRubberBand);
    d->shape = s;
    setAttribute(Qt::WA_TransparentForMouseEvents);
#ifndef Q_WS_WIN
    setAttribute(Qt::WA_NoSystemBackground);
#endif //Q_WS_WIN
    setAttribute(Qt::WA_WState_ExplicitShowHide);
    setVisible(false);
#ifdef Q_WS_MAC
    if (isWindow()) {
        createWinId();
        extern OSWindowRef qt_mac_window_for(const QWidget *); //qwidget_mac.cpp
        macWindowSetHasShadow(qt_mac_window_for(this), false);
    }
#endif
}

/*!
    Destructor.
*/
QRubberBand::~QRubberBand()
{
}

/*!
    \enum QRubberBand::Shape

    This enum specifies what shape a QRubberBand should have. This is
    a drawing hint that is passed down to the style system, and can be
    interpreted by each QStyle.

    \value Line A QRubberBand can represent a vertical or horizontal
                line. Geometry is still given in rect() and the line
                will fill the given geometry on most styles.

    \value Rectangle A QRubberBand can represent a rectangle. Some
                     styles will interpret this as a filled (often
                     semi-transparent) rectangle, or a rectangular
                     outline.
*/

/*!
  Returns the shape of this rubber band. The shape can only be set
  upon construction.
*/
QRubberBand::Shape QRubberBand::shape() const
{
    Q_D(const QRubberBand);
    return d->shape;
}

/*!
    \internal
*/
void QRubberBandPrivate::updateMask()
{
    Q_Q(QRubberBand);
    QStyleHintReturnMask mask;
    QStyleOptionRubberBand opt;
    q->initStyleOption(&opt);
    if (q->style()->styleHint(QStyle::SH_RubberBand_Mask, &opt, q, &mask)) {
        q->setMask(mask.region);
    } else {
        q->clearMask();
    }
}

/*!
    \reimp
*/
void QRubberBand::paintEvent(QPaintEvent *)
{
    QStylePainter painter(this);
    QStyleOptionRubberBand option;
    initStyleOption(&option);
    painter.drawControl(QStyle::CE_RubberBand, option);
}

/*!
    \reimp
*/
void QRubberBand::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::ParentChange:
        if (parent()) {
            setWindowFlags(windowFlags() & ~RUBBERBAND_WINDOW_TYPE);
        } else {
            setWindowFlags(windowFlags() | RUBBERBAND_WINDOW_TYPE);
        }
        break;
    default:
        break;
    }

    if (e->type() == QEvent::ZOrderChange)
        raise();
}

/*!
    \reimp
*/
void QRubberBand::showEvent(QShowEvent *e)
{
    raise();
    e->ignore();
}

/*!
    \reimp
*/
void QRubberBand::resizeEvent(QResizeEvent *)
{
    Q_D(QRubberBand);
    d->updateMask();
}

/*!
    \reimp
*/
void QRubberBand::moveEvent(QMoveEvent *)
{
    Q_D(QRubberBand);
    d->updateMask();
}

/*!
    \fn void QRubberBand::move(const QPoint &p);

    \overload

    Moves the rubberband to point \a p.

    \sa resize()
*/

/*!
    \fn void QRubberBand::move(int x, int y);

    Moves the rubberband to point (\a x, \a y).

    \sa resize()
*/

/*!
    \fn void QRubberBand::resize(const QSize &size);

    \overload

    Resizes the rubberband so that its new size is \a size.

    \sa move()
*/

/*!
    \fn void QRubberBand::resize(int width, int height);

    Resizes the rubberband so that its width is \a width, and its
    height is \a height.

    \sa move()
*/

/*!
    \fn void QRubberBand::setGeometry(const QRect &rect)

    Sets the geometry of the rubber band to \a rect, specified in the coordinate system
    of its parent widget.

    \sa QWidget::geometry
*/
void QRubberBand::setGeometry(const QRect &geom)
{
    QWidget::setGeometry(geom);
}

/*!
    \fn void QRubberBand::setGeometry(int x, int y, int width, int height)
    \overload

    Sets the geometry of the rubberband to the rectangle whose top-left corner lies at
    the point (\a x, \a y), and with dimensions specified by \a width and \a height.
    The geometry is specified in the parent widget's coordinate system.
*/

/*! \reimp */
bool QRubberBand::event(QEvent *e)
{
    return QWidget::event(e);
}

QT_END_NAMESPACE

#endif // QT_NO_RUBBERBAND
