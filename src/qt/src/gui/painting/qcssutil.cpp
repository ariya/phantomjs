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

#include "qcssutil_p.h"
#include "private/qcssparser_p.h"
#include "qpainter.h"
#include <qmath.h>

#ifndef QT_NO_CSSPARSER

QT_BEGIN_NAMESPACE

using namespace QCss;

static QPen qPenFromStyle(const QBrush& b, qreal width, BorderStyle s)
{
    Qt::PenStyle ps = Qt::NoPen;

    switch (s) {
    case BorderStyle_Dotted:
        ps  = Qt::DotLine;
        break;
    case BorderStyle_Dashed:
        ps = width == 1 ? Qt::DotLine : Qt::DashLine;
        break;
    case BorderStyle_DotDash:
        ps = Qt::DashDotLine;
        break;
    case BorderStyle_DotDotDash:
        ps = Qt::DashDotDotLine;
        break;
    case BorderStyle_Inset:
    case BorderStyle_Outset:
    case BorderStyle_Solid:
        ps = Qt::SolidLine;
        break;
    default:
        break;
    }

    return QPen(b, width, ps, Qt::FlatCap);
}

void qDrawRoundedCorners(QPainter *p, qreal x1, qreal y1, qreal x2, qreal y2,
                         const QSizeF& r1, const QSizeF& r2,
                         Edge edge, BorderStyle s, QBrush c)
{
    const qreal pw = (edge == TopEdge || edge == BottomEdge) ? y2-y1 : x2-x1;
    if (s == BorderStyle_Double) {
        qreal wby3 = pw/3;
        switch (edge) {
        case TopEdge:
        case BottomEdge:
            qDrawRoundedCorners(p, x1, y1, x2, y1+wby3, r1, r2, edge, BorderStyle_Solid, c);
            qDrawRoundedCorners(p, x1, y2-wby3, x2, y2, r1, r2, edge, BorderStyle_Solid, c);
            break;
        case LeftEdge:
            qDrawRoundedCorners(p, x1, y1+1, x1+wby3, y2, r1, r2, LeftEdge, BorderStyle_Solid, c);
            qDrawRoundedCorners(p, x2-wby3, y1+1, x2, y2, r1, r2, LeftEdge, BorderStyle_Solid, c);
            break;
        case RightEdge:
            qDrawRoundedCorners(p, x1, y1+1, x1+wby3, y2, r1, r2, RightEdge, BorderStyle_Solid, c);
            qDrawRoundedCorners(p, x2-wby3, y1+1, x2, y2, r1, r2, RightEdge, BorderStyle_Solid, c);
            break;
        default:
            break;
        }
        return;
    } else if (s == BorderStyle_Ridge || s == BorderStyle_Groove) {
        BorderStyle s1, s2;
        if (s == BorderStyle_Groove) {
            s1 = BorderStyle_Inset;
            s2 = BorderStyle_Outset;
        } else {
            s1 = BorderStyle_Outset;
            s2 = BorderStyle_Inset;
        }
        int pwby2 = qRound(pw/2);
        switch (edge) {
        case TopEdge:
            qDrawRoundedCorners(p, x1, y1, x2, y1 + pwby2, r1, r2, TopEdge, s1, c);
            qDrawRoundedCorners(p, x1, y1 + pwby2, x2, y2, r1, r2, TopEdge, s2, c);
            break;
        case BottomEdge:
            qDrawRoundedCorners(p, x1, y1 + pwby2, x2, y2, r1, r2, BottomEdge, s1, c);
            qDrawRoundedCorners(p, x1, y1, x2, y2-pwby2, r1, r2, BottomEdge, s2, c);
            break;
        case LeftEdge:
            qDrawRoundedCorners(p, x1, y1, x1 + pwby2, y2, r1, r2, LeftEdge, s1, c);
            qDrawRoundedCorners(p, x1 + pwby2, y1, x2, y2, r1, r2, LeftEdge, s2, c);
            break;
        case RightEdge:
            qDrawRoundedCorners(p, x1 + pwby2, y1, x2, y2, r1, r2, RightEdge, s1, c);
            qDrawRoundedCorners(p, x1, y1, x2 - pwby2, y2, r1, r2, RightEdge, s2, c);
            break;
        default:
            break;
        }
    } else if ((s == BorderStyle_Outset && (edge == TopEdge || edge == LeftEdge))
            || (s == BorderStyle_Inset && (edge == BottomEdge || edge == RightEdge)))
            c = c.color().lighter();

    p->save();
    qreal pwby2 = pw/2;
    p->setBrush(Qt::NoBrush);
    QPen pen = qPenFromStyle(c, pw, s);
    pen.setCapStyle(Qt::SquareCap); // this eliminates the offby1 errors that we might hit below
    p->setPen(pen);
    switch (edge) {
    case TopEdge:
        if (!r1.isEmpty())
            p->drawArc(QRectF(x1 - r1.width() + pwby2, y1 + pwby2,
                              2*r1.width() - pw, 2*r1.height() - pw), 135*16, -45*16);
        if (!r2.isEmpty())
            p->drawArc(QRectF(x2 - r2.width() + pwby2, y1 + pwby2,
                       2*r2.width() - pw, 2*r2.height() - pw), 45*16, 45*16);
        break;
    case BottomEdge:
        if (!r1.isEmpty())
            p->drawArc(QRectF(x1 - r1.width() + pwby2, y2 - 2*r1.height() + pwby2,
                              2*r1.width() - pw, 2*r1.height() - pw), -90 * 16, -45 * 16);
        if (!r2.isEmpty())
            p->drawArc(QRectF(x2 - r2.width() + pwby2, y2 - 2*r2.height() + pwby2,
                       2*r2.width() - pw, 2*r2.height() - pw), -90 * 16, 45 * 16);
        break;
    case LeftEdge:
        if (!r1.isEmpty())
            p->drawArc(QRectF(x1 + pwby2, y1 - r1.height() + pwby2,
                       2*r1.width() - pw, 2*r1.height() - pw), 135*16, 45*16);
        if (!r2.isEmpty())
            p->drawArc(QRectF(x1 + pwby2, y2 - r2.height() + pwby2,
                       2*r2.width() - pw, 2*r2.height() - pw), 180*16, 45*16);
        break;
    case RightEdge:
        if (!r1.isEmpty())
            p->drawArc(QRectF(x2 - 2*r1.width() + pwby2, y1 - r1.height() + pwby2,
                       2*r1.width() - pw, 2*r1.height() - pw), 45*16, -45*16);
        if (!r2.isEmpty())
            p->drawArc(QRectF(x2 - 2*r2.width() + pwby2, y2 - r2.height() + pwby2,
                       2*r2.width() - pw, 2*r2.height() - pw), 315*16, 45*16);
        break;
    default:
        break;
    }
    p->restore();
}


void qDrawEdge(QPainter *p, qreal x1, qreal y1, qreal x2, qreal y2, qreal dw1, qreal dw2,
               QCss::Edge edge, QCss::BorderStyle style, QBrush c)
{
    p->save();
    const qreal width = (edge == TopEdge || edge == BottomEdge) ? (y2-y1) : (x2-x1);

    if (width <= 2 && style == BorderStyle_Double)
        style = BorderStyle_Solid;

    switch (style) {
    case BorderStyle_Inset:
    case BorderStyle_Outset:
        if ((style == BorderStyle_Outset && (edge == TopEdge || edge == LeftEdge))
            || (style == BorderStyle_Inset && (edge == BottomEdge || edge == RightEdge)))
            c = c.color().lighter();
        // fall through!
    case BorderStyle_Solid: {
        p->setPen(Qt::NoPen);
        p->setBrush(c);
        if (width == 1 || (dw1 == 0 && dw2 == 0)) {
            p->drawRect(QRectF(x1, y1, x2-x1, y2-y1));
        } else { // draw trapezoid
            QPolygonF quad;
            switch (edge) {
            case TopEdge:
                quad << QPointF(x1, y1) << QPointF(x1 + dw1, y2)
                     << QPointF(x2 - dw2, y2) << QPointF(x2, y1);
                break;
            case BottomEdge:
                quad << QPointF(x1 + dw1, y1) << QPointF(x1, y2)
                     << QPointF(x2, y2) << QPointF(x2 - dw2, y1);
                break;
            case LeftEdge:
                quad << QPointF(x1, y1) << QPointF(x1, y2)
                     << QPointF(x2, y2 - dw2) << QPointF(x2, y1 + dw1);
                break;
            case RightEdge:
                quad << QPointF(x1, y1 + dw1) << QPointF(x1, y2 - dw2)
                     << QPointF(x2, y2) << QPointF(x2, y1);
                break;
            default:
                break;
            }
            p->drawConvexPolygon(quad);
        }
        break;
    }
    case BorderStyle_Dotted:
    case BorderStyle_Dashed:
    case BorderStyle_DotDash:
    case BorderStyle_DotDotDash:
        p->setPen(qPenFromStyle(c, width, style));
        if (width == 1)
            p->drawLine(QLineF(x1, y1, x2 - 1, y2 - 1));
        else if (edge == TopEdge || edge == BottomEdge)
            p->drawLine(QLineF(x1 + width/2, (y1 + y2)/2, x2 - width/2, (y1 + y2)/2));
        else
            p->drawLine(QLineF((x1+x2)/2, y1 + width/2, (x1+x2)/2, y2 - width/2));
        break;

    case BorderStyle_Double: {
        int wby3 = qRound(width/3);
        int dw1by3 = qRound(dw1/3);
        int dw2by3 = qRound(dw2/3);
        switch (edge) {
        case TopEdge:
            qDrawEdge(p, x1, y1, x2, y1 + wby3, dw1by3, dw2by3, TopEdge, BorderStyle_Solid, c);
            qDrawEdge(p, x1 + dw1 - dw1by3, y2 - wby3, x2 - dw2 + dw1by3, y2,
                      dw1by3, dw2by3, TopEdge, BorderStyle_Solid, c);
            break;
        case LeftEdge:
            qDrawEdge(p, x1, y1, x1 + wby3, y2, dw1by3, dw2by3, LeftEdge, BorderStyle_Solid, c);
            qDrawEdge(p, x2 - wby3, y1 + dw1 - dw1by3, x2, y2 - dw2 + dw2by3, dw1by3, dw2by3,
                      LeftEdge, BorderStyle_Solid, c);
            break;
        case BottomEdge:
            qDrawEdge(p, x1 + dw1 - dw1by3, y1, x2 - dw2 + dw2by3, y1 + wby3, dw1by3, dw2by3,
                      BottomEdge, BorderStyle_Solid, c);
            qDrawEdge(p, x1, y2 - wby3, x2, y2, dw1by3, dw2by3, BottomEdge, BorderStyle_Solid, c);
            break;
        case RightEdge:
            qDrawEdge(p, x2 - wby3, y1, x2, y2, dw1by3, dw2by3, RightEdge, BorderStyle_Solid, c);
            qDrawEdge(p, x1, y1 + dw1 - dw1by3, x1 + wby3, y2 - dw2 + dw2by3, dw1by3, dw2by3,
                      RightEdge, BorderStyle_Solid, c);
            break;
        default:
            break;
        }
        break;
    }
    case BorderStyle_Ridge:
    case BorderStyle_Groove: {
        BorderStyle s1, s2;
        if (style == BorderStyle_Groove) {
            s1 = BorderStyle_Inset;
            s2 = BorderStyle_Outset;
        } else {
            s1 = BorderStyle_Outset;
            s2 = BorderStyle_Inset;
        }
        int dw1by2 = qFloor(dw1/2), dw2by2 = qFloor(dw2/2);
        int wby2 = qRound(width/2);
        switch (edge) {
        case TopEdge:
            qDrawEdge(p, x1, y1, x2, y1 + wby2, dw1by2, dw2by2, TopEdge, s1, c);
            qDrawEdge(p, x1 + dw1by2, y1 + wby2, x2 - dw2by2, y2, dw1by2, dw2by2, TopEdge, s2, c);
            break;
        case BottomEdge:
            qDrawEdge(p, x1, y1 + wby2, x2, y2, dw1by2, dw2by2, BottomEdge, s1, c);
            qDrawEdge(p, x1 + dw1by2, y1, x2 - dw2by2, y1 + wby2, dw1by2, dw2by2, BottomEdge, s2, c);
            break;
        case LeftEdge:
            qDrawEdge(p, x1, y1, x1 + wby2, y2, dw1by2, dw2by2, LeftEdge, s1, c);
            qDrawEdge(p, x1 + wby2, y1 + dw1by2, x2, y2 - dw2by2, dw1by2, dw2by2, LeftEdge, s2, c);
            break;
        case RightEdge:
            qDrawEdge(p, x1 + wby2, y1, x2, y2, dw1by2, dw2by2, RightEdge, s1, c);
            qDrawEdge(p, x1, y1 + dw1by2, x1 + wby2, y2 - dw2by2, dw1by2, dw2by2, RightEdge, s2, c);
            break;
        default:
            break;
        }
    }
    default:
        break;
    }
    p->restore();
}

void qNormalizeRadii(const QRect &br, const QSize *radii,
                     QSize *tlr, QSize *trr, QSize *blr, QSize *brr)
{
    *tlr = radii[0].expandedTo(QSize(0, 0));
    *trr = radii[1].expandedTo(QSize(0, 0));
    *blr = radii[2].expandedTo(QSize(0, 0));
    *brr = radii[3].expandedTo(QSize(0, 0));
    if (tlr->width() + trr->width() > br.width())
        *tlr = *trr = QSize(0, 0);
    if (blr->width() + brr->width() > br.width())
        *blr = *brr = QSize(0, 0);
    if (tlr->height() + blr->height() > br.height())
        *tlr = *blr = QSize(0, 0);
    if (trr->height() + brr->height() > br.height())
        *trr = *brr = QSize(0, 0);
}

// Determines if Edge e1 draws over Edge e2. Depending on this trapezoids or rectanges are drawn
static bool paintsOver(const QCss::BorderStyle *styles, const QBrush *colors, QCss::Edge e1, QCss::Edge e2)
{
    QCss::BorderStyle s1 = styles[e1];
    QCss::BorderStyle s2 = styles[e2];

    if (s2 == BorderStyle_None || colors[e2] == Qt::transparent)
        return true;

    if ((s1 == BorderStyle_Solid && s2 == BorderStyle_Solid) && (colors[e1] == colors[e2]))
        return true;

    return false;
}

void qDrawBorder(QPainter *p, const QRect &rect, const QCss::BorderStyle *styles,
                 const int *borders, const QBrush *colors, const QSize *radii)
{
    const QRectF br(rect);
    QSize tlr, trr, blr, brr;
    qNormalizeRadii(rect, radii, &tlr, &trr, &blr, &brr);

    // Drawn in increasing order of precendence
    if (styles[BottomEdge] != BorderStyle_None && borders[BottomEdge] > 0) {
        qreal dw1 = (blr.width() || paintsOver(styles, colors, BottomEdge, LeftEdge)) ? 0 : borders[LeftEdge];
        qreal dw2 = (brr.width() || paintsOver(styles, colors, BottomEdge, RightEdge)) ? 0 : borders[RightEdge];
        qreal x1 = br.x() + blr.width();
        qreal y1 = br.y() + br.height() - borders[BottomEdge];
        qreal x2 = br.x() + br.width() - brr.width();
        qreal y2 = br.y() + br.height() ;

        qDrawEdge(p, x1, y1, x2, y2, dw1, dw2, BottomEdge, styles[BottomEdge], colors[BottomEdge]);
        if (blr.width() || brr.width())
            qDrawRoundedCorners(p, x1, y1, x2, y2, blr, brr, BottomEdge, styles[BottomEdge], colors[BottomEdge]);
    }
    if (styles[RightEdge] != BorderStyle_None && borders[RightEdge] > 0) {
        qreal dw1 = (trr.height() || paintsOver(styles, colors, RightEdge, TopEdge)) ? 0 : borders[TopEdge];
        qreal dw2 = (brr.height() || paintsOver(styles, colors, RightEdge, BottomEdge)) ? 0 : borders[BottomEdge];
        qreal x1 = br.x() + br.width() - borders[RightEdge];
        qreal y1 = br.y() + trr.height();
        qreal x2 = br.x() + br.width();
        qreal y2 = br.y() + br.height() - brr.height();

        qDrawEdge(p, x1, y1, x2, y2, dw1, dw2, RightEdge, styles[RightEdge], colors[RightEdge]);
        if (trr.height() || brr.height())
            qDrawRoundedCorners(p, x1, y1, x2, y2, trr, brr, RightEdge, styles[RightEdge], colors[RightEdge]);
    }
    if (styles[LeftEdge] != BorderStyle_None && borders[LeftEdge] > 0) {
        qreal dw1 = (tlr.height() || paintsOver(styles, colors, LeftEdge, TopEdge)) ? 0 : borders[TopEdge];
        qreal dw2 = (blr.height() || paintsOver(styles, colors, LeftEdge, BottomEdge)) ? 0 : borders[BottomEdge];
        qreal x1 = br.x();
        qreal y1 = br.y() + tlr.height();
        qreal x2 = br.x() + borders[LeftEdge];
        qreal y2 = br.y() + br.height() - blr.height();

        qDrawEdge(p, x1, y1, x2, y2, dw1, dw2, LeftEdge, styles[LeftEdge], colors[LeftEdge]);
        if (tlr.height() || blr.height())
            qDrawRoundedCorners(p, x1, y1, x2, y2, tlr, blr, LeftEdge, styles[LeftEdge], colors[LeftEdge]);
    }
    if (styles[TopEdge] != BorderStyle_None && borders[TopEdge] > 0) {
        qreal dw1 = (tlr.width() || paintsOver(styles, colors, TopEdge, LeftEdge)) ? 0 : borders[LeftEdge];
        qreal dw2 = (trr.width() || paintsOver(styles, colors, TopEdge, RightEdge)) ? 0 : borders[RightEdge];
        qreal x1 = br.x() + tlr.width();
        qreal y1 = br.y();
        qreal x2 = br.left() + br.width() - trr.width();
        qreal y2 = br.y() + borders[TopEdge];

        qDrawEdge(p, x1, y1, x2, y2, dw1, dw2, TopEdge, styles[TopEdge], colors[TopEdge]);
        if (tlr.width() || trr.width())
            qDrawRoundedCorners(p, x1, y1, x2, y2, tlr, trr, TopEdge, styles[TopEdge], colors[TopEdge]);
    }
}

#endif //QT_NO_CSSPARSER

QT_END_NAMESPACE
