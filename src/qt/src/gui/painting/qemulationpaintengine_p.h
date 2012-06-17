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

#ifndef QEMULATIONPAINTENGINE_P_H
#define QEMULATIONPAINTENGINE_P_H

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

#include <private/qpaintengineex_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QEmulationPaintEngine : public QPaintEngineEx
{
public:
    QEmulationPaintEngine(QPaintEngineEx *engine);
    
    virtual bool begin(QPaintDevice *pdev);
    virtual bool end();
    
    virtual Type type() const;
    virtual QPainterState *createState(QPainterState *orig) const;

    virtual void fill(const QVectorPath &path, const QBrush &brush);
    virtual void stroke(const QVectorPath &path, const QPen &pen);
    virtual void clip(const QVectorPath &path, Qt::ClipOperation op);

    virtual void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr);
    virtual void drawTextItem(const QPointF &p, const QTextItem &textItem);
    virtual void drawStaticTextItem(QStaticTextItem *item);
    virtual void drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &s);
    virtual void drawImage(const QRectF &r, const QImage &pm, const QRectF &sr, Qt::ImageConversionFlags flags);

    virtual void drawPixmapFragments(const QPainter::PixmapFragment *fragments, int fragmentCount, const QPixmap &pixmap,
                                     QPainter::PixmapFragmentHints hints);
    virtual void drawPixmapFragments(const QRectF *targetRects, const QRectF *sourceRects, int fragmentCount, const QPixmap &pixmap,
                                     QPainter::PixmapFragmentHints hints);
    
    virtual void clipEnabledChanged();
    virtual void penChanged();
    virtual void brushChanged();
    virtual void brushOriginChanged();
    virtual void opacityChanged();
    virtual void compositionModeChanged();
    virtual void renderHintsChanged();
    virtual void transformChanged();

    virtual void setState(QPainterState *s);

    virtual void beginNativePainting();
    virtual void endNativePainting();

    virtual uint flags() const {return QPaintEngineEx::IsEmulationEngine | QPaintEngineEx::DoNotEmulate;}

    inline QPainterState *state() { return (QPainterState *)QPaintEngine::state; }
    inline const QPainterState *state() const { return (const QPainterState *)QPaintEngine::state; }

    QPaintEngineEx *real_engine;
private:
    void fillBGRect(const QRectF &r);
};

QT_END_NAMESPACE

QT_END_HEADER

#endif
