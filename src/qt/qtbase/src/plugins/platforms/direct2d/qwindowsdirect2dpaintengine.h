/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINDOWSDIRECT2DPAINTENGINE_H
#define QWINDOWSDIRECT2DPAINTENGINE_H

#include <QtCore/QScopedPointer>
#include <QtGui/private/qpaintengineex_p.h>

#include <d2d1_1.h>
#include <dwrite_1.h>
#include <wrl.h>

QT_BEGIN_NAMESPACE

class QWindowsDirect2DPaintEnginePrivate;
class QWindowsDirect2DBitmap;

class QWindowsDirect2DPaintEngine : public QPaintEngineEx
{
    Q_DECLARE_PRIVATE(QWindowsDirect2DPaintEngine)
    friend class QWindowsDirect2DPaintEngineSuspenderImpl;
    friend class QWindowsDirect2DPaintEngineSuspenderPrivate;
public:
    enum Flag {
        NoFlag = 0,
        TranslucentTopLevelWindow = 1,
        EmulateComposition = 2,
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    QWindowsDirect2DPaintEngine(QWindowsDirect2DBitmap *bitmap, Flags flags);

    bool begin(QPaintDevice *pdev) Q_DECL_OVERRIDE;
    bool end() Q_DECL_OVERRIDE;

    Type type() const Q_DECL_OVERRIDE;

    void setState(QPainterState *s) Q_DECL_OVERRIDE;

    void draw(const QVectorPath &path) Q_DECL_OVERRIDE;

    void fill(const QVectorPath &path, const QBrush &brush) Q_DECL_OVERRIDE;
    void fill(ID2D1Geometry *geometry, const QBrush &brush);

    void stroke(const QVectorPath &path, const QPen &pen) Q_DECL_OVERRIDE;
    void stroke(ID2D1Geometry *geometry, const QPen &pen);

    void clip(const QVectorPath &path, Qt::ClipOperation op) Q_DECL_OVERRIDE;

    void clipEnabledChanged() Q_DECL_OVERRIDE;
    void penChanged() Q_DECL_OVERRIDE;
    void brushChanged() Q_DECL_OVERRIDE;
    void brushOriginChanged() Q_DECL_OVERRIDE;
    void opacityChanged() Q_DECL_OVERRIDE;
    void compositionModeChanged() Q_DECL_OVERRIDE;
    void renderHintsChanged() Q_DECL_OVERRIDE;
    void transformChanged() Q_DECL_OVERRIDE;

    void fillRect(const QRectF &rect, const QBrush &brush) Q_DECL_OVERRIDE;

    void drawRects(const QRect *rects, int rectCount) Q_DECL_OVERRIDE;
    void drawRects(const QRectF *rects, int rectCount) Q_DECL_OVERRIDE;

    void drawLines(const QLine *lines, int lineCount) Q_DECL_OVERRIDE;
    void drawLines(const QLineF *lines, int lineCount) Q_DECL_OVERRIDE;

    void drawEllipse(const QRectF &r) Q_DECL_OVERRIDE;
    void drawEllipse(const QRect &r) Q_DECL_OVERRIDE;

    void drawImage(const QRectF &rectangle, const QImage &image, const QRectF &sr, Qt::ImageConversionFlags flags = Qt::AutoColor) Q_DECL_OVERRIDE;
    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) Q_DECL_OVERRIDE;

    void drawStaticTextItem(QStaticTextItem *staticTextItem) Q_DECL_OVERRIDE;
    void drawTextItem(const QPointF &p, const QTextItem &textItem) Q_DECL_OVERRIDE;

private:
    void ensureBrush();
    void ensureBrush(const QBrush &brush);
    void ensurePen();
    void ensurePen(const QPen &pen);

    void rasterFill(const QVectorPath &path, const QBrush &brush);

    enum EmulationType { PenEmulation, BrushEmulation };
    bool emulationRequired(EmulationType type) const;

    bool antiAliasingEnabled() const;
    void adjustForAliasing(QRectF *rect);
    void adjustForAliasing(QPointF *point);

    void suspend();
    void resume();
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QWindowsDirect2DPaintEngine::Flags)

class QWindowsDirect2DPaintEngineSuspenderPrivate;
class QWindowsDirect2DPaintEngineSuspender
{
    Q_DISABLE_COPY(QWindowsDirect2DPaintEngineSuspender)
    Q_DECLARE_PRIVATE(QWindowsDirect2DPaintEngineSuspender)
    QScopedPointer<QWindowsDirect2DPaintEngineSuspenderPrivate> d_ptr;
public:
    QWindowsDirect2DPaintEngineSuspender(QWindowsDirect2DPaintEngine *engine);
    ~QWindowsDirect2DPaintEngineSuspender();
    void resume();
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECT2DPAINTENGINE_H
