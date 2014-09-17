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

#ifndef QPAINTER_P_H
#define QPAINTER_P_H

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

#include "QtGui/qbrush.h"
#include "QtGui/qfont.h"
#include "QtGui/qpen.h"
#include "QtGui/qregion.h"
#include "QtGui/qmatrix.h"
#include "QtGui/qpainter.h"
#include "QtGui/qpainterpath.h"
#include "QtGui/qpaintengine.h"
#include <QtCore/qhash.h>

#include <private/qpen_p.h>

QT_BEGIN_NAMESPACE

class QPaintEngine;
class QEmulationPaintEngine;
class QPaintEngineEx;
struct QFixedPoint;

struct QTLWExtra;

struct DataPtrContainer {
    void *ptr;
};

inline void *data_ptr(const QTransform &t) { return (DataPtrContainer *) &t; }
inline bool qtransform_fast_equals(const QTransform &a, const QTransform &b) { return data_ptr(a) == data_ptr(b); }

// QPen inline functions...
inline QPen::DataPtr &data_ptr(const QPen &p) { return const_cast<QPen &>(p).data_ptr(); }
inline bool qpen_fast_equals(const QPen &a, const QPen &b) { return data_ptr(a) == data_ptr(b); }
inline QBrush qpen_brush(const QPen &p) { return data_ptr(p)->brush; }
inline qreal qpen_widthf(const QPen &p) { return data_ptr(p)->width; }
inline Qt::PenStyle qpen_style(const QPen &p) { return data_ptr(p)->style; }
inline Qt::PenCapStyle qpen_capStyle(const QPen &p) { return data_ptr(p)->capStyle; }
inline Qt::PenJoinStyle qpen_joinStyle(const QPen &p) { return data_ptr(p)->joinStyle; }

// QBrush inline functions...
inline QBrush::DataPtr &data_ptr(const QBrush &p) { return const_cast<QBrush &>(p).data_ptr(); }
inline bool qbrush_fast_equals(const QBrush &a, const QBrush &b) { return data_ptr(a) == data_ptr(b); }
inline Qt::BrushStyle qbrush_style(const QBrush &b) { return data_ptr(b)->style; }
inline const QColor &qbrush_color(const QBrush &b) { return data_ptr(b)->color; }
inline bool qbrush_has_transform(const QBrush &b) { return data_ptr(b)->transform.type() > QTransform::TxNone; }

class QPainterClipInfo
{
public:
    enum ClipType { RegionClip, PathClip, RectClip, RectFClip };

    QPainterClipInfo(const QPainterPath &p, Qt::ClipOperation op, const QTransform &m) :
        clipType(PathClip), matrix(m), operation(op), path(p) { }

    QPainterClipInfo(const QRegion &r, Qt::ClipOperation op, const QTransform &m) :
        clipType(RegionClip), matrix(m), operation(op), region(r) { }

    QPainterClipInfo(const QRect &r, Qt::ClipOperation op, const QTransform &m) :
        clipType(RectClip), matrix(m), operation(op), rect(r) { }

    QPainterClipInfo(const QRectF &r, Qt::ClipOperation op, const QTransform &m) :
        clipType(RectFClip), matrix(m), operation(op), rectf(r) { }

    ClipType clipType;
    QTransform matrix;
    Qt::ClipOperation operation;
    QPainterPath path;
    QRegion region;
    QRect rect;
    QRectF rectf;

    // ###
//     union {
//         QRegionData *d;
//         QPainterPathPrivate *pathData;

//         struct {
//             int x, y, w, h;
//         } rectData;
//         struct {
//             qreal x, y, w, h;
//         } rectFData;
//     };

};


class Q_GUI_EXPORT QPainterState : public QPaintEngineState
{
public:
    QPainterState();
    QPainterState(const QPainterState *s);
    virtual ~QPainterState();
    void init(QPainter *p);

    QPointF brushOrigin;
    QFont font;
    QFont deviceFont;
    QPen pen;
    QBrush brush;
    QBrush bgBrush;             // background brush
    QRegion clipRegion;
    QPainterPath clipPath;
    Qt::ClipOperation clipOperation;
    QPainter::RenderHints renderHints;
    QList<QPainterClipInfo> clipInfo; // ### Make me smaller and faster to copy around...
    QTransform worldMatrix;       // World transformation matrix, not window and viewport
    QTransform matrix;            // Complete transformation matrix,
    QTransform redirectionMatrix;
    int wx, wy, ww, wh;         // window rectangle
    int vx, vy, vw, vh;         // viewport rectangle
    qreal opacity;

    uint WxF:1;                 // World transformation
    uint VxF:1;                 // View transformation
    uint clipEnabled:1;

    Qt::BGMode bgMode;
    QPainter *painter;
    Qt::LayoutDirection layoutDirection;
    QPainter::CompositionMode composition_mode;
    uint emulationSpecifier;
    uint changeFlags;
};

struct QPainterDummyState
{
    QFont font;
    QPen pen;
    QBrush brush;
    QTransform transform;
};

class QRawFont;
class QPainterPrivate
{
    Q_DECLARE_PUBLIC(QPainter)
public:
    QPainterPrivate(QPainter *painter)
    : q_ptr(painter), d_ptrs(0), state(0), dummyState(0), txinv(0), inDestructor(false), d_ptrs_size(0),
        refcount(1), device(0), original_device(0), helper_device(0), engine(0), emulationEngine(0),
        extended(0)
    {
    }

    ~QPainterPrivate();

    QPainter *q_ptr;
    QPainterPrivate **d_ptrs;

    QPainterState *state;
    QVector<QPainterState*> states;

    mutable QPainterDummyState *dummyState;

    QTransform invMatrix;
    uint txinv:1;
    uint inDestructor : 1;
    uint d_ptrs_size;
    uint refcount;

    enum DrawOperation { StrokeDraw        = 0x1,
                         FillDraw          = 0x2,
                         StrokeAndFillDraw = 0x3
    };

    QPainterDummyState *fakeState() const {
        if (!dummyState)
            dummyState = new QPainterDummyState();
        return dummyState;
    }

    void updateEmulationSpecifier(QPainterState *s);
    void updateStateImpl(QPainterState *state);
    void updateState(QPainterState *state);

    void draw_helper(const QPainterPath &path, DrawOperation operation = StrokeAndFillDraw);
    void drawStretchedGradient(const QPainterPath &path, DrawOperation operation);
    void drawOpaqueBackground(const QPainterPath &path, DrawOperation operation);

#if !defined(QT_NO_RAWFONT)
    void drawGlyphs(const quint32 *glyphArray, QFixedPoint *positionArray, int glyphCount,
                    const QRawFont &font, bool overline = false, bool underline = false,
                    bool strikeOut = false);
#endif

    void updateMatrix();
    void updateInvMatrix();

    int rectSubtraction() const {
        return state->pen.style() != Qt::NoPen && state->pen.width() == 0 ? 1 : 0;
    }

    void checkEmulation();

    static QPainterPrivate *get(QPainter *painter)
    {
        return painter->d_ptr.data();
    }

    QTransform viewTransform() const;
    static bool attachPainterPrivate(QPainter *q, QPaintDevice *pdev);
    void detachPainterPrivate(QPainter *q);

    QPaintDevice *device;
    QPaintDevice *original_device;
    QPaintDevice *helper_device;
    QPaintEngine *engine;
    QEmulationPaintEngine *emulationEngine;
    QPaintEngineEx *extended;
    QBrush colorBrush;          // for fill with solid color
};

Q_GUI_EXPORT void qt_draw_helper(QPainterPrivate *p, const QPainterPath &path, QPainterPrivate::DrawOperation operation);

QString qt_generate_brush_key(const QBrush &brush);

QT_END_NAMESPACE

#endif // QPAINTER_P_H
