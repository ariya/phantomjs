/*
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/
 * Copyright (C) 2010 Sencha, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Image.h"

#include "AffineTransform.h"
#include "BitmapImage.h"
#include "ContextShadow.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "ImageObserver.h"
#include "PlatformString.h"
#include "StillImageQt.h"
#include "qwebsettings.h"

#include <QPixmap>
#include <QPainter>
#include <QImage>
#include <QImageReader>
#include <QTransform>

#include <QDebug>

#include <math.h>

// This function loads resources into WebKit
static QPixmap loadResourcePixmap(const char *name)
{
    QPixmap pixmap;
    if (qstrcmp(name, "missingImage") == 0)
        pixmap = QWebSettings::webGraphic(QWebSettings::MissingImageGraphic);
    else if (qstrcmp(name, "nullPlugin") == 0)
        pixmap = QWebSettings::webGraphic(QWebSettings::MissingPluginGraphic);
    else if (qstrcmp(name, "urlIcon") == 0)
        pixmap = QWebSettings::webGraphic(QWebSettings::DefaultFrameIconGraphic);
    else if (qstrcmp(name, "textAreaResizeCorner") == 0)
        pixmap = QWebSettings::webGraphic(QWebSettings::TextAreaSizeGripCornerGraphic);
    else if (qstrcmp(name, "deleteButton") == 0)
        pixmap = QWebSettings::webGraphic(QWebSettings::DeleteButtonGraphic);
    else if (!qstrcmp(name, "inputSpeech"))
        pixmap = QWebSettings::webGraphic(QWebSettings::InputSpeechButtonGraphic);
    else if (!qstrcmp(name, "searchCancelButton"))
        pixmap = QWebSettings::webGraphic(QWebSettings::SearchCancelButtonGraphic);
    else if (!qstrcmp(name, "searchCancelButtonPressed"))
        pixmap = QWebSettings::webGraphic(QWebSettings::SearchCancelButtonPressedGraphic);

    return pixmap;
}

namespace WebCore {

bool FrameData::clear(bool clearMetadata)
{
    if (clearMetadata)
        m_haveMetadata = false;

    if (m_frame) {
        delete m_frame;
        m_frame = 0;
        return true;
    }
    return false;
}


// ================================================
// Image Class
// ================================================

PassRefPtr<Image> Image::loadPlatformResource(const char* name)
{
    return StillImage::create(loadResourcePixmap(name));
}

void Image::drawPattern(GraphicsContext* ctxt, const FloatRect& tileRect, const AffineTransform& patternTransform,
                        const FloatPoint& phase, ColorSpace, CompositeOperator op, const FloatRect& destRect)
{
    QPixmap* framePixmap = nativeImageForCurrentFrame();
    if (!framePixmap) // If it's too early we won't have an image yet.
        return;

    // Qt interprets 0 width/height as full width/height so just short circuit.
    QRectF dr = QRectF(destRect).normalized();
    QRect tr = QRectF(tileRect).toRect().normalized();
    if (!dr.width() || !dr.height() || !tr.width() || !tr.height())
        return;

    QPixmap pixmap = *framePixmap;
    if (tr.x() || tr.y() || tr.width() != pixmap.width() || tr.height() != pixmap.height())
        pixmap = pixmap.copy(tr);

    CompositeOperator previousOperator = ctxt->compositeOperation();

    ctxt->setCompositeOperation(!pixmap.hasAlpha() && op == CompositeSourceOver ? CompositeCopy : op);

    QPainter* p = ctxt->platformContext();
    QTransform transform(patternTransform);

    // If this would draw more than one scaled tile, we scale the pixmap first and then use the result to draw.
    if (transform.type() == QTransform::TxScale) {
        QRectF tileRectInTargetCoords = (transform * QTransform().translate(phase.x(), phase.y())).mapRect(tr);

        bool tileWillBePaintedOnlyOnce = tileRectInTargetCoords.contains(dr);
        if (!tileWillBePaintedOnlyOnce) {
            QSizeF scaledSize(float(pixmap.width()) * transform.m11(), float(pixmap.height()) * transform.m22());
            QPixmap scaledPixmap(scaledSize.toSize());
            if (pixmap.hasAlpha())
                scaledPixmap.fill(Qt::transparent);
            {
                QPainter painter(&scaledPixmap);
                painter.setCompositionMode(QPainter::CompositionMode_Source);
                painter.setRenderHints(p->renderHints());
                painter.drawPixmap(QRect(0, 0, scaledPixmap.width(), scaledPixmap.height()), pixmap);
            }
            pixmap = scaledPixmap;
            transform = QTransform::fromTranslate(transform.dx(), transform.dy());
        }
    }

    /* Translate the coordinates as phase is not in world matrix coordinate space but the tile rect origin is. */
    transform *= QTransform().translate(phase.x(), phase.y());
    transform.translate(tr.x(), tr.y());

    QBrush b(pixmap);
    b.setTransform(transform);
    p->fillRect(dr, b);

    ctxt->setCompositeOperation(previousOperator);

    if (imageObserver())
        imageObserver()->didDraw(this);
}

BitmapImage::BitmapImage(QPixmap* pixmap, ImageObserver* observer)
    : Image(observer)
    , m_currentFrame(0)
    , m_frames(0)
    , m_frameTimer(0)
    , m_repetitionCount(cAnimationNone)
    , m_repetitionCountStatus(Unknown)
    , m_repetitionsComplete(0)
    , m_isSolidColor(false)
    , m_checkedForSolidColor(false)
    , m_animationFinished(true)
    , m_allDataReceived(true)
    , m_haveSize(true)
    , m_sizeAvailable(true)
    , m_decodedSize(0)
    , m_haveFrameCount(true)
    , m_frameCount(1)
{
    initPlatformData();

    int width = pixmap->width();
    int height = pixmap->height();
    m_decodedSize = width * height * 4;
    m_size = IntSize(width, height);

    m_frames.grow(1);
    m_frames[0].m_frame = pixmap;
    m_frames[0].m_hasAlpha = pixmap->hasAlpha();
    m_frames[0].m_haveMetadata = true;
    checkForSolidColor();
}

void BitmapImage::initPlatformData()
{
}

void BitmapImage::invalidatePlatformData()
{
}

// Drawing Routines
void BitmapImage::draw(GraphicsContext* ctxt, const FloatRect& dst,
                       const FloatRect& src, ColorSpace styleColorSpace, CompositeOperator op)
{
    QRectF normalizedDst = dst.normalized();
    QRectF normalizedSrc = src.normalized();

    startAnimation();

    if (normalizedSrc.isEmpty() || normalizedDst.isEmpty())
        return;

    QPixmap* image = nativeImageForCurrentFrame();
    if (!image)
        return;

    if (mayFillWithSolidColor()) {
        fillWithSolidColor(ctxt, normalizedDst, solidColor(), styleColorSpace, op);
        return;
    }

    CompositeOperator previousOperator = ctxt->compositeOperation();
    ctxt->setCompositeOperation(!image->hasAlpha() && op == CompositeSourceOver ? CompositeCopy : op);

    ContextShadow* shadow = ctxt->contextShadow();
    if (shadow->m_type != ContextShadow::NoShadow) {
        QPainter* shadowPainter = shadow->beginShadowLayer(ctxt, normalizedDst);
        if (shadowPainter) {
            shadowPainter->setOpacity(static_cast<qreal>(shadow->m_color.alpha()) / 255);
            shadowPainter->drawPixmap(normalizedDst, *image, normalizedSrc);
            shadow->endShadowLayer(ctxt);
        }
    }

    ctxt->platformContext()->drawPixmap(normalizedDst, *image, normalizedSrc);

    ctxt->setCompositeOperation(previousOperator);

    if (imageObserver())
        imageObserver()->didDraw(this);
}

void BitmapImage::checkForSolidColor()
{
    m_isSolidColor = false;
    m_checkedForSolidColor = true;

    if (frameCount() > 1)
        return;

    QPixmap* framePixmap = frameAtIndex(0);
    if (!framePixmap || framePixmap->width() != 1 || framePixmap->height() != 1)
        return;

    m_isSolidColor = true;
    m_solidColor = QColor::fromRgba(framePixmap->toImage().pixel(0, 0));
}

#if OS(WINDOWS)
PassRefPtr<BitmapImage> BitmapImage::create(HBITMAP hBitmap)
{
    return BitmapImage::create(new QPixmap(QPixmap::fromWinHBITMAP(hBitmap)));
}
#endif

}


// vim: ts=4 sw=4 et
