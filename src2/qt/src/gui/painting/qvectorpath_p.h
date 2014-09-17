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

#ifndef QVECTORPATH_P_H
#define QVECTORPATH_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/qpaintengine.h>

#include <private/qpaintengine_p.h>
#include <private/qstroker_p.h>
#include <private/qpainter_p.h>


QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QPaintEngineEx;

typedef void (*qvectorpath_cache_cleanup)(QPaintEngineEx *engine, void *data);

struct QRealRect {
    qreal x1, y1, x2, y2;
};

class Q_GUI_EXPORT QVectorPath
{
public:
    enum Hint {
        // Shape hints, in 0x000000ff, access using shape()
        AreaShapeMask           = 0x0001,       // shape covers an area
        NonConvexShapeMask      = 0x0002,       // shape is not convex
        CurvedShapeMask         = 0x0004,       // shape contains curves...
        LinesShapeMask          = 0x0008,
        RectangleShapeMask      = 0x0010,
        ShapeMask               = 0x001f,

        // Shape hints merged into basic shapes..
        LinesHint               = LinesShapeMask,
        RectangleHint           = AreaShapeMask | RectangleShapeMask,
        EllipseHint             = AreaShapeMask | CurvedShapeMask,
        ConvexPolygonHint       = AreaShapeMask,
        PolygonHint             = AreaShapeMask | NonConvexShapeMask,
        RoundedRectHint         = AreaShapeMask | CurvedShapeMask,
        ArbitraryShapeHint      = AreaShapeMask | NonConvexShapeMask | CurvedShapeMask,

        // Other hints
        IsCachedHint            = 0x0100, // Set if the cache hint is set
        ShouldUseCacheHint      = 0x0200, // Set if the path should be cached when possible..
        ControlPointRect        = 0x0400, // Set if the control point rect has been calculated...

        // Shape rendering specifiers...
        OddEvenFill             = 0x1000,
        WindingFill             = 0x2000,
        ImplicitClose           = 0x4000
    };

    // ### Falcon: introduca a struct XY for points so lars is not so confused...
    QVectorPath(const qreal *points,
                int count,
                const QPainterPath::ElementType *elements = 0,
                uint hints = ArbitraryShapeHint)
        : m_elements(elements),
          m_points(points),
          m_count(count),
          m_hints(hints)
    {
    }

    ~QVectorPath();

    QRectF controlPointRect() const;

    inline Hint shape() const { return (Hint) (m_hints & ShapeMask); }
    inline bool isConvex() const { return (m_hints & NonConvexShapeMask) == 0; }
    inline bool isCurved() const { return m_hints & CurvedShapeMask; }

    inline bool isCacheable() const { return m_hints & ShouldUseCacheHint; }
    inline bool hasImplicitClose() const { return m_hints & ImplicitClose; }
    inline bool hasWindingFill() const { return m_hints & WindingFill; }

    inline void makeCacheable() const { m_hints |= ShouldUseCacheHint; m_cache = 0; }
    inline uint hints() const { return m_hints; }

    inline const QPainterPath::ElementType *elements() const { return m_elements; }
    inline const qreal *points() const { return m_points; }
    inline bool isEmpty() const { return m_points == 0; }

    inline int elementCount() const { return m_count; }
    inline const QPainterPath convertToPainterPath() const;

    static inline uint polygonFlags(QPaintEngine::PolygonDrawMode mode);

    struct CacheEntry {
        QPaintEngineEx *engine;
        void *data;
        qvectorpath_cache_cleanup cleanup;
        CacheEntry *next;
    };

    CacheEntry *addCacheData(QPaintEngineEx *engine, void *data, qvectorpath_cache_cleanup cleanup) const;
    inline CacheEntry *lookupCacheData(QPaintEngineEx *engine) const {
        Q_ASSERT(m_hints & ShouldUseCacheHint);
        CacheEntry *e = m_cache;
        while (e) {
            if (e->engine == engine)
                return e;
            e = e->next;
        }
        return 0;
    }


private:
    Q_DISABLE_COPY(QVectorPath)

    const QPainterPath::ElementType *m_elements;
    const qreal *m_points;
    const int m_count;

    mutable uint m_hints;
    mutable QRealRect m_cp_rect;

    mutable CacheEntry *m_cache;
};

Q_GUI_EXPORT const QVectorPath &qtVectorPathForPath(const QPainterPath &path);

QT_END_NAMESPACE

QT_END_HEADER

#endif
