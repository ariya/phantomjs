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

#include "qplatformdefs.h"

#include "private/qpixmap_x11_p.h"

#include "qapplication.h"
#include "qdebug.h"
#include "qfont.h"
#include "qwidget.h"
#include "qbitmap.h"
#include "qpixmapcache.h"
#include "qtextcodec.h"
#include "qcoreevent.h"
#include "qiodevice.h"
#include <qmath.h>

#include "qpainter_p.h"
#include <qtextlayout.h>
#include <qvarlengtharray.h>
#include <private/qfont_p.h>
#include <private/qtextengine_p.h>
#include <private/qpaintengine_x11_p.h>
#include <private/qfontengine_x11_p.h>
#include <private/qwidget_p.h>
#include <private/qpainterpath_p.h>

#include "qpen.h"
#include "qcolor.h"
#include "qcolormap.h"

#include <private/qpaintengine_p.h>
#include "qpaintengine_x11_p.h"

#include <private/qt_x11_p.h>
#include <private/qnumeric_p.h>
#include <limits.h>

#ifndef QT_NO_XRENDER
#include <private/qtessellator_p.h>
#endif

#include <private/qstylehelper_p.h>

QT_BEGIN_NAMESPACE

extern Drawable qt_x11Handle(const QPaintDevice *pd);
extern const QX11Info *qt_x11Info(const QPaintDevice *pd);
extern QPixmap qt_pixmapForBrush(int brushStyle, bool invert); //in qbrush.cpp
extern QPixmap qt_toX11Pixmap(const QPixmap &pixmap);

// use the same rounding as in qrasterizer.cpp (6 bit fixed point)
static const qreal aliasedCoordinateDelta = 0.5 - 0.015625;

#undef X11 // defined in qt_x11_p.h
/*!
    Returns the X11 specific pen GC for the painter \a p. Note that
    QPainter::begin() must be called before this function returns a
    valid GC.
*/
Q_GUI_EXPORT GC qt_x11_get_pen_gc(QPainter *p)
{
    if (p && p->paintEngine()
        && p->paintEngine()->isActive()
        && p->paintEngine()->type() == QPaintEngine::X11) {
        return static_cast<QX11PaintEngine *>(p->paintEngine())->d_func()->gc;
    }
    return 0;
}

/*!
    Returns the X11 specific brush GC for the painter \a p. Note that
    QPainter::begin() must be called before this function returns a
    valid GC.
*/
Q_GUI_EXPORT GC qt_x11_get_brush_gc(QPainter *p)
{
    if (p && p->paintEngine()
        && p->paintEngine()->isActive()
        && p->paintEngine()->type() == QPaintEngine::X11) {
        return static_cast<QX11PaintEngine *>(p->paintEngine())->d_func()->gc_brush;
    }
    return 0;
}
#define X11 qt_x11Data

#ifndef QT_NO_XRENDER
static const int compositionModeToRenderOp[QPainter::CompositionMode_Xor + 1] = {
    PictOpOver, //CompositionMode_SourceOver,
    PictOpOverReverse, //CompositionMode_DestinationOver,
    PictOpClear, //CompositionMode_Clear,
    PictOpSrc, //CompositionMode_Source,
    PictOpDst, //CompositionMode_Destination,
    PictOpIn, //CompositionMode_SourceIn,
    PictOpInReverse, //CompositionMode_DestinationIn,
    PictOpOut, //CompositionMode_SourceOut,
    PictOpOutReverse, //CompositionMode_DestinationOut,
    PictOpAtop, //CompositionMode_SourceAtop,
    PictOpAtopReverse, //CompositionMode_DestinationAtop,
    PictOpXor //CompositionMode_Xor
};

static inline int qpainterOpToXrender(QPainter::CompositionMode mode)
{
    Q_ASSERT(mode <= QPainter::CompositionMode_Xor);
    return compositionModeToRenderOp[mode];
}
#endif

// hack, so we don't have to make QRegion::clipRectangles() public or include
// X11 headers in qregion.h
Q_GUI_EXPORT void *qt_getClipRects(const QRegion &r, int &num)
{
    return r.clipRectangles(num);
}

static inline void x11SetClipRegion(Display *dpy, GC gc, GC gc2,
#ifndef QT_NO_XRENDER
                                    Picture picture,
#else
                                    Qt::HANDLE picture,
#endif
                                    const QRegion &r)
{
    int num;
    XRectangle *rects = (XRectangle *)qt_getClipRects(r, num);

    if (gc)
        XSetClipRectangles( dpy, gc, 0, 0, rects, num, YXBanded );
    if (gc2)
        XSetClipRectangles( dpy, gc2, 0, 0, rects, num, YXBanded );

#ifndef QT_NO_XRENDER
    if (picture)
        XRenderSetPictureClipRectangles(dpy, picture, 0, 0, rects, num);
#else
    Q_UNUSED(picture);
#endif // QT_NO_XRENDER
}


static inline void x11ClearClipRegion(Display *dpy, GC gc, GC gc2,
#ifndef QT_NO_XRENDER
                                    Picture picture
#else
                                    Qt::HANDLE picture
#endif
                                      )
{
    if (gc)
        XSetClipMask(dpy, gc, XNone);
    if (gc2)
        XSetClipMask(dpy, gc2, XNone);

#ifndef QT_NO_XRENDER
    if (picture) {
        XRenderPictureAttributes attrs;
        attrs.clip_mask = XNone;
        XRenderChangePicture (dpy, picture, CPClipMask, &attrs);
    }
#else
    Q_UNUSED(picture);
#endif // QT_NO_XRENDER
}


#define DITHER_SIZE 16
static const uchar base_dither_matrix[DITHER_SIZE][DITHER_SIZE] = {
  {   0,192, 48,240, 12,204, 60,252,  3,195, 51,243, 15,207, 63,255 },
  { 128, 64,176,112,140, 76,188,124,131, 67,179,115,143, 79,191,127 },
  {  32,224, 16,208, 44,236, 28,220, 35,227, 19,211, 47,239, 31,223 },
  { 160, 96,144, 80,172,108,156, 92,163, 99,147, 83,175,111,159, 95 },
  {   8,200, 56,248,  4,196, 52,244, 11,203, 59,251,  7,199, 55,247 },
  { 136, 72,184,120,132, 68,180,116,139, 75,187,123,135, 71,183,119 },
  {  40,232, 24,216, 36,228, 20,212, 43,235, 27,219, 39,231, 23,215 },
  { 168,104,152, 88,164,100,148, 84,171,107,155, 91,167,103,151, 87 },
  {   2,194, 50,242, 14,206, 62,254,  1,193, 49,241, 13,205, 61,253 },
  { 130, 66,178,114,142, 78,190,126,129, 65,177,113,141, 77,189,125 },
  {  34,226, 18,210, 46,238, 30,222, 33,225, 17,209, 45,237, 29,221 },
  { 162, 98,146, 82,174,110,158, 94,161, 97,145, 81,173,109,157, 93 },
  {  10,202, 58,250,  6,198, 54,246,  9,201, 57,249,  5,197, 53,245 },
  { 138, 74,186,122,134, 70,182,118,137, 73,185,121,133, 69,181,117 },
  {  42,234, 26,218, 38,230, 22,214, 41,233, 25,217, 37,229, 21,213 },
  { 170,106,154, 90,166,102,150, 86,169,105,153, 89,165,101,149, 85 }
};

static QPixmap qt_patternForAlpha(uchar alpha, int screen)
{
    QPixmap pm;
    QString key = QLatin1Literal("$qt-alpha-brush$")
                  % HexString<uchar>(alpha)
                  % HexString<int>(screen);

    if (!QPixmapCache::find(key, pm)) {
        // #### why not use a mono image here????
        QImage pattern(DITHER_SIZE, DITHER_SIZE, QImage::Format_ARGB32);
        pattern.fill(0xffffffff);
        for (int y = 0; y < DITHER_SIZE; ++y) {
            for (int x = 0; x < DITHER_SIZE; ++x) {
                if (base_dither_matrix[x][y] <= alpha)
                    pattern.setPixel(x, y, 0x00000000);
            }
        }
        pm = QBitmap::fromImage(pattern);
        pm.x11SetScreen(screen);
        QPixmapCache::insert(key, pm);
    }
    return pm;
}

#if !defined(QT_NO_XRENDER)

class QXRenderTessellator : public QTessellator
{
public:
    QXRenderTessellator() : traps(0), allocated(0), size(0) {}
    ~QXRenderTessellator() { free(traps); }
    XTrapezoid *traps;
    int allocated;
    int size;
    void addTrap(const Trapezoid &trap);
    QRect tessellate(const QPointF *points, int nPoints, bool winding) {
        size = 0;
        setWinding(winding);
        return QTessellator::tessellate(points, nPoints).toRect();
    }
    void done() {
        if (allocated > 64) {
            free(traps);
            traps = 0;
            allocated = 0;
        }
    }
};

void QXRenderTessellator::addTrap(const Trapezoid &trap)
{
    if (size == allocated) {
        allocated = qMax(2*allocated, 64);
        traps = q_check_ptr((XTrapezoid *)realloc(traps, allocated * sizeof(XTrapezoid)));
    }
    traps[size].top = Q27Dot5ToXFixed(trap.top);
    traps[size].bottom = Q27Dot5ToXFixed(trap.bottom);
    traps[size].left.p1.x = Q27Dot5ToXFixed(trap.topLeft->x);
    traps[size].left.p1.y = Q27Dot5ToXFixed(trap.topLeft->y);
    traps[size].left.p2.x = Q27Dot5ToXFixed(trap.bottomLeft->x);
    traps[size].left.p2.y = Q27Dot5ToXFixed(trap.bottomLeft->y);
    traps[size].right.p1.x = Q27Dot5ToXFixed(trap.topRight->x);
    traps[size].right.p1.y = Q27Dot5ToXFixed(trap.topRight->y);
    traps[size].right.p2.x = Q27Dot5ToXFixed(trap.bottomRight->x);
    traps[size].right.p2.y = Q27Dot5ToXFixed(trap.bottomRight->y);
    ++size;
}

#endif // !defined(QT_NO_XRENDER)


#ifndef QT_NO_XRENDER
static Picture getPatternFill(int screen, const QBrush &b)
{
    if (!X11->use_xrender)
        return XNone;

    XRenderColor color = X11->preMultiply(b.color());
    XRenderColor bg_color;

    bg_color = X11->preMultiply(QColor(0, 0, 0, 0));

    for (int i = 0; i < X11->pattern_fill_count; ++i) {
        if (X11->pattern_fills[i].screen == screen
            && X11->pattern_fills[i].opaque == false
            && X11->pattern_fills[i].style == b.style()
            && X11->pattern_fills[i].color.alpha == color.alpha
            && X11->pattern_fills[i].color.red == color.red
            && X11->pattern_fills[i].color.green == color.green
            && X11->pattern_fills[i].color.blue == color.blue
            && X11->pattern_fills[i].bg_color.alpha == bg_color.alpha
            && X11->pattern_fills[i].bg_color.red == bg_color.red
            && X11->pattern_fills[i].bg_color.green == bg_color.green
            && X11->pattern_fills[i].bg_color.blue == bg_color.blue)
            return X11->pattern_fills[i].picture;
    }
    // none found, replace one
    int i = qrand() % 16;

    if (X11->pattern_fills[i].screen != screen && X11->pattern_fills[i].picture) {
	XRenderFreePicture (X11->display, X11->pattern_fills[i].picture);
	X11->pattern_fills[i].picture = 0;
    }

    if (!X11->pattern_fills[i].picture) {
        Pixmap pixmap = XCreatePixmap (X11->display, RootWindow (X11->display, screen), 8, 8, 32);
        XRenderPictureAttributes attrs;
        attrs.repeat = True;
        X11->pattern_fills[i].picture = XRenderCreatePicture (X11->display, pixmap,
                                                              XRenderFindStandardFormat(X11->display, PictStandardARGB32),
                                                              CPRepeat, &attrs);
        XFreePixmap (X11->display, pixmap);
    }

    X11->pattern_fills[i].screen = screen;
    X11->pattern_fills[i].color = color;
    X11->pattern_fills[i].bg_color = bg_color;
    X11->pattern_fills[i].opaque = false;
    X11->pattern_fills[i].style = b.style();

    XRenderFillRectangle(X11->display, PictOpSrc, X11->pattern_fills[i].picture, &bg_color, 0, 0, 8, 8);

    QPixmap pattern(qt_pixmapForBrush(b.style(), true));
    XRenderPictureAttributes attrs;
    attrs.repeat = true;
    XRenderChangePicture(X11->display, pattern.x11PictureHandle(), CPRepeat, &attrs);

    Picture fill_fg = X11->getSolidFill(screen, b.color());
    XRenderComposite(X11->display, PictOpOver, fill_fg, pattern.x11PictureHandle(),
                     X11->pattern_fills[i].picture,
                     0, 0, 0, 0, 0, 0, 8, 8);

    return X11->pattern_fills[i].picture;
}

static void qt_render_bitmap(Display *dpy, int scrn, Picture src, Picture dst,
                      int sx, int sy, int x, int y, int sw, int sh,
                      const QPen &pen)
{
    Picture fill_fg = X11->getSolidFill(scrn, pen.color());
    XRenderComposite(dpy, PictOpOver,
                     fill_fg, src, dst, sx, sy, sx, sy, x, y, sw, sh);
}
#endif

void QX11PaintEnginePrivate::init()
{
    dpy = 0;
    scrn = 0;
    hd = 0;
    picture = 0;
    xinfo = 0;
#ifndef QT_NO_XRENDER
    current_brush = 0;
    composition_mode = PictOpOver;
    tessellator = new QXRenderTessellator;
#endif
}

void QX11PaintEnginePrivate::setupAdaptedOrigin(const QPoint &p)
{
    if (adapted_pen_origin)
        XSetTSOrigin(dpy, gc, p.x(), p.y());
    if (adapted_brush_origin)
        XSetTSOrigin(dpy, gc_brush, p.x(), p.y());
}

void QX11PaintEnginePrivate::resetAdaptedOrigin()
{
    if (adapted_pen_origin)
        XSetTSOrigin(dpy, gc, 0, 0);
    if (adapted_brush_origin)
        XSetTSOrigin(dpy, gc_brush, 0, 0);
}

void QX11PaintEnginePrivate::clipPolygon_dev(const QPolygonF &poly, QPolygonF *clipped_poly)
{
    int clipped_count = 0;
    qt_float_point *clipped_points = 0;
    polygonClipper.clipPolygon((qt_float_point *) poly.data(), poly.size(),
                               &clipped_points, &clipped_count);
    clipped_poly->resize(clipped_count);
    for (int i=0; i<clipped_count; ++i)
        (*clipped_poly)[i] = *((QPointF *)(&clipped_points[i]));
}

void QX11PaintEnginePrivate::systemStateChanged()
{
    Q_Q(QX11PaintEngine);
    QPainter *painter = q->state ? static_cast<QPainterState *>(q->state)->painter : 0;
    if (painter && painter->hasClipping()) {
        if (q->testDirty(QPaintEngine::DirtyTransform))
            q->updateMatrix(q->state->transform());
        QPolygonF clip_poly_dev(matrix.map(painter->clipPath().toFillPolygon()));
        QPolygonF clipped_poly_dev;
        clipPolygon_dev(clip_poly_dev, &clipped_poly_dev);
        q->updateClipRegion_dev(QRegion(clipped_poly_dev.toPolygon()), Qt::ReplaceClip);
    } else {
        q->updateClipRegion_dev(QRegion(), Qt::NoClip);
    }
}

static QPaintEngine::PaintEngineFeatures qt_decide_features()
{
    QPaintEngine::PaintEngineFeatures features =
        QPaintEngine::PrimitiveTransform
        | QPaintEngine::PatternBrush
        | QPaintEngine::AlphaBlend
        | QPaintEngine::PainterPaths
        | QPaintEngine::RasterOpModes;

    if (X11->use_xrender) {
        features |= QPaintEngine::Antialiasing;
        features |= QPaintEngine::PorterDuff;
        features |= QPaintEngine::MaskedBrush;
#if 0
        if (X11->xrender_version > 10) {
            features |= QPaintEngine::LinearGradientFill;
            // ###
        }
#endif
    }

    return features;
}

/*
 * QX11PaintEngine members
 */

QX11PaintEngine::QX11PaintEngine()
    : QPaintEngine(*(new QX11PaintEnginePrivate), qt_decide_features())
{
    d_func()->init();
}

QX11PaintEngine::QX11PaintEngine(QX11PaintEnginePrivate &dptr)
    : QPaintEngine(dptr, qt_decide_features())
{
    d_func()->init();
}

QX11PaintEngine::~QX11PaintEngine()
{
#ifndef QT_NO_XRENDER
    Q_D(QX11PaintEngine);
    delete d->tessellator;
#endif
}

bool QX11PaintEngine::begin(QPaintDevice *pdev)
{
    Q_D(QX11PaintEngine);
    d->xinfo = qt_x11Info(pdev);
    QWidget *w = d->pdev->devType() == QInternal::Widget ? static_cast<QWidget *>(d->pdev) : 0;
    const bool isAlienWidget = w && !w->internalWinId() && w->testAttribute(Qt::WA_WState_Created);
#ifndef QT_NO_XRENDER
    if (w) {
        if (isAlienWidget)
            d->picture = (::Picture)w->nativeParentWidget()->x11PictureHandle();
        else
            d->picture = (::Picture)w->x11PictureHandle();
    } else if (pdev->devType() == QInternal::Pixmap) {
        const QPixmap *pm = static_cast<const QPixmap *>(pdev);
        QX11PixmapData *data = static_cast<QX11PixmapData*>(pm->data.data());
        if (X11->use_xrender && data->depth() != 32 && data->x11_mask)
            data->convertToARGB32();
        d->picture = (::Picture)static_cast<const QPixmap *>(pdev)->x11PictureHandle();
    }
#else
    d->picture = 0;
#endif
    d->hd = !isAlienWidget ? qt_x11Handle(pdev) : qt_x11Handle(w->nativeParentWidget());

    Q_ASSERT(d->xinfo != 0);
    d->dpy = d->xinfo->display(); // get display variable
    d->scrn = d->xinfo->screen(); // get screen variable

    d->crgn = QRegion();
    d->gc = XCreateGC(d->dpy, d->hd, 0, 0);
    d->gc_brush = XCreateGC(d->dpy, d->hd, 0, 0);
    d->has_alpha_brush = false;
    d->has_alpha_pen = false;
    d->has_clipping = false;
    d->has_complex_xform = false;
    d->has_scaling_xform = false;
    d->has_non_scaling_xform = true;
    d->xform_scale = 1;
    d->has_custom_pen = false;
    d->matrix = QTransform();
    d->pdev_depth = d->pdev->depth();
    d->render_hints = 0;
    d->txop = QTransform::TxNone;
    d->use_path_fallback = false;
#if !defined(QT_NO_XRENDER)
    d->composition_mode = PictOpOver;
#endif
    d->xlibMaxLinePoints = 32762; // a safe number used to avoid, call to XMaxRequestSize(d->dpy) - 3;
    d->opacity = 1;

    // Set up the polygon clipper. Note: This will only work in
    // polyline mode as long as we have a buffer zone, since a
    // polyline may be clipped into several non-connected polylines.
    const int BUFFERZONE = 1000;
    QRect devClipRect(-BUFFERZONE, -BUFFERZONE,
                      pdev->width() + 2*BUFFERZONE, pdev->height() + 2*BUFFERZONE);
    d->polygonClipper.setBoundingRect(devClipRect);

    if (isAlienWidget) {
        // Set system clip for alien widgets painting outside the paint event.
        // This is not a problem with native windows since the windowing system
        // will handle the clip.
        QWidgetPrivate *wd = w->d_func();
        QRegion widgetClip(wd->clipRect());
        wd->clipToEffectiveMask(widgetClip);
        wd->subtractOpaqueSiblings(widgetClip);
        widgetClip.translate(w->mapTo(w->nativeParentWidget(), QPoint()));
        setSystemClip(widgetClip);
    }

    QPixmap::x11SetDefaultScreen(d->xinfo->screen());

    if (w && w->testAttribute(Qt::WA_PaintUnclipped)) {  // paint direct on device
 	updatePen(QPen(Qt::black));
 	updateBrush(QBrush(Qt::white), QPoint());
        XSetSubwindowMode(d->dpy, d->gc, IncludeInferiors);
        XSetSubwindowMode(d->dpy, d->gc_brush, IncludeInferiors);
#ifndef QT_NO_XRENDER
        XRenderPictureAttributes attrs;
        attrs.subwindow_mode = IncludeInferiors;
        XRenderChangePicture(d->dpy, d->picture, CPSubwindowMode, &attrs);
#endif
    }

    setDirty(QPaintEngine::DirtyClipRegion);
    setDirty(QPaintEngine::DirtyPen);
    setDirty(QPaintEngine::DirtyBrush);
    setDirty(QPaintEngine::DirtyBackground);

    return true;
}

bool QX11PaintEngine::end()
{
    Q_D(QX11PaintEngine);

#if !defined(QT_NO_XRENDER)
    if (d->picture) {
        // reset clipping/subwindow mode on our render picture
	XRenderPictureAttributes attrs;
	attrs.subwindow_mode = ClipByChildren;
        attrs.clip_mask = XNone;
	XRenderChangePicture(d->dpy, d->picture, CPClipMask|CPSubwindowMode, &attrs);
    }
#endif

    if (d->gc_brush && d->pdev->painters < 2) {
        XFreeGC(d->dpy, d->gc_brush);
        d->gc_brush = 0;
    }

    if (d->gc && d->pdev->painters < 2) {
        XFreeGC(d->dpy, d->gc);
        d->gc = 0;
    }

    // Restore system clip for alien widgets painting outside the paint event.
    if (d->pdev->devType() == QInternal::Widget && !static_cast<QWidget *>(d->pdev)->internalWinId())
        setSystemClip(QRegion());

    return true;
}

static bool clipLine(QLineF *line, const QRect &rect)
{
    qreal x1 = line->x1();
    qreal x2 = line->x2();
    qreal y1 = line->y1();
    qreal y2 = line->y2();

    qreal left = rect.x();
    qreal right = rect.x() + rect.width() - 1;
    qreal top = rect.y();
    qreal bottom = rect.y() + rect.height() - 1;

    enum { Left, Right, Top, Bottom };
    // clip the lines, after cohen-sutherland, see e.g. http://www.nondot.org/~sabre/graphpro/line6.html
    int p1 = ((x1 < left) << Left)
             | ((x1 > right) << Right)
             | ((y1 < top) << Top)
             | ((y1 > bottom) << Bottom);
    int p2 = ((x2 < left) << Left)
             | ((x2 > right) << Right)
             | ((y2 < top) << Top)
             | ((y2 > bottom) << Bottom);

    if (p1 & p2)
        // completely outside
        return false;

    if (p1 | p2) {
        qreal dx = x2 - x1;
        qreal dy = y2 - y1;

        // clip x coordinates
        if (x1 < left) {
            y1 += dy/dx * (left - x1);
            x1 = left;
        } else if (x1 > right) {
            y1 -= dy/dx * (x1 - right);
            x1 = right;
        }
        if (x2 < left) {
            y2 += dy/dx * (left - x2);
            x2 = left;
        } else if (x2 > right) {
            y2 -= dy/dx * (x2 - right);
            x2 = right;
        }
        p1 = ((y1 < top) << Top)
             | ((y1 > bottom) << Bottom);
        p2 = ((y2 < top) << Top)
             | ((y2 > bottom) << Bottom);
        if (p1 & p2)
            return false;
        // clip y coordinates
        if (y1 < top) {
            x1 += dx/dy * (top - y1);
            y1 = top;
        } else if (y1 > bottom) {
            x1 -= dx/dy * (y1 - bottom);
            y1 = bottom;
        }
        if (y2 < top) {
            x2 += dx/dy * (top - y2);
            y2 = top;
        } else if (y2 > bottom) {
            x2 -= dx/dy * (y2 - bottom);
            y2 = bottom;
        }
        *line = QLineF(QPointF(x1, y1), QPointF(x2, y2));
    }
    return true;
}

void QX11PaintEngine::drawLines(const QLine *lines, int lineCount)
{
    Q_ASSERT(lines);
    Q_ASSERT(lineCount);
    Q_D(QX11PaintEngine);
    if (d->has_alpha_brush
        || d->has_alpha_pen
        || d->has_custom_pen
        || (d->cpen.widthF() > 0 && d->has_complex_xform
            && !d->has_non_scaling_xform)
        || (d->render_hints & QPainter::Antialiasing)) {
        for (int i = 0; i < lineCount; ++i) {
            QPainterPath path(lines[i].p1());
            path.lineTo(lines[i].p2());
            drawPath(path);
        }
        return;
    }

    if (d->has_pen) {
        for (int i = 0; i < lineCount; ++i) {
            QLineF linef;
            if (d->txop == QTransform::TxNone) {
                linef = lines[i];
            } else {
                linef = d->matrix.map(QLineF(lines[i]));
            }
            if (clipLine(&linef, d->polygonClipper.boundingRect())) {
                int x1 = qRound(linef.x1() + aliasedCoordinateDelta);
                int y1 = qRound(linef.y1() + aliasedCoordinateDelta);
                int x2 = qRound(linef.x2() + aliasedCoordinateDelta);
                int y2 = qRound(linef.y2() + aliasedCoordinateDelta);

                XDrawLine(d->dpy, d->hd, d->gc, x1, y1, x2, y2);
            }
        }
    }
}

void QX11PaintEngine::drawLines(const QLineF *lines, int lineCount)
{
    Q_ASSERT(lines);
    Q_ASSERT(lineCount);
    Q_D(QX11PaintEngine);
    if (d->has_alpha_brush
        || d->has_alpha_pen
        || d->has_custom_pen
        || (d->cpen.widthF() > 0 && d->has_complex_xform
            && !d->has_non_scaling_xform)
        || (d->render_hints & QPainter::Antialiasing)) {
        for (int i = 0; i < lineCount; ++i) {
            QPainterPath path(lines[i].p1());
            path.lineTo(lines[i].p2());
            drawPath(path);
        }
        return;
    }

    if (d->has_pen) {
        for (int i = 0; i < lineCount; ++i) {
            QLineF linef = d->matrix.map(lines[i]);
            if (clipLine(&linef, d->polygonClipper.boundingRect())) {
                int x1 = qRound(linef.x1() + aliasedCoordinateDelta);
                int y1 = qRound(linef.y1() + aliasedCoordinateDelta);
                int x2 = qRound(linef.x2() + aliasedCoordinateDelta);
                int y2 = qRound(linef.y2() + aliasedCoordinateDelta);

                XDrawLine(d->dpy, d->hd, d->gc, x1, y1, x2, y2);
            }
        }
    }
}

static inline QLine clipStraightLine(const QRect &clip, const QLine &l)
{
    if (l.p1().x() == l.p2().x()) {
        int x = qBound(clip.left(), l.p1().x(), clip.right());
        int y1 = qBound(clip.top(), l.p1().y(), clip.bottom());
        int y2 = qBound(clip.top(), l.p2().y(), clip.bottom());

        return QLine(x, y1, x, y2);
    } else {
        Q_ASSERT(l.p1().y() == l.p2().y());

        int x1 = qBound(clip.left(), l.p1().x(), clip.right());
        int x2 = qBound(clip.left(), l.p2().x(), clip.right());
        int y = qBound(clip.top(), l.p1().y(), clip.bottom());

        return QLine(x1, y, x2, y);
    }
}

void QX11PaintEngine::drawRects(const QRectF *rects, int rectCount)
{
    Q_D(QX11PaintEngine);
    Q_ASSERT(rects);
    Q_ASSERT(rectCount);

    if (rectCount != 1
        || d->has_pen
        || d->has_alpha_brush
        || d->has_complex_xform
        || d->has_custom_pen
        || d->cbrush.style() != Qt::SolidPattern)
    {
        QPaintEngine::drawRects(rects, rectCount);
        return;
    }

    QPoint alignedOffset;
    if (d->txop == QTransform::TxTranslate) {
        QPointF offset(d->matrix.dx(), d->matrix.dy());
        alignedOffset = offset.toPoint();
        if (offset != QPointF(alignedOffset)) {
            QPaintEngine::drawRects(rects, rectCount);
            return;
        }
    }

    const QRectF& r = rects[0];
    QRect alignedRect = r.toAlignedRect();
    if (r != QRectF(alignedRect)) {
        QPaintEngine::drawRects(rects, rectCount);
        return;
    }
    alignedRect.translate(alignedOffset);

    QRect clip(d->polygonClipper.boundingRect());
    alignedRect = alignedRect.intersected(clip);
    if (alignedRect.isEmpty())
        return;

    // simple-case:
    //   the rectangle is pixel-aligned
    //   the fill brush is just a solid non-alpha color
    //   the painter transform is only integer translation
    // ignore: antialiasing and just XFillRectangles directly
    XRectangle xrect;
    xrect.x = short(alignedRect.x());
    xrect.y = short(alignedRect.y());
    xrect.width = ushort(alignedRect.width());
    xrect.height = ushort(alignedRect.height());
    XFillRectangles(d->dpy, d->hd, d->gc_brush, &xrect, 1);
}

void QX11PaintEngine::drawRects(const QRect *rects, int rectCount)
{
    Q_D(QX11PaintEngine);
    Q_ASSERT(rects);
    Q_ASSERT(rectCount);

    if (d->has_alpha_pen
        || d->has_complex_xform
        || d->has_custom_pen
        || (d->render_hints & QPainter::Antialiasing))
    {
        for (int i = 0; i < rectCount; ++i) {
            QPainterPath path;
            path.addRect(rects[i]);
            drawPath(path);
        }
        return;
    }

    QRect clip(d->polygonClipper.boundingRect());
    QPoint offset(qRound(d->matrix.dx()), qRound(d->matrix.dy()));
#if !defined(QT_NO_XRENDER)
    ::Picture pict = d->picture;

    if (X11->use_xrender && pict && d->has_brush && d->pdev_depth != 1
        && (d->has_texture || d->has_alpha_brush))
    {
        XRenderColor xc;
        if (!d->has_texture && !d->has_pattern)
            xc = X11->preMultiply(d->cbrush.color());

        for (int i = 0; i < rectCount; ++i) {
            QRect r(rects[i]);
            if (d->txop == QTransform::TxTranslate)
                r.translate(offset);

            if (r.width() == 0 || r.height() == 0) {
                if (d->has_pen) {
                    const QLine l = clipStraightLine(clip, QLine(r.left(), r.top(), r.left() + r.width(), r.top() + r.height()));
                    XDrawLine(d->dpy, d->hd, d->gc, l.p1().x(), l.p1().y(), l.p2().x(), l.p2().y());
                }
                continue;
            }

            r = r.intersected(clip);
            if (r.isEmpty())
                continue;
            if (d->has_texture || d->has_pattern) {
                XRenderComposite(d->dpy, d->composition_mode, d->current_brush, 0, pict,
                                 qRound(r.x() - d->bg_origin.x()), qRound(r.y() - d->bg_origin.y()),
                                 0, 0, r.x(), r.y(), r.width(), r.height());
            } else {
                XRenderFillRectangle(d->dpy, d->composition_mode, pict, &xc, r.x(), r.y(), r.width(), r.height());
            }
            if (d->has_pen)
                XDrawRectangle(d->dpy, d->hd, d->gc, r.x(), r.y(), r.width(), r.height());
        }
    } else
#endif // !QT_NO_XRENDER
    {
        if (d->has_brush && d->has_pen) {
            for (int i = 0; i < rectCount; ++i) {
                QRect r(rects[i]);
                if (d->txop == QTransform::TxTranslate)
                    r.translate(offset);

                if (r.width() == 0 || r.height() == 0) {
                    const QLine l = clipStraightLine(clip, QLine(r.left(), r.top(), r.left() + r.width(), r.top() + r.height()));
                    XDrawLine(d->dpy, d->hd, d->gc, l.p1().x(), l.p1().y(), l.p2().x(), l.p2().y());
                    continue;
                }

                r = r.intersected(clip);
                if (r.isEmpty())
                    continue;
                d->setupAdaptedOrigin(r.topLeft());
                XFillRectangle(d->dpy, d->hd, d->gc_brush, r.x(), r.y(), r.width(), r.height());
                XDrawRectangle(d->dpy, d->hd, d->gc, r.x(), r.y(), r.width(), r.height());
            }
            d->resetAdaptedOrigin();
        } else {
            QVarLengthArray<XRectangle> xrects(rectCount);
            int numClipped = rectCount;
            for (int i = 0; i < rectCount; ++i) {
                QRect r(rects[i]);
                if (d->txop == QTransform::TxTranslate)
                    r.translate(offset);

                if (r.width() == 0 || r.height() == 0) {
                    --numClipped;
                    if (d->has_pen) {
                        const QLine l = clipStraightLine(clip, QLine(r.left(), r.top(), r.left() + r.width(), r.top() + r.height()));
                        XDrawLine(d->dpy, d->hd, d->gc, l.p1().x(), l.p1().y(), l.p2().x(), l.p2().y());
                    }
                    continue;
                }

                r = r.intersected(clip);
                if (r.isEmpty()) {
                    --numClipped;
                    continue;
                }
                xrects[i].x = short(r.x());
                xrects[i].y = short(r.y());
                xrects[i].width = ushort(r.width());
                xrects[i].height = ushort(r.height());
            }
            if (numClipped) {
                d->setupAdaptedOrigin(rects[0].topLeft());
                if (d->has_brush)
                    XFillRectangles(d->dpy, d->hd, d->gc_brush, xrects.data(), numClipped);
                else if (d->has_pen)
                    XDrawRectangles(d->dpy, d->hd, d->gc, xrects.data(), numClipped);
                d->resetAdaptedOrigin();
            }
        }
    }
}

static inline void setCapStyle(int cap_style, GC gc)
{
    ulong mask = GCCapStyle;
    XGCValues vals;
    vals.cap_style = cap_style;
    XChangeGC(X11->display, gc, mask, &vals);
}

void QX11PaintEngine::drawPoints(const QPoint *points, int pointCount)
{
    Q_ASSERT(points);
    Q_ASSERT(pointCount);
    Q_D(QX11PaintEngine);

    if (!d->has_pen)
        return;

    // use the same test here as in drawPath to ensure that we don't use the path fallback
    // and end up in XDrawLines for pens with width <= 1
    if (d->cpen.widthF() > 1.0f
        || (X11->use_xrender && (d->has_alpha_pen || (d->render_hints & QPainter::Antialiasing)))
        || (!d->cpen.isCosmetic() && d->txop > QTransform::TxTranslate))
    {
        Qt::PenCapStyle capStyle = d->cpen.capStyle();
        if (capStyle == Qt::FlatCap) {
            setCapStyle(CapProjecting, d->gc);
            d->cpen.setCapStyle(Qt::SquareCap);
        }
        const QPoint *end = points + pointCount;
        while (points < end) {
            QPainterPath path;
            path.moveTo(*points);
            path.lineTo(points->x()+.005, points->y());
            drawPath(path);
            ++points;
        }

        if (capStyle == Qt::FlatCap) {
            setCapStyle(CapButt, d->gc);
            d->cpen.setCapStyle(capStyle);
        }
        return;
    }

    static const int BUF_SIZE = 1024;
    XPoint xPoints[BUF_SIZE];
    int i = 0, j = 0;
    while (i < pointCount) {
        while (i < pointCount && j < BUF_SIZE) {
            const QPoint &xformed = d->matrix.map(points[i]);
            int x = xformed.x();
            int y = xformed.y();
            if (x >= SHRT_MIN && y >= SHRT_MIN && x < SHRT_MAX && y < SHRT_MAX) {
                xPoints[j].x = x;
                xPoints[j].y = y;
                ++j;
            }
            ++i;
        }
        if (j)
            XDrawPoints(d->dpy, d->hd, d->gc, xPoints, j, CoordModeOrigin);

        j = 0;
    }
}

void QX11PaintEngine::drawPoints(const QPointF *points, int pointCount)
{
    Q_ASSERT(points);
    Q_ASSERT(pointCount);
    Q_D(QX11PaintEngine);

    if (!d->has_pen)
        return;

    // use the same test here as in drawPath to ensure that we don't use the path fallback
    // and end up in XDrawLines for pens with width <= 1
    if (d->cpen.widthF() > 1.0f
        || (X11->use_xrender && (d->has_alpha_pen || (d->render_hints & QPainter::Antialiasing)))
        || (!d->cpen.isCosmetic() && d->txop > QTransform::TxTranslate))
    {
        Qt::PenCapStyle capStyle = d->cpen.capStyle();
        if (capStyle == Qt::FlatCap) {
            setCapStyle(CapProjecting, d->gc);
            d->cpen.setCapStyle(Qt::SquareCap);
        }

        const QPointF *end = points + pointCount;
        while (points < end) {
            QPainterPath path;
            path.moveTo(*points);
            path.lineTo(points->x() + 0.005, points->y());
            drawPath(path);
            ++points;
        }
        if (capStyle == Qt::FlatCap) {
            setCapStyle(CapButt, d->gc);
            d->cpen.setCapStyle(capStyle);
        }
        return;
    }

    static const int BUF_SIZE = 1024;
    XPoint xPoints[BUF_SIZE];
    int i = 0, j = 0;
    while (i < pointCount) {
        while (i < pointCount && j < BUF_SIZE) {
            const QPointF &xformed = d->matrix.map(points[i]);
            int x = qFloor(xformed.x());
            int y = qFloor(xformed.y());

            if (x >= SHRT_MIN && y >= SHRT_MIN && x < SHRT_MAX && y < SHRT_MAX) {
                xPoints[j].x = x;
                xPoints[j].y = y;
                ++j;
            }
            ++i;
        }
        if (j)
            XDrawPoints(d->dpy, d->hd, d->gc, xPoints, j, CoordModeOrigin);

        j = 0;
    }
}

QPainter::RenderHints QX11PaintEngine::supportedRenderHints() const
{
#if !defined(QT_NO_XRENDER)
    if (X11->use_xrender)
        return QPainter::Antialiasing;
#endif
    return QFlag(0);
}

void QX11PaintEngine::updateState(const QPaintEngineState &state)
{
    Q_D(QX11PaintEngine);
    QPaintEngine::DirtyFlags flags = state.state();


    if (flags & DirtyOpacity) {
        d->opacity = state.opacity();
        // Force update pen/brush as to get proper alpha colors propagated
        flags |= DirtyPen;
        flags |= DirtyBrush;
    }

    if (flags & DirtyTransform) updateMatrix(state.transform());
    if (flags & DirtyPen) updatePen(state.pen());
    if (flags & (DirtyBrush | DirtyBrushOrigin)) updateBrush(state.brush(), state.brushOrigin());
    if (flags & DirtyFont) updateFont(state.font());

    if (state.state() & DirtyClipEnabled) {
        if (state.isClipEnabled()) {
            QPolygonF clip_poly_dev(d->matrix.map(painter()->clipPath().toFillPolygon()));
            QPolygonF clipped_poly_dev;
            d->clipPolygon_dev(clip_poly_dev, &clipped_poly_dev);
            updateClipRegion_dev(QRegion(clipped_poly_dev.toPolygon()), Qt::ReplaceClip);
        } else {
            updateClipRegion_dev(QRegion(), Qt::NoClip);
        }
    }

    if (flags & DirtyClipPath) {
        QPolygonF clip_poly_dev(d->matrix.map(state.clipPath().toFillPolygon()));
        QPolygonF clipped_poly_dev;
        d->clipPolygon_dev(clip_poly_dev, &clipped_poly_dev);
        updateClipRegion_dev(QRegion(clipped_poly_dev.toPolygon(), state.clipPath().fillRule()),
                             state.clipOperation());
    } else if (flags & DirtyClipRegion) {
        extern QPainterPath qt_regionToPath(const QRegion &region);
        QPainterPath clip_path = qt_regionToPath(state.clipRegion());
        QPolygonF clip_poly_dev(d->matrix.map(clip_path.toFillPolygon()));
        QPolygonF clipped_poly_dev;
        d->clipPolygon_dev(clip_poly_dev, &clipped_poly_dev);
        updateClipRegion_dev(QRegion(clipped_poly_dev.toPolygon()), state.clipOperation());
    }
    if (flags & DirtyHints) updateRenderHints(state.renderHints());
    if (flags & DirtyCompositionMode) {
        int function = GXcopy;
        if (state.compositionMode() >= QPainter::RasterOp_SourceOrDestination) {
            switch (state.compositionMode()) {
            case QPainter::RasterOp_SourceOrDestination:
                function = GXor;
                break;
            case QPainter::RasterOp_SourceAndDestination:
                function = GXand;
                break;
            case QPainter::RasterOp_SourceXorDestination:
                function = GXxor;
                break;
            case QPainter::RasterOp_NotSourceAndNotDestination:
                function = GXnor;
                break;
            case QPainter::RasterOp_NotSourceOrNotDestination:
                function = GXnand;
                break;
            case QPainter::RasterOp_NotSourceXorDestination:
                function = GXequiv;
                break;
            case QPainter::RasterOp_NotSource:
                function = GXcopyInverted;
                break;
            case QPainter::RasterOp_SourceAndNotDestination:
                function = GXandReverse;
                break;
            case QPainter::RasterOp_NotSourceAndDestination:
                function = GXandInverted;
                break;
            default:
                function = GXcopy;
            }
        }
#if !defined(QT_NO_XRENDER)
        else {
            d->composition_mode =
            qpainterOpToXrender(state.compositionMode());
        }
#endif
        XSetFunction(X11->display, d->gc, function);
        XSetFunction(X11->display, d->gc_brush, function);
    }
    d->decidePathFallback();
    d->decideCoordAdjust();
}

void QX11PaintEngine::updateRenderHints(QPainter::RenderHints hints)
{
    Q_D(QX11PaintEngine);
    d->render_hints = hints;

#if !defined(QT_NO_XRENDER)
    if (X11->use_xrender && d->picture) {
        XRenderPictureAttributes attrs;
        attrs.poly_edge = (hints & QPainter::Antialiasing) ? PolyEdgeSmooth : PolyEdgeSharp;
        XRenderChangePicture(d->dpy, d->picture, CPPolyEdge, &attrs);
    }
#endif
}

void QX11PaintEngine::updatePen(const QPen &pen)
{
    Q_D(QX11PaintEngine);
    d->cpen = pen;
    int cp = CapButt;
    int jn = JoinMiter;
    int ps = pen.style();

    if (d->opacity < 1.0) {
        QColor c = d->cpen.color();
        c.setAlpha(qRound(c.alpha()*d->opacity));
        d->cpen.setColor(c);
    }

    d->has_pen = (ps != Qt::NoPen);
    d->has_alpha_pen = (pen.color().alpha() != 255);

    switch (pen.capStyle()) {
    case Qt::SquareCap:
        cp = CapProjecting;
        break;
    case Qt::RoundCap:
        cp = CapRound;
        break;
    case Qt::FlatCap:
    default:
        cp = CapButt;
        break;
    }
    switch (pen.joinStyle()) {
    case Qt::BevelJoin:
        jn = JoinBevel;
        break;
    case Qt::RoundJoin:
        jn = JoinRound;
        break;
    case Qt::MiterJoin:
    default:
        jn = JoinMiter;
        break;
    }

    d->adapted_pen_origin = false;

    char dashes[10];                            // custom pen dashes
    int dash_len = 0;                           // length of dash list
    int xStyle = LineSolid;

    /*
      We are emulating Windows here.  Windows treats cpen.width() == 1
      (or 0) as a very special case.  The fudge variable unifies this
      case with the general case.
    */
    qreal pen_width = pen.widthF();
    int scale =  qRound(pen_width < 1 ? 1 : pen_width);
    int space = (pen_width < 1 && pen_width > 0 ? 1 : (2 * scale));
    int dot = 1 * scale;
    int dash = 4 * scale;

    d->has_custom_pen = false;

    switch (ps) {
    case Qt::NoPen:
    case Qt::SolidLine:
        xStyle = LineSolid;
	break;
    case Qt::DashLine:
	dashes[0] = dash;
	dashes[1] = space;
	dash_len = 2;
        xStyle = LineOnOffDash;
	break;
    case Qt::DotLine:
	dashes[0] = dot;
	dashes[1] = space;
	dash_len = 2;
        xStyle = LineOnOffDash;
	break;
    case Qt::DashDotLine:
	dashes[0] = dash;
	dashes[1] = space;
	dashes[2] = dot;
	dashes[3] = space;
	dash_len = 4;
        xStyle = LineOnOffDash;
	break;
    case Qt::DashDotDotLine:
	dashes[0] = dash;
	dashes[1] = space;
	dashes[2] = dot;
	dashes[3] = space;
	dashes[4] = dot;
	dashes[5] = space;
	dash_len = 6;
        xStyle = LineOnOffDash;
        break;
    case Qt::CustomDashLine:
        d->has_custom_pen = true;
        break;
    }

    ulong mask = GCForeground | GCBackground | GCGraphicsExposures | GCLineWidth
                 | GCCapStyle | GCJoinStyle | GCLineStyle;
    XGCValues vals;
    vals.graphics_exposures = false;
    if (d->pdev_depth == 1) {
        vals.foreground = qGray(pen.color().rgb()) > 127 ? 0 : 1;
        vals.background = qGray(QColor(Qt::transparent).rgb()) > 127 ? 0 : 1;
    } else if (d->pdev->devType() == QInternal::Pixmap && d->pdev_depth == 32
        && X11->use_xrender) {
        vals.foreground = pen.color().rgba();
        vals.background = QColor(Qt::transparent).rgba();
    } else {
        QColormap cmap = QColormap::instance(d->scrn);
        vals.foreground = cmap.pixel(pen.color());
        vals.background = cmap.pixel(QColor(Qt::transparent));
    }


    vals.line_width = qRound(pen.widthF());
    vals.cap_style = cp;
    vals.join_style = jn;
    vals.line_style = xStyle;

    XChangeGC(d->dpy, d->gc, mask, &vals);

    if (dash_len) { // make dash list
        XSetDashes(d->dpy, d->gc, 0, dashes, dash_len);
    }

    if (!d->has_clipping) { // if clipping is set the paintevent clip region is merged with the clip region
        QRegion sysClip = systemClip();
        if (!sysClip.isEmpty())
            x11SetClipRegion(d->dpy, d->gc, 0, d->picture, sysClip);
        else
            x11ClearClipRegion(d->dpy, d->gc, 0, d->picture);
    }
}

void QX11PaintEngine::updateBrush(const QBrush &brush, const QPointF &origin)
{
    Q_D(QX11PaintEngine);
    d->cbrush = brush;
    d->bg_origin = origin;
    d->adapted_brush_origin = false;
#if !defined(QT_NO_XRENDER)
    d->current_brush = 0;
#endif
    if (d->opacity < 1.0) {
        QColor c = d->cbrush.color();
        c.setAlpha(qRound(c.alpha()*d->opacity));
        d->cbrush.setColor(c);
    }

    int s  = FillSolid;
    int  bs = d->cbrush.style();
    d->has_brush = (bs != Qt::NoBrush);
    d->has_pattern = bs >= Qt::Dense1Pattern && bs <= Qt::DiagCrossPattern;
    d->has_texture = bs == Qt::TexturePattern;
    d->has_alpha_brush = brush.color().alpha() != 255;
    d->has_alpha_texture = d->has_texture && d->cbrush.texture().hasAlphaChannel();

    ulong mask = GCForeground | GCBackground | GCGraphicsExposures
                 | GCLineStyle | GCCapStyle | GCJoinStyle | GCFillStyle;
    XGCValues vals;
    vals.graphics_exposures = false;
    if (d->pdev_depth == 1) {
        vals.foreground = qGray(d->cbrush.color().rgb()) > 127 ? 0 : 1;
        vals.background = qGray(QColor(Qt::transparent).rgb()) > 127 ? 0 : 1;
    } else if (X11->use_xrender && d->pdev->devType() == QInternal::Pixmap
               && d->pdev_depth == 32) {
        vals.foreground = d->cbrush.color().rgba();
        vals.background = QColor(Qt::transparent).rgba();
    } else {
        QColormap cmap = QColormap::instance(d->scrn);
        vals.foreground = cmap.pixel(d->cbrush.color());
        vals.background = cmap.pixel(QColor(Qt::transparent));

        if (!X11->use_xrender && d->has_brush && !d->has_pattern && !brush.isOpaque()) {
            QPixmap pattern = qt_patternForAlpha(brush.color().alpha(), d->scrn);
            mask |= GCStipple;
            vals.stipple = pattern.handle();
            s = FillStippled;
            d->adapted_brush_origin = true;
        }
    }
    vals.cap_style = CapButt;
    vals.join_style = JoinMiter;
    vals.line_style = LineSolid;

    if (d->has_pattern || d->has_texture) {
        if (bs == Qt::TexturePattern) {
            d->brush_pm = qt_toX11Pixmap(d->cbrush.texture());
#if !defined(QT_NO_XRENDER)
            if (X11->use_xrender) {
                XRenderPictureAttributes attrs;
                attrs.repeat = true;
                XRenderChangePicture(d->dpy, d->brush_pm.x11PictureHandle(), CPRepeat, &attrs);
                QX11PixmapData *data = static_cast<QX11PixmapData*>(d->brush_pm.data.data());
                if (data->mask_picture)
                    XRenderChangePicture(d->dpy, data->mask_picture, CPRepeat, &attrs);
            }
#endif
        } else {
            d->brush_pm = qt_toX11Pixmap(qt_pixmapForBrush(bs, true));
        }
        d->brush_pm.x11SetScreen(d->scrn);
        if (d->brush_pm.depth() == 1) {
            mask |= GCStipple;
            vals.stipple = d->brush_pm.handle();
            s = FillStippled;
#if !defined(QT_NO_XRENDER)
            if (X11->use_xrender) {
                d->bitmap_texture = QPixmap(d->brush_pm.size());
                d->bitmap_texture.fill(Qt::transparent);
                d->bitmap_texture = qt_toX11Pixmap(d->bitmap_texture);
                d->bitmap_texture.x11SetScreen(d->scrn);

                ::Picture src  = X11->getSolidFill(d->scrn, d->cbrush.color());
                XRenderComposite(d->dpy, PictOpSrc, src, d->brush_pm.x11PictureHandle(),
                                 d->bitmap_texture.x11PictureHandle(),
                                 0, 0, d->brush_pm.width(), d->brush_pm.height(),
                                 0, 0, d->brush_pm.width(), d->brush_pm.height());

                XRenderPictureAttributes attrs;
                attrs.repeat = true;
                XRenderChangePicture(d->dpy, d->bitmap_texture.x11PictureHandle(), CPRepeat, &attrs);

                d->current_brush = d->bitmap_texture.x11PictureHandle();
            }
#endif
        } else {
            mask |= GCTile;
#ifndef QT_NO_XRENDER
            if (d->pdev_depth == 32 && d->brush_pm.depth() != 32) {
                d->brush_pm.detach();
                QX11PixmapData *brushData = static_cast<QX11PixmapData*>(d->brush_pm.data.data());
                brushData->convertToARGB32();
            }
#endif
            vals.tile = (d->brush_pm.depth() == d->pdev_depth
                         ? d->brush_pm.handle()
                         : static_cast<QX11PixmapData*>(d->brush_pm.data.data())->x11ConvertToDefaultDepth());
            s = FillTiled;
#if !defined(QT_NO_XRENDER)
            d->current_brush = d->cbrush.texture().x11PictureHandle();
#endif
        }

        mask |= GCTileStipXOrigin | GCTileStipYOrigin;
        vals.ts_x_origin = qRound(origin.x());
        vals.ts_y_origin = qRound(origin.y());
    }
#if !defined(QT_NO_XRENDER)
    else if (d->has_alpha_brush) {
        d->current_brush = X11->getSolidFill(d->scrn, d->cbrush.color());
    }
#endif

    vals.fill_style = s;
    XChangeGC(d->dpy, d->gc_brush, mask, &vals);
    if (!d->has_clipping) {
        QRegion sysClip = systemClip();
        if (!sysClip.isEmpty())
            x11SetClipRegion(d->dpy, d->gc_brush, 0, d->picture, sysClip);
        else
            x11ClearClipRegion(d->dpy, d->gc_brush, 0, d->picture);
    }
}

void QX11PaintEngine::drawEllipse(const QRectF &rect)
{
    QRect aligned = rect.toAlignedRect();
    if (aligned == rect)
        drawEllipse(aligned);
    else
        QPaintEngine::drawEllipse(rect);
}

void QX11PaintEngine::drawEllipse(const QRect &rect)
{
    if (rect.isEmpty()) {
        drawRects(&rect, 1);
        return;
    }

    Q_D(QX11PaintEngine);
    QRect devclip(SHRT_MIN, SHRT_MIN, SHRT_MAX*2 - 1, SHRT_MAX*2 - 1);
    QRect r(rect);
    if (d->txop < QTransform::TxRotate) {
        r = d->matrix.mapRect(rect);
    } else if (d->txop == QTransform::TxRotate && rect.width() == rect.height()) {
        QPainterPath path;
        path.addEllipse(rect);
        r = d->matrix.map(path).boundingRect().toRect();
    }

    if (d->has_alpha_brush || d->has_alpha_pen || d->has_custom_pen || (d->render_hints & QPainter::Antialiasing)
        || d->has_alpha_texture || devclip.intersected(r) != r
        || (d->has_complex_xform
            && !(d->has_non_scaling_xform && rect.width() == rect.height())))
    {
        QPainterPath path;
        path.addEllipse(rect);
        drawPath(path);
        return;
    }

    int x = r.x();
    int y = r.y();
    int w = r.width();
    int h = r.height();
    if (w < 1 || h < 1)
        return;
    if (w == 1 && h == 1) {
        XDrawPoint(d->dpy, d->hd, d->has_pen ? d->gc : d->gc_brush, x, y);
        return;
    }
    d->setupAdaptedOrigin(rect.topLeft());
    if (d->has_brush) {             // draw filled ellipse
        XFillArc(d->dpy, d->hd, d->gc_brush, x, y, w, h, 0, 360*64);
        if (!d->has_pen)            // make smoother outline
            XDrawArc(d->dpy, d->hd, d->gc_brush, x, y, w-1, h-1, 0, 360*64);
    }
    if (d->has_pen)                // draw outline
        XDrawArc(d->dpy, d->hd, d->gc, x, y, w, h, 0, 360*64);
    d->resetAdaptedOrigin();
}



void QX11PaintEnginePrivate::fillPolygon_translated(const QPointF *polygonPoints, int pointCount,
                                                    QX11PaintEnginePrivate::GCMode gcMode,
                                                    QPaintEngine::PolygonDrawMode mode)
{

    QVarLengthArray<QPointF> translated_points(pointCount);
    QPointF offset(matrix.dx(), matrix.dy());

    qreal offs = adjust_coords ? aliasedCoordinateDelta : 0.0;
    if (!X11->use_xrender || !(render_hints & QPainter::Antialiasing))
        offset += QPointF(aliasedCoordinateDelta, aliasedCoordinateDelta);

    for (int i = 0; i < pointCount; ++i) {
        translated_points[i] = polygonPoints[i] + offset;

        translated_points[i].rx() = qRound(translated_points[i].x()) + offs;
        translated_points[i].ry() = qRound(translated_points[i].y()) + offs;
    }

    fillPolygon_dev(translated_points.data(), pointCount, gcMode, mode);
}

#ifndef QT_NO_XRENDER
static void qt_XRenderCompositeTrapezoids(Display *dpy,
                                          int op,
                                          Picture src,
                                          Picture dst,
                                          _Xconst XRenderPictFormat *maskFormat,
                                          int xSrc,
                                          int ySrc,
                                          const XTrapezoid *traps, int size)
{
    const int MAX_TRAPS = 50000;
    while (size) {
        int to_draw = size;
        if (to_draw > MAX_TRAPS)
            to_draw = MAX_TRAPS;
        XRenderCompositeTrapezoids(dpy, op, src, dst,
                                   maskFormat,
                                   xSrc, ySrc,
                                   traps, to_draw);
        size -= to_draw;
        traps += to_draw;
    }
}
#endif

void QX11PaintEnginePrivate::fillPolygon_dev(const QPointF *polygonPoints, int pointCount,
                                             QX11PaintEnginePrivate::GCMode gcMode,
                                             QPaintEngine::PolygonDrawMode mode)
{
    Q_Q(QX11PaintEngine);

    int clippedCount = 0;
    qt_float_point *clippedPoints = 0;

#ifndef QT_NO_XRENDER
    //can change if we switch to pen if gcMode != BrushGC
    bool has_fill_texture = has_texture;
    bool has_fill_pattern = has_pattern;
    ::Picture src;
#endif
    QBrush fill;
    GC fill_gc;
    if (gcMode == BrushGC) {
        fill = cbrush;
        fill_gc = gc_brush;
#ifndef QT_NO_XRENDER
        if (current_brush)
            src = current_brush;
        else
            src = X11->getSolidFill(scrn, fill.color());
#endif
    } else {
        fill = QBrush(cpen.brush());
        fill_gc = gc;
#ifndef QT_NO_XRENDER
        //we use the pens brush
        has_fill_texture = (fill.style() == Qt::TexturePattern);
        has_fill_pattern = (fill.style() >= Qt::Dense1Pattern && fill.style() <= Qt::DiagCrossPattern);
        if (has_fill_texture)
            src = fill.texture().x11PictureHandle();
        else if (has_fill_pattern)
            src = getPatternFill(scrn, fill);
        else
            src = X11->getSolidFill(scrn, fill.color());
#endif
    }

    polygonClipper.clipPolygon((qt_float_point *) polygonPoints, pointCount,
                               &clippedPoints, &clippedCount);

#ifndef QT_NO_XRENDER
    bool solid_fill = fill.color().alpha() == 255;
    if (has_fill_texture && fill.texture().depth() == 1 && solid_fill) {
        has_fill_texture = false;
        has_fill_pattern = true;
    }

    bool antialias = render_hints & QPainter::Antialiasing;

    if (X11->use_xrender
        && picture
        && !has_fill_pattern
        && (clippedCount > 0)
        && (fill.style() != Qt::NoBrush)
        && ((has_fill_texture && fill.texture().hasAlpha()) || antialias || !solid_fill || has_alpha_pen != has_alpha_brush))
    {
        tessellator->tessellate((QPointF *)clippedPoints, clippedCount,
                                mode == QPaintEngine::WindingMode);
        if (tessellator->size > 0) {
            XRenderPictureAttributes attrs;
            attrs.poly_edge = antialias ? PolyEdgeSmooth : PolyEdgeSharp;
            XRenderChangePicture(dpy, picture, CPPolyEdge, &attrs);
            int x_offset = int(XFixedToDouble(tessellator->traps[0].left.p1.x) - bg_origin.x());
            int y_offset = int(XFixedToDouble(tessellator->traps[0].left.p1.y) - bg_origin.y());
            qt_XRenderCompositeTrapezoids(dpy, composition_mode, src, picture,
                                          antialias
                                          ? XRenderFindStandardFormat(dpy, PictStandardA8)
                                          : XRenderFindStandardFormat(dpy, PictStandardA1),
                                          x_offset, y_offset,
                                          tessellator->traps, tessellator->size);
            tessellator->done();
        }
    } else
#endif
        if (fill.style() != Qt::NoBrush) {
            if (clippedCount > 200000) {
                QPolygon poly;
                for (int i = 0; i < clippedCount; ++i)
                    poly << QPoint(qFloor(clippedPoints[i].x), qFloor(clippedPoints[i].y));

                const QRect bounds = poly.boundingRect();
                const QRect aligned = bounds
                    & QRect(QPoint(), QSize(pdev->width(), pdev->height()));

                QImage img(aligned.size(), QImage::Format_ARGB32_Premultiplied);
                img.fill(0);

                QPainter painter(&img);
                painter.translate(-aligned.x(), -aligned.y());
                painter.setPen(Qt::NoPen);
                painter.setBrush(fill);
                if (gcMode == BrushGC)
                    painter.setBrushOrigin(q->painter()->brushOrigin());
                painter.drawPolygon(poly);
                painter.end();

                q->drawImage(aligned, img, img.rect(), Qt::AutoColor);
            } else if (clippedCount > 0) {
                QVarLengthArray<XPoint> xpoints(clippedCount);
                for (int i = 0; i < clippedCount; ++i) {
                    xpoints[i].x = qFloor(clippedPoints[i].x);
                    xpoints[i].y = qFloor(clippedPoints[i].y);
                }
                if (mode == QPaintEngine::WindingMode)
                    XSetFillRule(dpy, fill_gc, WindingRule);
                setupAdaptedOrigin(QPoint(xpoints[0].x, xpoints[0].y));
                XFillPolygon(dpy, hd, fill_gc,
                             xpoints.data(), clippedCount,
                             mode == QPaintEngine::ConvexMode ? Convex : Complex, CoordModeOrigin);
                resetAdaptedOrigin();
                if (mode == QPaintEngine::WindingMode)
                    XSetFillRule(dpy, fill_gc, EvenOddRule);
            }
        }
}

void QX11PaintEnginePrivate::strokePolygon_translated(const QPointF *polygonPoints, int pointCount, bool close)
{
    QVarLengthArray<QPointF> translated_points(pointCount);
    QPointF offset(matrix.dx(), matrix.dy());
    for (int i = 0; i < pointCount; ++i)
        translated_points[i] = polygonPoints[i] + offset;
    strokePolygon_dev(translated_points.data(), pointCount, close);
}

void QX11PaintEnginePrivate::strokePolygon_dev(const QPointF *polygonPoints, int pointCount, bool close)
{
    int clippedCount = 0;
    qt_float_point *clippedPoints = 0;
    polygonClipper.clipPolygon((qt_float_point *) polygonPoints, pointCount,
                               &clippedPoints, &clippedCount, close);

    if (clippedCount > 0) {
        QVarLengthArray<XPoint> xpoints(clippedCount);
        for (int i = 0; i < clippedCount; ++i) {
            xpoints[i].x = qRound(clippedPoints[i].x + aliasedCoordinateDelta);
            xpoints[i].y = qRound(clippedPoints[i].y + aliasedCoordinateDelta);
        }
        uint numberPoints = qMin(clippedCount, xlibMaxLinePoints);
        XPoint *pts = xpoints.data();
        XDrawLines(dpy, hd, gc, pts, numberPoints, CoordModeOrigin);
        pts += numberPoints;
        clippedCount -= numberPoints;
        numberPoints = qMin(clippedCount, xlibMaxLinePoints-1);
        while (clippedCount) {
            XDrawLines(dpy, hd, gc, pts-1, numberPoints+1, CoordModeOrigin);
            pts += numberPoints;
            clippedCount -= numberPoints;
            numberPoints = qMin(clippedCount, xlibMaxLinePoints-1);
        }
    }
}

void QX11PaintEngine::drawPolygon(const QPointF *polygonPoints, int pointCount, PolygonDrawMode mode)
{
    Q_D(QX11PaintEngine);
    if (d->use_path_fallback) {
        QPainterPath path(polygonPoints[0]);
        for (int i = 1; i < pointCount; ++i)
            path.lineTo(polygonPoints[i]);
        if (mode == PolylineMode) {
            QBrush oldBrush = d->cbrush;
            d->cbrush = QBrush(Qt::NoBrush);
            path.setFillRule(Qt::WindingFill);
            drawPath(path);
            d->cbrush = oldBrush;
        } else {
            path.setFillRule(mode == OddEvenMode ? Qt::OddEvenFill : Qt::WindingFill);
            path.closeSubpath();
            drawPath(path);
        }
        return;
    }
    if (mode != PolylineMode && d->has_brush)
        d->fillPolygon_translated(polygonPoints, pointCount, QX11PaintEnginePrivate::BrushGC, mode);

    if (d->has_pen)
        d->strokePolygon_translated(polygonPoints, pointCount, mode != PolylineMode);
}


void QX11PaintEnginePrivate::fillPath(const QPainterPath &path, QX11PaintEnginePrivate::GCMode gc_mode, bool transform)
{
    qreal offs = adjust_coords ? aliasedCoordinateDelta : 0.0;

    QPainterPath clippedPath;
    QPainterPath clipPath;
    clipPath.addRect(polygonClipper.boundingRect());

    if (transform)
         clippedPath = (path*matrix).intersected(clipPath);
    else
        clippedPath = path.intersected(clipPath);

    QList<QPolygonF> polys = clippedPath.toFillPolygons();
    for (int i = 0; i < polys.size(); ++i) {
        QVarLengthArray<QPointF> translated_points(polys.at(i).size());

        for (int j = 0; j < polys.at(i).size(); ++j) {
            translated_points[j] = polys.at(i).at(j);
            if (!X11->use_xrender || !(render_hints & QPainter::Antialiasing)) {
                translated_points[j].rx() = qRound(translated_points[j].rx() + aliasedCoordinateDelta) + offs;
                translated_points[j].ry() = qRound(translated_points[j].ry() + aliasedCoordinateDelta) + offs;
            }
        }

        fillPolygon_dev(translated_points.data(), polys.at(i).size(), gc_mode,
                        path.fillRule() == Qt::OddEvenFill ? QPaintEngine::OddEvenMode : QPaintEngine::WindingMode);
    }
}

void QX11PaintEngine::drawPath(const QPainterPath &path)
{
    Q_D(QX11PaintEngine);
    if (path.isEmpty())
        return;

    if (d->has_brush)
        d->fillPath(path, QX11PaintEnginePrivate::BrushGC, true);
    if (d->has_pen
        && ((X11->use_xrender && (d->has_alpha_pen || (d->render_hints & QPainter::Antialiasing)))
            || (!d->cpen.isCosmetic() && d->txop > QTransform::TxTranslate
                && !d->has_non_scaling_xform)
            || (d->cpen.style() == Qt::CustomDashLine))) {
        QPainterPathStroker stroker;
        if (d->cpen.style() == Qt::CustomDashLine) {
            stroker.setDashPattern(d->cpen.dashPattern());
            stroker.setDashOffset(d->cpen.dashOffset());
        } else {
            stroker.setDashPattern(d->cpen.style());
        }
        stroker.setCapStyle(d->cpen.capStyle());
        stroker.setJoinStyle(d->cpen.joinStyle());
        QPainterPath stroke;
        qreal width = d->cpen.widthF();
        QPolygonF poly;
        QRectF deviceRect(0, 0, d->pdev->width(), d->pdev->height());
        // necessary to get aliased alphablended primitives to be drawn correctly
        if (d->cpen.isCosmetic() || d->has_scaling_xform) {
            if (d->cpen.isCosmetic())
                stroker.setWidth(width == 0 ? 1 : width);
            else
                stroker.setWidth(width * d->xform_scale);
            stroker.d_ptr->stroker.setClipRect(deviceRect);
            stroke = stroker.createStroke(path * d->matrix);
            if (stroke.isEmpty())
                return;
            stroke.setFillRule(Qt::WindingFill);
            d->fillPath(stroke, QX11PaintEnginePrivate::PenGC, false);
        } else {
            stroker.setWidth(width);
            stroker.d_ptr->stroker.setClipRect(d->matrix.inverted().mapRect(deviceRect));
            stroke = stroker.createStroke(path);
            if (stroke.isEmpty())
                return;
            stroke.setFillRule(Qt::WindingFill);
            d->fillPath(stroke, QX11PaintEnginePrivate::PenGC, true);
        }
    } else if (d->has_pen) {
        // if we have a cosmetic pen - use XDrawLine() for speed
        QList<QPolygonF> polys = path.toSubpathPolygons(d->matrix);
        for (int i = 0; i < polys.size(); ++i)
            d->strokePolygon_dev(polys.at(i).data(), polys.at(i).size(), false);
    }
}

Q_GUI_EXPORT void qt_x11_drawImage(const QRect &rect, const QPoint &pos, const QImage &image,
                                   Drawable hd, GC gc, Display *dpy, Visual *visual, int depth)
{
    Q_ASSERT(image.format() == QImage::Format_RGB32);
    Q_ASSERT(image.depth() == 32);

    XImage *xi;
    // Note: this code assumes either RGB or BGR, 8 bpc server layouts
    const uint red_mask = (uint) visual->red_mask;
    bool bgr_layout = (red_mask == 0xff);

    const int w = rect.width();
    const int h = rect.height();

    QImage im;
    int image_byte_order = ImageByteOrder(X11->display);
    if ((QSysInfo::ByteOrder == QSysInfo::BigEndian && ((image_byte_order == LSBFirst) || bgr_layout))
        || (image_byte_order == MSBFirst && QSysInfo::ByteOrder == QSysInfo::LittleEndian)
        || (image_byte_order == LSBFirst && bgr_layout))
    {
        im = image.copy(rect);
        const int iw = im.bytesPerLine() / 4;
        uint *data = (uint *)im.bits();
        for (int i=0; i < h; i++) {
            uint *p = data;
            uint *end = p + w;
            if (bgr_layout && image_byte_order == MSBFirst && QSysInfo::ByteOrder == QSysInfo::LittleEndian) {
                while (p < end) {
                    *p = ((*p << 8) & 0xffffff00) | ((*p >> 24) & 0x000000ff);
                    p++;
                }
            } else if ((image_byte_order == LSBFirst && QSysInfo::ByteOrder == QSysInfo::BigEndian)
                    || (image_byte_order == MSBFirst && QSysInfo::ByteOrder == QSysInfo::LittleEndian)) {
                while (p < end) {
                    *p = ((*p << 24) & 0xff000000) | ((*p << 8) & 0x00ff0000)
                        | ((*p >> 8) & 0x0000ff00) | ((*p >> 24) & 0x000000ff);
                    p++;
                }
            } else if ((image_byte_order == MSBFirst && QSysInfo::ByteOrder == QSysInfo::BigEndian)
                       || (image_byte_order == LSBFirst && bgr_layout))
            {
                while (p < end) {
                    *p = ((*p << 16) & 0x00ff0000) | ((*p >> 16) & 0x000000ff)
                        | ((*p ) & 0xff00ff00);
                    p++;
                }
            }
            data += iw;
        }
        xi = XCreateImage(dpy, visual, depth, ZPixmap,
                          0, (char *) im.bits(), w, h, 32, im.bytesPerLine());
    } else {
        xi = XCreateImage(dpy, visual, depth, ZPixmap,
                          0, (char *) image.scanLine(rect.y())+rect.x()*sizeof(uint), w, h, 32, image.bytesPerLine());
    }
    XPutImage(dpy, hd, gc, xi, 0, 0, pos.x(), pos.y(), w, h);
    xi->data = 0; // QImage owns these bits
    XDestroyImage(xi);
}

void QX11PaintEngine::drawImage(const QRectF &r, const QImage &image, const QRectF &sr, Qt::ImageConversionFlags flags)
{
    Q_D(QX11PaintEngine);

    if (image.format() == QImage::Format_RGB32
        && d->pdev_depth >= 24 && image.depth() == 32
        && r.size() == sr.size())
    {
        int sx = qRound(sr.x());
        int sy = qRound(sr.y());
        int x = qRound(r.x());
        int y = qRound(r.y());
        int w = qRound(r.width());
        int h = qRound(r.height());

        qt_x11_drawImage(QRect(sx, sy, w, h), QPoint(x, y), image, d->hd, d->gc, d->dpy,
                         (Visual *)d->xinfo->visual(), d->pdev_depth);
    } else {
        QPaintEngine::drawImage(r, image, sr, flags);
    }
}

void QX11PaintEngine::drawPixmap(const QRectF &r, const QPixmap &px, const QRectF &_sr)
{
    Q_D(QX11PaintEngine);
    QRectF sr = _sr;
    int x = qRound(r.x());
    int y = qRound(r.y());
    int sx = qRound(sr.x());
    int sy = qRound(sr.y());
    int sw = qRound(sr.width());
    int sh = qRound(sr.height());

    QPixmap pixmap = qt_toX11Pixmap(px);
    if(pixmap.isNull())
        return;

    if ((d->xinfo && d->xinfo->screen() != pixmap.x11Info().screen())
        || (pixmap.x11Info().screen() != DefaultScreen(X11->display))) {
        QPixmap* p = const_cast<QPixmap *>(&pixmap);
        p->x11SetScreen(d->xinfo ? d->xinfo->screen() : DefaultScreen(X11->display));
    }

    QPixmap::x11SetDefaultScreen(pixmap.x11Info().screen());

#ifndef QT_NO_XRENDER
    ::Picture src_pict = static_cast<QX11PixmapData*>(pixmap.data.data())->picture;
    if (src_pict && d->picture) {
        const int pDepth = pixmap.depth();
        if (pDepth == 1 && (d->has_alpha_pen)) {
            qt_render_bitmap(d->dpy, d->scrn, src_pict, d->picture,
                             sx, sy, x, y, sw, sh, d->cpen);
            return;
        } else if (pDepth != 1 && (pDepth == 32 || pDepth != d->pdev_depth)) {
            XRenderComposite(d->dpy, d->composition_mode,
                             src_pict, 0, d->picture, sx, sy, 0, 0, x, y, sw, sh);
            return;
        }
    }
#endif

    bool mono_src = pixmap.depth() == 1;
    bool mono_dst = d->pdev_depth == 1;
    bool restore_clip = false;

    if (static_cast<QX11PixmapData*>(pixmap.data.data())->x11_mask) { // pixmap has a mask
        QBitmap comb(sw, sh);
        GC cgc = XCreateGC(d->dpy, comb.handle(), 0, 0);
        XSetForeground(d->dpy, cgc, 0);
        XFillRectangle(d->dpy, comb.handle(), cgc, 0, 0, sw, sh);
        XSetBackground(d->dpy, cgc, 0);
        XSetForeground(d->dpy, cgc, 1);
        if (!d->crgn.isEmpty()) {
            int num;
            XRectangle *rects = (XRectangle *)qt_getClipRects(d->crgn, num);
            XSetClipRectangles(d->dpy, cgc, -x, -y, rects, num, Unsorted);
        } else if (d->has_clipping) {
            XSetClipRectangles(d->dpy, cgc, 0, 0, 0, 0, Unsorted);
        }
        XSetFillStyle(d->dpy, cgc, FillOpaqueStippled);
        XSetTSOrigin(d->dpy, cgc, -sx, -sy);
        XSetStipple(d->dpy, cgc,
                    static_cast<QX11PixmapData*>(pixmap.data.data())->x11_mask);
        XFillRectangle(d->dpy, comb.handle(), cgc, 0, 0, sw, sh);
        XFreeGC(d->dpy, cgc);

        XSetClipOrigin(d->dpy, d->gc, x, y);
        XSetClipMask(d->dpy, d->gc, comb.handle());
        restore_clip = true;
    }

    if (mono_src) {
        if (!d->crgn.isEmpty()) {
            Pixmap comb = XCreatePixmap(d->dpy, d->hd, sw, sh, 1);
            GC cgc = XCreateGC(d->dpy, comb, 0, 0);
            XSetForeground(d->dpy, cgc, 0);
            XFillRectangle(d->dpy, comb, cgc, 0, 0, sw, sh);
            int num;
            XRectangle *rects = (XRectangle *)qt_getClipRects(d->crgn, num);
            XSetClipRectangles(d->dpy, cgc, -x, -y, rects, num, Unsorted);
            XCopyArea(d->dpy, pixmap.handle(), comb, cgc, sx, sy, sw, sh, 0, 0);
            XFreeGC(d->dpy, cgc);

            XSetClipMask(d->dpy, d->gc, comb);
            XSetClipOrigin(d->dpy, d->gc, x, y);
            XFreePixmap(d->dpy, comb);
        } else {
            XSetClipMask(d->dpy, d->gc, pixmap.handle());
            XSetClipOrigin(d->dpy, d->gc, x - sx, y - sy);
        }

        if (mono_dst) {
            XSetForeground(d->dpy, d->gc, qGray(d->cpen.color().rgb()) > 127 ? 0 : 1);
        } else {
            QColormap cmap = QColormap::instance(d->scrn);
            XSetForeground(d->dpy, d->gc, cmap.pixel(d->cpen.color()));
        }
        XFillRectangle(d->dpy, d->hd, d->gc, x, y, sw, sh);
        restore_clip = true;
    } else if (mono_dst && !mono_src) {
        QBitmap bitmap(pixmap);
        XCopyArea(d->dpy, bitmap.handle(), d->hd, d->gc, sx, sy, sw, sh, x, y);
    } else {
        XCopyArea(d->dpy, pixmap.handle(), d->hd, d->gc, sx, sy, sw, sh, x, y);
    }

    if (d->pdev->devType() == QInternal::Pixmap) {
        const QPixmap *px = static_cast<const QPixmap*>(d->pdev);
        Pixmap src_mask = static_cast<QX11PixmapData*>(pixmap.data.data())->x11_mask;
        Pixmap dst_mask = static_cast<QX11PixmapData*>(px->data.data())->x11_mask;
        if (dst_mask) {
            GC cgc = XCreateGC(d->dpy, dst_mask, 0, 0);
            if (src_mask) { // copy src mask into dst mask
                XCopyArea(d->dpy, src_mask, dst_mask, cgc, sx, sy, sw, sh, x, y);
            } else { // no src mask, but make sure the area copied is opaque in dest
                XSetBackground(d->dpy, cgc, 0);
                XSetForeground(d->dpy, cgc, 1);
                XFillRectangle(d->dpy, dst_mask, cgc, x, y, sw, sh);
            }
            XFreeGC(d->dpy, cgc);
        }
    }

    if (restore_clip) {
        XSetClipOrigin(d->dpy, d->gc, 0, 0);
        int num;
        XRectangle *rects = (XRectangle *)qt_getClipRects(d->crgn, num);
        if (num == 0)
            XSetClipMask(d->dpy, d->gc, XNone);
        else
            XSetClipRectangles(d->dpy, d->gc, 0, 0, rects, num, Unsorted);
    }
}

void QX11PaintEngine::updateMatrix(const QTransform &mtx)
{
    Q_D(QX11PaintEngine);
    d->txop = mtx.type();
    d->matrix = mtx;

    d->has_complex_xform = (d->txop > QTransform::TxTranslate);

    extern bool qt_scaleForTransform(const QTransform &transform, qreal *scale);
    bool scaling = qt_scaleForTransform(d->matrix, &d->xform_scale);
    d->has_scaling_xform = scaling && d->xform_scale != 1.0;
    d->has_non_scaling_xform = scaling && d->xform_scale == 1.0;
}

/*
   NB! the clip region is expected to be in dev coordinates
*/
void QX11PaintEngine::updateClipRegion_dev(const QRegion &clipRegion, Qt::ClipOperation op)
{
    Q_D(QX11PaintEngine);
    QRegion sysClip = systemClip();
    if (op == Qt::NoClip) {
        d->has_clipping = false;
        d->crgn = sysClip;
        if (!sysClip.isEmpty()) {
            x11SetClipRegion(d->dpy, d->gc, d->gc_brush, d->picture, sysClip);
        } else {
            x11ClearClipRegion(d->dpy, d->gc, d->gc_brush, d->picture);
        }
        return;
    }

    switch (op) {
    case Qt::IntersectClip:
        if (d->has_clipping) {
            d->crgn &= clipRegion;
            break;
        }
        // fall through
    case Qt::ReplaceClip:
        if (!sysClip.isEmpty())
            d->crgn = clipRegion.intersected(sysClip);
        else
            d->crgn = clipRegion;
        break;
    case Qt::UniteClip:
        d->crgn |= clipRegion;
        if (!sysClip.isEmpty())
            d->crgn = d->crgn.intersected(sysClip);
        break;
    default:
        break;
    }
    d->has_clipping = true;
    x11SetClipRegion(d->dpy, d->gc, d->gc_brush, d->picture, d->crgn);
}

void QX11PaintEngine::updateFont(const QFont &)
{
}

Qt::HANDLE QX11PaintEngine::handle() const
{
    Q_D(const QX11PaintEngine);
    Q_ASSERT(isActive());
    Q_ASSERT(d->hd);
    return d->hd;
}

extern void qt_draw_tile(QPaintEngine *, qreal, qreal, qreal, qreal, const QPixmap &,
                         qreal, qreal);

void QX11PaintEngine::drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &p)
{
    int x = qRound(r.x());
    int y = qRound(r.y());
    int w = qRound(r.width());
    int h = qRound(r.height());
    int sx = qRound(p.x());
    int sy = qRound(p.y());

    bool mono_src = pixmap.depth() == 1;
    Q_D(QX11PaintEngine);

    if ((d->xinfo && d->xinfo->screen() != pixmap.x11Info().screen())
        || (pixmap.x11Info().screen() != DefaultScreen(X11->display))) {
        QPixmap* p = const_cast<QPixmap *>(&pixmap);
        p->x11SetScreen(d->xinfo ? d->xinfo->screen() : DefaultScreen(X11->display));
    }

    QPixmap::x11SetDefaultScreen(pixmap.x11Info().screen());

#ifndef QT_NO_XRENDER
    if (X11->use_xrender && d->picture && pixmap.x11PictureHandle()) {
#if 0
        // ### Qt 5: enable this
        XRenderPictureAttributes attrs;
        attrs.repeat = true;
        XRenderChangePicture(d->dpy, pixmap.x11PictureHandle(), CPRepeat, &attrs);

        if (mono_src) {
            qt_render_bitmap(d->dpy, d->scrn, pixmap.x11PictureHandle(), d->picture,
                             sx, sy, x, y, w, h, d->cpen);
        } else {
            XRenderComposite(d->dpy, d->composition_mode,
                             pixmap.x11PictureHandle(), XNone, d->picture,
                             sx, sy, 0, 0, x, y, w, h);
        }
#else
        const int numTiles = (w / pixmap.width()) * (h / pixmap.height());
        if (numTiles < 100) {
            // this is essentially qt_draw_tile(), inlined for
            // the XRenderComposite call
            int yPos, xPos, drawH, drawW, yOff, xOff;
            yPos = y;
            yOff = sy;
            while(yPos < y + h) {
                drawH = pixmap.height() - yOff;    // Cropping first row
                if (yPos + drawH > y + h)        // Cropping last row
                    drawH = y + h - yPos;
                xPos = x;
                xOff = sx;
                while(xPos < x + w) {
                    drawW = pixmap.width() - xOff; // Cropping first column
                    if (xPos + drawW > x + w)    // Cropping last column
                        drawW = x + w - xPos;
                    if (mono_src) {
                        qt_render_bitmap(d->dpy, d->scrn, pixmap.x11PictureHandle(), d->picture,
                                         xOff, yOff, xPos, yPos, drawW, drawH, d->cpen);
                    } else {
                        XRenderComposite(d->dpy, d->composition_mode,
                                         pixmap.x11PictureHandle(), XNone, d->picture,
                                         xOff, yOff, 0, 0, xPos, yPos, drawW, drawH);
                    }
                    xPos += drawW;
                    xOff = 0;
                }
                yPos += drawH;
                yOff = 0;
            }
        } else {
            w = qMin(w, d->pdev->width() - x);
            h = qMin(h, d->pdev->height() - y);
            if (w <= 0 || h <= 0)
                return;

            const int pw = w + sx;
            const int ph = h + sy;
            QPixmap pm(pw, ph);
            if (pixmap.hasAlpha() || mono_src)
                pm.fill(Qt::transparent);

            const int mode = pixmap.hasAlpha() ? PictOpOver : PictOpSrc;
            const ::Picture pmPicture = pm.x11PictureHandle();

            // first tile
            XRenderComposite(d->dpy, mode,
                             pixmap.x11PictureHandle(), XNone, pmPicture,
                             0, 0, 0, 0, 0, 0, qMin(pw, pixmap.width()), qMin(ph, pixmap.height()));

            // first row of tiles
            int xPos = pixmap.width();
            const int sh = qMin(ph, pixmap.height());
            while (xPos < pw) {
                const int sw = qMin(xPos, pw - xPos);
                XRenderComposite(d->dpy, mode,
                                 pmPicture, XNone, pmPicture,
                                 0, 0, 0, 0, xPos, 0, sw, sh);
                xPos *= 2;
            }

            // remaining rows
            int yPos = pixmap.height();
            const int sw = pw;
            while (yPos < ph) {
                const int sh = qMin(yPos, ph - yPos);
                XRenderComposite(d->dpy, mode,
                                 pmPicture, XNone, pmPicture,
                                 0, 0, 0, 0, 0, yPos, sw, sh);
                yPos *= 2;
            }

            // composite
            if (mono_src)
                qt_render_bitmap(d->dpy, d->scrn, pmPicture, d->picture,
                                 sx, sy, x, y, w, h, d->cpen);
            else
                XRenderComposite(d->dpy, d->composition_mode,
                                 pmPicture, XNone, d->picture,
                                 sx, sy, 0, 0, x, y, w, h);
        }
#endif
    } else
#endif // !QT_NO_XRENDER
        if (pixmap.depth() > 1 && !static_cast<QX11PixmapData*>(pixmap.data.data())->x11_mask) {
            XSetTile(d->dpy, d->gc, pixmap.handle());
            XSetFillStyle(d->dpy, d->gc, FillTiled);
            XSetTSOrigin(d->dpy, d->gc, x-sx, y-sy);
            XFillRectangle(d->dpy, d->hd, d->gc, x, y, w, h);
            XSetTSOrigin(d->dpy, d->gc, 0, 0);
            XSetFillStyle(d->dpy, d->gc, FillSolid);
        } else {
            qt_draw_tile(this, x, y, w, h, pixmap, sx, sy);
        }
}

void QX11PaintEngine::drawTextItem(const QPointF &p, const QTextItem &textItem)
{
    const QTextItemInt &ti = static_cast<const QTextItemInt &>(textItem);

    switch(ti.fontEngine->type()) {
    case QFontEngine::TestFontEngine:
    case QFontEngine::Box:
        d_func()->drawBoxTextItem(p, ti);
        break;
    case QFontEngine::XLFD:
        drawXLFD(p, ti);
        break;
#ifndef QT_NO_FONTCONFIG
    case QFontEngine::Freetype:
        drawFreetype(p, ti);
        break;
#endif
    default:
        Q_ASSERT(false);
    }
}

void QX11PaintEngine::drawXLFD(const QPointF &p, const QTextItemInt &ti)
{
    Q_D(QX11PaintEngine);

    if (d->txop > QTransform::TxTranslate) {
        // XServer or font don't support server side transformations, need to do it by hand
        QPaintEngine::drawTextItem(p, ti);
        return;
    }

    if (!ti.glyphs.numGlyphs)
        return;

    QVarLengthArray<QFixedPoint> positions;
    QVarLengthArray<glyph_t> glyphs;
    QTransform matrix = d->matrix;
    matrix.translate(p.x(), p.y());
    ti.fontEngine->getGlyphPositions(ti.glyphs, matrix, ti.flags, glyphs, positions);
    if (glyphs.size() == 0)
        return;

    QFontEngineXLFD *xlfd = static_cast<QFontEngineXLFD *>(ti.fontEngine);
    Qt::HANDLE font_id = xlfd->fontStruct()->fid;

    XSetFont(d->dpy, d->gc, font_id);

    const QFixed offs = QFixed::fromReal(aliasedCoordinateDelta);
    for (int i = 0; i < glyphs.size(); i++) {
        int xp = qRound(positions[i].x + offs);
        int yp = qRound(positions[i].y + offs);
        if (xp < SHRT_MAX && xp > SHRT_MIN &&  yp > SHRT_MIN && yp < SHRT_MAX) {
            XChar2b ch;
            ch.byte1 = glyphs[i] >> 8;
            ch.byte2 = glyphs[i] & 0xff;
            XDrawString16(d->dpy, d->hd, d->gc, xp, yp, &ch, 1);
        }
    }
}

#ifndef QT_NO_FONTCONFIG
static QPainterPath path_for_glyphs(const QVarLengthArray<glyph_t> &glyphs,
                                    const QVarLengthArray<QFixedPoint> &positions,
                                    const QFontEngineFT *ft)
{
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    ft->lockFace();
    int i = 0;
    while (i < glyphs.size()) {
        QFontEngineFT::Glyph *glyph = ft->loadGlyph(glyphs[i], QFontEngineFT::Format_Mono);
        // #### fix case where we don't get a glyph
        if (!glyph)
            break;

        Q_ASSERT(glyph->format == QFontEngineFT::Format_Mono);
        int n = 0;
        int h = glyph->height;
        int xp = qRound(positions[i].x);
        int yp = qRound(positions[i].y);

        xp += glyph->x;
        yp += -glyph->y + glyph->height;
        int pitch = ((glyph->width + 31) & ~31) >> 3;

        uchar *src = glyph->data;
        while (h--) {
            for (int x = 0; x < glyph->width; ++x) {
                bool set = src[x >> 3] & (0x80 >> (x & 7));
                if (set) {
                    QRect r(xp + x, yp - h, 1, 1);
                    while (x+1 < glyph->width && src[(x+1) >> 3] & (0x80 >> ((x+1) & 7))) {
                        ++x;
                        r.setRight(r.right()+1);
                    }

                    path.addRect(r);
                    ++n;
                }
            }
            src += pitch;
        }
        ++i;
    }
    ft->unlockFace();
    return path;
}

void QX11PaintEngine::drawFreetype(const QPointF &p, const QTextItemInt &ti)
{
    Q_D(QX11PaintEngine);
    if (!ti.glyphs.numGlyphs)
        return;

    QFontEngineX11FT *ft = static_cast<QFontEngineX11FT *>(ti.fontEngine);

    if (!d->cpen.isSolid()) {
        QPaintEngine::drawTextItem(p, ti);
        return;
    }

    const bool xrenderPath = (X11->use_xrender
                              && !(d->pdev->devType() == QInternal::Pixmap
                                   && static_cast<const QPixmap *>(d->pdev)->data->pixelType() == QPixmapData::BitmapType));

    QVarLengthArray<QFixedPoint> positions;
    QVarLengthArray<glyph_t> glyphs;
    QTransform matrix;

    if (xrenderPath)
        matrix = d->matrix;
    matrix.translate(p.x(), p.y());
    ft->getGlyphPositions(ti.glyphs, matrix, ti.flags, glyphs, positions);
    if (glyphs.count() == 0)
        return;

#ifndef QT_NO_XRENDER
    QFontEngineFT::QGlyphSet *set = ft->defaultGlyphs();
    if (d->txop >= QTransform::TxScale && xrenderPath)
        set = ft->loadTransformedGlyphSet(d->matrix);

    if (!set || set->outline_drawing
        || !ft->loadGlyphs(set, glyphs.constData(), glyphs.size(), positions.constData(), QFontEngineFT::Format_Render))
    {
        QPaintEngine::drawTextItem(p, ti);
        return;
    }

    if (xrenderPath) {
        GlyphSet glyphSet = set->id;
        const QColor &pen = d->cpen.color();
        ::Picture src = X11->getSolidFill(d->scrn, pen);
        XRenderPictFormat *maskFormat = 0;
        if (ft->xglyph_format != PictStandardA1)
            maskFormat = XRenderFindStandardFormat(X11->display, ft->xglyph_format);

        enum { t_min = SHRT_MIN, t_max = SHRT_MAX };

        int i = 0;
        for (; i < glyphs.size()
                 && (positions[i].x < t_min || positions[i].x > t_max
                     || positions[i].y < t_min || positions[i].y > t_max);
             ++i)
            ;

        if (i >= glyphs.size())
            return;
        ++i;

        QFixed xp = positions[i - 1].x;
        QFixed yp = positions[i - 1].y;
        QFixed offs = QFixed::fromReal(aliasedCoordinateDelta);

        XGlyphElt32 elt;
        elt.glyphset = glyphSet;
        elt.chars = &glyphs[i - 1];
        elt.nchars = 1;
        elt.xOff = qRound(xp + offs);
        elt.yOff = qRound(yp + offs);
        for (; i < glyphs.size(); ++i) {
            if (positions[i].x < t_min || positions[i].x > t_max
                || positions[i].y < t_min || positions[i].y > t_max) {
                break;
            }
            QFontEngineFT::Glyph *g = ft->cachedGlyph(glyphs[i - 1]);
            if (g
                && positions[i].x == xp + g->advance
                && positions[i].y == yp
                && elt.nchars < 253 // don't draw more than 253 characters as some X servers
                                    // hang with it
                ) {
                elt.nchars++;
                xp += g->advance;
            } else {
                xp = positions[i].x;
                yp = positions[i].y;

                XRenderCompositeText32(X11->display, PictOpOver, src, d->picture,
                                       maskFormat, 0, 0, 0, 0,
                                       &elt, 1);
                elt.chars = &glyphs[i];
                elt.nchars = 1;
                elt.xOff = qRound(xp + offs);
                elt.yOff = qRound(yp + offs);
            }
        }
        XRenderCompositeText32(X11->display, PictOpOver, src, d->picture,
                               maskFormat, 0, 0, 0, 0,
                               &elt, 1);

        return;

    }
#endif

    QPainterPath path = path_for_glyphs(glyphs, positions, ft);
    if (path.elementCount() <= 1)
        return;
    Q_ASSERT((path.elementCount() % 5) == 0);
    if (d->txop >= QTransform::TxScale) {
        painter()->save();
        painter()->setBrush(d->cpen.brush());
        painter()->setPen(Qt::NoPen);
        painter()->drawPath(path);
        painter()->restore();
        return;
    }

    const int rectcount = 256;
    XRectangle rects[rectcount];
    int num_rects = 0;

    QPoint delta(qRound(d->matrix.dx()), qRound(d->matrix.dy()));
    QRect clip(d->polygonClipper.boundingRect());
    for (int i=0; i < path.elementCount(); i+=5) {
        int x = qRound(path.elementAt(i).x);
        int y = qRound(path.elementAt(i).y);
        int w = qRound(path.elementAt(i+1).x) - x;
        int h = qRound(path.elementAt(i+2).y) - y;

        QRect rect = QRect(x + delta.x(), y + delta.y(), w, h);
        rect = rect.intersected(clip);
        if (rect.isEmpty())
            continue;

        rects[num_rects].x = short(rect.x());
        rects[num_rects].y = short(rect.y());
        rects[num_rects].width = ushort(rect.width());
        rects[num_rects].height = ushort(rect.height());
        ++num_rects;
        if (num_rects == rectcount) {
            XFillRectangles(d->dpy, d->hd, d->gc, rects, num_rects);
            num_rects = 0;
        }
    }
    if (num_rects > 0)
        XFillRectangles(d->dpy, d->hd, d->gc, rects, num_rects);

}
#endif // !QT_NO_XRENDER

QT_END_NAMESPACE
