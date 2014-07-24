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
                                    |QBlittable::OpacityPixmapCapability);
}

QDirectFbBlitter::QDirectFbBlitter(const QSize &rect, IDirectFBSurface *surface)
    : QBlittable(rect, dfb_blitter_capabilities())
        , m_surface(surface)
{
    m_surface->AddRef(m_surface.data());

    DFBSurfaceCapabilities surfaceCaps;
    m_surface->GetCapabilities(m_surface.data(), &surfaceCaps);
    m_premult = (surfaceCaps & DSCAPS_PREMULTIPLIED);
}

QDirectFbBlitter::QDirectFbBlitter(const QSize &rect, bool alpha)
    : QBlittable(rect, dfb_blitter_capabilities()), m_premult(false)
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

    if ((sRect.w == dRect.w) && (sRect.h == dRect.h))
        result = m_surface->Blit(m_surface.data(), s, &sRect, dRect.x, dRect.y);
    else
        result = m_surface->StretchBlit(m_surface.data(), s, &sRect, &dRect);

    if (result != DFB_OK)
        DirectFBError("QDirectFBBlitter::drawPixmapExtended()", result);
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

QT_END_NAMESPACE
