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

#include "qframe.h"
#include "qbitmap.h"
#include "qdrawutil.h"
#include "qevent.h"
#include "qpainter.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qapplication.h"

#include "qframe_p.h"

QT_BEGIN_NAMESPACE

QFramePrivate::QFramePrivate()
    : frect(QRect(0, 0, 0, 0)),
      frameStyle(QFrame::NoFrame | QFrame::Plain),
      lineWidth(1),
      midLineWidth(0),
      frameWidth(0),
      leftFrameWidth(0), rightFrameWidth(0),
      topFrameWidth(0), bottomFrameWidth(0)
{
}

inline void QFramePrivate::init()
{
    setLayoutItemMargins(QStyle::SE_FrameLayoutItem);
}

/*!
    \class QFrame
    \brief The QFrame class is the base class of widgets that can have a frame.

    \ingroup abstractwidgets


    QMenu uses this to "raise" the menu above the surrounding
    screen. QProgressBar has a "sunken" look. QLabel has a flat look.
    The frames of widgets like these can be changed.

    \snippet doc/src/snippets/code/src_gui_widgets_qframe.cpp 0

    The QFrame class can also be used directly for creating simple
    placeholder frames without any contents.

    The frame style is specified by a \l{QFrame::Shape}{frame shape} and
    a \l{QFrame::Shadow}{shadow style} that is used to visually separate
    the frame from surrounding widgets. These properties can be set
    together using the setFrameStyle() function and read with frameStyle().

    The frame shapes are \l NoFrame, \l Box, \l Panel, \l StyledPanel,
    HLine and \l VLine; the shadow styles are \l Plain, \l Raised and
    \l Sunken.

    A frame widget has three attributes that describe the thickness of the
    border: \l lineWidth, \l midLineWidth, and \l frameWidth.

    \list
    \o The line width is the width of the frame border. It can be modified
       to customize the frame's appearance.

    \o The mid-line width specifies the width of an extra line in the
       middle of the frame, which uses a third color to obtain a special
       3D effect. Notice that a mid-line is only drawn for \l Box, \l
       HLine and \l VLine frames that are raised or sunken.

    \o The frame width is determined by the frame style, and the frameWidth()
       function is used to obtain the value defined for the style used.
    \endlist

    The margin between the frame and the contents of the frame can be
    customized with the QWidget::setContentsMargins() function.

    \target picture
    This table shows some of the combinations of styles and line widths:

    \image frames.png Table of frame styles
*/


/*!
    \enum QFrame::Shape

    This enum type defines the shapes of frame available.

    \value NoFrame  QFrame draws nothing
    \value Box  QFrame draws a box around its contents
    \value Panel  QFrame draws a panel to make the contents appear
    raised or sunken
    \value StyledPanel  draws a rectangular panel with a look that
    depends on the current GUI style. It can be raised or sunken.
    \value HLine  QFrame draws a horizontal line that frames nothing
    (useful as separator)
    \value VLine  QFrame draws a vertical line that frames nothing
    (useful as separator)
    \value WinPanel draws a rectangular panel that can be raised or
    sunken like those in Windows 2000. Specifying this shape sets
    the line width to 2 pixels. WinPanel is provided for compatibility.
    For GUI style independence we recommend using StyledPanel instead.

    \omitvalue GroupBoxPanel
    \omitvalue ToolBarPanel
    \omitvalue MenuBarPanel
    \omitvalue PopupPanel
    \omitvalue LineEditPanel
    \omitvalue TabWidgetPanel

    When it does not call QStyle, Shape interacts with QFrame::Shadow,
    the lineWidth() and the midLineWidth() to create the total result.
    See the picture of the frames in the main class documentation.

    \sa QFrame::Shadow QFrame::style() QStyle::drawPrimitive()
*/


/*!
    \enum QFrame::Shadow

    This enum type defines the types of shadow that are used to give
    a 3D effect to frames.

    \value Plain  the frame and contents appear level with the
    surroundings; draws using the palette QPalette::WindowText color
    (without any 3D effect)

    \value Raised the frame and contents appear raised; draws a 3D
    raised line using the light and dark colors of the current color
    group
    \value Sunken the frame and contents appear sunken; draws a 3D
    sunken line using the light and dark colors of the current color
    group

    Shadow interacts with QFrame::Shape, the lineWidth() and the
    midLineWidth(). See the picture of the frames in the main class
    documentation.

    \sa QFrame::Shape lineWidth() midLineWidth()
*/

/*!
    \enum QFrame::StyleMask

    This enum defines two constants that can be used to extract the
    two components of frameStyle():

    \value Shadow_Mask The \l Shadow part of frameStyle()
    \value Shape_Mask  The \l Shape part of frameStyle()

    \omitvalue MShadow
    \omitvalue MShape

    Normally, you don't need to use these, since frameShadow() and
    frameShape() already extract the \l Shadow and the \l Shape parts
    of frameStyle().

    \sa frameStyle(), setFrameStyle()
*/

/*!
    Constructs a frame widget with frame style \l NoFrame and a
    1-pixel frame width.

    The \a parent and \a f arguments are passed to the QWidget
    constructor.
*/

QFrame::QFrame(QWidget* parent, Qt::WindowFlags f)
    : QWidget(*new QFramePrivate, parent, f)
{
    Q_D(QFrame);
    d->init();
}

/*! \internal */
QFrame::QFrame(QFramePrivate &dd, QWidget* parent, Qt::WindowFlags f)
    : QWidget(dd, parent, f)
{
    Q_D(QFrame);
    d->init();
}

#ifdef QT3_SUPPORT
/*!
    Use one of the constructors that doesn't take the \a name
    argument and then use setObjectName() instead.
*/
QFrame::QFrame(QWidget *parent, const char *name, Qt::WindowFlags f)
    : QWidget(*new QFramePrivate, parent, f)
{
    Q_D(QFrame);
    setObjectName(QString::fromAscii(name));
    d->init();
}
#endif

/*!
  Destroys the frame.
 */
QFrame::~QFrame()
{
}

/*!
    Returns the frame style.

    The default value is QFrame::Plain.

    \sa setFrameStyle(), frameShape(), frameShadow()
*/
int QFrame::frameStyle() const
{
    Q_D(const QFrame);
    return d->frameStyle;
}

/*!
    \property QFrame::frameShape
    \brief the frame shape value from the frame style

    \sa frameStyle(), frameShadow()
*/

QFrame::Shape QFrame::frameShape() const
{
    Q_D(const QFrame);
    return (Shape) (d->frameStyle & Shape_Mask);
}

void QFrame::setFrameShape(QFrame::Shape s)
{
    Q_D(QFrame);
    setFrameStyle((d->frameStyle & Shadow_Mask) | s);
}


/*!
    \property QFrame::frameShadow
    \brief the frame shadow value from the frame style

    \sa frameStyle(), frameShape()
*/
QFrame::Shadow QFrame::frameShadow() const
{
    Q_D(const QFrame);
    return (Shadow) (d->frameStyle & Shadow_Mask);
}

void QFrame::setFrameShadow(QFrame::Shadow s)
{
    Q_D(QFrame);
    setFrameStyle((d->frameStyle & Shape_Mask) | s);
}

/*!
    Sets the frame style to \a style.

    The \a style is the bitwise OR between a frame shape and a frame
    shadow style. See the picture of the frames in the main class
    documentation.

    The frame shapes are given in \l{QFrame::Shape} and the shadow
    styles in \l{QFrame::Shadow}.

    If a mid-line width greater than 0 is specified, an additional
    line is drawn for \l Raised or \l Sunken \l Box, \l HLine, and \l
    VLine frames. The mid-color of the current color group is used for
    drawing middle lines.

    \sa frameStyle()
*/

void QFrame::setFrameStyle(int style)
{
    Q_D(QFrame);
    if (!testAttribute(Qt::WA_WState_OwnSizePolicy)) {
        QSizePolicy sp;

        switch (style & Shape_Mask) {
        case HLine:
            sp = QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed, QSizePolicy::Line);
            break;
        case VLine:
            sp = QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum, QSizePolicy::Line);
            break;
        default:
            sp = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred, QSizePolicy::Frame);
        }
        setSizePolicy(sp);
        setAttribute(Qt::WA_WState_OwnSizePolicy, false);
    }
    d->frameStyle = (short)style;
    update();
    d->updateFrameWidth();
}

/*!
    \property QFrame::lineWidth
    \brief the line width

    Note that the \e total line width for frames used as separators
    (\l HLine and \l VLine) is specified by \l frameWidth.

    The default value is 1.

    \sa midLineWidth, frameWidth
*/

void QFrame::setLineWidth(int w)
{
    Q_D(QFrame);
    if (short(w) == d->lineWidth)
        return;
    d->lineWidth = short(w);
    d->updateFrameWidth();
}

int QFrame::lineWidth() const
{
    Q_D(const QFrame);
    return d->lineWidth;
}

/*!
    \property QFrame::midLineWidth
    \brief the width of the mid-line

    The default value is 0.

    \sa lineWidth, frameWidth
*/

void QFrame::setMidLineWidth(int w)
{
    Q_D(QFrame);
    if (short(w) == d->midLineWidth)
        return;
    d->midLineWidth = short(w);
    d->updateFrameWidth();
}

int QFrame::midLineWidth() const
{
    Q_D(const QFrame);
    return d->midLineWidth;
}

/*!
  \internal
  Updates the frame widths from the style.
*/
void QFramePrivate::updateStyledFrameWidths()
{
    Q_Q(const QFrame);
    QStyleOptionFrameV3 opt;
    opt.initFrom(q);
    opt.lineWidth = lineWidth;
    opt.midLineWidth = midLineWidth;
    opt.frameShape = QFrame::Shape(frameStyle & QFrame::Shape_Mask);

    QRect cr = q->style()->subElementRect(QStyle::SE_ShapedFrameContents, &opt, q);
    leftFrameWidth = cr.left() - opt.rect.left();
    topFrameWidth = cr.top() - opt.rect.top();
    rightFrameWidth = opt.rect.right() - cr.right(),
    bottomFrameWidth = opt.rect.bottom() - cr.bottom();
    frameWidth = qMax(qMax(leftFrameWidth, rightFrameWidth),
                      qMax(topFrameWidth, bottomFrameWidth));
}

/*!
  \internal
  Updated the frameWidth parameter.
*/

void QFramePrivate::updateFrameWidth()
{
    Q_Q(QFrame);
    QRect fr = q->frameRect();
    updateStyledFrameWidths();
    q->setFrameRect(fr);
    setLayoutItemMargins(QStyle::SE_FrameLayoutItem);
}

/*!
    \property QFrame::frameWidth
    \brief the width of the frame that is drawn.

    Note that the frame width depends on the \l{QFrame::setFrameStyle()}{frame style},
    not only the line width and the mid-line width. For example, the style specified
    by \l NoFrame always has a frame width of 0, whereas the style \l Panel has a
    frame width equivalent to the line width.

    \sa lineWidth(), midLineWidth(), frameStyle()
*/
int QFrame::frameWidth() const
{
    Q_D(const QFrame);
    return d->frameWidth;
}


/*!
    \property QFrame::frameRect
    \brief the frame's rectangle

    The frame's rectangle is the rectangle the frame is drawn in. By
    default, this is the entire widget. Setting the rectangle does
    does \e not cause a widget update. The frame rectangle is
    automatically adjusted when the widget changes size.

    If you set the rectangle to a null rectangle (for example,
    QRect(0, 0, 0, 0)), then the resulting frame rectangle is
    equivalent to the \link QWidget::rect() widget rectangle\endlink.
*/

QRect QFrame::frameRect() const
{
    Q_D(const QFrame);
    QRect fr = contentsRect();
    fr.adjust(-d->leftFrameWidth, -d->topFrameWidth, d->rightFrameWidth, d->bottomFrameWidth);
    return fr;
}

void QFrame::setFrameRect(const QRect &r)
{
    Q_D(QFrame);
    QRect cr = r.isValid() ? r : rect();
    cr.adjust(d->leftFrameWidth, d->topFrameWidth, -d->rightFrameWidth, -d->bottomFrameWidth);
    setContentsMargins(cr.left(), cr.top(), rect().right() - cr.right(), rect().bottom() - cr.bottom());
}

/*!\reimp
*/
QSize QFrame::sizeHint() const
{
    Q_D(const QFrame);
    //   Returns a size hint for the frame - for HLine and VLine
    //   shapes, this is stretchable one way and 3 pixels wide the
    //   other.  For other shapes, QWidget::sizeHint() is used.
    switch (d->frameStyle & Shape_Mask) {
    case HLine:
        return QSize(-1,3);
    case VLine:
        return QSize(3,-1);
    default:
        return QWidget::sizeHint();
    }
}

/*!\reimp
*/

void QFrame::paintEvent(QPaintEvent *)
{
    QPainter paint(this);
    drawFrame(&paint);
}

/*!
    \internal

    Mostly for the sake of Q3Frame
 */
void QFrame::drawFrame(QPainter *p)
{
    Q_D(QFrame);
    QStyleOptionFrameV3 opt;
    opt.init(this);
    int frameShape  = d->frameStyle & QFrame::Shape_Mask;
    int frameShadow = d->frameStyle & QFrame::Shadow_Mask;
    opt.frameShape = Shape(int(opt.frameShape) | frameShape);
    opt.rect = frameRect();
    switch (frameShape) {
        case QFrame::Box:
        case QFrame::HLine:
        case QFrame::VLine:
        case QFrame::StyledPanel:
        case QFrame::Panel:
            opt.lineWidth = d->lineWidth;
            opt.midLineWidth = d->midLineWidth;
            break;
        default:
            // most frame styles do not handle customized line and midline widths
            // (see updateFrameWidth()).
            opt.lineWidth = d->frameWidth;
            break;
    }

    if (frameShadow == Sunken)
        opt.state |= QStyle::State_Sunken;
    else if (frameShadow == Raised)
        opt.state |= QStyle::State_Raised;

    style()->drawControl(QStyle::CE_ShapedFrame, &opt, p, this);
}


/*!\reimp
 */
void QFrame::changeEvent(QEvent *ev)
{
    Q_D(QFrame);
    if (ev->type() == QEvent::StyleChange
#ifdef Q_WS_MAC
            || ev->type() == QEvent::MacSizeChange
#endif
            )
        d->updateFrameWidth();
    QWidget::changeEvent(ev);
}

/*! \reimp */
bool QFrame::event(QEvent *e)
{
    if (e->type() == QEvent::ParentChange)
        d_func()->updateFrameWidth();
    bool result = QWidget::event(e);
    //this has to be done after the widget has been polished
    if (e->type() == QEvent::Polish)
        d_func()->updateFrameWidth();
    return result;
}

QT_END_NAMESPACE
