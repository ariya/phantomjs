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

#include "qcolormap.h"

#include "qapplication.h"
#include "qdebug.h"
#include "qdesktopwidget.h"
#include "qvarlengtharray.h"

#include "qx11info_x11.h"
#include <private/qt_x11_p.h>
#include <limits.h>

QT_BEGIN_NAMESPACE

class QColormapPrivate
{
public:
    QColormapPrivate()
        : ref(1), mode(QColormap::Direct), depth(0),
          colormap(0), defaultColormap(true),
          visual(0), defaultVisual(true),
          r_max(0), g_max(0), b_max(0),
          r_shift(0), g_shift(0), b_shift(0)
    {}

    QAtomicInt ref;

    QColormap::Mode mode;
    int depth;

    Colormap colormap;
    bool defaultColormap;

    Visual *visual;
    bool defaultVisual;

    int r_max;
    int g_max;
    int b_max;

    uint r_shift;
    uint g_shift;
    uint b_shift;

    QVector<QColor> colors;
    QVector<int> pixels;
};


static uint right_align(uint v)
{
    while (!(v & 0x1))
        v >>= 1;
    return v;
}

static int lowest_bit(uint v)
{
    int i;
    uint b = 1u;
    for (i = 0; ((v & b) == 0u) && i < 32;  ++i)
        b <<= 1u;
    return i == 32 ? -1 : i;
}

static int cube_root(int v)
{
    if (v == 1)
        return 1;
    // brute force algorithm
    int i = 1;
    for (;;) {
        const int b = i * i * i;
        if (b <= v) {
            ++i;
        } else {
            --i;
            break;
        }
    }
    return i;
}

static Visual *find_visual(Display *display,
                           int screen,
                           int visual_class,
                           int visual_id,
                           int *depth,
                           bool *defaultVisual)
{
    XVisualInfo *vi, rvi;
    int count;

    uint mask = VisualScreenMask;
    rvi.screen = screen;

    if (visual_class != -1) {
        rvi.c_class = visual_class;
        mask |= VisualClassMask;
    }
    if (visual_id != -1) {
        rvi.visualid = visual_id;
        mask |= VisualIDMask;
    }

    Visual *visual = DefaultVisual(display, screen);
    *defaultVisual = true;
    *depth = DefaultDepth(display, screen);

    vi = XGetVisualInfo(display, mask, &rvi, &count);
    if (vi) {
        int best = 0;
        for (int x = 0; x < count; ++x) {
            if (vi[x].depth > vi[best].depth)
                best = x;
        }
        if (best >= 0 && best <= count && vi[best].visualid != XVisualIDFromVisual(visual)) {
            visual = vi[best].visual;
            *defaultVisual = (visual == DefaultVisual(display, screen));
            *depth = vi[best].depth;
        }
    }
    if (vi)
        XFree((char *)vi);
    return visual;
}

static void query_colormap(QColormapPrivate *d, int screen)
{
    Display *display = QX11Info::display();

    // query existing colormap
    int q_colors = (((1u << d->depth) > 256u) ? 256u : (1u << d->depth));
    XColor queried[256];
    memset(queried, 0, sizeof(queried));
    for (int x = 0; x < q_colors; ++x)
        queried[x].pixel = x;
    XQueryColors(display, d->colormap, queried, q_colors);

    d->colors.resize(q_colors);
    for (int x = 0; x < q_colors; ++x) {
        if (queried[x].red == 0
            && queried[x].green == 0
            && queried[x].blue == 0
            && queried[x].pixel != BlackPixel(display, screen)) {
            // unallocated color cell, skip it
            continue;
        }

        d->colors[x] = QColor::fromRgbF(queried[x].red / float(USHRT_MAX),
                                        queried[x].green / float(USHRT_MAX),
                                        queried[x].blue / float(USHRT_MAX));
    }

    // for missing colors, find the closest color in the existing colormap
    Q_ASSERT(d->pixels.size());
    for (int x = 0; x < d->pixels.size(); ++x) {
        if (d->pixels.at(x) != -1)
            continue;

        QRgb rgb;
        if (d->mode == QColormap::Indexed) {
            const int r = (x / (d->g_max * d->b_max)) % d->r_max;
            const int g = (x / d->b_max) % d->g_max;
            const int b = x % d->b_max;
            rgb = qRgb((r * 0xff + (d->r_max - 1) / 2) / (d->r_max - 1),
                       (g * 0xff + (d->g_max - 1) / 2) / (d->g_max - 1),
                       (b * 0xff + (d->b_max - 1) / 2) / (d->b_max - 1));
        } else {
            rgb = qRgb(x, x, x);
        }

        // find closest color
        int mindist = INT_MAX, best = -1;
        for (int y = 0; y < q_colors; ++y) {
            int r =   qRed(rgb) - (queried[y].red   >> 8);
            int g = qGreen(rgb) - (queried[y].green >> 8);
            int b =  qBlue(rgb) - (queried[y].blue  >> 8);
            int dist = (r * r) + (g * g) + (b * b);
            if (dist < mindist) {
                mindist = dist;
                best = y;
            }
        }

        Q_ASSERT(best >= 0 && best < q_colors);
        if (d->visual->c_class & 1) {
            XColor xcolor;
            xcolor.red   = queried[best].red;
            xcolor.green = queried[best].green;
            xcolor.blue  = queried[best].blue;
            xcolor.pixel = queried[best].pixel;

            if (XAllocColor(display, d->colormap, &xcolor)) {
                d->pixels[x] = xcolor.pixel;
            } else {
                // some weird stuff is going on...
                d->pixels[x] = (qGray(rgb) < 127
                                ? BlackPixel(display, screen)
                                : WhitePixel(display, screen));
            }
        } else {
            d->pixels[x] = best;
        }
    }
}

static void init_gray(QColormapPrivate *d, int screen)
{
    d->pixels.resize(d->r_max);

    for (int g = 0; g < d->g_max; ++g) {
        const int gray = (g * 0xff + (d->r_max - 1) / 2) / (d->r_max - 1);
        const QRgb rgb = qRgb(gray, gray, gray);

        d->pixels[g] = -1;

        if (d->visual->c_class & 1) {
            XColor xcolor;
            xcolor.red   =   qRed(rgb) * 0x101;
            xcolor.green = qGreen(rgb) * 0x101;
            xcolor.blue  =  qBlue(rgb) * 0x101;
            xcolor.pixel = 0ul;

            if (XAllocColor(QX11Info::display(), d->colormap, &xcolor))
                d->pixels[g] = xcolor.pixel;
        }
    }

    query_colormap(d, screen);
}

static void init_indexed(QColormapPrivate *d, int screen)
{
    d->pixels.resize(d->r_max * d->g_max * d->b_max);

    // create color cube
    for (int x = 0, r = 0; r < d->r_max; ++r) {
        for (int g = 0; g < d->g_max; ++g) {
            for (int b = 0; b < d->b_max; ++b, ++x) {
                const QRgb rgb = qRgb((r * 0xff + (d->r_max - 1) / 2) / (d->r_max - 1),
                                      (g * 0xff + (d->g_max - 1) / 2) / (d->g_max - 1),
                                      (b * 0xff + (d->b_max - 1) / 2) / (d->b_max - 1));

                d->pixels[x] = -1;

                if (d->visual->c_class & 1) {
                    XColor xcolor;
                    xcolor.red   =   qRed(rgb) * 0x101;
                    xcolor.green = qGreen(rgb) * 0x101;
                    xcolor.blue  =  qBlue(rgb) * 0x101;
                    xcolor.pixel = 0ul;

                    if (XAllocColor(QX11Info::display(), d->colormap, &xcolor))
                        d->pixels[x] = xcolor.pixel;
                }
            }
        }
    }

    query_colormap(d, screen);
}

static void init_direct(QColormapPrivate *d, bool ownColormap)
{
    if (d->visual->c_class != DirectColor || !ownColormap)
        return;

    // preallocate 768 on the stack, so that we don't have to malloc
    // for the common case (<= 24 bpp)
    QVarLengthArray<XColor, 768> colorTable(d->r_max + d->g_max + d->b_max);
    int i = 0;

    for (int r = 0; r < d->r_max; ++r) {
        colorTable[i].red = r << 8 | r;
        colorTable[i].pixel = r << d->r_shift;
        colorTable[i].flags = DoRed;
        ++i;
    }

    for (int g = 0; g < d->g_max; ++g) {
        colorTable[i].green = g << 8 | g;
        colorTable[i].pixel = g << d->g_shift;
        colorTable[i].flags = DoGreen;
        ++i;
    }

    for (int b = 0; b < d->b_max; ++b) {
        colorTable[i].blue = (b << 8 | b);
        colorTable[i].pixel = b << d->b_shift;
        colorTable[i].flags = DoBlue;
        ++i;
    }

    XStoreColors(X11->display, d->colormap, colorTable.data(), colorTable.count());
}

static QColormap **cmaps = 0;

void QColormap::initialize()
{
    Display *display = QX11Info::display();
    const int screens = ScreenCount(display);

    cmaps = new QColormap*[screens];

    for (int i = 0; i < screens; ++i) {
        cmaps[i] = new QColormap;
        QColormapPrivate * const d = cmaps[i]->d;

        bool use_stdcmap = false;
        int color_count = X11->color_count;

        // defaults
        d->depth = DefaultDepth(display, i);
        d->colormap = DefaultColormap(display, i);
        d->defaultColormap = true;
        d->visual = DefaultVisual(display, i);
        d->defaultVisual = true;

        Visual *argbVisual = 0;

        if (X11->visual && i == DefaultScreen(display)) {
            // only use the outside colormap on the default screen
            d->visual = find_visual(display, i, X11->visual->c_class,
                                    XVisualIDFromVisual(X11->visual),
                                    &d->depth, &d->defaultVisual);
        } else if ((X11->visual_class != -1 && X11->visual_class >= 0 && X11->visual_class < 6)
                   || (X11->visual_id != -1)) {
            // look for a specific visual or type of visual
            d->visual = find_visual(display, i, X11->visual_class, X11->visual_id,
                                    &d->depth, &d->defaultVisual);
        } else if (QApplication::colorSpec() == QApplication::ManyColor) {
            // look for a TrueColor w/ a depth higher than 8bpp
            d->visual = find_visual(display, i, TrueColor, -1, &d->depth, &d->defaultVisual);
            if (d->depth <= 8) {
                d->visual = DefaultVisual(display, i);
                d->defaultVisual = true;
                color_count = 216;
            }
        } else if (!X11->custom_cmap) {
            XStandardColormap *stdcmap = 0;
            int ncmaps = 0;

#ifndef QT_NO_XRENDER
            if (X11->use_xrender) {
                int nvi;
                XVisualInfo templ;
                templ.screen  = i;
                templ.depth   = 32;
                templ.c_class = TrueColor;
                XVisualInfo *xvi = XGetVisualInfo(X11->display, VisualScreenMask |
                                                  VisualDepthMask |
                                                  VisualClassMask, &templ, &nvi);
                for (int idx = 0; idx < nvi; ++idx) {
                    XRenderPictFormat *format = XRenderFindVisualFormat(X11->display,
                                                                        xvi[idx].visual);
                    if (format->type == PictTypeDirect && format->direct.alphaMask) {
                        argbVisual = xvi[idx].visual;
                        break;
                    }
                }
                XFree(xvi);
            }
#endif
            if (XGetRGBColormaps(display, RootWindow(display, i),
                                 &stdcmap, &ncmaps, XA_RGB_DEFAULT_MAP)) {
                if (stdcmap) {
                    for (int c = 0; c < ncmaps; ++c) {
                        if (!stdcmap[c].red_max ||
                            !stdcmap[c].green_max ||
                            !stdcmap[c].blue_max ||
                            !stdcmap[c].red_mult ||
                            !stdcmap[c].green_mult ||
                            !stdcmap[c].blue_mult)
                            continue; // invalid stdcmap

                        XVisualInfo proto;
                        proto.visualid = stdcmap[c].visualid;
                        proto.screen = i;

                        int nvisuals = 0;
                        XVisualInfo *vi = XGetVisualInfo(display, VisualIDMask | VisualScreenMask,
                                                         &proto, &nvisuals);
                        if (vi) {
                            if (nvisuals > 0) {
                                use_stdcmap = true;

                                d->mode = ((vi[0].visual->c_class < StaticColor)
                                           ? Gray
                                           : ((vi[0].visual->c_class < TrueColor)
                                              ? Indexed
                                              : Direct));

                                d->depth = vi[0].depth;
                                d->colormap = stdcmap[c].colormap;
                                d->defaultColormap = true;
                                d->visual = vi[0].visual;
                                d->defaultVisual = (d->visual == DefaultVisual(display, i));

                                d->r_max = stdcmap[c].red_max   + 1;
                                d->g_max = stdcmap[c].green_max + 1;
                                d->b_max = stdcmap[c].blue_max  + 1;

                                if (d->mode == Direct) {
                                    // calculate offsets
                                    d->r_shift = lowest_bit(d->visual->red_mask);
                                    d->g_shift = lowest_bit(d->visual->green_mask);
                                    d->b_shift = lowest_bit(d->visual->blue_mask);
                                } else {
                                    d->r_shift = 0;
                                    d->g_shift = 0;
                                    d->b_shift = 0;
                                }
                            }
                            XFree(vi);
                        }
                        break;
                    }
                    XFree(stdcmap);
                }
            }
        }
        if (!use_stdcmap) {
            switch (d->visual->c_class) {
            case StaticGray:
                d->mode = Gray;

                d->r_max = d->g_max = d->b_max = d->visual->map_entries;
                break;

            case XGrayScale:
                d->mode = Gray;

                // follow precedent set in libXmu...
                if (color_count != 0)
                    d->r_max = d->g_max = d->b_max = color_count;
                else if (d->visual->map_entries > 65000)
                    d->r_max = d->g_max = d->b_max = 4096;
                else if (d->visual->map_entries > 4000)
                    d->r_max = d->g_max = d->b_max = 512;
                else if (d->visual->map_entries > 250)
                    d->r_max = d->g_max = d->b_max = 12;
                else
                    d->r_max = d->g_max = d->b_max = 4;
                break;

            case StaticColor:
                d->mode = Indexed;

                d->r_max = right_align(d->visual->red_mask)   + 1;
                d->g_max = right_align(d->visual->green_mask) + 1;
                d->b_max = right_align(d->visual->blue_mask)  + 1;
                break;

            case PseudoColor:
                d->mode = Indexed;

                // follow precedent set in libXmu...
                if (color_count != 0)
                    d->r_max = d->g_max = d->b_max = cube_root(color_count);
                else if (d->visual->map_entries > 65000)
                    d->r_max = d->g_max = d->b_max = 27;
                else if (d->visual->map_entries > 4000)
                    d->r_max = d->g_max = d->b_max = 12;
                else if (d->visual->map_entries > 250)
                    d->r_max = d->g_max = d->b_max = cube_root(d->visual->map_entries - 125);
                else
                    d->r_max = d->g_max = d->b_max = cube_root(d->visual->map_entries);
                break;

            case TrueColor:
            case DirectColor:
                d->mode = Direct;

                d->r_max = right_align(d->visual->red_mask)   + 1;
                d->g_max = right_align(d->visual->green_mask) + 1;
                d->b_max = right_align(d->visual->blue_mask)  + 1;

                d->r_shift = lowest_bit(d->visual->red_mask);
                d->g_shift = lowest_bit(d->visual->green_mask);
                d->b_shift = lowest_bit(d->visual->blue_mask);
                break;
            }
        }

        bool ownColormap = false;
        if (X11->colormap && i == DefaultScreen(display)) {
            // only use the outside colormap on the default screen
            d->colormap = X11->colormap;
            d->defaultColormap = (d->colormap == DefaultColormap(display, i));
        } else if ((!use_stdcmap
                   && (((d->visual->c_class & 1) && X11->custom_cmap)
                       || d->visual != DefaultVisual(display, i)))
                   || d->visual->c_class == DirectColor) {
            // allocate custom colormap (we always do this when using DirectColor visuals)
            d->colormap =
                XCreateColormap(display, RootWindow(display, i), d->visual,
                                d->visual->c_class == DirectColor ? AllocAll : AllocNone);
            d->defaultColormap = false;
            ownColormap = true;
        }

        switch (d->mode) {
        case Gray:
            init_gray(d, i);
            break;
        case Indexed:
            init_indexed(d, i);
            break;
        case Direct:
            init_direct(d, ownColormap);
            break;
        }

        QX11InfoData *screen = X11->screens + i;
        screen->depth = d->depth;
        screen->visual = d->visual;
        screen->defaultVisual = d->defaultVisual;
        screen->colormap = d->colormap;
        screen->defaultColormap = d->defaultColormap;
        screen->cells = screen->visual->map_entries;

        if (argbVisual) {
            X11->argbVisuals[i] = argbVisual;
            X11->argbColormaps[i] = XCreateColormap(display, RootWindow(display, i), argbVisual, AllocNone);
        }

        // ###
        // We assume that 8bpp == pseudocolor, but this is not
        // always the case (according to the X server), so we need
        // to make sure that our internal data is setup in a way
        // that is compatible with our assumptions
        if (screen->visual->c_class == TrueColor && screen->depth == 8 && screen->cells == 8)
            screen->cells = 256;
    }
}

void QColormap::cleanup()
{
    Display *display = QX11Info::display();
    const int screens = ScreenCount(display);

    for (int i = 0; i < screens; ++i)
        delete cmaps[i];

    delete [] cmaps;
    cmaps = 0;
}


QColormap QColormap::instance(int screen)
{
    if (screen == -1)
        screen = QX11Info::appScreen();
    return *cmaps[screen];
}

/*! \internal
    Constructs a new colormap.
*/
QColormap::QColormap()
    : d(new QColormapPrivate)
{}

QColormap::QColormap(const QColormap &colormap)
    :d (colormap.d)
{ d->ref.ref(); }

QColormap::~QColormap()
{
    if (!d->ref.deref()) {
        if (!d->defaultColormap)
            XFreeColormap(QX11Info::display(), d->colormap);
        delete d;
    }
}

QColormap::Mode QColormap::mode() const
{ return d->mode; }

int QColormap::depth() const
{ return d->depth; }

int QColormap::size() const
{
    return (d->mode == Gray
            ? d->r_max
            : (d->mode == Indexed
               ? d->r_max * d->g_max * d->b_max
               : -1));
}

uint QColormap::pixel(const QColor &color) const
{
    const QColor c = color.toRgb();
    const uint r = (c.ct.argb.red   * d->r_max) >> 16;
    const uint g = (c.ct.argb.green * d->g_max) >> 16;
    const uint b = (c.ct.argb.blue  * d->b_max) >> 16;
    if (d->mode != Direct) {
        if (d->mode == Gray)
            return d->pixels.at((r * 30 + g * 59 + b * 11) / 100);
        return d->pixels.at(r * d->g_max * d->b_max + g * d->b_max + b);
    }
    return (r << d->r_shift) + (g << d->g_shift) + (b << d->b_shift);
}

const QColor QColormap::colorAt(uint pixel) const
{
    if (d->mode != Direct) {
        Q_ASSERT(pixel <= (uint)d->colors.size());
        return d->colors.at(pixel);
    }

    const int r = (((pixel & d->visual->red_mask)   >> d->r_shift) << 8) / d->r_max;
    const int g = (((pixel & d->visual->green_mask) >> d->g_shift) << 8) / d->g_max;
    const int b = (((pixel & d->visual->blue_mask)  >> d->b_shift) << 8) / d->b_max;
    return QColor(r, g, b);
}

const QVector<QColor> QColormap::colormap() const
{ return d->colors; }

QColormap &QColormap::operator=(const QColormap &colormap)
{
    qAtomicAssign(d, colormap.d);
    return *this;
}

QT_END_NAMESPACE
