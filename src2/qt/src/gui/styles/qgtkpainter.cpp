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

#include "qgtkpainter_p.h"

#include <QtCore/qglobal.h>
#if !defined(QT_NO_STYLE_GTK)

// This class is primarily a wrapper around the gtk painter functions
// and takes care of converting all such calls into cached Qt pixmaps.

#include <private/qstylehelper_p.h>
#include <QtGui/QWidget>
#include <QtGui/QStyleOption>
#include <QtGui/QPixmapCache>

QT_BEGIN_NAMESPACE

#undef GTK_OBJECT_FLAGS
#define GTK_OBJECT_FLAGS(obj)(((GtkObject*)(obj))->flags)

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
#   define QT_RED 3
#   define QT_GREEN 2
#   define QT_BLUE 1
#   define QT_ALPHA 0
#else
#   define QT_RED 0
#   define QT_GREEN 1
#   define QT_BLUE 2
#   define QT_ALPHA 3
#endif
#   define GTK_RED 2
#   define GTK_GREEN 1
#   define GTK_BLUE 0
#   define GTK_ALPHA 3

// To recover alpha we apply the gtk painting function two times to
// white, and black window backgrounds. This can be used to
// recover the premultiplied alpha channel
QPixmap QGtkPainter::renderTheme(uchar *bdata, uchar *wdata, const QRect &rect)
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
#define DRAW_TO_CACHE(draw_func)                                                                   \
    if (rect.width() > QWIDGETSIZE_MAX || rect.height() > QWIDGETSIZE_MAX)                         \
        return;                                                                                    \
    QRect pixmapRect(0, 0, rect.width(), rect.height());                                           \
    {                                                                                              \
        GdkPixmap *pixmap = QGtkStylePrivate::gdk_pixmap_new((GdkDrawable*)(m_window->window),                    \
                                            rect.width(), rect.height(), -1);                      \
        if (!pixmap)                                                                               \
            return;                                                                                \
        style = QGtkStylePrivate::gtk_style_attach (style, m_window->window);                                  \
        QGtkStylePrivate::gdk_draw_rectangle(pixmap, m_alpha ? style->black_gc : *style->bg_gc, true,                \
                           0, 0, rect.width(), rect.height());                                     \
        draw_func;                                                                                 \
        GdkPixbuf *imgb = QGtkStylePrivate::gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, rect.width(), rect.height());\
        if (!imgb)                                                                                 \
            return;                                                                                \
        imgb = QGtkStylePrivate::gdk_pixbuf_get_from_drawable(imgb, pixmap, NULL, 0, 0, 0, 0,                  \
                                            rect.width(), rect.height());                          \
        uchar* bdata = (uchar*)QGtkStylePrivate::gdk_pixbuf_get_pixels(imgb);                                  \
        if (m_alpha) {                                                                             \
            QGtkStylePrivate::gdk_draw_rectangle(pixmap, style->white_gc, true, 0, 0, rect.width(), rect.height());  \
            draw_func;                                                                             \
            GdkPixbuf *imgw = QGtkStylePrivate::gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, rect.              \
                                             width(), rect.height());                              \
            if (!imgw)                                                                             \
                return;                                                                            \
            imgw = QGtkStylePrivate::gdk_pixbuf_get_from_drawable(imgw, pixmap, NULL, 0, 0, 0, 0,              \
                                                rect.width(), rect.height());                      \
            uchar* wdata = (uchar*)QGtkStylePrivate::gdk_pixbuf_get_pixels(imgw);                                    \
            cache = renderTheme(bdata, wdata, rect);                                               \
            QGtkStylePrivate::gdk_pixbuf_unref(imgw);                                                          \
        } else {                                                                                   \
            cache = renderTheme(bdata, 0, rect);                                                   \
        }                                                                                          \
        QGtkStylePrivate::gdk_drawable_unref(pixmap);                                                          \
        QGtkStylePrivate::gdk_pixbuf_unref(imgb);                                                              \
    }

QGtkPainter::QGtkPainter(QPainter *_painter)
        : m_window(QGtkStylePrivate::gtkWidget("GtkWindow"))
        , m_painter(_painter)
        , m_alpha(true)
        , m_hflipped(false)
        , m_vflipped(false)
        , m_usePixmapCache(true)
{}


static QString uniqueName(const QString &key, GtkStateType state, GtkShadowType shadow,
                          const QSize &size, GtkWidget *widget = 0)
{
    // Note the widget arg should ideally use the widget path, though would compromise performance
    QString tmp = key
                  % HexString<uint>(state)
                  % HexString<uint>(shadow)
                  % HexString<uint>(size.width())
                  % HexString<uint>(size.height())
                  % HexString<quint64>(quint64(widget));
    return tmp;
}


GtkStateType QGtkPainter::gtkState(const QStyleOption *option)

{
    GtkStateType state = GTK_STATE_NORMAL;
    if (!(option->state & QStyle::State_Enabled))
        state = GTK_STATE_INSENSITIVE;
    else if (option->state & QStyle::State_MouseOver)
        state = GTK_STATE_PRELIGHT;

    return state;
}


GtkStyle* QGtkPainter::getStyle(GtkWidget *gtkWidget)

{
    Q_ASSERT(gtkWidget);
    GtkStyle* style = gtkWidget->style;
    Q_ASSERT(style);
    return style;
}

QPixmap QGtkPainter::getIcon(const char* iconName, GtkIconSize size)
{
    GtkStyle *style = QGtkStylePrivate::gtkStyle();
    GtkIconSet* iconSet  = QGtkStylePrivate::gtk_icon_factory_lookup_default (iconName);
    GdkPixbuf* icon = QGtkStylePrivate::gtk_icon_set_render_icon(iconSet,
                                                 style,
                                                 GTK_TEXT_DIR_LTR,
                                                 GTK_STATE_NORMAL,
                                                 size,
                                                 NULL,
                                                 "button");
    uchar* data = (uchar*)QGtkStylePrivate::gdk_pixbuf_get_pixels(icon);
    int width = QGtkStylePrivate::gdk_pixbuf_get_width(icon);
    int height = QGtkStylePrivate::gdk_pixbuf_get_height(icon);
    QImage converted(width, height, QImage::Format_ARGB32);
    uchar* tdata = (uchar*)converted.bits();

    for ( int index = 0 ; index < height * width*4 ; index +=4 ) {
        //int index = y * rowstride + x;
        tdata[index + QT_RED] = data[index + GTK_RED];
        tdata[index + QT_GREEN] = data[index + GTK_GREEN];
        tdata[index + QT_BLUE] = data[index + GTK_BLUE];
        tdata[index + QT_ALPHA] = data[index + GTK_ALPHA];
    }

    QGtkStylePrivate::gdk_pixbuf_unref(icon);

    // should we free iconset?
    return QPixmap::fromImage(converted);

}

// Note currently painted without alpha for performance reasons
void QGtkPainter::paintBoxGap(GtkWidget *gtkWidget, const gchar* part,
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
        DRAW_TO_CACHE(QGtkStylePrivate::gtk_paint_box_gap (style,
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

void QGtkPainter::paintBox(GtkWidget *gtkWidget, const gchar* part,
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
        DRAW_TO_CACHE(QGtkStylePrivate::gtk_paint_box (style,
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

void QGtkPainter::paintHline(GtkWidget *gtkWidget, const gchar* part,
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
        DRAW_TO_CACHE(QGtkStylePrivate::gtk_paint_hline (style,
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

void QGtkPainter::paintVline(GtkWidget *gtkWidget, const gchar* part,
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
        DRAW_TO_CACHE(QGtkStylePrivate::gtk_paint_vline (style,
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


void QGtkPainter::paintExpander(GtkWidget *gtkWidget,
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
        DRAW_TO_CACHE(QGtkStylePrivate::gtk_paint_expander (style, pixmap,
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

void QGtkPainter::paintFocus(GtkWidget *gtkWidget, const gchar* part,
                             const QRect &rect, GtkStateType state,
                             GtkStyle *style, const QString &pmKey)
{
    if (!rect.isValid())
        return;

    QPixmap cache;
    QString pixmapName = uniqueName(QLS(part), state, GTK_SHADOW_NONE, rect.size(), gtkWidget) % pmKey;
    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtkStylePrivate::gtk_paint_focus (style, pixmap, state, NULL,
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


void QGtkPainter::paintResizeGrip(GtkWidget *gtkWidget, const gchar* part,
                                  const QRect &rect, GtkStateType state,
                                  GtkShadowType shadow, GdkWindowEdge edge,
                                  GtkStyle *style, const QString &pmKey)
{
    if (!rect.isValid())
        return;

    QPixmap cache;
    QString pixmapName = uniqueName(QLS(part), state, shadow, rect.size(), gtkWidget) % pmKey;
    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtkStylePrivate::gtk_paint_resize_grip (style, pixmap, state,
                                               NULL, gtkWidget,
                                               part, edge, 0, 0,
                                               rect.width(),
                                               rect.height()));
        if (m_usePixmapCache)
            QPixmapCache::insert(pixmapName, cache);
    }

    m_painter->drawPixmap(rect.topLeft(), cache);
}


void QGtkPainter::paintArrow(GtkWidget *gtkWidget, const gchar* part,
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
        DRAW_TO_CACHE(QGtkStylePrivate::gtk_paint_arrow (style, pixmap, state, shadow,
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


void QGtkPainter::paintHandle(GtkWidget *gtkWidget, const gchar* part, const QRect &rect,
                              GtkStateType state, GtkShadowType shadow,
                              GtkOrientation orientation, GtkStyle *style)
{
    if (!rect.isValid())
        return;

    QPixmap cache;
    QString pixmapName = uniqueName(QLS(part), state, shadow, rect.size())
                         % HexString<uchar>(orientation);

    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtkStylePrivate::gtk_paint_handle (style,
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


void QGtkPainter::paintSlider(GtkWidget *gtkWidget, const gchar* part, const QRect &rect,
                              GtkStateType state, GtkShadowType shadow,
                              GtkStyle *style, GtkOrientation orientation,
                              const QString &pmKey)
{
    if (!rect.isValid())
        return;

    QPixmap cache;
    QString pixmapName = uniqueName(QLS(part), state, shadow, rect.size(), gtkWidget) % pmKey;
    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtkStylePrivate::gtk_paint_slider (style,
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


void QGtkPainter::paintShadow(GtkWidget *gtkWidget, const gchar* part,
                              const QRect &rect, GtkStateType state,
                              GtkShadowType shadow, GtkStyle *style,
                              const QString &pmKey)

{
    if (!rect.isValid())
        return;

    QPixmap cache;
    QString pixmapName = uniqueName(QLS(part), state, shadow, rect.size()) % pmKey;
    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtkStylePrivate::gtk_paint_shadow(style, pixmap, state, shadow, NULL,
                                         gtkWidget, part, 0, 0, rect.width(), rect.height()));
        if (m_usePixmapCache)
            QPixmapCache::insert(pixmapName, cache);
    }
    m_painter->drawPixmap(rect.topLeft(), cache);
}

void QGtkPainter::paintFlatBox(GtkWidget *gtkWidget, const gchar* part,
                               const QRect &rect, GtkStateType state,
                               GtkShadowType shadow, GtkStyle *style,
                               const QString &pmKey)
{
    if (!rect.isValid())
        return;
    QPixmap cache;
    QString pixmapName = uniqueName(QLS(part), state, shadow, rect.size()) % pmKey;
    if (!m_usePixmapCache || !QPixmapCache::find(pixmapName, cache)) {
        DRAW_TO_CACHE(QGtkStylePrivate::gtk_paint_flat_box (style,
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

void QGtkPainter::paintExtention(GtkWidget *gtkWidget,
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
        DRAW_TO_CACHE(QGtkStylePrivate::gtk_paint_extension (style, pixmap, state, shadow,
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

void QGtkPainter::paintOption(GtkWidget *gtkWidget, const QRect &radiorect,
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
        DRAW_TO_CACHE(QGtkStylePrivate::gtk_paint_option(style, pixmap,
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

void QGtkPainter::paintCheckbox(GtkWidget *gtkWidget, const QRect &checkrect,
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
        DRAW_TO_CACHE(QGtkStylePrivate::gtk_paint_check (style,
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
