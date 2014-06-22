/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include "qpaintengine.h"
#include "qpaintengine_p.h"
#include "qpainter_p.h"
#include "qpolygon.h"
#include "qbitmap.h"
#include <qdebug.h>
#include <qmath.h>
#include <qguiapplication.h>
#include <private/qtextengine_p.h>
#include <qvarlengtharray.h>
#include <private/qfontengine_p.h>
#include <private/qpaintengineex_p.h>


QT_BEGIN_NAMESPACE

/*!
    \class QTextItem
    \inmodule QtGui

    \brief The QTextItem class provides all the information required to draw
    text in a custom paint engine.

    When you reimplement your own paint engine, you must reimplement
    QPaintEngine::drawTextItem(), a function that takes a QTextItem as
    one of its arguments.
*/

/*!
  \enum QTextItem::RenderFlag

  \value  RightToLeft Render the text from right to left.
  \value  Overline    Paint a line above the text.
  \value  Underline   Paint a line under the text.
  \value  StrikeOut   Paint a line through the text.
  \omitvalue Dummy
*/


/*!
    \fn qreal QTextItem::descent() const

    Corresponds to the \l{QFontMetrics::descent()}{descent} of the piece of text that is drawn.
*/
qreal QTextItem::descent() const
{
    const QTextItemInt *ti = static_cast<const QTextItemInt *>(this);
    return ti->descent.toReal();
}

/*!
    \fn qreal QTextItem::ascent() const

    Corresponds to the \l{QFontMetrics::ascent()}{ascent} of the piece of text that is drawn.
*/
qreal QTextItem::ascent() const
{
    const QTextItemInt *ti = static_cast<const QTextItemInt *>(this);
    return ti->ascent.toReal();
}

/*!
    \fn qreal QTextItem::width() const

    Specifies the total width of the text to be drawn.
*/
qreal QTextItem::width() const
{
    const QTextItemInt *ti = static_cast<const QTextItemInt *>(this);
    return ti->width.toReal();
}

/*!
    \fn QTextItem::RenderFlags QTextItem::renderFlags() const

    Returns the render flags used.
*/
QTextItem::RenderFlags QTextItem::renderFlags() const
{
    const QTextItemInt *ti = static_cast<const QTextItemInt *>(this);
    return ti->flags;
}

/*!
    \fn QString QTextItem::text() const

    Returns the text that should be drawn.
*/
QString QTextItem::text() const
{
    const QTextItemInt *ti = static_cast<const QTextItemInt *>(this);
    return QString(ti->chars, ti->num_chars);
}

/*!
    \fn QFont QTextItem::font() const

    Returns the font that should be used to draw the text.
*/
QFont QTextItem::font() const
{
    const QTextItemInt *ti = static_cast<const QTextItemInt *>(this);
    return ti->f ? *ti->f : QGuiApplication::font();
}


/*!
  \class QPaintEngine
  \ingroup painting
    \inmodule QtGui

  \brief The QPaintEngine class provides an abstract definition of how
  QPainter draws to a given device on a given platform.

  Qt provides several premade implementations of QPaintEngine for the
  different painter backends we support. The primary paint engine
  provided is the raster paint engine, which contains a software
  rasterizer which supports the full feature set on all supported platforms.
  This is the default for painting on QWidget-based classes in e.g. on Windows,
  X11 and Mac OS X, it is the backend for painting on QImage and it is
  used as a fallback for paint engines that do not support a certain
  capability. In addition we provide QPaintEngine implementations for
  OpenGL (accessible through QGLWidget) and printing (which allows using
  QPainter to draw on a QPrinter object).

  If one wants to use QPainter to draw to a different backend,
  one must subclass QPaintEngine and reimplement all its virtual
  functions. The QPaintEngine implementation is then made available by
  subclassing QPaintDevice and reimplementing the virtual function
  QPaintDevice::paintEngine().

  QPaintEngine is created and owned by the QPaintDevice that created it.

  \sa QPainter, QPaintDevice::paintEngine(), {Paint System}
*/

/*!
  \enum QPaintEngine::PaintEngineFeature

  This enum is used to describe the features or capabilities that the
  paint engine has. If a feature is not supported by the engine,
  QPainter will do a best effort to emulate that feature through other
  means and pass on an alpha blended QImage to the engine with the
  emulated results. Some features cannot be emulated: AlphaBlend and PorterDuff.

  \value AlphaBlend         The engine can alpha blend primitives.
  \value Antialiasing       The engine can use antialising to improve the appearance
                            of rendered primitives.
  \value BlendModes         The engine supports blending modes.
  \value BrushStroke        The engine supports drawing strokes that
                            contain brushes as fills, not just solid
                            colors (e.g. a dashed gradient line of
                            width 2).
  \value ConicalGradientFill The engine supports conical gradient fills.
  \value ConstantOpacity    The engine supports the feature provided by
                            QPainter::setOpacity().
  \value LinearGradientFill The engine supports linear gradient fills.
  \value MaskedBrush        The engine is capable of rendering brushes that has a
                            texture with an alpha channel or a mask.
  \value ObjectBoundingModeGradients The engine has native support for gradients
                            with coordinate mode QGradient::ObjectBoundingMode.
                            Otherwise, if QPaintEngine::PatternTransform is
                            supported, object bounding mode gradients are
                            converted to gradients with coordinate mode
                            QGradient::LogicalMode and a brush transform for
                            the coordinate mapping.
  \value PainterPaths       The engine has path support.
  \value PaintOutsidePaintEvent The engine is capable of painting outside of
                                paint events.
  \value PatternBrush       The engine is capable of rendering brushes with
                            the brush patterns specified in Qt::BrushStyle.
  \value PatternTransform   The engine has support for transforming brush
                            patterns.
  \value PerspectiveTransform The engine has support for performing perspective
                            transformations on primitives.
  \value PixmapTransform    The engine can transform pixmaps, including
                            rotation and shearing.
  \value PorterDuff         The engine supports Porter-Duff operations
  \value PrimitiveTransform The engine has support for transforming
                            drawing primitives.
  \value RadialGradientFill The engine supports radial gradient fills.
  \value RasterOpModes      The engine supports bitwise raster operations.
  \value AllFeatures        All of the above features. This enum value is usually
                            used as a bit mask.
*/

/*!
    \enum QPaintEngine::PolygonDrawMode

    \value OddEvenMode The polygon should be drawn using OddEven fill
    rule.

    \value WindingMode The polygon should be drawn using Winding fill rule.

    \value ConvexMode The polygon is a convex polygon and can be drawn
    using specialized algorithms where available.

    \value PolylineMode Only the outline of the polygon should be
    drawn.

*/

/*!
    \enum QPaintEngine::DirtyFlag

    \value DirtyPen The pen is dirty and needs to be updated.

    \value DirtyBrush The brush is dirty and needs to be updated.

    \value DirtyBrushOrigin The brush origin is dirty and needs to
    updated.

    \value DirtyFont The font is dirty and needs to be updated.

    \value DirtyBackground The background is dirty and needs to be
    updated.

    \value DirtyBackgroundMode The background mode is dirty and needs
    to be updated.

    \value DirtyTransform The transform is dirty and needs to be
    updated.

    \value DirtyClipRegion The clip region is dirty and needs to be
    updated.

    \value DirtyClipPath The clip path is dirty and needs to be
    updated.

    \value DirtyHints The render hints is dirty and needs to be
    updated.

    \value DirtyCompositionMode The composition mode is dirty and
    needs to be updated.

    \value DirtyClipEnabled Whether clipping is enabled or not is
    dirty and needs to be updated.

    \value DirtyOpacity The constant opacity has changed and needs to
                        be updated as part of the state change in
                        QPaintEngine::updateState().

    \value AllDirty Convenience enum used internally.

    These types are used by QPainter to trigger lazy updates of the
    various states in the QPaintEngine using
    QPaintEngine::updateState().

    A paint engine must update every dirty state.
*/

/*!
    \fn void QPaintEngine::syncState()

    \internal

    Updates all dirty states in this engine. This function should ONLY
    be used when drawing with native handles directly and immediate sync
    from QPainters state to the native state is required.
*/
void QPaintEngine::syncState()
{
    Q_ASSERT(state);
    updateState(*state);

    if (isExtended())
        static_cast<QPaintEngineEx *>(this)->sync();
}

static QPaintEngine *qt_polygon_recursion = 0;
struct QT_Point {
    int x;
    int y;
};

/*!
    \fn void QPaintEngine::drawPolygon(const QPointF *points, int pointCount,
    PolygonDrawMode mode)

    Reimplement this virtual function to draw the polygon defined
    by the \a pointCount first points in \a points, using mode \a
    mode.

    \note At least one of the drawPolygon() functions must be reimplemented.
*/
void QPaintEngine::drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode)
{
    Q_ASSERT_X(qt_polygon_recursion != this, "QPaintEngine::drawPolygon",
               "At least one drawPolygon function must be implemented");
    qt_polygon_recursion = this;
    Q_ASSERT(sizeof(QT_Point) == sizeof(QPoint));
    QVarLengthArray<QT_Point> p(pointCount);
    for (int i = 0; i < pointCount; ++i) {
        p[i].x = qRound(points[i].x());
        p[i].y = qRound(points[i].y());
    }
    drawPolygon((QPoint *)p.data(), pointCount, mode);
    qt_polygon_recursion = 0;
}

struct QT_PointF {
    qreal x;
    qreal y;
};
/*!
    \overload

    Reimplement this virtual function to draw the polygon defined by the
    \a pointCount first points in \a points, using mode \a mode.

    \note At least one of the drawPolygon() functions must be reimplemented.
*/
void QPaintEngine::drawPolygon(const QPoint *points, int pointCount, PolygonDrawMode mode)
{
    Q_ASSERT_X(qt_polygon_recursion != this, "QPaintEngine::drawPolygon",
               "At least one drawPolygon function must be implemented");
    qt_polygon_recursion = this;
    Q_ASSERT(sizeof(QT_PointF) == sizeof(QPointF));
    QVarLengthArray<QT_PointF> p(pointCount);
    for (int i=0; i<pointCount; ++i) {
        p[i].x = points[i].x();
        p[i].y = points[i].y();
    }
    drawPolygon((QPointF *)p.data(), pointCount, mode);
    qt_polygon_recursion = 0;
}

/*!
    \enum QPaintEngine::Type

    \value X11
    \value Windows
    \value MacPrinter
    \value CoreGraphics Mac OS X's Quartz2D (CoreGraphics)
    \value QuickDraw Mac OS X's QuickDraw
    \value QWindowSystem Qt for Embedded Linux
    \value PostScript (No longer supported)
    \value OpenGL
    \value Picture QPicture format
    \value SVG Scalable Vector Graphics XML format
    \value Raster
    \value Direct3D Windows only, Direct3D based engine
    \value Pdf Portable Document Format
    \value OpenVG
    \value User First user type ID
    \value MaxUser Last user type ID
    \value OpenGL2
    \value PaintBuffer
    \value Blitter
    \value Direct2D Windows only, Direct2D based engine
*/

/*!
    \fn bool QPaintEngine::isActive() const

    Returns \c true if the paint engine is actively drawing; otherwise
    returns \c false.

    \sa setActive()
*/

/*!
    \fn void QPaintEngine::setActive(bool state)

    Sets the active state of the paint engine to \a state.

    \sa isActive()
*/

/*!
    \fn bool QPaintEngine::begin(QPaintDevice *pdev)

    Reimplement this function to initialise your paint engine when
    painting is to start on the paint device \a pdev. Return true if
    the initialization was successful; otherwise return false.

    \sa end(), isActive()
*/

/*!
    \fn bool QPaintEngine::end()

    Reimplement this function to finish painting on the current paint
    device. Return true if painting was finished successfully;
    otherwise return false.

    \sa begin(), isActive()
*/


/*!
    Draws the first \a pointCount points in the buffer \a points
*/
void QPaintEngine::drawPoints(const QPointF *points, int pointCount)
{
    QPainter *p = painter();
    if (!p)
        return;

    qreal penWidth = p->pen().widthF();
    if (penWidth == 0)
        penWidth = 1;

    bool ellipses = p->pen().capStyle() == Qt::RoundCap;

    p->save();

    QTransform transform;
    if (qt_pen_is_cosmetic(p->pen(), p->renderHints())) {
        transform = p->transform();
        p->setTransform(QTransform());
    }

    p->setBrush(p->pen().brush());
    p->setPen(Qt::NoPen);

    for (int i=0; i<pointCount; ++i) {
        QPointF pos = transform.map(points[i]);
        QRectF rect(pos.x() - penWidth / 2, pos.y() - penWidth / 2, penWidth, penWidth);

        if (ellipses)
            p->drawEllipse(rect);
        else
            p->drawRect(rect);
    }

    p->restore();
}


/*!
    Draws the first \a pointCount points in the buffer \a points

    The default implementation converts the first \a pointCount QPoints in \a points
    to QPointFs and calls the floating point version of drawPoints.

*/
void QPaintEngine::drawPoints(const QPoint *points, int pointCount)
{
    Q_ASSERT(sizeof(QT_PointF) == sizeof(QPointF));
    QT_PointF fp[256];
    while (pointCount) {
        int i = 0;
        while (i < pointCount && i < 256) {
            fp[i].x = points[i].x();
            fp[i].y = points[i].y();
            ++i;
        }
        drawPoints((QPointF *)(void *)fp, i);
        points += i;
        pointCount -= i;
    }
}

/*!
    \fn void QPaintEngine::drawEllipse(const QRectF &rect)

    Reimplement this function to draw the largest ellipse that can be
    contained within rectangle \a rect.

    The default implementation calls drawPolygon().
*/
void QPaintEngine::drawEllipse(const QRectF &rect)
{
    QPainterPath path;
    path.addEllipse(rect);
    if (hasFeature(PainterPaths)) {
        drawPath(path);
    } else {
        QPolygonF polygon = path.toFillPolygon();
        drawPolygon(polygon.data(), polygon.size(), ConvexMode);
    }
}

/*!
    The default implementation of this function calls the floating
    point version of this function
*/
void QPaintEngine::drawEllipse(const QRect &rect)
{
    drawEllipse(QRectF(rect));
}

/*!
    \fn void QPaintEngine::drawPixmap(const QRectF &r, const QPixmap
    &pm, const QRectF &sr)

    Reimplement this function to draw the part of the \a pm
    specified by the \a sr rectangle in the given \a r.
*/


void qt_fill_tile(QPixmap *tile, const QPixmap &pixmap)
{
    QPainter p(tile);
    p.drawPixmap(0, 0, pixmap);
    int x = pixmap.width();
    while (x < tile->width()) {
        p.drawPixmap(x, 0, *tile, 0, 0, x, pixmap.height());
        x *= 2;
    }
    int y = pixmap.height();
    while (y < tile->height()) {
        p.drawPixmap(0, y, *tile, 0, 0, tile->width(), y);
        y *= 2;
    }
}

void qt_draw_tile(QPaintEngine *gc, qreal x, qreal y, qreal w, qreal h,
                  const QPixmap &pixmap, qreal xOffset, qreal yOffset)
{
    qreal yPos, xPos, drawH, drawW, yOff, xOff;
    yPos = y;
    yOff = yOffset;
    while(yPos < y + h) {
        drawH = pixmap.height() - yOff;    // Cropping first row
        if (yPos + drawH > y + h)           // Cropping last row
            drawH = y + h - yPos;
        xPos = x;
        xOff = xOffset;
        while(xPos < x + w) {
            drawW = pixmap.width() - xOff; // Cropping first column
            if (xPos + drawW > x + w)           // Cropping last column
                drawW = x + w - xPos;
            if (drawW > 0 && drawH > 0)
                gc->drawPixmap(QRectF(xPos, yPos, drawW, drawH), pixmap, QRectF(xOff, yOff, drawW, drawH));
            xPos += drawW;
            xOff = 0;
        }
        yPos += drawH;
        yOff = 0;
    }
}


/*!
    Reimplement this function to draw the \a pixmap in the given \a
    rect, starting at the given \a p. The pixmap will be
    drawn repeatedly until the \a rect is filled.
*/
void QPaintEngine::drawTiledPixmap(const QRectF &rect, const QPixmap &pixmap, const QPointF &p)
{
    int sw = pixmap.width();
    int sh = pixmap.height();

    if (sw*sh < 8192 && sw*sh < 16*rect.width()*rect.height()) {
        int tw = sw, th = sh;
        while (tw*th < 32678 && tw < rect.width()/2)
            tw *= 2;
        while (tw*th < 32678 && th < rect.height()/2)
            th *= 2;
        QPixmap tile;
        if (pixmap.depth() == 1) {
            tile = QBitmap(tw, th);
        } else {
            tile = QPixmap(tw, th);
            if (pixmap.hasAlphaChannel())
                tile.fill(Qt::transparent);
        }
        qt_fill_tile(&tile, pixmap);
        qt_draw_tile(this, rect.x(), rect.y(), rect.width(), rect.height(), tile, p.x(), p.y());
    } else {
        qt_draw_tile(this, rect.x(), rect.y(), rect.width(), rect.height(), pixmap, p.x(), p.y());
    }
}

/*!
    \fn void QPaintEngine::drawImage(const QRectF &rectangle, const QImage
    &image, const QRectF &sr, Qt::ImageConversionFlags flags)

    Reimplement this function to draw the part of the \a image
    specified by the \a sr rectangle in the given \a rectangle using
    the given conversion flags \a flags, to convert it to a pixmap.
*/

void QPaintEngine::drawImage(const QRectF &r, const QImage &image, const QRectF &sr,
                             Qt::ImageConversionFlags flags)
{
    QRectF baseSize(0, 0, image.width(), image.height());
    QImage im = image;
    if (baseSize != sr)
        im = im.copy(qFloor(sr.x()), qFloor(sr.y()),
                     qCeil(sr.width()), qCeil(sr.height()));
    QPixmap pm = QPixmap::fromImage(im, flags);
    drawPixmap(r, pm, QRectF(QPointF(0, 0), pm.size()));
}

/*!
    \fn Type QPaintEngine::type() const

    Reimplement this function to return the paint engine \l{Type}.
*/

/*!
    \fn void QPaintEngine::fix_neg_rect(int *x, int *y, int *w, int *h);

    \internal
*/

/*!
    \fn bool QPaintEngine::testDirty(DirtyFlags df)

    \internal
*/

/*!
    \fn void QPaintEngine::clearDirty(DirtyFlags df)

    \internal
*/

/*!
    \fn void QPaintEngine::setDirty(DirtyFlags df)

    \internal
*/

/*!
    \fn bool QPaintEngine::hasFeature(PaintEngineFeatures feature) const

    Returns \c true if the paint engine supports the specified \a
    feature; otherwise returns \c false.
*/

/*!
    \fn bool QPaintEngine::isExtended() const

    \internal

    Returns \c true if the paint engine is a QPaintEngineEx derivative.
*/

/*!
    \fn void QPaintEngine::updateState(const QPaintEngineState &state)

    Reimplement this function to update the state of a paint engine.

    When implemented, this function is responsible for checking the
    paint engine's current \a state and update the properties that are
    changed. Use the QPaintEngineState::state() function to find out
    which properties that must be updated, then use the corresponding
    \l {GetFunction}{get function} to retrieve the current values for
    the given properties.

    \sa QPaintEngineState
*/

/*!
    Creates a paint engine with the featureset specified by \a caps.
*/

QPaintEngine::QPaintEngine(PaintEngineFeatures caps)
    : state(0),
      gccaps(caps),
      active(0),
      selfDestruct(false),
      extended(false),
      d_ptr(new QPaintEnginePrivate)
{
    d_ptr->q_ptr = this;
}

/*!
  \internal
*/

QPaintEngine::QPaintEngine(QPaintEnginePrivate &dptr, PaintEngineFeatures caps)
    : state(0),
      gccaps(caps),
      active(0),
      selfDestruct(false),
      extended(false),
      d_ptr(&dptr)
{
    d_ptr->q_ptr = this;
}

/*!
    Destroys the paint engine.
*/
QPaintEngine::~QPaintEngine()
{
}

/*!
    Returns the paint engine's painter.
*/
QPainter *QPaintEngine::painter() const
{
    return state ? state->painter() : 0;
}

/*!
    The default implementation ignores the \a path and does nothing.
*/

void QPaintEngine::drawPath(const QPainterPath &)
{
    if (hasFeature(PainterPaths)) {
        qWarning("QPaintEngine::drawPath: Must be implemented when feature PainterPaths is set");
    }
}

/*!
    This function draws the text item \a textItem at position \a p. The
    default implementation of this function converts the text to a
    QPainterPath and paints the resulting path.
*/

void QPaintEngine::drawTextItem(const QPointF &p, const QTextItem &textItem)
{
    const QTextItemInt &ti = static_cast<const QTextItemInt &>(textItem);

    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    if (ti.glyphs.numGlyphs)
        ti.fontEngine->addOutlineToPath(0, 0, ti.glyphs, &path, ti.flags);
    if (!path.isEmpty()) {
        painter()->save();
        painter()->setRenderHint(QPainter::Antialiasing,
                                 bool((painter()->renderHints() & QPainter::TextAntialiasing)
                                      && !(painter()->font().styleStrategy() & QFont::NoAntialias)));
        painter()->translate(p.x(), p.y());
        painter()->fillPath(path, state->pen().brush());
        painter()->restore();
    }
}

/*!
    The default implementation splits the list of lines in \a lines
    into \a lineCount separate calls to drawPath() or drawPolygon()
    depending on the feature set of the paint engine.
*/
void QPaintEngine::drawLines(const QLineF *lines, int lineCount)
{
    for (int i=0; i<lineCount; ++i) {
        QPointF pts[2] = { lines[i].p1(), lines[i].p2() };

        if (pts[0] == pts[1]) {
            if (state->pen().capStyle() != Qt::FlatCap)
                drawPoints(pts, 1);
            continue;
        }

        drawPolygon(pts, 2, PolylineMode);
    }
}

/*!
    \overload

    The default implementation converts the first \a lineCount lines
    in \a lines to a QLineF and calls the floating point version of
    this function.
*/
void QPaintEngine::drawLines(const QLine *lines, int lineCount)
{
    struct PointF {
        qreal x;
        qreal y;
    };
    struct LineF {
        PointF p1;
        PointF p2;
    };
    Q_ASSERT(sizeof(PointF) == sizeof(QPointF));
    Q_ASSERT(sizeof(LineF) == sizeof(QLineF));
    LineF fl[256];
    while (lineCount) {
        int i = 0;
        while (i < lineCount && i < 256) {
            fl[i].p1.x = lines[i].x1();
            fl[i].p1.y = lines[i].y1();
            fl[i].p2.x = lines[i].x2();
            fl[i].p2.y = lines[i].y2();
            ++i;
        }
        drawLines((QLineF *)(void *)fl, i);
        lines += i;
        lineCount -= i;
    }
}


/*!
    \overload

    The default implementation converts the first \a rectCount
    rectangles in the buffer \a rects to a QRectF and calls the
    floating point version of this function.
*/
void QPaintEngine::drawRects(const QRect *rects, int rectCount)
{
    struct RectF {
        qreal x;
        qreal y;
        qreal w;
        qreal h;
    };
    Q_ASSERT(sizeof(RectF) == sizeof(QRectF));
    RectF fr[256];
    while (rectCount) {
        int i = 0;
        while (i < rectCount && i < 256) {
            fr[i].x = rects[i].x();
            fr[i].y = rects[i].y();
            fr[i].w = rects[i].width();
            fr[i].h = rects[i].height();
            ++i;
        }
        drawRects((QRectF *)(void *)fr, i);
        rects += i;
        rectCount -= i;
    }
}

/*!
    Draws the first \a rectCount rectangles in the buffer \a
    rects. The default implementation of this function calls drawPath()
    or drawPolygon() depending on the feature set of the paint engine.
*/
void QPaintEngine::drawRects(const QRectF *rects, int rectCount)
{
    if (hasFeature(PainterPaths) &&
        !state->penNeedsResolving() &&
        !state->brushNeedsResolving()) {
        for (int i=0; i<rectCount; ++i) {
            QPainterPath path;
            path.addRect(rects[i]);
            if (path.isEmpty())
                continue;
            drawPath(path);
        }
    } else {
        for (int i=0; i<rectCount; ++i) {
            QRectF rf = rects[i];
            QPointF pts[4] = { QPointF(rf.x(), rf.y()),
                               QPointF(rf.x() + rf.width(), rf.y()),
                               QPointF(rf.x() + rf.width(), rf.y() + rf.height()),
                               QPointF(rf.x(), rf.y() + rf.height()) };
            drawPolygon(pts, 4, ConvexMode);
        }
    }
}

/*!
    \internal
    Sets the paintdevice that this engine operates on to \a device
*/
void QPaintEngine::setPaintDevice(QPaintDevice *device)
{
    d_func()->pdev = device;
}

/*!
    Returns the device that this engine is painting on, if painting is
    active; otherwise returns 0.
*/
QPaintDevice *QPaintEngine::paintDevice() const
{
    return d_func()->pdev;
}


/*!
    \internal

    Returns the offset from the painters origo to the engines
    origo. This value is used by QPainter for engines who have
    internal double buffering.

    This function only makes sense when the engine is active.
*/
QPoint QPaintEngine::coordinateOffset() const
{
    return QPoint();
}

/*!
    \internal

    Sets the system clip for this engine. The system clip defines the
    basis area that the engine has to draw in. All clips that are
    set will be an intersection with the system clip.

    Reset the systemclip to no clip by setting an empty region.
*/
void QPaintEngine::setSystemClip(const QRegion &region)
{
    Q_D(QPaintEngine);
    d->systemClip = region;
    // Be backward compatible and only call d->systemStateChanged()
    // if we currently have a system transform/viewport set.
    if (d->hasSystemTransform || d->hasSystemViewport) {
        d->transformSystemClip();
        d->systemStateChanged();
    }
}

/*!
    \internal

    Returns the system clip. The system clip is read only while the
    painter is active. An empty region indicates that system clip
    is not in use.
*/

QRegion QPaintEngine::systemClip() const
{
    return d_func()->systemClip;
}

/*!
    \internal

    Sets the target rect for drawing within the backing store. This
    function should ONLY be used by the backing store.
*/
void QPaintEngine::setSystemRect(const QRect &rect)
{
    if (isActive()) {
        qWarning("QPaintEngine::setSystemRect: Should not be changed while engine is active");
        return;
    }
    d_func()->systemRect = rect;
}

/*!
    \internal

    Retrieves the rect for drawing within the backing store. This
    function should ONLY be used by the backing store.
 */
QRect QPaintEngine::systemRect() const
{
    return d_func()->systemRect;
}

void QPaintEnginePrivate::drawBoxTextItem(const QPointF &p, const QTextItemInt &ti)
{
    if (!ti.glyphs.numGlyphs)
        return;

    // any fixes here should probably also be done in QFontEngineBox::draw
    const int size = qRound(ti.fontEngine->ascent());
    QVarLengthArray<QFixedPoint> positions;
    QVarLengthArray<glyph_t> glyphs;
    QTransform matrix = QTransform::fromTranslate(p.x(), p.y() - size);
    ti.fontEngine->getGlyphPositions(ti.glyphs, matrix, ti.flags, glyphs, positions);
    if (glyphs.size() == 0)
        return;

    QSize s(size - 3, size - 3);

    QPainter *painter = q_func()->state->painter();
    painter->save();
    painter->setBrush(Qt::NoBrush);
    QPen pen = painter->pen();
    pen.setWidthF(ti.fontEngine->lineThickness().toReal());
    painter->setPen(pen);
    for (int k = 0; k < positions.size(); k++)
        painter->drawRect(QRectF(positions[k].toPointF(), s));
    painter->restore();
}

QT_END_NAMESPACE
