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

#include "qplastiquestyle.h"

#if !defined(QT_NO_STYLE_PLASTIQUE) || defined(QT_PLUGIN)

static const bool AnimateBusyProgressBar = true;
static const bool AnimateProgressBar = false;
// #define QPlastique_MaskButtons
static const int ProgressBarFps = 25;
static const int blueFrameWidth =  2;  // with of line edit focus frame

#include "qwindowsstyle_p.h"
#include <qapplication.h>
#include <qbitmap.h>
#include <qabstractitemview.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qdebug.h>
#include <qdialogbuttonbox.h>
#include <qformlayout.h>
#include <qgroupbox.h>
#include <qimage.h>
#include <qlineedit.h>
#include <qmainwindow.h>
#include <qmenu.h>
#include <qmenubar.h>
#include <qpainter.h>
#include <qpaintengine.h>
#include <qpainterpath.h>
#include <qpalette.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qpixmapcache.h>
#include <qprogressbar.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qscrollbar.h>
#include <qspinbox.h>
#include <qsplitter.h>
#include <qstyleoption.h>
#include <qtextedit.h>
#include <qelapsedtimer.h>
#include <qtoolbar.h>
#include <qtoolbox.h>
#include <qtoolbutton.h>
#include <qworkspace.h>
#include <qprocess.h>
#include <qvarlengtharray.h>
#include <limits.h>
#include <private/qstylehelper_p.h>

QT_BEGIN_NAMESPACE

// from windows style
static const int windowsItemFrame        =  2; // menu item frame width
static const int windowsSepHeight        =  2; // separator item height
static const int windowsItemHMargin      =  3; // menu item hor text margin
static const int windowsItemVMargin      =  2; // menu item ver text margin
static const int windowsArrowHMargin     =  6; // arrow horizontal margin
static const int windowsTabSpacing       = 12; // space between text and tab
static const int windowsRightBorder      = 15; // right border on windows
static const int windowsCheckMarkWidth   = 12; // checkmarks width on windows

static const char * const qt_plastique_slider_verticalhandle[] = {
    "15 11 6 1",
    "   c None",
    "+  c #979797",
    "@  c #C9C9C9",
    "$  c #C1C1C1",
    "b  c None",
    "d  c None",
    " $++++++++$    ",
    "$+bbbbbbbb+$   ",
    "+b $$      +$  ",
    "+b $@       +$ ",
    "+b           +$",
    "+b           d+",
    "+b          d+$",
    "+b $$      d+$ ",
    "+b $@     d+$  ",
    "$+dddddddd+$   ",
    " $++++++++$    "};

static const char * const qt_plastique_slider_verticalhandle_left[] = {
    "15 11 6 1",
    "   c None",
    "+  c #979797",
    "@  c #C9C9C9",
    "$  c #C1C1C1",
    "b  c None",
    "d  c None",
    "    $++++++++$ ",
    "   $+bbbbbbbb+$",
    "  $+b     $$ d+",
    " $+b      $@ d+",
    "$+b          d+",
    "+b           d+",
    "$+           d+",
    " $+       $$ d+",
    "  $+      $@ d+",
    "   $+dddddddd+$",
    "    $++++++++$ "};

static const char * const qt_plastique_slider_horizontalhandle[] = {
    "11 15 6 1",
    "   c None",
    "+  c #979797",
    "@  c #C9C9C9",
    "$  c #C1C1C1",
    "b  c None",
    "d  c None",
    " $+++++++$ ",
    "$+bbbbbbb+$",
    "+b       d+",
    "+b$$   $$d+",
    "+b$@   $@d+",
    "+b       d+",
    "+b       d+",
    "+b       d+",
    "+b       d+",
    "+b       d+",
    "$+      d+$",
    " $+    d+$ ",
    "  $+  d+$  ",
    "   $+d+$   ",
    "    $+$    "};

static const char * const qt_plastique_slider_horizontalhandle_up[] = {
    "11 15 6 1",
    "   c None",
    "+  c #979797",
    "@  c #C9C9C9",
    "$  c #C1C1C1",
    "b  c None",
    "d  c None",
    "    $+$    ",
    "   $+b+$   ",
    "  $+b  +$  ",
    " $+b    +$ ",
    "$+b      +$",
    "+b       d+",
    "+b       d+",
    "+b       d+",
    "+b       d+",
    "+b       d+",
    "+b$$   $$d+",
    "+b$@   $@d+",
    "+b       d+",
    "$+ddddddd+$",
    " $+++++++$ "};

static const char * const qt_scrollbar_button_arrow_left[] = {
    "4 7 2 1",
    "   c None",
    "*  c #BFBFBF",
    "   *",
    "  **",
    " ***",
    "****",
    " ***",
    "  **",
    "   *"};

static const char * const qt_scrollbar_button_arrow_right[] = {
    "4 7 2 1",
    "   c None",
    "*  c #BFBFBF",
    "*   ",
    "**  ",
    "*** ",
    "****",
    "*** ",
    "**  ",
    "*   "};

static const char * const qt_scrollbar_button_arrow_up[] = {
    "7 4 2 1",
    "   c None",
    "*  c #BFBFBF",
    "   *   ",
    "  ***  ",
    " ***** ",
    "*******"};

static const char * const qt_scrollbar_button_arrow_down[] = {
    "7 4 2 1",
    "   c None",
    "*  c #BFBFBF",
    "*******",
    " ***** ",
    "  ***  ",
    "   *   "};

static const char * const qt_scrollbar_button_left[] = {
    "16 16 6 1",
    "   c None",
    ".  c #BFBFBF",
    "+  c #979797",
    "#  c #FAFAFA",
    "<  c #FAFAFA",
    "*  c #FAFAFA",
    " .+++++++++++++.",
    ".+#############+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    ".+<<<<<<<<<<<<<+",
    " .+++++++++++++."};

static const char * const qt_scrollbar_button_right[] = {
    "16 16 6 1",
    "   c None",
    ".  c #BFBFBF",
    "+  c #979797",
    "#  c #FAFAFA",
    "<  c #FAFAFA",
    "*  c #FAFAFA",
    ".+++++++++++++. ",
    "+#############+.",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+<<<<<<<<<<<<<+.",
    ".+++++++++++++. "};

static const char * const qt_scrollbar_button_up[] = {
    "16 16 6 1",
    "   c None",
    ".  c #BFBFBF",
    "+  c #979797",
    "#  c #FAFAFA",
    "<  c #FAFAFA",
    "*  c #FAFAFA",
    " .++++++++++++. ",
    ".+############+.",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+<<<<<<<<<<<<<<+",
    ".++++++++++++++."};

static const char * const qt_scrollbar_button_down[] = {
    "16 16 6 1",
    "   c None",
    ".  c #BFBFBF",
    "+  c #979797",
    "#  c #FAFAFA",
    "<  c #FAFAFA",
    "*  c #FAFAFA",
    "++++++++++++++++",
    "+##############+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    "+#            <+",
    ".+<<<<<<<<<<<<+.",
    " .++++++++++++. "};

static const char * const qt_scrollbar_slider_pattern_vertical[] = {
    "10 18 3 1",
    "   c None",
    ".  c #BFBFBF",
    "+  c #979797",
    "..  ..  ..",
    ".+  .+  .+",
    "          ",
    "          ",
    "..  ..  ..",
    ".+  .+  .+",
    "          ",
    "          ",
    "..  ..  ..",
    ".+  .+  .+",
    "          ",
    "          ",
    "..  ..  ..",
    ".+  .+  .+",
    "          ",
    "          ",
    "..  ..  ..",
    ".+  .+  .+"};

static const char * const qt_scrollbar_slider_pattern_horizontal[] = {
    "18 10 3 1",
    "   c None",
    ".  c #BFBFBF",
    "+  c #979797",
    "..  ..  ..  ..  ..",
    ".+  .+  .+  .+  .+",
    "                  ",
    "                  ",
    "..  ..  ..  ..  ..",
    ".+  .+  .+  .+  .+",
    "                  ",
    "                  ",
    "..  ..  ..  ..  ..",
    ".+  .+  .+  .+  .+"};

static const char * const qt_toolbarhandle[] = {
    "6 6 4 1",
    "       c None",
    ".      c #C5C5C5",
    "+      c #EEEEEE",
    "@      c #FAFAFA",
    "..    ",
    ".+@   ",
    " @@   ",
    "   .. ",
    "   .+@",
    "    @@"};

static const char * const qt_simple_toolbarhandle[] = {
    "3 3 4 1",
    "       c None",
    ".      c #C5C5C5",
    "+      c #EEEEEE",
    "@      c #FAFAFA",
    ".. ",
    ".+@",
    " @@"};

static const char * const qt_titlebar_context_help[] = {
"27 27 5 1",
"  c None",
". c #0A0C12",
"+ c #1B202D",
"@ c #293144",
"# c #3C435D",
"                           ",
"                           ",
"                           ",
"                           ",
"                           ",
"                           ",
"                           ",
"                           ",
"           +@##@+          ",
"         .@@@.+@@..        ",
"         .##+  +@@+.       ",
"         .##@  @#@+.       ",
"         ....  +@+..       ",
"            .@+@@..        ",
"            +#@@+          ",
"            .##.           ",
"            .++.           ",
"            .++.           ",
"            +##+           ",
"            .@@.           ",
"                           ",
"                           ",
"                           ",
"                           ",
"                           ",
"                           ",
"                           "};

static QLinearGradient qMapGradientToRect(const QLinearGradient &gradient, const QRectF &rect)
{
    QLinearGradient tmpGrad(rect.center().x(), rect.top(),
                            rect.center().x(), rect.bottom());
    tmpGrad.setStops(gradient.stops());
    return tmpGrad;
}

static QBrush qMapBrushToRect(const QBrush &brush, const QRectF &rect)
{
    if (!brush.gradient())
        return brush;

    // ### Ugly assumption that it's a linear gradient
    QBrush tmp(qMapGradientToRect(*static_cast<const QLinearGradient *>(brush.gradient()), rect));
    return tmp;
}

static void qBrushSetAlphaF(QBrush *brush, qreal alpha)
{
    if (const QGradient *gradient = brush->gradient()) {
        // Use the gradient. Call QColor::setAlphaF() on all color stops.
        QGradientStops stops = gradient->stops();
        QMutableVectorIterator<QGradientStop> it(stops);
        QColor tmpColor;
        while (it.hasNext()) {
            it.next();
            tmpColor = it.value().second;
            tmpColor.setAlphaF(alpha * tmpColor.alphaF());
            it.setValue(QPair<qreal, QColor>(it.value().first, tmpColor));
        }

        switch (gradient->type()) {
        case QGradient::RadialGradient: {
            QRadialGradient grad = *static_cast<const QRadialGradient *>(gradient);
            grad.setStops(stops);
            *brush = QBrush(grad);
            break;
        }
        case QGradient::ConicalGradient: {
            QConicalGradient grad = *static_cast<const QConicalGradient *>(gradient);
            grad.setStops(stops);
            *brush = QBrush(grad);
            break;
        }
        default:
            qWarning("QPlastiqueStyle::qBrushLight() - unknown gradient type"
                     " - falling back to QLinearGradient");
        case QGradient::LinearGradient: {
            QLinearGradient grad = *static_cast<const QLinearGradient *>(gradient);
            grad.setStops(stops);
            *brush = QBrush(grad);
            break;
        }
        }
    } else if (!brush->texture().isNull()) {
        // Modify the texture - ridiculously expensive.
        QPixmap texture = brush->texture();
        QPixmap pixmap;
        QString name = QLatin1Literal("qbrushtexture-alpha")
                       % HexString<qreal>(alpha)
                       % HexString<qint64>(texture.cacheKey());
        if (!QPixmapCache::find(name, pixmap)) {
            QImage image = texture.toImage();
            QRgb *rgb = reinterpret_cast<QRgb *>(image.bits());
            int pixels = image.width() * image.height();
            QColor tmpColor;
            while (pixels--) {
                tmpColor.setRgb(*rgb);
                tmpColor.setAlphaF(alpha * tmpColor.alphaF());
                *rgb++ = tmpColor.rgba();
            }
            pixmap = QPixmap::fromImage(image);
            QPixmapCache::insert(name, pixmap);
        }
        brush->setTexture(pixmap);
    } else {
        // Use the color
        QColor tmpColor = brush->color();
        tmpColor.setAlphaF(alpha * tmpColor.alphaF());
        brush->setColor(tmpColor);
    }
}

static QBrush qBrushLight(QBrush brush, int light)
{
    if (const QGradient *gradient = brush.gradient()) {
        // Use the gradient. Call QColor::lighter() on all color stops.
        QGradientStops stops = gradient->stops();
        QMutableVectorIterator<QGradientStop> it(stops);
        while (it.hasNext()) {
            it.next();
            it.setValue(QPair<qreal, QColor>(it.value().first, it.value().second.lighter(light)));
        }

        switch (gradient->type()) {
        case QGradient::RadialGradient: {
            QRadialGradient grad = *static_cast<const QRadialGradient *>(gradient);
            grad.setStops(stops);
            brush = QBrush(grad);
            break;
        }
        case QGradient::ConicalGradient: {
            QConicalGradient grad = *static_cast<const QConicalGradient *>(gradient);
            grad.setStops(stops);
            brush = QBrush(grad);
            break;
        }
        default:
            qWarning("QPlastiqueStyle::qBrushLight() - unknown gradient type"
                     " - falling back to QLinearGradient");
        case QGradient::LinearGradient: {
            QLinearGradient grad = *static_cast<const QLinearGradient *>(gradient);
            grad.setStops(stops);
            brush = QBrush(grad);
            break;
        }
        }
    } else if (!brush.texture().isNull()) {
        // Modify the texture - ridiculously expensive.
        QPixmap texture = brush.texture();
        QPixmap pixmap;
        QString name = QLatin1Literal("qbrushtexture-light")
                       % HexString<int>(light)
                       % HexString<qint64>(texture.cacheKey());

        if (!QPixmapCache::find(name, pixmap)) {
            QImage image = texture.toImage();
            QRgb *rgb = reinterpret_cast<QRgb *>(image.bits());
            int pixels = image.width() * image.height();
            QColor tmpColor;
            while (pixels--) {
                tmpColor.setRgb(*rgb);
                *rgb++ = tmpColor.lighter(light).rgba();
            }
            pixmap = QPixmap::fromImage(image);
            QPixmapCache::insert(name, pixmap);
        }
        brush.setTexture(pixmap);
    } else {
        // Use the color
        brush.setColor(brush.color().lighter(light));
    }
    return brush;
}

static QBrush qBrushDark(QBrush brush, int dark)
{
    if (const QGradient *gradient = brush.gradient()) {
        // Use the gradient. Call QColor::darker() on all color stops.
        QGradientStops stops = gradient->stops();
        QMutableVectorIterator<QGradientStop> it(stops);
        while (it.hasNext()) {
            it.next();
            it.setValue(QPair<qreal, QColor>(it.value().first, it.value().second.darker(dark)));
        }

        switch (gradient->type()) {
        case QGradient::RadialGradient: {
            QRadialGradient grad = *static_cast<const QRadialGradient *>(gradient);
            grad.setStops(stops);
            brush = QBrush(grad);
            break;
        }
        case QGradient::ConicalGradient: {
            QConicalGradient grad = *static_cast<const QConicalGradient *>(gradient);
            grad.setStops(stops);
            brush = QBrush(grad);
            break;
        }
        default:
            qWarning("QPlastiqueStyle::qBrushDark() - unknown gradient type"
                     " - falling back to QLinearGradient");
        case QGradient::LinearGradient: {
            QLinearGradient grad = *static_cast<const QLinearGradient *>(gradient);
            grad.setStops(stops);
            brush = QBrush(grad);
            break;
        }
        }
    } else if (!brush.texture().isNull()) {
        // Modify the texture - ridiculously expensive.
        QPixmap texture = brush.texture();
        QPixmap pixmap;
        QString name = QLatin1Literal("qbrushtexture-dark")
                       % HexString<int>(dark)
                       % HexString<qint64>(texture.cacheKey());

        if (!QPixmapCache::find(name, pixmap)) {
            QImage image = texture.toImage();
            QRgb *rgb = reinterpret_cast<QRgb *>(image.bits());
            int pixels = image.width() * image.height();
            QColor tmpColor;
            while (pixels--) {
                tmpColor.setRgb(*rgb);
                *rgb++ = tmpColor.darker(dark).rgba();
            }
            pixmap = QPixmap::fromImage(image);
            QPixmapCache::insert(name, pixmap);
        }
        brush.setTexture(pixmap);
    } else {
        // Use the color
        brush.setColor(brush.color().darker(dark));
    }
    return brush;
}

/*
    Draws a rounded frame using the provided brush for 1, and adds 0.5 alpha
    for 0.

     0111111110
    01        10
    1          1
    1          1
    1          1
    01        10
     0111111110
*/
static void qt_plastique_draw_frame(QPainter *painter, const QRect &rect, const QStyleOption *option,
                                    QFrame::Shadow shadow = QFrame::Plain)
{
    QPen oldPen = painter->pen();
    QBrush border;
    QBrush corner;
    QBrush innerTopLeft;
    QBrush innerBottomRight;

    if (shadow != QFrame::Plain && (option->state & QStyle::State_HasFocus)) {
        border = option->palette.highlight();
        qBrushSetAlphaF(&border, qreal(0.8));
        corner = option->palette.highlight();
        qBrushSetAlphaF(&corner, 0.5);
        innerTopLeft = qBrushDark(option->palette.highlight(), 125);
        innerBottomRight = option->palette.highlight();
        qBrushSetAlphaF(&innerBottomRight, qreal(0.65));
    } else {
        border = option->palette.shadow();
        qBrushSetAlphaF(&border, qreal(0.4));
        corner = option->palette.shadow();
        qBrushSetAlphaF(&corner, 0.25);
        innerTopLeft = option->palette.shadow();
        innerBottomRight = option->palette.shadow();
        if (shadow == QFrame::Sunken) {
            qBrushSetAlphaF(&innerTopLeft, qreal(0.23));
            qBrushSetAlphaF(&innerBottomRight, qreal(0.075));
        } else {
            qBrushSetAlphaF(&innerTopLeft, qreal(0.075));
            qBrushSetAlphaF(&innerBottomRight, qreal(0.23));
        }
    }

    QLine lines[4];
    QPoint points[8];

    // Opaque corner lines
    painter->setPen(QPen(border, 0));
    lines[0] = QLine(rect.left() + 2, rect.top(), rect.right() - 2, rect.top());
    lines[1] = QLine(rect.left() + 2, rect.bottom(), rect.right() - 2, rect.bottom());
    lines[2] = QLine(rect.left(), rect.top() + 2, rect.left(), rect.bottom() - 2);
    lines[3] = QLine(rect.right(), rect.top() + 2, rect.right(), rect.bottom() - 2);
    painter->drawLines(lines, 4);

    // Opaque corner dots
    points[0] = QPoint(rect.left() + 1, rect.top() + 1);
    points[1] = QPoint(rect.left() + 1, rect.bottom() - 1);
    points[2] = QPoint(rect.right() - 1, rect.top() + 1);
    points[3] = QPoint(rect.right() - 1, rect.bottom() - 1);
    painter->drawPoints(points, 4);

    // Shaded corner dots
    painter->setPen(QPen(corner, 0));
    points[0] = QPoint(rect.left(), rect.top() + 1);
    points[1] = QPoint(rect.left(), rect.bottom() - 1);
    points[2] = QPoint(rect.left() + 1, rect.top());
    points[3] = QPoint(rect.left() + 1, rect.bottom());
    points[4] = QPoint(rect.right(), rect.top() + 1);
    points[5] = QPoint(rect.right(), rect.bottom() - 1);
    points[6] = QPoint(rect.right() - 1, rect.top());
    points[7] = QPoint(rect.right() - 1, rect.bottom());
    painter->drawPoints(points, 8);

    // Shadows
    if (shadow != QFrame::Plain) {
        painter->setPen(QPen(innerTopLeft, 0));
        lines[0] = QLine(rect.left() + 2, rect.top() + 1, rect.right() - 2, rect.top() + 1);
        lines[1] = QLine(rect.left() + 1, rect.top() + 2, rect.left() + 1, rect.bottom() - 2);
        painter->drawLines(lines, 2);
        painter->setPen(QPen(innerBottomRight, 0));
        lines[0] = QLine(rect.left() + 2, rect.bottom() - 1, rect.right() - 2, rect.bottom() - 1);
        lines[1] = QLine(rect.right() - 1, rect.top() + 2, rect.right() - 1, rect.bottom() - 2);
        painter->drawLines(lines, 2);
    }

    painter->setPen(oldPen);
}

static QColor mergedColors(const QColor &colorA, const QColor &colorB, int factor = 50)
{
    const int maxFactor = 100;
    QColor tmp = colorA;
    tmp.setRed((tmp.red() * factor) / maxFactor + (colorB.red() * (maxFactor - factor)) / maxFactor);
    tmp.setGreen((tmp.green() * factor) / maxFactor + (colorB.green() * (maxFactor - factor)) / maxFactor);
    tmp.setBlue((tmp.blue() * factor) / maxFactor + (colorB.blue() * (maxFactor - factor)) / maxFactor);
    return tmp;
}

static void qt_plastique_draw_gradient(QPainter *painter, const QRect &rect, const QColor &gradientStart,
                                       const QColor &gradientStop)
{
    QString gradientName = QLatin1Literal("qplastique-g")
                   % HexString<int>(rect.width())
                   % HexString<int>(rect.height())
                   % HexString<QRgb>(gradientStart.rgba())
                   % HexString<QRgb>(gradientStop.rgba());

    QPixmap cache;
    QPainter *p = painter;
    QRect r = rect;

    bool doPixmapCache = painter->deviceTransform().isIdentity()
	&& painter->worldMatrix().isIdentity();
    if (doPixmapCache && QPixmapCache::find(gradientName, cache)) {
        painter->drawPixmap(rect, cache);
    } else {
        if (doPixmapCache) {
            cache = QPixmap(rect.size());
            cache.fill(Qt::transparent);
            p = new QPainter(&cache);
	    r = QRect(0, 0, rect.width(), rect.height());
        }

        int x = r.center().x();
        QLinearGradient gradient(x, r.top(), x, r.bottom());
        gradient.setColorAt(0, gradientStart);
        gradient.setColorAt(1, gradientStop);
        p->fillRect(r, gradient);

        if (doPixmapCache) {
	    p->end();
	    delete p;
	    painter->drawPixmap(rect, cache);
            QPixmapCache::insert(gradientName, cache);
	}
    }
}

static void qt_plastique_drawFrame(QPainter *painter, const QStyleOption *option, const QWidget *widget)
{
    QRect rect = option->rect;
    QPen oldPen = painter->pen();

    QColor borderColor = option->palette.background().color().darker(178);
    QColor gradientStartColor = option->palette.button().color().lighter(104);
    QColor gradientStopColor = option->palette.button().color().darker(105);
    QColor alphaCornerColor;
    if (widget) {
        // ### backgroundrole/foregroundrole should be part of the style option
        alphaCornerColor = mergedColors(option->palette.color(widget->backgroundRole()), borderColor);
    } else {
        alphaCornerColor = mergedColors(option->palette.background().color(), borderColor);
    }

    QLine lines[4];
    QPoint points[8];

    // outline / border
    painter->setPen(borderColor);
    lines[0] = QLine(rect.left() + 2, rect.top(), rect.right() - 2, rect.top());
    lines[1] = QLine(rect.left() + 2, rect.bottom(), rect.right() - 2, rect.bottom());
    lines[2] = QLine(rect.left(), rect.top() + 2, rect.left(), rect.bottom() - 2);
    lines[3] = QLine(rect.right(), rect.top() + 2, rect.right(), rect.bottom() - 2);
    painter->drawLines(lines, 4);

    points[0] = QPoint(rect.left() + 1, rect.top() + 1);
    points[1] = QPoint(rect.right() - 1, rect.top() + 1);
    points[2] = QPoint(rect.left() + 1, rect.bottom() - 1);
    points[3] = QPoint(rect.right() - 1, rect.bottom() - 1);
    painter->drawPoints(points, 4);

    painter->setPen(alphaCornerColor);

    points[0] = QPoint(rect.left() + 1, rect.top());
    points[1] = QPoint(rect.right() - 1, rect.top());
    points[2] = QPoint(rect.left() + 1, rect.bottom());
    points[3] = QPoint(rect.right() - 1, rect.bottom());
    points[4] = QPoint(rect.left(), rect.top() + 1);
    points[5] = QPoint(rect.right(), rect.top() + 1);
    points[6] = QPoint(rect.left(), rect.bottom() - 1);
    points[7] = QPoint(rect.right(), rect.bottom() - 1);
    painter->drawPoints(points, 8);

    // inner border
    if ((option->state & QStyle::State_Sunken) || (option->state & QStyle::State_On))
        painter->setPen(option->palette.button().color().darker(118));
    else
        painter->setPen(gradientStartColor);

    lines[0] = QLine(rect.left() + 2, rect.top() + 1, rect.right() - 2, option->rect.top() + 1);
    lines[1] = QLine(rect.left() + 1, rect.top() + 2, rect.left() + 1, option->rect.bottom() - 2);
    painter->drawLines(lines, 2);

    if ((option->state & QStyle::State_Sunken) || (option->state & QStyle::State_On))
        painter->setPen(option->palette.button().color().darker(110));
    else
        painter->setPen(gradientStopColor.darker(102));

    lines[0] = QLine(rect.left() + 2, rect.bottom() - 1, rect.right() - 2, rect.bottom() - 1);
    lines[1] = QLine(rect.right() - 1, rect.top() + 2, rect.right() - 1, rect.bottom() - 2);
    painter->drawLines(lines, 2);

    painter->setPen(oldPen);
}

static void qt_plastique_drawShadedPanel(QPainter *painter, const QStyleOption *option, bool base,
                                         const QWidget *widget)
{
    QRect rect = option->rect;
    QPen oldPen = painter->pen();

    QColor gradientStartColor = option->palette.button().color().lighter(104);
    QColor gradientStopColor = option->palette.button().color().darker(105);

    // gradient fill
    if ((option->state & QStyle::State_Enabled) || !(option->state & QStyle::State_AutoRaise)) {
        if ((option->state & QStyle::State_Sunken) || (option->state & QStyle::State_On)) {
            qt_plastique_draw_gradient(painter, rect.adjusted(1, 1, -1, -1),
                                       option->palette.button().color().darker(114),
                                       option->palette.button().color().darker(106));
        } else {
            qt_plastique_draw_gradient(painter, rect.adjusted(1, 1, -1, -1),
                                       base ? option->palette.background().color().lighter(105) : gradientStartColor,
                                       base ? option->palette.background().color().darker(102) : gradientStopColor);
        }
    }

    qt_plastique_drawFrame(painter, option, widget);

    painter->setPen(oldPen);
}

static void qt_plastique_draw_mdibutton(QPainter *painter, const QStyleOptionTitleBar *option, const QRect &tmp, bool hover, bool sunken)
{
    if (tmp.isNull())
        return;
    bool active = (option->titleBarState & QStyle::State_Active);

    // ### use palette colors instead
    QColor mdiButtonGradientStartColor;
    QColor mdiButtonGradientStopColor;
    if (active) {
        mdiButtonGradientStartColor = QColor((hover || sunken) ? 0x7d8bb1 : 0x55689a);
        mdiButtonGradientStopColor = QColor((hover || sunken) ? 0x939ebe : 0x7381ab);
    } else {
        mdiButtonGradientStartColor = QColor((hover || sunken) ? 0x9e9e9e : 0x818181);
        mdiButtonGradientStopColor = QColor((hover || sunken) ? 0xababab : 0x929292);
    }

    qt_plastique_draw_gradient(painter, tmp.adjusted(1, 1, -1, -1),
                               mdiButtonGradientStartColor, mdiButtonGradientStopColor);

    QColor mdiButtonBorderColor;
    if (active) {
        mdiButtonBorderColor = (hover || sunken) ? QColor(0x627097) : QColor(0x324577);
    } else {
        mdiButtonBorderColor = (hover || sunken) ? QColor(0x838383) : QColor(0x5e5e5e);
    }
    painter->setPen(QPen(mdiButtonBorderColor, 1));

    const QLine lines[4] = {
        QLine(tmp.left() + 2, tmp.top(), tmp.right() - 2, tmp.top()),
        QLine(tmp.left() + 2, tmp.bottom(), tmp.right() - 2, tmp.bottom()),
        QLine(tmp.left(), tmp.top() + 2, tmp.left(), tmp.bottom() - 2),
        QLine(tmp.right(), tmp.top() + 2, tmp.right(), tmp.bottom() - 2) };
    painter->drawLines(lines, 4);

    const QPoint points[4] = {
        QPoint(tmp.left() + 1, tmp.top() + 1),
        QPoint(tmp.right() - 1, tmp.top() + 1),
        QPoint(tmp.left() + 1, tmp.bottom() - 1),
        QPoint(tmp.right() - 1, tmp.bottom() - 1) };
    painter->drawPoints(points, 4);
}

#ifndef QT_NO_DOCKWIDGET
static QString elliditide(const QString &text, const QFontMetrics &fontMetrics, const QRect &rect, int *textWidth = 0)
{
    // Chop and insert ellide into title if text is too wide
    QString title = text;
    int width = textWidth ? *textWidth : fontMetrics.width(text);
    QString ellipsis = QLatin1String("...");
    if (width > rect.width()) {
        QString leftHalf = title.left(title.size() / 2);
        QString rightHalf = title.mid(leftHalf.size() + 1);
        while (!leftHalf.isEmpty() && !rightHalf.isEmpty()) {
            leftHalf.chop(1);
            int width = fontMetrics.width(leftHalf + ellipsis + rightHalf);
            if (width < rect.width()) {
                title = leftHalf + ellipsis + rightHalf;
                width = width;
                break;
            }
            rightHalf.remove(0, 1);
            width = fontMetrics.width(leftHalf + ellipsis + rightHalf);
            if (width < rect.width()) {
                title = leftHalf + ellipsis + rightHalf;
                width = width;
                break;
            }
        }
    }
    if (textWidth)
        *textWidth = width;
    return title;
}
#endif

#if !defined(QT_NO_DOCKWIDGET) || !defined(QT_NO_SPLITTER)
static void qt_plastique_draw_handle(QPainter *painter, const QStyleOption *option,
                                     const QRect &rect, Qt::Orientation orientation,
                                     const QWidget *widget)
{
    QColor borderColor = option->palette.background().color().darker(178);
    QColor alphaCornerColor;
    if (widget) {
        // ### backgroundrole/foregroundrole should be part of the style option
        alphaCornerColor = mergedColors(option->palette.color(widget->backgroundRole()), borderColor);
    } else {
        alphaCornerColor = mergedColors(option->palette.background().color(), borderColor);
    }
    QImage handle(qt_simple_toolbarhandle);
    alphaCornerColor.setAlpha(170);
    handle.setColor(1, alphaCornerColor.rgba());
    handle.setColor(2, mergedColors(alphaCornerColor, option->palette.light().color()).rgba());
    handle.setColor(3, option->palette.light().color().rgba());

    const int spacing = 2;

    if (orientation == Qt::Vertical) {
        int nchunks = rect.width() / (handle.width() + spacing);
        for (int i = 0; i < nchunks; ++i)
            painter->drawImage(QPoint(rect.left() + i * (handle.width() + spacing), rect.top()), handle);
    } else {
        int nchunks = rect.height() / (handle.height() + spacing);
        for (int i = 0; i < nchunks; ++i)
            painter->drawImage(QPoint(rect.left(), rect.top() + i * (handle.height() + spacing)), handle);
    }
}
#endif

class QPlastiqueStylePrivate : public QWindowsStylePrivate
{
    Q_DECLARE_PUBLIC(QPlastiqueStyle)
public:
    QPlastiqueStylePrivate();
    virtual ~QPlastiqueStylePrivate();
    void drawPartialFrame(QPainter *painter, const QStyleOptionComplex *option,
                          const QRect &rect, const QWidget *widget) const;

#ifndef QT_NO_PROGRESSBAR
    QList<QProgressBar *> bars;
    int progressBarAnimateTimer;
    QElapsedTimer timer;
#endif
};

/*!
  \internal
 */
QPlastiqueStylePrivate::QPlastiqueStylePrivate() :
    QWindowsStylePrivate()
#ifndef QT_NO_PROGRESSBAR
    , progressBarAnimateTimer(0)
#endif
{
}

/*!
  \internal
 */
QPlastiqueStylePrivate::~QPlastiqueStylePrivate()
{
}

/*!
    \class QPlastiqueStyle
    \brief The QPlastiqueStyle class provides a widget style similar to the
    Plastik style available in KDE.

    The Plastique style provides a default look and feel for widgets on X11
    that closely resembles the Plastik style, introduced by Sandro Giessl in
    KDE 3.2.

    \img qplastiquestyle.png
    \sa QWindowsXPStyle, QMacStyle, QWindowsStyle, QCDEStyle, QMotifStyle
*/

/*!
    Constructs a QPlastiqueStyle object.
*/
QPlastiqueStyle::QPlastiqueStyle()
    : QWindowsStyle(*new QPlastiqueStylePrivate)
{
    setObjectName(QLatin1String("Plastique"));
}

/*!
    Destructs the QPlastiqueStyle object.
*/
QPlastiqueStyle::~QPlastiqueStyle()
{
}

/*
    Used by spin- and combo box.
    Draws a rounded frame around rect but omits the right hand edge
*/
void QPlastiqueStylePrivate::drawPartialFrame(QPainter *painter, const QStyleOptionComplex *option,
                                              const QRect &rect, const QWidget *widget) const
{
    Q_Q(const QPlastiqueStyle);
    bool reverse = option->direction == Qt::RightToLeft;
    QStyleOptionFrame frameOpt;
#ifndef QT_NO_LINEEDIT
    if (QLineEdit *lineedit = widget->findChild<QLineEdit *>())
        frameOpt.initFrom(lineedit);
#else
    Q_UNUSED(widget)
#endif // QT_NO_LINEEDIT

    frameOpt.rect = rect;
    painter->save();
    frameOpt.rect.adjust(-blueFrameWidth + (reverse ? 1 : 0), -blueFrameWidth,
                          blueFrameWidth + (reverse ? 0 : -1), blueFrameWidth);
    painter->setClipRect(frameOpt.rect);
    frameOpt.rect.adjust(reverse ? -2 : 0, 0, reverse ? 0 : 2, 0);
    frameOpt.lineWidth = q->pixelMetric(QStyle::PM_DefaultFrameWidth);
    frameOpt.midLineWidth = 0;
    frameOpt.state = option->state | QStyle::State_Sunken;
    frameOpt.palette = option->palette;
    q->drawPrimitive(QStyle::PE_PanelLineEdit, &frameOpt, painter, widget);
    painter->restore();

    // Draw a two pixel highlight on the flat edge
    if (option->state & QStyle::State_HasFocus) {
        painter->setPen(QPen(option->palette.highlight(), 0));
        QBrush focusBorder = option->palette.highlight();
        qBrushSetAlphaF(&focusBorder, qreal(0.65));
        if (!reverse) {
            painter->drawLine(rect.topRight()    + QPoint(1, -1),
                              rect.bottomRight() + QPoint(1, 1));
            painter->setPen(QPen(focusBorder, 0));
            painter->drawLine(rect.topRight(),
                              rect.bottomRight());
        }
        else {
            painter->drawLine(rect.topLeft()    + QPoint(-1, -1),
                              rect.bottomLeft() + QPoint(-1, 1));
            painter->setPen(QPen(focusBorder, 0));
            painter->drawLine(rect.topLeft(),
                              rect.bottomLeft());
        }
    }
}

/*!
  \reimp
*/
void QPlastiqueStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                                    QPainter *painter, const QWidget *widget) const
{
    Q_ASSERT(option);

    QColor borderColor = option->palette.background().color().darker(178);
    QColor alphaCornerColor;
    if (widget) {
        // ### backgroundrole/foregroundrole should be part of the style option
        alphaCornerColor = mergedColors(option->palette.color(widget->backgroundRole()), borderColor);
    } else {
        alphaCornerColor = mergedColors(option->palette.background().color(), borderColor);
    }
    QColor alphaTextColor = mergedColors(option->palette.background().color(), option->palette.text().color());

    switch (element) {
    case PE_IndicatorButtonDropDown:
        proxy()->drawPrimitive(PE_PanelButtonTool, option, painter, widget);
        break;
    case PE_FrameDefaultButton: {
        if (!(option->state & QStyle::State_Enabled))
            break;
        painter->setPen(QPen(QColor(0, 0, 0, 127), 0));
        const QLine lines[4] = {
            QLine(option->rect.left() + 2, option->rect.top(),
                  option->rect.right() - 2, option->rect.top()),
            QLine(option->rect.left() + 2, option->rect.bottom(),
                  option->rect.right() - 2, option->rect.bottom()),
            QLine(option->rect.left(), option->rect.top() + 2,
                  option->rect.left(), option->rect.bottom() - 2),
            QLine(option->rect.right(), option->rect.top() + 2,
                  option->rect.right(), option->rect.bottom() - 2) };
        painter->drawLines(lines, 4);

        QPoint points[8];
        points[0] = QPoint(option->rect.left() + 1, option->rect.top() + 1);
        points[1] = QPoint(option->rect.right() - 1, option->rect.top() + 1);
        points[2] = QPoint(option->rect.left() + 1, option->rect.bottom() - 1);
        points[3] = QPoint(option->rect.right() - 1, option->rect.bottom() - 1);
        painter->drawPoints(points, 4);

        painter->setPen(QPen(QColor(0, 0, 0, 63), 0));
        points[0] = QPoint(option->rect.left() + 1, option->rect.top());
        points[1] = QPoint(option->rect.right() - 1, option->rect.top());
        points[2] = QPoint(option->rect.left(), option->rect.top() + 1);
        points[3] = QPoint(option->rect.right(), option->rect.top() + 1);
        points[4] = QPoint(option->rect.left() + 1, option->rect.bottom());
        points[5] = QPoint(option->rect.right() - 1, option->rect.bottom());
        points[6] = QPoint(option->rect.left(), option->rect.bottom() - 1);
        points[7] = QPoint(option->rect.right(), option->rect.bottom() - 1);
        painter->drawPoints(points, 8);

        break;
    }
#ifndef QT_NO_TABWIDGET
    case PE_FrameTabWidget:
        if (const QStyleOptionTabWidgetFrame *twf = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option)) {
            if (twf->shape != QTabBar::RoundedNorth && twf->shape != QTabBar::RoundedWest &&
                twf->shape != QTabBar::RoundedSouth && twf->shape != QTabBar::RoundedEast) {
                QWindowsStyle::drawPrimitive(element, option, painter, widget);
                break;
            }

            int borderThickness = proxy()->pixelMetric(PM_TabBarBaseOverlap, twf, widget);
            bool reverse = (twf->direction == Qt::RightToLeft);

            painter->save();

            // Start by filling the contents of the tab widget frame (which is
            // actually a panel).
            painter->fillRect(option->rect.adjusted(1, 1, -1, -1), option->palette.window());

            QRect tabBarRect;
            switch (twf->shape) {
            case QTabBar::RoundedNorth:
                if (reverse)
                    tabBarRect = QRect(twf->rect.right() - twf->leftCornerWidgetSize.width() - twf->tabBarSize.width() + 1, twf->rect.top(), twf->tabBarSize.width(), borderThickness);
                else
                    tabBarRect = QRect(twf->rect.left() + twf->leftCornerWidgetSize.width(), twf->rect.top(), twf->tabBarSize.width(), borderThickness);
                break ;
            case QTabBar::RoundedWest:
                tabBarRect = QRect(twf->rect.left(), twf->rect.top() + twf->leftCornerWidgetSize.height(), borderThickness, twf->tabBarSize.height());
                break ;
            case QTabBar::RoundedEast:
                tabBarRect = QRect(twf->rect.right() - borderThickness + 1, twf->rect.top()  + twf->leftCornerWidgetSize.height(),
                                   borderThickness, twf->tabBarSize.height());
                break ;
            case QTabBar::RoundedSouth:
                if (reverse)
                    tabBarRect = QRect(twf->rect.right() - twf->leftCornerWidgetSize.width() - twf->tabBarSize.width() + 1,
                                       twf->rect.bottom() - borderThickness + 1, twf->tabBarSize.width(), borderThickness);
                else
                    tabBarRect = QRect(twf->rect.left() + twf->leftCornerWidgetSize.width(),
                                       twf->rect.bottom() - borderThickness + 1, twf->tabBarSize.width(), borderThickness);
                break ;
            default:
                break;
            }

            QRegion region(twf->rect);
            region -= tabBarRect;
            painter->setClipRegion(region);

            // Outer border
            QLine leftLine = QLine(twf->rect.topLeft() + QPoint(0, 2), twf->rect.bottomLeft() - QPoint(0, 2));
            QLine rightLine = QLine(twf->rect.topRight() + QPoint(0, 2), twf->rect.bottomRight() - QPoint(0, 2));
            QLine bottomLine = QLine(twf->rect.bottomLeft() + QPoint(2, 0), twf->rect.bottomRight() - QPoint(2, 0));
            QLine topLine = QLine(twf->rect.topLeft() + QPoint(2, 0), twf->rect.topRight() - QPoint(2, 0));

            QBrush border = option->palette.shadow();
            qBrushSetAlphaF(&border, qreal(0.4));
            painter->setPen(QPen(border, 0));

            QVarLengthArray<QLine, 4> lines;
            QVarLengthArray<QPoint, 8> points;

            lines.append(topLine);

            // Inner border
            QLine innerLeftLine = QLine(leftLine.p1() + QPoint(1, 0), leftLine.p2() + QPoint(1, 0));
            QLine innerRightLine = QLine(rightLine.p1() - QPoint(1, 0), rightLine.p2() - QPoint(1, 0));
            QLine innerBottomLine = QLine(bottomLine.p1() - QPoint(0, 1), bottomLine.p2() - QPoint(0, 1));
            QLine innerTopLine = QLine(topLine.p1() + QPoint(0, 1), topLine.p2() + QPoint(0, 1));

            // Rounded Corner
            QPoint leftBottomOuterCorner = QPoint(innerLeftLine.p2() + QPoint(0, 1));
            QPoint leftBottomInnerCorner1 = QPoint(leftLine.p2() + QPoint(0, 1));
            QPoint leftBottomInnerCorner2 = QPoint(bottomLine.p1() - QPoint(1, 0));
            QPoint rightBottomOuterCorner = QPoint(innerRightLine.p2() + QPoint(0, 1));
            QPoint rightBottomInnerCorner1 = QPoint(rightLine.p2() + QPoint(0, 1));
            QPoint rightBottomInnerCorner2 = QPoint(bottomLine.p2() + QPoint(1, 0));
            QPoint rightTopOuterCorner = QPoint(innerRightLine.p1() - QPoint(0, 1));
            QPoint rightTopInnerCorner1 = QPoint(rightLine.p1() - QPoint(0, 1));
            QPoint rightTopInnerCorner2 = QPoint(topLine.p2() + QPoint(1, 0));
            QPoint leftTopOuterCorner = QPoint(innerLeftLine.p1() - QPoint(0, 1));
            QPoint leftTopInnerCorner1 = QPoint(leftLine.p1() - QPoint(0, 1));
            QPoint leftTopInnerCorner2 = QPoint(topLine.p1() - QPoint(1, 0));

            lines.append(leftLine);
            lines.append(rightLine);
            lines.append(bottomLine);

            painter->drawLines(lines.constData(), lines.size());
            lines.clear();

            points.append(leftBottomOuterCorner);
            points.append(rightBottomOuterCorner);
            points.append(rightTopOuterCorner);
            points.append(leftTopOuterCorner);

            painter->drawPoints(points.constData(), points.size());
            points.clear();

            QBrush innerTopLeft = option->palette.shadow();
            qBrushSetAlphaF(&innerTopLeft, qreal(0.075));
            painter->setPen(QPen(innerTopLeft, 0));

            lines.append(innerLeftLine);
            lines.append(innerTopLine);
            painter->drawLines(lines.constData(), lines.size());
            lines.clear();

            QBrush innerBottomRight = option->palette.shadow();
            qBrushSetAlphaF(&innerBottomRight, qreal(0.23));
            painter->setPen(QPen(innerBottomRight, 0));
            lines.append(innerRightLine);
            lines.append(innerBottomLine);
            painter->drawLines(lines.constData(), lines.size());
            lines.clear();

            QBrush corner = option->palette.shadow();
            qBrushSetAlphaF(&corner, 0.25);
            painter->setPen(QPen(corner, 0));
            points.append(leftBottomInnerCorner1);
            points.append(leftBottomInnerCorner2);
            points.append(rightBottomInnerCorner1);
            points.append(rightBottomInnerCorner2);
            points.append(rightTopInnerCorner1);
            points.append(rightTopInnerCorner2);
            points.append(leftTopInnerCorner1);
            points.append(leftTopInnerCorner2);
            painter->drawPoints(points.constData(), points.size());
            points.clear();

            painter->restore();
        }
        break ;
#endif // QT_NO_TABWIDGET
#ifndef QT_NO_TABBAR
    case PE_FrameTabBarBase:
        if (const QStyleOptionTabBarBase *tbb = qstyleoption_cast<const QStyleOptionTabBarBase *>(option)) {
            if (tbb->shape != QTabBar::RoundedNorth && tbb->shape != QTabBar::RoundedWest &&
                tbb->shape != QTabBar::RoundedSouth && tbb->shape != QTabBar::RoundedEast) {
                QWindowsStyle::drawPrimitive(element, option, painter, widget);
                break;
            }

            painter->save();

            QRegion region(tbb->rect);
            region -= tbb->tabBarRect;
            painter->setClipRegion(region);

            QLine topLine = QLine(tbb->rect.bottomLeft() - QPoint(0, 1), tbb->rect.bottomRight() - QPoint(0, 1));
            QLine bottomLine = QLine(tbb->rect.bottomLeft(), tbb->rect.bottomRight());

            QBrush border = option->palette.shadow();
            qBrushSetAlphaF(&border, qreal(0.4));
            QBrush innerTopLeft = option->palette.shadow();
            qBrushSetAlphaF(&innerTopLeft, qreal(0.075));
            QBrush innerBottomRight = option->palette.shadow();
            qBrushSetAlphaF(&innerBottomRight, qreal(0.23));
            QBrush corner = option->palette.shadow();
            qBrushSetAlphaF(&corner, 0.25);

            if (tbb->shape == QTabBar::RoundedSouth)
                painter->setPen(QPen(corner, 0));
            else
                painter->setPen(QPen(border, 0));
            painter->drawLine(topLine);

            if (tbb->shape != QTabBar::RoundedSouth)
                painter->setPen(QPen(innerTopLeft, 0));
            else
                painter->setPen(QPen(border, 0));
            painter->drawLine(bottomLine);

            painter->restore();
        }
        break ;
#endif // QT_NO_TABBAR
#ifndef QT_NO_GROUPBOX
    case PE_FrameGroupBox:
        if (const QStyleOptionFrame *frame = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            QStyleOptionFrameV2 frameV2(*frame);
            if (frameV2.features & QStyleOptionFrameV2::Flat) {
                QPen oldPen = painter->pen();
                painter->setPen(borderColor);
                painter->drawLine(frameV2.rect.topLeft(), frameV2.rect.topRight());
                painter->setPen(oldPen);
            } else {
                frameV2.state &= ~(State_Sunken | State_HasFocus);
                proxy()->drawPrimitive(PE_Frame, &frameV2, painter, widget);
            }
        }
        break;
#endif // QT_NO_GROUPBOX
    case PE_Frame: {
        QFrame::Shadow shadow = QFrame::Plain;
        if (option->state & State_Sunken)
            shadow = QFrame::Sunken;
        else if (option->state & State_Raised)
            shadow = QFrame::Raised;
        qt_plastique_draw_frame(painter, option->rect, option, shadow);
        break;
    }
#ifndef QT_NO_LINEEDIT
    case PE_FrameLineEdit:
        qt_plastique_draw_frame(painter, option->rect, option, QFrame::Sunken);
        break;
    case PE_PanelLineEdit:
        if (const QStyleOptionFrame *lineEdit = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            // Panel of a line edit inside combo box or spin box is drawn in CC_ComboBox and CC_SpinBox
            if (widget) {
#ifndef QT_NO_SPINBOX
                // Spinbox doesn't need a separate palette for the lineedit
                if (qobject_cast<const QAbstractSpinBox *>(widget->parentWidget()))
                    break;
#endif
            }

            painter->save();

            // Fill the line edit insides
            QRect filledRect = lineEdit->rect.adjusted(1, 1, -1, -1);
            QBrush baseBrush = qMapBrushToRect(lineEdit->palette.base(), filledRect);
            painter->setBrushOrigin(filledRect.topLeft());
            painter->fillRect(filledRect.adjusted(1, 1, -1, -1), baseBrush);

            painter->setPen(QPen(baseBrush, 0));
            const QLine lines[4] = {
                QLine(filledRect.left(), filledRect.top() + 1,
                      filledRect.left(), filledRect.bottom() - 1),
                QLine(filledRect.right(), filledRect.top() + 1,
                      filledRect.right(), filledRect.bottom() - 1),
                QLine(filledRect.left() + 1, filledRect.top(),
                      filledRect.right() - 1, filledRect.top()),
                QLine(filledRect.left() + 1, filledRect.bottom(),
                      filledRect.right() - 1, filledRect.bottom()) };
            painter->drawLines(lines, 4);

            if (lineEdit->lineWidth != 0)
                qt_plastique_draw_frame(painter, option->rect, option, QFrame::Sunken);

            painter->restore();
            break;
        }
#endif // QT_NO_LINEEDIT
    case PE_FrameDockWidget:
    case PE_FrameMenu:
    case PE_FrameStatusBarItem: {
        // Draws the frame around a popup menu.
        QPen oldPen = painter->pen();
        painter->setPen(borderColor);
        painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
        painter->setPen(alphaCornerColor);
        const QPoint points[4] = {
            QPoint(option->rect.topLeft()),
            QPoint(option->rect.topRight()),
            QPoint(option->rect.bottomLeft()),
            QPoint(option->rect.bottomRight()) };
        painter->drawPoints(points, 4);
        painter->setPen(oldPen);
        break;
    }
#ifdef QT3_SUPPORT
    case PE_Q3DockWindowSeparator: {
        QPen oldPen = painter->pen();
        painter->setPen(alphaCornerColor);
        QRect rect = option->rect;
        if (option->state & State_Horizontal) {
            painter->drawLine(rect.right(), rect.top() + 2, rect.right(), rect.bottom() - 1);
        } else {
            painter->drawLine(rect.left() + 2, rect.bottom(), rect.right() - 1, rect.bottom());
        }
        painter->setPen(oldPen);
        break;
    }
    case PE_Q3Separator: {
        QPen oldPen = painter->pen();
        painter->setPen(alphaCornerColor);
        if ((option->state & State_Horizontal) == 0)
            painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
        else
            painter->drawLine(option->rect.topRight(), option->rect.bottomRight());
        painter->setPen(option->palette.background().color().lighter(104));
        if ((option->state & State_Horizontal) == 0)
            painter->drawLine(option->rect.topLeft(), option->rect.topRight());
        else
            painter->drawLine(option->rect.topLeft(), option->rect.bottomLeft());
        painter->setPen(oldPen);
        break;
    }
#endif // QT3_SUPPORT
#ifndef QT_NO_MAINWINDOW
    case PE_PanelMenuBar:
        if ((widget && qobject_cast<const QMainWindow *>(widget->parentWidget()))
#ifdef QT3_SUPPORT
            || (widget && widget->parentWidget() && widget->parentWidget()->inherits("Q3MainWindow"))
#endif
            ) {
            // Draws the light line above and the dark line below menu bars and
            // tool bars.
            QPen oldPen = painter->pen();
            if (element == PE_PanelMenuBar || (option->state & State_Horizontal)) {
                painter->setPen(alphaCornerColor);
                painter->drawLine(option->rect.left(), option->rect.bottom(),
                                  option->rect.right(), option->rect.bottom());
                painter->setPen(option->palette.background().color().lighter(104));
                painter->drawLine(option->rect.left(), option->rect.top(),
                                  option->rect.right(), option->rect.top());
            } else {
                painter->setPen(option->palette.background().color().lighter(104));
                painter->drawLine(option->rect.left(), option->rect.top(),
                                  option->rect.left(), option->rect.bottom());
                painter->setPen(alphaCornerColor);
                painter->drawLine(option->rect.right(), option->rect.top(),
                                  option->rect.right(), option->rect.bottom());
            }
            painter->setPen(oldPen);
        }
        break;
#endif // QT_NO_MAINWINDOW
    case PE_IndicatorHeaderArrow: {
        bool usedAntialiasing = painter->renderHints() & QPainter::Antialiasing;
        if (!usedAntialiasing)
            painter->setRenderHint(QPainter::Antialiasing);
        QWindowsStyle::drawPrimitive(element, option, painter, widget);
        if (!usedAntialiasing)
            painter->setRenderHint(QPainter::Antialiasing, false);
        break;
    }
    case PE_PanelButtonTool:
        // Draws a tool button (f.ex., in QToolBar and QTabBar)
        if ((option->state & State_Enabled || option->state & State_On) || !(option->state & State_AutoRaise))
            qt_plastique_drawShadedPanel(painter, option, true, widget);
        break;
#ifndef QT_NO_TOOLBAR
    case PE_IndicatorToolBarHandle: {
        QPixmap cache;
        QRect rect = option->rect;
#ifdef QT3_SUPPORT
        if (widget && widget->inherits("Q3DockWindowHandle") && widget->parentWidget()->inherits("Q3DockWindow")) {
            if (!(option->state & State_Horizontal))
                rect.adjust(2, 0, -2, 0);
        }
#endif
        QString pixmapName = QStyleHelper::uniqueName(QLatin1String("toolbarhandle"), option, rect.size());
        if (!QPixmapCache::find(pixmapName, cache)) {
            cache = QPixmap(rect.size());
            cache.fill(Qt::transparent);
            QPainter cachePainter(&cache);
            QRect cacheRect(QPoint(0, 0), rect.size());
            if (widget)
                cachePainter.fillRect(cacheRect, option->palette.brush(widget->backgroundRole()));
            else
                cachePainter.fillRect(cacheRect, option->palette.background());

            QImage handle(qt_toolbarhandle);
            alphaCornerColor.setAlpha(170);
            handle.setColor(1, alphaCornerColor.rgba());
            handle.setColor(2, mergedColors(alphaCornerColor, option->palette.light().color()).rgba());
            handle.setColor(3, option->palette.light().color().rgba());

            if (option->state & State_Horizontal) {
                int nchunks = cacheRect.height() / handle.height();
                int indent = (cacheRect.height() - (nchunks * handle.height())) / 2;
                for (int i = 0; i < nchunks; ++i)
                    cachePainter.drawImage(QPoint(cacheRect.left() + 3, cacheRect.top() + indent + i * handle.height()),
                                           handle);
            } else {
                int nchunks = cacheRect.width() / handle.width();
                int indent = (cacheRect.width() - (nchunks * handle.width())) / 2;
                for (int i = 0; i < nchunks; ++i)
                    cachePainter.drawImage(QPoint(cacheRect.left() + indent + i * handle.width(), cacheRect.top() + 3),
                                           handle);
            }
            cachePainter.end();
            QPixmapCache::insert(pixmapName, cache);
        }
        painter->drawPixmap(rect.topLeft(), cache);
        break;
    }
    case PE_IndicatorToolBarSeparator: {
        QPen oldPen = painter->pen();
        painter->setPen(alphaCornerColor);
        if (option->state & State_Horizontal) {
            painter->drawLine(option->rect.left(), option->rect.top() + 1, option->rect.left(), option->rect.bottom() - 2);
            painter->setPen(option->palette.base().color());
            painter->drawLine(option->rect.right(), option->rect.top() + 1, option->rect.right(), option->rect.bottom() - 2);
        } else {
            painter->drawLine(option->rect.left() + 1, option->rect.top(), option->rect.right() - 2, option->rect.top());
            painter->setPen(option->palette.base().color());
            painter->drawLine(option->rect.left() + 1, option->rect.bottom(), option->rect.right() - 2, option->rect.bottom());
        }
        painter->setPen(oldPen);
        break;
    }
#endif // QT_NO_TOOLBAR
    case PE_PanelButtonCommand:
        if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            bool sunken = (button->state & State_Sunken) || (button->state & State_On);
            if ((button->features & QStyleOptionButton::Flat) && !sunken)
                break;

            bool defaultButton = (button->features & (QStyleOptionButton::DefaultButton
                                                      | QStyleOptionButton::AutoDefaultButton));

            BEGIN_STYLE_PIXMAPCACHE(QString::fromLatin1("pushbutton-%1").arg(defaultButton))

            QPen oldPen = p->pen();
            bool hover = (button->state & State_Enabled) && (button->state & State_MouseOver);

            // Give the painter a different brush origin for sunken buttons
            if (sunken) {
                // ### No such function
                // p->setPenOrigin(rect.left() + 1, rect.top() + 1);
                p->setBrushOrigin(rect.left() + 1, rect.top() + 1);
            }

            // Draw border
            qt_plastique_draw_frame(p, rect, option);

            // Fill the panel
            QRectF fillRect = rect.adjusted(2, 2, -2, -2);

            // Button colors
            QBrush alphaCornerBrush = qMapBrushToRect(qBrushDark(option->palette.button(), 165), rect);
            qBrushSetAlphaF(&alphaCornerBrush, 0.5);
            QBrush buttonGradientBrush;
            QBrush leftLineGradientBrush;
            QBrush rightLineGradientBrush;
            QBrush sunkenButtonGradientBrush;
            QBrush sunkenLeftLineGradientBrush;
            QBrush sunkenRightLineGradientBrush;
            QBrush buttonBrush = qMapBrushToRect(option->palette.button(), rect);
            if (buttonBrush.gradient() || !buttonBrush.texture().isNull()) {
                buttonGradientBrush = buttonBrush;
                sunkenButtonGradientBrush = qBrushDark(buttonBrush, 108);
                leftLineGradientBrush = qBrushLight(buttonBrush, 105);
                rightLineGradientBrush = qBrushDark(buttonBrush, 105);
                sunkenLeftLineGradientBrush = qBrushDark(buttonBrush, 110);
                sunkenRightLineGradientBrush = qBrushDark(buttonBrush, 106);
            } else {
                // Generate gradients
                QLinearGradient buttonGradient(rect.topLeft(), rect.bottomLeft());
                if (hover) {
                    buttonGradient.setColorAt(0.0, mergedColors(option->palette.highlight().color(),
                                                                buttonBrush.color().lighter(104), 6));
                    buttonGradient.setColorAt(1.0, mergedColors(option->palette.highlight().color(),
                                                                buttonBrush.color().darker(110), 6));
                } else {
                    buttonGradient.setColorAt(0.0, buttonBrush.color().lighter(104));
                    buttonGradient.setColorAt(1.0, buttonBrush.color().darker(110));
                }
                buttonGradientBrush = QBrush(buttonGradient);

                QLinearGradient buttonGradient2(rect.topLeft(), rect.bottomLeft());
                buttonGradient2.setColorAt(0.0, buttonBrush.color().darker(113));
                buttonGradient2.setColorAt(1.0, buttonBrush.color().darker(103));
                sunkenButtonGradientBrush = QBrush(buttonGradient2);

                QLinearGradient buttonGradient3(rect.topLeft(), rect.bottomLeft());
                buttonGradient3.setColorAt(0.0, buttonBrush.color().lighter(105));
                buttonGradient3.setColorAt(1.0, buttonBrush.color());
                leftLineGradientBrush = QBrush(buttonGradient3);

                QLinearGradient buttonGradient4(rect.topLeft(), rect.bottomLeft());
                buttonGradient4.setColorAt(0.0, buttonBrush.color());
                buttonGradient4.setColorAt(1.0, buttonBrush.color().darker(110));
                rightLineGradientBrush = QBrush(buttonGradient4);

                QLinearGradient buttonGradient5(rect.topLeft(), rect.bottomLeft());
                buttonGradient5.setColorAt(0.0, buttonBrush.color().darker(113));
                buttonGradient5.setColorAt(1.0, buttonBrush.color().darker(107));
                sunkenLeftLineGradientBrush = QBrush(buttonGradient5);

                QLinearGradient buttonGradient6(rect.topLeft(), rect.bottomLeft());
                buttonGradient6.setColorAt(0.0, buttonBrush.color().darker(108));
                buttonGradient6.setColorAt(1.0, buttonBrush.color().darker(103));
                sunkenRightLineGradientBrush = QBrush(buttonGradient6);
            }

            // Main fill
            p->fillRect(fillRect,
                              qMapBrushToRect(sunken ? sunkenButtonGradientBrush
                                              : buttonGradientBrush, rect));

            // Top line
            p->setPen(QPen(qBrushLight(qMapBrushToRect(sunken ? sunkenButtonGradientBrush
                                            : buttonGradientBrush, rect), 105), 0));
            p->drawLine(QPointF(rect.left() + 2, rect.top() + 1),
                              QPointF(rect.right() - 2, rect.top() + 1));

            // Bottom line
            p->setPen(QPen(qBrushDark(qMapBrushToRect(sunken ? sunkenButtonGradientBrush
                                            : buttonGradientBrush, rect), 105), 0));
            p->drawLine(QPointF(rect.left() + 2, rect.bottom() - 1),
                              QPointF(rect.right() - 2, rect.bottom() - 1));

            // Left line
            p->setPen(QPen(qMapBrushToRect(sunken ? sunkenLeftLineGradientBrush
                                                 : leftLineGradientBrush, rect), 1));
            p->drawLine(QPointF(rect.left() + 1, rect.top() + 2),
                              QPointF(rect.left() + 1, rect.bottom() - 2));

            // Right line
            p->setPen(QPen(qMapBrushToRect(sunken ? sunkenRightLineGradientBrush
                                                 : rightLineGradientBrush, rect), 1));
            p->drawLine(QPointF(rect.right() - 1, rect.top() + 2),
                              QPointF(rect.right() - 1, rect.bottom() - 2));

            // Hovering
            if (hover && !sunken) {
                QBrush hover = qMapBrushToRect(option->palette.highlight(), rect);
                QBrush hoverOuter = hover;
                qBrushSetAlphaF(&hoverOuter, qreal(0.7));

                QLine lines[2];

                p->setPen(QPen(hoverOuter, 0));
                lines[0] = QLine(rect.left() + 1, rect.top() + 1, rect.right() - 1, rect.top() + 1);
                lines[1] = QLine(rect.left() + 1, rect.bottom() - 1, rect.right() - 1, rect.bottom() - 1);
                p->drawLines(lines, 2);

                QBrush hoverInner = hover;
                qBrushSetAlphaF(&hoverInner, qreal(0.45));
                p->setPen(QPen(hoverInner, 0));
                lines[0] = QLine(rect.left() + 1, rect.top() + 2, rect.right() - 1, rect.top() + 2);
                lines[1] = QLine(rect.left() + 1, rect.bottom() - 2, rect.right() - 1, rect.bottom() - 2);
                p->drawLines(lines, 2);

                QBrush hoverSide = hover;
                qBrushSetAlphaF(&hoverSide, qreal(0.075));
                p->setPen(QPen(hoverSide, 0));
                lines[0] = QLine(rect.left() + 1, rect.top() + 2, rect.left() + 1, rect.bottom() - 2);
                lines[1] = QLine(rect.right() - 1, rect.top() + 2, rect.right() - 1, rect.bottom() - 2);
                p->drawLines(lines, 2);
            }

            p->setPen(oldPen);

            END_STYLE_PIXMAPCACHE
        }
        break;
    case PE_IndicatorCheckBox:
        if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            BEGIN_STYLE_PIXMAPCACHE(QLatin1String("checkbox"))

            p->save();

            // Outline
            QBrush border = option->palette.shadow();
            qBrushSetAlphaF(&border, qreal(0.4));
            p->setPen(QPen(border, 0));
            const QLine lines[4] = {
                QLine(rect.left() + 1, rect.top(), rect.right() - 1, rect.top()),
                QLine(rect.left() + 1, rect.bottom(), rect.right() - 1, rect.bottom()),
                QLine(rect.left(), rect.top() + 1, rect.left(), rect.bottom() - 1),
                QLine(rect.right(), rect.top() + 1, rect.right(), rect.bottom() - 1) };
            p->drawLines(lines, 4);

            QBrush corner = option->palette.shadow();
            qBrushSetAlphaF(&corner, qreal(0.2));
            p->setPen(QPen(corner, 0));
            const QPoint points[4] = {
                rect.topLeft(), rect.topRight(),
                rect.bottomLeft(), rect.bottomRight() };
            p->drawPoints(points, 4);

            // Fill
            QBrush baseBrush = qMapBrushToRect(button->palette.base(), rect);
            if (!baseBrush.gradient() && baseBrush.texture().isNull()) {
                QLinearGradient gradient(rect.center().x(), rect.top(), rect.center().x(), rect.bottom());
                gradient.setColorAt(0, baseBrush.color());
                gradient.setColorAt(1, baseBrush.color().darker(105));
                baseBrush = gradient;
            }
            p->fillRect(rect.adjusted(1, 1, -1, -1), baseBrush);

            // Hover
            if ((button->state & State_Enabled) && (button->state & State_MouseOver)) {
                QBrush pen = qMapBrushToRect(button->palette.highlight(), rect);
                qBrushSetAlphaF(&pen, qreal(0.8));
                p->setPen(QPen(pen, 0));
                p->drawRect(rect.adjusted(1, 1, -2, -2));
                qBrushSetAlphaF(&pen, 0.5);
                p->setPen(QPen(pen, 0));
                p->drawRect(rect.adjusted(2, 2, -3, -3));

                qBrushSetAlphaF(&pen, qreal(0.2));
                p->setBrush(pen);
                p->drawRect(rect.adjusted(2, 2, -3, -3));
            }

            // Indicator
            bool on = button->state & State_On;
            bool sunken = button->state & State_Sunken;
            bool unchanged = button->state & State_NoChange;
            bool enabled = button->state & State_Enabled;
            if (on || (enabled && sunken) || unchanged) {
                p->setRenderHint(QPainter::Antialiasing);
                QBrush pointBrush = qMapBrushToRect(button->palette.text(), rect);
                if (sunken)
                    qBrushSetAlphaF(&pointBrush, qreal(0.5));
                else if (unchanged)
                    qBrushSetAlphaF(&pointBrush, qreal(0.3));
                p->setPen(QPen(pointBrush, 3));
                const QLine lines[2] = {
                    QLine(rect.left() + 4, rect.top() + 4, rect.right() - 3, rect.bottom() - 3),
                    QLine(rect.right() - 3, rect.top() + 4, rect.left() + 4, rect.bottom() - 3) };
                p->drawLines(lines, 2);
            }

            p->restore();
            END_STYLE_PIXMAPCACHE
        }
        break;
    case PE_IndicatorRadioButton:
        if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            BEGIN_STYLE_PIXMAPCACHE(QLatin1String("radiobutton"))

            p->save();
            p->setRenderHint(QPainter::Antialiasing);

            // The the filled ellipse
            QBrush border = qMapBrushToRect(option->palette.shadow(), rect);
            qBrushSetAlphaF(&border, qreal(0.51));
            p->setPen(QPen(border, 0));

            QBrush baseBrush = qMapBrushToRect(button->palette.base(), rect);
            if (!baseBrush.gradient() && baseBrush.texture().isNull()) {
                QLinearGradient gradient(rect.center().x(), rect.top(), rect.center().x(), rect.bottom());
                gradient.setColorAt(0, baseBrush.color());
                gradient.setColorAt(1, baseBrush.color().darker(105));
                baseBrush = gradient;
            }
            p->setBrush(baseBrush);
            p->drawEllipse(QRectF(rect).adjusted(1, 1, -1, -1));

            // Hover
            if ((button->state & State_Enabled) && (button->state & State_MouseOver)) {
                QBrush pen = qMapBrushToRect(button->palette.highlight(), rect);
                qBrushSetAlphaF(&pen, qreal(0.8));
                p->setPen(QPen(pen, 0));
                qBrushSetAlphaF(&pen, qreal(0.2));
                p->setBrush(pen);
                p->drawEllipse(QRectF(rect).adjusted(2, 2, -2, -2));
            }

            // Indicator
            bool on = button->state & State_On;
            bool sunken = button->state & State_Sunken;
            bool enabled = button->state & State_Enabled;
            if (on || (enabled && sunken)) {
                p->setPen(Qt::NoPen);
                QBrush pointBrush = qMapBrushToRect(button->palette.text(), rect);
                if (sunken)
                    qBrushSetAlphaF(&pointBrush, 0.5);
                p->setBrush(pointBrush);
                p->drawEllipse(QRectF(rect).adjusted(3, 3, -3, -3));
            }

            p->restore();
            END_STYLE_PIXMAPCACHE
        }
        break;
#ifndef QT_NO_DOCKWIDGET
    case PE_IndicatorDockWidgetResizeHandle:
        if ((option->state & State_Enabled) && (option->state & State_MouseOver))
            painter->fillRect(option->rect, QColor(255, 255, 255, 128));
        if (option->state & State_Horizontal) {
            int width = option->rect.width() / 3;
            QRect rect(option->rect.center().x() - width / 2,
                       option->rect.top() + (option->rect.height() / 2) - 1, width, 3);
            qt_plastique_draw_handle(painter, option, rect, Qt::Vertical, widget);
        } else {
            int height = option->rect.height() / 3;
            QRect rect(option->rect.left() + (option->rect.width() / 2 - 1),
                       option->rect.center().y() - height / 2, 3, height);
            qt_plastique_draw_handle(painter, option, rect, Qt::Horizontal, widget);
        }
        break;
#endif // QT_NO_DOCKWIDGET
    case PE_IndicatorViewItemCheck: {
        QStyleOptionButton button;
        button.QStyleOption::operator=(*option);
        button.state &= ~State_MouseOver;
        proxy()->drawPrimitive(PE_IndicatorCheckBox, &button, painter, widget);
        break;
    }
    case PE_FrameWindow: {
        painter->save();
        bool active = (option->state & State_Active);
        int titleBarStop = option->rect.top() + proxy()->pixelMetric(PM_TitleBarHeight, option, widget);

        QPalette palette = option->palette;
        if (!active)
            palette.setCurrentColorGroup(QPalette::Disabled);

        // Frame and rounded corners
        painter->setPen(mergedColors(palette.highlight().color(), Qt::black, 50));

        QLine lines[3];
        QPoint points[4];

        // bottom border line
        lines[0] = QLine(option->rect.left() + 1, option->rect.bottom(), option->rect.right() - 1, option->rect.bottom());

        // bottom left and right side border lines
        lines[1] = QLine(option->rect.left(), titleBarStop, option->rect.left(), option->rect.bottom() - 1);
        lines[2] = QLine(option->rect.right(), titleBarStop, option->rect.right(), option->rect.bottom() - 1);
        painter->drawLines(lines, 3);
        points[0] = QPoint(option->rect.left() + 1, option->rect.bottom() - 1);
        points[1] = QPoint(option->rect.right() - 1, option->rect.bottom() - 1);
        painter->drawPoints(points, 2);

#ifdef QT3_SUPPORT
        if (widget && widget->inherits("Q3DockWindow")) {
            // also draw the frame on the title bar
            lines[0] = QLine(option->rect.left() + 1, option->rect.top(),
                             option->rect.right() - 1, option->rect.top());
            lines[1] = QLine(option->rect.left(), option->rect.top() + 1,
                             option->rect.left(), titleBarStop);
            lines[2] = QLine(option->rect.right(), option->rect.top() + 1,
                             option->rect.right(), titleBarStop);
            painter->drawLines(lines, 3);
        }
#endif

        // alpha corners
        painter->setPen(mergedColors(palette.highlight().color(), palette.background().color(), 55));
        points[0] = QPoint(option->rect.left() + 2, option->rect.bottom() - 1);
        points[1] = QPoint(option->rect.left() + 1, option->rect.bottom() - 2);
        points[2] = QPoint(option->rect.right() - 2, option->rect.bottom() - 1);
        points[3] = QPoint(option->rect.right() - 1, option->rect.bottom() - 2);
        painter->drawPoints(points, 4);

#ifdef QT3_SUPPORT
        if (widget && widget->inherits("Q3DockWindow")) {
            // also draw the frame on the title bar
            points[0] = option->rect.topLeft();
            points[1] = option->rect.topRight();
            painter->drawPoints(points, 2);
        }
#endif

        // upper and lower left inner
        painter->setPen(active ? mergedColors(palette.highlight().color(), palette.background().color()) : palette.background().color().darker(120));
        painter->drawLine(option->rect.left() + 1, titleBarStop, option->rect.left() + 1, option->rect.bottom() - 2);

#ifdef QT3_SUPPORT
        if (widget && widget->inherits("Q3DockWindow")) {
            // also draw the frame on the title bar
            lines[0] = QLine(option->rect.left() + 1, option->rect.top() + 1,
                             option->rect.left() + 1, titleBarStop);
            lines[1] = QLine(option->rect.right() - 1, option->rect.top() + 1,
                             option->rect.right() - 1, titleBarStop);
            lines[2] = QLine(option->rect.left() + 1, option->rect.top() + 1,
                             option->rect.right() - 1, option->rect.top() + 1);
            painter->drawLines(lines, 3);
        }
#endif

        painter->setPen(active ? mergedColors(palette.highlight().color(), palette.background().color(), 57) : palette.background().color().darker(130));
        lines[0] = QLine(option->rect.right() - 1, titleBarStop, option->rect.right() - 1, option->rect.bottom() - 2);
        lines[1] = QLine(option->rect.left() + 1, option->rect.bottom() - 1, option->rect.right() - 1, option->rect.bottom() - 1);
        painter->drawLines(lines, 2);

        painter->restore();
    }
        break;
    case PE_IndicatorBranch: {
        int mid_h = option->rect.x() + option->rect.width() / 2;
        int mid_v = option->rect.y() + option->rect.height() / 2;
        int bef_h = mid_h;
        int bef_v = mid_v;
        int aft_h = mid_h;
        int aft_v = mid_v;
        QBrush brush(option->palette.dark().color(), Qt::Dense4Pattern);
        if (option->state & State_Item) {
            if (option->direction == Qt::RightToLeft)
                painter->fillRect(option->rect.left(), mid_v, bef_h - option->rect.left(), 1, brush);
            else
                painter->fillRect(aft_h, mid_v, option->rect.right() - aft_h + 1, 1, brush);
        }
        if (option->state & State_Sibling)
            painter->fillRect(mid_h, aft_v, 1, option->rect.bottom() - aft_v + 1, brush);
        if (option->state & (State_Open | State_Children | State_Item | State_Sibling))
            painter->fillRect(mid_h, option->rect.y(), 1, bef_v - option->rect.y(), brush);

        if (option->state & State_Children) {
            painter->save();
            QPoint center = option->rect.center();
            // border
            QRect fullRect(center.x() - 4, center.y() - 4, 9, 9);
            painter->setPen(borderColor);

            const QLine lines[4] = {
                QLine(fullRect.left() + 1, fullRect.top(),
                      fullRect.right() - 1, fullRect.top()),
                QLine(fullRect.left() + 1, fullRect.bottom(),
                      fullRect.right() - 1, fullRect.bottom()),
                QLine(fullRect.left(), fullRect.top() + 1,
                      fullRect.left(), fullRect.bottom() - 1),
                QLine(fullRect.right(), fullRect.top() + 1,
                      fullRect.right(), fullRect.bottom() - 1) };
            painter->drawLines(lines, 4);

            // "antialiased" corners
            painter->setPen(alphaCornerColor);
            const QPoint points[4] = {
                fullRect.topLeft(),
                fullRect.topRight(),
                fullRect.bottomLeft(),
                fullRect.bottomRight() };
            painter->drawPoints(points, 4);

            // fill
            QRect adjustedRect = fullRect;
            QRect gradientRect(adjustedRect.left() + 1, adjustedRect.top() + 1,
                               adjustedRect.right() - adjustedRect.left() - 1,
                               adjustedRect.bottom() - adjustedRect.top() - 1);
            if (option->palette.base().style() == Qt::SolidPattern) {
                QColor baseGradientStartColor = option->palette.base().color().darker(101);
                QColor baseGradientStopColor = option->palette.base().color().darker(106);
                qt_plastique_draw_gradient(painter, gradientRect, baseGradientStartColor, baseGradientStopColor);
            } else {
                painter->fillRect(gradientRect, option->palette.base());
            }
            // draw "+" or "-"
            painter->setPen(alphaTextColor);
            painter->drawLine(center.x() - 2, center.y(), center.x() + 2, center.y());
            if (!(option->state & State_Open))
                painter->drawLine(center.x(), center.y() - 2, center.x(), center.y() + 2);
            painter->restore();
        }
    }
        break;
    default:
        QWindowsStyle::drawPrimitive(element, option, painter, widget);
        break;
    }
}

/*!
  \reimp
*/
void QPlastiqueStyle::drawControl(ControlElement element, const QStyleOption *option,
                                  QPainter *painter, const QWidget *widget) const
{
    QColor borderColor = option->palette.background().color().darker(178);
    QColor alphaCornerColor;
    if (widget) {
        // ### backgroundrole/foregroundrole should be part of the style option
        alphaCornerColor = mergedColors(option->palette.color(widget->backgroundRole()), borderColor);
    } else {
        alphaCornerColor = mergedColors(option->palette.background().color(), borderColor);
    }

    QColor gradientStartColor = option->palette.button().color().lighter(104);
    QColor gradientStopColor = option->palette.button().color().darker(105);

    QColor highlightedGradientStartColor = option->palette.button().color().lighter(101);
    QColor highlightedGradientStopColor = mergedColors(option->palette.button().color(), option->palette.highlight().color(), 85);

    QColor lightShadowGradientStartColor = highlightedGradientStartColor.lighter(105);
    QColor lightShadowGradientStopColor = highlightedGradientStopColor.lighter(105);

    QColor highlightedDarkInnerBorderColor = mergedColors(option->palette.button().color(), option->palette.highlight().color(), 35);
    QColor highlightedLightInnerBorderColor = mergedColors(option->palette.button().color(), option->palette.highlight().color(), 58);

    QColor alphaInnerColor = mergedColors(highlightedDarkInnerBorderColor, option->palette.base().color());

    switch (element) {
#ifndef QT_NO_TABBAR
    case CE_TabBarTabShape:
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {

            if (tab->shape != QTabBar::RoundedNorth && tab->shape != QTabBar::RoundedWest &&
                tab->shape != QTabBar::RoundedSouth && tab->shape != QTabBar::RoundedEast) {
                QWindowsStyle::drawControl(element, option, painter, widget);
                break;
            }

            painter->save();

            // Set up some convenience variables
            bool disabled = !(tab->state & State_Enabled);
            bool onlyTab = tab->position == QStyleOptionTab::OnlyOneTab;
            bool selected = tab->state & State_Selected;
            bool mouseOver = (tab->state & State_MouseOver) && !selected && !disabled;
            bool previousSelected = tab->selectedPosition == QStyleOptionTab::PreviousIsSelected;
            bool nextSelected = tab->selectedPosition == QStyleOptionTab::NextIsSelected;
            bool leftCornerWidget = (tab->cornerWidgets & QStyleOptionTab::LeftCornerWidget);
            bool reverse = (tab->direction == Qt::RightToLeft);

            int lowerTop = selected ? 0 : 3; // to make the selected tab bigger than the rest
            bool atEnd = (tab->position == QStyleOptionTab::End) || onlyTab;
            bool atBeginning = ((tab->position == QStyleOptionTab::Beginning) || onlyTab)
                               && !leftCornerWidget;
            bool reverseShadow = false;

            int borderThickness = proxy()->pixelMetric(PM_TabBarBaseOverlap, tab, widget);
            int marginLeft = 0;
            if ((atBeginning && !selected) || (selected && leftCornerWidget && ((tab->position == QStyleOptionTab::Beginning) || onlyTab))) {
                marginLeft = 1;
            }

            // I've set the names based on the natural coordinate system. Vectors are used to rotate everything
            // if the orientation of the tab bare is different than north.
            {
                // Coordinates of corners of rectangle for transformation
                QPoint topLeft;
                QPoint topRight;
                QPoint bottomLeft;
                QPoint bottomRight;

                // Fill with normalized vectors in the direction of the coordinate system
                // (down and right should be complement of up and left, or it will look odd)
                QPoint vectorUp;
                QPoint vectorDown;
                QPoint vectorLeft;
                QPoint vectorRight;

                QBrush border = option->palette.shadow();
                qBrushSetAlphaF(&border, qreal(0.4));
                QBrush innerTopLeft = option->palette.shadow();
                qBrushSetAlphaF(&innerTopLeft, qreal(0.075));
                QBrush innerBottomRight = option->palette.shadow();
                qBrushSetAlphaF(&innerBottomRight, qreal(0.23));
                QBrush corner = option->palette.shadow();
                qBrushSetAlphaF(&corner, qreal(0.25));

                QBrush baseColor1;
                QBrush baseColor2;

                switch (tab->shape) {
                case QTabBar::RoundedNorth:
                    vectorUp = QPoint(0, -1);
                    vectorDown = QPoint(0, 1);

                    if (reverse) {
                        vectorLeft = QPoint(1, 0);
                        vectorRight = QPoint(-1, 0);
                        reverseShadow = true;
                    } else {
                        vectorLeft = QPoint(-1, 0);
                        vectorRight = QPoint(1, 0);
                    }

                    if (reverse) {
                        topLeft = tab->rect.topRight();
                        topRight = tab->rect.topLeft();
                        bottomLeft = tab->rect.bottomRight();
                        bottomRight = tab->rect.bottomLeft();
                    } else {
                        topLeft = tab->rect.topLeft();
                        topRight = tab->rect.topRight();
                        bottomLeft = tab->rect.bottomLeft();
                        bottomRight = tab->rect.bottomRight();
                    }


                    baseColor1 = border;
                    baseColor2 = innerTopLeft;
                    break ;
                case QTabBar::RoundedWest:
                    vectorUp = QPoint(-1, 0);
                    vectorDown = QPoint(1, 0);
                    vectorLeft = QPoint(0, -1);
                    vectorRight = QPoint(0, 1);

                    topLeft = tab->rect.topLeft();
                    topRight = tab->rect.bottomLeft();
                    bottomLeft = tab->rect.topRight();
                    bottomRight = tab->rect.bottomRight();

                    baseColor1 = border;
                    baseColor2 = innerTopLeft;
                    break ;
                case QTabBar::RoundedEast:
                    vectorUp = QPoint(1, 0);
                    vectorDown = QPoint(-1, 0);
                    vectorLeft = QPoint(0, -1);
                    vectorRight = QPoint(0, 1);

                    topLeft = tab->rect.topRight();
                    topRight = tab->rect.bottomRight();
                    bottomLeft = tab->rect.topLeft();
                    bottomRight = tab->rect.bottomLeft();

                    baseColor1 = border;
                    baseColor2 = innerBottomRight;
                    break ;
                case QTabBar::RoundedSouth:
                    vectorUp = QPoint(0, 1);
                    vectorDown = QPoint(0, -1);

                    if (reverse) {
                        vectorLeft = QPoint(1, 0);
                        vectorRight = QPoint(-1, 0);
                        reverseShadow = true;

                        topLeft = tab->rect.bottomRight();
                        topRight = tab->rect.bottomLeft();
                        bottomLeft = tab->rect.topRight();
                        bottomRight = tab->rect.topLeft();
                    } else {
                        vectorLeft = QPoint(-1, 0);
                        vectorRight = QPoint(1, 0);

                        topLeft = tab->rect.bottomLeft();
                        topRight = tab->rect.bottomRight();
                        bottomLeft = tab->rect.topLeft();
                        bottomRight = tab->rect.topRight();
                    }

                    baseColor1 = border;
                    baseColor2 = innerBottomRight;
                    break ;
                default:
                    break;
                }

                // Make the tab smaller when it's at the end, so that we are able to draw the corner
                if (atEnd) {
                    topRight += vectorLeft;
                    bottomRight += vectorLeft;
                }

                {
                    // Outer border
                    QLine topLine;
                    {
                        QPoint adjustTopLineLeft = (vectorRight * (marginLeft + (previousSelected ? 0 : 1))) +
                                                   (vectorDown * lowerTop);
                        QPoint adjustTopLineRight = (vectorDown * lowerTop);
                        if (atBeginning || selected)
                            adjustTopLineLeft += vectorRight;
                        if (atEnd || selected)
                            adjustTopLineRight += 2 * vectorLeft;

                        topLine = QLine(topLeft + adjustTopLineLeft, topRight + adjustTopLineRight);
                    }

                    QLine leftLine;
                    {
                        QPoint adjustLeftLineTop = (vectorRight * marginLeft) + (vectorDown * (lowerTop + 1));
                        QPoint adjustLeftLineBottom = (vectorRight * marginLeft) + (vectorUp * borderThickness);
                        if (atBeginning || selected)
                            adjustLeftLineTop += vectorDown; // Make place for rounded corner
                        if (atBeginning && selected)
                            adjustLeftLineBottom += borderThickness * vectorDown;
                        else if (selected)
                            adjustLeftLineBottom += vectorUp;

                        leftLine = QLine(topLeft + adjustLeftLineTop, bottomLeft + adjustLeftLineBottom);
                    }

                    QLine rightLine;
                    {
                        QPoint adjustRightLineTop = vectorDown * (2 + lowerTop);
                        QPoint adjustRightLineBottom = vectorUp * borderThickness;
                        if (selected)
                            adjustRightLineBottom += vectorUp;

                        rightLine = QLine(topRight + adjustRightLineTop, bottomRight + adjustRightLineBottom);
                    }

                    // Background
                    QPoint startPoint = topLine.p1() + vectorDown + vectorLeft;
                    if (mouseOver)
                        startPoint += vectorDown;
                    QPoint endPoint = rightLine.p2();

                    if (tab->state & State_Enabled) {
                        QRect fillRect = QRect(startPoint, endPoint).normalized();
                        if (fillRect.isValid()) {
                            if (selected) {
                                fillRect = QRect(startPoint, endPoint + vectorLeft + vectorDown * 3).normalized();
                                painter->fillRect(fillRect, option->palette.window());

                                // Connect to the base
                                painter->setPen(QPen(option->palette.window(), 0));
                                QVarLengthArray<QPoint, 6> points;
                                points.append(rightLine.p2() + vectorDown);
                                points.append(rightLine.p2() + vectorDown + vectorDown);
                                points.append(rightLine.p2() + vectorDown + vectorDown + vectorRight);
                                if (tab->position != QStyleOptionTab::Beginning) {
                                    points.append(leftLine.p2() + vectorDown);
                                    points.append(leftLine.p2() + vectorDown + vectorDown);
                                    points.append(leftLine.p2() + vectorDown + vectorDown + vectorLeft);
                                }
                                painter->drawPoints(points.constData(), points.size());
                            } else {
                                QBrush buttonGradientBrush;
                                QBrush buttonBrush = qMapBrushToRect(option->palette.button(), fillRect);
                                if (buttonBrush.gradient() || !buttonBrush.texture().isNull()) {
                                    buttonGradientBrush = buttonBrush;
                                } else {
                                    // Generate gradients
                                    QLinearGradient buttonGradient(fillRect.topLeft(), fillRect.bottomLeft());
                                    buttonGradient.setColorAt(0.0, buttonBrush.color().lighter(104));
                                    buttonGradient.setColorAt(1.0, buttonBrush.color().darker(110));
                                    buttonGradientBrush = QBrush(buttonGradient);
                                }

                                painter->fillRect(fillRect, buttonGradientBrush);
                            }
                        }
                    }

                    QPoint rightCornerDot = topRight + vectorLeft + (lowerTop + 1)*vectorDown;
                    QPoint leftCornerDot = topLeft + (marginLeft + 1)*vectorRight + (lowerTop + 1)*vectorDown;
                    QPoint bottomRightConnectToBase = rightLine.p2() + vectorRight + vectorDown;
                    QPoint bottomLeftConnectToBase = leftLine.p2() + vectorLeft + vectorDown;

                    painter->setPen(QPen(border, 0));

                    QVarLengthArray<QLine, 3> lines;
                    QVarLengthArray<QPoint, 7> points;

                    lines.append(topLine);

                    if (mouseOver) {
                        painter->drawLines(lines.constData(), lines.count());
                        lines.clear();

                        QLine secondHoverLine = QLine(topLine.p1() + vectorDown * 2 + vectorLeft, topLine.p2() + vectorDown * 2 + vectorRight);
                        painter->setPen(highlightedLightInnerBorderColor);
                        painter->drawLine(secondHoverLine);
                    }

                    if (mouseOver)
                        painter->setPen(QPen(border, 0));

                    if (!previousSelected)
                        lines.append(leftLine);
                    if (atEnd || selected) {
                        lines.append(rightLine);
                        points.append(rightCornerDot);
                    }
                    if (atBeginning || selected)
                        points.append(leftCornerDot);
                    if (selected) {
                        points.append(bottomRightConnectToBase);
                        points.append(bottomLeftConnectToBase);
                    }
                    if (lines.size() > 0) {
                        painter->drawLines(lines.constData(), lines.size());
                        lines.clear();
                    }
                    if (points.size() > 0) {
                        painter->drawPoints(points.constData(), points.size());
                        points.clear();
                    }

                    // Antialiasing
                    painter->setPen(QPen(corner, 0));
                    if (atBeginning || selected)
                        points.append(topLine.p1() + vectorLeft);
                    if (!previousSelected)
                        points.append(leftLine.p1() + vectorUp);
                    if (atEnd || selected) {
                        points.append(topLine.p2() + vectorRight);
                        points.append(rightLine.p1() + vectorUp);
                    }

                    if (selected) {
                        points.append(bottomRightConnectToBase + vectorLeft);
                        if (!atBeginning) {
                            points.append(bottomLeftConnectToBase + vectorRight);

                            if (((tab->position == QStyleOptionTab::Beginning) || onlyTab) && leftCornerWidget) {
                                // A special case: When the first tab is selected and
                                // has a left corner widget, it needs to do more work
                                // to connect to the base
                                QPoint p1 = bottomLeftConnectToBase + vectorDown;

                                points.append(p1);
                            }
                        }
                    }
                    if (points.size() > 0) {
                        painter->drawPoints(points.constData(), points.size());
                        points.clear();
                    }

                    // Inner border
                    QLine innerTopLine = QLine(topLine.p1() + vectorDown, topLine.p2() + vectorDown);
                    if (!selected) {
                        QLinearGradient topLineGradient(innerTopLine.p1(),innerTopLine.p2());
                        topLineGradient.setColorAt(0, lightShadowGradientStartColor);
                        topLineGradient.setColorAt(1, lightShadowGradientStopColor);
                        painter->setPen(QPen(mouseOver ? QBrush(highlightedDarkInnerBorderColor) : QBrush(topLineGradient), 1));
                    } else {
                        painter->setPen(QPen(innerTopLeft, 0));
                    }
                    painter->drawLine(innerTopLine);

                    QLine innerLeftLine = QLine(leftLine.p1() + vectorRight + vectorDown, leftLine.p2() + vectorRight);
                    QLine innerRightLine = QLine(rightLine.p1() + vectorLeft + vectorDown, rightLine.p2() + vectorLeft);

                    if (selected) {
                        innerRightLine = QLine(innerRightLine.p1() + vectorUp, innerRightLine.p2());
                        innerLeftLine = QLine(innerLeftLine.p1() + vectorUp, innerLeftLine.p2());
                    }

                    if (selected || atBeginning) {
                        QBrush leftLineGradientBrush;
                        QRect rect = QRect(innerLeftLine.p1(), innerLeftLine.p2()).normalized();
                        QBrush buttonBrush = qMapBrushToRect(option->palette.button(), rect);
                        if (buttonBrush.gradient() || !buttonBrush.texture().isNull()) {
                            leftLineGradientBrush = qBrushLight(buttonBrush, 105);
                        } else {
                            QLinearGradient buttonGradient3(rect.topLeft(), rect.bottomLeft());
                            buttonGradient3.setColorAt(0.0, buttonBrush.color().lighter(105));
                            buttonGradient3.setColorAt(1.0, buttonBrush.color());
                            leftLineGradientBrush = QBrush(buttonGradient3);
                        }

                        if (!selected)
                            painter->setPen(QPen(leftLineGradientBrush, 0));

                        // Assume the sun is on the same side in Right-To-Left layouts and draw the
                        // light shadow on the left side always (the right line is on the left side in
                        // reverse layouts for north and south)
                        if (reverseShadow)
                            painter->drawLine(innerRightLine);
                        else
                            painter->drawLine(innerLeftLine);
                    }

                    if (atEnd || selected) {
                        if (!selected) {
                            QBrush rightLineGradientBrush;
                            QRect rect = QRect(innerRightLine.p1(), innerRightLine.p2()).normalized();
                            QBrush buttonBrush = qMapBrushToRect(option->palette.button(), rect);
                            if (buttonBrush.gradient() || !buttonBrush.texture().isNull()) {
                                rightLineGradientBrush = qBrushDark(buttonBrush, 105);
                            } else {
                                QLinearGradient buttonGradient4(rect.topLeft(), rect.bottomLeft());
                                buttonGradient4.setColorAt(0.0, buttonBrush.color());
                                buttonGradient4.setColorAt(1.0, buttonBrush.color().darker(110));
                                rightLineGradientBrush = QBrush(buttonGradient4);
                            }

                            painter->setPen(QPen(rightLineGradientBrush, 0));
                        } else {
                            painter->setPen(QPen(innerBottomRight, 0));
                        }

                        if (reverseShadow)
                            painter->drawLine(innerLeftLine);
                        else
                            painter->drawLine(innerRightLine);
                    }


                    // Base
                    QLine baseLine = QLine(bottomLeft + marginLeft * 2 * vectorRight, bottomRight);
                    {

                        QPoint adjustedLeft;
                        QPoint adjustedRight;

                        if (atEnd && !selected) {
                            baseLine = QLine(baseLine.p1(), baseLine.p2() + vectorRight);
                        }

                        if (nextSelected) {
                            adjustedRight += vectorLeft;
                            baseLine = QLine(baseLine.p1(), baseLine.p2() + vectorLeft);
                        }
                        if (previousSelected) {
                            adjustedLeft += vectorRight;
                            baseLine = QLine(baseLine.p1() + vectorRight, baseLine.p2());
                        }
                        if (atBeginning)
                            adjustedLeft += vectorRight;

                        painter->setPen(QPen(baseColor2, 0));
                        if (!selected)
                            painter->drawLine(baseLine);

                        if (atEnd && !selected)
                            painter->drawPoint(baseLine.p2() + vectorRight);

                        if (atBeginning && !selected)
                            adjustedLeft = vectorRight;
                        else
                            adjustedLeft = QPoint(0, 0);
                        painter->setPen(QPen(baseColor1, 0));
                        if (!selected)
                            painter->drawLine(bottomLeft + vectorUp + adjustedLeft, baseLine.p2() + vectorUp);

                        QPoint endPoint = bottomRight + vectorUp;
                        if (atEnd && !selected)
                            painter->drawPoint(endPoint);

                        // For drawing a lower left "fake" corner on the base when the first tab is unselected
                        if (atBeginning && !selected) {
                            painter->drawPoint(baseLine.p1() + vectorLeft);
                        }

                        painter->setPen(QPen(corner, 0));
                        if (nextSelected)
                            painter->drawPoint(endPoint);
                        else if (selected)
                            painter->drawPoint(endPoint + vectorRight);

                        // For drawing a lower left "fake" corner on the base when the first tab is unselected
                        if (atBeginning && !selected) {
                            painter->drawPoint(baseLine.p1() + 2 * vectorLeft);
                        }
                    }
                }
            }

            // Yay we're done

            painter->restore();
        }
        break;
#endif // QT_NO_TABBAR
#ifndef QT_NO_PROGRESSBAR
    case CE_ProgressBarGroove:
        if (const QStyleOptionProgressBar *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            QRect rect = bar->rect;
            QPen oldPen = painter->pen();

            QLine lines[4];

            // outline
            painter->setPen(borderColor);
            lines[0] = QLine(rect.left() + 2, rect.top(), rect.right() - 2, rect.top());
            lines[1] = QLine(rect.left() + 2, rect.bottom(), rect.right() - 2, rect.bottom());
            lines[2] = QLine(rect.left(), rect.top() + 2, rect.left(), rect.bottom() - 2);
            lines[3] = QLine(rect.right(), rect.top() + 2, rect.right(), rect.bottom() - 2);
            painter->drawLines(lines, 4);

            QPoint points[8];
            points[0] = QPoint(rect.left() + 1, rect.top() + 1);
            points[1] = QPoint(rect.right() - 1, rect.top() + 1);
            points[2] = QPoint(rect.left() + 1, rect.bottom() - 1);
            points[3] = QPoint(rect.right() - 1, rect.bottom() - 1);
            painter->drawPoints(points, 4);

            // alpha corners
            painter->setPen(alphaCornerColor);
            points[0] = QPoint(rect.left(), rect.top() + 1);
            points[1] = QPoint(rect.left() + 1, rect.top());
            points[2] = QPoint(rect.right(), rect.top() + 1);
            points[3] = QPoint(rect.right() - 1, rect.top());
            points[4] = QPoint(rect.left(), rect.bottom() - 1);
            points[5] = QPoint(rect.left() + 1, rect.bottom());
            points[6] = QPoint(rect.right(), rect.bottom() - 1);
            points[7] = QPoint(rect.right() - 1, rect.bottom());
            painter->drawPoints(points, 8);

            // inner outline, north-west
            painter->setPen(gradientStartColor.darker(105));
            lines[0] = QLine(rect.left() + 2, rect.top() + 1, rect.right() - 2, rect.top() + 1);
            lines[1] = QLine(rect.left() + 1, rect.top() + 2, rect.left() + 1, rect.bottom() - 2);
            painter->drawLines(lines, 2);

            // base of the groove
            painter->setPen(QPen());
            painter->fillRect(rect.adjusted(2, 2, -2, -1), QBrush(bar->palette.base().color()));
            painter->setPen(bar->palette.base().color());
            painter->drawLine(rect.right() - 1, rect.top() + 2, rect.right() - 1, rect.bottom() - 2);

            painter->setPen(oldPen);
        }
        break;
    case CE_ProgressBarLabel:
        if (const QStyleOptionProgressBar *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            // The busy indicator doesn't draw a label
            if (bar->minimum == 0 && bar->maximum == 0)
                return;

            painter->save();

            QRect rect = bar->rect;
            QRect leftRect;

            QFont font;
            font.setBold(true);
            painter->setFont(font);
            painter->setPen(bar->palette.text().color());

            bool vertical = false;
            bool inverted = false;
            bool bottomToTop = false;
            // Get extra style options if version 2
            if (const QStyleOptionProgressBarV2 *bar2 = qstyleoption_cast<const QStyleOptionProgressBarV2 *>(option)) {
                vertical = (bar2->orientation == Qt::Vertical);
                inverted = bar2->invertedAppearance;
                bottomToTop = bar2->bottomToTop;
            }

            if (vertical) {
                rect = QRect(rect.left(), rect.top(), rect.height(), rect.width()); // flip width and height
                QTransform m;
                if (bottomToTop) {
                    m.translate(0.0, rect.width());
                    m.rotate(-90);
                } else {
                    m.translate(rect.height(), 0.0);
                    m.rotate(90);
                }
                painter->setTransform(m, true);
            }

            int progressIndicatorPos = (bar->progress - qreal(bar->minimum)) / qMax(qreal(1.0), qreal(bar->maximum) - bar->minimum) * rect.width();

            bool flip = (!vertical && (((bar->direction == Qt::RightToLeft) && !inverted)
                                       || ((bar->direction == Qt::LeftToRight) && inverted))) || (vertical && ((!inverted && !bottomToTop) || (inverted && bottomToTop)));
            if (flip) {
                int indicatorPos = rect.width() - progressIndicatorPos;
                if (indicatorPos >= 0 && indicatorPos <= rect.width()) {
                    painter->setPen(bar->palette.base().color());
                    leftRect = QRect(rect.left(), rect.top(), indicatorPos, rect.height());
                } else if (indicatorPos > rect.width()) {
                    painter->setPen(bar->palette.text().color());
                } else {
                    painter->setPen(bar->palette.base().color());
                }
            } else {
                if (progressIndicatorPos >= 0 && progressIndicatorPos <= rect.width()) {
                    leftRect = QRect(rect.left(), rect.top(), progressIndicatorPos, rect.height());
                } else if (progressIndicatorPos > rect.width()) {
                    painter->setPen(bar->palette.base().color());
                } else {
                    painter->setPen(bar->palette.text().color());
                }
            }

            QRegion rightRect = rect;
            rightRect = rightRect.subtracted(leftRect);
            painter->setClipRegion(rightRect);
            painter->drawText(rect, bar->text, QTextOption(Qt::AlignAbsolute | Qt::AlignHCenter | Qt::AlignVCenter));
            if (!leftRect.isNull()) {
                painter->setPen(flip ? bar->palette.text().color() : bar->palette.base().color());
                painter->setClipRect(leftRect);
                painter->drawText(rect, bar->text, QTextOption(Qt::AlignAbsolute | Qt::AlignHCenter | Qt::AlignVCenter));
            }

            painter->restore();
        }
        break;
    case CE_ProgressBarContents:
        if (const QStyleOptionProgressBar *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            Q_D(const QPlastiqueStyle);
            QRect rect = bar->rect;
            bool vertical = false;
            bool inverted = false;
            bool indeterminate = (bar->minimum == 0 && bar->maximum == 0);
            if (!indeterminate && bar->progress == -1)
                break;

            painter->save();

            // Get extra style options if version 2
            if (const QStyleOptionProgressBarV2 *bar2 = qstyleoption_cast<const QStyleOptionProgressBarV2 *>(option)) {
                vertical = (bar2->orientation == Qt::Vertical);
                inverted = bar2->invertedAppearance;
            }

            // If the orientation is vertical, we use a transform to rotate
            // the progress bar 90 degrees clockwise.  This way we can use the
            // same rendering code for both orientations.
            if (vertical) {
                rect = QRect(rect.left(), rect.top(), rect.height(), rect.width()); // flip width and height
                QTransform m = QTransform::fromTranslate(rect.height()-1, 0);
                m.rotate(90.0);
                painter->setTransform(m, true);
            }

            int maxWidth = rect.width() - 4;
            int minWidth = 4;
            qint64 progress = qMax<qint64>(bar->progress, bar->minimum); // workaround for bug in QProgressBar
            double vc6_workaround = ((progress - qint64(bar->minimum)) / qMax(double(1.0), double(qint64(bar->maximum) - qint64(bar->minimum))) * maxWidth);
            int width = indeterminate ? maxWidth : qMax(int(vc6_workaround), minWidth);
            bool reverse = (!vertical && (bar->direction == Qt::RightToLeft)) || vertical;
            if (inverted)
                reverse = !reverse;

            QRect progressBar;
            if (!indeterminate) {
                if (!reverse) {
                    progressBar.setRect(rect.left() + 2, rect.top() + 2, width, rect.height() - 4);
                } else {
                    progressBar.setRect(rect.right() - 1 - width, rect.top() + 2, width, rect.height() - 4);
                }
            } else {
                int slideWidth = ((rect.width() - 4) * 2) / 3;
                int step = ((d->animateStep * slideWidth) / ProgressBarFps) % slideWidth;
                if ((((d->animateStep * slideWidth) / ProgressBarFps) % (2 * slideWidth)) >= slideWidth)
                    step = slideWidth - step;
                progressBar.setRect(rect.left() + 2 + step, rect.top() + 2,
                                    slideWidth / 2, rect.height() - 4);
            }

            // outline
            painter->setPen(highlightedDarkInnerBorderColor);

            QVarLengthArray<QLine, 4> lines;
            QVarLengthArray<QPoint, 8> points;
            if (!reverse) {
                if (width == minWidth) {
                    points.append(QPoint(progressBar.left() + 1, progressBar.top()));
                    points.append(QPoint(progressBar.left() + 1, progressBar.bottom()));
                } else {
                    if (indeterminate) {
                        lines.append(QLine(progressBar.left() + 2, progressBar.top(),
                                           progressBar.right() - 2, progressBar.top()));
                        lines.append(QLine(progressBar.left() + 2, progressBar.bottom(),
                                           progressBar.right() - 2, progressBar.bottom()));
                    } else {
                        lines.append(QLine(progressBar.left() + 1, progressBar.top(),
                                           progressBar.right() - 2, progressBar.top()));
                        lines.append(QLine(progressBar.left() + 1, progressBar.bottom(),
                                           progressBar.right() - 2, progressBar.bottom()));
                    }
                }

                if (indeterminate) {
                    lines.append(QLine(progressBar.left(), progressBar.top() + 2,
                                       progressBar.left(), progressBar.bottom() - 2));
                } else {
                    lines.append(QLine(progressBar.left(), progressBar.top() + 1,
                                       progressBar.left(), progressBar.bottom() - 1));
                }
                lines.append(QLine(progressBar.right(), progressBar.top() + 2,
                                   progressBar.right(), progressBar.bottom() - 2));
            } else {
                if (width == minWidth) {
                    points.append(QPoint(progressBar.right() - 1, progressBar.top()));
                    points.append(QPoint(progressBar.right() - 1, progressBar.bottom()));
                } else {
                    if (indeterminate) {
                        lines.append(QLine(progressBar.right() - 2, progressBar.top(),
                                           progressBar.left() + 2, progressBar.top()));
                        lines.append(QLine(progressBar.right() - 2, progressBar.bottom(),
                                           progressBar.left() + 2, progressBar.bottom()));
                    } else {
                        lines.append(QLine(progressBar.right() - 1, progressBar.top(),
                                           progressBar.left() + 2, progressBar.top()));
                        lines.append(QLine(progressBar.right() - 1, progressBar.bottom(),
                                           progressBar.left() + 2, progressBar.bottom()));
                    }
                }
                if (indeterminate) {
                    lines.append(QLine(progressBar.right(), progressBar.top() + 2,
                                       progressBar.right(), progressBar.bottom() - 2));
                } else {
                    lines.append(QLine(progressBar.right(), progressBar.top() + 1,
                                       progressBar.right(), progressBar.bottom() - 1));
                }
                lines.append(QLine(progressBar.left(), progressBar.top() + 2,
                                   progressBar.left(), progressBar.bottom() - 2));
            }

            if (points.size() > 0) {
                painter->drawPoints(points.constData(), points.size());
                points.clear();
            }
            painter->drawLines(lines.constData(), lines.size());
            lines.clear();

            // alpha corners
            painter->setPen(alphaInnerColor);
            if (!reverse) {
                if (indeterminate) {
                    points.append(QPoint(progressBar.left() + 1, progressBar.top()));
                    points.append(QPoint(progressBar.left(), progressBar.top() + 1));
                    points.append(QPoint(progressBar.left() + 1, progressBar.bottom()));
                    points.append(QPoint(progressBar.left(), progressBar.bottom() - 1));
                } else {
                    points.append(QPoint(progressBar.left(), progressBar.top()));
                    points.append(QPoint(progressBar.left(), progressBar.bottom()));
                }
                points.append(QPoint(progressBar.right() - 1, progressBar.top()));
                points.append(QPoint(progressBar.right(), progressBar.top() + 1));
                points.append(QPoint(progressBar.right() - 1, progressBar.bottom()));
                points.append(QPoint(progressBar.right(), progressBar.bottom() - 1));
            } else {
                if (indeterminate) {
                    points.append(QPoint(progressBar.right() - 1, progressBar.top()));
                    points.append(QPoint(progressBar.right(), progressBar.top() + 1));
                    points.append(QPoint(progressBar.right() - 1, progressBar.bottom()));
                    points.append(QPoint(progressBar.right(), progressBar.bottom() - 1));
                } else {
                    points.append(QPoint(progressBar.right(), progressBar.top()));
                    points.append(QPoint(progressBar.right(), progressBar.bottom()));
                }
                points.append(QPoint(progressBar.left() + 1, progressBar.top()));
                points.append(QPoint(progressBar.left(), progressBar.top() + 1));
                points.append(QPoint(progressBar.left() + 1, progressBar.bottom()));
                points.append(QPoint(progressBar.left(), progressBar.bottom() - 1));
            }

            painter->drawPoints(points.constData(), points.size());
            points.clear();

            // contents
            painter->setPen(QPen());

            QString progressBarName = QStyleHelper::uniqueName(QLatin1String("progressBarContents"),
                                                 option, rect.size());
            QPixmap cache;
            if (!QPixmapCache::find(progressBarName, cache) && rect.height() > 7) {
                QSize size = rect.size();
                cache = QPixmap(QSize(size.width() - 6 + 30, size.height() - 6));
                cache.fill(Qt::white);
                QPainter cachePainter(&cache);
                QRect pixmapRect(0, 0, cache.width(), cache.height());

                int leftEdge = 0;
                bool flip = false;
                while (leftEdge < cache.width() + 1) {
                    QColor rectColor = option->palette.highlight().color();
                    QColor lineColor = option->palette.highlight().color();
                    if (flip) {
                        flip = false;
                        rectColor = rectColor.lighter(105);
                        lineColor = lineColor.lighter(105);
                    } else {
                        flip = true;
                    }

                    cachePainter.setPen(lineColor);
                    const QLine cacheLines[2] = {
                        QLine(pixmapRect.left() + leftEdge - 1, pixmapRect.top(),
                              pixmapRect.left() + leftEdge + 9, pixmapRect.top()),
                        QLine(pixmapRect.left() + leftEdge - 1, pixmapRect.bottom(),
                              pixmapRect.left() + leftEdge + 9, pixmapRect.bottom()) };
                    cachePainter.drawLines(cacheLines, 2);
                    cachePainter.fillRect(QRect(pixmapRect.left() + leftEdge, pixmapRect.top(),
                                                10, pixmapRect.height()), rectColor);

                    leftEdge += 10;
                }

                QPixmapCache::insert(progressBarName, cache);
            }
            painter->setClipRect(progressBar.adjusted(1, 0, -1, -1));

            if (!vertical)
                progressBar.adjust(0, 1, 0, 1);
            if (!indeterminate) {
                int step = (AnimateProgressBar || (indeterminate && AnimateBusyProgressBar)) ? (d->animateStep % 20) : 0;
                if (reverse)
                    painter->drawPixmap(progressBar.left() - 25 + step, progressBar.top(), cache);
                else
                    painter->drawPixmap(progressBar.left() - 25 - step + width % 20, progressBar.top(), cache);
            } else {
                painter->drawPixmap(progressBar.left(), progressBar.top(), cache);
            }

            painter->restore();
        }
        break;
#endif // QT_NO_PROGRESSBAR
    case CE_HeaderSection:
        // Draws the header in tables.
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            QPixmap cache;
            QString pixmapName = QStyleHelper::uniqueName(QLatin1String("headersection"), option, option->rect.size());
            pixmapName += QString::number(- int(header->position));
            pixmapName += QString::number(- int(header->orientation));

            if (!QPixmapCache::find(pixmapName, cache)) {
                cache = QPixmap(option->rect.size());
                cache.fill(Qt::white);
                QRect pixmapRect(0, 0, option->rect.width(), option->rect.height());
                QPainter cachePainter(&cache);

                bool sunken = (header->state & State_Enabled) && (header->state & State_Sunken);

                QColor headerGradientStart = sunken ? option->palette.background().color().darker(114) : gradientStartColor;
                QColor headerGradientStop = sunken ? option->palette.background().color().darker(106) : gradientStopColor;

                QColor lightLine = sunken ? option->palette.background().color().darker(118) : gradientStartColor;
                QColor darkLine = sunken ? option->palette.background().color().darker(110) : gradientStopColor.darker(105);

                qt_plastique_draw_gradient(&cachePainter, pixmapRect,
                                           headerGradientStart, headerGradientStop);

                cachePainter.setPen(borderColor);
                cachePainter.drawRect(pixmapRect.adjusted(0, 0, -1, -1));
                cachePainter.setPen(alphaCornerColor);

                const QPoint points[4] = {
                    pixmapRect.topLeft(), pixmapRect.topRight(),
                    pixmapRect.bottomLeft(), pixmapRect.bottomRight() };
                cachePainter.drawPoints(points, 4);

                QLine lines[2];

                // inner lines
                cachePainter.setPen(lightLine);
                lines[0] = QLine(pixmapRect.left() + 2, pixmapRect.top() + 1,
                                 pixmapRect.right() - 2, pixmapRect.top() + 1);
                lines[1] = QLine(pixmapRect.left() + 1, pixmapRect.top() + 2,
                                 pixmapRect.left() + 1, pixmapRect.bottom() - 2);
                cachePainter.drawLines(lines, 2);

                cachePainter.setPen(darkLine);
                lines[0] = QLine(pixmapRect.left() + 2, pixmapRect.bottom() - 1,
                                 pixmapRect.right() - 2, pixmapRect.bottom() - 1);
                lines[1] = QLine(pixmapRect.right() - 1, pixmapRect.bottom() - 2,
                                 pixmapRect.right() - 1, pixmapRect.top() + 2);
                cachePainter.drawLines(lines, 2);

                cachePainter.end();
                QPixmapCache::insert(pixmapName, cache);
            }
            painter->drawPixmap(option->rect.topLeft(), cache);

        }
        break;
#ifndef QT_NO_MENU
    case CE_MenuItem:
        // Draws one item in a popup menu.
        if (const QStyleOptionMenuItem *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            painter->save();
            QBrush textBrush;
            if (option->palette.resolve() & (1 << QPalette::ButtonText))
                textBrush = option->palette.buttonText();
            else
                textBrush = option->palette.windowText(); // KDE uses windowText rather than buttonText for menus

            if (menuItem->menuItemType == QStyleOptionMenuItem::Separator) {
                painter->fillRect(menuItem->rect, option->palette.background().color().lighter(103));

                int w = 0;
                if (!menuItem->text.isEmpty()) {
                    painter->setFont(menuItem->font);
                    proxy()->drawItemText(painter, menuItem->rect.adjusted(5, 0, -5, 0), Qt::AlignLeft | Qt::AlignVCenter,
                                 menuItem->palette, menuItem->state & State_Enabled, menuItem->text,
                                 QPalette::Text);
                    w = menuItem->fontMetrics.width(menuItem->text) + 5;
                }

                painter->setPen(alphaCornerColor);
                bool reverse = menuItem->direction == Qt::RightToLeft;
                painter->drawLine(menuItem->rect.left() + 5 + (reverse ? 0 : w), menuItem->rect.center().y(),
                                  menuItem->rect.right() - 5 - (reverse ? w : 0), menuItem->rect.center().y());

                painter->restore();
                break;
            }

            bool selected = menuItem->state & State_Selected;
            bool checkable = menuItem->checkType != QStyleOptionMenuItem::NotCheckable;
            bool checked = menuItem->checked;

            if (selected) {
                qt_plastique_draw_gradient(painter, menuItem->rect,
                                           option->palette.highlight().color().lighter(105),
                                           option->palette.highlight().color().darker(110));

                painter->setPen(option->palette.highlight().color().lighter(110));
                painter->drawLine(option->rect.topLeft(), option->rect.topRight());
                painter->setPen(option->palette.highlight().color().darker(115));
                painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
            } else {
                painter->fillRect(option->rect, option->palette.background().color().lighter(103));
            }

            // Check
            QRect checkRect(option->rect.left() + 7, option->rect.center().y() - 6, 13, 13);
            checkRect = visualRect(menuItem->direction, menuItem->rect, checkRect);
            if (checkable) {
                if ((menuItem->checkType & QStyleOptionMenuItem::Exclusive) && menuItem->icon.isNull()) {
                    QStyleOptionButton button;
                    button.rect = checkRect;
                    button.state = menuItem->state;
                    if (checked)
                        button.state |= State_On;
                    button.palette = menuItem->palette;
                    proxy()->drawPrimitive(PE_IndicatorRadioButton, &button, painter, widget);
                } else {
                    if (menuItem->icon.isNull()) {
                        QStyleOptionButton button;
                        button.rect = checkRect;
                        button.state = menuItem->state;
                        if (checked)
                            button.state |= State_On;
                        button.palette = menuItem->palette;
                        proxy()->drawPrimitive(PE_IndicatorCheckBox, &button, painter, widget);
                    } else if (checked) {
                        int iconSize = qMax(menuItem->maxIconWidth, 20);
                        QRect sunkenRect(option->rect.left() + 1,
                                         option->rect.top() + (option->rect.height() - iconSize) / 2 + 1,
                                         iconSize, iconSize);
                        sunkenRect = visualRect(menuItem->direction, menuItem->rect, sunkenRect);

                        QStyleOption opt = *option;
                        opt.state |= State_Sunken;
                        opt.rect = sunkenRect;
                        qt_plastique_drawShadedPanel(painter, &opt, false, widget);
                    }
                }
            }

            // Text and icon, ripped from windows style
            bool dis = !(menuItem->state & State_Enabled);
            bool act = menuItem->state & State_Selected;
            const QStyleOption *opt = option;
            const QStyleOptionMenuItem *menuitem = menuItem;
            int checkcol = qMax(menuitem->maxIconWidth, 20);
            QPainter *p = painter;
            QRect vCheckRect = visualRect(opt->direction, menuitem->rect,
                                          QRect(menuitem->rect.x(), menuitem->rect.y(),
                                                checkcol, menuitem->rect.height()));
            if (!menuItem->icon.isNull()) {
                QIcon::Mode mode = dis ? QIcon::Disabled : QIcon::Normal;
                if (act && !dis)
                    mode = QIcon::Active;
                QPixmap pixmap;
                if (checked)
                    pixmap = menuItem->icon.pixmap(pixelMetric(PM_SmallIconSize, option, widget), mode, QIcon::On);
                else
                    pixmap = menuItem->icon.pixmap(pixelMetric(PM_SmallIconSize, option, widget), mode);
                int pixw = pixmap.width();
                int pixh = pixmap.height();

                QRect pmr(0, 0, pixw, pixh);
                pmr.moveCenter(vCheckRect.center());
                painter->setPen(textBrush.color());
                if (checkable && checked)
                    painter->drawPixmap(QPoint(pmr.left() + 1, pmr.top() + 1), pixmap);
                else
                    painter->drawPixmap(pmr.topLeft(), pixmap);
            }

            if (selected) {
                painter->setPen(menuItem->palette.highlightedText().color());
            } else {
                painter->setPen(textBrush.color());
            }
            int x, y, w, h;
            menuitem->rect.getRect(&x, &y, &w, &h);
            int tab = menuitem->tabWidth;
            QColor discol;
            if (dis) {
                discol = textBrush.color();
                p->setPen(discol);
            }
            int xm = windowsItemFrame + checkcol + windowsItemHMargin;
            int xpos = menuitem->rect.x() + xm;
            QRect textRect(xpos, y + windowsItemVMargin, w - xm - windowsRightBorder - tab + 1, h - 2 * windowsItemVMargin);
            QRect vTextRect = visualRect(opt->direction, menuitem->rect, textRect);
            QString s = menuitem->text;
            if (!s.isEmpty()) {                     // draw text
                p->save();
                int t = s.indexOf(QLatin1Char('\t'));
                int text_flags = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                if (!styleHint(SH_UnderlineShortcut, menuitem, widget))
                    text_flags |= Qt::TextHideMnemonic;
                text_flags |= Qt::AlignLeft;
                if (t >= 0) {
                    QRect vShortcutRect = visualRect(opt->direction, menuitem->rect,
                        QRect(textRect.topRight(), QPoint(menuitem->rect.right(), textRect.bottom())));
                    if (dis && !act && styleHint(SH_EtchDisabledText, option, widget)) {
                        p->setPen(menuitem->palette.light().color());
                        p->drawText(vShortcutRect.adjusted(1,1,1,1), text_flags, s.mid(t + 1));
                        p->setPen(discol);
                    }
                    p->drawText(vShortcutRect, text_flags, s.mid(t + 1));
                    s = s.left(t);
                }
                QFont font = menuitem->font;
                if (menuitem->menuItemType == QStyleOptionMenuItem::DefaultItem)
                    font.setBold(true);
                p->setFont(font);
                if (dis && !act && styleHint(SH_EtchDisabledText, option, widget)) {
                    p->setPen(menuitem->palette.light().color());
                    p->drawText(vTextRect.adjusted(1,1,1,1), text_flags, s.left(t));
                    p->setPen(discol);
                }
                p->drawText(vTextRect, text_flags, s.left(t));
                p->restore();
            }

            // Arrow
            if (menuItem->menuItemType == QStyleOptionMenuItem::SubMenu) {// draw sub menu arrow
                int dim = (menuItem->rect.height() - 4) / 2;
                PrimitiveElement arrow;
                arrow = (opt->direction == Qt::RightToLeft) ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight;
                int xpos = menuItem->rect.left() + menuItem->rect.width() - 6 - 2 - dim;
                QRect  vSubMenuRect = visualRect(option->direction, menuItem->rect,
                                                 QRect(xpos, menuItem->rect.top() + menuItem->rect.height() / 2 - dim / 2, dim, dim));
                QStyleOptionMenuItem newMI = *menuItem;
                newMI.rect = vSubMenuRect;
                newMI.state = option->state & State_Enabled;
                if (selected)
                    newMI.palette.setColor(QPalette::ButtonText,
                                           newMI.palette.highlightedText().color());
                else
                    newMI.palette.setColor(QPalette::ButtonText, textBrush.color());
                proxy()->drawPrimitive(arrow, &newMI, painter, widget);
            }

            painter->restore();
        }
        break;
#endif // QT_NO_MENU
#ifndef QT_NO_MENUBAR
    case CE_MenuBarItem:
        // Draws a menu bar item; File, Edit, Help etc..
        if ((option->state & State_Selected)) {
            QPixmap cache;
            QString pixmapName = QStyleHelper::uniqueName(QLatin1String("menubaritem"), option, option->rect.size());
            if (!QPixmapCache::find(pixmapName, cache)) {
                cache = QPixmap(option->rect.size());
                cache.fill(Qt::white);
                QRect pixmapRect(0, 0, option->rect.width(), option->rect.height());
                QPainter cachePainter(&cache);

                QRect rect = pixmapRect;

                // gradient fill
                if ((option->state & QStyle::State_Sunken) || (option->state & QStyle::State_On)) {
                    qt_plastique_draw_gradient(&cachePainter, rect.adjusted(1, 1, -1, -1),
                                               option->palette.button().color().darker(114),
                                               option->palette.button().color().darker(106));
                } else {
                    qt_plastique_draw_gradient(&cachePainter, rect.adjusted(1, 1, -1, -1),
                                               option->palette.background().color().lighter(105),
                                               option->palette.background().color().darker(102));
                }

                // outer border and corners
                cachePainter.setPen(borderColor);
                cachePainter.drawRect(rect.adjusted(0, 0, -1, -1));
                cachePainter.setPen(alphaCornerColor);

                const QPoint points[4] = {
                    rect.topLeft(),
                    rect.topRight(),
                    rect.bottomLeft(),
                    rect.bottomRight() };
                cachePainter.drawPoints(points, 4);

                // inner border
                if ((option->state & QStyle::State_Sunken) || (option->state & QStyle::State_On))
                    cachePainter.setPen(option->palette.button().color().darker(118));
                else
                    cachePainter.setPen(gradientStartColor);

                QLine lines[2];
                lines[0] = QLine(rect.left() + 1, rect.top() + 1, rect.right() - 1, rect.top() + 1);
                lines[1] = QLine(rect.left() + 1, rect.top() + 2, rect.left() + 1, rect.bottom() - 2);
                cachePainter.drawLines(lines, 2);

                if ((option->state & QStyle::State_Sunken) || (option->state & QStyle::State_On))
                    cachePainter.setPen(option->palette.button().color().darker(114));
                else
                    cachePainter.setPen(gradientStopColor.darker(102));
                lines[0] = QLine(rect.left() + 1, rect.bottom() - 1, rect.right() - 1, rect.bottom() - 1);
                lines[1] = QLine(rect.right() - 1, rect.top() + 1, rect.right() - 1, rect.bottom() - 2);
                cachePainter.drawLines(lines, 2);
                cachePainter.end();
                QPixmapCache::insert(pixmapName, cache);
            }
            painter->drawPixmap(option->rect.topLeft(), cache);
        } else {
            painter->fillRect(option->rect, option->palette.background());
        }

        if (const QStyleOptionMenuItem *mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            QStyleOptionMenuItem newMI = *mbi;
            if (!(option->palette.resolve() & (1 << QPalette::ButtonText))) //KDE uses windowText rather than buttonText for menus
                newMI.palette.setColor(QPalette::ButtonText, newMI.palette.windowText().color());
            QCommonStyle::drawControl(element, &newMI, painter, widget);
        }
        break;

#ifndef QT_NO_MAINWINDOW
    case CE_MenuBarEmptyArea:
        if (widget && qobject_cast<const QMainWindow *>(widget->parentWidget())) {
            painter->fillRect(option->rect, option->palette.window());
            QPen oldPen = painter->pen();
            painter->setPen(QPen(option->palette.dark().color()));
            painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
            painter->setPen(oldPen);
        }
        break;
#endif // QT_NO_MAINWINDOW

#endif // QT_NO_MENUBAR

#ifndef QT_NO_TOOLBOX
    case CE_ToolBoxTabShape:
        if (const QStyleOptionToolBox *toolBox = qstyleoption_cast<const QStyleOptionToolBox *>(option)) {
            painter->save();

            int width = toolBox->rect.width();
            int diag = toolBox->rect.height() - 2;

            // The essential points
            QPoint rightMost;
            QPoint rightEdge;
            QPoint leftEdge;
            QPoint leftMost;
            QPoint leftOne;
            QPoint rightOne;
            QPoint upOne(0, -1);
            QPoint downOne(0, 1);

            if (toolBox->direction != Qt::RightToLeft) {
                rightMost = QPoint(toolBox->rect.right(), toolBox->rect.bottom() - 2);
                rightEdge = QPoint(toolBox->rect.right() - width / 10, toolBox->rect.bottom() - 2);
                leftEdge = QPoint(toolBox->rect.right() - width / 10 - diag, toolBox->rect.top());
                leftMost = QPoint(toolBox->rect.left(), toolBox->rect.top());
                leftOne = QPoint(-1, 0);
                rightOne = QPoint(1, 0);
            } else {
                rightMost = QPoint(toolBox->rect.left(), toolBox->rect.bottom() - 2);
                rightEdge = QPoint(toolBox->rect.left() + width / 10, toolBox->rect.bottom() - 2);
                leftEdge = QPoint(toolBox->rect.left() + width / 10 + diag, toolBox->rect.top());
                leftMost = QPoint(toolBox->rect.right(), toolBox->rect.top());
                leftOne = QPoint(1, 0);
                rightOne = QPoint(-1, 0);
            }

            QLine lines[3];

            // Draw the outline
            painter->setPen(borderColor);
            lines[0] = QLine(rightMost, rightEdge);
            lines[1] = QLine(rightEdge + leftOne, leftEdge);
            lines[2] = QLine(leftEdge + leftOne, leftMost);
            painter->drawLines(lines, 3);
            painter->setPen(toolBox->palette.base().color());
            lines[0] = QLine(rightMost + downOne, rightEdge + downOne);
            lines[1] = QLine(rightEdge + leftOne + downOne, leftEdge + downOne);
            lines[2] = QLine(leftEdge + leftOne + downOne, leftMost + downOne);
            painter->drawLines(lines, 3);

            painter->restore();
        }
        break;
#endif // QT_NO_TOOLBOX
#ifndef QT_NO_SPLITTER
    case CE_Splitter:
        if ((option->state & State_Enabled) && (option->state & State_MouseOver))
            painter->fillRect(option->rect, QColor(255, 255, 255, 128));
        if (option->state & State_Horizontal) {
            int height = option->rect.height() / 3;
            QRect rect(option->rect.left() + (option->rect.width() / 2 - 1),
                       option->rect.center().y() - height / 2, 3, height);
            qt_plastique_draw_handle(painter, option, rect, Qt::Horizontal, widget);
        } else {
            int width = option->rect.width() / 3;
            QRect rect(option->rect.center().x() - width / 2,
                       option->rect.top() + (option->rect.height() / 2) - 1, width, 3);
            qt_plastique_draw_handle(painter, option, rect, Qt::Vertical, widget);
        }
        break;
#endif // QT_NO_SPLITTER
#ifndef QT_NO_DOCKWIDGET
    case CE_DockWidgetTitle:
        if (const QStyleOptionDockWidget *dockWidget = qstyleoption_cast<const QStyleOptionDockWidget *>(option)) {
            painter->save();

            const QStyleOptionDockWidgetV2 *v2
                = qstyleoption_cast<const QStyleOptionDockWidgetV2*>(dockWidget);
            bool verticalTitleBar = v2 == 0 ? false : v2->verticalTitleBar;

            // Find text width and title rect
            int textWidth = option->fontMetrics.width(dockWidget->title);
            int margin = 4;
            QRect titleRect = subElementRect(SE_DockWidgetTitleBarText, option, widget);
            QRect rect = dockWidget->rect;

            if (verticalTitleBar) {
                QRect r = rect;
                QSize s = r.size();
                s.transpose();
                r.setSize(s);

                titleRect = QRect(r.left() + rect.bottom()
                                    - titleRect.bottom(),
                                r.top() + titleRect.left() - rect.left(),
                                titleRect.height(), titleRect.width());

                painter->translate(r.left(), r.top() + r.width());
                painter->rotate(-90);
                painter->translate(-r.left(), -r.top());

                rect = r;
            }

            // Chop and insert ellide into title if text is too wide
            QString title = elliditide(dockWidget->title, dockWidget->fontMetrics, titleRect, &textWidth);

            // Draw the toolbar handle pattern to the left and right of the text
            QImage handle(qt_toolbarhandle);
            alphaCornerColor.setAlpha(170);
            handle.setColor(1, alphaCornerColor.rgba());
            handle.setColor(2, mergedColors(alphaCornerColor, option->palette.light().color()).rgba());
            handle.setColor(3, option->palette.light().color().rgba());

            if (title.isEmpty()) {
                // Joint handle if there's no title
                QRect r;
#ifdef QT3_SUPPORT
                // Q3DockWindow doesn't need space for buttons
                if (widget && widget->inherits("Q3DockWindowTitleBar")) {
                    r = rect;
                } else
#endif
                    r.setRect(titleRect.left(), titleRect.top(), titleRect.width(), titleRect.bottom());
                    int nchunks = (r.width() / handle.width()) - 1;
                    int indent = (r.width() - (nchunks * handle.width())) / 2;
                    for (int i = 0; i < nchunks; ++i) {
                        painter->drawImage(QPoint(r.left() + indent + i * handle.width(),
                                                r.center().y() - handle.height() / 2),
                                        handle);
                    }
            } else {
                // Handle pattern to the left of the title
                QRect leftSide(titleRect.left(), titleRect.top(),
                               titleRect.width() / 2 - textWidth / 2 - margin, titleRect.bottom());
                int nchunks = leftSide.width() / handle.width();
                int indent = (leftSide.width() - (nchunks * handle.width())) / 2;
                for (int i = 0; i < nchunks; ++i) {
                    painter->drawImage(QPoint(leftSide.left() + indent
                                                + i * handle.width(),
                                              leftSide.center().y()
                                                - handle.height() / 2),
                                       handle);
                }

                // Handle pattern to the right of the title
                QRect rightSide = titleRect.adjusted(titleRect.width() / 2 + textWidth / 2 + margin, 0, 0, 0);
                nchunks = rightSide.width() / handle.width();
                indent = (rightSide.width() - (nchunks * handle.width())) / 2;
                for (int j = 0; j < nchunks; ++j) {
                    painter->drawImage(QPoint(rightSide.left() + indent + j * handle.width(),
                                              rightSide.center().y() - handle.height() / 2),
                                       handle);
                }
            }

            // Draw the text centered
            QFont font = painter->font();
            font.setPointSize(QFontInfo(font).pointSize() - 1);
            painter->setFont(font);
            painter->setPen(dockWidget->palette.windowText().color());
            painter->drawText(titleRect,
                              int(Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextShowMnemonic),
                              title);

            painter->restore();
        }

        break;
#endif // QT_NO_DOCKWIDGET
#ifndef QT_NO_TOOLBAR
    case CE_ToolBar:
        if (const QStyleOptionToolBar *toolBar = qstyleoption_cast<const QStyleOptionToolBar *>(option)) {
            // Draws the light line above and the dark line below menu bars and
            // tool bars.
            QPen oldPen = painter->pen();
            if (toolBar->toolBarArea == Qt::TopToolBarArea) {
                if (toolBar->positionOfLine == QStyleOptionToolBar::End
                    || toolBar->positionOfLine == QStyleOptionToolBar::OnlyOne) {
                    // The end and onlyone top toolbar lines draw a double
                    // line at the bottom to blend with the central
                    // widget.
                    painter->setPen(option->palette.background().color().lighter(104));
                    painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
                    painter->setPen(alphaCornerColor);
                    painter->drawLine(option->rect.left(), option->rect.bottom() - 1,
                                      option->rect.right(), option->rect.bottom() - 1);
                } else {
                    // All others draw a single dark line at the bottom.
                    painter->setPen(alphaCornerColor);
                    painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
                }
                // All top toolbar lines draw a light line at the top.
                painter->setPen(option->palette.background().color().lighter(104));
                painter->drawLine(option->rect.topLeft(), option->rect.topRight());
            } else if (toolBar->toolBarArea == Qt::BottomToolBarArea) {
                if (toolBar->positionOfLine == QStyleOptionToolBar::End
                    || toolBar->positionOfLine == QStyleOptionToolBar::Middle) {
                    // The end and middle bottom tool bar lines draw a dark
                    // line at the bottom.
                    painter->setPen(alphaCornerColor);
                    painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
                }
                if (toolBar->positionOfLine == QStyleOptionToolBar::Beginning
                    || toolBar->positionOfLine == QStyleOptionToolBar::OnlyOne) {
                    // The beginning and only one tool bar lines draw a
                    // double line at the bottom to blend with the
                    // status bar.
                    // ### The styleoption could contain whether the
                    // main window has a menu bar and a status bar, and
                    // possibly dock widgets.
                    painter->setPen(alphaCornerColor);
                    painter->drawLine(option->rect.left(), option->rect.bottom() - 1,
                                      option->rect.right(), option->rect.bottom() - 1);
                    painter->setPen(option->palette.background().color().lighter(104));
                    painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
                }
                if (toolBar->positionOfLine == QStyleOptionToolBar::End) {
                    painter->setPen(alphaCornerColor);
                    painter->drawLine(option->rect.topLeft(), option->rect.topRight());
                    painter->setPen(option->palette.background().color().lighter(104));
                    painter->drawLine(option->rect.left(), option->rect.top() + 1,
                                      option->rect.right(), option->rect.top() + 1);

                } else {
                    // All other bottom toolbars draw a light line at the top.
                    painter->setPen(option->palette.background().color().lighter(104));
                    painter->drawLine(option->rect.topLeft(), option->rect.topRight());
                }
            }
            if (toolBar->toolBarArea == Qt::LeftToolBarArea) {
                if (toolBar->positionOfLine == QStyleOptionToolBar::Middle
                    || toolBar->positionOfLine == QStyleOptionToolBar::End) {
                    // The middle and left end toolbar lines draw a light
                    // line to the left.
                    painter->setPen(option->palette.background().color().lighter(104));
                    painter->drawLine(option->rect.topLeft(), option->rect.bottomLeft());
                }
                if (toolBar->positionOfLine == QStyleOptionToolBar::End) {
                    // All other left toolbar lines draw a dark line to the right
                    painter->setPen(alphaCornerColor);
                    painter->drawLine(option->rect.right() - 1, option->rect.top(),
                                      option->rect.right() - 1, option->rect.bottom());
                    painter->setPen(option->palette.background().color().lighter(104));
                    painter->drawLine(option->rect.topRight(), option->rect.bottomRight());
                } else {
                    // All other left toolbar lines draw a dark line to the right
                    painter->setPen(alphaCornerColor);
                    painter->drawLine(option->rect.topRight(), option->rect.bottomRight());
                }
            } else if (toolBar->toolBarArea == Qt::RightToolBarArea) {
                if (toolBar->positionOfLine == QStyleOptionToolBar::Middle
                    || toolBar->positionOfLine == QStyleOptionToolBar::End) {
                    // Right middle and end toolbar lines draw the dark right line
                    painter->setPen(alphaCornerColor);
                    painter->drawLine(option->rect.topRight(), option->rect.bottomRight());
                }
                if (toolBar->positionOfLine == QStyleOptionToolBar::End
                    || toolBar->positionOfLine == QStyleOptionToolBar::OnlyOne) {
                    // The right end and single toolbar draws the dark
                    // line on its left edge
                    painter->setPen(alphaCornerColor);
                    painter->drawLine(option->rect.topLeft(), option->rect.bottomLeft());
                    // And a light line next to it
                    painter->setPen(option->palette.background().color().lighter(104));
                    painter->drawLine(option->rect.left() + 1, option->rect.top(),
                                      option->rect.left() + 1, option->rect.bottom());
                } else {
                    // Other right toolbars draw a light line on its left edge
                    painter->setPen(option->palette.background().color().lighter(104));
                    painter->drawLine(option->rect.topLeft(), option->rect.bottomLeft());
                }
            }
            painter->setPen(oldPen);
        }
        break;
#endif // QT_NO_TOOLBAR
#ifndef QT_NO_SCROLLBAR
    case CE_ScrollBarAddLine:
        if (const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {

            bool horizontal = scrollBar->orientation == Qt::Horizontal;
            bool reverse = scrollBar->direction == Qt::RightToLeft;
            bool sunken = scrollBar->state & State_Sunken;

            QString addLinePixmapName = QStyleHelper::uniqueName(QLatin1String("scrollbar_addline"), option, option->rect.size());
            QPixmap cache;
            if (!QPixmapCache::find(addLinePixmapName, cache)) {
                cache = QPixmap(option->rect.size());
                cache.fill(Qt::white);
                QRect pixmapRect(0, 0, cache.width(), cache.height());
                QPainter addLinePainter(&cache);
                addLinePainter.fillRect(pixmapRect, option->palette.background());

                if (option->state & State_Enabled) {
                    // Gradient
                    QLinearGradient gradient(pixmapRect.center().x(), pixmapRect.top() + 2,
                                             pixmapRect.center().x(), pixmapRect.bottom() - 2);
                    if ((scrollBar->activeSubControls & SC_ScrollBarAddLine) && sunken) {
                        gradient.setColorAt(0, gradientStopColor);
                        gradient.setColorAt(1, gradientStopColor);
                    } else {
                        gradient.setColorAt(0, gradientStartColor.lighter(105));
                        gradient.setColorAt(1, gradientStopColor);
                    }
                    addLinePainter.fillRect(pixmapRect.left() + 2, pixmapRect.top() + 2,
                                            pixmapRect.right() - 3, pixmapRect.bottom() - 3,
                                            gradient);
                }

                // Details
                QImage addButton;
                if (horizontal) {
                    addButton = QImage(reverse ? qt_scrollbar_button_left : qt_scrollbar_button_right);
                } else {
                    addButton = QImage(qt_scrollbar_button_down);
                }
                addButton.setColor(1, alphaCornerColor.rgba());
                addButton.setColor(2, borderColor.rgba());
                if ((scrollBar->activeSubControls & SC_ScrollBarAddLine) && sunken) {
                    addButton.setColor(3, gradientStopColor.rgba());
                    addButton.setColor(4, gradientStopColor.rgba());
                } else {
                    addButton.setColor(3, gradientStartColor.lighter(105).rgba());
                    addButton.setColor(4, gradientStopColor.rgba());
                }
                addButton.setColor(5, scrollBar->palette.text().color().rgba());
                addLinePainter.drawImage(pixmapRect, addButton);

                // Arrow
                if (horizontal) {
                    QImage arrow(reverse ? qt_scrollbar_button_arrow_left : qt_scrollbar_button_arrow_right);
                    arrow.setColor(1, scrollBar->palette.foreground().color().rgba());

                    if ((scrollBar->activeSubControls & SC_ScrollBarAddLine) && sunken)
                        addLinePainter.translate(1, 1);
                    addLinePainter.drawImage(QPoint(pixmapRect.center().x() - 2, pixmapRect.center().y() - 3), arrow);
                } else {
                    QImage arrow(qt_scrollbar_button_arrow_down);
                    arrow.setColor(1, scrollBar->palette.foreground().color().rgba());

                    if ((scrollBar->activeSubControls & SC_ScrollBarAddLine) && sunken)
                        addLinePainter.translate(1, 1);
                    addLinePainter.drawImage(QPoint(pixmapRect.center().x() - 3, pixmapRect.center().y() - 2), arrow);
                }
                addLinePainter.end();
                QPixmapCache::insert(addLinePixmapName, cache);
            }
            painter->drawPixmap(option->rect.topLeft(), cache);
        }
        break;
    case CE_ScrollBarSubPage:
    case CE_ScrollBarAddPage:
        if (const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            bool sunken = scrollBar->state & State_Sunken;
            bool horizontal = scrollBar->orientation == Qt::Horizontal;

            QString groovePixmapName = QStyleHelper::uniqueName(QLatin1String("scrollbar_groove"), option, option->rect.size());
            if (sunken)
                groovePixmapName += QLatin1String("-sunken");
            if (element == CE_ScrollBarAddPage)
                groovePixmapName += QLatin1String("-addpage");

            QPixmap cache;
            if (!QPixmapCache::find(groovePixmapName, cache)) {
                cache = QPixmap(option->rect.size());
                cache.fill(option->palette.background().color());
                QPainter groovePainter(&cache);
                QRect pixmapRect = QRect(0, 0, option->rect.width(), option->rect.height());
                QColor color = scrollBar->palette.base().color().darker(sunken ? 125 : 100);
                groovePainter.setBrushOrigin((element == CE_ScrollBarAddPage) ? pixmapRect.width() : 0,
                                             (element == CE_ScrollBarAddPage) ? pixmapRect.height() : 0);
                groovePainter.fillRect(pixmapRect, QBrush(color, Qt::Dense4Pattern));

                QColor edgeColor = scrollBar->palette.base().color().darker(125);
                if (horizontal) {
                    groovePainter.setBrushOrigin((element == CE_ScrollBarAddPage) ? pixmapRect.width() : 1, 0);
                    groovePainter.fillRect(QRect(pixmapRect.topLeft(), QSize(pixmapRect.width(), 1)),
                                           QBrush(edgeColor, Qt::Dense4Pattern));
                    groovePainter.fillRect(QRect(pixmapRect.bottomLeft(), QSize(pixmapRect.width(), 1)),
                                           QBrush(edgeColor, Qt::Dense4Pattern));
                } else {
                    groovePainter.setBrushOrigin(0, (element == CE_ScrollBarAddPage) ? pixmapRect.height() : 1);
                    groovePainter.fillRect(QRect(pixmapRect.topLeft(), QSize(1, pixmapRect.height())),
                                           QBrush(edgeColor, Qt::Dense4Pattern));
                    groovePainter.fillRect(QRect(pixmapRect.topRight(), QSize(1, pixmapRect.height())),
                                           QBrush(edgeColor, Qt::Dense4Pattern));
                }

                groovePainter.end();
                QPixmapCache::insert(groovePixmapName, cache);
            }
            painter->drawPixmap(option->rect.topLeft(), cache);
        }
        break;
    case CE_ScrollBarSubLine:
        if (const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            QRect scrollBarSubLine = scrollBar->rect;

            bool horizontal = scrollBar->orientation == Qt::Horizontal;
            bool isEnabled = scrollBar->state & State_Enabled;
            bool reverse = scrollBar->direction == Qt::RightToLeft;
            bool sunken = scrollBar->state & State_Sunken;

            // The SubLine (up/left) buttons
            QRect button1;
            QRect button2;
            int scrollBarExtent = proxy()->pixelMetric(PM_ScrollBarExtent, option, widget);
            if (horizontal) {
                button1.setRect(scrollBarSubLine.left(), scrollBarSubLine.top(), scrollBarExtent, scrollBarSubLine.height());
                button2.setRect(scrollBarSubLine.right() - (scrollBarExtent - 1), scrollBarSubLine.top(), scrollBarExtent, scrollBarSubLine.height());
            } else {
                button1.setRect(scrollBarSubLine.left(), scrollBarSubLine.top(), scrollBarSubLine.width(), scrollBarExtent);
                button2.setRect(scrollBarSubLine.left(), scrollBarSubLine.bottom() - (scrollBarExtent - 1), scrollBarSubLine.width(), scrollBarExtent);
            }

            QString subLinePixmapName = QStyleHelper::uniqueName(QLatin1String("scrollbar_subline"), option, button1.size());
            QPixmap cache;
            if (!QPixmapCache::find(subLinePixmapName, cache)) {
                cache = QPixmap(button1.size());
                cache.fill(Qt::white);
                QRect pixmapRect(0, 0, cache.width(), cache.height());
                QPainter subLinePainter(&cache);
                subLinePainter.fillRect(pixmapRect, option->palette.background());

                if (isEnabled) {
                    // Gradients
                    if ((scrollBar->activeSubControls & SC_ScrollBarSubLine) && sunken) {
                        qt_plastique_draw_gradient(&subLinePainter,
                                                   QRect(pixmapRect.left() + 2, pixmapRect.top() + 2,
                                                         pixmapRect.right() - 3, pixmapRect.bottom() - 3),
                                                   gradientStopColor,
                                                   gradientStopColor);
                    } else {
                        qt_plastique_draw_gradient(&subLinePainter,
                                                   QRect(pixmapRect.left() + 2, pixmapRect.top() + 2,
                                                         pixmapRect.right() - 3, pixmapRect.bottom() - 3),
                                                   gradientStartColor.lighter(105),
                                                   gradientStopColor);
                    }
                }

                // Details
                QImage subButton;
                if (horizontal) {
                    subButton = QImage(reverse ? qt_scrollbar_button_right : qt_scrollbar_button_left);
                } else {
                    subButton = QImage(qt_scrollbar_button_up);
                }
                subButton.setColor(1, alphaCornerColor.rgba());
                subButton.setColor(2, borderColor.rgba());
                if ((scrollBar->activeSubControls & SC_ScrollBarSubLine) && sunken) {
                    subButton.setColor(3, gradientStopColor.rgba());
                    subButton.setColor(4, gradientStopColor.rgba());
                } else {
                    subButton.setColor(3, gradientStartColor.lighter(105).rgba());
                    subButton.setColor(4, gradientStopColor.rgba());
                }
                subButton.setColor(5, scrollBar->palette.text().color().rgba());
                subLinePainter.drawImage(pixmapRect, subButton);

                // Arrows
                if (horizontal) {
                    QImage arrow(reverse ? qt_scrollbar_button_arrow_right : qt_scrollbar_button_arrow_left);
                    arrow.setColor(1, scrollBar->palette.foreground().color().rgba());

                    if ((scrollBar->activeSubControls & SC_ScrollBarSubLine) && sunken)
                        subLinePainter.translate(1, 1);
                    subLinePainter.drawImage(QPoint(pixmapRect.center().x() - 2, pixmapRect.center().y() - 3), arrow);
                } else {
                    QImage arrow(qt_scrollbar_button_arrow_up);
                    arrow.setColor(1, scrollBar->palette.foreground().color().rgba());

                    if ((scrollBar->activeSubControls & SC_ScrollBarSubLine) && sunken)
                        subLinePainter.translate(1, 1);
                    subLinePainter.drawImage(QPoint(pixmapRect.center().x() - 3, pixmapRect.center().y() - 2), arrow);
                }
                subLinePainter.end();
                QPixmapCache::insert(subLinePixmapName, cache);
            }
            painter->drawPixmap(button1.topLeft(), cache);
            painter->drawPixmap(button2.topLeft(), cache);
        }
        break;
    case CE_ScrollBarSlider:
        if (const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            bool horizontal = scrollBar->orientation == Qt::Horizontal;
            bool isEnabled = scrollBar->state & State_Enabled;

            // The slider
            if (option->rect.isValid()) {
                QString sliderPixmapName = QStyleHelper::uniqueName(QLatin1String("scrollbar_slider"), option, option->rect.size());
                if (horizontal)
                    sliderPixmapName += QLatin1String("-horizontal");

                QPixmap cache;
                if (!QPixmapCache::find(sliderPixmapName, cache)) {
                    cache = QPixmap(option->rect.size());
                    cache.fill(Qt::white);
                    QRect pixmapRect(0, 0, cache.width(), cache.height());
                    QPainter sliderPainter(&cache);
                    bool sunken = (scrollBar->state & State_Sunken);

                    if (isEnabled) {
                        QLinearGradient gradient(pixmapRect.left(), pixmapRect.center().y(),
                                                 pixmapRect.right(), pixmapRect.center().y());
                        if (horizontal)
                            gradient = QLinearGradient(pixmapRect.center().x(), pixmapRect.top(),
                                                 pixmapRect.center().x(), pixmapRect.bottom());

                        if (sunken) {
                            gradient.setColorAt(0, gradientStartColor.lighter(110));
                            gradient.setColorAt(1, gradientStopColor.lighter(105));
                        } else {
                            gradient.setColorAt(0, gradientStartColor.lighter(105));
                            gradient.setColorAt(1, gradientStopColor);
                        }
                        sliderPainter.fillRect(pixmapRect.adjusted(2, 2, -2, -2), gradient);
                    } else {
                        sliderPainter.fillRect(pixmapRect.adjusted(2, 2, -2, -2), option->palette.background());
                    }

                    sliderPainter.setPen(borderColor);
                    sliderPainter.drawRect(pixmapRect.adjusted(0, 0, -1, -1));
                    sliderPainter.setPen(alphaCornerColor);
                    QPoint points[4] = {
                        QPoint(pixmapRect.left(), pixmapRect.top()),
                        QPoint(pixmapRect.left(), pixmapRect.bottom()),
                        QPoint(pixmapRect.right(), pixmapRect.top()),
                        QPoint(pixmapRect.right(), pixmapRect.bottom()) };
                    sliderPainter.drawPoints(points, 4);

                    QLine lines[2];
                    sliderPainter.setPen(sunken ? gradientStartColor.lighter(110) : gradientStartColor.lighter(105));
                    lines[0] = QLine(pixmapRect.left() + 1, pixmapRect.top() + 1,
                                     pixmapRect.right() - 1, pixmapRect.top() + 1);
                    lines[1] = QLine(pixmapRect.left() + 1, pixmapRect.top() + 2,
                                     pixmapRect.left() + 1, pixmapRect.bottom() - 2);
                    sliderPainter.drawLines(lines, 2);

                    sliderPainter.setPen(sunken ? gradientStopColor.lighter(105) : gradientStopColor);
                    lines[0] = QLine(pixmapRect.left() + 1, pixmapRect.bottom() - 1,
                                     pixmapRect.right() - 1, pixmapRect.bottom() - 1);
                    lines[1] = QLine(pixmapRect.right() - 1, pixmapRect.top() + 2,
                                     pixmapRect.right() - 1, pixmapRect.bottom() - 1);
                    sliderPainter.drawLines(lines, 2);

                    int sliderMinLength = proxy()->pixelMetric(PM_ScrollBarSliderMin, scrollBar, widget);
                    if ((horizontal && scrollBar->rect.width() > sliderMinLength)
                        || (!horizontal && scrollBar->rect.height() > sliderMinLength)) {
                        QImage pattern(horizontal ? qt_scrollbar_slider_pattern_horizontal
                                       : qt_scrollbar_slider_pattern_vertical);
                        pattern.setColor(1, alphaCornerColor.rgba());
                        pattern.setColor(2, (sunken ? gradientStartColor.lighter(110) : gradientStartColor.lighter(105)).rgba());

                        if (horizontal) {
                            sliderPainter.drawImage(pixmapRect.center().x() - pattern.width() / 2 + 1,
                                                    pixmapRect.center().y() - 4,
                                                    pattern);
                        } else {
                            sliderPainter.drawImage(pixmapRect.center().x() - 4,
                                                    pixmapRect.center().y() - pattern.height() / 2 + 1,
                                                    pattern);
                        }
                    }
                    sliderPainter.end();
                    // insert the slider into the cache
                    QPixmapCache::insert(sliderPixmapName, cache);
                }
                painter->drawPixmap(option->rect.topLeft(), cache);
            }
        }
        break;
#endif
#ifndef QT_NO_COMBOBOX
    case CE_ComboBoxLabel:
        if (const QStyleOptionComboBox *comboBox = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            painter->save();
            if (!comboBox->editable) {
                // Plastique's non-editable combo box is drawn as a button, so
                // we need the label to be drawn using ButtonText where it
                // would usually use Text.
                painter->setPen(QPen(comboBox->palette.buttonText(), 0));
                QWindowsStyle::drawControl(element, option, painter, widget);
            } else if (!comboBox->currentIcon.isNull()) {
                {
                    QRect editRect = proxy()->subControlRect(CC_ComboBox, comboBox, SC_ComboBoxEditField, widget);
                    if (comboBox->direction == Qt::RightToLeft)
                        editRect.adjust(0, 2, -2, -2);
                    else
                        editRect.adjust(2, 2, 0, -2);
                    painter->save();
                    painter->setClipRect(editRect);
                    if (!comboBox->currentIcon.isNull()) {
                        QIcon::Mode mode = comboBox->state & State_Enabled ? QIcon::Normal
                                           : QIcon::Disabled;
                        QPixmap pixmap = comboBox->currentIcon.pixmap(comboBox->iconSize, mode);
                        QRect iconRect(editRect);
                        iconRect.setWidth(comboBox->iconSize.width() + 5);
                        iconRect = alignedRect(comboBox->direction,
                                               Qt::AlignLeft | Qt::AlignVCenter,
                                               iconRect.size(), editRect);
                        painter->fillRect(iconRect, option->palette.brush(QPalette::Base));
                        proxy()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, pixmap);
                    }
                    painter->restore();
                }
            } else {
                QWindowsStyle::drawControl(element, option, painter, widget);
            }

            painter->restore();
        }
        break;
#endif
    default:
        QWindowsStyle::drawControl(element, option, painter, widget);
        break;
    }
}

/*!
  \reimp
*/
void QPlastiqueStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                         QPainter *painter, const QWidget *widget) const
{
    QColor borderColor = option->palette.background().color().darker(178);
    QColor alphaCornerColor;
   if (widget) {
        // ### backgroundrole/foregroundrole should be part of the style option
        alphaCornerColor = mergedColors(option->palette.color(widget->backgroundRole()), borderColor);
    } else {
        alphaCornerColor = mergedColors(option->palette.background().color(), borderColor);
    }
    QColor gradientStartColor = option->palette.button().color().lighter(104);
    QColor gradientStopColor = option->palette.button().color().darker(105);

    switch (control) {
#ifndef QT_NO_SLIDER
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            QRect grooveRegion = proxy()->subControlRect(CC_Slider, option, SC_SliderGroove, widget);
            QRect handle = proxy()->subControlRect(CC_Slider, option, SC_SliderHandle, widget);
            bool horizontal = slider->orientation == Qt::Horizontal;
            bool ticksAbove = slider->tickPosition & QSlider::TicksAbove;
            bool ticksBelow = slider->tickPosition & QSlider::TicksBelow;

            QRect groove;
            //The clickable region is 5 px wider than the visible groove for improved usability
            if (grooveRegion.isValid())
                groove = horizontal ? grooveRegion.adjusted(0, 5, 0, -5) : grooveRegion.adjusted(5, 0, -5, 0);


            QPixmap cache;

            if ((option->subControls & SC_SliderGroove) && groove.isValid()) {
                BEGIN_STYLE_PIXMAPCACHE(QString::fromLatin1("slider_groove-%0-%1").arg(ticksAbove).arg(ticksBelow))
                p->fillRect(groove, option->palette.background());

                // draw groove
                if (horizontal) {
                    p->setPen(borderColor);
                    const QLine lines[4] = {
                        QLine(groove.left() + 1, groove.top(),
                              groove.right() - 1, groove.top()),
                        QLine(groove.left() + 1, groove.bottom(),
                              groove.right() - 1, groove.bottom()),
                        QLine(groove.left(), groove.top() + 1,
                              groove.left(), groove.bottom() - 1),
                        QLine(groove.right(), groove.top() + 1,
                              groove.right(), groove.bottom() - 1) };
                    p->drawLines(lines, 4);

                    p->setPen(alphaCornerColor);
                    const QPoint points[4] = {
                        QPoint(groove.left(), groove.top()),
                        QPoint(groove.left(), groove.bottom()),
                        QPoint(groove.right(), groove.top()),
                        QPoint(groove.right(), groove.bottom()) };
                    p->drawPoints(points, 4);
                } else {
                    p->setPen(borderColor);
                    const QLine lines[4] = {
                        QLine(groove.left() + 1, groove.top(),
                              groove.right() - 1, groove.top()),
                        QLine(groove.left() + 1, groove.bottom(),
                              groove.right() - 1, groove.bottom()),
                        QLine(groove.left(), groove.top() + 1,
                              groove.left(), groove.bottom() - 1),
                        QLine(groove.right(), groove.top() + 1,
                              groove.right(), groove.bottom() - 1) };
                    p->drawLines(lines, 4);

                    p->setPen(alphaCornerColor);
                    const QPoint points[4] = {
                        QPoint(groove.left(), groove.top()),
                        QPoint(groove.right(), groove.top()),
                        QPoint(groove.left(), groove.bottom()),
                        QPoint(groove.right(), groove.bottom()) };
                    p->drawPoints(points, 4);
                }
                END_STYLE_PIXMAPCACHE
            }

            if ((option->subControls & SC_SliderHandle) && handle.isValid()) {
                QString handlePixmapName = QStyleHelper::uniqueName(QLatin1String("slider_handle"), option, handle.size());
                if (ticksAbove && !ticksBelow)
                    handlePixmapName += QLatin1String("-flipped");
                if ((option->activeSubControls & SC_SliderHandle) && (option->state & State_Sunken))
                    handlePixmapName += QLatin1String("-sunken");

                if (!QPixmapCache::find(handlePixmapName, cache)) {
                    cache = QPixmap(handle.size());
                    cache.fill(Qt::white);
                    QRect pixmapRect(0, 0, handle.width(), handle.height());
                    QPainter handlePainter(&cache);
                    handlePainter.fillRect(pixmapRect, option->palette.background());

                    // draw handle
                    if (horizontal) {
                        QPainterPath path;
                        if (ticksAbove && !ticksBelow) {
                            path.moveTo(QPoint(pixmapRect.right(), pixmapRect.bottom()));
                            path.lineTo(QPoint(pixmapRect.right(), pixmapRect.bottom() - 10));
                            path.lineTo(QPoint(pixmapRect.right() - 5, pixmapRect.bottom() - 14));
                            path.lineTo(QPoint(pixmapRect.left() + 1, pixmapRect.bottom() - 10));
                            path.lineTo(QPoint(pixmapRect.left() + 1, pixmapRect.bottom()));
                            path.lineTo(QPoint(pixmapRect.right(), pixmapRect.bottom()));
                        } else {
                            path.moveTo(QPoint(pixmapRect.right(), pixmapRect.top() + 1));
                            path.lineTo(QPoint(pixmapRect.right(), pixmapRect.top() + 10));
                            path.lineTo(QPoint(pixmapRect.right() - 5, pixmapRect.top() + 14));
                            path.lineTo(QPoint(pixmapRect.left() + 1, pixmapRect.top() + 10));
                            path.lineTo(QPoint(pixmapRect.left() + 1, pixmapRect.top() + 1));
                            path.lineTo(QPoint(pixmapRect.right(), pixmapRect.top() + 1));
                        }
                        if (slider->state & State_Enabled) {
                            QLinearGradient gradient(pixmapRect.center().x(), pixmapRect.top(),
                                                     pixmapRect.center().x(), pixmapRect.bottom());
                            if ((option->activeSubControls & SC_SliderHandle) && (option->state & State_Sunken)) {
                                gradient.setColorAt(0, gradientStartColor.lighter(110));
                                gradient.setColorAt(1, gradientStopColor.lighter(110));
                            } else {
                                gradient.setColorAt(0, gradientStartColor);
                                gradient.setColorAt(1, gradientStopColor);
                            }
                            handlePainter.fillPath(path, gradient);
                        } else {
                            handlePainter.fillPath(path, slider->palette.background());
                        }
                    } else {
                        QPainterPath path;
                        if (ticksAbove && !ticksBelow) {
                            path.moveTo(QPoint(pixmapRect.right(), pixmapRect.top() + 1));
                            path.lineTo(QPoint(pixmapRect.right() - 10, pixmapRect.top() + 1));
                            path.lineTo(QPoint(pixmapRect.right() - 14, pixmapRect.top() + 5));
                            path.lineTo(QPoint(pixmapRect.right() - 10, pixmapRect.bottom()));
                            path.lineTo(QPoint(pixmapRect.right(), pixmapRect.bottom()));
                            path.lineTo(QPoint(pixmapRect.right(), pixmapRect.top() + 1));
                        } else {
                            path.moveTo(QPoint(pixmapRect.left() + 1, pixmapRect.top() + 1));
                            path.lineTo(QPoint(pixmapRect.left() + 10, pixmapRect.top() + 1));
                            path.lineTo(QPoint(pixmapRect.left() + 14, pixmapRect.top() + 5));
                            path.lineTo(QPoint(pixmapRect.left() + 10, pixmapRect.bottom()));
                            path.lineTo(QPoint(pixmapRect.left() + 1, pixmapRect.bottom()));
                            path.lineTo(QPoint(pixmapRect.left() + 1, pixmapRect.top() + 1));
                        }
                        if (slider->state & State_Enabled) {
                            QLinearGradient gradient(pixmapRect.center().x(), pixmapRect.top(),
                                                     pixmapRect.center().x(), pixmapRect.bottom());
                            gradient.setColorAt(0, gradientStartColor);
                            gradient.setColorAt(1, gradientStopColor);
                            handlePainter.fillPath(path, gradient);
                        } else {
                            handlePainter.fillPath(path, slider->palette.background());
                        }
                    }

                    QImage image;
                    if (horizontal) {
                        image = QImage((ticksAbove && !ticksBelow) ? qt_plastique_slider_horizontalhandle_up : qt_plastique_slider_horizontalhandle);
                    } else {
                        image = QImage((ticksAbove && !ticksBelow) ? qt_plastique_slider_verticalhandle_left : qt_plastique_slider_verticalhandle);
                    }

                    image.setColor(1, borderColor.rgba());
                    image.setColor(2, gradientStartColor.rgba());
                    image.setColor(3, alphaCornerColor.rgba());
                    if (option->state & State_Enabled) {
                        image.setColor(4, 0x80ffffff);
                        image.setColor(5, 0x25000000);
                    }
                    handlePainter.drawImage(pixmapRect, image);
                    handlePainter.end();
                    QPixmapCache::insert(handlePixmapName, cache);
                }

                painter->drawPixmap(handle.topLeft(), cache);

                if (slider->state & State_HasFocus) {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=(*slider);
                    fropt.rect = subElementRect(SE_SliderFocusRect, slider, widget);
                    proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
                }
            }

            if (option->subControls & SC_SliderTickmarks) {
                QPen oldPen = painter->pen();
                painter->setPen(borderColor);
                int tickSize = proxy()->pixelMetric(PM_SliderTickmarkOffset, option, widget);
                int available = proxy()->pixelMetric(PM_SliderSpaceAvailable, slider, widget);
                int interval = slider->tickInterval;
                if (interval <= 0) {
                    interval = slider->singleStep;
                    if (QStyle::sliderPositionFromValue(slider->minimum, slider->maximum, interval,
                                                        available)
                        - QStyle::sliderPositionFromValue(slider->minimum, slider->maximum,
                                                          0, available) < 3)
                        interval = slider->pageStep;
                }
                if (interval <= 0)
                    interval = 1;

                int v = slider->minimum;
                int len = proxy()->pixelMetric(PM_SliderLength, slider, widget);
                QVarLengthArray<QLine, 32> lines;
                while (v <= slider->maximum + 1) {
                    if (v == slider->maximum + 1 && interval == 1)
                        break;
                    const int v_ = qMin(v, slider->maximum);
                    int pos = sliderPositionFromValue(slider->minimum, slider->maximum,
                                                      v_, (horizontal
                                                          ? slider->rect.width()
                                                          : slider->rect.height()) - len,
                                                      slider->upsideDown) + len / 2;

                    int extra = 2 - ((v_ == slider->minimum || v_ == slider->maximum) ? 1 : 0);

                    if (horizontal) {
                        if (ticksAbove) {
                            lines.append(QLine(pos, slider->rect.top() + extra,
                                               pos, slider->rect.top() + tickSize));
                        }
                        if (ticksBelow) {
                            lines.append(QLine(pos, slider->rect.bottom() - extra,
                                               pos, slider->rect.bottom() - tickSize));
                        }
                    } else {
                        if (ticksAbove) {
                            lines.append(QLine(slider->rect.left() + extra, pos,
                                               slider->rect.left() + tickSize, pos));
                        }
                        if (ticksBelow) {
                            lines.append(QLine(slider->rect.right() - extra, pos,
                                               slider->rect.right() - tickSize, pos));
                        }
                    }

                    // in the case where maximum is max int
                    int nextInterval = v + interval;
                    if (nextInterval < v)
                        break;
                    v = nextInterval;
                }
                painter->drawLines(lines.constData(), lines.size());
                painter->setPen(oldPen);
            }
        }
        break;
#endif // QT_NO_SLIDER
#ifndef QT_NO_SPINBOX
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            painter->save();
            bool upSunken = (spinBox->activeSubControls & SC_SpinBoxUp) && (spinBox->state & (State_Sunken | State_On));
            bool downSunken = (spinBox->activeSubControls & SC_SpinBoxDown) && (spinBox->state & (State_Sunken | State_On));
            bool reverse = (spinBox->direction == Qt::RightToLeft);

            // Rects
            QRect upRect = proxy()->subControlRect(CC_SpinBox, option, SC_SpinBoxUp, widget);
            QRect downRect = proxy()->subControlRect(CC_SpinBox, option, SC_SpinBoxDown, widget);
            QRect buttonRect = upRect | downRect;

            // Brushes
            QBrush corner = qMapBrushToRect(option->palette.shadow(), buttonRect);
            qBrushSetAlphaF(&corner, qreal(0.25));
            QBrush border = qMapBrushToRect(option->palette.shadow(), buttonRect);
            qBrushSetAlphaF(&border, qreal(0.4));

            QVarLengthArray<QPoint, 4> points;

            Q_D(const QPlastiqueStyle);
            if (spinBox->buttonSymbols == QAbstractSpinBox::NoButtons) {
                QRect filledRect = option->rect.adjusted(1, 1, -1, -1);
                QBrush baseBrush = qMapBrushToRect(option->palette.base(), filledRect);
                painter->setBrushOrigin(filledRect.topLeft());
                painter->fillRect(filledRect.adjusted(1, 1, -1, -1), baseBrush);
                qt_plastique_draw_frame(painter, option->rect, option, QFrame::Sunken);        
            } else {
                d->drawPartialFrame(painter,
                                    option,
                                    proxy()->subControlRect(CC_SpinBox, spinBox, SC_SpinBoxEditField, widget),
                                    widget);
            }
            // Paint buttons
            if (spinBox->buttonSymbols == QAbstractSpinBox::NoButtons) {
                painter->restore();
                break;
            }
            // Button outlines
            painter->setPen(QPen(border, 0));
            if (!reverse)
                painter->drawLine(buttonRect.topLeft() + QPoint(0, 1), buttonRect.bottomLeft() + QPoint(0, -1));
            else
                painter->drawLine(buttonRect.topRight() + QPoint(0, -1), buttonRect.bottomRight() + QPoint(0, 1));

            if (!reverse) {
                const QLine lines[4] = {
                    QLine(upRect.left(), upRect.top(), upRect.right() - 2, upRect.top()),
                    QLine(upRect.left() + 1, upRect.bottom(), upRect.right() - 1, upRect.bottom()),
                    QLine(downRect.left(), downRect.bottom(), downRect.right() - 2, downRect.bottom()),
                    QLine(buttonRect.right(), buttonRect.top() + 2, buttonRect.right(), buttonRect.bottom() - 2) };
                painter->drawLines(lines, 4);

                points.append(QPoint(upRect.right() - 1, upRect.top() + 1));
                points.append(QPoint(downRect.right() - 1, downRect.bottom() - 1));
                painter->drawPoints(points.constData(), points.size());
                points.clear();
                painter->setPen(QPen(corner, 0));
                points.append(QPoint(upRect.right() - 1, upRect.top()));
                points.append(QPoint(upRect.right(), upRect.top() + 1));
                points.append(QPoint(upRect.right(), downRect.bottom() - 1));
                points.append(QPoint(upRect.right() - 1, downRect.bottom()));
            } else {
                const QLine lines[4] = {
                    QLine(upRect.right(), upRect.top(), upRect.left() + 2, upRect.top()),
                    QLine(upRect.right() - 1, upRect.bottom(), upRect.left() + 1, upRect.bottom()),
                    QLine(downRect.right(), downRect.bottom(), downRect.left() + 2, downRect.bottom()),
                    QLine(buttonRect.left(), buttonRect.top() + 2, buttonRect.left(), buttonRect.bottom() - 2) };
                painter->drawLines(lines, 4);

                points.append(QPoint(upRect.left() + 1, upRect.top() + 1));
                points.append(QPoint(downRect.left() + 1, downRect.bottom() - 1));
                painter->drawPoints(points.constData(), points.size());
                points.clear();
                painter->setPen(QPen(corner, 0));
                points.append(QPoint(upRect.left() + 1, upRect.top()));
                points.append(QPoint(upRect.left(), upRect.top() + 1));
                points.append(QPoint(upRect.left(), downRect.bottom() - 1));
                points.append(QPoint(upRect.left() + 1, downRect.bottom()));
            }
            painter->drawPoints(points.constData(), points.size());
            points.clear();

            // Button colors
            QBrush buttonGradientBrush;
            QBrush leftLineGradientBrush;
            QBrush rightLineGradientBrush;
            QBrush sunkenButtonGradientBrush;
            QBrush sunkenLeftLineGradientBrush;
            QBrush sunkenRightLineGradientBrush;
            QBrush buttonBrush = qMapBrushToRect(option->palette.button(), buttonRect);
            if (buttonBrush.gradient() || !buttonBrush.texture().isNull()) {
                buttonGradientBrush = buttonBrush;
                sunkenButtonGradientBrush = qBrushDark(buttonBrush, 108);
                leftLineGradientBrush = qBrushLight(buttonBrush, 105);
                rightLineGradientBrush = qBrushDark(buttonBrush, 105);
                sunkenLeftLineGradientBrush = qBrushDark(buttonBrush, 110);
                sunkenRightLineGradientBrush = qBrushDark(buttonBrush, 106);
            } else {
                // Generate gradients
                QLinearGradient buttonGradient(buttonRect.topLeft(), buttonRect.bottomLeft());
                buttonGradient.setColorAt(0.0, buttonBrush.color().lighter(104));
                buttonGradient.setColorAt(1.0, buttonBrush.color().darker(110));
                buttonGradientBrush = QBrush(buttonGradient);

                QLinearGradient buttonGradient2(buttonRect.topLeft(), buttonRect.bottomLeft());
                buttonGradient2.setColorAt(0.0, buttonBrush.color().darker(113));
               buttonGradient2.setColorAt(1.0, buttonBrush.color().darker(103));
                sunkenButtonGradientBrush = QBrush(buttonGradient2);

                QLinearGradient buttonGradient3(buttonRect.topLeft(), buttonRect.bottomLeft());
                buttonGradient3.setColorAt(0.0, buttonBrush.color().lighter(105));
                buttonGradient3.setColorAt(1.0, buttonBrush.color());
                leftLineGradientBrush = QBrush(buttonGradient3);

                QLinearGradient buttonGradient4(buttonRect.topLeft(), buttonRect.bottomLeft());
                buttonGradient4.setColorAt(0.0, buttonBrush.color());
                buttonGradient4.setColorAt(1.0, buttonBrush.color().darker(110));
                rightLineGradientBrush = QBrush(buttonGradient4);

                QLinearGradient buttonGradient5(buttonRect.topLeft(), buttonRect.bottomLeft());
                buttonGradient5.setColorAt(0.0, buttonBrush.color().darker(113));
                buttonGradient5.setColorAt(1.0, buttonBrush.color().darker(107));
                sunkenLeftLineGradientBrush = QBrush(buttonGradient5);

                QLinearGradient buttonGradient6(buttonRect.topLeft(), buttonRect.bottomLeft());
                buttonGradient6.setColorAt(0.0, buttonBrush.color().darker(108));
                buttonGradient6.setColorAt(1.0, buttonBrush.color().darker(103));
                sunkenRightLineGradientBrush = QBrush(buttonGradient6);
            }

            // Main fill
            painter->fillRect(upRect.adjusted(2, 2, -2, -2),
                              qMapBrushToRect(upSunken ? sunkenButtonGradientBrush
                                              : buttonGradientBrush, upRect));
            painter->fillRect(downRect.adjusted(2, 2, -2, -2),
                              qMapBrushToRect(downSunken ? sunkenButtonGradientBrush
                                              : buttonGradientBrush, downRect));

            // Top line
            painter->setPen(QPen(qBrushLight(qMapBrushToRect(upSunken ? sunkenButtonGradientBrush
                                                             : buttonGradientBrush, upRect), 105), 0));
            if (!reverse) {
                painter->drawLine(upRect.left() + 1, upRect.top() + 1,
                                  upRect.right() - 2, upRect.top() + 1);
            } else {
                painter->drawLine(upRect.right() - 1, upRect.top() + 1,
                                  upRect.left() + 2, upRect.top() + 1);
            }
            painter->setPen(QPen(qBrushLight(qMapBrushToRect(downSunken ? sunkenButtonGradientBrush
                                                             : buttonGradientBrush, downRect), 105), 0));
            if (!reverse) {
                painter->drawLine(downRect.left() + 1, downRect.top() + 1,
                                  downRect.right() - 1, downRect.top() + 1);
            } else {
                painter->drawLine(downRect.right() - 1, downRect.top() + 1,
                                  downRect.left() + 1, downRect.top() + 1);
            }

            // Left line
            painter->setPen(QPen(qMapBrushToRect(upSunken ? sunkenLeftLineGradientBrush
                                                 : leftLineGradientBrush, upRect), 1));
            if (!reverse) {
                painter->drawLine(upRect.left() + 1, upRect.top() + 2,
                                  upRect.left() + 1, upRect.bottom() - 1);
            } else {
                painter->drawLine(upRect.left() + 1, upRect.top() + 2,
                                  upRect.left() + 1, upRect.bottom() - 1);
            }
            painter->setPen(QPen(qMapBrushToRect(downSunken ? sunkenLeftLineGradientBrush
                                                 : leftLineGradientBrush, downRect), 1));
            if (!reverse) {
                painter->drawLine(downRect.left() + 1, downRect.top() + 2,
                                  downRect.left() + 1, downRect.bottom() - 1);
            } else {
                painter->drawLine(downRect.left() + 1, downRect.top() + 1,
                                  downRect.left() + 1, downRect.bottom() - 2);
            }

            // Bottom line
            painter->setPen(QPen(qBrushDark(qMapBrushToRect(upSunken ? sunkenButtonGradientBrush
                                                            : buttonGradientBrush, upRect), 105), 0));
            if (!reverse) {
                painter->drawLine(upRect.left() + 2, upRect.bottom() - 1,
                                  upRect.right() - 1, upRect.bottom() - 1);
            } else {
                painter->drawLine(upRect.right() - 2, upRect.bottom() - 1,
                                  upRect.left() + 1, upRect.bottom() - 1);
            }
            painter->setPen(QPen(qBrushDark(qMapBrushToRect(downSunken ? sunkenButtonGradientBrush
                                                            : buttonGradientBrush, downRect), 105), 0));
            if (!reverse) {
                painter->drawLine(downRect.left() + 2, downRect.bottom() - 1,
                                  downRect.right() - 2, downRect.bottom() - 1);
            } else {
                painter->drawLine(downRect.right() - 2, downRect.bottom() - 1,
                                  downRect.left() + 2, downRect.bottom() - 1);
            }

            // Right line
            painter->setPen(QPen(qMapBrushToRect(upSunken ? sunkenRightLineGradientBrush
                                                 : rightLineGradientBrush, upRect), 1));
            if (!reverse) {
                painter->drawLine(upRect.right() - 1, upRect.top() + 2,
                                  upRect.right() - 1, upRect.bottom() - 1);
            } else {
                painter->drawLine(upRect.right() - 1, upRect.top() + 2,
                                  upRect.right() - 1, upRect.bottom() - 1);
            }
            painter->setPen(QPen(qMapBrushToRect(downSunken ? sunkenRightLineGradientBrush
                                                 : rightLineGradientBrush, downRect), 1));
            if (!reverse) {
                painter->drawLine(downRect.right() - 1, downRect.top() + 1,
                                  downRect.right() - 1, downRect.bottom() - 2);
            } else {
                painter->drawLine(downRect.right() - 1, downRect.top() + 2,
                                  downRect.right() - 1, downRect.bottom() - 1);
            }

            QBrush indicatorBrush = qMapBrushToRect(option->palette.buttonText(), buttonRect);
            painter->setPen(QPen(indicatorBrush, 0));
            if (spinBox->buttonSymbols == QAbstractSpinBox::PlusMinus) {
                QPoint center;
                if (spinBox->subControls & SC_SpinBoxUp) {
                    // .......
                    // ...X...
                    // ...X...
                    // .XXXXX.
                    // ...X...
                    // ...X...
                    // .......
                    center = upRect.center();
                    if (upSunken) {
                        ++center.rx();
                        ++center.ry();
                    }
                    painter->drawLine(center.x(), center.y() - 2, center.x(), center.y() + 2);
                    painter->drawLine(center.x() - 2, center.y(), center.x() + 2, center.y());
                }
                if (spinBox->subControls & SC_SpinBoxDown) {
                    // .......
                    // .......
                    // .......
                    // .XXXXX.
                    // .......
                    // .......
                    // .......
                    center = downRect.center();
                    if (downSunken) {
                        ++center.rx();
                        ++center.ry();
                    }
                    painter->drawLine(center.x() - 2, center.y(), center.x() + 2, center.y());
                }
            } else {
                int offset;
                int centerX;
                if (spinBox->subControls & SC_SpinBoxUp) {
                    // ...........
                    // .....X.....
                    // ....XXX....
                    // ...XXXXX...
                    // ..XXXXXXX..
                    // ...........
                    offset = upSunken ? 1 : 0;
                    QRect upArrowRect(upRect.center().x() - 3 + offset, upRect.center().y() - 2 + offset, 7, 4);
                    centerX = upArrowRect.center().x();
                    painter->drawPoint(centerX, upArrowRect.top());
                    const QLine lines[3] = {
                        QLine(centerX - 1, upArrowRect.top() + 1, centerX + 1, upArrowRect.top() + 1),
                        QLine(centerX - 2, upArrowRect.top() + 2, centerX + 2, upArrowRect.top() + 2),
                        QLine(centerX - 3, upArrowRect.top() + 3, centerX + 3, upArrowRect.top() + 3) };
                    painter->drawLines(lines, 3);
                }
                if (spinBox->subControls & SC_SpinBoxDown) {
                    // ...........
                    // ..XXXXXXX..
                    // ...XXXXX...
                    // ....XXX....
                    // .....X.....
                    // ...........
                    offset = downSunken ? 1 : 0;
                    QRect downArrowRect(downRect.center().x() - 3 + offset, downRect.center().y() - 2 + offset + 1, 7, 4);
                    centerX = downArrowRect.center().x();
                    const QLine lines[3] = {
                        QLine(centerX - 3, downArrowRect.top(), centerX + 3, downArrowRect.top()),
                        QLine(centerX - 2, downArrowRect.top() + 1, centerX + 2, downArrowRect.top() + 1),
                        QLine(centerX - 1, downArrowRect.top() + 2, centerX + 1, downArrowRect.top() + 2) };
                    painter->drawLines(lines, 3);
                    painter->drawPoint(centerX, downArrowRect.top() + 3);
                }
            }
            painter->restore();
        }
        break;
#endif // QT_NO_SPINBOX
#ifndef QT_NO_COMBOBOX
    case CC_ComboBox:
        if (const QStyleOptionComboBox *comboBox = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            bool sunken = comboBox->state & State_On; // play dead if combobox has no items
            bool reverse = comboBox->direction == Qt::RightToLeft;
            int menuButtonWidth = 16;
            int xoffset = sunken ? (reverse ? -1 : 1) : 0;
            int yoffset = sunken ? 1 : 0;
            QRect rect = comboBox->rect;
            QPen oldPen = painter->pen();

            // Fill
            if (comboBox->editable) {
                // Button colors
                QBrush alphaCornerBrush = qBrushDark(option->palette.button(), 165);
                qBrushSetAlphaF(&alphaCornerBrush, 0.5);
                QBrush buttonGradientBrush;
                QBrush leftLineGradientBrush;
                QBrush rightLineGradientBrush;
                QBrush sunkenButtonGradientBrush;
                QBrush sunkenLeftLineGradientBrush;
                QBrush sunkenRightLineGradientBrush;
                QBrush button = option->palette.button();
                if (button.gradient() || !button.texture().isNull()) {
                    buttonGradientBrush = button;
                    sunkenButtonGradientBrush = qBrushDark(button, 108);
                    leftLineGradientBrush = qBrushLight(button, 105);
                    rightLineGradientBrush = qBrushDark(button, 105);
                    sunkenLeftLineGradientBrush = qBrushDark(button, 110);
                    sunkenRightLineGradientBrush = qBrushDark(button, 106);
                } else {
                    // Generate gradients
                    QLinearGradient buttonGradient(option->rect.topLeft(), option->rect.bottomLeft());
                    buttonGradient.setColorAt(0.0, button.color().lighter(104));
                    buttonGradient.setColorAt(1.0, button.color().darker(110));
                    buttonGradientBrush = QBrush(buttonGradient);

                    QLinearGradient buttonGradient2(option->rect.topLeft(), option->rect.bottomLeft());
                    buttonGradient2.setColorAt(0.0, button.color().darker(113));
                    buttonGradient2.setColorAt(1.0, button.color().darker(103));
                    sunkenButtonGradientBrush = QBrush(buttonGradient2);

                    QLinearGradient buttonGradient3(option->rect.topLeft(), option->rect.bottomLeft());
                    buttonGradient3.setColorAt(0.0, button.color().lighter(105));
                    buttonGradient3.setColorAt(1.0, button.color());
                    leftLineGradientBrush = QBrush(buttonGradient3);

                    QLinearGradient buttonGradient4(option->rect.topLeft(), option->rect.bottomLeft());
                    buttonGradient4.setColorAt(0.0, button.color());
                    buttonGradient4.setColorAt(1.0, button.color().darker(110));
                    rightLineGradientBrush = QBrush(buttonGradient4);

                    QLinearGradient buttonGradient5(option->rect.topLeft(), option->rect.bottomLeft());
                    buttonGradient5.setColorAt(0.0, button.color().darker(113));
                    buttonGradient5.setColorAt(1.0, button.color().darker(107));
                    sunkenLeftLineGradientBrush = QBrush(buttonGradient5);

                    QLinearGradient buttonGradient6(option->rect.topLeft(), option->rect.bottomLeft());
                    buttonGradient6.setColorAt(0.0, button.color().darker(108));
                    buttonGradient6.setColorAt(1.0, button.color().darker(103));
                    sunkenRightLineGradientBrush = QBrush(buttonGradient6);
                }

                // ComboBox starts with a lineedit in place already.
                QRect buttonRect;
                if (!reverse) {
                    buttonRect.setRect(rect.right() - menuButtonWidth, rect.top(), menuButtonWidth + 1, rect.height());
                } else {
                    buttonRect.setRect(rect.left(), rect.top(), menuButtonWidth + 1, rect.height());
                }

                Q_D(const QPlastiqueStyle);
                d->drawPartialFrame(painter,
                                option,
                                proxy()->subControlRect(CC_ComboBox, option, SC_ComboBoxEditField, widget),
                                widget);

                QBrush border = qMapBrushToRect(option->palette.shadow(), buttonRect);
                qBrushSetAlphaF(&border, qreal(0.4));
                painter->setPen(QPen(border, 0));
                if (!reverse)
                    painter->drawLine(buttonRect.topLeft() + QPoint(0, 1), buttonRect.bottomLeft() + QPoint(0, -1));
                else
                    painter->drawLine(buttonRect.topRight() + QPoint(0, -1), buttonRect.bottomRight() + QPoint(0, 1));

                // Outline the button border
                if (!reverse) {
                    const QLine lines[3] = {
                        QLine(buttonRect.left(), buttonRect.top(),
                              buttonRect.right() - 2, buttonRect.top()),
                        QLine(buttonRect.right(), buttonRect.top() + 2,
                              buttonRect.right(), buttonRect.bottom() - 2),
                        QLine(buttonRect.left(), buttonRect.bottom(),
                              buttonRect.right() - 2, buttonRect.bottom()) };
                    painter->drawLines(lines, 3);
                    {
                        const QPoint points[2] = {
                            QPoint(buttonRect.right() - 1, buttonRect.top() + 1),
                            QPoint(buttonRect.right() - 1, buttonRect.bottom() - 1) };
                        painter->drawPoints(points, 2);
                    }

                    QBrush corner = qMapBrushToRect(option->palette.shadow(), buttonRect);
                    qBrushSetAlphaF(&corner, qreal(0.16));
                    painter->setPen(QPen(corner, 0));
                    {
                        const QPoint points[4] = {
                            QPoint(buttonRect.right() - 1, buttonRect.top()),
                            QPoint(buttonRect.right() - 1, buttonRect.bottom()),
                            QPoint(buttonRect.right(), buttonRect.top() + 1),
                            QPoint(buttonRect.right(), buttonRect.bottom() - 1) };
                        painter->drawPoints(points, 4);
                    }
                } else {
                    const QLine lines[3] = {
                        QLine(buttonRect.right(), buttonRect.top(),
                              buttonRect.left() + 2, buttonRect.top()),
                        QLine(buttonRect.left(), buttonRect.top() + 2,
                              buttonRect.left(), buttonRect.bottom() - 2),
                        QLine(buttonRect.right(), buttonRect.bottom(),
                              buttonRect.left() + 2, buttonRect.bottom()) };
                    painter->drawLines(lines, 3);
                    {
                        const QPoint points[2] = {
                            QPoint(buttonRect.left() + 1, buttonRect.top() + 1),
                            QPoint(buttonRect.left() + 1, buttonRect.bottom() - 1) };
                        painter->drawPoints(points, 2);
                    }

                    QBrush corner = qMapBrushToRect(option->palette.shadow(), buttonRect);
                    qBrushSetAlphaF(&corner, qreal(0.16));
                    painter->setPen(QPen(corner, 0));
                    {
                        const QPoint points[4] = {
                            QPoint(buttonRect.left() + 1, buttonRect.top()),
                            QPoint(buttonRect.left() + 1, buttonRect.bottom()),
                            QPoint(buttonRect.left(), buttonRect.top() + 1),
                            QPoint(buttonRect.left(), buttonRect.bottom() - 1) };
                        painter->drawPoints(points, 4);
                    }
                }

                QRect fillRect = buttonRect.adjusted(2, 2, -2, -2);
                // Main fill
                painter->fillRect(fillRect,
                                  qMapBrushToRect(sunken ? sunkenButtonGradientBrush
                                                  : buttonGradientBrush, option->rect));

                // Top line
                painter->setPen(QPen(qBrushLight(qMapBrushToRect(sunken ? sunkenButtonGradientBrush
                                                                 : buttonGradientBrush, option->rect), 105), 0));
                if (!reverse) {
                    painter->drawLine(QPointF(buttonRect.left() + 1, buttonRect.top() + 1),
                                      QPointF(buttonRect.right() - 2, buttonRect.top() + 1));
                } else {
                    painter->drawLine(QPointF(buttonRect.right() - 1, buttonRect.top() + 1),
                                      QPointF(buttonRect.left() + 2, buttonRect.top() + 1));
                }

                // Bottom line
                painter->setPen(QPen(qBrushDark(qMapBrushToRect(sunken ? sunkenButtonGradientBrush
                                                                : buttonGradientBrush, option->rect), 105), 0));
                if (!reverse) {
                    painter->drawLine(QPointF(buttonRect.left() + 1, buttonRect.bottom() - 1),
                                      QPointF(buttonRect.right() - 2, buttonRect.bottom() - 1));
                } else {
                    painter->drawLine(QPointF(buttonRect.right() - 1, buttonRect.bottom() - 1),
                                      QPointF(buttonRect.left() + 2, buttonRect.bottom() - 1));
                }

                // Left line
                painter->setPen(QPen(qMapBrushToRect(sunken ? sunkenLeftLineGradientBrush
                                                     : leftLineGradientBrush, option->rect), 1));
                if (!reverse) {
                    painter->drawLine(QPointF(buttonRect.left() + 1, buttonRect.top() + 2),
                                      QPointF(buttonRect.left() + 1, buttonRect.bottom() - 2));
                } else {
                    painter->drawLine(QPointF(buttonRect.left() + 1, buttonRect.top() + 2),
                                      QPointF(buttonRect.left() + 1, buttonRect.bottom() - 2));
                }

                // Right line
                painter->setPen(QPen(qMapBrushToRect(sunken ? sunkenRightLineGradientBrush
                                                     : rightLineGradientBrush, option->rect), 1));
                if (!reverse) {
                    painter->drawLine(QPointF(buttonRect.right() - 1, buttonRect.top() + 2),
                                      QPointF(buttonRect.right() - 1, buttonRect.bottom() - 2));
                } else {
                    painter->drawLine(QPointF(buttonRect.right() - 1, buttonRect.top() + 2),
                                      QPointF(buttonRect.right() - 1, buttonRect.bottom() - 2));
                }
            } else {
                // Start with a standard panel button fill
                QStyleOptionButton buttonOption;
                buttonOption.QStyleOption::operator=(*comboBox);
                if (!sunken) {
                    buttonOption.state &= ~State_Sunken;
                }
                proxy()->drawPrimitive(PE_PanelButtonCommand, &buttonOption, painter, widget);

                // Draw the menu button separator line
                QBrush border = qMapBrushToRect(option->palette.shadow(), rect);
                qBrushSetAlphaF(&border, qreal(0.35));
                painter->setPen(QPen(border, 0));
                if (!reverse) {
                    painter->drawLine(rect.right() - menuButtonWidth + xoffset, rect.top() + 1,
                                      rect.right() - menuButtonWidth + xoffset, rect.bottom() - 1);
                } else {
                    painter->drawLine(rect.left() + menuButtonWidth + xoffset, rect.top() + 1,
                                      rect.left() + menuButtonWidth + xoffset, rect.bottom() - 1);
                }
            }

            // Draw the little arrow
            if (comboBox->subControls & SC_ComboBoxArrow) {
                int left = !reverse ? rect.right() - menuButtonWidth : rect.left();
                int right = !reverse ? rect.right() : rect.left() + menuButtonWidth;
                QRect arrowRect((left + right) / 2 - 3 + xoffset,
                                rect.center().y() - 1 + yoffset, 7, 4);
                painter->setPen(QPen(qMapBrushToRect(option->palette.buttonText(), rect), 0));
                const QLine lines[3] = {
                    QLine(arrowRect.topLeft(), arrowRect.topRight()),
                    QLine(arrowRect.left() + 1, arrowRect.top() + 1,
                          arrowRect.right() - 1, arrowRect.top() + 1),
                    QLine(arrowRect.left() + 2, arrowRect.top() + 2,
                          arrowRect.right() - 2, arrowRect.top() + 2) };
                painter->drawLines(lines, 3);
                painter->drawPoint(arrowRect.center().x(), arrowRect.bottom());
            }

            // Draw the focus rect
            if ((option->state & State_HasFocus) && !comboBox->editable
                && ((option->state & State_KeyboardFocusChange) || styleHint(SH_UnderlineShortcut, option, widget))) {
                QStyleOptionFocusRect focus;
                focus.rect = proxy()->subControlRect(CC_ComboBox, option, SC_ComboBoxEditField, widget)
                             .adjusted(-2, 0, 2, 0);
                proxy()->drawPrimitive(PE_FrameFocusRect, &focus, painter, widget);
            }

            painter->setPen(oldPen);
        }
        break;
#endif // QT_NO_COMBOBOX
    case CC_TitleBar:
        if (const QStyleOptionTitleBar *titleBar = qstyleoption_cast<const QStyleOptionTitleBar *>(option)) {
            painter->save();
            bool active = (titleBar->titleBarState & State_Active);
            QRect fullRect = titleBar->rect;

            // ### use palette colors instead
            QColor titleBarGradientStart(active ? 0x3b508a : 0x6e6e6e);
            QColor titleBarGradientStop(active ? 0x5d6e9e : 0x818181);
            QColor titleBarFrameBorder(0x393939);
            QColor titleBarAlphaCorner(active ? 0x4b5e7f : 0x6a6a6a);
            QColor titleBarInnerTopLine(active ? 0x8e98ba : 0xa4a4a4);
            QColor titleBarInnerInnerTopLine(active ? 0x57699b : 0x808080);
            QColor leftCorner(active ? 0x6f7ea8 : 0x8e8e8e);
            QColor rightCorner(active ? 0x44537d : 0x676767);
            QColor textColor(active ? 0x282e40 : 0x282e40);
            QColor textAlphaColor(active ? 0x3f4862 : 0x3f4862);

#ifdef  QT3_SUPPORT
            if (widget && widget->inherits("Q3DockWindowTitleBar")) {
                QStyleOptionDockWidgetV2 dockwidget;
                dockwidget.QStyleOption::operator=(*option);
                dockwidget.title = titleBar->text;
                proxy()->drawControl(CE_DockWidgetTitle, &dockwidget, painter, widget);
            } else
#endif // QT3_SUPPORT

            {
                // Fill title bar gradient
                qt_plastique_draw_gradient(painter, option->rect.adjusted(1, 1, -1, 0),
                                           titleBarGradientStart,
                                           titleBarGradientStop);

                // Frame and rounded corners
                painter->setPen(titleBarFrameBorder);

                // top border line
                {
                    const QLine lines[3] = {
                        QLine(fullRect.left() + 2, fullRect.top(), fullRect.right() - 2, fullRect.top()),
                        QLine(fullRect.left(), fullRect.top() + 2, fullRect.left(), fullRect.bottom()),
                        QLine(fullRect.right(), fullRect.top() + 2, fullRect.right(), fullRect.bottom()) };
                    painter->drawLines(lines, 3);
                    const QPoint points[2] = {
                        QPoint(fullRect.left() + 1, fullRect.top() + 1),
                        QPoint(fullRect.right() - 1, fullRect.top() + 1) };
                    painter->drawPoints(points, 2);
                }

                // alpha corners
                painter->setPen(titleBarAlphaCorner);
                {
                    const QPoint points[4] = {
                        QPoint(fullRect.left() + 2, fullRect.top() + 1),
                        QPoint(fullRect.left() + 1, fullRect.top() + 2),
                        QPoint(fullRect.right() - 2, fullRect.top() + 1),
                        QPoint(fullRect.right() - 1, fullRect.top() + 2) };
                    painter->drawPoints(points, 4);
                }

                // inner top line
                painter->setPen(titleBarInnerTopLine);
                painter->drawLine(fullRect.left() + 3, fullRect.top() + 1, fullRect.right() - 3, fullRect.top() + 1);

                // inner inner top line
                painter->setPen(titleBarInnerInnerTopLine);
                painter->drawLine(fullRect.left() + 2, fullRect.top() + 2, fullRect.right() - 2, fullRect.top() + 2);

                // left and right inner
                painter->setPen(leftCorner);
                painter->drawLine(fullRect.left() + 1, fullRect.top() + 3, fullRect.left() + 1, fullRect.bottom());
                painter->setPen(rightCorner);
                painter->drawLine(fullRect.right() - 1, fullRect.top() + 3, fullRect.right() - 1, fullRect.bottom());

                if (titleBar->titleBarState & Qt::WindowMinimized) {
                    painter->setPen(titleBarFrameBorder);
                    painter->drawLine(fullRect.left() + 2, fullRect.bottom(), fullRect.right() - 2, fullRect.bottom());
                    {
                        const QPoint points[2] = {
                            QPoint(fullRect.left() + 1, fullRect.bottom() - 1),
                            QPoint(fullRect.right() - 1, fullRect.bottom() - 1) };
                        painter->drawPoints(points, 2);
                    }
                    painter->setPen(rightCorner);
                    painter->drawLine(fullRect.left() + 2, fullRect.bottom() - 1, fullRect.right() - 2, fullRect.bottom() - 1);
                    painter->setPen(titleBarAlphaCorner);
                    {
                        const QPoint points[4] = {
                            QPoint(fullRect.left() + 1, fullRect.bottom() - 2),
                            QPoint(fullRect.left() + 2, fullRect.bottom() - 1),
                            QPoint(fullRect.right() - 1, fullRect.bottom() - 2),
                            QPoint(fullRect.right() - 2, fullRect.bottom() - 1) };
                        painter->drawPoints(points, 4);
                    }
                }
                // draw title
                QRect textRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarLabel, widget);

                QFont font = painter->font();
                font.setBold(true);
                painter->setFont(font);
                painter->setPen(titleBar->palette.text().color());

                // Attempt to align left if there is not enough room for the title
                // text. Otherwise, align center. QWorkspace does elliding for us,
                // and it doesn't know about the bold title, so we need to work
                // around some of the width mismatches.
                bool tooWide = (QFontMetrics(font).width(titleBar->text) > textRect.width());
                QTextOption option((tooWide ? Qt::AlignLeft : Qt::AlignHCenter) | Qt::AlignVCenter);
                option.setWrapMode(QTextOption::NoWrap);

                painter->drawText(textRect.adjusted(1, 1, 1, 1), titleBar->text, option);
                painter->setPen(titleBar->palette.highlightedText().color());
                painter->drawText(textRect, titleBar->text, option);
            }

            // min button
            if ((titleBar->subControls & SC_TitleBarMinButton)
                    && (titleBar->titleBarFlags & Qt::WindowMinimizeButtonHint)
                    && !(titleBar->titleBarState & Qt::WindowMinimized)) {
                bool hover = (titleBar->activeSubControls & SC_TitleBarMinButton) && (titleBar->state & State_MouseOver);
                bool sunken = (titleBar->activeSubControls & SC_TitleBarMinButton) && (titleBar->state & State_Sunken);

                QRect minButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarMinButton, widget);
                qt_plastique_draw_mdibutton(painter, titleBar, minButtonRect, hover, sunken);

                int xoffset = minButtonRect.width() / 3;
                int yoffset = minButtonRect.height() / 3;

                QRect minButtonIconRect(minButtonRect.left() + xoffset, minButtonRect.top() + yoffset,
                                        minButtonRect.width() - xoffset * 2, minButtonRect.height() - yoffset * 2);

                painter->setPen(textColor);
                {
                    const QLine lines[2] = {
                        QLine(minButtonIconRect.center().x() - 2,
                              minButtonIconRect.center().y() + 3,
                              minButtonIconRect.center().x() + 3,
                              minButtonIconRect.center().y() + 3),
                        QLine(minButtonIconRect.center().x() - 2,
                              minButtonIconRect.center().y() + 4,
                              minButtonIconRect.center().x() + 3,
                              minButtonIconRect.center().y() + 4) };
                    painter->drawLines(lines, 2);
                }
                painter->setPen(textAlphaColor);
                {
                    const QLine lines[2] = {
                        QLine(minButtonIconRect.center().x() - 3,
                              minButtonIconRect.center().y() + 3,
                              minButtonIconRect.center().x() - 3,
                              minButtonIconRect.center().y() + 4),
                        QLine(minButtonIconRect.center().x() + 4,
                              minButtonIconRect.center().y() + 3,
                              minButtonIconRect.center().x() + 4,
                              minButtonIconRect.center().y() + 4) };
                    painter->drawLines(lines, 2);
                }
            }

            // max button
            if ((titleBar->subControls & SC_TitleBarMaxButton)
                    && (titleBar->titleBarFlags & Qt::WindowMaximizeButtonHint)
                    && !(titleBar->titleBarState & Qt::WindowMaximized)) {
                bool hover = (titleBar->activeSubControls & SC_TitleBarMaxButton) && (titleBar->state & State_MouseOver);
                bool sunken = (titleBar->activeSubControls & SC_TitleBarMaxButton) && (titleBar->state & State_Sunken);

                QRect maxButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarMaxButton, widget);
                qt_plastique_draw_mdibutton(painter, titleBar, maxButtonRect, hover, sunken);

                int xoffset = maxButtonRect.width() / 3;
                int yoffset = maxButtonRect.height() / 3;

                QRect maxButtonIconRect(maxButtonRect.left() + xoffset, maxButtonRect.top() + yoffset,
                                        maxButtonRect.width() - xoffset * 2, maxButtonRect.height() - yoffset * 2);

                painter->setPen(textColor);
                painter->drawRect(maxButtonIconRect.adjusted(0, 0, -1, -1));
                painter->drawLine(maxButtonIconRect.left() + 1, maxButtonIconRect.top() + 1,
                                  maxButtonIconRect.right() - 1, maxButtonIconRect.top() + 1);
                painter->setPen(textAlphaColor);
                const QPoint points[4] = {
                    maxButtonIconRect.topLeft(), maxButtonIconRect.topRight(),
                    maxButtonIconRect.bottomLeft(), maxButtonIconRect.bottomRight() };
                painter->drawPoints(points, 4);
            }

            // close button
            if (titleBar->subControls & SC_TitleBarCloseButton && titleBar->titleBarFlags & Qt::WindowSystemMenuHint) {
                bool hover = (titleBar->activeSubControls & SC_TitleBarCloseButton) && (titleBar->state & State_MouseOver);
                bool sunken = (titleBar->activeSubControls & SC_TitleBarCloseButton) && (titleBar->state & State_Sunken);

                QRect closeButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarCloseButton, widget);
                qt_plastique_draw_mdibutton(painter, titleBar, closeButtonRect, hover, sunken);

                int xoffset = closeButtonRect.width() / 3;
                int yoffset = closeButtonRect.height() / 3;

                QRect closeIconRect(closeButtonRect.left() + xoffset, closeButtonRect.top() + yoffset,
                                    closeButtonRect.width() - xoffset * 2, closeButtonRect.height() - yoffset * 2);

                painter->setPen(textAlphaColor);
                {
                    const QLine lines[4] = {
                        QLine(closeIconRect.left() + 1, closeIconRect.top(),
                              closeIconRect.right(), closeIconRect.bottom() - 1),
                        QLine(closeIconRect.left(), closeIconRect.top() + 1,
                              closeIconRect.right() - 1, closeIconRect.bottom()),
                        QLine(closeIconRect.right() - 1, closeIconRect.top(),
                              closeIconRect.left(), closeIconRect.bottom() - 1),
                        QLine(closeIconRect.right(), closeIconRect.top() + 1,
                              closeIconRect.left() + 1, closeIconRect.bottom()) };
                    painter->drawLines(lines, 4);
                    const QPoint points[4] = {
                        closeIconRect.topLeft(), closeIconRect.topRight(),
                        closeIconRect.bottomLeft(), closeIconRect.bottomRight() };
                    painter->drawPoints(points, 4);
                }
                painter->setPen(textColor);
                {
                    const QLine lines[2] = {
                        QLine(closeIconRect.left() + 1, closeIconRect.top() + 1,
                              closeIconRect.right() - 1, closeIconRect.bottom() - 1),
                        QLine(closeIconRect.left() + 1, closeIconRect.bottom() - 1,
                              closeIconRect.right() - 1, closeIconRect.top() + 1) };
                    painter->drawLines(lines, 2);
                }
            }

            // normalize button
            if ((titleBar->subControls & SC_TitleBarNormalButton) &&
                (((titleBar->titleBarFlags & Qt::WindowMinimizeButtonHint) &&
                 (titleBar->titleBarState & Qt::WindowMinimized)) ||
                 ((titleBar->titleBarFlags & Qt::WindowMaximizeButtonHint) &&
                  (titleBar->titleBarState & Qt::WindowMaximized)))) {
                bool hover = (titleBar->activeSubControls & SC_TitleBarNormalButton) && (titleBar->state & State_MouseOver);
                bool sunken = (titleBar->activeSubControls & SC_TitleBarNormalButton) && (titleBar->state & State_Sunken);

                QRect normalButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarNormalButton, widget);
                qt_plastique_draw_mdibutton(painter, titleBar, normalButtonRect, hover, sunken);
                int xoffset = int(normalButtonRect.width() / 3.5);
                int yoffset = int(normalButtonRect.height() / 3.5);

                QRect normalButtonIconRect(normalButtonRect.left() + xoffset, normalButtonRect.top() + yoffset,
                                           normalButtonRect.width() - xoffset * 2, normalButtonRect.height() - yoffset * 2);

                QRect frontWindowRect = normalButtonIconRect.adjusted(0, 3, -3, 0);
                painter->setPen(textColor);
                painter->drawRect(frontWindowRect.adjusted(0, 0, -1, -1));
                painter->drawLine(frontWindowRect.left() + 1, frontWindowRect.top() + 1,
                                  frontWindowRect.right() - 1, frontWindowRect.top() + 1);
                painter->setPen(textAlphaColor);
                {
                    const QPoint points[4] = {
                        frontWindowRect.topLeft(), frontWindowRect.topRight(),
                        frontWindowRect.bottomLeft(), frontWindowRect.bottomRight() };
                    painter->drawPoints(points, 4);
                }

                QRect backWindowRect = normalButtonIconRect.adjusted(3, 0, 0, -3);
                QRegion clipRegion = backWindowRect;
                clipRegion -= frontWindowRect;
                painter->save();
                painter->setClipRegion(clipRegion);
                painter->setPen(textColor);
                painter->drawRect(backWindowRect.adjusted(0, 0, -1, -1));
                painter->drawLine(backWindowRect.left() + 1, backWindowRect.top() + 1,
                                  backWindowRect.right() - 1, backWindowRect.top() + 1);
                painter->setPen(textAlphaColor);
                {
                    const QPoint points[4] = {
                        backWindowRect.topLeft(), backWindowRect.topRight(),
                        backWindowRect.bottomLeft(), backWindowRect.bottomRight() };
                    painter->drawPoints(points, 4);
                }
                painter->restore();
            }

            // context help button
            if (titleBar->subControls & SC_TitleBarContextHelpButton
                && (titleBar->titleBarFlags & Qt::WindowContextHelpButtonHint)) {
                bool hover = (titleBar->activeSubControls & SC_TitleBarContextHelpButton) && (titleBar->state & State_MouseOver);
                bool sunken = (titleBar->activeSubControls & SC_TitleBarContextHelpButton) && (titleBar->state & State_Sunken);

                QRect contextHelpButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarContextHelpButton, widget);

                qt_plastique_draw_mdibutton(painter, titleBar, contextHelpButtonRect, hover, sunken);

                QColor blend;
                // ### Use palette colors
                if (active) {
                    blend = mergedColors(QColor(hover ? 0x7d8bb1 : 0x55689a),
                                         QColor(hover ? 0x939ebe : 0x7381ab));
                } else {
                    blend = mergedColors(QColor(hover ? 0x9e9e9e : 0x818181),
                                         QColor(hover ? 0xababab : 0x929292));
                }
                QImage image(qt_titlebar_context_help);
                image.setColor(4, textColor.rgba());
                image.setColor(3, mergedColors(blend, textColor, 30).rgba());
                image.setColor(2, mergedColors(blend, textColor, 70).rgba());
                image.setColor(1, mergedColors(blend, textColor, 90).rgba());

                painter->drawImage(contextHelpButtonRect, image);
            }

            // shade button
            if (titleBar->subControls & SC_TitleBarShadeButton) {
                bool hover = (titleBar->activeSubControls & SC_TitleBarShadeButton) && (titleBar->state & State_MouseOver);
                bool sunken = (titleBar->activeSubControls & SC_TitleBarShadeButton) && (titleBar->state & State_Sunken);

                QRect shadeButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarShadeButton, widget);
                qt_plastique_draw_mdibutton(painter, titleBar, shadeButtonRect, hover, sunken);

                int xoffset = shadeButtonRect.width() / 3;
                int yoffset = shadeButtonRect.height() / 3;

                QRect shadeButtonIconRect(shadeButtonRect.left() + xoffset, shadeButtonRect.top() + yoffset,
                                          shadeButtonRect.width() - xoffset * 2, shadeButtonRect.height() - yoffset * 2);

                QPainterPath path(shadeButtonIconRect.bottomLeft());
                path.lineTo(shadeButtonIconRect.center().x(), shadeButtonIconRect.bottom() - shadeButtonIconRect.height() / 2);
                path.lineTo(shadeButtonIconRect.bottomRight());
                path.lineTo(shadeButtonIconRect.bottomLeft());

                painter->setPen(textAlphaColor);
                painter->setBrush(textColor);
                painter->drawPath(path);
            }

            // unshade button
            if (titleBar->subControls & SC_TitleBarUnshadeButton) {
                bool hover = (titleBar->activeSubControls & SC_TitleBarUnshadeButton) && (titleBar->state & State_MouseOver);
                bool sunken = (titleBar->activeSubControls & SC_TitleBarUnshadeButton) && (titleBar->state & State_Sunken);

                QRect unshadeButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarUnshadeButton, widget);
                qt_plastique_draw_mdibutton(painter, titleBar, unshadeButtonRect, hover, sunken);

                int xoffset = unshadeButtonRect.width() / 3;
                int yoffset = unshadeButtonRect.height() / 3;

                QRect unshadeButtonIconRect(unshadeButtonRect.left() + xoffset, unshadeButtonRect.top() + yoffset,
                                          unshadeButtonRect.width() - xoffset * 2, unshadeButtonRect.height() - yoffset * 2);

                int midY = unshadeButtonIconRect.bottom() - unshadeButtonIconRect.height() / 2;
                QPainterPath path(QPoint(unshadeButtonIconRect.left(), midY));
                path.lineTo(unshadeButtonIconRect.right(), midY);
                path.lineTo(unshadeButtonIconRect.center().x(), unshadeButtonIconRect.bottom());
                path.lineTo(unshadeButtonIconRect.left(), midY);

                painter->setPen(textAlphaColor);
                painter->setBrush(textColor);
                painter->drawPath(path);
            }

            // from qwindowsstyle.cpp
            if ((titleBar->subControls & SC_TitleBarSysMenu) && (titleBar->titleBarFlags & Qt::WindowSystemMenuHint)) {
                bool hover = (titleBar->activeSubControls & SC_TitleBarSysMenu) && (titleBar->state & State_MouseOver);
                bool sunken = (titleBar->activeSubControls & SC_TitleBarSysMenu) && (titleBar->state & State_Sunken);

                QRect iconRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarSysMenu, widget);
                if (hover)
                    qt_plastique_draw_mdibutton(painter, titleBar, iconRect, hover, sunken);

                if (!titleBar->icon.isNull()) {
                    titleBar->icon.paint(painter, iconRect);
                } else {
                    QStyleOption tool(0);
                    tool.palette = titleBar->palette;
                    QPixmap pm = standardPixmap(SP_TitleBarMenuButton, &tool, widget);
                    tool.rect = iconRect;
                    painter->save();
                    proxy()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, pm);
                    painter->restore();
                }
            }
            painter->restore();
        }
        break;
#ifndef QT_NO_DIAL
    case CC_Dial:
        if (const QStyleOptionSlider *dial = qstyleoption_cast<const QStyleOptionSlider *>(option))
            QStyleHelper::drawDial(dial, painter);
        break;
#endif // QT_NO_DIAL
    default:
        QWindowsStyle::drawComplexControl(control, option, painter, widget);
        break;
    }
}

/*!
  \reimp
*/
QSize QPlastiqueStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
                                        const QSize &size, const QWidget *widget) const
{
    QSize newSize = QWindowsStyle::sizeFromContents(type, option, size, widget);

    switch (type) {
    case CT_RadioButton:
        ++newSize.rheight();
        ++newSize.rwidth();
        break;
#ifndef QT_NO_SLIDER
    case CT_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            int tickSize = proxy()->pixelMetric(PM_SliderTickmarkOffset, option, widget);
            if (slider->tickPosition & QSlider::TicksBelow) {
                if (slider->orientation == Qt::Horizontal)
                    newSize.rheight() += tickSize;
                else
                    newSize.rwidth() += tickSize;
            }
            if (slider->tickPosition & QSlider::TicksAbove) {
                if (slider->orientation == Qt::Horizontal)
                    newSize.rheight() += tickSize;
                else
                    newSize.rwidth() += tickSize;
            }
        }
        break;
#endif // QT_NO_SLIDER
#ifndef QT_NO_SCROLLBAR
    case CT_ScrollBar:
        if (const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            int scrollBarExtent = proxy()->pixelMetric(PM_ScrollBarExtent, option, widget);
            int scrollBarSliderMinimum = proxy()->pixelMetric(PM_ScrollBarSliderMin, option, widget);
            if (scrollBar->orientation == Qt::Horizontal) {
                newSize = QSize(scrollBarExtent * 3 + scrollBarSliderMinimum, scrollBarExtent);
            } else {
                newSize = QSize(scrollBarExtent, scrollBarExtent * 3 + scrollBarSliderMinimum);
            }
        }
        break;
#endif // QT_NO_SCROLLBAR
#ifndef QT_NO_SPINBOX
    case CT_SpinBox:
        // Make sure the size is odd
        newSize.setHeight(sizeFromContents(CT_LineEdit, option, size, widget).height());
        newSize.rheight() -= ((1 - newSize.rheight()) & 1);
        break;
#endif
#ifndef QT_NO_TOOLBUTTON
    case CT_ToolButton:
        newSize.rheight() += 3;
        newSize.rwidth() += 3;
        break;
#endif
#ifndef QT_NO_COMBOBOX
    case CT_ComboBox:
        newSize = sizeFromContents(CT_PushButton, option, size, widget);
        newSize.rwidth() += 30; // Make room for drop-down indicator
        newSize.rheight() += 4;
        break;
#endif
    case CT_MenuItem:
        if (const QStyleOptionMenuItem *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            if (menuItem->menuItemType == QStyleOptionMenuItem::Separator)
                newSize.setHeight(menuItem->text.isEmpty() ? 2 : menuItem->fontMetrics.height());
        }
        break;
    case CT_MenuBarItem:
        newSize.setHeight(newSize.height());
        break;
    default:
        break;
    }

    return newSize;
}

/*!
  \reimp
*/
QRect QPlastiqueStyle::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const
{
    QRect rect;
    switch (element) {
    case SE_RadioButtonIndicator:
        rect = visualRect(option->direction, option->rect,
                          QWindowsStyle::subElementRect(element, option, widget)).adjusted(0, 0, 1, 1);
        break;
#ifndef QT_NO_PROGRESSBAR
    case SE_ProgressBarLabel:
    case SE_ProgressBarContents:
    case SE_ProgressBarGroove:
        return option->rect;
#endif // QT_NO_PROGRESSBAR
    default:
        return QWindowsStyle::subElementRect(element, option, widget);
    }

    return visualRect(option->direction, option->rect, rect);
}

/*!
  \reimp
*/
QRect QPlastiqueStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                                      SubControl subControl, const QWidget *widget) const
{
    QRect rect = QWindowsStyle::subControlRect(control, option, subControl, widget);

    switch (control) {
#ifndef QT_NO_SLIDER
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            int tickSize = proxy()->pixelMetric(PM_SliderTickmarkOffset, option, widget);

            switch (subControl) {
            case SC_SliderHandle:
                if (slider->orientation == Qt::Horizontal) {
                    rect.setWidth(11);
                    rect.setHeight(15);
                    int centerY = slider->rect.center().y() - rect.height() / 2;
                    if (slider->tickPosition & QSlider::TicksAbove)
                        centerY += tickSize;
                    if (slider->tickPosition & QSlider::TicksBelow)
                        centerY -= tickSize;
                    rect.moveTop(centerY);
                } else {
                    rect.setWidth(15);
                    rect.setHeight(11);
                    int centerX = slider->rect.center().x() - rect.width() / 2;
                    if (slider->tickPosition & QSlider::TicksAbove)
                        centerX += tickSize;
                    if (slider->tickPosition & QSlider::TicksBelow)
                        centerX -= tickSize;
                    rect.moveLeft(centerX);
                }
                break;
            case SC_SliderGroove: {
                QPoint grooveCenter = slider->rect.center();
                if (slider->orientation == Qt::Horizontal) {
                    rect.setHeight(14);
                    --grooveCenter.ry();
                    if (slider->tickPosition & QSlider::TicksAbove)
                        grooveCenter.ry() += tickSize;
                    if (slider->tickPosition & QSlider::TicksBelow)
                        grooveCenter.ry() -= tickSize;
                } else {
                    rect.setWidth(14);
                    --grooveCenter.rx();
                    if (slider->tickPosition & QSlider::TicksAbove)
                        grooveCenter.rx() += tickSize;
                    if (slider->tickPosition & QSlider::TicksBelow)
                        grooveCenter.rx() -= tickSize;
                }
                rect.moveCenter(grooveCenter);
                break;
            }
            default:
                break;
            }
        }
        break;
#endif // QT_NO_SLIDER
#ifndef QT_NO_SCROLLBAR
    case CC_ScrollBar:
        if (const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            int scrollBarExtent = proxy()->pixelMetric(PM_ScrollBarExtent, scrollBar, widget);
            int sliderMaxLength = ((scrollBar->orientation == Qt::Horizontal) ?
                                   scrollBar->rect.width() : scrollBar->rect.height()) - (scrollBarExtent * 3);
            int sliderMinLength = proxy()->pixelMetric(PM_ScrollBarSliderMin, scrollBar, widget);
            int sliderLength;

            // calculate slider length
            if (scrollBar->maximum != scrollBar->minimum) {
                uint valueRange = scrollBar->maximum - scrollBar->minimum;
                sliderLength = (scrollBar->pageStep * sliderMaxLength) / (valueRange + scrollBar->pageStep);

                if (sliderLength < sliderMinLength || valueRange > INT_MAX / 2)
                    sliderLength = sliderMinLength;
                if (sliderLength > sliderMaxLength)
                    sliderLength = sliderMaxLength;
            } else {
                sliderLength = sliderMaxLength;
            }

            int sliderStart = scrollBarExtent + sliderPositionFromValue(scrollBar->minimum,
                                                           scrollBar->maximum,
                                                           scrollBar->sliderPosition,
                                                           sliderMaxLength - sliderLength,
                                                           scrollBar->upsideDown);

            QRect scrollBarRect = scrollBar->rect;

            switch (subControl) {
            case SC_ScrollBarSubLine: // top/left button
                if (scrollBar->orientation == Qt::Horizontal) {
                    rect.setRect(scrollBarRect.left(), scrollBarRect.top(), scrollBarRect.width() - scrollBarExtent, scrollBarRect.height());
                } else {
                    rect.setRect(scrollBarRect.left(), scrollBarRect.top(), scrollBarRect.width(), scrollBarRect.height() - scrollBarExtent);
                }
                break;
            case SC_ScrollBarAddLine: // bottom/right button
                if (scrollBar->orientation == Qt::Horizontal) {
                    rect.setRect(scrollBarRect.right() - (scrollBarExtent - 1), scrollBarRect.top(), scrollBarExtent, scrollBarRect.height());
                } else {
                    rect.setRect(scrollBarRect.left(), scrollBarRect.bottom() - (scrollBarExtent - 1), scrollBarRect.width(), scrollBarExtent);
                }
                break;
            case SC_ScrollBarSubPage:
                if (scrollBar->orientation == Qt::Horizontal) {
                    rect.setRect(scrollBarRect.left() + scrollBarExtent, scrollBarRect.top(),
                                 sliderStart - (scrollBarRect.left() + scrollBarExtent), scrollBarRect.height());
                } else {
                    rect.setRect(scrollBarRect.left(), scrollBarRect.top() + scrollBarExtent,
                                 scrollBarRect.width(), sliderStart - (scrollBarRect.left() + scrollBarExtent));
                }
                break;
            case SC_ScrollBarAddPage:
                if (scrollBar->orientation == Qt::Horizontal)
                    rect.setRect(sliderStart + sliderLength, 0,
                                 sliderMaxLength - sliderStart - sliderLength + scrollBarExtent, scrollBarRect.height());
                else
                    rect.setRect(0, sliderStart + sliderLength,
                                 scrollBarRect.width(), sliderMaxLength - sliderStart - sliderLength + scrollBarExtent);
                break;
            case SC_ScrollBarGroove:
                if (scrollBar->orientation == Qt::Horizontal) {
                    rect = scrollBarRect.adjusted(scrollBarExtent, 0, -2 * scrollBarExtent, 0);
                } else {
                    rect = scrollBarRect.adjusted(0, scrollBarExtent, 0, -2 * scrollBarExtent);
                }
                break;
            case SC_ScrollBarSlider:
                if (scrollBar->orientation == Qt::Horizontal) {
                    rect.setRect(sliderStart, 0, sliderLength, scrollBarRect.height());
                } else {
                    rect.setRect(0, sliderStart, scrollBarRect.width(), sliderLength);
                }
                break;
            default:
                break;
            }
            rect = visualRect(scrollBar->direction, scrollBarRect, rect);
        }
        break;
#endif // QT_NO_SCROLLBAR
#ifndef QT_NO_SPINBOX
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            int center = spinBox->rect.height() / 2;
            switch (subControl) {
            case SC_SpinBoxUp:
                if (spinBox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();
                rect = visualRect(spinBox->direction, spinBox->rect, rect);
                rect.setRect(spinBox->rect.right() - 16, spinBox->rect.top(), 17, center + 1);
                rect = visualRect(spinBox->direction, spinBox->rect, rect);
                break;
            case SC_SpinBoxDown:
                if (spinBox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();
                rect = visualRect(spinBox->direction, spinBox->rect, rect);
                rect.setRect(spinBox->rect.right() - 16, spinBox->rect.top() + center, 17, spinBox->rect.height() - center);
                rect = visualRect(spinBox->direction, spinBox->rect, rect);
                break;
            case SC_SpinBoxEditField:
                if (spinBox->buttonSymbols != QAbstractSpinBox::NoButtons) {
                    rect = spinBox->rect.adjusted(0, 0, -16, 0);
                } else {
                    rect = spinBox->rect;
                }
                rect.adjust(blueFrameWidth, blueFrameWidth, -blueFrameWidth, -blueFrameWidth);
                rect = visualRect(spinBox->direction, spinBox->rect, rect);
                break;
            default:
                break;
            }
        }
        break;
#endif // QT_NO_SPINBOX
#ifndef QT_NO_COMBOBOX
    case CC_ComboBox:
        switch (subControl) {
        case SC_ComboBoxArrow:
            rect = visualRect(option->direction, option->rect, rect);
            rect.setRect(rect.right() - 17, rect.top() - 2,
                         19, rect.height() + 4);
            rect = visualRect(option->direction, option->rect, rect);
            break;
        case SC_ComboBoxEditField: {
            if (const QStyleOptionComboBox *box = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
                int frameWidth = proxy()->pixelMetric(PM_DefaultFrameWidth);
                rect = visualRect(option->direction, option->rect, rect);

                if (box->editable) {
                    rect = box->rect.adjusted(blueFrameWidth, blueFrameWidth, -blueFrameWidth, -blueFrameWidth);
                    rect.setRight(rect.right() - 16); // Overlaps the combobox button by 2 pixels
                } else {
                    rect.setRect(option->rect.left() + frameWidth, option->rect.top() + frameWidth,
                                 option->rect.width() - 16 - 2 * frameWidth,
                                 option->rect.height() - 2 * frameWidth);
                    rect.setLeft(rect.left() + 2);
                    rect.setRight(rect.right() - 2);
                    if (box->state & (State_Sunken | State_On))
                        rect.translate(1, 1);
                }
                rect = visualRect(option->direction, option->rect, rect);
            }
            break;
        }
        default:
            break;
        }
        break;
#endif // QT_NO_COMBOBOX
    case CC_TitleBar:
        if (const QStyleOptionTitleBar *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(option)) {
            SubControl sc = subControl;
            QRect &ret = rect;
            const int indent = 3;
            const int controlTopMargin = 4;
            const int controlBottomMargin = 3;
            const int controlWidthMargin = 1;
            const int controlHeight = tb->rect.height() - controlTopMargin - controlBottomMargin;
            const int delta = controlHeight + controlWidthMargin;
            int offset = 0;

            bool isMinimized = tb->titleBarState & Qt::WindowMinimized;
            bool isMaximized = tb->titleBarState & Qt::WindowMaximized;

            switch (sc) {
            case SC_TitleBarLabel:
                if (tb->titleBarFlags & (Qt::WindowTitleHint | Qt::WindowSystemMenuHint)) {
                    ret = tb->rect;
                    if (tb->titleBarFlags & Qt::WindowSystemMenuHint)
                        ret.adjust(delta, 0, -delta, 0);
                    if (tb->titleBarFlags & Qt::WindowMinimizeButtonHint)
                        ret.adjust(0, 0, -delta, 0);
                    if (tb->titleBarFlags & Qt::WindowMaximizeButtonHint)
                        ret.adjust(0, 0, -delta, 0);
                    if (tb->titleBarFlags & Qt::WindowShadeButtonHint)
                        ret.adjust(0, 0, -delta, 0);
                    if (tb->titleBarFlags & Qt::WindowContextHelpButtonHint)
                        ret.adjust(0, 0, -delta, 0);
                    ret.adjusted(indent, 0, -indent, 0);
                }
                break;
            case SC_TitleBarContextHelpButton:
                if (tb->titleBarFlags & Qt::WindowContextHelpButtonHint)
                    offset += delta;
            case SC_TitleBarMinButton:
                if (!isMinimized && (tb->titleBarFlags & Qt::WindowMinimizeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarMinButton)
                    break;
            case SC_TitleBarNormalButton:
                if (isMinimized && (tb->titleBarFlags & Qt::WindowMinimizeButtonHint))
                    offset += delta;
                else if (isMaximized && (tb->titleBarFlags & Qt::WindowMaximizeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarNormalButton)
                    break;
            case SC_TitleBarMaxButton:
                if (!isMaximized && (tb->titleBarFlags & Qt::WindowMaximizeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarMaxButton)
                    break;
            case SC_TitleBarShadeButton:
                if (!isMinimized && (tb->titleBarFlags & Qt::WindowShadeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarShadeButton)
                    break;
            case SC_TitleBarUnshadeButton:
                if (isMinimized && (tb->titleBarFlags & Qt::WindowShadeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarUnshadeButton)
                    break;
            case SC_TitleBarCloseButton:
                if (tb->titleBarFlags & Qt::WindowSystemMenuHint)
                    offset += delta;
                else if (sc == SC_TitleBarCloseButton)
                    break;
                ret.setRect(tb->rect.right() - indent - offset, tb->rect.top() + controlTopMargin,
                            controlHeight, controlHeight);
                break;
            case SC_TitleBarSysMenu:
                if (tb->titleBarFlags & Qt::WindowSystemMenuHint) {
                    ret.setRect(tb->rect.left() + controlWidthMargin + indent, tb->rect.top() + controlTopMargin,
                                controlHeight, controlHeight);
                }
                break;
            default:
                break;
            }
            ret = visualRect(tb->direction, tb->rect, ret);
        }
        break;
    default:
        break;
    }

    return rect;
}

/*!
  \reimp
*/
int QPlastiqueStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                               QStyleHintReturn *returnData) const
{
    int ret = 0;
    switch (hint) {
    case SH_WindowFrame_Mask:
        ret = 1;
        if (QStyleHintReturnMask *mask = qstyleoption_cast<QStyleHintReturnMask *>(returnData)) {
            mask->region = option->rect;
            mask->region -= QRect(option->rect.left(), option->rect.top(), 2, 1);
            mask->region -= QRect(option->rect.right() - 1, option->rect.top(), 2, 1);
            mask->region -= QRect(option->rect.left(), option->rect.top() + 1, 1, 1);
            mask->region -= QRect(option->rect.right(), option->rect.top() + 1, 1, 1);

            const QStyleOptionTitleBar *titleBar = qstyleoption_cast<const QStyleOptionTitleBar *>(option);
            if (titleBar && (titleBar->titleBarState & Qt::WindowMinimized)) {
                mask->region -= QRect(option->rect.left(), option->rect.bottom(), 2, 1);
                mask->region -= QRect(option->rect.right() - 1, option->rect.bottom(), 2, 1);
                mask->region -= QRect(option->rect.left(), option->rect.bottom() - 1, 1, 1);
                mask->region -= QRect(option->rect.right(), option->rect.bottom() - 1, 1, 1);
            } else {
                mask->region -= QRect(option->rect.bottomLeft(), QSize(1, 1));
                mask->region -= QRect(option->rect.bottomRight(), QSize(1, 1));
            }
        }
        break;
    case SH_TitleBar_NoBorder:
        ret = 1;
        break;
    case SH_TitleBar_AutoRaise:
        ret = 1;
        break;
    case SH_ItemView_ShowDecorationSelected:
        ret = true;
        break;
    case SH_ToolBox_SelectedPageTitleBold:
    case SH_ScrollBar_MiddleClickAbsolutePosition:
        ret = true;
        break;
    case SH_MainWindow_SpaceBelowMenuBar:
        ret = 0;
        break;
    case SH_FormLayoutWrapPolicy:
        ret = QFormLayout::DontWrapRows;
        break;
    case SH_FormLayoutFieldGrowthPolicy:
        ret = QFormLayout::ExpandingFieldsGrow;
        break;
    case SH_FormLayoutFormAlignment:
        ret = Qt::AlignLeft | Qt::AlignTop;
        break;
    case SH_FormLayoutLabelAlignment:
        ret = Qt::AlignRight;
        break;
    case SH_MessageBox_TextInteractionFlags:
        ret = Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse;
        break;
    case SH_LineEdit_PasswordCharacter:
        ret = QCommonStyle::styleHint(hint, option, widget, returnData);
        break;
    case SH_ItemView_ArrowKeysNavigateIntoChildren:
        ret = true;
        break;
    case SH_Menu_SubMenuPopupDelay:
        ret = 96; // from Plastik
        break;
#ifdef Q_WS_X11
    case SH_DialogButtonBox_ButtonsHaveIcons:
        ret = true;
        break;
#endif
#ifndef Q_OS_WIN
    case SH_Menu_AllowActiveAndDisabled:
        ret = false;
        break;
#endif
    default:
        ret = QWindowsStyle::styleHint(hint, option, widget, returnData);
        break;
    }
    return ret;
}

/*!
  \reimp
*/
QStyle::SubControl QPlastiqueStyle::hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                                          const QPoint &pos, const QWidget *widget) const
{
    SubControl ret = SC_None;
    switch (control) {
#ifndef QT_NO_SCROLLBAR
    case CC_ScrollBar:
        if (const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            QRect slider = proxy()->subControlRect(control, scrollBar, SC_ScrollBarSlider, widget);
            if (slider.contains(pos)) {
                ret = SC_ScrollBarSlider;
                break;
            }

            QRect scrollBarAddLine = proxy()->subControlRect(control, scrollBar, SC_ScrollBarAddLine, widget);
            if (scrollBarAddLine.contains(pos)) {
                ret = SC_ScrollBarAddLine;
                break;
            }

            QRect scrollBarSubPage = proxy()->subControlRect(control, scrollBar, SC_ScrollBarSubPage, widget);
            if (scrollBarSubPage.contains(pos)) {
                ret = SC_ScrollBarSubPage;
                break;
            }

            QRect scrollBarAddPage = proxy()->subControlRect(control, scrollBar, SC_ScrollBarAddPage, widget);
            if (scrollBarAddPage.contains(pos)) {
                ret = SC_ScrollBarAddPage;
                break;
            }

            QRect scrollBarSubLine = proxy()->subControlRect(control, scrollBar, SC_ScrollBarSubLine, widget);
            if (scrollBarSubLine.contains(pos)) {
                ret = SC_ScrollBarSubLine;
                break;
            }
        }
        break;
#endif // QT_NO_SCROLLBAR
    default:
        break;
    }

    return ret != SC_None ? ret : QWindowsStyle::hitTestComplexControl(control, option, pos, widget);
}

/*!
  \reimp
*/
int QPlastiqueStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    int ret = -1;
    switch (metric) {
    case PM_MenuVMargin:
    case PM_MenuHMargin:
        ret = 0;
        break;
    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical:
        ret = 1;
        break;
    case PM_ButtonDefaultIndicator:
        ret = 0;
        break;
#ifndef QT_NO_SLIDER
    case PM_SliderThickness:
        ret = 15;
        break;
    case PM_SliderLength:
    case PM_SliderControlThickness:
        ret = 11;
        break;
    case PM_SliderTickmarkOffset:
        ret = 5;
        break;
    case PM_SliderSpaceAvailable:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            int size = 15;
            if (slider->tickPosition & QSlider::TicksBelow)
                ++size;
            if (slider->tickPosition & QSlider::TicksAbove)
                ++size;
            ret = size;
            break;
        }
#endif // QT_NO_SLIDER
    case PM_ScrollBarExtent:
        ret = 16;
        break;
    case PM_ScrollBarSliderMin:
        ret = 26;
        break;
    case PM_ProgressBarChunkWidth:
        ret = 1;
        break;
    case PM_MenuBarItemSpacing:
        ret = 3;
        break;
    case PM_MenuBarVMargin:
        ret = 2;
        break;
    case PM_MenuBarHMargin:
        ret = 0;
        break;
    case PM_MenuBarPanelWidth:
        ret = 1;
        break;
    case PM_ToolBarHandleExtent:
        ret = 9;
        break;
    case PM_ToolBarSeparatorExtent:
        ret = 2;
        break;
    case PM_ToolBarItemSpacing:
        ret = 1;
        break;
    case PM_ToolBarItemMargin:
        ret = 1;
        break;
    case PM_ToolBarFrameWidth:
        ret = 2;
        break;
    case PM_SplitterWidth:
        ret = 6;
        break;
    case PM_DockWidgetSeparatorExtent:
        ret = 6;
        break;
    case PM_DockWidgetHandleExtent:
        ret = 20;
        break;
    case PM_DefaultFrameWidth:
#ifndef QT_NO_MENU
        if (qobject_cast<const QMenu *>(widget)) {
            ret = 1;
            break;
        }
#endif
        ret = 2;
        break;
    case PM_MdiSubWindowFrameWidth:
        ret = 4;
        break;
    case PM_TitleBarHeight:
#ifdef QT3_SUPPORT
        if (widget && widget->inherits("Q3DockWindowTitleBar")) {
            // Q3DockWindow has smaller title bars than QDockWidget
            ret = qMax(widget->fontMetrics().height(), 20);
        } else
#endif
        ret = qMax(widget ? widget->fontMetrics().height() :
                   (option ? option->fontMetrics.height() : 0), 30);
        break;
    case PM_MaximumDragDistance:
        return -1;
    case PM_DockWidgetTitleMargin:
        return 2;
    case PM_LayoutHorizontalSpacing:
    case PM_LayoutVerticalSpacing:
        return -1;  // rely on layoutHorizontalSpacing()
    case PM_LayoutLeftMargin:
    case PM_LayoutTopMargin:
    case PM_LayoutRightMargin:
    case PM_LayoutBottomMargin:
        {
            bool isWindow = false;
            if (option) {
                isWindow = (option->state & State_Window);
            } else if (widget) {
                isWindow = widget->isWindow();
            }

            if (isWindow) {
                ret = 11;
            } else {
                ret = 9;
            }
        }
    default:
        break;
    }

    return ret != -1 ? ret : QWindowsStyle::pixelMetric(metric, option, widget);
}

/*!
  \reimp
*/
QPalette QPlastiqueStyle::standardPalette() const
{
    QPalette palette;

    palette.setBrush(QPalette::Disabled, QPalette::WindowText, QColor(QRgb(0xff808080)));
    palette.setBrush(QPalette::Disabled, QPalette::Button, QColor(QRgb(0xffdddfe4)));
    palette.setBrush(QPalette::Disabled, QPalette::Light, QColor(QRgb(0xffffffff)));
    palette.setBrush(QPalette::Disabled, QPalette::Midlight, QColor(QRgb(0xffffffff)));
    palette.setBrush(QPalette::Disabled, QPalette::Dark, QColor(QRgb(0xff555555)));
    palette.setBrush(QPalette::Disabled, QPalette::Mid, QColor(QRgb(0xffc7c7c7)));
    palette.setBrush(QPalette::Disabled, QPalette::Text, QColor(QRgb(0xffc7c7c7)));
    palette.setBrush(QPalette::Disabled, QPalette::BrightText, QColor(QRgb(0xffffffff)));
    palette.setBrush(QPalette::Disabled, QPalette::ButtonText, QColor(QRgb(0xff808080)));
    palette.setBrush(QPalette::Disabled, QPalette::Base, QColor(QRgb(0xffefefef)));
    palette.setBrush(QPalette::Disabled, QPalette::AlternateBase, palette.color(QPalette::Disabled, QPalette::Base).darker(110));
    palette.setBrush(QPalette::Disabled, QPalette::Window, QColor(QRgb(0xffefefef)));
    palette.setBrush(QPalette::Disabled, QPalette::Shadow, QColor(QRgb(0xff000000)));
    palette.setBrush(QPalette::Disabled, QPalette::Highlight, QColor(QRgb(0xff567594)));
    palette.setBrush(QPalette::Disabled, QPalette::HighlightedText, QColor(QRgb(0xffffffff)));
    palette.setBrush(QPalette::Disabled, QPalette::Link, QColor(QRgb(0xff0000ee)));
    palette.setBrush(QPalette::Disabled, QPalette::LinkVisited, QColor(QRgb(0xff52188b)));
    palette.setBrush(QPalette::Active, QPalette::WindowText, QColor(QRgb(0xff000000)));
    palette.setBrush(QPalette::Active, QPalette::Button, QColor(QRgb(0xffdddfe4)));
    palette.setBrush(QPalette::Active, QPalette::Light, QColor(QRgb(0xffffffff)));
    palette.setBrush(QPalette::Active, QPalette::Midlight, QColor(QRgb(0xffffffff)));
    palette.setBrush(QPalette::Active, QPalette::Dark, QColor(QRgb(0xff555555)));
    palette.setBrush(QPalette::Active, QPalette::Mid, QColor(QRgb(0xffc7c7c7)));
    palette.setBrush(QPalette::Active, QPalette::Text, QColor(QRgb(0xff000000)));
    palette.setBrush(QPalette::Active, QPalette::BrightText, QColor(QRgb(0xffffffff)));
    palette.setBrush(QPalette::Active, QPalette::ButtonText, QColor(QRgb(0xff000000)));
    palette.setBrush(QPalette::Active, QPalette::Base, QColor(QRgb(0xffffffff)));
    palette.setBrush(QPalette::Active, QPalette::AlternateBase, palette.color(QPalette::Active, QPalette::Base).darker(110));
    palette.setBrush(QPalette::Active, QPalette::Window, QColor(QRgb(0xffefefef)));
    palette.setBrush(QPalette::Active, QPalette::Shadow, QColor(QRgb(0xff000000)));
    palette.setBrush(QPalette::Active, QPalette::Highlight, QColor(QRgb(0xff678db2)));
    palette.setBrush(QPalette::Active, QPalette::HighlightedText, QColor(QRgb(0xffffffff)));
    palette.setBrush(QPalette::Active, QPalette::Link, QColor(QRgb(0xff0000ee)));
    palette.setBrush(QPalette::Active, QPalette::LinkVisited, QColor(QRgb(0xff52188b)));
    palette.setBrush(QPalette::Inactive, QPalette::WindowText, QColor(QRgb(0xff000000)));
    palette.setBrush(QPalette::Inactive, QPalette::Button, QColor(QRgb(0xffdddfe4)));
    palette.setBrush(QPalette::Inactive, QPalette::Light, QColor(QRgb(0xffffffff)));
    palette.setBrush(QPalette::Inactive, QPalette::Midlight, QColor(QRgb(0xffffffff)));
    palette.setBrush(QPalette::Inactive, QPalette::Dark, QColor(QRgb(0xff555555)));
    palette.setBrush(QPalette::Inactive, QPalette::Mid, QColor(QRgb(0xffc7c7c7)));
    palette.setBrush(QPalette::Inactive, QPalette::Text, QColor(QRgb(0xff000000)));
    palette.setBrush(QPalette::Inactive, QPalette::BrightText, QColor(QRgb(0xffffffff)));
    palette.setBrush(QPalette::Inactive, QPalette::ButtonText, QColor(QRgb(0xff000000)));
    palette.setBrush(QPalette::Inactive, QPalette::Base, QColor(QRgb(0xffffffff)));
    palette.setBrush(QPalette::Inactive, QPalette::AlternateBase, palette.color(QPalette::Inactive, QPalette::Base).darker(110));
    palette.setBrush(QPalette::Inactive, QPalette::Window, QColor(QRgb(0xffefefef)));
    palette.setBrush(QPalette::Inactive, QPalette::Shadow, QColor(QRgb(0xff000000)));
    palette.setBrush(QPalette::Inactive, QPalette::Highlight, QColor(QRgb(0xff678db2)));
    palette.setBrush(QPalette::Inactive, QPalette::HighlightedText, QColor(QRgb(0xffffffff)));
    palette.setBrush(QPalette::Inactive, QPalette::Link, QColor(QRgb(0xff0000ee)));
    palette.setBrush(QPalette::Inactive, QPalette::LinkVisited, QColor(QRgb(0xff52188b)));
    return palette;
}

/*!
  \reimp
*/
void QPlastiqueStyle::polish(QWidget *widget)
{
    if (qobject_cast<QPushButton *>(widget)
#ifndef QT_NO_COMBOBOX
        || qobject_cast<QComboBox *>(widget)
#endif
#ifndef QT_NO_SPINBOX
        || qobject_cast<QAbstractSpinBox *>(widget)
#endif
        || qobject_cast<QCheckBox *>(widget)
#ifndef QT_NO_GROUPBOX
        || qobject_cast<QGroupBox *>(widget)
#endif
        || qobject_cast<QRadioButton *>(widget)
#ifndef QT_NO_SPLITTER
        || qobject_cast<QSplitterHandle *>(widget)
#endif
#ifndef QT_NO_TABBAR
        || qobject_cast<QTabBar *>(widget)
#endif
        ) {
        widget->setAttribute(Qt::WA_Hover);
    }

    if (widget->inherits("QWorkspaceTitleBar")
        || widget->inherits("QDockSeparator")
        || widget->inherits("QDockWidgetSeparator")
        || widget->inherits("Q3DockWindowResizeHandle")) {
        widget->setAttribute(Qt::WA_Hover);
    }

    if (false // to simplify the #ifdefs
#ifndef QT_NO_MENUBAR
        || qobject_cast<QMenuBar *>(widget)
#endif
#ifdef QT3_SUPPORT
        || widget->inherits("Q3ToolBar")
#endif
#ifndef QT_NO_TOOLBAR
        || qobject_cast<QToolBar *>(widget)
        || (widget && qobject_cast<QToolBar *>(widget->parent()))
#endif
        ) {
        widget->setBackgroundRole(QPalette::Window);
    }

#ifndef QT_NO_PROGRESSBAR
    if (AnimateBusyProgressBar && qobject_cast<QProgressBar *>(widget))
        widget->installEventFilter(this);
#endif

#if defined QPlastique_MaskButtons
    if (qobject_cast<QPushButton *>(widget) || qobject_cast<QToolButton *>(widget))
        widget->installEventFilter(this);
#endif
}

/*!
  \reimp
*/
void QPlastiqueStyle::unpolish(QWidget *widget)
{
    if (qobject_cast<QPushButton *>(widget)
#ifndef QT_NO_COMBOBOX
        || qobject_cast<QComboBox *>(widget)
#endif
#ifndef QT_NO_SPINBOX
        || qobject_cast<QAbstractSpinBox *>(widget)
#endif
        || qobject_cast<QCheckBox *>(widget)
#ifndef QT_NO_GROUPBOX
        || qobject_cast<QGroupBox *>(widget)
#endif
#ifndef QT_NO_SPLITTER
        || qobject_cast<QSplitterHandle *>(widget)
#endif
#ifndef QT_NO_TABBAR
        || qobject_cast<QTabBar *>(widget)
#endif
        || qobject_cast<QRadioButton *>(widget)) {
        widget->setAttribute(Qt::WA_Hover, false);
    }

    if (widget->inherits("QWorkspaceTitleBar")
        || widget->inherits("QDockSeparator")
        || widget->inherits("QDockWidgetSeparator")
        || widget->inherits("Q3DockWindowResizeHandle")) {
        widget->setAttribute(Qt::WA_Hover, false);
    }

    if (false // to simplify the #ifdefs
#ifndef QT_NO_MENUBAR
        || qobject_cast<QMenuBar *>(widget)
#endif
#ifndef QT_NO_TOOLBOX
        || qobject_cast<QToolBox *>(widget)
#endif
#ifdef QT3_SUPPORT
        || widget->inherits("Q3ToolBar")
#endif
#ifndef QT_NO_TOOLBAR
        || qobject_cast<QToolBar *>(widget)
        || (widget && qobject_cast<QToolBar *>(widget->parent()))
#endif
        ) {
        widget->setBackgroundRole(QPalette::Button);
    }

#ifndef QT_NO_PROGRESSBAR
    if (AnimateBusyProgressBar && qobject_cast<QProgressBar *>(widget)) {
        Q_D(QPlastiqueStyle);
        widget->removeEventFilter(this);
        d->bars.removeAll(static_cast<QProgressBar*>(widget));
    }
#endif

#if defined QPlastique_MaskButtons
    if (qobject_cast<QPushButton *>(widget) || qobject_cast<QToolButton *>(widget))
        widget->removeEventFilter(this);
#endif
}

/*!
  \reimp
*/
void QPlastiqueStyle::polish(QApplication *app)
{
    QWindowsStyle::polish(app);
}

/*!
  \reimp
*/
void QPlastiqueStyle::polish(QPalette &pal)
{
    QWindowsStyle::polish(pal);
#ifdef Q_WS_MAC
    pal.setBrush(QPalette::Shadow, Qt::black);
#endif
}

/*!
  \reimp
*/
void QPlastiqueStyle::unpolish(QApplication *app)
{
    QWindowsStyle::unpolish(app);
}

/*!
    \internal
*/
QIcon QPlastiqueStyle::standardIconImplementation(StandardPixmap standardIcon, const QStyleOption *option,
                                                  const QWidget *widget) const
{
    return QWindowsStyle::standardIconImplementation(standardIcon, option, widget);
}

/*!
    \reimp
*/
QPixmap QPlastiqueStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt,
                                        const QWidget *widget) const
{
    return QWindowsStyle::standardPixmap(standardPixmap, opt, widget);
}

// this works as long as we have at most 16 different control types
#define CT1(c) CT2(c, c)
#define CT2(c1, c2) (((uint)c1 << 16) | (uint)c2)

/*!
    \internal
*/
int QPlastiqueStyle::layoutSpacingImplementation(QSizePolicy::ControlType control1,
                                                 QSizePolicy::ControlType control2,
                                                 Qt::Orientation orientation,
                                                 const QStyleOption * /* option */,
                                                 const QWidget * /* widget */) const
{
    const int ButtonMask = QSizePolicy::ButtonBox | QSizePolicy::PushButton;

    if (control2 == QSizePolicy::ButtonBox)
        return 11;

    if ((control1 | control2) & ButtonMask)
        return (orientation == Qt::Horizontal) ? 10 : 9;

    switch (CT2(control1, control2)) {
    case CT1(QSizePolicy::Label):
    case CT2(QSizePolicy::Label, QSizePolicy::DefaultType):
    case CT2(QSizePolicy::Label, QSizePolicy::CheckBox):
    case CT2(QSizePolicy::Label, QSizePolicy::ComboBox):
    case CT2(QSizePolicy::Label, QSizePolicy::LineEdit):
    case CT2(QSizePolicy::Label, QSizePolicy::RadioButton):
    case CT2(QSizePolicy::Label, QSizePolicy::Slider):
    case CT2(QSizePolicy::Label, QSizePolicy::SpinBox):
    case CT2(QSizePolicy::Label, QSizePolicy::ToolButton):
        return 5;
    case CT2(QSizePolicy::CheckBox, QSizePolicy::RadioButton):
    case CT2(QSizePolicy::RadioButton, QSizePolicy::CheckBox):
    case CT1(QSizePolicy::CheckBox):
        if (orientation == Qt::Vertical)
            return 2;
    case CT1(QSizePolicy::RadioButton):
        if (orientation == Qt::Vertical)
            return 1;
    }

    if (orientation == Qt::Horizontal
            && (control2 & (QSizePolicy::CheckBox | QSizePolicy::RadioButton)))
        return 8;

    if ((control1 | control2) & (QSizePolicy::Frame
                                 | QSizePolicy::GroupBox
                                 | QSizePolicy::TabWidget)) {
        return 11;
    }

    if ((control1 | control2) & (QSizePolicy::Line | QSizePolicy::Slider
                                 | QSizePolicy::LineEdit | QSizePolicy::ComboBox
                                 | QSizePolicy::SpinBox))
        return 7;

    return 6;
}

/*!
    \reimp
*/
bool QPlastiqueStyle::eventFilter(QObject *watched, QEvent *event)
{
#ifndef QT_NO_PROGRESSBAR
    Q_D(QPlastiqueStyle);

    switch (event->type()) {
    case QEvent::Show:
        if (QProgressBar *bar = qobject_cast<QProgressBar *>(watched)) {
            d->bars.append(bar);
            if (d->bars.size() == 1) {
                Q_ASSERT(ProgressBarFps > 0);
                d->timer.start();
                d->progressBarAnimateTimer = startTimer(1000 / ProgressBarFps);
            }
        }
        break;
    case QEvent::Destroy:
    case QEvent::Hide:
        if(!d->bars.isEmpty()) {
            d->bars.removeAll(reinterpret_cast<QProgressBar*>(watched));
            if (d->bars.isEmpty()) {
                killTimer(d->progressBarAnimateTimer);
                d->progressBarAnimateTimer = 0;
            }
        }
        break;
#if defined QPlastique_MaskButtons
    case QEvent::Resize:
        if (qobject_cast<QPushButton *>(watched) || qobject_cast<QToolButton *>(watched)) {
            QWidget *widget = qobject_cast<QWidget *>(watched);
            QRect rect = widget->rect();
            QRegion region(rect);
            region -= QRect(rect.left(), rect.top(), 2, 1);
            region -= QRect(rect.left(), rect.top() + 1, 1, 1);
            region -= QRect(rect.left(), rect.bottom(), 2, 1);
            region -= QRect(rect.left(), rect.bottom() - 1, 1, 1);
            region -= QRect(rect.right() - 1, rect.top(), 2, 1);
            region -= QRect(rect.right(), rect.top() + 1, 1, 1);
            region -= QRect(rect.right() - 1, rect.bottom(), 2, 1);
            region -= QRect(rect.right(), rect.bottom() - 1, 1, 1);
            widget->setMask(region);
        }
        break;
#endif
    default:
        break;
    }
#endif // QT_NO_PROGRESSBAR

    return QWindowsStyle::eventFilter(watched, event);
}

/*!
    \reimp
*/
void QPlastiqueStyle::timerEvent(QTimerEvent *event)
{
#ifndef QT_NO_PROGRESSBAR
    Q_D(QPlastiqueStyle);

    if (event->timerId() == d->progressBarAnimateTimer) {
        Q_ASSERT(ProgressBarFps > 0);
        d->animateStep = d->timer.elapsed() / (1000 / ProgressBarFps);
        foreach (QProgressBar *bar, d->bars) {
            if (AnimateProgressBar || (bar->minimum() == 0 && bar->maximum() == 0))
                bar->update();
        }
    }
#endif // QT_NO_PROGRESSBAR
    event->ignore();
}

QT_END_NAMESPACE

#endif // !defined(QT_NO_STYLE_PLASTIQUE) || defined(QT_PLUGIN)
