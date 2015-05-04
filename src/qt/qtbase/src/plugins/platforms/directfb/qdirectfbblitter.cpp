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

#include "qdirectfbblitter.h"
#include "qdirectfbconvenience.h"

#include <QtGui/private/qpixmap_blitter_p.h>

#include <QDebug>
#include <QFile>

#include <directfb.h>

QT_BEGIN_NAMESPACE

static QBlittable::Capabilities dfb_blitter_capabilities()
{
    return QBlittable::Capabilities(QBlittable::SolidRectCapability
                                    |QBlittable::SourcePixmapCapability
                                    |QBlittable::SourceOverPixmapCapability
                                    |QBlittable::SourceOverScaledPixmapCapability
                                    |QBlittable::AlphaFillRectCapability
                                    |QBlittable::OpacityPixmapCapability
                                    |QBlittable::DrawScaledCachedGlyphsCapability
                                    );
}

QDirectFbBlitter::QDirectFbBlitter(const QSize &rect, IDirectFBSurface *surface)
    : QBlittable(rect, dfb_blitter_capabilities())
    , m_surface(surface)
    , m_debugPaint(false)
{
    m_surface->AddRef(m_surface.data());

    DFBSurfaceCapabilities surfaceCaps;
    m_surface->GetCapabilities(m_surface.data(), &surfaceCaps);
    m_premult = (surfaceCaps & DSCAPS_PREMULTIPLIED);
    if (qgetenv("QT_DIRECTFB_BLITTER_DEBUGPAINT").toInt())
        m_debugPaint = true;
}

QDirectFbBlitter::QDirectFbBlitter(const QSize &rect, bool alpha)
    : QBlittable(rect, dfb_blitter_capabilities())
    , m_premult(false)
    , m_debugPaint(false)
{
    DFBSurfaceDescription surfaceDesc;
    memset(&surfaceDesc,0,sizeof(DFBSurfaceDescription));
    surfaceDesc.width = rect.width();
    surfaceDesc.height = rect.height();

    // force alpha format to get AlphaFillRectCapability and ExtendedPixmapCapability support
    alpha = true;

    if (alpha) {
        m_premult = true;
        surfaceDesc.caps = DSCAPS_PREMULTIPLIED;
        surfaceDesc.pixelformat = QDirectFbBlitter::alphaPixmapFormat();
        surfaceDesc.flags = DFBSurfaceDescriptionFlags(DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_CAPS | DSDESC_PIXELFORMAT);
    } else {
        surfaceDesc.flags = DFBSurfaceDescriptionFlags(DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT);
        surfaceDesc.pixelformat = QDirectFbBlitter::pixmapFormat();
    }

    if (qgetenv("QT_DIRECTFB_BLITTER_DEBUGPAINT").toInt())
        m_debugPaint = true;

    IDirectFB *dfb = QDirectFbConvenience::dfbInterface();
    dfb->CreateSurface(dfb , &surfaceDesc, m_surface.outPtr());
    m_surface->Clear(m_surface.data(), 0, 0, 0, 0);
}

QDirectFbBlitter::~QDirectFbBlitter()
{
    unlock();
}

DFBSurfacePixelFormat QDirectFbBlitter::alphaPixmapFormat()
{
    return DSPF_ARGB;
}

DFBSurfacePixelFormat QDirectFbBlitter::pixmapFormat()
{
    return DSPF_RGB32;
}

DFBSurfacePixelFormat QDirectFbBlitter::selectPixmapFormat(bool withAlpha)
{
    return withAlpha ? alphaPixmapFormat() : pixmapFormat();
}

void QDirectFbBlitter::fillRect(const QRectF &rect, const QColor &color)
{
    alphaFillRect(rect, color, QPainter::CompositionMode_Source);
}

void QDirectFbBlitter::drawPixmap(const QRectF &rect, const QPixmap &pixmap, const QRectF &srcRect)
{
    drawPixmapOpacity(rect, pixmap, srcRect, QPainter::CompositionMode_SourceOver, 1.0);
}

void QDirectFbBlitter::alphaFillRect(const QRectF &rect, const QColor &color, QPainter::CompositionMode cmode)
{
    int x, y, w, h;
    DFBResult result;

    // check parameters
    rect.toRect().getRect(&x, &y ,&w, &h);
    if ((w <= 0) || (h <= 0)) return;

    if ((cmode == QPainter::CompositionMode_Source) || (color.alpha() == 255)) {
        // CompositionMode_Source case or CompositionMode_SourceOver with opaque color

        m_surface->SetDrawingFlags(m_surface.data(),
            DFBSurfaceDrawingFlags(m_premult ? (DSDRAW_NOFX | DSDRAW_SRC_PREMULTIPLY) : DSDRAW_NOFX));
        m_surface->SetPorterDuff(m_surface.data(), DSPD_SRC);

    } else {
        // CompositionMode_SourceOver case

        // check if operation is useless
        if (color.alpha() == 0)
            return;

        m_surface->SetDrawingFlags(m_surface.data(),
            DFBSurfaceDrawingFlags(m_premult ? (DSDRAW_BLEND | DSDRAW_SRC_PREMULTIPLY) : DSDRAW_BLEND));
        m_surface->SetPorterDuff(m_surface.data(), DSPD_SRC_OVER);
    }

    // set color
    m_surface->SetColor(m_surface.data(), color.red(), color.green(), color.blue(), color.alpha());

    // perform fill
    result = m_surface->FillRectangle(m_surface.data(), x, y, w, h);
    if (result != DFB_OK)
        DirectFBError("QDirectFBBlitter::alphaFillRect()", result);
    if (m_debugPaint)
        drawDebugRect(QRect(x, y, w, h), QColor(Qt::blue));
}

void QDirectFbBlitter::drawPixmapOpacity(const QRectF &rect, const QPixmap &pixmap, const QRectF &subrect, QPainter::CompositionMode cmode, qreal opacity)
{
    QRect sQRect = subrect.toRect();
    QRect dQRect = rect.toRect();
    DFBRectangle sRect = { sQRect.x(), sQRect.y(), sQRect.width(), sQRect.height() };
    DFBRectangle dRect = { dQRect.x(), dQRect.y(), dQRect.width(), dQRect.height() };
    DFBResult result;

    // skip if dst too small
    if ((dRect.w <= 0) || (dRect.h <= 0)) return;

    // correct roundings if needed
    if (sRect.w <= 0) sRect.w = 1;
    if (sRect.h <= 0) sRect.h = 1;

    QDirectFbBlitterPlatformPixmap *blitPm = static_cast<QDirectFbBlitterPlatformPixmap *>(pixmap.handle());
    QDirectFbBlitter *dfbBlitter = static_cast<QDirectFbBlitter *>(blitPm->blittable());
    dfbBlitter->unlock();

    IDirectFBSurface *s = dfbBlitter->m_surface.data();

    DFBSurfaceBlittingFlags blittingFlags = DFBSurfaceBlittingFlags(DSBLIT_BLEND_ALPHACHANNEL);
    DFBSurfacePorterDuffRule porterDuff = (cmode == QPainter::CompositionMode_SourceOver) ? DSPD_SRC_OVER : DSPD_SRC;

    if (opacity != 1.0)
    {
        blittingFlags = DFBSurfaceBlittingFlags(blittingFlags | DSBLIT_BLEND_COLORALPHA | (m_premult ? DSBLIT_SRC_PREMULTCOLOR : 0));
        m_surface->SetColor(m_surface.data(), 0xff, 0xff, 0xff, (u8) (opacity * 255.0));
    }

    m_surface->SetBlittingFlags(m_surface.data(), DFBSurfaceBlittingFlags(blittingFlags));
    m_surface->SetPorterDuff(m_surface.data(), porterDuff);

    if (cmode == QPainter::CompositionMode_SourceOver)
        m_surface->SetDstBlendFunction(m_surface.data(), DSBF_INVSRCALPHA);

    if ((sRect.w == dRect.w) && (sRect.h == dRect.h)) {
        result = m_surface->Blit(m_surface.data(), s, &sRect, dRect.x, dRect.y);
        if (result != DFB_OK)
            DirectFBError("QDirectFBBlitter::drawPixmapOpacity()", result);
        if (m_debugPaint)
            drawDebugRect(QRect(dRect.x, dRect.y, sRect.w, sRect.h), QColor(Qt::green));
    } else {
        result = m_surface->StretchBlit(m_surface.data(), s, &sRect, &dRect);
        if (result != DFB_OK)
            DirectFBError("QDirectFBBlitter::drawPixmapOpacity()", result);
        if (m_debugPaint)
            drawDebugRect(QRect(dRect.x, dRect.y, dRect.w, dRect.h), QColor(Qt::red));
    }
}

bool QDirectFbBlitter::drawCachedGlyphs(const QPaintEngineState *state, QFontEngine::GlyphFormat glyphFormat, int numGlyphs, const glyph_t *glyphs, const QFixedPoint *positions, QFontEngine *fontEngine)
{
    void *cacheKey = QDirectFbConvenience::dfbInterface();

    QDirectFbTextureGlyphCache *cache =
            static_cast<QDirectFbTextureGlyphCache *>(fontEngine->glyphCache(cacheKey, glyphFormat, state->transform()));
    if (!cache) {
        cache = new QDirectFbTextureGlyphCache(glyphFormat, state->transform());
        fontEngine->setGlyphCache(cacheKey, cache);
    }

    cache->populate(fontEngine, numGlyphs, glyphs, positions);
    cache->fillInPendingGlyphs();

    if (cache->image().width() == 0 || cache->image().height() == 0)
        return false;

    const int margin = fontEngine->glyphMargin(glyphFormat);

    QVarLengthArray<DFBRectangle, 64> sourceRects(numGlyphs);
    QVarLengthArray<DFBPoint, 64> destPoints(numGlyphs);
    int nGlyphs = 0;

    for (int i=0; i<numGlyphs; ++i) {

        QFixed subPixelPosition = fontEngine->subPixelPositionForX(positions[i].x);
        QTextureGlyphCache::GlyphAndSubPixelPosition glyph(glyphs[i], subPixelPosition);
        const QTextureGlyphCache::Coord &c = cache->coords[glyph];
        if (c.isNull())
            continue;

        int x = qFloor(positions[i].x) + c.baseLineX - margin;
        int y = qRound(positions[i].y) - c.baseLineY - margin;

        // printf("drawing [%d %d %d %d] baseline [%d %d], glyph: %d, to: %d %d, pos: %d %d\n",
        //        c.x, c.y,
        //        c.w, c.h,
        //        c.baseLineX, c.baseLineY,
        //        glyphs[i],
        //        x, y,
        //        positions[i].x.toInt(), positions[i].y.toInt());

        sourceRects[nGlyphs].x = c.x;
        sourceRects[nGlyphs].y = c.y;
        sourceRects[nGlyphs].w = c.w;
        sourceRects[nGlyphs].h = c.h;
        destPoints[nGlyphs].x = x;
        destPoints[nGlyphs].y = y;
        ++nGlyphs;
    }

    const QColor color = state->pen().color();
    m_surface->SetColor(m_surface.data(), color.red(), color.green(), color.blue(), color.alpha());

    m_surface->SetSrcBlendFunction(m_surface.data(), DSBF_SRCALPHA);
    m_surface->SetDstBlendFunction(m_surface.data(), DSBF_INVSRCALPHA);

    int flags = DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_COLORIZE;
    if (color.alpha() != 0xff)
        flags |= DSBLIT_BLEND_COLORALPHA;
    m_surface->SetBlittingFlags(m_surface.data(), DFBSurfaceBlittingFlags(flags));

    const QRasterPaintEngineState *rs = static_cast<const QRasterPaintEngineState*>(state);
    if (rs->clip && rs->clip->enabled) {
        Q_ASSERT(rs->clip->hasRectClip);
        DFBRegion dfbClip;
        dfbClip.x1 = rs->clip->clipRect.x();
        dfbClip.y1 = rs->clip->clipRect.y();
        dfbClip.x2 = rs->clip->clipRect.right();
        dfbClip.y2 = rs->clip->clipRect.bottom();
        m_surface->SetClip(m_surface.data(), &dfbClip);
    }

    m_surface->BatchBlit(m_surface.data(), cache->sourceSurface(), sourceRects.constData(), destPoints.constData(), nGlyphs);

    if (m_debugPaint) {
        for (int i = 0; i < nGlyphs; ++i) {
            drawDebugRect(QRect(destPoints[i].x, destPoints[i].y, sourceRects[i].w, sourceRects[i].h), QColor(Qt::yellow));
        }
    }

    if (rs->clip && rs->clip->enabled)
        m_surface->SetClip(m_surface.data(), 0);
    return true;
}

QImage *QDirectFbBlitter::doLock()
{
    Q_ASSERT(m_surface);
    Q_ASSERT(size().isValid());

    void *mem;
    int bpl;
    const DFBResult result = m_surface->Lock(m_surface.data(), DFBSurfaceLockFlags(DSLF_WRITE|DSLF_READ), static_cast<void**>(&mem), &bpl);
    if (result == DFB_OK) {
        DFBSurfacePixelFormat dfbFormat;
        DFBSurfaceCapabilities dfbCaps;
        m_surface->GetPixelFormat(m_surface.data(), &dfbFormat);
        m_surface->GetCapabilities(m_surface.data(), &dfbCaps);
        QImage::Format format = QDirectFbConvenience::imageFormatFromSurfaceFormat(dfbFormat, dfbCaps);
        int w, h;
        m_surface->GetSize(m_surface.data(), &w, &h);
        m_image = QImage(static_cast<uchar *>(mem),w,h,bpl,format);
    } else {
        DirectFBError("Failed to lock image", result);
    }

    return &m_image;
}

bool QDirectFbBlitterPlatformPixmap::fromDataBufferDescription(const DFBDataBufferDescription &dataBufferDescription)
{
    DFBResult result;
    IDirectFB *dfb = QDirectFbConvenience::dfbInterface();

    // Create a data buffer
    QDirectFBPointer<IDirectFBDataBuffer> dataBuffer;
    result = dfb->CreateDataBuffer(dfb, &dataBufferDescription, dataBuffer.outPtr());
    if (result != DFB_OK) {
        DirectFBError(QDFB_PRETTY, result);
        return false;
    }

    // Create the image provider
    QDirectFBPointer<IDirectFBImageProvider> provider;
    result = dataBuffer->CreateImageProvider(dataBuffer.data(), provider.outPtr());
    if (result != DFB_OK) {
        DirectFBError(QDFB_PRETTY, result);
        return false;
    }

    // Extract image information
    DFBImageDescription imageDescription;
    result = provider->GetImageDescription(provider.data(), &imageDescription);
    if (result != DFB_OK) {
        DirectFBError(QDFB_PRETTY, result);
        return false;
    }

    // Can we handle this directlu?
    if (imageDescription.caps & DICAPS_COLORKEY)
        return false;

    DFBSurfaceDescription surfaceDescription;
    result = provider->GetSurfaceDescription(provider.data(), &surfaceDescription);
    if (result != DFB_OK) {
        DirectFBError(QDFB_PRETTY, result);
        return false;
    }

    m_alpha = imageDescription.caps & DICAPS_ALPHACHANNEL;
    resize(surfaceDescription.width, surfaceDescription.height);
    // TODO: FIXME; update d


    result = provider->RenderTo(provider.data(), dfbBlitter()->dfbSurface(), 0);
    if (result != DFB_OK) {
        DirectFBError(QDFB_PRETTY, result);
        return false;
    }

    return true;
}

bool QDirectFbBlitterPlatformPixmap::fromFile(const QString &filename, const char *format,
                                              Qt::ImageConversionFlags flags)
{
    // If we can't find the file, pass it on to the base class as it is
    // trying harder by appending various extensions to the path.
    if (!QFile::exists(filename))
        return QBlittablePlatformPixmap::fromFile(filename, format, flags);

    // Stop if there is a requirement for colors
    if (flags != Qt::AutoColor)
        return QBlittablePlatformPixmap::fromFile(filename, format, flags);

    // Deal with resources
    if (filename.startsWith(QLatin1Char(':')))
        return QBlittablePlatformPixmap::fromFile(filename, format, flags);

    // Try to use directfb to load it.
    DFBDataBufferDescription description;
    description.flags = DBDESC_FILE;
    const QByteArray fileNameData = filename.toLocal8Bit();
    description.file = fileNameData.constData();
    if (fromDataBufferDescription(description))
        return true;

    // Fallback
    return QBlittablePlatformPixmap::fromFile(filename, format, flags);
}

void QDirectFbBlitter::doUnlock()
{
    m_surface->Unlock(m_surface.data());
}

void QDirectFbBlitter::drawDebugRect(const QRect &rect, const QColor &color)
{
    int x, y, w, h;
    DFBResult result;

    // check parameters
    rect.getRect(&x, &y ,&w, &h);
    if ((w <= 0) || (h <= 0)) return;

    m_surface->SetDrawingFlags(m_surface.data(),
    DFBSurfaceDrawingFlags(m_premult ? (DSDRAW_BLEND | DSDRAW_SRC_PREMULTIPLY) : DSDRAW_BLEND));
    m_surface->SetPorterDuff(m_surface.data(), DSPD_SRC_OVER);

    // set color
    m_surface->SetColor(m_surface.data(), color.red(), color.green(), color.blue(), 120);

    result = m_surface->DrawLine(m_surface.data(), x, y, x + w-1, y);
    if (result != DFB_OK)
        DirectFBError("QDirectFBBlitter::drawDebugRect()", result);
    result = m_surface->DrawLine(m_surface.data(), x + w-1, y, x + w-1, y + h-1);
    if (result != DFB_OK)
        DirectFBError("QDirectFBBlitter::drawDebugRect()", result);
    result = m_surface->DrawLine(m_surface.data(), x + w-1, y + h-1, x, y + h-1);
    if (result != DFB_OK)
        DirectFBError("QDirectFBBlitter::drawDebugRect()", result);
    result = m_surface->DrawLine(m_surface.data(), x, y + h-1, x, y);
    if (result != DFB_OK)
        DirectFBError("QDirectFBBlitter::drawDebugRect()", result);

    m_surface->SetColor(m_surface.data(), color.red(), color.green(), color.blue(), 10);
    result = m_surface->FillRectangle(m_surface.data(), x, y, w, h);
    if (result != DFB_OK)
        DirectFBError("QDirectFBBlitter::drawDebugRect()", result);
}

void QDirectFbTextureGlyphCache::resizeTextureData(int width, int height)
{
    m_surface.reset();;
    QImageTextureGlyphCache::resizeTextureData(width, height);
}

IDirectFBSurface *QDirectFbTextureGlyphCache::sourceSurface()
{
    if (m_surface.isNull()) {
        const QImage &source = image();
        DFBSurfaceDescription desc;
        memset(&desc, 0, sizeof(desc));
        desc.flags = DFBSurfaceDescriptionFlags(DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_PREALLOCATED | DSDESC_CAPS);
        desc.width = source.width();
        desc.height = source.height();
        desc.caps = DSCAPS_SYSTEMONLY;

        switch (source.format()) {
        case QImage::Format_Mono:
            desc.pixelformat = DSPF_A1;
            break;
        case QImage::Format_Indexed8:
            desc.pixelformat = DSPF_A8;
            break;
        default:
            qFatal("QDirectFBTextureGlyphCache: Unsupported source texture image format.");
            break;
        }

        desc.preallocated[0].data = const_cast<void*>(static_cast<const void*>(source.bits()));
        desc.preallocated[0].pitch = source.bytesPerLine();
        desc.preallocated[1].data = 0;
        desc.preallocated[1].pitch = 0;

        IDirectFB *dfb = QDirectFbConvenience::dfbInterface();
        dfb->CreateSurface(dfb , &desc, m_surface.outPtr());
    }
    return m_surface.data();
}

QT_END_NAMESPACE
