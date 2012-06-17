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

#include <private/qt_x11_p.h>

#include <limits.h>

QT_BEGIN_NAMESPACE

QRegion::QRegionData QRegion::shared_empty = {Q_BASIC_ATOMIC_INITIALIZER(1), 0, 0, 0};

void QRegion::updateX11Region() const
{
    d->rgn = XCreateRegion();
    if (!d->qt_rgn)
        return;

    int n = d->qt_rgn->numRects;
    const QRect *rect = (n == 1 ? &d->qt_rgn->extents : d->qt_rgn->rects.constData());
    while (n--) {
        XRectangle r;
        r.x = qMax(SHRT_MIN, rect->x());
        r.y = qMax(SHRT_MIN, rect->y());
        r.width = qMin((int)USHRT_MAX, rect->width());
        r.height = qMin((int)USHRT_MAX, rect->height());
        XUnionRectWithRegion(&r, d->rgn, d->rgn);
        ++rect;
    }
}

void *QRegion::clipRectangles(int &num) const
{
    if (!d->xrectangles && !(d == &shared_empty || d->qt_rgn->numRects == 0)) {
        XRectangle *r = static_cast<XRectangle*>(malloc(d->qt_rgn->numRects * sizeof(XRectangle)));
        d->xrectangles = r;
        int n = d->qt_rgn->numRects;
        const QRect *rect = (n == 1 ? &d->qt_rgn->extents : d->qt_rgn->rects.constData());
        while (n--) {
            r->x = qMax(SHRT_MIN, rect->x());
            r->y = qMax(SHRT_MIN, rect->y());
            r->width = qMin((int)USHRT_MAX, rect->width());
            r->height = qMin((int)USHRT_MAX, rect->height());
            ++r;
            ++rect;
        }
    }
    if (d == &shared_empty || d->qt_rgn->numRects == 0)
        num = 0;
    else
        num = d->qt_rgn->numRects;
    return d->xrectangles;
}

QT_END_NAMESPACE
