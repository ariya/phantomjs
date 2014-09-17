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

#ifndef QCOSMETICSTROKER_P_H
#define QCOSMETICSTROKER_P_H

#include <private/qdrawhelper_p.h>
#include <private/qvectorpath_p.h>
#include <private/qpaintengine_raster_p.h>
#include <qpen.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QCosmeticStroker;


typedef void (*StrokeLine)(QCosmeticStroker *stroker, qreal x1, qreal y1, qreal x2, qreal y2, int caps);

class QCosmeticStroker
{
public:
    struct Point {
        int x;
        int y;
    };
    struct PointF {
        qreal x;
        qreal y;
    };

    enum Caps {
        NoCaps = 0,
        CapBegin = 0x1,
        CapEnd = 0x2,
    };

    // used to avoid drop outs or duplicated points
    enum Direction {
        TopToBottom = 0x1,
        BottomToTop = 0x2,
        LeftToRight = 0x4,
        RightToLeft = 0x8,
        VerticalMask = 0x3,
        HorizontalMask = 0xc
    };

    QCosmeticStroker(QRasterPaintEngineState *s, const QRect &dr)
        : state(s),
          clip(dr),
          pattern(0),
          reversePattern(0),
          patternSize(0),
          patternLength(0),
          patternOffset(0),
          current_span(0),
          lastDir(LeftToRight),
          lastAxisAligned(false)
    { setup(); }
    ~QCosmeticStroker() { free(pattern); free(reversePattern); }
    void drawLine(const QPointF &p1, const QPointF &p2);
    void drawPath(const QVectorPath &path);
    void drawPoints(const QPoint *points, int num);
    void drawPoints(const QPointF *points, int num);


    QRasterPaintEngineState *state;
    QRect clip;
    // clip bounds in real
    qreal xmin, xmax;
    qreal ymin, ymax;

    StrokeLine stroke;
    bool drawCaps;

    int *pattern;
    int *reversePattern;
    int patternSize;
    int patternLength;
    int patternOffset;

    enum { NSPANS = 255 };
    QT_FT_Span spans[NSPANS];
    int current_span;
    ProcessSpans blend;

    int opacity;

    uint color;
    uint *pixels;
    int ppl;

    Direction lastDir;
    Point lastPixel;
    bool lastAxisAligned;

private:
    void setup();

    void renderCubic(const QPointF &p1, const QPointF &p2, const QPointF &p3, const QPointF &p4, int caps);
    void renderCubicSubdivision(PointF *points, int level, int caps);
    // used for closed subpaths
    void calculateLastPoint(qreal rx1, qreal ry1, qreal rx2, qreal ry2);

public:
    bool clipLine(qreal &x1, qreal &y1, qreal &x2, qreal &y2);
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QCOSMETICLINE_H
