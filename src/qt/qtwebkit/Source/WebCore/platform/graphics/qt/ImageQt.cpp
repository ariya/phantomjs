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
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "ImageObserver.h"
#include "ShadowBlur.h"
#include "StillImageQt.h"
#include <wtf/text/WTFString.h>

#include <QCoreApplication>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QPaintEngine>
#include <QPixmap>
#include <QPixmapCache>
#include <QTransform>
#include <private/qhexstring_p.h>

#include <math.h>

#if OS(WINDOWS)
QT_BEGIN_NAMESPACE
Q_GUI_EXPORT QPixmap qt_pixmapFromWinHBITMAP(HBITMAP, int hbitmapFormat = 0);
QT_END_NAMESPACE
#endif

typedef QHash<QByteArray, QPixmap> WebGraphicHash;
Q_GLOBAL_STATIC(WebGraphicHash, _graphics)

static void earlyClearGraphics()
{
    _graphics()->clear();
}

static WebGraphicHash* graphics()
{
    WebGraphicHash* hash = _graphics();

    if (hash->isEmpty()) {

        // prevent ~QPixmap running after ~QApplication (leaks native pixmaps)
        qAddPostRoutine(earlyClearGraphics);

        // QWebSettings::MissingImageGraphic
        hash->insert("missingImage", QPixmap(QLatin1String(":webkit/resources/missingImage.png")));
        // QWebSettings::MissingPluginGraphic
        hash->insert("nullPlugin", QPixmap(QLatin1String(":webkit/resources/nullPlugin.png")));
        // QWebSettings::DefaultFrameIconGraphic
        hash->insert("urlIcon", QPixmap(QLatin1String(":webkit/resources/urlIcon.png")));
        // QWebSettings::TextAreaSizeGripCornerGraphic
        hash->insert("textAreaResizeCorner", QPixmap(QLatin1String(":webkit/resources/textAreaResizeCorner.png")));
        // QWebSettings::DeleteButtonGraphic
        hash->insert("deleteButton", QPixmap(QLatin1String(":webkit/resources/deleteButton.png")));
        // QWebSettings::InputSpeechButtonGraphic
        hash->insert("inputSpeech", QPixmap(QLatin1String(":webkit/resources/inputSpeech.png")));
    }

    return hash;
}

// This function loads resources into WebKit
static QPixmap loadResourcePixmap(const char *name)
{
    return graphics()->value(name);
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

void Image::setPlatformResource(const char* name, const QPixmap& pixmap)
{
    WebGraphicHash* h = graphics();
    if (pixmap.isNull())
        h->remove(name);
    else
        h->insert(name, pixmap);
}

void Image::drawPattern(GraphicsContext* ctxt, const FloatRect& tileRect, const AffineTransform& patternTransform,
    const FloatPoint& phase, ColorSpace, CompositeOperator op, const FloatRect& destRect, BlendMode)
{
    QPixmap* framePixmap = nativeImageForCurrentFrame();
    if (!framePixmap) // If it's too early we won't have an image yet.
        return;

#if ENABLE(IMAGE_DECODER_DOWN_SAMPLING)
    FloatRect tileRectAdjusted = adjustSourceRectForDownSampling(tileRect, framePixmap->size());
#else
    FloatRect tileRectAdjusted = tileRect;
#endif

    // Qt interprets 0 width/height as full width/height so just short circuit.
    QRectF dr = QRectF(destRect).normalized();
    QRect tr = QRectF(tileRectAdjusted).toRect().normalized();
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
    if (transform.type() == QTransform::TxScale && p->transform().type() < QTransform::TxScale) {
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
    , m_decodedSize(0)
    , m_frameCount(1)
    , m_isSolidColor(false)
    , m_checkedForSolidColor(false)
    , m_animationFinished(true)
    , m_allDataReceived(true)
    , m_haveSize(true)
    , m_sizeAvailable(true)
    , m_haveFrameCount(true)
{
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

void BitmapImage::invalidatePlatformData()
{
}

QPixmap* prescaleImageIfRequired(QPainter* painter, QPixmap* image, QPixmap* buffer, const QRectF& destRect, QRectF* srcRect)
{
    // The quality of down scaling at 0.5x and below in QPainter is not very good
    // due to using bilinear sampling, so for high quality scaling we need to
    // perform scaling ourselves.
    ASSERT(image);
    ASSERT(painter);
    if (!(painter->renderHints() & QPainter::SmoothPixmapTransform))
        return image;

    if (painter->paintEngine()->type() != QPaintEngine::Raster)
        return image;

    QTransform transform = painter->combinedTransform();

    // Prescaling transforms that does more than scale or translate is not supported.
    if (transform.type() > QTransform::TxScale)
        return image;

    QRectF transformedDst = transform.mapRect(destRect);
    // Only prescale if downscaling to 0.5x or less
    if (transformedDst.width() * 2 > srcRect->width() && transformedDst.height() * 2 > srcRect->height())
        return image;

    // This may not work right with subpixel positions, but that can not currently happen.
    QRect pixelSrc = srcRect->toRect();
    QSize scaledSize = transformedDst.size().toSize();

    QString key = QStringLiteral("qtwebkit_prescaled_")
        % HexString<qint64>(image->cacheKey())
        % HexString<int>(pixelSrc.x()) % HexString<int>(pixelSrc.y())
        % HexString<int>(pixelSrc.width()) % HexString<int>(pixelSrc.height())
        % HexString<int>(scaledSize.width()) % HexString<int>(scaledSize.height());

    if (!QPixmapCache::find(key, buffer)) {
        if (pixelSrc != image->rect())
            *buffer = image->copy(pixelSrc).scaled(scaledSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        else
            *buffer = image->scaled(scaledSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        QPixmapCache::insert(key, *buffer);
    }

    *srcRect = QRectF(QPointF(), buffer->size());
    return buffer;
}

// Drawing Routines
void BitmapImage::draw(GraphicsContext* ctxt, const FloatRect& dst,
    const FloatRect& src, ColorSpace styleColorSpace, CompositeOperator op, BlendMode blendMode)
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

#if ENABLE(IMAGE_DECODER_DOWN_SAMPLING)
    normalizedSrc = adjustSourceRectForDownSampling(normalizedSrc, image->size());
#endif

    QPixmap prescaledBuffer;
    image = prescaleImageIfRequired(ctxt->platformContext(), image, &prescaledBuffer, normalizedDst, &normalizedSrc);

    CompositeOperator previousOperator = ctxt->compositeOperation();
    BlendMode previousBlendMode = ctxt->blendModeOperation();
    ctxt->setCompositeOperation(!image->hasAlpha() && op == CompositeSourceOver && blendMode == BlendModeNormal ? CompositeCopy : op, blendMode);

    if (ctxt->hasShadow()) {
        ShadowBlur shadow(ctxt->state());
        GraphicsContext* shadowContext = shadow.beginShadowLayer(ctxt, normalizedDst);
        if (shadowContext) {
            QPainter* shadowPainter = shadowContext->platformContext();
            shadowPainter->drawPixmap(normalizedDst, *image, normalizedSrc);
            shadow.endShadowLayer(ctxt);
        }
    }

    ctxt->platformContext()->drawPixmap(normalizedDst, *image, normalizedSrc);

    ctxt->setCompositeOperation(previousOperator, previousBlendMode);

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
    QPixmap* qPixmap = new QPixmap(qt_pixmapFromWinHBITMAP(hBitmap));

    return BitmapImage::create(qPixmap);
}
#endif

}


// vim: ts=4 sw=4 et
