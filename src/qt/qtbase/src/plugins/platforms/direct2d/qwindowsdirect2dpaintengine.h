/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QWINDOWSDIRECT2DPAINTENGINE_H
#define QWINDOWSDIRECT2DPAINTENGINE_H

#include <QtCore/QScopedPointer>
#include <QtGui/private/qpaintengineex_p.h>

#include <d2d1_1.h>
#include <dwrite_1.h>

QT_BEGIN_NAMESPACE

class QWindowsDirect2DPaintEnginePrivate;
class QWindowsDirect2DBitmap;

class QWindowsDirect2DPaintEngine : public QPaintEngineEx
{
    Q_DECLARE_PRIVATE(QWindowsDirect2DPaintEngine)

public:
    QWindowsDirect2DPaintEngine(QWindowsDirect2DBitmap *bitmap);

    bool begin(QPaintDevice *pdev) Q_DECL_OVERRIDE;
    bool end() Q_DECL_OVERRIDE;

    Type type() const Q_DECL_OVERRIDE;

    void fill(const QVectorPath &path, const QBrush &brush) Q_DECL_OVERRIDE;

    void clip(const QVectorPath &path, Qt::ClipOperation op) Q_DECL_OVERRIDE;
    void clip(const QRect &rect, Qt::ClipOperation op) Q_DECL_OVERRIDE;
    void clip(const QRegion &region, Qt::ClipOperation op) Q_DECL_OVERRIDE;
    void clip(const QPainterPath &path, Qt::ClipOperation op) Q_DECL_OVERRIDE;

    void clipEnabledChanged() Q_DECL_OVERRIDE;
    void penChanged() Q_DECL_OVERRIDE;
    void brushChanged() Q_DECL_OVERRIDE;
    void brushOriginChanged() Q_DECL_OVERRIDE;
    void opacityChanged() Q_DECL_OVERRIDE;
    void compositionModeChanged() Q_DECL_OVERRIDE;
    void renderHintsChanged() Q_DECL_OVERRIDE;
    void transformChanged() Q_DECL_OVERRIDE;

    void drawImage(const QRectF &rectangle, const QImage &image, const QRectF &sr, Qt::ImageConversionFlags flags = Qt::AutoColor) Q_DECL_OVERRIDE;
    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) Q_DECL_OVERRIDE;

    void drawStaticTextItem(QStaticTextItem *staticTextItem) Q_DECL_OVERRIDE;
    void drawTextItem(const QPointF &p, const QTextItem &textItem) Q_DECL_OVERRIDE;

private:
    void drawGlyphRun(const D2D1_POINT_2F &pos, IDWriteFontFace *fontFace, const QFont &font,
                      int numGlyphs, const UINT16 *glyphIndices, const FLOAT *glyphAdvances,
                      const DWRITE_GLYPH_OFFSET *glyphOffsets, bool rtl);
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECT2DPAINTENGINE_H
