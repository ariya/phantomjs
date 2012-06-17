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

#include "qfocusframe.h"
#include "qstyle.h"
#include "qbitmap.h"
#include "qstylepainter.h"
#include "qstyleoption.h"
#include "qdebug.h"
#include <private/qwidget_p.h>

QT_BEGIN_NAMESPACE

class QFocusFramePrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QFocusFrame)
    QWidget *widget;
    QWidget *frameParent;
    bool showFrameAboveWidget;
public:
    QFocusFramePrivate() {
        widget = 0;
        frameParent = 0;
        sendChildEvents = false;
        showFrameAboveWidget = false;
    }
    void updateSize();
    void update();
};

void QFocusFramePrivate::update()
{
    Q_Q(QFocusFrame);
    q->setParent(frameParent);
    updateSize();
    if (q->parentWidget()->rect().intersects(q->geometry())) {
        if (showFrameAboveWidget)
            q->raise();
        else
            q->stackUnder(widget);
        q->show();
    } else {
        q->hide();
    }
}

void QFocusFramePrivate::updateSize()
{
    Q_Q(QFocusFrame);
    if (!widget)
        return;

    int vmargin = q->style()->pixelMetric(QStyle::PM_FocusFrameVMargin),
        hmargin = q->style()->pixelMetric(QStyle::PM_FocusFrameHMargin);
    QPoint pos(widget->x(), widget->y());
    if (q->parentWidget() != widget->parentWidget())
        pos = widget->parentWidget()->mapTo(q->parentWidget(), pos);
    QRect geom(pos.x()-hmargin, pos.y()-vmargin,
               widget->width()+(hmargin*2), widget->height()+(vmargin*2));
    if(q->geometry() == geom)
        return;

    q->setGeometry(geom);
    QStyleHintReturnMask mask;
    QStyleOption opt;
    q->initStyleOption(&opt);
    if (q->style()->styleHint(QStyle::SH_FocusFrame_Mask, &opt, q, &mask))
        q->setMask(mask.region);
}

/*!
    Initialize \a option with the values from this QFocusFrame. This method is useful
    for subclasses when they need a QStyleOption, but don't want to fill
    in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QFocusFrame::initStyleOption(QStyleOption *option) const
{
    if (!option)
        return;

    option->initFrom(this);
}

/*!
    \class QFocusFrame
    \brief The QFocusFrame widget provides a focus frame which can be
    outside of a widget's normal paintable area.

    \ingroup basicwidgets


    Normally an application will not need to create its own
    QFocusFrame as QStyle will handle this detail for
    you. A style writer can optionally use a QFocusFrame to have a
    focus area outside of the widget's paintable geometry. In this way
    space need not be reserved for the widget to have focus but only
    set on a QWidget with QFocusFrame::setWidget. It is, however,
    legal to create your own QFocusFrame on a custom widget and set
    its geometry manually via QWidget::setGeometry however you will
    not get auto-placement when the focused widget changes size or
    placement.
*/

/*!
    Constructs a QFocusFrame.

    The focus frame will not monitor \a parent for updates but rather
    can be placed manually or by using QFocusFrame::setWidget. A
    QFocusFrame sets Qt::WA_NoChildEventsForParent attribute; as a
    result the parent will not receive a QEvent::ChildInserted event,
    this will make it possible to manually set the geometry of the
    QFocusFrame inside of a QSplitter or other child event monitoring
    widget.

    \sa QFocusFrame::setWidget()
*/

QFocusFrame::QFocusFrame(QWidget *parent)
    : QWidget(*new QFocusFramePrivate, parent, 0)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_NoChildEventsForParent, true);
    setAttribute(Qt::WA_AcceptDrops, style()->styleHint(QStyle::SH_FocusFrame_AboveWidget, 0, this));
}

/*!
    Destructor.
*/

QFocusFrame::~QFocusFrame()
{
}

/*!
  QFocusFrame will track changes to \a widget and resize itself automatically.
  If the monitored widget's parent changes, QFocusFrame will follow the widget
  and place itself around the widget automatically. If the monitored widget is deleted,
  QFocusFrame will set it to zero.

  \sa QFocusFrame::widget()
*/

void
QFocusFrame::setWidget(QWidget *widget)
{
    Q_D(QFocusFrame);

    if (style()->styleHint(QStyle::SH_FocusFrame_AboveWidget, 0, this))
        d->showFrameAboveWidget = true;
    else
        d->showFrameAboveWidget = false;

    if (widget == d->widget)
        return;
    if (d->widget) {
        // Remove event filters from the widget hierarchy.
        QWidget *p = d->widget;
        do {
            p->removeEventFilter(this);
            if (!d->showFrameAboveWidget || p == d->frameParent)
                break;
            p = p->parentWidget();
        }while (p);
    }
    if (widget && !widget->isWindow() && widget->parentWidget()->windowType() != Qt::SubWindow) {
        d->widget = widget;
        d->widget->installEventFilter(this);
        QWidget *p = widget->parentWidget();
        QWidget *prev = 0;
        if (d->showFrameAboveWidget) {
            // Find the right parent for the focus frame.
            while (p) {
                // Traverse the hirerarchy of the 'widget' for setting event filter.
                // During this if come across toolbar or a top level, use that
                // as the parent for the focus frame. If we find a scroll area
                // use its viewport as the parent.
                bool isScrollArea = false;
                if (p->isWindow() || p->inherits("QToolBar") || (isScrollArea = p->inherits("QAbstractScrollArea"))) {
                    d->frameParent = p;
                    // The previous one in the hierarchy will be the viewport.
                    if (prev && isScrollArea)
                        d->frameParent = prev;
                    break;
                } else {
                    p->installEventFilter(this);
                    prev = p;
                    p = p->parentWidget();
                }
            }
        } else {
            d->frameParent = p;
        }
        d->update();
    } else {
        d->widget = 0;
        hide();
    }
}

/*!
  Returns the currently monitored widget for automatically resize and
  update.

   \sa QFocusFrame::setWidget()
*/

QWidget *
QFocusFrame::widget() const
{
    Q_D(const QFocusFrame);
    return d->widget;
}


/*! \reimp */
void
QFocusFrame::paintEvent(QPaintEvent *)
{
    Q_D(QFocusFrame);
    QStylePainter p(this);
    QStyleOption option;
    initStyleOption(&option);
    int vmargin = style()->pixelMetric(QStyle::PM_FocusFrameVMargin);
    int hmargin = style()->pixelMetric(QStyle::PM_FocusFrameHMargin);
    QWidgetPrivate *wd = qt_widget_private(d->widget);
    QRect rect = wd->clipRect().adjusted(0, 0, hmargin*2, vmargin*2);
    p.setClipRect(rect);
    p.drawControl(QStyle::CE_FocusFrame, option);
}


/*! \reimp */
bool
QFocusFrame::eventFilter(QObject *o, QEvent *e)
{
    Q_D(QFocusFrame);
    if(o == d->widget) {
        switch(e->type()) {
        case QEvent::Move:
        case QEvent::Resize:
            d->updateSize();
            break;
        case QEvent::Hide:
        case QEvent::StyleChange:
            hide();
            break;
        case QEvent::ParentChange:
            if (d->showFrameAboveWidget) {
                QWidget *w = d->widget;
                setWidget(0);
                setWidget(w);
            } else {
                d->update();
            }
            break;
        case QEvent::Show:
            d->update();
            show();
            break;
        case QEvent::PaletteChange:
            setPalette(d->widget->palette());
            break;
        case QEvent::ZOrderChange:
            if (style()->styleHint(QStyle::SH_FocusFrame_AboveWidget, 0, this))
                raise();
            else
                stackUnder(d->widget);
            break;
        case QEvent::Destroy:
            setWidget(0);
            break;
        default:
            break;
        }
    } else if (d->showFrameAboveWidget) {
        // Handle changes in the parent widgets we are monitoring.
        switch(e->type()) {
        case QEvent::Move:
        case QEvent::Resize:
            d->updateSize();
            break;
        case QEvent::ZOrderChange:
            raise();
            break;
        default:
            break;
        }
    }
    return false;
}

/*! \reimp */
bool QFocusFrame::event(QEvent *e)
{
    return QWidget::event(e);
}

QT_END_NAMESPACE
