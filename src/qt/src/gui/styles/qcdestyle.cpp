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

#include "qcdestyle.h"

#if !defined(QT_NO_STYLE_CDE) || defined(QT_PLUGIN)

#include "qmenu.h"
#include "qapplication.h"
#include "qpainter.h"
#include "qdrawutil.h"
#include "qpixmap.h"
#include "qpalette.h"
#include "qwidget.h"
#include "qpushbutton.h"
#include "qscrollbar.h"
#include "qtabbar.h"
#include "qtabwidget.h"
#include "qlistview.h"
#include "qsplitter.h"
#include "qslider.h"
#include "qcombobox.h"
#include "qlineedit.h"
#include "qprogressbar.h"
#include "qimage.h"
#include "qfocusframe.h"
#include "qpainterpath.h"
#include "qdebug.h"
#include <limits.h>

QT_BEGIN_NAMESPACE

/*!
    \class QCDEStyle
    \brief The QCDEStyle class provides a CDE look and feel.

    \ingroup appearance

    This style provides a slightly improved Motif look similar to some
    versions of the Common Desktop Environment (CDE). The main
    differences are thinner frames and more modern radio buttons and
    checkboxes. Together with a dark background and a bright
    text/foreground color, the style looks quite attractive (at least
    for Motif fans).

    Note that most of the functions provided by QCDEStyle are
    reimplementations of QStyle functions; see QStyle for their
    documentation. QCDEStyle provides overloads for drawControl() and
    drawPrimitive() which are documented here.

    \img qcdestyle.png
    \sa QWindowsXPStyle, QMacStyle, QWindowsStyle, QPlastiqueStyle, QMotifStyle
*/

/*!
    Constructs a QCDEStyle.

    If \a useHighlightCols is false (the default), then the style will
    polish the application's color palette to emulate the Motif way of
    highlighting, which is a simple inversion between the base and the
    text color.
*/
QCDEStyle::QCDEStyle(bool useHighlightCols)
    : QMotifStyle(useHighlightCols)
{
}

/*!
    Destroys the style.
*/
QCDEStyle::~QCDEStyle()
{
}


/*!\reimp
*/
int QCDEStyle::pixelMetric(PixelMetric metric, const QStyleOption *option,
                           const QWidget *widget) const
/*
int QCDEStyle::pixelMetric(PixelMetric metric, const QStyleOption *option,
                           const QWidget *widget) const
                           */
{
    int ret = 0;

    switch(metric) {
    case PM_MenuBarPanelWidth:
    case PM_DefaultFrameWidth:
    case PM_FocusFrameVMargin:
    case PM_FocusFrameHMargin:
    case PM_MenuPanelWidth:
    case PM_SpinBoxFrameWidth:
    case PM_MenuBarVMargin:
    case PM_MenuBarHMargin:
    case PM_DockWidgetFrameWidth:
        ret = 1;
        break;
    case PM_ScrollBarExtent:
        ret = 13;
        break;
    default:
        ret = QMotifStyle::pixelMetric(metric, option, widget);
        break;
    }
    return ret;
}

/*!
    \reimp
*/
void QCDEStyle::drawControl(ControlElement element, const QStyleOption *opt, QPainter *p,
                            const QWidget *widget) const
{

    switch(element) {
    case CE_MenuBarItem: {
        if (opt->state & State_Selected)  // active item
            qDrawShadePanel(p, opt->rect, opt->palette, true, 1,
                            &opt->palette.brush(QPalette::Button));
        else  // other item
            p->fillRect(opt->rect, opt->palette.brush(QPalette::Button));
        QCommonStyle::drawControl(element, opt, p, widget);
        break; }
    case CE_RubberBand: {
        p->save();
        p->setClipping(false);
        QPainterPath path;
        path.addRect(opt->rect);
        path.addRect(opt->rect.adjusted(2, 2, -2, -2));
        p->fillPath(path, opt->palette.color(QPalette::Active, QPalette::Text));
        p->restore();
        break; }
    default:
        QMotifStyle::drawControl(element, opt, p, widget);
    break;
    }
}

/*!
    \reimp
*/
void QCDEStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                        const QWidget *widget) const
{
    switch(pe) {
    case PE_IndicatorCheckBox: {
        bool down = opt->state & State_Sunken;
        bool on = opt->state & State_On;
        bool showUp = !(down ^ on);
        QBrush fill = (showUp || (opt->state & State_NoChange)) ? opt->palette.brush(QPalette::Button) : opt->palette.brush(QPalette::Mid);
        qDrawShadePanel(p, opt->rect, opt->palette, !showUp, pixelMetric(PM_DefaultFrameWidth), &opt->palette.brush(QPalette::Button));

        if (on || (opt->state & State_NoChange)) {
            QRect r = opt->rect;
            QPolygon a(7 * 2);
            int i, xx, yy;
            xx = r.x() + 3;
            yy = r.y() + 5;
            if (opt->rect.width() <= 9) {
                // When called from CE_MenuItem in QMotifStyle
                xx -= 2;
                yy -= 2;
            }

            for (i = 0; i < 3; i++) {
                a.setPoint(2 * i, xx, yy);
                a.setPoint(2 * i + 1, xx, yy + 2);
                xx++; yy++;
            }
            yy -= 2;
            for (i = 3; i < 7; i++) {
                a.setPoint(2 * i, xx, yy);
                a.setPoint(2 * i + 1, xx, yy + 2);
                xx++; yy--;
            }
            if (opt->state & State_NoChange)
                p->setPen(opt->palette.dark().color());
            else
                p->setPen(opt->palette.foreground().color());
            p->drawPolyline(a);
        }
        if (!(opt->state & State_Enabled) && styleHint(SH_DitherDisabledText))
            p->fillRect(opt->rect, QBrush(p->background().color(), Qt::Dense5Pattern));
    } break;
    case PE_IndicatorRadioButton:
        {
            QRect r = opt->rect;
#define INTARRLEN(x) sizeof(x)/(sizeof(int)*2)
            static const int pts1[] = {              // up left  lines
                1,9, 1,8, 0,7, 0,4, 1,3, 1,2, 2,1, 3,1, 4,0, 7,0, 8,1, 9,1 };
            static const int pts4[] = {              // bottom right  lines
                2,10, 3,10, 4,11, 7,11, 8,10, 9,10, 10,9, 10,8, 11,7,
                11,4, 10,3, 10,2 };
            static const int pts5[] = {              // inner fill
                4,2, 7,2, 9,4, 9,7, 7,9, 4,9, 2,7, 2,4 };
            bool down = opt->state & State_Sunken;
            bool on = opt->state & State_On;
            QPolygon a(INTARRLEN(pts1), pts1);

            //center when rect is larger than indicator size
            int xOffset = 0;
            int yOffset = 0;
            int indicatorWidth = pixelMetric(PM_ExclusiveIndicatorWidth);
            int indicatorHeight = pixelMetric(PM_ExclusiveIndicatorWidth);
            if (r.width() > indicatorWidth)
                xOffset += (r.width() - indicatorWidth)/2;
            if (r.height() > indicatorHeight)
                yOffset += (r.height() - indicatorHeight)/2;
            p->translate(xOffset, yOffset);

            a.translate(r.x(), r.y());
            QPen oldPen = p->pen();
            QBrush oldBrush = p->brush();
            p->setPen((down || on) ? opt->palette.dark().color() : opt->palette.light().color());
            p->drawPolyline(a);
            a.setPoints(INTARRLEN(pts4), pts4);
            a.translate(r.x(), r.y());
            p->setPen((down || on) ? opt->palette.light().color() : opt->palette.dark().color());
            p->drawPolyline(a);
            a.setPoints(INTARRLEN(pts5), pts5);
            a.translate(r.x(), r.y());
            QColor fillColor = on ? opt->palette.dark().color() : opt->palette.background().color();
            p->setPen(fillColor);
            p->setBrush(on ? opt->palette.brush(QPalette::Dark) :
                         opt->palette.brush(QPalette::Window));
            p->drawPolygon(a);
            if (!(opt->state & State_Enabled) && styleHint(SH_DitherDisabledText))
                p->fillRect(opt->rect, QBrush(p->background().color(), Qt::Dense5Pattern));
            p->setPen(oldPen);
            p->setBrush(oldBrush);

            p->translate(-xOffset, -yOffset);

        } break;
    default:
        QMotifStyle::drawPrimitive(pe, opt, p, widget);
    }
}

/*!\reimp*/
QPalette QCDEStyle::standardPalette() const
{
    QColor background(0xb6, 0xb6, 0xcf);
    QColor light = background.lighter();
    QColor mid = background.darker(150);
    QColor dark = background.darker();
    QPalette palette(Qt::black, background, light, dark, mid, Qt::black, Qt::white);
    palette.setBrush(QPalette::Disabled, QPalette::WindowText, dark);
    palette.setBrush(QPalette::Disabled, QPalette::Text, dark);
    palette.setBrush(QPalette::Disabled, QPalette::ButtonText, dark);
    palette.setBrush(QPalette::Disabled, QPalette::Base, background);
    return palette;
}

/*!
    \internal
*/
QIcon QCDEStyle::standardIconImplementation(StandardPixmap standardIcon, const QStyleOption *opt,
                                            const QWidget *widget) const
{
    return QMotifStyle::standardIconImplementation(standardIcon, opt, widget);
}

QT_END_NAMESPACE

#endif
