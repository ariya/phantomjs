/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qgtk2painter_p.h"

#include <QtCore/qglobal.h>
#if !defined(QT_NO_STYLE_GTK)

// This class is primarily a wrapper around the gtk painter functions
// and takes care of converting all such calls into cached Qt pixmaps.

#include <private/qgtkstyle_p_p.h>
#include <private/qhexstring_p.h>
#include <QtWidgets/QWidget>
#include <QtGui/QPixmapCache>
#include <QtCore/QLibrary>

QT_BEGIN_NAMESPACE

typedef GdkPixbuf* (*Ptr_gdk_pixbuf_get_from_drawable) (GdkPixbuf *, GdkDrawable *, GdkColormap *, int, int, int, int, int, int);
typedef GdkPixmap* (*Ptr_gdk_pixmap_new) (GdkDrawable *, gint, gint, gint);
typedef void (*Ptr_gdk_draw_rectangle) (GdkDrawable *, GdkGC *, gboolean, gint, gint, gint, gint);
typedef void (*Ptr_gdk_drawable_unref)(GdkDrawable *);

typedef void (*Ptr_gtk_paint_check) (GtkStyle *, GdkWindow *, GtkStateType, GtkShadowType, const GdkRectangle *, GtkWidget *, const gchar *, gint , gint , gint , gint);
typedef void (*Ptr_gtk_paint_box) (GtkStyle *, GdkWindow *, GtkStateType, GtkShadowType, const GdkRectangle *, GtkWidget *, const gchar *, gint , gint , gint , gint);
typedef void (*Ptr_gtk_paint_box_gap) (GtkStyle *, GdkWindow *, GtkStateType, GtkShadowType, const GdkRectangle *, GtkWidget *, const gchar *, gint, gint, gint , gint, GtkPositionType, gint, gint);
typedef void (*Ptr_gtk_paint_resize_grip) (GtkStyle *, GdkWindow *, GtkStateType, const GdkRectangle *, GtkWidget *, const gchar *, GdkWindowEdge, gint , gint , gint , gint);
typedef void (*Ptr_gtk_paint_focus) (GtkStyle *, GdkWindow *, GtkStateType, const GdkRectangle *, GtkWidget *, const gchar *, gint , gint , gint , gint);
typedef void (*Ptr_gtk_paint_shadow) (GtkStyle *, GdkWindow *, GtkStateType, GtkShadowType, const GdkRectangle *, GtkWidget *, const gchar *, gint , gint , gint , gint);
typedef void (*Ptr_gtk_paint_slider) (GtkStyle *, GdkWindow *, GtkStateType, GtkShadowType, const GdkRectangle *, GtkWidget *, const gchar *, gint , gint , gint , gint, GtkOrientation);
typedef void (*Ptr_gtk_paint_expander) (GtkStyle *, GdkWindow *, GtkStateType, const GdkRectangle *, GtkWidget *, const gchar *, gint , gint , GtkExpanderStyle );
typedef void (*Ptr_gtk_paint_handle) (GtkStyle *, GdkWindow *, GtkStateType, GtkShadowType, const GdkRectangle *, GtkWidget *, const gchar *, gint , gint , gint , gint, GtkOrientation);
typedef void (*Ptr_gtk_paint_arrow) (GtkStyle *, GdkWindow *, GtkStateType, GtkShadowType, const GdkRectangle *, GtkWidget *, const gchar *, GtkArrowType, gboolean, gint , gint , gint , gint);
typedef void (*Ptr_gtk_paint_option) (GtkStyle *, GdkWindow *, GtkStateType, GtkShadowType, const GdkRectangle *, GtkWidget *, const gchar *, gint , gint , gint , gint);
typedef void (*Ptr_gtk_paint_flat_box) (GtkStyle *, GdkWindow *, GtkStateType, GtkShadowType, const GdkRectangle *, GtkWidget *, const gchar *, gint , gint , gint , gint);
typedef void (*Ptr_gtk_paint_extension) (GtkStyle *, GdkWindow *, GtkStateType, GtkShadowType, const GdkRectangle *, GtkWidget *, const gchar *, gint, gint, gint, gint, GtkPositionType);
typedef void (*Ptr_gtk_paint_hline) (GtkStyle *, GdkWindow *, GtkStateType, const GdkRectangle *, GtkWidget *, const gchar *, gint, gint, gint y);
typedef void (*Ptr_gtk_paint_vline) (GtkStyle *, GdkWindow *, GtkStateType, const GdkRectangle *, GtkWidget *, const gchar *, gint, gint, gint);

namespace QGtk2PainterPrivate {
    static Ptr_gdk_pixmap_new gdk_pixmap_new = 0;
    static Ptr_gdk_pixbuf_get_from_drawable gdk_pixbuf_get_from_drawable = 0;
    static Ptr_gdk_draw_rectangle gdk_draw_rectangle = 0;
    static Ptr_gdk_drawable_unref gdk_drawable_unref = 0;

    static Ptr_gtk_paint_check gtk_paint_check = 0;
    static Ptr_gtk_paint_box gtk_paint_box = 0;
    static Ptr_gtk_paint_box_gap gtk_paint_box_gap = 0;
    static Ptr_gtk_paint_flat_box gtk_paint_flat_box = 0;
    static Ptr_gtk_paint_option gtk_paint_option = 0;
    static Ptr_gtk_paint_extension gtk_paint_extension = 0;
    static Ptr_gtk_paint_slider gtk_paint_slider = 0;
    static Ptr_gtk_paint_shadow gtk_paint_shadow = 0;
    static Ptr_gtk_paint_resize_grip gtk_paint_resize_grip = 0;
    static Ptr_gtk_paint_focus gtk_paint_focus = 0;
    static Ptr_gtk_paint_arrow gtk_paint_arrow = 0;
    static Ptr_gtk_paint_handle gtk_paint_handle = 0;
    static Ptr_gtk_paint_expander gtk_paint_expander = 0;
    static Ptr_gtk_paint_vline gtk_paint_vline = 0;
    static Ptr_gtk_paint_hline gtk_paint_hline = 0;
}

static void initGtk()
{
    static bool initialized = false;
    if (!initialized) {
        // enforce the "0" suffix, so we'll open libgtk-x11-2.0.so.0
        QLibrary libgtk(QLS("gtk-x11-2.0"), 0, 0);

        QGtk2PainterPrivate::gdk_pixmap_new = (Ptr_gdk_pixmap_new)libgtk.resolve("gdk_pixmap_new");
        QGtk2PainterPrivate::gdk_pixbuf_get_from_drawable = (Ptr_gdk_pixbuf_get_from_drawable)libgtk.resolve("gdk_pixbuf_get_from_drawable");
        QGtk2PainterPrivate::gdk_draw_rectangle = (Ptr_gdk_draw_rectangle)libgtk.resolve("gdk_draw_rectangle");
        QGtk2PainterPrivate::gdk_drawable_unref = (Ptr_gdk_drawable_unref)libgtk.resolve("gdk_drawable_unref");

        QGtk2PainterPrivate::gtk_paint_check = (Ptr_gtk_paint_check)libgtk.resolve("gtk_paint_check");
        QGtk2PainterPrivate::gtk_paint_box = (Ptr_gtk_paint_box)libgtk.resolve("gtk_paint_box");
        QGtk2PainterPrivate::gtk_paint_flat_box = (Ptr_gtk_paint_flat_box)libgtk.resolve("gtk_paint_flat_box");
        QGtk2PainterPrivate::gtk_paint_check = (Ptr_gtk_paint_check)libgtk.resolve("gtk_paint_check");
        QGtk2PainterPrivate::gtk_paint_box = (Ptr_gtk_paint_box)libgtk.resolve("gtk_paint_box");
        QGtk2PainterPrivate::gtk_paint_resize_grip = (Ptr_gtk_paint_resize_grip)libgtk.resolve("gtk_paint_resize_grip");
        QGtk2PainterPrivate::gtk_paint_focus = (Ptr_gtk_paint_focus)libgtk.resolve("gtk_paint_focus");
        QGtk2PainterPrivate::gtk_paint_shadow = (Ptr_gtk_paint_shadow)libgtk.resolve("gtk_paint_shadow");
        QGtk2PainterPrivate::gtk_paint_slider = (Ptr_gtk_paint_slider)libgtk.resolve("gtk_paint_slider");
        QGtk2PainterPrivate::gtk_paint_expander = (Ptr_gtk_paint_expander)libgtk.resolve("gtk_paint_expander");
        QGtk2PainterPrivate::gtk_paint_handle = (Ptr_gtk_paint_handle)libgtk.resolve("gtk_paint_handle");
        QGtk2PainterPrivate::gtk_paint_option = (Ptr_gtk_paint_option)libgtk.resolve("gtk_paint_option");
        QGtk2PainterPrivate::gtk_paint_arrow = (Ptr_gtk_paint_arrow)libgtk.resolve("gtk_paint_arrow");
        QGtk2PainterPrivate::gtk_paint_box_gap = (Ptr_gtk_paint_box_gap)libgtk.resolve("gtk_paint_box_gap");
        QGtk2PainterPrivate::gtk_paint_extension = (Ptr_gtk_paint_extension)libgtk.resolve("gtk_paint_extension");
        QGtk2PainterPrivate::gtk_paint_hline = (Ptr_gtk_paint_hline)libgtk.resolve("gtk_paint_hline");
        QGtk2PainterPrivate::gtk_paint_vline = (Ptr_gtk_paint_vline)libgtk.resolve("gtk_paint_vline");

        initialized = true;
    }
}

// To recover alpha we apply the gtk painting function two times to
// white, and black window backgrounds. This can be used to
// recover the premultiplied alpha channel
QPixmap QGtk2Painter::renderTheme(uchar *bdata, uchar *wdata, const QRect &rect) const
{
    const int bytecount = rect.width() * rect.height() * 4;
    for (int index = 0; index < bytecount ; index += 4) {
        uchar val = bdata[index + GTK_BLUE];
        if (m_alpha) {
            int alphaval = qMax(bdata[index + GTK_BLUE] - wdata[index + GTK_BLUE],
                                bdata[index + GTK_GREEN] - wdata[index + GTK_GREEN]);
            alphaval = qMax(alphaval, bdata[index + GTK_RED] - wdata[index + GTK_RED]) + 255;
            bdata[index + QT_ALPHA] = alphaval;
        }
        bdata[index + QT_RED] = bdata[index + GTK_RED];
        bdata[index + QT_GREEN] = bdata[index + GTK_GREEN];
        bdata[index + QT_BLUE] = val;
    }
    QImage converted((const uchar*)bdata, rect.width(), rect.height(), m_alpha ?
                     QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32);

    if (m_hflipped || m_vflipped) {
        return QPixmap::fromImage(converted.mirrored(m_hflipped, m_vflipped));
    } else {
        // on raster graphicssystem we need to do a copy here, because
        // we intend to deallocate the qimage bits shortly after...
        return QPixmap::fromImage(converted.copy());
    }
}

// This macro is responsible for painting any GtkStyle painting function onto a QPixmap
#define DRAW_TO_CACHE(draw_func)                                                                    \
    if (rect.width() > QWIDGETSIZE_MAX || rect.height() > QWIDGETSIZE_MAX)                          \
        return;                                                                                     \
    QRect pixmapRect(0, 0, rect.width(), rect.height());                                            \
    {                                                                                               \
        GdkPixmap *pixmap = QGtk2PainterPrivate::gdk_pixmap_new((GdkDrawable*)(m_window->window),   \
                                                                rect.width(), rect.height(), -1);   \
        if (!pixmap)                                                                                \
            return;                                                                                 \
        style = QGtkStylePrivate::gtk_style_attach (style, m_window->window);                       \
        QGtk2PainterPrivate::gdk_draw_rectangle(pixmap, m_alpha ? style->black_gc : *style->bg_gc,  \
                                                true, 0, 0, rect.width(), rect.height());           \
        draw_func;                                                                                  \
        GdkPixbuf *imgb = QGtkStylePrivate::gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8,             \
                                                           rect.width(), rect.height());            \
        if (!imgb)                                                                                  \
            return;                                                                                 \
        imgb = QGtk2PainterPrivate::gdk_pixbuf_get_from_drawable(imgb, pixmap, NULL, 0, 0, 0, 0,    \
                                                                 rect.width(), rect.height());      \
        uchar* bdata = (uchar*)QGtkStylePrivate::gdk_pixbuf_get_pixels(imgb);                       \
        if (m_alpha) {                                                                              \
            QGtk2PainterPrivate::gdk_draw_rectangle(pixmap, style->white_gc, true, 0, 0,            \
                                                    rect.width(), rect.height());                   \
            draw_func;                                                                              \
            GdkPixbuf *imgw = QGtkStylePrivate::gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8,         \
                                                               rect.width(), rect.height());        \
            if (!imgw)                                                                              \
                return;                                                                             \
            imgw = QGtk2PainterPrivate::gdk_pixbuf_get_from_drawable(imgw, pixmap, NULL, 0, 0, 0, 0,\
                                                                     rect.width(), rect.height());  \
            uchar* wdata = (uchar*)QGtkStylePrivate::gdk_pixbuf_get_pixels(imgw);                   \
            cache = renderTheme(bdata, wdata, rect);                                                \
            QGtkStylePrivate::gdk_pixbuf_unref(imgw);                                               \
        } else {                                                                                    \
            cache = renderTheme(bdata, 0, rect);                                                    \
        }                                                                                           \
        QGtk2PainterPrivate::gdk_drawable_unref(pixmap);                                            \
        QGtkStylePrivate::gdk_pixbuf_unref(imgb);                                                   \
    }

QGtk2Painter::QGtk2Painter() : QGtkPainter(), m_window(QGtkStylePrivate::gtkWidget("GtkWindow"))
{
    initGtk();
}

// Note currently painted without alpha for performance reasons
void QGtk2Painter::paintBoxGap(GtkWidget *gtkWidget, const gchar* part,
                              const QRect &paintRect, GtkStateType state,
                              GtkShadowType shadow, GtkPositionType gap_side,
                              gint x, gint width,
                              GtkStyle *style)
{
    if (!paintRect.isValid())
        return;

    QPixmap cache;
    QRect rect = paintRect;

    // To avoid exhausting cache on large tabframes we cheat a bit by
    // tiling the center part.

    const int maxHeight = 256;
    const int border = 16;
    if (rect.height() > maxHeight && (gap_side == GTK_POS_TOP || gap_side == GTK_POS_BOTTOM))
        rect.setHeight(2 * border + 1);

    QString pixmapName = uniqueName(QLS(part), state, shadow, rect.size(), gtkWidget)
                         % HexString<uchar>(gap_side)
                         % HexString<gint>(width)
                         % HexString<gint>(x);

    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtk2PainterPrivate::gtk_paint_box_gap (style,
                                           pixmap,
                                           state,
                                           shadow,
                                           NULL,
                                           gtkWidget,
                                           (gchar*)part,
                                           0, 0,
                                           rect.width(),
                                           rect.height(),
                                           gap_side,
                                           x,
                                           width));
        if (m_usePixmapCache)
            QPixmapCache::insert(pixmapName, cache);
    }
    if (rect.size() != paintRect.size()) {
        // We assume we can stretch the middle tab part
        // Note: the side effect of this is that pinstripe patterns will get fuzzy
        const QSize size = cache.size();
        // top part
        m_painter->drawPixmap(QRect(paintRect.left(), paintRect.top(),
                                    paintRect.width(), border), cache,
                             QRect(0, 0, size.width(), border));

        // tiled center part
        QPixmap tilePart(cache.width(), 1);
        QPainter scanLinePainter(&tilePart);
        scanLinePainter.drawPixmap(QRect(0, 0, tilePart.width(), tilePart.height()), cache, QRect(0, border, size.width(), 1));
        scanLinePainter.end();
        m_painter->drawTiledPixmap(QRect(paintRect.left(), paintRect.top() + border,
                                         paintRect.width(), paintRect.height() - 2*border), tilePart);

        // bottom part
        m_painter->drawPixmap(QRect(paintRect.left(), paintRect.top() + paintRect.height() - border,
                                    paintRect.width(), border), cache,
                             QRect(0, size.height() - border, size.width(), border));
    } else
        m_painter->drawPixmap(paintRect.topLeft(), cache);
}

void QGtk2Painter::paintBox(GtkWidget *gtkWidget, const gchar* part,
                           const QRect &paintRect, GtkStateType state,
                           GtkShadowType shadow, GtkStyle *style,
                           const QString &pmKey)
{
    if (!paintRect.isValid())
        return;

    QPixmap cache;
    QRect rect = paintRect;

    // To avoid exhausting cache on large tabframes we cheat a bit by
    // tiling the center part.

    const int maxHeight = 256;
    const int maxArea = 256*512;
    const int border = 32;
    if (rect.height() > maxHeight && (rect.width()*rect.height() > maxArea))
        rect.setHeight(2 * border + 1);

    QString pixmapName = uniqueName(QLS(part), state, shadow,
                                    rect.size(), gtkWidget) % pmKey;

    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtk2PainterPrivate::gtk_paint_box (style,
                                           pixmap,
                                           state,
                                           shadow,
                                           NULL,
                                           gtkWidget,
                                           part,
                                           0, 0,
                                           rect.width(),
                                           rect.height()));
        if (m_usePixmapCache)
            QPixmapCache::insert(pixmapName, cache);
    }
    if (rect.size() != paintRect.size()) {
        // We assume we can stretch the middle tab part
        // Note: the side effect of this is that pinstripe patterns will get fuzzy
        const QSize size = cache.size();
        // top part
        m_painter->drawPixmap(QRect(paintRect.left(), paintRect.top(),
                                    paintRect.width(), border), cache,
                              QRect(0, 0, size.width(), border));

        // tiled center part
        QPixmap tilePart(cache.width(), 1);
        QPainter scanLinePainter(&tilePart);
        scanLinePainter.drawPixmap(QRect(0, 0, tilePart.width(), tilePart.height()), cache, QRect(0, border, size.width(), 1));
        scanLinePainter.end();
        m_painter->drawTiledPixmap(QRect(paintRect.left(), paintRect.top() + border,
                                         paintRect.width(), paintRect.height() - 2*border), tilePart);

        // bottom part
        m_painter->drawPixmap(QRect(paintRect.left(), paintRect.top() + paintRect.height() - border,
                                    paintRect.width(), border), cache,
                              QRect(0, size.height() - border, size.width(), border));
    } else
        m_painter->drawPixmap(paintRect.topLeft(), cache);
}

void QGtk2Painter::paintHline(GtkWidget *gtkWidget, const gchar* part,
                             const QRect &rect, GtkStateType state,
                             GtkStyle *style, int x1, int x2, int y,
                             const QString &pmKey)
{
    if (!rect.isValid())
        return;

    QPixmap cache;
    QString pixmapName = uniqueName(QLS(part), state, GTK_SHADOW_NONE, rect.size(), gtkWidget)
                         % HexString<int>(x1)
                         % HexString<int>(x2)
                         % HexString<int>(y)
                         % pmKey;
    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtk2PainterPrivate::gtk_paint_hline (style,
                                         pixmap,
                                         state,
                                         NULL,
                                         gtkWidget,
                                         part,
                                         x1, x2, y));
        if (m_usePixmapCache)
            QPixmapCache::insert(pixmapName, cache);
    }

    m_painter->drawPixmap(rect.topLeft(), cache);
}

void QGtk2Painter::paintVline(GtkWidget *gtkWidget, const gchar* part,
                             const QRect &rect, GtkStateType state,
                             GtkStyle *style, int y1, int y2, int x,
                             const QString &pmKey)
{
    if (!rect.isValid())
        return;

    QPixmap cache;
    QString pixmapName = uniqueName(QLS(part), state, GTK_SHADOW_NONE, rect.size(), gtkWidget)
                        % HexString<int>(y1)
                        % HexString<int>(y2)
                        % HexString<int>(x)
                        % pmKey;

    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtk2PainterPrivate::gtk_paint_vline (style,
                                         pixmap,
                                         state,
                                         NULL,
                                         gtkWidget,
                                         part,
                                         y1, y2,
                                         x));
        if (m_usePixmapCache)
            QPixmapCache::insert(pixmapName, cache);
    }
    m_painter->drawPixmap(rect.topLeft(), cache);
}


void QGtk2Painter::paintExpander(GtkWidget *gtkWidget,
                                const gchar* part, const QRect &rect,
                                GtkStateType state, GtkExpanderStyle expander_state,
                                GtkStyle *style, const QString &pmKey)
{
    if (!rect.isValid())
        return;

    QPixmap cache;
    QString pixmapName = uniqueName(QLS(part), state, GTK_SHADOW_NONE, rect.size(), gtkWidget)
                         % HexString<uchar>(expander_state)
                         % pmKey;

    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtk2PainterPrivate::gtk_paint_expander (style, pixmap,
                                            state, NULL,
                                            gtkWidget, part,
                                            rect.width()/2,
                                            rect.height()/2,
                                            expander_state));
        if (m_usePixmapCache)
            QPixmapCache::insert(pixmapName, cache);
    }

    m_painter->drawPixmap(rect.topLeft(), cache);
}

void QGtk2Painter::paintFocus(GtkWidget *gtkWidget, const gchar* part,
                             const QRect &rect, GtkStateType state,
                             GtkStyle *style, const QString &pmKey)
{
    if (!rect.isValid())
        return;

    QPixmap cache;
    QString pixmapName = uniqueName(QLS(part), state, GTK_SHADOW_NONE, rect.size(), gtkWidget) % pmKey;
    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtk2PainterPrivate::gtk_paint_focus (style, pixmap, state, NULL,
                                         gtkWidget,
                                         part,
                                         0, 0,
                                         rect.width(),
                                         rect.height()));
        if (m_usePixmapCache)
            QPixmapCache::insert(pixmapName, cache);
    }

    m_painter->drawPixmap(rect.topLeft(), cache);
}


void QGtk2Painter::paintResizeGrip(GtkWidget *gtkWidget, const gchar* part,
                                  const QRect &rect, GtkStateType state,
                                  GtkShadowType shadow, GdkWindowEdge edge,
                                  GtkStyle *style, const QString &pmKey)
{
    if (!rect.isValid())
        return;

    QPixmap cache;
    QString pixmapName = uniqueName(QLS(part), state, shadow, rect.size(), gtkWidget) % pmKey;
    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtk2PainterPrivate::gtk_paint_resize_grip (style, pixmap, state,
                                               NULL, gtkWidget,
                                               part, edge, 0, 0,
                                               rect.width(),
                                               rect.height()));
        if (m_usePixmapCache)
            QPixmapCache::insert(pixmapName, cache);
    }

    m_painter->drawPixmap(rect.topLeft(), cache);
}


void QGtk2Painter::paintArrow(GtkWidget *gtkWidget, const gchar* part,
                             const QRect &arrowrect, GtkArrowType arrow_type,
                             GtkStateType state, GtkShadowType shadow,
                             gboolean fill, GtkStyle *style, const QString &pmKey)
{
    QRect rect = m_cliprect.isValid() ? m_cliprect : arrowrect;
    if (!rect.isValid())
        return;

    QPixmap cache;
    QString pixmapName = uniqueName(QLS(part), state, shadow, rect.size())
                         % HexString<uchar>(arrow_type)
                         % pmKey;

    GdkRectangle gtkCliprect = {0, 0, rect.width(), rect.height()};
    int xOffset = m_cliprect.isValid() ? arrowrect.x() - m_cliprect.x() : 0;
    int yOffset = m_cliprect.isValid() ? arrowrect.y() - m_cliprect.y() : 0;
    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtk2PainterPrivate::gtk_paint_arrow (style, pixmap, state, shadow,
                                         &gtkCliprect,
                                         gtkWidget,
                                         part,
                                         arrow_type, fill,
                                         xOffset, yOffset,
                                         arrowrect.width(),
                                         arrowrect.height()))
        if (m_usePixmapCache)
            QPixmapCache::insert(pixmapName, cache);
    }

    m_painter->drawPixmap(rect.topLeft(), cache);
}


void QGtk2Painter::paintHandle(GtkWidget *gtkWidget, const gchar* part, const QRect &rect,
                              GtkStateType state, GtkShadowType shadow,
                              GtkOrientation orientation, GtkStyle *style)
{
    if (!rect.isValid())
        return;

    QPixmap cache;
    QString pixmapName = uniqueName(QLS(part), state, shadow, rect.size())
                         % HexString<uchar>(orientation);

    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtk2PainterPrivate::gtk_paint_handle (style,
                                          pixmap,
                                          state,
                                          shadow,
                                          NULL,
                                          gtkWidget,
                                          part, 0, 0,
                                          rect.width(),
                                          rect.height(),
                                          orientation));
        if (m_usePixmapCache)
            QPixmapCache::insert(pixmapName, cache);
    }
    m_painter->drawPixmap(rect.topLeft(), cache);
}


void QGtk2Painter::paintSlider(GtkWidget *gtkWidget, const gchar* part, const QRect &rect,
                              GtkStateType state, GtkShadowType shadow,
                              GtkStyle *style, GtkOrientation orientation,
                              const QString &pmKey)
{
    if (!rect.isValid())
        return;

    QPixmap cache;
    QString pixmapName = uniqueName(QLS(part), state, shadow, rect.size(), gtkWidget) % pmKey;
    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtk2PainterPrivate::gtk_paint_slider (style,
                                          pixmap,
                                          state,
                                          shadow,
                                          NULL,
                                          gtkWidget,
                                          part,
                                          0, 0,
                                          rect.width(),
                                          rect.height(),
                                          orientation));
        if (m_usePixmapCache)
            QPixmapCache::insert(pixmapName, cache);
    }
    m_painter->drawPixmap(rect.topLeft(), cache);
}


void QGtk2Painter::paintShadow(GtkWidget *gtkWidget, const gchar* part,
                              const QRect &rect, GtkStateType state,
                              GtkShadowType shadow, GtkStyle *style,
                              const QString &pmKey)

{
    if (!rect.isValid())
        return;

    QPixmap cache;
    QString pixmapName = uniqueName(QLS(part), state, shadow, rect.size()) % pmKey;
    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtk2PainterPrivate::gtk_paint_shadow(style, pixmap, state, shadow, NULL,
                                         gtkWidget, part, 0, 0, rect.width(), rect.height()));
        if (m_usePixmapCache)
            QPixmapCache::insert(pixmapName, cache);
    }
    m_painter->drawPixmap(rect.topLeft(), cache);
}

void QGtk2Painter::paintFlatBox(GtkWidget *gtkWidget, const gchar* part,
                               const QRect &rect, GtkStateType state,
                               GtkShadowType shadow, GtkStyle *style,
                               const QString &pmKey)
{
    if (!rect.isValid())
        return;
    QPixmap cache;
    QString pixmapName = uniqueName(QLS(part), state, shadow, rect.size()) % pmKey;
    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtk2PainterPrivate::gtk_paint_flat_box (style,
                                            pixmap,
                                            state,
                                            shadow,
                                            NULL,
                                            gtkWidget,
                                            part, 0, 0,
                                            rect.width(),
                                            rect.height()));
        if (m_usePixmapCache)
            QPixmapCache::insert(pixmapName, cache);
    }
    m_painter->drawPixmap(rect.topLeft(), cache);
}

void QGtk2Painter::paintExtention(GtkWidget *gtkWidget,
                                 const gchar *part, const QRect &rect,
                                 GtkStateType state, GtkShadowType shadow,
                                 GtkPositionType gap_pos, GtkStyle *style)
{
    if (!rect.isValid())
        return;

    QPixmap cache;
    QString pixmapName = uniqueName(QLS(part), state, shadow, rect.size(), gtkWidget)
                         % HexString<uchar>(gap_pos);

    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtk2PainterPrivate::gtk_paint_extension (style, pixmap, state, shadow,
                                             NULL, gtkWidget,
                                             (gchar*)part, 0, 0,
                                             rect.width(),
                                             rect.height(),
                                             gap_pos));
        if (m_usePixmapCache)
            QPixmapCache::insert(pixmapName, cache);
    }

    m_painter->drawPixmap(rect.topLeft(), cache);
}

void QGtk2Painter::paintOption(GtkWidget *gtkWidget, const QRect &radiorect,
                              GtkStateType state, GtkShadowType shadow,
                              GtkStyle *style, const QString &detail)

{
    QRect rect = m_cliprect.isValid() ? m_cliprect : radiorect;
    if (!rect.isValid())
        return;

    QPixmap cache;
    QString pixmapName = uniqueName(detail, state, shadow, rect.size());
    GdkRectangle gtkCliprect = {0, 0, rect.width(), rect.height()};
    int xOffset = m_cliprect.isValid() ? radiorect.x() - m_cliprect.x() : 0;
    int yOffset = m_cliprect.isValid() ? radiorect.y() - m_cliprect.y() : 0;
    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtk2PainterPrivate::gtk_paint_option(style, pixmap,
                                         state, shadow,
                                         &gtkCliprect,
                                         gtkWidget,
                                         detail.toLatin1(),
                                         xOffset, yOffset,
                                         radiorect.width(),
                                         radiorect.height()));

        if (m_usePixmapCache)
            QPixmapCache::insert(pixmapName, cache);
    }

    m_painter->drawPixmap(rect.topLeft(), cache);
}

void QGtk2Painter::paintCheckbox(GtkWidget *gtkWidget, const QRect &checkrect,
                                GtkStateType state, GtkShadowType shadow,
                                GtkStyle *style, const QString &detail)

{
    QRect rect = m_cliprect.isValid() ? m_cliprect : checkrect;
    if (!rect.isValid())
        return;

    QPixmap cache;
    QString pixmapName = uniqueName(detail, state, shadow, rect.size());
    GdkRectangle gtkCliprect = {0, 0, rect.width(), rect.height()};
    int xOffset = m_cliprect.isValid() ? checkrect.x() - m_cliprect.x() : 0;
    int yOffset = m_cliprect.isValid() ? checkrect.y() - m_cliprect.y() : 0;
    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtk2PainterPrivate::gtk_paint_check (style,
                                         pixmap,
                                         state,
                                         shadow,
                                         &gtkCliprect,
                                         gtkWidget,
                                         detail.toLatin1(),
                                         xOffset, yOffset,
                                         checkrect.width(),
                                         checkrect.height()));
        if (m_usePixmapCache)
            QPixmapCache::insert(pixmapName, cache);
    }

    m_painter->drawPixmap(rect.topLeft(), cache);
}

QT_END_NAMESPACE

#endif //!defined(QT_NO_STYLE_GTK)
