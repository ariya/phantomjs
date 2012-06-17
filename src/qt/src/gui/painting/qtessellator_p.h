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

#ifndef QTESSELATOR_P_H
#define QTESSELATOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qpoint.h>
#include <qrect.h>

QT_BEGIN_NAMESPACE

class QTessellatorPrivate;

typedef int Q27Dot5;
#define Q27Dot5ToDouble(i) ((i)/32.)
#define FloatToQ27Dot5(i) (int)((i) * 32)
#define IntToQ27Dot5(i) ((i) << 5)
#define Q27Dot5ToXFixed(i) ((i) << 11)
#define Q27Dot5Factor 32

class Q_GUI_EXPORT QTessellator {
public:
    QTessellator();
    virtual ~QTessellator();

    QRectF tessellate(const QPointF *points, int nPoints);
    void tessellateConvex(const QPointF *points, int nPoints);
    void tessellateRect(const QPointF &a, const QPointF &b, qreal width);

    void setWinding(bool w);

    struct Vertex {
        Q27Dot5 x;
        Q27Dot5 y;
    };
    struct Trapezoid {
        Q27Dot5 top;
        Q27Dot5 bottom;
        const Vertex *topLeft;
        const Vertex *bottomLeft;
        const Vertex *topRight;
        const Vertex *bottomRight;
    };
    virtual void addTrap(const Trapezoid &trap) = 0;

private:
    friend class QTessellatorPrivate;
    QTessellatorPrivate *d;
};

QT_END_NAMESPACE

#endif
