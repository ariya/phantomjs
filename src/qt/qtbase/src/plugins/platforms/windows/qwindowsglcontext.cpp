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

#include "qwindowsglcontext.h"
#include "qwindowscontext.h"
#include "qwindowswindow.h"
#include "qwindowsintegration.h"

#include <QtCore/QDebug>
#include <QtCore/QSysInfo>
#include <QtGui/QGuiApplication>
#include <qpa/qplatformnativeinterface.h>

#include <algorithm>

#include <wingdi.h>
#include <GL/gl.h>

// #define DEBUG_GL

// ARB extension API
#ifndef WGL_ARB_multisample
#define WGL_SAMPLE_BUFFERS_ARB               0x2041
#define WGL_SAMPLES_ARB                      0x2042
#endif

#ifndef WGL_ARB_pixel_format
#define WGL_NUMBER_PIXEL_FORMATS_ARB   0x2000
#define WGL_DRAW_TO_WINDOW_ARB         0x2001
#define WGL_DRAW_TO_BITMAP_ARB         0x2002
#define WGL_ACCELERATION_ARB           0x2003
#define WGL_NEED_PALETTE_ARB           0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB    0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB     0x2006
#define WGL_SWAP_METHOD_ARB            0x2007
#define WGL_NUMBER_OVERLAYS_ARB        0x2008
#define WGL_NUMBER_UNDERLAYS_ARB       0x2009
#define WGL_TRANSPARENT_ARB            0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB  0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB 0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB 0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB 0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB 0x203B
#define WGL_SHARE_DEPTH_ARB            0x200C
#define WGL_SHARE_STENCIL_ARB          0x200D
#define WGL_SHARE_ACCUM_ARB            0x200E
#define WGL_SUPPORT_GDI_ARB            0x200F
#define WGL_SUPPORT_OPENGL_ARB         0x2010
#define WGL_DOUBLE_BUFFER_ARB          0x2011
#define WGL_STEREO_ARB                 0x2012
#define WGL_PIXEL_TYPE_ARB             0x2013
#define WGL_COLOR_BITS_ARB             0x2014
#define WGL_RED_BITS_ARB               0x2015
#define WGL_RED_SHIFT_ARB              0x2016
#define WGL_GREEN_BITS_ARB             0x2017
#define WGL_GREEN_SHIFT_ARB            0x2018
#define WGL_BLUE_BITS_ARB              0x2019
#define WGL_BLUE_SHIFT_ARB             0x201A
#define WGL_ALPHA_BITS_ARB             0x201B
#define WGL_ALPHA_SHIFT_ARB            0x201C
#define WGL_ACCUM_BITS_ARB             0x201D
#define WGL_ACCUM_RED_BITS_ARB         0x201E
#define WGL_ACCUM_GREEN_BITS_ARB       0x201F
#define WGL_ACCUM_BLUE_BITS_ARB        0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB       0x2021
#define WGL_DEPTH_BITS_ARB             0x2022
#define WGL_STENCIL_BITS_ARB           0x2023
#define WGL_AUX_BUFFERS_ARB            0x2024
#define WGL_NO_ACCELERATION_ARB        0x2025
#define WGL_GENERIC_ACCELERATION_ARB   0x2026
#define WGL_FULL_ACCELERATION_ARB      0x2027
#define WGL_SWAP_EXCHANGE_ARB          0x2028
#define WGL_SWAP_COPY_ARB              0x2029
#define WGL_SWAP_UNDEFINED_ARB         0x202A
#define WGL_TYPE_RGBA_ARB              0x202B
#define WGL_TYPE_COLORINDEX_ARB        0x202C
#endif

#ifndef WGL_ARB_create_context
#define WGL_CONTEXT_MAJOR_VERSION_ARB               0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB               0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB                 0x2093
#define WGL_CONTEXT_FLAGS_ARB                       0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB                0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB                   0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB      0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB            0x0001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB   0x0002
// Error codes returned by GetLastError().
#define ERROR_INVALID_VERSION_ARB                   0x2095
#define ERROR_INVALID_PROFILE_ARB                   0x2096
#endif

#ifndef GL_VERSION_3_2
#define GL_CONTEXT_PROFILE_MASK                     0x9126
#define GL_MAJOR_VERSION                            0x821B
#define GL_MINOR_VERSION                            0x821C
#define GL_NUM_EXTENSIONS                           0x821D
#define GL_CONTEXT_FLAGS                            0x821E
#define GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT      0x0001
#endif

#ifndef GL_CONTEXT_FLAG_DEBUG_BIT
#define GL_CONTEXT_FLAG_DEBUG_BIT 0x00000002
#endif

QT_BEGIN_NAMESPACE

template <class MaskType, class FlagType> inline bool testFlag(MaskType mask, FlagType flag)
{
    return (mask & MaskType(flag)) != 0;
}

static inline bool hasGLOverlay(const PIXELFORMATDESCRIPTOR &pd)
{ return (pd.bReserved & 0x0f) != 0; }

static inline bool isDirectRendering(const PIXELFORMATDESCRIPTOR &pfd)
{ return (pfd.dwFlags & PFD_GENERIC_ACCELERATED) || !(pfd.dwFlags & PFD_GENERIC_FORMAT); }

static inline void initPixelFormatDescriptor(PIXELFORMATDESCRIPTOR *d)
{
    memset(d, 0, sizeof(PIXELFORMATDESCRIPTOR));
    d->nSize = sizeof(PIXELFORMATDESCRIPTOR);
    d->nVersion = 1;
}

QDebug operator<<(QDebug d, const PIXELFORMATDESCRIPTOR &pd)
{
    QDebug nsp = d.nospace();
    nsp << "PIXELFORMATDESCRIPTOR "
        << "dwFlags=" << hex << showbase << pd.dwFlags << dec << noshowbase;
    if (pd.dwFlags & PFD_DRAW_TO_WINDOW) nsp << " PFD_DRAW_TO_WINDOW";
    if (pd.dwFlags & PFD_DRAW_TO_BITMAP) nsp << " PFD_DRAW_TO_BITMAP";
    if (pd.dwFlags & PFD_SUPPORT_GDI) nsp << " PFD_SUPPORT_GDI";
    if (pd.dwFlags & PFD_SUPPORT_OPENGL) nsp << " PFD_SUPPORT_OPENGL";
    if (pd.dwFlags & PFD_GENERIC_ACCELERATED) nsp << " PFD_GENERIC_ACCELERATED";
    if (pd.dwFlags & PFD_SUPPORT_DIRECTDRAW) nsp << " PFD_SUPPORT_DIRECTDRAW";
    if (pd.dwFlags & PFD_DIRECT3D_ACCELERATED) nsp << " PFD_DIRECT3D_ACCELERATED";
    if (pd.dwFlags & PFD_SUPPORT_COMPOSITION) nsp << " PFD_SUPPORT_COMPOSITION";
    if (pd.dwFlags & PFD_GENERIC_FORMAT) nsp << " PFD_GENERIC_FORMAT";
    if (pd.dwFlags & PFD_NEED_PALETTE) nsp << " PFD_NEED_PALETTE";
    if (pd.dwFlags & PFD_NEED_SYSTEM_PALETTE) nsp << " PFD_NEED_SYSTEM_PALETTE";
    if (pd.dwFlags & PFD_DOUBLEBUFFER) nsp << " PFD_DOUBLEBUFFER";
    if (pd.dwFlags & PFD_STEREO) nsp << " PFD_STEREO";
    if (pd.dwFlags & PFD_SWAP_LAYER_BUFFERS) nsp << " PFD_SWAP_LAYER_BUFFERS";
    if (hasGLOverlay(pd)) nsp << " overlay";
    nsp << " iPixelType=" << pd.iPixelType << " cColorBits=" << pd.cColorBits
        << " cRedBits=" << pd.cRedBits << " cRedShift=" << pd.cRedShift
        << " cGreenBits=" << pd.cGreenBits << " cGreenShift=" << pd.cGreenShift
        << " cBlueBits=" << pd.cBlueBits << " cBlueShift=" << pd.cBlueShift;
    nsp  << " cDepthBits=" << pd.cDepthBits;
    if (pd.cStencilBits)
        nsp << " cStencilBits=" << pd.cStencilBits;
    if (pd.cAuxBuffers)
        nsp << " cAuxBuffers=" << pd.cAuxBuffers;
    nsp << " iLayerType=" << pd.iLayerType;
    if (pd.dwVisibleMask)
        nsp << " dwVisibleMask=" << pd.dwVisibleMask;
    if (pd.cAlphaBits)
        nsp << " cAlphaBits=" << pd.cAlphaBits << " cAlphaShift=" << pd.cAlphaShift;
    if (pd.cAccumBits)
        nsp << " cAccumBits=" << pd.cAccumBits << " cAccumRedBits=" << pd.cAccumRedBits
        << " cAccumGreenBits=" << pd.cAccumGreenBits << " cAccumBlueBits=" << pd.cAccumBlueBits
        << " cAccumAlphaBits=" << pd.cAccumAlphaBits;
    return d;
}

// Check whether an obtained PIXELFORMATDESCRIPTOR matches the request.
static inline bool
    isAcceptableFormat(const QWindowsOpenGLAdditionalFormat &additional,
                       const PIXELFORMATDESCRIPTOR &pfd,
                       bool ignoreGLSupport = false) // ARB format may not contain it.
{
    const bool pixmapRequested = testFlag(additional.formatFlags, QWindowsGLRenderToPixmap);
    return (ignoreGLSupport || testFlag(pfd.dwFlags, PFD_SUPPORT_OPENGL))
        && testFlag(pfd.dwFlags, PFD_DRAW_TO_BITMAP) == pixmapRequested
        && hasGLOverlay(pfd) == testFlag(additional.formatFlags, QWindowsGLOverlay)
        && (!pixmapRequested || pfd.cColorBits == additional.pixmapDepth);
}

static void describeFormats(HDC hdc)
{
    const int pfiMax = DescribePixelFormat(hdc, 0, 0, NULL);
    for (int i = 0; i < pfiMax; i++) {
        PIXELFORMATDESCRIPTOR pfd;
        initPixelFormatDescriptor(&pfd);
        DescribePixelFormat(hdc, i, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
        qCDebug(lcQpaGl) << '#' << i << '/' << pfiMax << ':' << pfd;
    }
}

// Classic GDI API
namespace GDI {
static QSurfaceFormat
    qSurfaceFormatFromPixelFormat(const PIXELFORMATDESCRIPTOR &pfd,
                                         QWindowsOpenGLAdditionalFormat *additionalIn = 0)
{
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    if (pfd.dwFlags & PFD_DOUBLEBUFFER)
        format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setDepthBufferSize(pfd.cDepthBits);

    if (pfd.iPixelType == PFD_TYPE_RGBA)
        format.setAlphaBufferSize(pfd.cAlphaBits);
    format.setRedBufferSize(pfd.cRedBits);
    format.setGreenBufferSize(pfd.cGreenBits);
    format.setBlueBufferSize(pfd.cBlueBits);
    format.setStencilBufferSize(pfd.cStencilBits);
    format.setStereo(pfd.dwFlags & PFD_STEREO);
    if (additionalIn) {
        QWindowsOpenGLAdditionalFormat additional;
        if (isDirectRendering(pfd))
            additional.formatFlags |= QWindowsGLDirectRendering;
        if (hasGLOverlay(pfd))
            additional.formatFlags |= QWindowsGLOverlay;
        if (pfd.cAccumRedBits)
            additional.formatFlags |= QWindowsGLAccumBuffer;
        if (testFlag(pfd.dwFlags, PFD_DRAW_TO_BITMAP)) {
            additional.formatFlags |= QWindowsGLRenderToPixmap;
            additional.pixmapDepth = pfd.cColorBits;
        }
        *additionalIn = additional;
    }
    return format;
}

static PIXELFORMATDESCRIPTOR
    qPixelFormatFromSurfaceFormat(const QSurfaceFormat &format,
                                  const QWindowsOpenGLAdditionalFormat &additional)
{
    PIXELFORMATDESCRIPTOR pfd;
    initPixelFormatDescriptor(&pfd);
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.iLayerType  = PFD_MAIN_PLANE;
    pfd.dwFlags = PFD_SUPPORT_OPENGL;
    if (QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA)
        pfd.dwFlags = PFD_SUPPORT_COMPOSITION;
    const bool isPixmap = (additional.formatFlags & QWindowsGLRenderToPixmap) != 0;
    pfd.dwFlags |= isPixmap ? PFD_DRAW_TO_BITMAP : PFD_DRAW_TO_WINDOW;
    if (!(additional.formatFlags & QWindowsGLDirectRendering))
        pfd.dwFlags |= PFD_GENERIC_FORMAT;

    if (format.stereo())
        pfd.dwFlags |= PFD_STEREO;
    if (format.swapBehavior() != QSurfaceFormat::SingleBuffer && !isPixmap)
        pfd.dwFlags |= PFD_DOUBLEBUFFER;
    pfd.cDepthBits =
        format.depthBufferSize() >= 0 ? format.depthBufferSize() : 32;
    pfd.cAlphaBits = format.alphaBufferSize() > 0 ? format.alphaBufferSize() : 8;
    pfd.cStencilBits = format.stencilBufferSize() > 0 ? format.stencilBufferSize() : 8;
    if (additional.formatFlags & QWindowsGLAccumBuffer)
        pfd.cAccumRedBits = pfd.cAccumGreenBits = pfd.cAccumBlueBits = pfd.cAccumAlphaBits = 16;
    return pfd;
}

// Choose a suitable pixelformat using GDI WinAPI in case ARB
// functions cannot be found. First tries to find a suitable
// format using GDI function ChoosePixelFormat(). Since that
// does not handle overlay and direct-rendering requests, manually loop
// over the available formats to find the best one.
// Note: As of Windows 7, it seems direct-rendering is handled, so,
// the code might be obsolete?
static int choosePixelFormat(HDC hdc, const QSurfaceFormat &format,
                            const QWindowsOpenGLAdditionalFormat &additional,
                            PIXELFORMATDESCRIPTOR *obtainedPfd)
{
    // 1) Try ChoosePixelFormat().
    PIXELFORMATDESCRIPTOR requestedPfd = qPixelFormatFromSurfaceFormat(format, QWindowsGLDirectRendering);
    initPixelFormatDescriptor(obtainedPfd);
    int pixelFormat = ChoosePixelFormat(hdc, &requestedPfd);
    if (pixelFormat >= 0) {
        DescribePixelFormat(hdc, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), obtainedPfd);
        if (isAcceptableFormat(additional, *obtainedPfd))
            return pixelFormat;
    }
    // 2) No matching format found, manual search loop.
    const int pfiMax = DescribePixelFormat(hdc, 0, 0, NULL);
    int bestScore = -1;
    int bestPfi = -1;
    const bool stereoRequested = format.stereo();
    const bool accumBufferRequested = testFlag(additional.formatFlags, QWindowsGLAccumBuffer);
    const bool doubleBufferRequested = format.swapBehavior() == QSurfaceFormat::DoubleBuffer;
    const bool directRenderingRequested = testFlag(additional.formatFlags, QWindowsGLDirectRendering);
    for (int pfi = 1; pfi <= pfiMax; pfi++) {
        PIXELFORMATDESCRIPTOR checkPfd;
        initPixelFormatDescriptor(&checkPfd);
        DescribePixelFormat(hdc, pfi, sizeof(PIXELFORMATDESCRIPTOR), &checkPfd);
        if (isAcceptableFormat(additional, checkPfd)) {
            int score = checkPfd.cColorBits + checkPfd.cAlphaBits + checkPfd.cStencilBits;
            if (accumBufferRequested)
                score += checkPfd.cAccumBits;
            if (doubleBufferRequested == testFlag(checkPfd.dwFlags, PFD_DOUBLEBUFFER))
                score += 1000;
            if (stereoRequested == testFlag(checkPfd.dwFlags, PFD_STEREO))
                score += 2000;
            if (directRenderingRequested == isDirectRendering(checkPfd))
                score += 4000;
            if (checkPfd.iPixelType == PFD_TYPE_RGBA)
                score += 8000;
            if (score > bestScore) {
                bestScore = score;
                bestPfi = pfi;
                *obtainedPfd = checkPfd;
            }
            qCDebug(lcQpaGl) << __FUNCTION__ << "    checking  " << pfi << '/' << pfiMax
                << " score=" << score << " (best " << bestPfi << '/' << bestScore << ") " << checkPfd;
        }
    } // for
    if (bestPfi > 0)
        pixelFormat = bestPfi;
    return pixelFormat;
}

static inline HGLRC createContext(HDC hdc, HGLRC shared)
{
    HGLRC result = wglCreateContext(hdc);
    if (!result) {
        qErrnoWarning("%s: wglCreateContext failed.", __FUNCTION__);
        return 0;
    }
    if (shared && !wglShareLists(shared, result))
        qErrnoWarning("%s: wglShareLists() failed.", __FUNCTION__);
    return result;
}
} // namespace GDI

// ARB OpenGL extension API
namespace ARB {
// Choose a suitable pixelformat using ARB extension functions.
static int choosePixelFormat(HDC hdc,
                             const QOpenGLStaticContext &staticContext,
                             const QSurfaceFormat &format,
                             const QWindowsOpenGLAdditionalFormat &additional,
                             PIXELFORMATDESCRIPTOR *obtainedPfd)
{
    enum { attribSize =40 };
    if ((additional.formatFlags & QWindowsGLRenderToPixmap) || !staticContext.hasExtensions())
        return 0;

    int iAttributes[attribSize];
    std::fill(iAttributes, iAttributes + attribSize, int(0));
    int i = 0;
    iAttributes[i++] = WGL_ACCELERATION_ARB;
    iAttributes[i++] = testFlag(additional.formatFlags, QWindowsGLDirectRendering) ?
                       WGL_FULL_ACCELERATION_ARB : WGL_NO_ACCELERATION_ARB;
    iAttributes[i++] = WGL_SUPPORT_OPENGL_ARB;
    iAttributes[i++] = TRUE;
    iAttributes[i++] = WGL_DRAW_TO_WINDOW_ARB;
    iAttributes[i++] = TRUE;
    iAttributes[i++] = WGL_COLOR_BITS_ARB;

    iAttributes[i++] = (format.redBufferSize() > 0)
                       && (format.greenBufferSize() > 0)
                       && (format.blueBufferSize() > 0) ?
                       format.redBufferSize() + format.greenBufferSize() + format.blueBufferSize() :
                       24;
    switch (format.swapBehavior()) {
    case QSurfaceFormat::SingleBuffer:
        iAttributes[i++] = WGL_DOUBLE_BUFFER_ARB;
        iAttributes[i++] = FALSE;
        break;
    case QSurfaceFormat::DefaultSwapBehavior:
    case QSurfaceFormat::DoubleBuffer:
    case QSurfaceFormat::TripleBuffer:
        iAttributes[i++] = WGL_DOUBLE_BUFFER_ARB;
        iAttributes[i++] = TRUE;
        break;
    }
    if (format.stereo()) {
        iAttributes[i++] = WGL_STEREO_ARB;
        iAttributes[i++] = TRUE;
    }
    if (format.depthBufferSize() >= 0) {
        iAttributes[i++] = WGL_DEPTH_BITS_ARB;
        iAttributes[i++] = format.depthBufferSize();
    }
    iAttributes[i++] = WGL_PIXEL_TYPE_ARB;
    iAttributes[i++] = WGL_TYPE_RGBA_ARB;
    if (format.redBufferSize() >= 0) {
        iAttributes[i++] = WGL_RED_BITS_ARB;
        iAttributes[i++] = format.redBufferSize();
    }
    if (format.greenBufferSize() >= 0) {
        iAttributes[i++] = WGL_GREEN_BITS_ARB;
        iAttributes[i++] = format.greenBufferSize();
    }
    if (format.blueBufferSize() >= 0) {
        iAttributes[i++] = WGL_BLUE_BITS_ARB;
        iAttributes[i++] = format.blueBufferSize();
    }
    iAttributes[i++] = WGL_ALPHA_BITS_ARB;
    iAttributes[i++] = format.alphaBufferSize() >= 0 ? format.alphaBufferSize() : 8;
    if (additional.formatFlags & QWindowsGLAccumBuffer) {
        iAttributes[i++] = WGL_ACCUM_BITS_ARB;
        iAttributes[i++] = 16;
    }
    iAttributes[i++] = WGL_STENCIL_BITS_ARB;
    iAttributes[i++] = 8;
    if (additional.formatFlags & QWindowsGLOverlay) {
        iAttributes[i++] = WGL_NUMBER_OVERLAYS_ARB;
        iAttributes[i++] = 1;
    }
    const int samples = format.samples();
    const bool sampleBuffersRequested = samples > 1
            && testFlag(staticContext.extensions, QOpenGLStaticContext::SampleBuffers);
    int samplesValuePosition = 0;
    if (sampleBuffersRequested) {
        iAttributes[i++] = WGL_SAMPLE_BUFFERS_ARB;
        iAttributes[i++] = TRUE;
        iAttributes[i++] = WGL_SAMPLES_ARB;
        samplesValuePosition = i;
        iAttributes[i++] = format.samples();
    } else {
        iAttributes[i++] = WGL_SAMPLE_BUFFERS_ARB;
        iAttributes[i++] = FALSE;
    }
    // If sample buffer request cannot be satisfied, reduce request.
    int pixelFormat = 0;
    uint numFormats = 0;
    while (true) {
        const bool valid =
            staticContext.wglChoosePixelFormatARB(hdc, iAttributes, 0, 1,
                                               &pixelFormat, &numFormats)
                && numFormats >= 1;
        if (valid || !sampleBuffersRequested)
            break;
        if (iAttributes[samplesValuePosition] > 1) {
            iAttributes[samplesValuePosition] /= 2;
        } else {
            break;
        }
    }
    // Verify if format is acceptable. Note that the returned
    // formats have been observed to not contain PFD_SUPPORT_OPENGL, ignore.
    initPixelFormatDescriptor(obtainedPfd);
    DescribePixelFormat(hdc, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), obtainedPfd);
    if (!isAcceptableFormat(additional, *obtainedPfd, true)) {
        qCDebug(lcQpaGl) << __FUNCTION__ << " obtained px #" << pixelFormat
            << " not acceptable=" << *obtainedPfd;
        pixelFormat = 0;
    }

#ifndef QT_NO_DEBUG_OUTPUT
    if (lcQpaGl().isDebugEnabled()) {
        QString message;
        QDebug nsp(&message);
        nsp << __FUNCTION__;
        if (sampleBuffersRequested)
            nsp << " samples=" << iAttributes[samplesValuePosition];
        nsp << " Attributes: " << hex << showbase;
        for (int ii = 0; ii < i; ++ii)
            nsp << iAttributes[ii] << ',';
        nsp << noshowbase << dec << "\n    obtained px #" << pixelFormat
            << " of " << numFormats << "\n    " << *obtainedPfd;
        qCDebug(lcQpaGl) << message;
    } // Debug
#endif

    return pixelFormat;
}

static QSurfaceFormat
    qSurfaceFormatFromHDC(const QOpenGLStaticContext &staticContext,
                          HDC hdc, int pixelFormat,
                          QWindowsOpenGLAdditionalFormat *additionalIn = 0)
{
    enum { attribSize =40 };

    QSurfaceFormat result;
    result.setRenderableType(QSurfaceFormat::OpenGL);
    if (!staticContext.hasExtensions())
        return result;
    int iAttributes[attribSize];
    int iValues[attribSize];
    std::fill(iAttributes, iAttributes + attribSize, int(0));
    std::fill(iValues, iValues + attribSize, int(0));

    int i = 0;
    const bool hasSampleBuffers = testFlag(staticContext.extensions, QOpenGLStaticContext::SampleBuffers);

    iAttributes[i++] = WGL_DOUBLE_BUFFER_ARB; // 0
    iAttributes[i++] = WGL_DEPTH_BITS_ARB; // 1
    iAttributes[i++] = WGL_PIXEL_TYPE_ARB; // 2
    iAttributes[i++] = WGL_RED_BITS_ARB; // 3
    iAttributes[i++] = WGL_GREEN_BITS_ARB; // 4
    iAttributes[i++] = WGL_BLUE_BITS_ARB; // 5
    iAttributes[i++] = WGL_ALPHA_BITS_ARB; // 6
    iAttributes[i++] = WGL_ACCUM_BITS_ARB; // 7
    iAttributes[i++] = WGL_STENCIL_BITS_ARB; // 8
    iAttributes[i++] = WGL_STEREO_ARB; // 9
    iAttributes[i++] = WGL_ACCELERATION_ARB; // 10
    iAttributes[i++] = WGL_NUMBER_OVERLAYS_ARB; // 11
    if (hasSampleBuffers) {
        iAttributes[i++] = WGL_SAMPLE_BUFFERS_ARB; // 12
        iAttributes[i++] = WGL_SAMPLES_ARB; // 13
    }
    if (!staticContext.wglGetPixelFormatAttribIVARB(hdc, pixelFormat, 0, i,
                                        iAttributes, iValues)) {
        qErrnoWarning("%s: wglGetPixelFormatAttribIVARB() failed for basic parameters.", __FUNCTION__);
        return result;
    }
    result.setSwapBehavior(iValues[0] ? QSurfaceFormat::DoubleBuffer : QSurfaceFormat::SingleBuffer);
    result.setDepthBufferSize(iValues[1]);
    result.setRedBufferSize(iValues[3]);
    result.setGreenBufferSize(iValues[4]);
    result.setBlueBufferSize(iValues[5]);
    result.setAlphaBufferSize(iValues[6]);
    result.setStencilBufferSize(iValues[8]);
    if (iValues[9])
        result.setOption(QSurfaceFormat::StereoBuffers);

    if (hasSampleBuffers)
        result.setSamples(iValues[13]);
    if (additionalIn) {
        if (iValues[7])
            additionalIn->formatFlags |= QWindowsGLAccumBuffer;
        if (iValues[10] == WGL_FULL_ACCELERATION_ARB)
            additionalIn->formatFlags |= QWindowsGLDirectRendering;
        if (iValues[11])
            additionalIn->formatFlags |= QWindowsGLOverlay;
    }
    return result;
}

static HGLRC createContext(const QOpenGLStaticContext &staticContext,
                           HDC hdc,
                           const QSurfaceFormat &format,
                           const QWindowsOpenGLAdditionalFormat &,
                           HGLRC shared = 0)
{
    enum { attribSize = 11 };

    if (!staticContext.hasExtensions())
        return 0;
    int attributes[attribSize];
    int attribIndex = 0;
    std::fill(attributes, attributes + attribSize, int(0));

    // We limit the requested version by the version of the static context as
    // wglCreateContextAttribsARB fails and returns NULL if the requested context
    // version is not supported. This means that we will get the closest supported
    // context format that that which was requested and is supported by the driver
    const int requestedVersion = qMin((format.majorVersion() << 8) + format.minorVersion(),
                                      staticContext.defaultFormat.version);
    const int majorVersion = requestedVersion >> 8;
    const int minorVersion = requestedVersion & 0xFF;

    if (requestedVersion > 0x0101) {
        attributes[attribIndex++] = WGL_CONTEXT_MAJOR_VERSION_ARB;
        attributes[attribIndex++] = majorVersion;
        attributes[attribIndex++] = WGL_CONTEXT_MINOR_VERSION_ARB;
        attributes[attribIndex++] = minorVersion;
    }

    int flags = 0;
    if (format.testOption(QSurfaceFormat::DebugContext))
        flags |= WGL_CONTEXT_DEBUG_BIT_ARB;
    if (requestedVersion >= 0x0300) {
        if (!format.testOption(QSurfaceFormat::DeprecatedFunctions))
            flags |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
    }
    attributes[attribIndex++] = WGL_CONTEXT_FLAGS_ARB;
    attributes[attribIndex++] = flags;

    if (requestedVersion >= 0x0302) {
        switch (format.profile()) {
        case QSurfaceFormat::NoProfile:
            break;
        case QSurfaceFormat::CoreProfile:
            attributes[attribIndex++] = WGL_CONTEXT_PROFILE_MASK_ARB;
            attributes[attribIndex++] = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
            break;
        case QSurfaceFormat::CompatibilityProfile:
            attributes[attribIndex++] = WGL_CONTEXT_PROFILE_MASK_ARB;
            attributes[attribIndex++] = WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
            break;
        }
    }
    qCDebug(lcQpaGl) << __FUNCTION__ << "Creating context version"
        << majorVersion << '.' << minorVersion <<  attribIndex / 2 << "attributes";

    const HGLRC result =
        staticContext.wglCreateContextAttribsARB(hdc, shared, attributes);
    if (!result) {
        QString message;
        QDebug(&message).nospace() << __FUNCTION__ << ": wglCreateContextAttribsARB() failed (GL error code: 0x"
            << hex << glGetError() << dec << ") for format: " << format << ", shared context: " << shared;
        qErrnoWarning("%s", qPrintable(message));
    }
    return result;
}

} // namespace ARB

// Helpers for temporary contexts
static inline HWND createDummyGLWindow()
{
    return QWindowsContext::instance()->
        createDummyWindow(QStringLiteral("QtOpenGLDummyWindow"),
                          L"OpenGLDummyWindow", 0, WS_OVERLAPPED | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
}

// Create a dummy GL context (see QOpenGLTemporaryContext).
static inline HGLRC createDummyGLContext(HDC dc)
{
    if (!dc)
        return 0;
    PIXELFORMATDESCRIPTOR pixelFormDescriptor;
    initPixelFormatDescriptor(&pixelFormDescriptor);
    pixelFormDescriptor.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_GENERIC_FORMAT;
    pixelFormDescriptor.iPixelType = PFD_TYPE_RGBA;
    const int pixelFormat = ChoosePixelFormat(dc, &pixelFormDescriptor);
    if (!pixelFormat) {
        qErrnoWarning("%s: ChoosePixelFormat failed.", __FUNCTION__);
        return 0;
    }
    if (!SetPixelFormat(dc, pixelFormat, &pixelFormDescriptor)) {
        qErrnoWarning("%s: SetPixelFormat failed.", __FUNCTION__);
        return 0;
    }
    HGLRC rc = wglCreateContext(dc);
    if (!rc) {
        qErrnoWarning("%s: wglCreateContext failed.", __FUNCTION__);
        return 0;
    }
    return rc;
}

static inline QOpenGLContextData currentOpenGLContextData()
{
    QOpenGLContextData result;
    result.hdc = wglGetCurrentDC();
    result.renderingContext = wglGetCurrentContext();
    return result;
}

static inline QOpenGLContextData createDummyWindowOpenGLContextData()
{
    QOpenGLContextData result;
    result.hwnd = createDummyGLWindow();
    result.hdc = GetDC(result.hwnd);
    result.renderingContext = createDummyGLContext(result.hdc);
    return result;
}

/*!
    \class QOpenGLContextFormat
    \brief Format options that are related to the context (not pixelformats)

    Provides utility function to retrieve from currently active
    context and to apply to a QSurfaceFormat.

    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsOpenGLContextFormat::QWindowsOpenGLContextFormat() :
    profile(QSurfaceFormat::NoProfile),
    version(0),
    options(0)
{
}

QWindowsOpenGLContextFormat QWindowsOpenGLContextFormat::current()
{
    QWindowsOpenGLContextFormat result;
    const QByteArray version = QOpenGLStaticContext::getGlString(GL_VERSION);
    const int majorDot = version.indexOf('.');
    if (majorDot != -1) {
        int minorDot = version.indexOf('.', majorDot + 1);
        if (minorDot == -1)
            minorDot = version.size();
        result.version = (version.mid(0, majorDot).toInt() << 8)
            + version.mid(majorDot + 1, minorDot - majorDot - 1).toInt();
    } else {
        result.version = 0x0200;
    }
    result.profile = QSurfaceFormat::NoProfile;
    if (result.version < 0x0300) {
        result.options |= QSurfaceFormat::DeprecatedFunctions;
        return result;
    }
    // v3 onwards
    GLint value = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &value);
    if (!(value & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT))
        result.options |= QSurfaceFormat::DeprecatedFunctions;
    if (value & GL_CONTEXT_FLAG_DEBUG_BIT)
        result.options |= QSurfaceFormat::DebugContext;
    if (result.version < 0x0302)
        return result;
    // v3.2 onwards: Profiles
    value = 0;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &value);
    if (value & GL_CONTEXT_CORE_PROFILE_BIT)
        result.profile = QSurfaceFormat::CoreProfile;
    else if (value & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
        result.profile = QSurfaceFormat::CompatibilityProfile;
    return result;
}

void QWindowsOpenGLContextFormat::apply(QSurfaceFormat *format) const
{
    format->setMajorVersion(version >> 8);
    format->setMinorVersion(version & 0xFF);
    format->setProfile(profile);
    if (options & QSurfaceFormat::DebugContext)
        format->setOption(QSurfaceFormat::DebugContext);
    if (options & QSurfaceFormat::DeprecatedFunctions)
        format->setOption(QSurfaceFormat::DeprecatedFunctions);
}

QDebug operator<<(QDebug d, const QWindowsOpenGLContextFormat &f)
{
    d.nospace() << "ContextFormat: v" << (f.version >> 8) << '.'
                << (f.version & 0xFF) << " profile: " << f.profile
                << " options: " << f.options;
    return d;
}

/*!
    \class QOpenGLTemporaryContext
    \brief A temporary context that can be instantiated on the stack.

    Functions like wglGetProcAddress() or glGetString() only work if there
    is a current GL context.

    \internal
    \ingroup qt-lighthouse-win
*/

class QOpenGLTemporaryContext
{
    Q_DISABLE_COPY(QOpenGLTemporaryContext)
public:
    QOpenGLTemporaryContext();
    ~QOpenGLTemporaryContext();

private:
    const QOpenGLContextData m_previous;
    const QOpenGLContextData m_current;
};

QOpenGLTemporaryContext::QOpenGLTemporaryContext() :
    m_previous(currentOpenGLContextData()),
    m_current(createDummyWindowOpenGLContextData())
{
    wglMakeCurrent(m_current.hdc, m_current.renderingContext);
}

QOpenGLTemporaryContext::~QOpenGLTemporaryContext()
{
    wglMakeCurrent(m_previous.hdc, m_previous.renderingContext);
    ReleaseDC(m_current.hwnd, m_current.hdc);
    DestroyWindow(m_current.hwnd);
    wglDeleteContext(m_current.renderingContext);
}

/*!
    \class QWindowsOpenGLAdditionalFormat
    \brief Additional format information that is not in QSurfaceFormat
    \ingroup qt-lighthouse-win
*/

/*!
    \class QOpenGLStaticContext
    \brief Static Open GL context containing version information, extension function pointers, etc.

    Functions pending integration in the next version of OpenGL are post-fixed ARB.

    \note Initialization requires an active context (see create()).

    \sa QWindowsGLContext
    \internal
    \ingroup qt-lighthouse-win
*/

#define SAMPLE_BUFFER_EXTENSION "GL_ARB_multisample"

QOpenGLStaticContext::QOpenGLStaticContext() :
    vendor(QOpenGLStaticContext::getGlString(GL_VENDOR)),
    renderer(QOpenGLStaticContext::getGlString(GL_RENDERER)),
    extensionNames(QOpenGLStaticContext::getGlString(GL_EXTENSIONS)),
    extensions(0),
    defaultFormat(QWindowsOpenGLContextFormat::current()),
    wglGetPixelFormatAttribIVARB((WglGetPixelFormatAttribIVARB)wglGetProcAddress("wglGetPixelFormatAttribivARB")),
    wglChoosePixelFormatARB((WglChoosePixelFormatARB)wglGetProcAddress("wglChoosePixelFormatARB")),
    wglCreateContextAttribsARB((WglCreateContextAttribsARB)wglGetProcAddress("wglCreateContextAttribsARB")),
    wglSwapInternalExt((WglSwapInternalExt)wglGetProcAddress("wglSwapIntervalEXT")),
    wglGetSwapInternalExt((WglGetSwapInternalExt)wglGetProcAddress("wglGetSwapIntervalEXT"))
{
    if (extensionNames.startsWith(SAMPLE_BUFFER_EXTENSION " ")
            || extensionNames.indexOf(" " SAMPLE_BUFFER_EXTENSION " ") != -1)
        extensions |= SampleBuffers;
}

QByteArray QOpenGLStaticContext::getGlString(unsigned int which)
{
    if (const GLubyte *s = glGetString(which))
        return QByteArray((const char*)s);
    return QByteArray();
}

QOpenGLStaticContext *QOpenGLStaticContext::create()
{
    // We need a current context for wglGetProcAdress()/getGLString() to work.
    QScopedPointer<QOpenGLTemporaryContext> temporaryContext;
    if (!wglGetCurrentContext())
        temporaryContext.reset(new QOpenGLTemporaryContext);
    QOpenGLStaticContext *result = new QOpenGLStaticContext;
    qCDebug(lcQpaGl) << __FUNCTION__ << *result;
    return result;
}

QDebug operator<<(QDebug d, const QOpenGLStaticContext &s)
{
    QDebug nsp = d.nospace();
    nsp << "OpenGL: " << s.vendor << ',' << s.renderer << " default "
        <<  s.defaultFormat;
    if (s.extensions &  QOpenGLStaticContext::SampleBuffers)
        nsp << ",SampleBuffers";
    if (s.hasExtensions())
        nsp << ", Extension-API present";
    nsp  << "\nExtensions: " << (s.extensionNames.count(' ') + 1);
    if (QWindowsContext::verbose > 1)
        nsp <<  s.extensionNames;
    return d;
}

/*!
    \class QWindowsGLContext
    \brief Open GL context.

    An Open GL context for use with several windows.
    As opposed to other implementations, activating a GL context for
    a window requires a HDC allocated for it. The first time this
    HDC is created for the window, the pixel format must be applied,
    which will affect the window as well. The HDCs are stored in a list of
    QOpenGLContextData and are released in doneCurrent().

    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsGLContext::QWindowsGLContext(const QOpenGLStaticContextPtr &staticContext,
                                     QOpenGLContext *context) :
    m_staticContext(staticContext),
    m_context(context),
    m_renderingContext(0),
    m_pixelFormat(0),
    m_extensionsUsed(false),
    m_swapInterval(-1)
{
    QSurfaceFormat format = context->format();
    if (format.renderableType() == QSurfaceFormat::DefaultRenderableType)
        format.setRenderableType(QSurfaceFormat::OpenGL);
    if (format.renderableType() != QSurfaceFormat::OpenGL)
        return;

    // workaround for matrox driver:
    // make a cheap call to opengl to force loading of DLL
    static bool opengl32dll = false;
    if (!opengl32dll) {
        GLint params;
        glGetIntegerv(GL_DEPTH_BITS, &params);
        opengl32dll = true;
    }

    // SetPixelFormat (as of Windows 7) requires a real window.
    // Create a dummy one as we are not associated with a window yet.
    // Try to find a suitable pixel format using preferably ARB extensions
    // (default to GDI) and store that.
    HWND dummyWindow = 0;
    HDC hdc = 0;
    bool tryExtensions = false;
    int obtainedSwapInternal = -1;
    do {
        dummyWindow = createDummyGLWindow();
        if (!dummyWindow)
            break;
        hdc = GetDC(dummyWindow);
        if (!hdc)
            break;

        if (QWindowsContext::verbose > 1)
            describeFormats(hdc);
        // Preferably use direct rendering and ARB extensions (unless pixmap
        // or explicitly turned off on command line).
        const QWindowsOpenGLAdditionalFormat
            requestedAdditional(QWindowsGLDirectRendering);
        tryExtensions = m_staticContext->hasExtensions()
                && !testFlag(requestedAdditional.formatFlags, QWindowsGLRenderToPixmap)
                && !(QWindowsIntegration::instance()->options() & QWindowsIntegration::DisableArb);
        QWindowsOpenGLAdditionalFormat obtainedAdditional;
        if (tryExtensions) {
            m_pixelFormat =
                ARB::choosePixelFormat(hdc, *m_staticContext, format,
                                       requestedAdditional, &m_obtainedPixelFormatDescriptor);
            if (m_pixelFormat > 0) {
                m_obtainedFormat =
                    ARB::qSurfaceFormatFromHDC(*m_staticContext, hdc, m_pixelFormat,
                                                &obtainedAdditional);
                m_extensionsUsed = true;
            }
        } // tryExtensions
        if (!m_pixelFormat) { // Failed, try GDI
            m_pixelFormat = GDI::choosePixelFormat(hdc, format, requestedAdditional,
                                                   &m_obtainedPixelFormatDescriptor);
            if (m_pixelFormat)
                m_obtainedFormat =
                    GDI::qSurfaceFormatFromPixelFormat(m_obtainedPixelFormatDescriptor,
                                                       &obtainedAdditional);
        } // try GDI
        if (!m_pixelFormat) {
            qWarning("%s: Unable find a suitable pixel format.", __FUNCTION__);
            break;
        }
        if (!SetPixelFormat(hdc, m_pixelFormat, &m_obtainedPixelFormatDescriptor)) {
            qErrnoWarning("SetPixelFormat failed.");
            break;
        }
        // Create context with sharing, again preferably using ARB.
        HGLRC sharingRenderingContext = 0;
        if (const QPlatformOpenGLContext *sc = context->shareHandle())
            sharingRenderingContext = static_cast<const QWindowsGLContext *>(sc)->renderingContext();

        if (m_extensionsUsed)
            m_renderingContext =
                ARB::createContext(*m_staticContext, hdc,
                                   format,
                                   requestedAdditional,
                                   sharingRenderingContext);
        if (!m_renderingContext)
            m_renderingContext = GDI::createContext(hdc, sharingRenderingContext);

        if (!m_renderingContext) {
            qWarning("Unable to create a GL Context.");
            break;
        }

        // Query obtained parameters and apply swap interval.
        if (!wglMakeCurrent(hdc, m_renderingContext)) {
            qWarning("Failed to make context current.");
            break;
        }

        QWindowsOpenGLContextFormat::current().apply(&m_obtainedFormat);

        if (m_staticContext->wglGetSwapInternalExt)
            obtainedSwapInternal = m_staticContext->wglGetSwapInternalExt();

        wglMakeCurrent(0, 0);
    } while (false);
    if (hdc)
        ReleaseDC(dummyWindow, hdc);
    if (dummyWindow)
        DestroyWindow(dummyWindow);

    qCDebug(lcQpaGl) << __FUNCTION__ << this << (tryExtensions ? "ARB" : "GDI")
        << " requested: " << context->format()
        << "\n    obtained #" << m_pixelFormat << (m_extensionsUsed ? "ARB" : "GDI") << m_obtainedFormat
        << "\n    " << m_obtainedPixelFormatDescriptor << " swap interval: " << obtainedSwapInternal
        << "\n    default: " << m_staticContext->defaultFormat
        << "\n    HGLRC=" << m_renderingContext;
}

QWindowsGLContext::~QWindowsGLContext()
{
    if (m_renderingContext)
        wglDeleteContext(m_renderingContext);
    releaseDCs();
}

void QWindowsGLContext::releaseDCs()
{
    const QOpenGLContextData *end = m_windowContexts.end();
    for (const QOpenGLContextData *p = m_windowContexts.begin(); p < end; ++p)
        ReleaseDC(p->hwnd, p->hdc);
    m_windowContexts.resize(0);
}

static inline QWindowsWindow *glWindowOf(QPlatformSurface *s)
{
    return static_cast<QWindowsWindow *>(s);
}

static inline HWND handleOf(QPlatformSurface *s)
{
    return glWindowOf(s)->handle();
}

// Find a window in a context list.
static inline const QOpenGLContextData *
    findByHWND(const Array<QOpenGLContextData> &data, HWND hwnd)
{
    const QOpenGLContextData *end = data.end();
    for (const QOpenGLContextData *p = data.begin(); p < end; ++p)
        if (p->hwnd == hwnd)
            return p;
    return 0;
}

void QWindowsGLContext::swapBuffers(QPlatformSurface *surface)
{
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaGl) << __FUNCTION__ << surface;
    if (const QOpenGLContextData *contextData = findByHWND(m_windowContexts, handleOf(surface))) {
        SwapBuffers(contextData->hdc);
    } else {
        qWarning("%s: Cannot find window %p", __FUNCTION__, handleOf(surface));
    }
}

bool QWindowsGLContext::makeCurrent(QPlatformSurface *surface)
{
#ifdef DEBUG_GL
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaGl) << __FUNCTION__ << this << m_windowContexts.size() << "contexts";
#endif // DEBUG_GL

    Q_ASSERT(surface->surface()->supportsOpenGL());

    // Do we already have a DC entry for that window?
    QWindowsWindow *window = static_cast<QWindowsWindow *>(surface);
    const HWND hwnd = window->handle();
    if (const QOpenGLContextData *contextData = findByHWND(m_windowContexts, hwnd)) {
        // Repeated calls to wglMakeCurrent when vsync is enabled in the driver will
        // often result in 100% cpuload. This check is cheap and avoids the problem.
        // This is reproducable on NVidia cards and Intel onboard chips.
        if (wglGetCurrentContext() == contextData->renderingContext
                && wglGetCurrentDC() == contextData->hdc) {
            return true;
        }
        return wglMakeCurrent(contextData->hdc, contextData->renderingContext);
    }
    // Create a new entry.
    const QOpenGLContextData newContext(m_renderingContext, hwnd, GetDC(hwnd));
    if (!newContext.hdc)
        return false;
    // Initialize pixel format first time. This will apply to
    // the HWND as well and  must be done only once.
    if (!window->testFlag(QWindowsWindow::OpenGlPixelFormatInitialized)) {
        if (!SetPixelFormat(newContext.hdc, m_pixelFormat, &m_obtainedPixelFormatDescriptor)) {
            qErrnoWarning("%s: SetPixelFormat() failed", __FUNCTION__);
            ReleaseDC(newContext.hwnd, newContext.hdc);
            return false;
        }
        window->setFlag(QWindowsWindow::OpenGlPixelFormatInitialized);
        if (m_obtainedFormat.swapBehavior() == QSurfaceFormat::DoubleBuffer)
            window->setFlag(QWindowsWindow::OpenGLDoubleBuffered);
    }
    m_windowContexts.append(newContext);

    bool success = wglMakeCurrent(newContext.hdc, newContext.renderingContext);

    // Set the swap interval
    if (m_staticContext->wglSwapInternalExt) {
        const int interval = surface->format().swapInterval();
        if (interval >= 0 && m_swapInterval != interval) {
            m_swapInterval = interval;
            m_staticContext->wglSwapInternalExt(interval);
        }
    }

    return success;
}

void QWindowsGLContext::doneCurrent()
{
#ifdef DEBUG_GL
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaGl) << __FUNCTION__ << this << m_windowContexts.size() << "contexts";
#endif // DEBUG_GL
    wglMakeCurrent(0, 0);
    releaseDCs();
}

QWindowsGLContext::GL_Proc QWindowsGLContext::getProcAddress(const QByteArray &procName)
{
    // TODO: Will that work with the calling conventions?
    GL_Proc procAddress = reinterpret_cast<GL_Proc>(wglGetProcAddress(procName.constData()));
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaGl) << __FUNCTION__ <<  procName << wglGetCurrentContext() << "returns" << procAddress;
    if (!procAddress)
        qWarning("%s: Unable to resolve '%s'", __FUNCTION__, procName.constData());
    return procAddress;
}

QT_END_NAMESPACE
