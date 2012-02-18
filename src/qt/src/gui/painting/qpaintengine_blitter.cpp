/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "private/qpaintengine_blitter_p.h"

#include "private/qblittable_p.h"
#include "private/qpaintengine_raster_p.h"
#include "private/qpainter_p.h"
#include "private/qapplication_p.h"
#include "private/qpixmap_blitter_p.h"

#ifndef QT_NO_BLITTABLE
QT_BEGIN_NAMESPACE

#define STATE_XFORM_SCALE       0x00000001
#define STATE_XFORM_COMPLEX     0x00000002

#define STATE_BRUSH_PATTERN     0x00000010
#define STATE_BRUSH_ALPHA       0x00000020

#define STATE_PEN_ENABLED       0x00000100

#define STATE_ANTIALIASING      0x00001000
#define STATE_ALPHA             0x00002000
#define STATE_BLENDING_COMPLEX  0x00004000

#define STATE_CLIPSYS_COMPLEX   0x00010000
#define STATE_CLIP_COMPLEX      0x00020000


static inline void updateStateBits(uint *state, uint mask, bool on)
{
    *state = on ? (*state | mask) : (*state & ~mask);
}

static inline bool checkStateAgainstMask(uint state, uint mask)
{
    return !state || (state & mask && !(state & ~mask));
}

class CapabilitiesToStateMask
{
public:
    CapabilitiesToStateMask(QBlittable::Capabilities capabilities)
            : m_capabilities(capabilities),
              fillRectMask(0),
              drawRectMask(0),
              drawPixmapMask(0),
              capabillitiesState(0)
    {
        if (capabilities & QBlittable::SolidRectCapability) {
            setFillRectMask();
        }
        if (capabilities & QBlittable::SourcePixmapCapability) {
           setSourcePixmapMask();
        }
        if (capabilities & QBlittable::SourceOverPixmapCapability) {
           setSourceOverPixmapMask();
        }
        if (capabilities & QBlittable::SourceOverScaledPixmapCapability) {
            setSourceOverScaledPixmapMask();
        }
    }

    inline bool canBlitterFillRect() const
    {
        return checkStateAgainstMask(capabillitiesState,fillRectMask);
    }

    inline bool canBlitterDrawRectMask() const
    {
        return checkStateAgainstMask(capabillitiesState,drawRectMask);
    }

    bool canBlitterDrawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) const
    {
        if (pm.pixmapData()->classId() != QPixmapData::BlitterClass)
            return false;
        if (checkStateAgainstMask(capabillitiesState,drawPixmapMask)) {
            if (m_capabilities & (QBlittable::SourceOverPixmapCapability
                                  | QBlittable::SourceOverScaledPixmapCapability)) {
                if (r.size() != sr.size()) {
                    return m_capabilities & QBlittable::SourceOverScaledPixmapCapability;
                } else {
                    return m_capabilities & QBlittable::SourceOverPixmapCapability;
                }
            }
            if ((m_capabilities & QBlittable::SourcePixmapCapability) && r.size() == sr.size() && !pm.hasAlphaChannel()) {
                return m_capabilities & QBlittable::SourcePixmapCapability;
            }
        }
        return false;
    }

    inline void updateState(uint mask, bool on) {
        updateStateBits(&capabillitiesState,mask,on);
    }

public:

    void setFillRectMask() {
        updateStateBits(&fillRectMask, STATE_XFORM_SCALE, false);
        updateStateBits(&fillRectMask, STATE_XFORM_COMPLEX, false);

        updateStateBits(&fillRectMask, STATE_BRUSH_PATTERN, false);
        updateStateBits(&fillRectMask, STATE_BRUSH_ALPHA, false);

        updateStateBits(&fillRectMask, STATE_PEN_ENABLED, true);

        //Sub-pixel aliasing should not be sent to the blitter
        updateStateBits(&fillRectMask, STATE_ANTIALIASING, true);
        updateStateBits(&fillRectMask, STATE_ALPHA, false);
        updateStateBits(&fillRectMask, STATE_BLENDING_COMPLEX, false);

        updateStateBits(&fillRectMask, STATE_CLIPSYS_COMPLEX, false);
        updateStateBits(&fillRectMask, STATE_CLIP_COMPLEX, false);
    }

    void setSourcePixmapMask() {
        updateStateBits(&drawPixmapMask, STATE_XFORM_SCALE, true);
        updateStateBits(&drawPixmapMask, STATE_XFORM_COMPLEX, false);

        updateStateBits(&drawPixmapMask, STATE_BRUSH_PATTERN, true);
        updateStateBits(&drawPixmapMask, STATE_BRUSH_ALPHA, false);

        updateStateBits(&drawPixmapMask, STATE_PEN_ENABLED, true);

        updateStateBits(&drawPixmapMask, STATE_ANTIALIASING, true);
        updateStateBits(&drawPixmapMask, STATE_ALPHA, false);
        updateStateBits(&drawPixmapMask, STATE_BLENDING_COMPLEX, false);

        updateStateBits(&drawPixmapMask, STATE_CLIPSYS_COMPLEX, false);
        updateStateBits(&drawPixmapMask, STATE_CLIP_COMPLEX, false);
    }

    void setSourceOverPixmapMask() {
        setSourcePixmapMask();
    }

    void setSourceOverScaledPixmapMask() {
        setSourceOverPixmapMask();
        updateStateBits(&drawRectMask, STATE_XFORM_SCALE, true);
    }

    QBlittable::Capabilities m_capabilities;
    uint fillRectMask;
    uint drawRectMask;
    uint drawPixmapMask;
    uint capabillitiesState;
};

class QBlitterPaintEnginePrivate : public QPaintEngineExPrivate
{
    Q_DECLARE_PUBLIC(QBlitterPaintEngine);
public:
    QBlitterPaintEnginePrivate(QBlittablePixmapData *p)
            : QPaintEngineExPrivate(),
              pmData(p),
              isBlitterLocked(false),
              hasXForm(false)

    {
        raster = new QRasterPaintEngine(p->buffer());
        capabillities = new CapabilitiesToStateMask(pmData->blittable()->capabilities());
    }

    inline void lock() {
        if (!isBlitterLocked) {
            raster->d_func()->rasterBuffer->prepare(pmData->blittable()->lock());
            isBlitterLocked = true;
        }
    }

    inline void unlock() {
        if (isBlitterLocked) {
            pmData->blittable()->unlock();
            isBlitterLocked = false;
        }
    }

    void fillRect(const QRectF &rect, const QColor &color) {
        Q_Q(QBlitterPaintEngine);
        pmData->unmarkRasterOverlay(rect);
        QRectF targetRect = rect;
        if (hasXForm) {
            targetRect = q->state()->matrix.mapRect(rect);
        }
        const QClipData *clipData = q->clip();
        if (clipData) {
            if (clipData->hasRectClip) {
                unlock();
                pmData->blittable()->fillRect(targetRect & clipData->clipRect, color);
            } else if (clipData->hasRegionClip) {
                QVector<QRect> rects = clipData->clipRegion.rects();
                for ( int i = 0; i < rects.size(); i++ ) {
                    QRect intersectRect = rects.at(i).intersected(targetRect.toRect());
                    if (!intersectRect.isEmpty()) {
                        unlock();
                        pmData->blittable()->fillRect(intersectRect,color);
                    }
                }
            }
        } else {
            if (targetRect.x() >= 0 && targetRect.y() >= 0
                && targetRect.width() <= raster->paintDevice()->width()
                && targetRect.height() <= raster->paintDevice()->height()) {
                unlock();
                pmData->blittable()->fillRect(targetRect,color);
            } else {
                QRectF deviceRect(0,0,raster->paintDevice()->width(), raster->paintDevice()->height());
                unlock();
                pmData->blittable()->fillRect(deviceRect&targetRect,color);
            }
        }
    }

    void clipAndDrawPixmap(const QRectF &clip, const QRectF &target, const QPixmap &pm, const QRectF &sr) {
        QRectF intersectedRect = clip.intersected(target);
        if (intersectedRect.isEmpty())
            return;
        QRectF source = sr;
        if(intersectedRect.size() != target.size()) {
            qreal deltaTop = target.top() - intersectedRect.top();
            qreal deltaLeft = target.left() - intersectedRect.left();
            qreal deltaBottom = target.bottom() - intersectedRect.bottom();
            qreal deltaRight = target.right() - intersectedRect.right();
            source.adjust(-deltaLeft,-deltaTop,-deltaRight,-deltaBottom);
        }
        pmData->unmarkRasterOverlay(intersectedRect);
        pmData->blittable()->drawPixmap(intersectedRect, pm, source);
    }

    void updateClip() {
        Q_Q(QBlitterPaintEngine);
        const QClipData *clip = q->clip();
        bool complex = clip && !(clip->hasRectClip || clip->hasRegionClip);
        capabillities->updateState(STATE_CLIP_COMPLEX, complex);
    }

    void systemStateChanged() {
        raster->d_func()->systemStateChanged();
    }

    QRasterPaintEngine *raster;

    QBlittablePixmapData *pmData;
    bool isBlitterLocked;

    CapabilitiesToStateMask *capabillities;

    uint hasXForm;
};

QBlitterPaintEngine::QBlitterPaintEngine(QBlittablePixmapData *p)
    : QPaintEngineEx(*(new QBlitterPaintEnginePrivate(p)))
{
}

QBlitterPaintEngine::~QBlitterPaintEngine()
{
}

QPainterState *QBlitterPaintEngine::createState(QPainterState *orig) const
{
    Q_D(const QBlitterPaintEngine);
    return d->raster->createState(orig);
}

bool QBlitterPaintEngine::begin(QPaintDevice *pdev)
{
    Q_D(QBlitterPaintEngine);

    setActive(true);
    bool ok = d->raster->begin(pdev);
#ifdef QT_BLITTER_RASTEROVERLAY
    d->pmData->unmergeOverlay();
#endif
    return ok;
}


bool QBlitterPaintEngine::end()
{
    Q_D(QBlitterPaintEngine);

    setActive(false);
#ifdef QT_BLITTER_RASTEROVERLAY
    d->pmData->mergeOverlay();
#endif
    return d->raster->end();
}


void QBlitterPaintEngine::fill(const QVectorPath &path, const QBrush &brush)
{
    Q_D(QBlitterPaintEngine);
    if (path.shape() == QVectorPath::RectangleHint) {
        QRectF rect(((QPointF *) path.points())[0], ((QPointF *) path.points())[2]);
        fillRect(rect, brush);
    } else {
        d->lock();
        d->pmData->markRasterOverlay(path);
        d->raster->fill(path, brush);
    }
}

void QBlitterPaintEngine::fillRect(const QRectF &rect, const QColor &color)
{
    Q_D(QBlitterPaintEngine);
    if (d->capabillities->canBlitterFillRect() && color.alpha() == 0xff) {
        d->fillRect(rect, color);
    } else {
        d->lock();
        d->pmData->markRasterOverlay(rect);
        d->raster->fillRect(rect, color);
    }
}

void QBlitterPaintEngine::fillRect(const QRectF &rect, const QBrush &brush)
{
    if(rect.size().isEmpty())
        return;

    Q_D(QBlitterPaintEngine);

    if (qbrush_style(brush) == Qt::SolidPattern
        && qbrush_color(brush).alpha() == 0xff
        && d->capabillities->canBlitterFillRect())
    {
        d->fillRect(rect, qbrush_color(brush));
    }else if (brush.style() == Qt::TexturePattern
              && d->capabillities->canBlitterDrawPixmap(rect,brush.texture(),rect))
    {
        bool rectIsFilled = false;
        QRectF transformedRect = state()->matrix.mapRect(rect);
        qreal x = transformedRect.x();
        qreal y = transformedRect.y();
        QPixmap pm = brush.texture();
        d->unlock();
        int srcX = int(rect.x() - state()->brushOrigin.x()) % pm.width();
        if (srcX < 0)
            srcX = pm.width() + srcX;
        const int startX = srcX;
        int srcY = int(rect.y() - state()->brushOrigin.y()) % pm.height();
        if (srcY < 0)
            srcY = pm.height() + srcY;
        while (!rectIsFilled) {
            qreal blitWidth = (pm.width() ) - srcX;
            qreal blitHeight = (pm.height() ) - srcY;
            if (x + blitWidth > transformedRect.right())
                blitWidth = transformedRect.right() -x;
            if (y + blitHeight > transformedRect.bottom())
                blitHeight = transformedRect.bottom() - y;
            const QClipData *clipData = clip();
            if (clipData->hasRectClip) {
                QRect targetRect = QRect(x,y,blitWidth,blitHeight).intersected(clipData->clipRect);
                if (targetRect.isValid()) {
                    int tmpSrcX  = srcX + (targetRect.x() - x);
                    int tmpSrcY = srcY + (targetRect.y() - y);
                    QRect srcRect(tmpSrcX,tmpSrcY,targetRect.width(),targetRect.height());
                    d->pmData->blittable()->drawPixmap(targetRect,pm,srcRect);
                }
            } else if (clipData->hasRegionClip) {
                QVector<QRect> clipRects = clipData->clipRegion.rects();
                QRect unclippedTargetRect(x,y,blitWidth,blitHeight);
                QRegion intersectedRects = clipData->clipRegion.intersected(unclippedTargetRect);

                for ( int i = 0; i < intersectedRects.rects().size(); i++ ) {
                    QRect targetRect = intersectedRects.rects().at(i);
                    if (!targetRect.isValid() || targetRect.isEmpty())
                        continue;
                    int tmpSrcX = srcX + (targetRect.x() - x);
                    int tmpSrcY = srcY + (targetRect.y() - y);
                    QRect srcRect(tmpSrcX,tmpSrcY,targetRect.width(),targetRect.height());
                    d->pmData->blittable()->drawPixmap(targetRect,pm,srcRect);
                }
            }
            x+=blitWidth;
            if (x>=transformedRect.right()) {
                x = transformedRect.x();
                srcX = startX;
                srcY = 0;
                y+=blitHeight;
                if (y>=transformedRect.bottom())
                    rectIsFilled = true;
            } else
                srcX = 0;
        }
    } else {
        d->lock();
        d->pmData->markRasterOverlay(rect);
        d->raster->fillRect(rect, brush);
    }

}

void QBlitterPaintEngine::stroke(const QVectorPath &path, const QPen &pen)
{
    Q_D(QBlitterPaintEngine);
    d->lock();
    d->pmData->markRasterOverlay(path);
    d->raster->stroke(path, pen);
}

void QBlitterPaintEngine::clip(const QVectorPath &path, Qt::ClipOperation op)
{
    Q_D(QBlitterPaintEngine);
    d->lock();
    d->raster->clip(path, op);
    d->updateClip();
}
void QBlitterPaintEngine::clip(const QRect &rect, Qt::ClipOperation op){
    Q_D(QBlitterPaintEngine);
    d->lock();
    d->raster->clip(rect,op);
    d->updateClip();
}
void QBlitterPaintEngine::clip(const QRegion &region, Qt::ClipOperation op)
{
    Q_D(QBlitterPaintEngine);
    d->lock();
    d->raster->clip(region,op);
    d->updateClip();
}

void QBlitterPaintEngine::clipEnabledChanged()
{
    Q_D(QBlitterPaintEngine);
    d->lock();
    d->raster->clipEnabledChanged();
}

void QBlitterPaintEngine::penChanged()
{
    Q_D(QBlitterPaintEngine);
    d->lock();
    d->raster->penChanged();
    d->capabillities->updateState(STATE_PEN_ENABLED,qpen_style(state()->pen) != Qt::NoPen);
}

void QBlitterPaintEngine::brushChanged()
{
    Q_D(QBlitterPaintEngine);
    d->raster->brushChanged();

    bool solid = qbrush_style(state()->brush) == Qt::SolidPattern;

    d->capabillities->updateState(STATE_BRUSH_PATTERN, !solid);
    d->capabillities->updateState(STATE_BRUSH_ALPHA,
                                        qbrush_color(state()->brush).alpha() < 255);
}

void QBlitterPaintEngine::brushOriginChanged()
{
    Q_D(QBlitterPaintEngine);
    d->raster->brushOriginChanged();
}

void QBlitterPaintEngine::opacityChanged()
{
    Q_D(QBlitterPaintEngine);
    d->raster->opacityChanged();

    bool translucent = state()->opacity < 1;
    d->capabillities->updateState(STATE_ALPHA,translucent);
}

void QBlitterPaintEngine::compositionModeChanged()
{
    Q_D(QBlitterPaintEngine);
    d->raster->compositionModeChanged();

    bool nonTrivial = state()->composition_mode != QPainter::CompositionMode_SourceOver
                      && state()->composition_mode != QPainter::CompositionMode_Source;

    d->capabillities->updateState(STATE_BLENDING_COMPLEX,nonTrivial);
}

void QBlitterPaintEngine::renderHintsChanged()
{
    Q_D(QBlitterPaintEngine);
    d->raster->renderHintsChanged();

    bool aa = state()->renderHints & QPainter::Antialiasing;
    d->capabillities->updateState(STATE_ANTIALIASING, aa);

}

void QBlitterPaintEngine::transformChanged()
{
    Q_D(QBlitterPaintEngine);
    d->raster->transformChanged();

    QTransform::TransformationType type = state()->matrix.type();

    d->capabillities->updateState(STATE_XFORM_COMPLEX, type > QTransform::TxScale);
    d->capabillities->updateState(STATE_XFORM_SCALE, type > QTransform::TxTranslate);

    d->hasXForm = type >= QTransform::TxTranslate;

}

void QBlitterPaintEngine::drawRects(const QRect *rects, int rectCount)
{
    Q_D(QBlitterPaintEngine);
    if (d->capabillities->canBlitterDrawRectMask()) {
        for (int i=0; i<rectCount; ++i) {
            d->fillRect(rects[i], qbrush_color(state()->brush));
        }
    } else {
        d->pmData->markRasterOverlay(rects,rectCount);
        QPaintEngineEx::drawRects(rects, rectCount);
    }
}

void QBlitterPaintEngine::drawRects(const QRectF *rects, int rectCount)
{
    Q_D(QBlitterPaintEngine);
    if (d->capabillities->canBlitterDrawRectMask()) {
        for (int i=0; i<rectCount; ++i) {
            d->fillRect(rects[i], qbrush_color(state()->brush));
        }
    } else {
        d->pmData->markRasterOverlay(rects,rectCount);
        QPaintEngineEx::drawRects(rects, rectCount);
    }
}

void QBlitterPaintEngine::drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr)
{
    Q_D(QBlitterPaintEngine);
    if (d->capabillities->canBlitterDrawPixmap(r,pm,sr)) {

        d->unlock();
        QRectF targetRect = r;
        if (d->hasXForm) {
            targetRect = state()->matrix.mapRect(r);
        }
        const QClipData *clipData = clip();
        if (clipData) {
            if (clipData->hasRectClip) {
                d->clipAndDrawPixmap(clipData->clipRect,targetRect,pm,sr);
            }else if (clipData->hasRegionClip) {
                QVector<QRect>rects = clipData->clipRegion.rects();
                for (int i = 0; i<rects.size(); i++) {
                    d->clipAndDrawPixmap(rects.at(i),targetRect,pm,sr);
                }
            }
        } else {
            QRectF deviceRect(0,0,d->raster->paintDevice()->width(), d->raster->paintDevice()->height());
            d->clipAndDrawPixmap(deviceRect,targetRect,pm,sr);
        }
    }else {
        d->lock();
        d->pmData->markRasterOverlay(r);
        d->raster->drawPixmap(r, pm, sr);
    }
}

void QBlitterPaintEngine::drawImage(const QRectF &r, const QImage &pm, const QRectF &sr,
                                    Qt::ImageConversionFlags flags)
{
    Q_D(QBlitterPaintEngine);
    d->lock();
    d->pmData->markRasterOverlay(r);
    d->raster->drawImage(r, pm, sr, flags);
}


void QBlitterPaintEngine::drawTextItem(const QPointF &pos, const QTextItem &ti)
{
    Q_D(QBlitterPaintEngine);
    d->lock();
    d->raster->drawTextItem(pos, ti);
    d->pmData->markRasterOverlay(pos,ti);
}

void QBlitterPaintEngine::drawStaticTextItem(QStaticTextItem *sti)
{
    Q_D(QBlitterPaintEngine);
    d->lock();
    d->raster->drawStaticTextItem(sti);

//#### d->pmData->markRasterOverlay(sti);
    qWarning("not implemented: markRasterOverlay for QStaticTextItem");

}


void QBlitterPaintEngine::drawEllipse(const QRectF &r)
{
    Q_D(QBlitterPaintEngine);
    d->lock();
    d->pmData->markRasterOverlay(r);
    d->raster->drawEllipse(r);
}

void QBlitterPaintEngine::setState(QPainterState *s)
{
    Q_D(QBlitterPaintEngine);
    d->lock();
    QPaintEngineEx::setState(s);
    d->raster->setState(s);

    clipEnabledChanged();
    penChanged();
    brushChanged();
    brushOriginChanged();
    opacityChanged();
    compositionModeChanged();
    renderHintsChanged();
    transformChanged();

    d->updateClip();
}

inline QRasterPaintEngine *QBlitterPaintEngine::raster() const
{
    Q_D(const QBlitterPaintEngine);
    return d->raster;
}

QT_END_NAMESPACE
#endif //QT_NO_BLITTABLE

