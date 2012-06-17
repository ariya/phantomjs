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

#ifndef QPAINTENGINE_PIC_P_H
#define QPAINTENGINE_PIC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of QAbstractItemModel*.  This header file may change from version
// to version without notice, or even be removed.
//
// We mean it.
//
//

#include "QtGui/qpaintengine.h"

#ifndef QT_NO_PICTURE

QT_BEGIN_NAMESPACE

class QPicturePaintEnginePrivate;
class QBuffer;

class QPicturePaintEngine : public QPaintEngine
{
    Q_DECLARE_PRIVATE(QPicturePaintEngine)
public:
    QPicturePaintEngine();
    ~QPicturePaintEngine();

    bool begin(QPaintDevice *pdev);
    bool end();

    void updateState(const QPaintEngineState &state);

    void updatePen(const QPen &pen);
    void updateBrush(const QBrush &brush);
    void updateBrushOrigin(const QPointF &origin);
    void updateFont(const QFont &font);
    void updateBackground(Qt::BGMode bgmode, const QBrush &bgBrush);
    void updateMatrix(const QTransform &matrix);
    void updateClipRegion(const QRegion &region, Qt::ClipOperation op);
    void updateClipPath(const QPainterPath &path, Qt::ClipOperation op);
    void updateRenderHints(QPainter::RenderHints hints);
    void updateCompositionMode(QPainter::CompositionMode cmode);
    void updateClipEnabled(bool enabled);
    void updateOpacity(qreal opacity);

    void drawEllipse(const QRectF &rect);
    void drawPath(const QPainterPath &path);
    void drawPolygon(const QPointF *points, int numPoints, PolygonDrawMode mode);
#ifdef Q_NO_USING_KEYWORD
    inline void drawPolygon(const QPoint *points, int pointCount, PolygonDrawMode mode)
        { QPaintEngine::drawPolygon(points, pointCount, mode); }
#else
    using QPaintEngine::drawPolygon;
#endif

    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr);
    void drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &s);
    void drawImage(const QRectF &r, const QImage &image, const QRectF &sr,
                   Qt::ImageConversionFlags flags = Qt::AutoColor);
    void drawTextItem(const QPointF &p, const QTextItem &ti);

    Type type() const { return Picture; }

protected:
    QPicturePaintEngine(QPaintEnginePrivate &dptr);

private:
    Q_DISABLE_COPY(QPicturePaintEngine)

    void writeCmdLength(int pos, const QRectF &r, bool corr);
};

QT_END_NAMESPACE

#endif // QT_NO_PICTURE

#endif // QPAINTENGINE_PIC_P_H
