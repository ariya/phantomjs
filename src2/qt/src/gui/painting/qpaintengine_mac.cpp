/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <qbitmap.h>
#include <qpaintdevice.h>
#include <private/qpaintengine_mac_p.h>
#include <qpainterpath.h>
#include <qpixmapcache.h>
#include <private/qpaintengine_raster_p.h>
#include <private/qprintengine_mac_p.h>
#include <qprinter.h>
#include <qstack.h>
#include <qtextcodec.h>
#include <qwidget.h>
#include <qvarlengtharray.h>
#include <qdebug.h>
#include <qcoreapplication.h>
#include <qmath.h>

#include <private/qfont_p.h>
#include <private/qfontengine_p.h>
#include <private/qfontengine_coretext_p.h>
#include <private/qfontengine_mac_p.h>
#include <private/qnumeric_p.h>
#include <private/qpainter_p.h>
#include <private/qpainterpath_p.h>
#include <private/qpixmap_mac_p.h>
#include <private/qt_mac_p.h>
#include <private/qtextengine_p.h>
#include <private/qwidget_p.h>
#include <private/qt_cocoa_helpers_mac_p.h>

#include <string.h>

QT_BEGIN_NAMESPACE

extern int qt_antialiasing_threshold; // QApplication.cpp

/*****************************************************************************
  External functions
 *****************************************************************************/
extern CGImageRef qt_mac_create_imagemask(const QPixmap &px, const QRectF &sr); //qpixmap_mac.cpp
extern QPoint qt_mac_posInWindow(const QWidget *w); //qwidget_mac.cpp
extern OSWindowRef qt_mac_window_for(const QWidget *); //qwidget_mac.cpp
extern CGContextRef qt_mac_cg_context(const QPaintDevice *); //qpaintdevice_mac.cpp
extern void qt_mac_dispose_rgn(RgnHandle r); //qregion_mac.cpp
extern QPixmap qt_pixmapForBrush(int, bool); //qbrush.cpp

void qt_mac_clip_cg(CGContextRef hd, const QRegion &rgn, CGAffineTransform *orig_xform);


//Implemented for qt_mac_p.h
QMacCGContext::QMacCGContext(QPainter *p)
{
    QPaintEngine *pe = p->paintEngine();
    if (pe->type() == QPaintEngine::MacPrinter)
        pe = static_cast<QMacPrintEngine*>(pe)->paintEngine();
    pe->syncState();
    context = 0;
    if(pe->type() == QPaintEngine::CoreGraphics)
        context = static_cast<QCoreGraphicsPaintEngine*>(pe)->handle();

    int devType = p->device()->devType();
    if (pe->type() == QPaintEngine::Raster
            && (devType == QInternal::Widget || devType == QInternal::Pixmap || devType == QInternal::Image)) {

        extern CGColorSpaceRef qt_mac_colorSpaceForDeviceType(const QPaintDevice *paintDevice);
        CGColorSpaceRef colorspace = qt_mac_colorSpaceForDeviceType(pe->paintDevice());
        uint flags = kCGImageAlphaPremultipliedFirst;
#ifdef kCGBitmapByteOrder32Host //only needed because CGImage.h added symbols in the minor version
        flags |= kCGBitmapByteOrder32Host;
#endif
        const QImage *image = (const QImage *) pe->paintDevice();

        context = CGBitmapContextCreate((void *) image->bits(), image->width(), image->height(),
                                        8, image->bytesPerLine(), colorspace, flags);

        CGContextTranslateCTM(context, 0, image->height());
        CGContextScaleCTM(context, 1, -1);

        if (devType == QInternal::Widget) {
            QRegion clip = p->paintEngine()->systemClip();
            QTransform native = p->deviceTransform();
            QTransform logical = p->combinedTransform();

            if (p->hasClipping()) {
                QRegion r = p->clipRegion();
                r.translate(native.dx(), native.dy());
                if (clip.isEmpty())
                    clip = r;
                else
                    clip &= r;
            }
            qt_mac_clip_cg(context, clip, 0);

            CGContextTranslateCTM(context, native.dx(), native.dy());
        }
    } else {
        CGContextRetain(context);
    }
}


/*****************************************************************************
  QCoreGraphicsPaintEngine utility functions
 *****************************************************************************/

//conversion
inline static float qt_mac_convert_color_to_cg(int c) { return ((float)c * 1000 / 255) / 1000; }
inline static int qt_mac_convert_color_from_cg(float c) { return qRound(c * 255); }
CGAffineTransform qt_mac_convert_transform_to_cg(const QTransform &t) {
    return CGAffineTransformMake(t.m11(), t.m12(), t.m21(), t.m22(), t.dx(),  t.dy());
}

CGColorSpaceRef qt_mac_colorSpaceForDeviceType(const QPaintDevice *paintDevice)
{
    bool isWidget = (paintDevice->devType() == QInternal::Widget);
    return QCoreGraphicsPaintEngine::macDisplayColorSpace(isWidget ? static_cast<const QWidget *>(paintDevice)
                                                                   : 0);
}

inline static QCFType<CGColorRef> cgColorForQColor(const QColor &col, QPaintDevice *pdev)
{
    CGFloat components[] = {
        qt_mac_convert_color_to_cg(col.red()),
        qt_mac_convert_color_to_cg(col.green()),
        qt_mac_convert_color_to_cg(col.blue()),
        qt_mac_convert_color_to_cg(col.alpha())
    };
    return CGColorCreate(qt_mac_colorSpaceForDeviceType(pdev), components);
}

// There's architectural problems with using native gradients
// on the Mac at the moment, so disable them.
// #define QT_MAC_USE_NATIVE_GRADIENTS

#ifdef QT_MAC_USE_NATIVE_GRADIENTS
static bool drawGradientNatively(const QGradient *gradient)
{
    return gradient->spread() == QGradient::PadSpread;
}

// gradiant callback
static void qt_mac_color_gradient_function(void *info, const CGFloat *in, CGFloat *out)
{
    QBrush *brush = static_cast<QBrush *>(info);
    Q_ASSERT(brush && brush->gradient());

    const QGradientStops stops = brush->gradient()->stops();
    const int n = stops.count();
    Q_ASSERT(n >= 1);
    const QGradientStop *begin = stops.constBegin();
    const QGradientStop *end = begin + n;

    qreal p = in[0];
    const QGradientStop *i = begin;
    while (i != end && i->first < p)
        ++i;

    QRgb c;
    if (i == begin) {
        c = begin->second.rgba();
    } else if (i == end) {
        c = (end - 1)->second.rgba();
    } else {
        const QGradientStop &s1 = *(i - 1);
        const QGradientStop &s2 = *i;
        qreal p1 = s1.first;
        qreal p2 = s2.first;
        QRgb c1 = s1.second.rgba();
        QRgb c2 = s2.second.rgba();
        int idist = 256 * (p - p1) / (p2 - p1);
        int dist = 256 - idist;
        c = qRgba(INTERPOLATE_PIXEL_256(qRed(c1), dist, qRed(c2), idist),
                  INTERPOLATE_PIXEL_256(qGreen(c1), dist, qGreen(c2), idist),
                  INTERPOLATE_PIXEL_256(qBlue(c1), dist, qBlue(c2), idist),
                  INTERPOLATE_PIXEL_256(qAlpha(c1), dist, qAlpha(c2), idist));
    }

    out[0] = qt_mac_convert_color_to_cg(qRed(c));
    out[1] = qt_mac_convert_color_to_cg(qGreen(c));
    out[2] = qt_mac_convert_color_to_cg(qBlue(c));
    out[3] = qt_mac_convert_color_to_cg(qAlpha(c));
}
#endif

//clipping handling
void QCoreGraphicsPaintEnginePrivate::resetClip()
{
    static bool inReset = false;
    if (inReset)
        return;
    inReset = true;

    CGAffineTransform old_xform = CGContextGetCTM(hd);

    //setup xforms
    CGContextConcatCTM(hd, CGAffineTransformInvert(old_xform));
    while (stackCount > 0) {
        restoreGraphicsState();
    }
    saveGraphicsState();
    inReset = false;
    //reset xforms
    CGContextConcatCTM(hd, CGAffineTransformInvert(CGContextGetCTM(hd)));
    CGContextConcatCTM(hd, old_xform);
}

static CGRect qt_mac_compose_rect(const QRectF &r, float off=0)
{
    return CGRectMake(r.x()+off, r.y()+off, r.width(), r.height());
}

static CGMutablePathRef qt_mac_compose_path(const QPainterPath &p, float off=0)
{
    CGMutablePathRef ret = CGPathCreateMutable();
    QPointF startPt;
    for (int i=0; i<p.elementCount(); ++i) {
        const QPainterPath::Element &elm = p.elementAt(i);
        switch (elm.type) {
            case QPainterPath::MoveToElement:
                if(i > 0
                        && p.elementAt(i - 1).x == startPt.x()
                        && p.elementAt(i - 1).y == startPt.y())
                    CGPathCloseSubpath(ret);
                startPt = QPointF(elm.x, elm.y);
                CGPathMoveToPoint(ret, 0, elm.x+off, elm.y+off);
                break;
            case QPainterPath::LineToElement:
                CGPathAddLineToPoint(ret, 0, elm.x+off, elm.y+off);
                break;
            case QPainterPath::CurveToElement:
                Q_ASSERT(p.elementAt(i+1).type == QPainterPath::CurveToDataElement);
                Q_ASSERT(p.elementAt(i+2).type == QPainterPath::CurveToDataElement);
                CGPathAddCurveToPoint(ret, 0,
                        elm.x+off, elm.y+off,
                        p.elementAt(i+1).x+off, p.elementAt(i+1).y+off,
                        p.elementAt(i+2).x+off, p.elementAt(i+2).y+off);
                i+=2;
                break;
            default:
                qFatal("QCoreGraphicsPaintEngine::drawPath(), unhandled type: %d", elm.type);
                break;
        }
    }
    if(!p.isEmpty()
            && p.elementAt(p.elementCount() - 1).x == startPt.x()
            && p.elementAt(p.elementCount() - 1).y == startPt.y())
        CGPathCloseSubpath(ret);
    return ret;
}

CGColorSpaceRef QCoreGraphicsPaintEngine::m_genericColorSpace = 0;
QHash<CGDirectDisplayID, CGColorSpaceRef> QCoreGraphicsPaintEngine::m_displayColorSpaceHash;
bool QCoreGraphicsPaintEngine::m_postRoutineRegistered = false;

CGColorSpaceRef QCoreGraphicsPaintEngine::macGenericColorSpace()
{
#if 0
    if (!m_genericColorSpace) {
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
        if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_4) {
            m_genericColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
        } else
#endif
        {
            m_genericColorSpace = CGColorSpaceCreateDeviceRGB();
        }
        if (!m_postRoutineRegistered) {
            m_postRoutineRegistered = true;
            qAddPostRoutine(QCoreGraphicsPaintEngine::cleanUpMacColorSpaces);
        }
    }
    return m_genericColorSpace;
#else
    // Just return the main display colorspace for the moment.
    return macDisplayColorSpace();
#endif
}

/*
    Ideally, we should pass the widget in here, and use CGGetDisplaysWithRect() etc.
    to support multiple displays correctly.
*/
CGColorSpaceRef QCoreGraphicsPaintEngine::macDisplayColorSpace(const QWidget *widget)
{
    CGColorSpaceRef colorSpace;

    CGDirectDisplayID displayID;
    CMProfileRef displayProfile = 0;
    if (widget == 0) {
        displayID = CGMainDisplayID();
    } else {
        const QRect &qrect = widget->window()->geometry();
        CGRect rect = CGRectMake(qrect.x(), qrect.y(), qrect.width(), qrect.height());
        CGDisplayCount throwAway;
        CGDisplayErr dErr = CGGetDisplaysWithRect(rect, 1, &displayID, &throwAway);
        if (dErr != kCGErrorSuccess)
            return macDisplayColorSpace(0); // fall back on main display
    }
    if ((colorSpace = m_displayColorSpaceHash.value(displayID)))
        return colorSpace;

    CMError err = CMGetProfileByAVID((CMDisplayIDType)displayID, &displayProfile);
    if (err == noErr) {
        colorSpace = CGColorSpaceCreateWithPlatformColorSpace(displayProfile);
    } else if (widget) {
        return macDisplayColorSpace(0); // fall back on main display
    }

    if (colorSpace == 0)
        colorSpace = CGColorSpaceCreateDeviceRGB();

    m_displayColorSpaceHash.insert(displayID, colorSpace);
    CMCloseProfile(displayProfile);
    if (!m_postRoutineRegistered) {
        m_postRoutineRegistered = true;
        qAddPostRoutine(QCoreGraphicsPaintEngine::cleanUpMacColorSpaces);
    }
    return colorSpace;
}

void QCoreGraphicsPaintEngine::cleanUpMacColorSpaces()
{
    if (m_genericColorSpace) {
        CFRelease(m_genericColorSpace);
        m_genericColorSpace = 0;
    }
    QHash<CGDirectDisplayID, CGColorSpaceRef>::const_iterator it = m_displayColorSpaceHash.constBegin();
    while (it != m_displayColorSpaceHash.constEnd()) {
        if (it.value())
            CFRelease(it.value());
        ++it;
    }
    m_displayColorSpaceHash.clear();
}

void qt_mac_clip_cg(CGContextRef hd, const QRegion &rgn, CGAffineTransform *orig_xform)
{
    CGAffineTransform old_xform = CGAffineTransformIdentity;
    if(orig_xform) { //setup xforms
        old_xform = CGContextGetCTM(hd);
        CGContextConcatCTM(hd, CGAffineTransformInvert(old_xform));
        CGContextConcatCTM(hd, *orig_xform);
    }

    //do the clipping
    CGContextBeginPath(hd);
    if(rgn.isEmpty()) {
        CGContextAddRect(hd, CGRectMake(0, 0, 0, 0));
    } else {
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
        if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_5) {
            QCFType<HIMutableShapeRef> shape = rgn.toHIMutableShape();
            Q_ASSERT(!HIShapeIsEmpty(shape));
            HIShapeReplacePathInCGContext(shape, hd);
        } else
#endif
        {
            QVector<QRect> rects = rgn.rects();
            const int count = rects.size();
            for(int i = 0; i < count; i++) {
                const QRect &r = rects[i];
                CGRect mac_r = CGRectMake(r.x(), r.y(), r.width(), r.height());
                CGContextAddRect(hd, mac_r);
            }
        }

    }
    CGContextClip(hd);

    if(orig_xform) {//reset xforms
        CGContextConcatCTM(hd, CGAffineTransformInvert(CGContextGetCTM(hd)));
        CGContextConcatCTM(hd, old_xform);
    }
}


//pattern handling (tiling)
#if 1
#  define QMACPATTERN_MASK_MULTIPLIER 32
#else
#  define QMACPATTERN_MASK_MULTIPLIER 1
#endif
class QMacPattern
{
public:
    QMacPattern() : as_mask(false), pdev(0), image(0) { data.bytes = 0; }
    ~QMacPattern() { CGImageRelease(image); }
    int width() {
        if(image)
            return CGImageGetWidth(image);
        if(data.bytes)
            return 8*QMACPATTERN_MASK_MULTIPLIER;
        return data.pixmap.width();
    }
    int height() {
        if(image)
            return CGImageGetHeight(image);
        if(data.bytes)
            return 8*QMACPATTERN_MASK_MULTIPLIER;
        return data.pixmap.height();
    }

    //input
    QColor foreground;
    bool as_mask;
    struct {
        QPixmap pixmap;
        const uchar *bytes;
    } data;
    QPaintDevice *pdev;
    //output
    CGImageRef image;
};
static void qt_mac_draw_pattern(void *info, CGContextRef c)
{
    QMacPattern *pat = (QMacPattern*)info;
    int w = 0, h = 0;
    bool isBitmap = (pat->data.pixmap.depth() == 1);
    if(!pat->image) { //lazy cache
        if(pat->as_mask) {
            Q_ASSERT(pat->data.bytes);
            w = h = 8;
#if (QMACPATTERN_MASK_MULTIPLIER == 1)
            CGDataProviderRef provider = CGDataProviderCreateWithData(0, pat->data.bytes, w*h, 0);
            pat->image = CGImageMaskCreate(w, h, 1, 1, 1, provider, 0, false);
            CGDataProviderRelease(provider);
#else
            const int numBytes = (w*h)/sizeof(uchar);
            uchar xor_bytes[numBytes];
            for(int i = 0; i < numBytes; ++i)
                xor_bytes[i] = pat->data.bytes[i] ^ 0xFF;
            CGDataProviderRef provider = CGDataProviderCreateWithData(0, xor_bytes, w*h, 0);
            CGImageRef swatch = CGImageMaskCreate(w, h, 1, 1, 1, provider, 0, false);
            CGDataProviderRelease(provider);

            const QColor c0(0, 0, 0, 0), c1(255, 255, 255, 255);
            QPixmap pm(w*QMACPATTERN_MASK_MULTIPLIER, h*QMACPATTERN_MASK_MULTIPLIER);
            pm.fill(c0);
            CGContextRef pm_ctx = qt_mac_cg_context(&pm);
            CGContextSetFillColorWithColor(c, cgColorForQColor(c1, pat->pdev));
            CGRect rect = CGRectMake(0, 0, w, h);
            for(int x = 0; x < QMACPATTERN_MASK_MULTIPLIER; ++x) {
                rect.origin.x = x * w;
                for(int y = 0; y < QMACPATTERN_MASK_MULTIPLIER; ++y) {
                    rect.origin.y = y * h;
                    qt_mac_drawCGImage(pm_ctx, &rect, swatch);
                }
            }
            pat->image = qt_mac_create_imagemask(pm, pm.rect());
            CGImageRelease(swatch);
            CGContextRelease(pm_ctx);
            w *= QMACPATTERN_MASK_MULTIPLIER;
            h *= QMACPATTERN_MASK_MULTIPLIER;
#endif
        } else {
            w = pat->data.pixmap.width();
            h = pat->data.pixmap.height();
            if (isBitmap)
                pat->image = qt_mac_create_imagemask(pat->data.pixmap, pat->data.pixmap.rect());
            else
                pat->image = (CGImageRef)pat->data.pixmap.macCGHandle();
        }
    } else {
        w = CGImageGetWidth(pat->image);
        h = CGImageGetHeight(pat->image);
    }

    //draw
    bool needRestore = false;
    if (CGImageIsMask(pat->image)) {
        CGContextSaveGState(c);
        CGContextSetFillColorWithColor(c, cgColorForQColor(pat->foreground, pat->pdev));
    }
    CGRect rect = CGRectMake(0, 0, w, h);
    qt_mac_drawCGImage(c, &rect, pat->image);
    if(needRestore)
        CGContextRestoreGState(c);
}
static void qt_mac_dispose_pattern(void *info)
{
    QMacPattern *pat = (QMacPattern*)info;
    delete pat;
}

/*****************************************************************************
  QCoreGraphicsPaintEngine member functions
 *****************************************************************************/

inline static QPaintEngine::PaintEngineFeatures qt_mac_cg_features()
{
    return QPaintEngine::PaintEngineFeatures(QPaintEngine::AllFeatures & ~QPaintEngine::PaintOutsidePaintEvent
                                              & ~QPaintEngine::PerspectiveTransform
                                              & ~QPaintEngine::ConicalGradientFill
                                              & ~QPaintEngine::LinearGradientFill
                                              & ~QPaintEngine::RadialGradientFill
                                              & ~QPaintEngine::BrushStroke);
}

QCoreGraphicsPaintEngine::QCoreGraphicsPaintEngine()
: QPaintEngine(*(new QCoreGraphicsPaintEnginePrivate), qt_mac_cg_features())
{
}

QCoreGraphicsPaintEngine::QCoreGraphicsPaintEngine(QPaintEnginePrivate &dptr)
: QPaintEngine(dptr, qt_mac_cg_features())
{
}

QCoreGraphicsPaintEngine::~QCoreGraphicsPaintEngine()
{
}

bool
QCoreGraphicsPaintEngine::begin(QPaintDevice *pdev)
{
    Q_D(QCoreGraphicsPaintEngine);
    if(isActive()) {                         // already active painting
        qWarning("QCoreGraphicsPaintEngine::begin: Painter already active");
        return false;
    }

    //initialization
    d->pdev = pdev;
    d->complexXForm = false;
    d->cosmeticPen = QCoreGraphicsPaintEnginePrivate::CosmeticSetPenWidth;
    d->cosmeticPenSize = 1;
    d->current.clipEnabled = false;
    d->pixelSize = QPoint(1,1);
    d->hd = qt_mac_cg_context(pdev);
    if(d->hd) {
        d->saveGraphicsState();
        d->orig_xform = CGContextGetCTM(d->hd);
        if (d->shading) {
            CGShadingRelease(d->shading);
            d->shading = 0;
        }
        d->setClip(0);  //clear the context's clipping
    }

    setActive(true);

    if(d->pdev->devType() == QInternal::Widget) {                    // device is a widget
        QWidget *w = (QWidget*)d->pdev;
        bool unclipped = w->testAttribute(Qt::WA_PaintUnclipped);

        if((w->windowType() == Qt::Desktop)) {
            if(!unclipped)
                qWarning("QCoreGraphicsPaintEngine::begin: Does not support clipped desktop on Mac OS X");
            // ## need to do [qt_mac_window_for(w) makeKeyAndOrderFront]; (need to rename the file)
        } else if(unclipped) {
            qWarning("QCoreGraphicsPaintEngine::begin: Does not support unclipped painting");
        }
    } else if(d->pdev->devType() == QInternal::Pixmap) {             // device is a pixmap
        QPixmap *pm = (QPixmap*)d->pdev;
        if(pm->isNull()) {
            qWarning("QCoreGraphicsPaintEngine::begin: Cannot paint null pixmap");
            end();
            return false;
        }
    }

    setDirty(QPaintEngine::DirtyPen);
    setDirty(QPaintEngine::DirtyBrush);
    setDirty(QPaintEngine::DirtyBackground);
    setDirty(QPaintEngine::DirtyHints);
    return true;
}

bool
QCoreGraphicsPaintEngine::end()
{
    Q_D(QCoreGraphicsPaintEngine);
    setActive(false);
    if(d->pdev->devType() == QInternal::Widget && static_cast<QWidget*>(d->pdev)->windowType() == Qt::Desktop) {
#ifndef QT_MAC_USE_COCOA
        HideWindow(qt_mac_window_for(static_cast<QWidget*>(d->pdev)));
#else
//        // ### need to do [qt_mac_window_for(static_cast<QWidget *>(d->pdev)) orderOut]; (need to rename)
#endif

	}
    if(d->shading) {
        CGShadingRelease(d->shading);
        d->shading = 0;
    }
    d->pdev = 0;
    if(d->hd) {
        d->restoreGraphicsState();
        CGContextSynchronize(d->hd);
        CGContextRelease(d->hd);
        d->hd = 0;
    }
    return true;
}

void
QCoreGraphicsPaintEngine::updateState(const QPaintEngineState &state)
{
    Q_D(QCoreGraphicsPaintEngine);
    QPaintEngine::DirtyFlags flags = state.state();

    if (flags & DirtyTransform)
        updateMatrix(state.transform());

    if (flags & DirtyClipEnabled) {
        if (state.isClipEnabled())
            updateClipPath(painter()->clipPath(), Qt::ReplaceClip);
        else
            updateClipPath(QPainterPath(), Qt::NoClip);
    }

    if (flags & DirtyClipPath) {
        updateClipPath(state.clipPath(), state.clipOperation());
    } else if (flags & DirtyClipRegion) {
        updateClipRegion(state.clipRegion(), state.clipOperation());
    }

    // If the clip has changed we need to update all other states
    // too, since they are included in the system context on OSX,
    // and changing the clip resets that context back to scratch.
    if (flags & (DirtyClipPath | DirtyClipRegion | DirtyClipEnabled))
        flags |= AllDirty;

    if (flags & DirtyPen)
        updatePen(state.pen());
    if (flags & (DirtyBrush|DirtyBrushOrigin))
        updateBrush(state.brush(), state.brushOrigin());
    if (flags & DirtyFont)
        updateFont(state.font());
    if (flags & DirtyOpacity)
        updateOpacity(state.opacity());
    if (flags & DirtyHints)
        updateRenderHints(state.renderHints());
    if (flags & DirtyCompositionMode)
        updateCompositionMode(state.compositionMode());

    if (flags & (DirtyPen | DirtyTransform)) {
        if (!d->current.pen.isCosmetic()) {
            d->cosmeticPen = QCoreGraphicsPaintEnginePrivate::CosmeticNone;
        } else if (d->current.transform.m11() < d->current.transform.m22()-1.0 ||
                  d->current.transform.m11() > d->current.transform.m22()+1.0) {
            d->cosmeticPen = QCoreGraphicsPaintEnginePrivate::CosmeticTransformPath;
            d->cosmeticPenSize = d->adjustPenWidth(d->current.pen.widthF());
            if (!d->cosmeticPenSize)
                d->cosmeticPenSize = 1.0;
        } else {
            d->cosmeticPen = QCoreGraphicsPaintEnginePrivate::CosmeticSetPenWidth;
            static const float sqrt2 = sqrt(2);
            qreal width = d->current.pen.widthF();
            if (!width)
                width = 1;
            d->cosmeticPenSize = sqrt(pow(d->pixelSize.y(), 2) + pow(d->pixelSize.x(), 2)) / sqrt2 * width;
        }
    }
}

void
QCoreGraphicsPaintEngine::updatePen(const QPen &pen)
{
    Q_D(QCoreGraphicsPaintEngine);
    Q_ASSERT(isActive());
    d->current.pen = pen;
    d->setStrokePen(pen);
}

void
QCoreGraphicsPaintEngine::updateBrush(const QBrush &brush, const QPointF &brushOrigin)
{
    Q_D(QCoreGraphicsPaintEngine);
    Q_ASSERT(isActive());
    d->current.brush = brush;

#ifdef QT_MAC_USE_NATIVE_GRADIENTS
    // Quartz supports only pad spread
    if (const QGradient *gradient = brush.gradient()) {
        if (drawGradientNatively(gradient)) {
            gccaps |= QPaintEngine::LinearGradientFill | QPaintEngine::RadialGradientFill;
        } else {
            gccaps &= ~(QPaintEngine::LinearGradientFill | QPaintEngine::RadialGradientFill);
        }
    }
#endif

    if (d->shading) {
        CGShadingRelease(d->shading);
        d->shading = 0;
    }
    d->setFillBrush(brushOrigin);
}

void
QCoreGraphicsPaintEngine::updateOpacity(qreal opacity)
{
    Q_D(QCoreGraphicsPaintEngine);
    CGContextSetAlpha(d->hd, opacity);
}

void
QCoreGraphicsPaintEngine::updateFont(const QFont &)
{
    Q_D(QCoreGraphicsPaintEngine);
    Q_ASSERT(isActive());
    updatePen(d->current.pen);
}

void
QCoreGraphicsPaintEngine::updateMatrix(const QTransform &transform)
{
    Q_D(QCoreGraphicsPaintEngine);
    Q_ASSERT(isActive());

    if (qt_is_nan(transform.m11()) || qt_is_nan(transform.m12()) || qt_is_nan(transform.m13())
	|| qt_is_nan(transform.m21()) || qt_is_nan(transform.m22()) || qt_is_nan(transform.m23())
	|| qt_is_nan(transform.m31()) || qt_is_nan(transform.m32()) || qt_is_nan(transform.m33()))
	return;

    d->current.transform = transform;
    d->setTransform(transform.isIdentity() ? 0 : &transform);
    d->complexXForm = (transform.m11() != 1 || transform.m22() != 1
            || transform.m12() != 0 || transform.m21() != 0);
    d->pixelSize = d->devicePixelSize(d->hd);
}

void
QCoreGraphicsPaintEngine::updateClipPath(const QPainterPath &p, Qt::ClipOperation op)
{
    Q_D(QCoreGraphicsPaintEngine);
    Q_ASSERT(isActive());
    if(op == Qt::NoClip) {
        if(d->current.clipEnabled) {
            d->current.clipEnabled = false;
            d->current.clip = QRegion();
            d->setClip(0);
        }
    } else {
        if(!d->current.clipEnabled)
            op = Qt::ReplaceClip;
        d->current.clipEnabled = true;
        QRegion clipRegion(p.toFillPolygon().toPolygon(), p.fillRule());
        if(op == Qt::ReplaceClip) {
            d->current.clip = clipRegion;
            d->setClip(0);
            if(p.isEmpty()) {
                CGRect rect = CGRectMake(0, 0, 0, 0);
                CGContextClipToRect(d->hd, rect);
            } else {
                CGMutablePathRef path = qt_mac_compose_path(p);
                CGContextBeginPath(d->hd);
                CGContextAddPath(d->hd, path);
                if(p.fillRule() == Qt::WindingFill)
                    CGContextClip(d->hd);
                else
                    CGContextEOClip(d->hd);
                CGPathRelease(path);
            }
        } else if(op == Qt::IntersectClip) {
            d->current.clip = d->current.clip.intersected(clipRegion);
            d->setClip(&d->current.clip);
        } else if(op == Qt::UniteClip) {
            d->current.clip = d->current.clip.united(clipRegion);
            d->setClip(&d->current.clip);
        }
    }
}

void
QCoreGraphicsPaintEngine::updateClipRegion(const QRegion &clipRegion, Qt::ClipOperation op)
{
    Q_D(QCoreGraphicsPaintEngine);
    Q_ASSERT(isActive());
    if(op == Qt::NoClip) {
        d->current.clipEnabled = false;
        d->current.clip = QRegion();
        d->setClip(0);
    } else {
        if(!d->current.clipEnabled)
            op = Qt::ReplaceClip;
        d->current.clipEnabled = true;
        if(op == Qt::IntersectClip)
            d->current.clip = d->current.clip.intersected(clipRegion);
        else if(op == Qt::ReplaceClip)
            d->current.clip = clipRegion;
        else if(op == Qt::UniteClip)
            d->current.clip = d->current.clip.united(clipRegion);
        d->setClip(&d->current.clip);
    }
}

void
QCoreGraphicsPaintEngine::drawPath(const QPainterPath &p)
{
    Q_D(QCoreGraphicsPaintEngine);
    Q_ASSERT(isActive());

    if (state->compositionMode() == QPainter::CompositionMode_Destination)
        return;

    CGMutablePathRef path = qt_mac_compose_path(p);
    uchar ops = QCoreGraphicsPaintEnginePrivate::CGStroke;
    if(p.fillRule() == Qt::WindingFill)
        ops |= QCoreGraphicsPaintEnginePrivate::CGFill;
    else
        ops |= QCoreGraphicsPaintEnginePrivate::CGEOFill;
    CGContextBeginPath(d->hd);
    d->drawPath(ops, path);
    CGPathRelease(path);
}

void
QCoreGraphicsPaintEngine::drawRects(const QRectF *rects, int rectCount)
{
    Q_D(QCoreGraphicsPaintEngine);
    Q_ASSERT(isActive());

    if (state->compositionMode() == QPainter::CompositionMode_Destination)
        return;

    for (int i=0; i<rectCount; ++i) {
        QRectF r = rects[i];

        CGMutablePathRef path = CGPathCreateMutable();
        CGPathAddRect(path, 0, qt_mac_compose_rect(r));
        d->drawPath(QCoreGraphicsPaintEnginePrivate::CGFill|QCoreGraphicsPaintEnginePrivate::CGStroke,
                path);
        CGPathRelease(path);
    }
}

void
QCoreGraphicsPaintEngine::drawPoints(const QPointF *points, int pointCount)
{
    Q_D(QCoreGraphicsPaintEngine);
    Q_ASSERT(isActive());

    if (state->compositionMode() == QPainter::CompositionMode_Destination)
        return;

    if (d->current.pen.capStyle() == Qt::FlatCap)
        CGContextSetLineCap(d->hd, kCGLineCapSquare);

    CGMutablePathRef path = CGPathCreateMutable();
    for(int i=0; i < pointCount; i++) {
        float x = points[i].x(), y = points[i].y();
        CGPathMoveToPoint(path, 0, x, y);
        CGPathAddLineToPoint(path, 0, x+0.001, y);
    }

    bool doRestore = false;
    if(d->cosmeticPen == QCoreGraphicsPaintEnginePrivate::CosmeticNone && !(state->renderHints() & QPainter::Antialiasing)) {
        //we don't want adjusted pens for point rendering
        doRestore = true;
        d->saveGraphicsState();
        CGContextSetLineWidth(d->hd, d->current.pen.widthF());
    }
    d->drawPath(QCoreGraphicsPaintEnginePrivate::CGStroke, path);
    if (doRestore)
        d->restoreGraphicsState();
    CGPathRelease(path);
    if (d->current.pen.capStyle() == Qt::FlatCap)
        CGContextSetLineCap(d->hd, kCGLineCapButt);
}

void
QCoreGraphicsPaintEngine::drawEllipse(const QRectF &r)
{
    Q_D(QCoreGraphicsPaintEngine);
    Q_ASSERT(isActive());

    if (state->compositionMode() == QPainter::CompositionMode_Destination)
        return;

    CGMutablePathRef path = CGPathCreateMutable();
    CGAffineTransform transform = CGAffineTransformMakeScale(r.width() / r.height(), 1);
    CGPathAddArc(path, &transform,(r.x() + (r.width() / 2)) / (r.width() / r.height()),
            r.y() + (r.height() / 2), r.height() / 2, 0, (2 * M_PI), false);
    d->drawPath(QCoreGraphicsPaintEnginePrivate::CGFill | QCoreGraphicsPaintEnginePrivate::CGStroke,
            path);
    CGPathRelease(path);
}

void
QCoreGraphicsPaintEngine::drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode)
{
    Q_D(QCoreGraphicsPaintEngine);
    Q_ASSERT(isActive());

    if (state->compositionMode() == QPainter::CompositionMode_Destination)
        return;

    CGMutablePathRef path = CGPathCreateMutable();
    CGPathMoveToPoint(path, 0, points[0].x(), points[0].y());
    for(int x = 1; x < pointCount; ++x)
        CGPathAddLineToPoint(path, 0, points[x].x(), points[x].y());
    if(mode != PolylineMode && points[0] != points[pointCount-1])
        CGPathAddLineToPoint(path, 0, points[0].x(), points[0].y());
    uint op = QCoreGraphicsPaintEnginePrivate::CGStroke;
    if (mode != PolylineMode)
        op |= mode == OddEvenMode ? QCoreGraphicsPaintEnginePrivate::CGEOFill
            : QCoreGraphicsPaintEnginePrivate::CGFill;
    d->drawPath(op, path);
    CGPathRelease(path);
}

void
QCoreGraphicsPaintEngine::drawLines(const QLineF *lines, int lineCount)
{
    Q_D(QCoreGraphicsPaintEngine);
    Q_ASSERT(isActive());

    if (state->compositionMode() == QPainter::CompositionMode_Destination)
        return;

    CGMutablePathRef path = CGPathCreateMutable();
    for(int i = 0; i < lineCount; i++) {
        const QPointF start = lines[i].p1(), end = lines[i].p2();
        CGPathMoveToPoint(path, 0, start.x(), start.y());
        CGPathAddLineToPoint(path, 0, end.x(), end.y());
    }
    d->drawPath(QCoreGraphicsPaintEnginePrivate::CGStroke, path);
    CGPathRelease(path);
}

void QCoreGraphicsPaintEngine::drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr)
{
    Q_D(QCoreGraphicsPaintEngine);
    Q_ASSERT(isActive());

    if (state->compositionMode() == QPainter::CompositionMode_Destination)
        return;

    if(pm.isNull())
        return;

    bool differentSize = (QRectF(0, 0, pm.width(), pm.height()) != sr), doRestore = false;
    CGRect rect = CGRectMake(r.x(), r.y(), r.width(), r.height());
    QCFType<CGImageRef> image;
    bool isBitmap = (pm.depth() == 1);
    if (isBitmap) {
        doRestore = true;
        d->saveGraphicsState();

        const QColor &col = d->current.pen.color();
        CGContextSetFillColorWithColor(d->hd, cgColorForQColor(col, d->pdev));
        image = qt_mac_create_imagemask(pm, sr);
    } else if (differentSize) {
        QCFType<CGImageRef> img = pm.toMacCGImageRef();
        image = CGImageCreateWithImageInRect(img, CGRectMake(qRound(sr.x()), qRound(sr.y()), qRound(sr.width()), qRound(sr.height())));
    } else {
        image = (CGImageRef)pm.macCGHandle();
    }
    qt_mac_drawCGImage(d->hd, &rect, image);
    if (doRestore)
        d->restoreGraphicsState();
}

static void drawImageReleaseData (void *info, const void *, size_t)
{
    delete static_cast<QImage *>(info);
}

CGImageRef qt_mac_createCGImageFromQImage(const QImage &img, const QImage **imagePtr = 0)
{
    QImage *image;
    if (img.depth() != 32)
        image = new QImage(img.convertToFormat(QImage::Format_ARGB32_Premultiplied));
    else
        image = new QImage(img);

    uint cgflags = kCGImageAlphaNone;
    switch (image->format()) {
    case QImage::Format_ARGB32_Premultiplied:
        cgflags = kCGImageAlphaPremultipliedFirst;
        break;
    case QImage::Format_ARGB32:
        cgflags = kCGImageAlphaFirst;
        break;
    case QImage::Format_RGB32:
        cgflags = kCGImageAlphaNoneSkipFirst;
    default:
        break;
    }
#if defined(kCGBitmapByteOrder32Host) //only needed because CGImage.h added symbols in the minor version
    cgflags |= kCGBitmapByteOrder32Host;
#endif
    QCFType<CGDataProviderRef> dataProvider = CGDataProviderCreateWithData(image,
                                                          static_cast<const QImage *>(image)->bits(),
                                                          image->byteCount(),
                                                          drawImageReleaseData);
    if (imagePtr)
        *imagePtr = image;
    return CGImageCreate(image->width(), image->height(), 8, 32,
                                        image->bytesPerLine(),
                                        QCoreGraphicsPaintEngine::macGenericColorSpace(),
                                        cgflags, dataProvider, 0, false, kCGRenderingIntentDefault);

}

void QCoreGraphicsPaintEngine::drawImage(const QRectF &r, const QImage &img, const QRectF &sr,
                                         Qt::ImageConversionFlags flags)
{
    Q_D(QCoreGraphicsPaintEngine);
    Q_UNUSED(flags);
    Q_ASSERT(isActive());

    if (img.isNull() || state->compositionMode() == QPainter::CompositionMode_Destination)
        return;

    const QImage *image;
    QCFType<CGImageRef> cgimage = qt_mac_createCGImageFromQImage(img, &image);
    CGRect rect = CGRectMake(r.x(), r.y(), r.width(), r.height());
    if (QRectF(0, 0, img.width(), img.height()) != sr)
        cgimage = CGImageCreateWithImageInRect(cgimage, CGRectMake(sr.x(), sr.y(),
                                               sr.width(), sr.height()));
    qt_mac_drawCGImage(d->hd, &rect, cgimage);
}

void QCoreGraphicsPaintEngine::initialize()
{
}

void QCoreGraphicsPaintEngine::cleanup()
{
}

CGContextRef
QCoreGraphicsPaintEngine::handle() const
{
    return d_func()->hd;
}

void
QCoreGraphicsPaintEngine::drawTiledPixmap(const QRectF &r, const QPixmap &pixmap,
        const QPointF &p)
{
    Q_D(QCoreGraphicsPaintEngine);
    Q_ASSERT(isActive());

    if (state->compositionMode() == QPainter::CompositionMode_Destination)
        return;

    //save the old state
    d->saveGraphicsState();

    //setup the pattern
    QMacPattern *qpattern = new QMacPattern;
    qpattern->data.pixmap = pixmap;
    qpattern->foreground = d->current.pen.color();
    qpattern->pdev = d->pdev;
    CGPatternCallbacks callbks;
    callbks.version = 0;
    callbks.drawPattern = qt_mac_draw_pattern;
    callbks.releaseInfo = qt_mac_dispose_pattern;
    const int width = qpattern->width(), height = qpattern->height();
    CGAffineTransform trans = CGContextGetCTM(d->hd);
    CGPatternRef pat = CGPatternCreate(qpattern, CGRectMake(0, 0, width, height),
            trans, width, height,
            kCGPatternTilingNoDistortion, true, &callbks);
    CGColorSpaceRef cs = CGColorSpaceCreatePattern(0);
    CGContextSetFillColorSpace(d->hd, cs);
    CGFloat component = 1.0; //just one
    CGContextSetFillPattern(d->hd, pat, &component);
    CGSize phase = CGSizeApplyAffineTransform(CGSizeMake(-(p.x()-r.x()), -(p.y()-r.y())), trans);
    CGContextSetPatternPhase(d->hd, phase);

    //fill the rectangle
    CGRect mac_rect = CGRectMake(r.x(), r.y(), r.width(), r.height());
    CGContextFillRect(d->hd, mac_rect);

    //restore the state
    d->restoreGraphicsState();
    //cleanup
    CGColorSpaceRelease(cs);
    CGPatternRelease(pat);
}

void QCoreGraphicsPaintEngine::drawTextItem(const QPointF &pos, const QTextItem &item)
{
    Q_D(QCoreGraphicsPaintEngine);
    if (d->current.transform.type() == QTransform::TxProject
#ifndef QMAC_NATIVE_GRADIENTS
        || painter()->pen().brush().gradient()  //Just let the base engine "emulate" the gradient
#endif
        ) {
        QPaintEngine::drawTextItem(pos, item);
        return;
    }

    if (state->compositionMode() == QPainter::CompositionMode_Destination)
        return;

    const QTextItemInt &ti = static_cast<const QTextItemInt &>(item);

    QPen oldPen = painter()->pen();
    QBrush oldBrush = painter()->brush();
    QPointF oldBrushOrigin = painter()->brushOrigin();
    updatePen(Qt::NoPen);
    updateBrush(oldPen.brush(), QPointF(0, 0));

    Q_ASSERT(type() == QPaintEngine::CoreGraphics);

    QFontEngine *fe = ti.fontEngine;

    const bool textAA = state->renderHints() & QPainter::TextAntialiasing && fe->fontDef.pointSize > qt_antialiasing_threshold && !(fe->fontDef.styleStrategy & QFont::NoAntialias);
    const bool lineAA = state->renderHints() & QPainter::Antialiasing;
    if(textAA != lineAA)
        CGContextSetShouldAntialias(d->hd, textAA);

    if (ti.glyphs.numGlyphs) {
        switch (fe->type()) {
        case QFontEngine::Mac:
#ifdef QT_MAC_USE_COCOA
            static_cast<QCoreTextFontEngine *>(fe)->draw(d->hd, pos.x(), pos.y(), ti, paintDevice()->height());
#else
            static_cast<QFontEngineMac *>(fe)->draw(d->hd, pos.x(), pos.y(), ti, paintDevice()->height());
#endif
            break;
        case QFontEngine::Box:
            d->drawBoxTextItem(pos, ti);
            break;
        default:
            break;
        }
    }

    if(textAA != lineAA)
        CGContextSetShouldAntialias(d->hd, !textAA);

    updatePen(oldPen);
    updateBrush(oldBrush, oldBrushOrigin);
}

QPainter::RenderHints
QCoreGraphicsPaintEngine::supportedRenderHints() const
{
    return QPainter::RenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
}
enum CGCompositeMode {
        kCGCompositeModeClear            = 0,
        kCGCompositeModeCopy             = 1,
        kCGCompositeModeSourceOver       = 2,
        kCGCompositeModeSourceIn         = 3,
        kCGCompositeModeSourceOut        = 4,
        kCGCompositeModeSourceAtop       = 5,
        kCGCompositeModeDestinationOver  = 6,
        kCGCompositeModeDestinationIn    = 7,
        kCGCompositeModeDestinationOut   = 8,
        kCGCompositeModeDestinationAtop  = 9,
        kCGCompositeModeXOR              = 10,
        kCGCompositeModePlusDarker       = 11, // (max (0, (1-d) + (1-s)))
        kCGCompositeModePlusLighter      = 12, // (min (1, s + d))
    };
extern "C" {
    extern void CGContextSetCompositeOperation(CGContextRef, int);
} // private function, but is in all versions of OS X.
void
QCoreGraphicsPaintEngine::updateCompositionMode(QPainter::CompositionMode mode)
{
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_5) {
        int cg_mode = kCGBlendModeNormal;
        switch(mode) {
        case QPainter::CompositionMode_Multiply:
            cg_mode = kCGBlendModeMultiply;
            break;
        case QPainter::CompositionMode_Screen:
            cg_mode = kCGBlendModeScreen;
            break;
        case QPainter::CompositionMode_Overlay:
            cg_mode = kCGBlendModeOverlay;
            break;
        case QPainter::CompositionMode_Darken:
            cg_mode = kCGBlendModeDarken;
            break;
        case QPainter::CompositionMode_Lighten:
            cg_mode = kCGBlendModeLighten;
            break;
        case QPainter::CompositionMode_ColorDodge:
            cg_mode = kCGBlendModeColorDodge;
            break;
        case QPainter::CompositionMode_ColorBurn:
            cg_mode = kCGBlendModeColorBurn;
            break;
        case QPainter::CompositionMode_HardLight:
            cg_mode = kCGBlendModeHardLight;
            break;
        case QPainter::CompositionMode_SoftLight:
            cg_mode = kCGBlendModeSoftLight;
            break;
        case QPainter::CompositionMode_Difference:
            cg_mode = kCGBlendModeDifference;
            break;
        case QPainter::CompositionMode_Exclusion:
            cg_mode = kCGBlendModeExclusion;
            break;
        case QPainter::CompositionMode_Plus:
            cg_mode = kCGBlendModePlusLighter;
            break;
        case QPainter::CompositionMode_SourceOver:
            cg_mode = kCGBlendModeNormal;
            break;
        case QPainter::CompositionMode_DestinationOver:
            cg_mode = kCGBlendModeDestinationOver;
            break;
        case QPainter::CompositionMode_Clear:
            cg_mode = kCGBlendModeClear;
            break;
        case QPainter::CompositionMode_Source:
            cg_mode = kCGBlendModeCopy;
            break;
        case QPainter::CompositionMode_Destination:
            cg_mode = -1;
            break;
        case QPainter::CompositionMode_SourceIn:
            cg_mode = kCGBlendModeSourceIn;
            break;
        case QPainter::CompositionMode_DestinationIn:
            cg_mode = kCGCompositeModeDestinationIn;
            break;
        case QPainter::CompositionMode_SourceOut:
            cg_mode = kCGBlendModeSourceOut;
            break;
        case QPainter::CompositionMode_DestinationOut:
            cg_mode = kCGBlendModeDestinationOver;
            break;
        case QPainter::CompositionMode_SourceAtop:
            cg_mode = kCGBlendModeSourceAtop;
            break;
        case QPainter::CompositionMode_DestinationAtop:
            cg_mode = kCGBlendModeDestinationAtop;
            break;
        case QPainter::CompositionMode_Xor:
            cg_mode = kCGBlendModeXOR;
            break;
        default:
            break;
        }
        if (cg_mode > -1) {
            CGContextSetBlendMode(d_func()->hd, CGBlendMode(cg_mode));
        }
    } else
#endif
    // The standard porter duff ops.
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_3
            && mode <= QPainter::CompositionMode_Xor) {
        int cg_mode = kCGCompositeModeCopy;
        switch (mode) {
        case QPainter::CompositionMode_SourceOver:
            cg_mode = kCGCompositeModeSourceOver;
            break;
        case QPainter::CompositionMode_DestinationOver:
            cg_mode = kCGCompositeModeDestinationOver;
            break;
        case QPainter::CompositionMode_Clear:
            cg_mode = kCGCompositeModeClear;
            break;
        default:
            qWarning("QCoreGraphicsPaintEngine: Unhandled composition mode %d", (int)mode);
            break;
        case QPainter::CompositionMode_Source:
            cg_mode = kCGCompositeModeCopy;
            break;
        case QPainter::CompositionMode_Destination:
            cg_mode = CGCompositeMode(-1);
            break;
        case QPainter::CompositionMode_SourceIn:
            cg_mode = kCGCompositeModeSourceIn;
            break;
        case QPainter::CompositionMode_DestinationIn:
            cg_mode = kCGCompositeModeDestinationIn;
            break;
        case QPainter::CompositionMode_SourceOut:
            cg_mode = kCGCompositeModeSourceOut;
            break;
        case QPainter::CompositionMode_DestinationOut:
            cg_mode = kCGCompositeModeDestinationOut;
            break;
        case QPainter::CompositionMode_SourceAtop:
            cg_mode = kCGCompositeModeSourceAtop;
            break;
        case QPainter::CompositionMode_DestinationAtop:
            cg_mode = kCGCompositeModeDestinationAtop;
            break;
        case QPainter::CompositionMode_Xor:
            cg_mode = kCGCompositeModeXOR;
            break;
        }
        if (cg_mode > -1)
            CGContextSetCompositeOperation(d_func()->hd, CGCompositeMode(cg_mode));
    } else {
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4)
        bool needPrivateAPI = false;
        if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_4) {
            int cg_mode = kCGBlendModeNormal;
            switch (mode) {
            case QPainter::CompositionMode_Multiply:
                cg_mode = kCGBlendModeMultiply;
                break;
            case QPainter::CompositionMode_Screen:
                cg_mode = kCGBlendModeScreen;
                break;
            case QPainter::CompositionMode_Overlay:
                cg_mode = kCGBlendModeOverlay;
                break;
            case QPainter::CompositionMode_Darken:
                cg_mode = kCGBlendModeDarken;
                break;
            case QPainter::CompositionMode_Lighten:
                cg_mode = kCGBlendModeLighten;
                break;
            case QPainter::CompositionMode_ColorDodge:
                cg_mode = kCGBlendModeColorDodge;
                break;
            case QPainter::CompositionMode_ColorBurn:
                cg_mode = kCGBlendModeColorBurn;
                break;
            case QPainter::CompositionMode_HardLight:
                cg_mode = kCGBlendModeHardLight;
                break;
            case QPainter::CompositionMode_SoftLight:
                cg_mode = kCGBlendModeSoftLight;
                break;
            case QPainter::CompositionMode_Difference:
                cg_mode = kCGBlendModeDifference;
                break;
            case QPainter::CompositionMode_Exclusion:
                cg_mode = kCGBlendModeExclusion;
                break;
            case QPainter::CompositionMode_Plus:
                needPrivateAPI = true;
                cg_mode = kCGCompositeModePlusLighter;
                break;
            default:
                break;
            }
            if (!needPrivateAPI)
                CGContextSetBlendMode(d_func()->hd, CGBlendMode(cg_mode));
            else
                CGContextSetCompositeOperation(d_func()->hd, CGCompositeMode(cg_mode));
        }
#endif
    }
}

void
QCoreGraphicsPaintEngine::updateRenderHints(QPainter::RenderHints hints)
{
    Q_D(QCoreGraphicsPaintEngine);
    CGContextSetShouldAntialias(d->hd, hints & QPainter::Antialiasing);
    static const CGFloat ScaleFactor = qt_mac_get_scalefactor();
    if (ScaleFactor > 1.) {
        CGContextSetInterpolationQuality(d->hd, kCGInterpolationHigh);
    } else {
        CGContextSetInterpolationQuality(d->hd, (hints & QPainter::SmoothPixmapTransform) ?
                                         kCGInterpolationHigh : kCGInterpolationNone);
    }
    bool textAntialiasing = (hints & QPainter::TextAntialiasing) == QPainter::TextAntialiasing;
    if (!textAntialiasing || d->disabledSmoothFonts) {
        d->disabledSmoothFonts = !textAntialiasing;
        CGContextSetShouldSmoothFonts(d->hd, textAntialiasing);
    }
}

/*
    Returns the size of one device pixel in user-space coordinates.
*/
QPointF QCoreGraphicsPaintEnginePrivate::devicePixelSize(CGContextRef)
{
    QPointF p1 = current.transform.inverted().map(QPointF(0, 0));
    QPointF p2 = current.transform.inverted().map(QPointF(1, 1));
    return QPointF(qAbs(p2.x() - p1.x()), qAbs(p2.y() - p1.y()));
}

/*
    Adjusts the pen width so we get correct line widths in the
    non-transformed, aliased case.
*/
float QCoreGraphicsPaintEnginePrivate::adjustPenWidth(float penWidth)
{
    Q_Q(QCoreGraphicsPaintEngine);
    float ret = penWidth;
    if (!complexXForm && !(q->state->renderHints() & QPainter::Antialiasing)) {
        if (penWidth < 2)
            ret = 1;
        else if (penWidth < 3)
            ret = 1.5;
        else
            ret = penWidth -1;
    }
    return ret;
}

void
QCoreGraphicsPaintEnginePrivate::setStrokePen(const QPen &pen)
{
    //pencap
    CGLineCap cglinecap = kCGLineCapButt;
    if(pen.capStyle() == Qt::SquareCap)
        cglinecap = kCGLineCapSquare;
    else if(pen.capStyle() == Qt::RoundCap)
        cglinecap = kCGLineCapRound;
    CGContextSetLineCap(hd, cglinecap);
    CGContextSetLineWidth(hd, adjustPenWidth(pen.widthF()));

    //join
    CGLineJoin cglinejoin = kCGLineJoinMiter;
    if(pen.joinStyle() == Qt::BevelJoin)
        cglinejoin = kCGLineJoinBevel;
    else if(pen.joinStyle() == Qt::RoundJoin)
        cglinejoin = kCGLineJoinRound;
    CGContextSetLineJoin(hd, cglinejoin);
//    CGContextSetMiterLimit(hd, pen.miterLimit());

    //pen style
    QVector<CGFloat> linedashes;
    if(pen.style() == Qt::CustomDashLine) {
        QVector<qreal> customs = pen.dashPattern();
        for(int i = 0; i < customs.size(); ++i)
            linedashes.append(customs.at(i));
    } else if(pen.style() == Qt::DashLine) {
        linedashes.append(4);
        linedashes.append(2);
    } else if(pen.style() == Qt::DotLine) {
        linedashes.append(1);
        linedashes.append(2);
    } else if(pen.style() == Qt::DashDotLine) {
        linedashes.append(4);
        linedashes.append(2);
        linedashes.append(1);
        linedashes.append(2);
    } else if(pen.style() == Qt::DashDotDotLine) {
        linedashes.append(4);
        linedashes.append(2);
        linedashes.append(1);
        linedashes.append(2);
        linedashes.append(1);
        linedashes.append(2);
    }
    const CGFloat cglinewidth = pen.widthF() <= 0.0f ? 1.0f : float(pen.widthF());
    for(int i = 0; i < linedashes.size(); ++i) {
        linedashes[i] *= cglinewidth;
        if(cglinewidth < 3 && (cglinecap == kCGLineCapSquare || cglinecap == kCGLineCapRound)) {
            if((i%2))
                linedashes[i] += cglinewidth/2;
            else
                linedashes[i] -= cglinewidth/2;
        }
    }
    CGContextSetLineDash(hd, pen.dashOffset() * cglinewidth, linedashes.data(), linedashes.size());

    // color
    CGContextSetStrokeColorWithColor(hd, cgColorForQColor(pen.color(), pdev));
}

// Add our own patterns here to deal with the fact that the coordinate system
// is flipped vertically with Quartz2D.
static const uchar *qt_mac_patternForBrush(int brushStyle)
{
    Q_ASSERT(brushStyle > Qt::SolidPattern && brushStyle < Qt::LinearGradientPattern);
    static const uchar dense1_pat[] = { 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x44, 0x00 };
    static const uchar dense2_pat[] = { 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00, 0x88 };
    static const uchar dense3_pat[] = { 0x11, 0xaa, 0x44, 0xaa, 0x11, 0xaa, 0x44, 0xaa };
    static const uchar dense4_pat[] = { 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55 };
    static const uchar dense5_pat[] = { 0xee, 0x55, 0xbb, 0x55, 0xee, 0x55, 0xbb, 0x55 };
    static const uchar dense6_pat[] = { 0xff, 0xdd, 0xff, 0x77, 0xff, 0xdd, 0xff, 0x77 };
    static const uchar dense7_pat[] = { 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb, 0xff };
    static const uchar hor_pat[]    = { 0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff };
    static const uchar ver_pat[]    = { 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef };
    static const uchar cross_pat[]  = { 0xef, 0xef, 0xef, 0xef, 0x00, 0xef, 0xef, 0xef };
    static const uchar fdiag_pat[]  = { 0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe };
    static const uchar bdiag_pat[]  = { 0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f };
    static const uchar dcross_pat[] = { 0x7e, 0xbd, 0xdb, 0xe7, 0xe7, 0xdb, 0xbd, 0x7e };
    static const uchar *const pat_tbl[] = {
        dense1_pat, dense2_pat, dense3_pat, dense4_pat, dense5_pat,
        dense6_pat, dense7_pat,
        hor_pat, ver_pat, cross_pat, bdiag_pat, fdiag_pat, dcross_pat };
    return pat_tbl[brushStyle - Qt::Dense1Pattern];
}

void QCoreGraphicsPaintEnginePrivate::setFillBrush(const QPointF &offset)
{
    // pattern
    Qt::BrushStyle bs = current.brush.style();
#ifdef QT_MAC_USE_NATIVE_GRADIENTS
    if (bs == Qt::LinearGradientPattern || bs == Qt::RadialGradientPattern) {
        const QGradient *grad = static_cast<const QGradient*>(current.brush.gradient());
        if (drawGradientNatively(grad)) {
            Q_ASSERT(grad->spread() == QGradient::PadSpread);

            static const CGFloat domain[] = { 0.0f, +1.0f };
            static const CGFunctionCallbacks callbacks = { 0, qt_mac_color_gradient_function, 0 };
            CGFunctionRef fill_func = CGFunctionCreate(reinterpret_cast<void *>(&current.brush),
                    1, domain, 4, 0, &callbacks);

            CGColorSpaceRef colorspace = qt_mac_colorSpaceForDeviceType(pdev);
            if (bs == Qt::LinearGradientPattern) {
                const QLinearGradient *linearGrad = static_cast<const QLinearGradient *>(grad);
                const QPointF start(linearGrad->start());
                const QPointF stop(linearGrad->finalStop());
                shading = CGShadingCreateAxial(colorspace, CGPointMake(start.x(), start.y()),
                                               CGPointMake(stop.x(), stop.y()), fill_func, true, true);
            } else {
                Q_ASSERT(bs == Qt::RadialGradientPattern);
                const QRadialGradient *radialGrad = static_cast<const QRadialGradient *>(grad);
                QPointF center(radialGrad->center());
                QPointF focal(radialGrad->focalPoint());
                qreal radius = radialGrad->radius();
                qreal focalRadius = radialGrad->focalRadius();
                shading = CGShadingCreateRadial(colorspace, CGPointMake(focal.x(), focal.y()),
                                                focalRadius, CGPointMake(center.x(), center.y()), radius, fill_func, false, true);
            }

            CGFunctionRelease(fill_func);
        }
    } else
#endif
    if(bs != Qt::SolidPattern && bs != Qt::NoBrush
#ifndef QT_MAC_USE_NATIVE_GRADIENTS
       && (bs < Qt::LinearGradientPattern || bs > Qt::ConicalGradientPattern)
#endif
        )
    {
        QMacPattern *qpattern = new QMacPattern;
        qpattern->pdev = pdev;
        CGFloat components[4] = { 1.0, 1.0, 1.0, 1.0 };
        CGColorSpaceRef base_colorspace = 0;
        if(bs == Qt::TexturePattern) {
            qpattern->data.pixmap = current.brush.texture();
            if(qpattern->data.pixmap.isQBitmap()) {
                const QColor &col = current.brush.color();
                components[0] = qt_mac_convert_color_to_cg(col.red());
                components[1] = qt_mac_convert_color_to_cg(col.green());
                components[2] = qt_mac_convert_color_to_cg(col.blue());
                base_colorspace = QCoreGraphicsPaintEngine::macGenericColorSpace();
            }
        } else {
            qpattern->as_mask = true;

            qpattern->data.bytes = qt_mac_patternForBrush(bs);
            const QColor &col = current.brush.color();
            components[0] = qt_mac_convert_color_to_cg(col.red());
            components[1] = qt_mac_convert_color_to_cg(col.green());
            components[2] = qt_mac_convert_color_to_cg(col.blue());
            base_colorspace = QCoreGraphicsPaintEngine::macGenericColorSpace();
        }
        int width = qpattern->width(), height = qpattern->height();
        qpattern->foreground = current.brush.color();

        CGColorSpaceRef fill_colorspace = CGColorSpaceCreatePattern(base_colorspace);
        CGContextSetFillColorSpace(hd, fill_colorspace);

        CGAffineTransform xform = CGContextGetCTM(hd);
        xform = CGAffineTransformConcat(qt_mac_convert_transform_to_cg(current.brush.transform()), xform);
        xform = CGAffineTransformTranslate(xform, offset.x(), offset.y());

        CGPatternCallbacks callbks;
        callbks.version = 0;
        callbks.drawPattern = qt_mac_draw_pattern;
        callbks.releaseInfo = qt_mac_dispose_pattern;
        CGPatternRef fill_pattern = CGPatternCreate(qpattern, CGRectMake(0, 0, width, height),
                xform, width, height, kCGPatternTilingNoDistortion,
                !base_colorspace, &callbks);
        CGContextSetFillPattern(hd, fill_pattern, components);

        CGPatternRelease(fill_pattern);
        CGColorSpaceRelease(fill_colorspace);
    } else if(bs != Qt::NoBrush) {
        CGContextSetFillColorWithColor(hd, cgColorForQColor(current.brush.color(), pdev));
    }
}

void
QCoreGraphicsPaintEnginePrivate::setClip(const QRegion *rgn)
{
    Q_Q(QCoreGraphicsPaintEngine);
    if(hd) {
        resetClip();
        QRegion sysClip = q->systemClip();
        if(!sysClip.isEmpty())
            qt_mac_clip_cg(hd, sysClip, &orig_xform);
        if(rgn)
            qt_mac_clip_cg(hd, *rgn, 0);
    }
}

struct qt_mac_cg_transform_path {
    CGMutablePathRef path;
    CGAffineTransform transform;
};

void qt_mac_cg_transform_path_apply(void *info, const CGPathElement *element)
{
    Q_ASSERT(info && element);
    qt_mac_cg_transform_path *t = (qt_mac_cg_transform_path*)info;
    switch(element->type) {
    case kCGPathElementMoveToPoint:
        CGPathMoveToPoint(t->path, &t->transform, element->points[0].x, element->points[0].y);
        break;
    case kCGPathElementAddLineToPoint:
        CGPathAddLineToPoint(t->path, &t->transform, element->points[0].x, element->points[0].y);
        break;
    case kCGPathElementAddQuadCurveToPoint:
        CGPathAddQuadCurveToPoint(t->path, &t->transform, element->points[0].x, element->points[0].y,
                                  element->points[1].x, element->points[1].y);
        break;
    case kCGPathElementAddCurveToPoint:
        CGPathAddCurveToPoint(t->path, &t->transform, element->points[0].x, element->points[0].y,
                              element->points[1].x, element->points[1].y,
                              element->points[2].x, element->points[2].y);
        break;
    case kCGPathElementCloseSubpath:
        CGPathCloseSubpath(t->path);
        break;
    default:
        qDebug() << "Unhandled path transform type: " << element->type;
    }
}

void QCoreGraphicsPaintEnginePrivate::drawPath(uchar ops, CGMutablePathRef path)
{
    Q_Q(QCoreGraphicsPaintEngine);
    Q_ASSERT((ops & (CGFill | CGEOFill)) != (CGFill | CGEOFill)); //can't really happen
    if((ops & (CGFill | CGEOFill))) {
        if (shading) {
            Q_ASSERT(path);
            CGContextBeginPath(hd);
            CGContextAddPath(hd, path);
            saveGraphicsState();
            if (ops & CGFill)
                CGContextClip(hd);
            else if (ops & CGEOFill)
                CGContextEOClip(hd);
            if (current.brush.gradient()->coordinateMode() == QGradient::ObjectBoundingMode) {
                CGRect boundingBox = CGPathGetBoundingBox(path);
                CGContextConcatCTM(hd,
                    CGAffineTransformMake(boundingBox.size.width, 0,
                                          0, boundingBox.size.height,
                                          boundingBox.origin.x, boundingBox.origin.y));
            }
            CGContextDrawShading(hd, shading);
            restoreGraphicsState();
            ops &= ~CGFill;
            ops &= ~CGEOFill;
        } else if (current.brush.style() == Qt::NoBrush) {
            ops &= ~CGFill;
            ops &= ~CGEOFill;
        }
    }
    if((ops & CGStroke) && current.pen.style() == Qt::NoPen)
        ops &= ~CGStroke;

    if(ops & (CGEOFill | CGFill)) {
        CGContextBeginPath(hd);
        CGContextAddPath(hd, path);
        if (ops & CGEOFill) {
            CGContextEOFillPath(hd);
        } else {
            CGContextFillPath(hd);
        }
    }

    // Avoid saving and restoring the context if we can.
    const bool needContextSave = (cosmeticPen != QCoreGraphicsPaintEnginePrivate::CosmeticNone ||
                                  !(q->state->renderHints() & QPainter::Antialiasing));
    if(ops & CGStroke) {
        if (needContextSave)
            saveGraphicsState();
        CGContextBeginPath(hd);

        // Translate a fraction of a pixel size in the y direction
        // to make sure that primitives painted at pixel borders
        // fills the right pixel. This is needed since the y xais
        // in the Quartz coordinate system is inverted compared to Qt.
        if (!(q->state->renderHints() & QPainter::Antialiasing)) {
            if (current.pen.style() == Qt::SolidLine || current.pen.width() >= 3)
                CGContextTranslateCTM(hd, double(pixelSize.x()) * 0.25, double(pixelSize.y()) * 0.25);
            else if (current.pen.style() == Qt::DotLine && QSysInfo::MacintoshVersion == QSysInfo::MV_10_3)
                ; // Do nothing.
            else
                CGContextTranslateCTM(hd, 0, double(pixelSize.y()) * 0.1);
        }

        if (cosmeticPen != QCoreGraphicsPaintEnginePrivate::CosmeticNone) {
            // If antialiazing is enabled, use the cosmetic pen size directly.
            if (q->state->renderHints() & QPainter::Antialiasing)
                CGContextSetLineWidth(hd,  cosmeticPenSize);
            else if (current.pen.widthF() <= 1)
                CGContextSetLineWidth(hd, cosmeticPenSize * 0.9f);
            else
                CGContextSetLineWidth(hd, cosmeticPenSize);
        }
        if(cosmeticPen == QCoreGraphicsPaintEnginePrivate::CosmeticTransformPath) {
            qt_mac_cg_transform_path t;
            t.transform = qt_mac_convert_transform_to_cg(current.transform);
            t.path = CGPathCreateMutable();
            CGPathApply(path, &t, qt_mac_cg_transform_path_apply); //transform the path
            setTransform(0); //unset the context transform
            CGContextSetLineWidth(hd,  cosmeticPenSize);
            CGContextAddPath(hd, t.path);
            CGPathRelease(t.path);
        } else {
            CGContextAddPath(hd, path);
        }

        CGContextStrokePath(hd);
        if (needContextSave)
            restoreGraphicsState();
    }
}

QT_END_NAMESPACE
