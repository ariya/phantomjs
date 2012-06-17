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

#include "qcleanlooksstyle.h"
#include "qcleanlooksstyle_p.h"

#if !defined(QT_NO_STYLE_CLEANLOOKS) || defined(QT_PLUGIN)

#include "qwindowsstyle_p.h"
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qpainter.h>
#include <qdir.h>
#include <qhash.h>
#include <qstyleoption.h>
#include <qapplication.h>
#include <qmainwindow.h>
#include <qfont.h>
#include <qgroupbox.h>
#include <qprocess.h>
#include <qpixmapcache.h>
#include <qdialogbuttonbox.h>
#include <qscrollbar.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qsplitter.h>
#include <qprogressbar.h>
#include <qtoolbar.h>
#include <qwizard.h>
#include <qlibrary.h>
#include <private/qstylehelper_p.h>

QT_BEGIN_NAMESPACE

using namespace QStyleHelper;

enum Direction {
    TopDown,
    FromLeft,
    BottomUp,
    FromRight
};

// from windows style
static const int windowsItemFrame        =  2; // menu item frame width
static const int windowsItemHMargin      =  3; // menu item hor text margin
static const int windowsItemVMargin      =  8; // menu item ver text margin
static const int windowsRightBorder      = 15; // right border on windows

/* XPM */
static const char * const dock_widget_close_xpm[] = {
    "11 13 7 1",
    " 	c None",
    ".	c #D5CFCB",
    "+	c #8F8B88",
    "@	c #6C6A67",
    "#	c #ABA6A3",
    "$	c #B5B0AC",
    "%	c #A4A09D",
    "           ",
    " +@@@@@@@+ ",
    "+#       #+",
    "@ $@   @$ @",
    "@ @@@ @@@ @",
    "@  @@@@@  @",
    "@   @@@   @",
    "@  @@@@@  @",
    "@ @@@ @@@ @",
    "@ $@   @$ @",
    "+%       #+",
    " +@@@@@@@+ ",
    "           "};

static const char * const qt_cleanlooks_arrow_down_xpm[] = {
    "11 7 2 1",
    " 	c None",
    "x	c #000000",
    "           ",
    "  x     x  ",
    " xxx   xxx ",
    "  xxxxxxx  ",
    "   xxxxx   ",
    "    xxx    ",
    "     x     "};

static const char * const qt_cleanlooks_arrow_up_xpm[] = {
    "11 7 2 1",
    " 	c None",
    "x	c #000000",
    "     x     ",
    "    xxx    ",
    "   xxxxx   ",
    "  xxxxxxx  ",
    " xxx   xxx ",
    "  x     x  ",
    "           "};

static const char * const dock_widget_restore_xpm[] = {
    "11 13 7 1",
    " 	c None",
    ".	c #D5CFCB",
    "+	c #8F8B88",
    "@	c #6C6A67",
    "#	c #ABA6A3",
    "$	c #B5B0AC",
    "%	c #A4A09D",
    "           ",
    " +@@@@@@@+ ",
    "+#       #+",
    "@   #@@@# @",
    "@   @   @ @",
    "@ #@@@# @ @",
    "@ @   @ @ @",
    "@ @   @@@ @",
    "@ @   @   @",
    "@ #@@@#   @",
    "+%       #+",
    " +@@@@@@@+ ",
    "           "};

static const char * const workspace_minimize[] = {
    "11 13 7 1",
    " 	c None",
    ".	c #D5CFCB",
    "+	c #8F8B88",
    "@	c #6C6A67",
    "#	c #ABA6A3",
    "$	c #B5B0AC",
    "%	c #A4A09D",
    "           ",
    " +@@@@@@@+ ",
    "+#       #+",
    "@         @",
    "@         @",
    "@         @",
    "@ @@@@@@@ @",
    "@ @@@@@@@ @",
    "@         @",
    "@         @",
    "+%       #+",
    " +@@@@@@@+ ",
    "           "};


static const char * const qt_titlebar_context_help[] = {
    "10 10 3 1",
    "  c None",
    "# c #000000",
    "+ c #444444",
    "  +####+  ",
    " ###  ### ",
    " ##    ## ",
    "     +##+ ",
    "    +##   ",
    "    ##    ",
    "    ##    ",
    "          ",
    "    ##    ",
    "    ##    "};

static const char * const qt_cleanlooks_radiobutton[] = {
    "13 13 9 1",
    " 	c None",
    ".	c #ABA094",
    "+	c #B7ADA0",
    "@	c #C4BBB2",
    "#	c #DDD4CD",
    "$	c #E7E1E0",
    "%	c #F4EFED",
    "&	c #FFFAF9",
    "*	c #FCFEFB",
    "   #@...@#   ",
    "  @+@#$$#+@  ",
    " @+$%%***&@@ ",
    "#+$%**&&**&+#",
    "@@$&&******#@",
    ".#**********.",
    ".$&******&*&.",
    ".$*&******&*.",
    "+#********&#@",
    "#+*********+#",
    " @@*******@@ ",
    "  @+#%*%#+@  ",
    "   #@...+#   "};

static const char * const qt_cleanlooks_radiobutton_checked[] = {
    "13 13 20 1",
    " 	c None",
    ".	c #A8ABAE",
    "+	c #596066",
    "@	c #283138",
    "#	c #A9ACAF",
    "$	c #A6A9AB",
    "%	c #6B7378",
    "&	c #8C9296",
    "*	c #A2A6AA",
    "=	c #61696F",
    "-	c #596065",
    ";	c #93989C",
    ">	c #777E83",
    ",	c #60686E",
    "'	c #252D33",
    ")	c #535B62",
    "!	c #21292E",
    "~	c #242B31",
    "{	c #1F262B",
    "]	c #41484E",
    "             ",
    "             ",
    "             ",
    "    .+@+#    ",
    "   $%&*&=#   ",
    "   -&;>,'+   ",
    "   @*>,)!@   ",
    "   +&,)~{+   ",
    "   #='!{]#   ",
    "    #+@+#    ",
    "             ",
    "             ",
    "             "};


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

static const char * const qt_spinbox_button_arrow_down[] = {
    "7 4 2 1",
    "   c None",
    "*  c #BFBFBF",
    "*******",
    " ***** ",
    "  ***  ",
    "   *   "};

static const char * const qt_spinbox_button_arrow_up[] = {
    "7 4 2 1",
    "   c None",
    "*  c #BFBFBF",
    "   *   ",
    "  ***  ",
    " ***** ",
    "*******"};

static const char * const qt_scrollbar_button_left[] = {
    "16 16 6 1",
    "   c None",
    ".  c #BFBFBF",
    "+  c #979797",
    "#  c #FAFAFA",
    "<  c #FAFAFA",
    "*  c #FAFAFA",
    " .++++++++++++++",
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
    " .++++++++++++++"};

static const char * const qt_scrollbar_button_right[] = {
    "16 16 6 1",
    "   c None",
    ".  c #BFBFBF",
    "+  c #979797",
    "#  c #FAFAFA",
    "<  c #FAFAFA",
    "*  c #FAFAFA",
    "++++++++++++++. ",
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
    "++++++++++++++. "};

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
    "++++++++++++++++"};

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

static const char * const qt_cleanlooks_menuitem_checkbox_checked[] = {
    "8 7 6 1",
    " 	g None",
    ".	g #959595",
    "+	g #676767",
    "@	g #454545",
    "#	g #1D1D1D",
    "0	g #101010",
    "      ..",
    "     .+ ",
    "    .+  ",
    "0  .@   ",
    "@#++.   ",
    "  @#    ",
    "   .    "};

static const char * const qt_cleanlooks_checkbox_checked[] = {
    "13 13 3 1",
    " 	c None",
    ".	c #272D33",
    "%	c #666666",

    "             ",
    "          %  ",
    "         %.  ",
    "        %.%  ",
    "       %..   ",
    "  %.% %..    ",
    "  %..%..%    ",
    "   %...%     ",
    "    %..%     ",
    "     %.%     ",
    "      %      ",
    "             ",
    "             "};

static void qt_cleanlooks_draw_gradient(QPainter *painter, const QRect &rect, const QColor &gradientStart,
                                        const QColor &gradientStop, Direction direction = TopDown, QBrush bgBrush = QBrush())
{
        int x = rect.center().x();
        int y = rect.center().y();
        QLinearGradient *gradient;
        switch(direction) {
            case FromLeft:
                gradient = new QLinearGradient(rect.left(), y, rect.right(), y);
                break;
            case FromRight:
                gradient = new QLinearGradient(rect.right(), y, rect.left(), y);
                break;
            case BottomUp:
                gradient = new QLinearGradient(x, rect.bottom(), x, rect.top());
                break;
            case TopDown:
            default:
                gradient = new QLinearGradient(x, rect.top(), x, rect.bottom());
                break;
        }
        if (bgBrush.gradient())
            gradient->setStops(bgBrush.gradient()->stops());
        else {
            gradient->setColorAt(0, gradientStart);
            gradient->setColorAt(1, gradientStop);
        }
        painter->fillRect(rect, *gradient);
        delete gradient;
}

static void qt_cleanlooks_draw_buttongradient(QPainter *painter, const QRect &rect, const QColor &gradientStart,
                                                const QColor &gradientMid, const QColor &gradientStop, Direction direction = TopDown,
                                                QBrush bgBrush = QBrush())
{
        int x = rect.center().x();
        int y = rect.center().y();
        QLinearGradient *gradient;
        bool horizontal = false;
        switch(direction) {
            case FromLeft:
                horizontal = true;
                gradient = new QLinearGradient(rect.left(), y, rect.right(), y);
                break;
            case FromRight:
                horizontal = true;
                gradient = new QLinearGradient(rect.right(), y, rect.left(), y);
                break;
            case BottomUp:
                gradient = new QLinearGradient(x, rect.bottom(), x, rect.top());
                break;
            case TopDown:
            default:
                gradient = new QLinearGradient(x, rect.top(), x, rect.bottom());
                break;
        }
        if (bgBrush.gradient())
            gradient->setStops(bgBrush.gradient()->stops());
        else {
            int size = horizontal ? rect.width() : rect.height() ;
            if (size > 4) {
                float edge = 4.0/(float)size;
                gradient->setColorAt(0, gradientStart);
                gradient->setColorAt(edge, gradientMid.lighter(104));
                gradient->setColorAt(1.0 - edge, gradientMid.darker(100));
                gradient->setColorAt(1.0, gradientStop);
            }
        }
        painter->fillRect(rect, *gradient);
        delete gradient;
}

static void qt_cleanlooks_draw_mdibutton(QPainter *painter, const QStyleOptionTitleBar *option, const QRect &tmp, bool hover, bool sunken)
{
    QColor dark;
    dark.setHsv(option->palette.button().color().hue(),
                qMin(255, (int)(option->palette.button().color().saturation()*1.9)),
                qMin(255, (int)(option->palette.button().color().value()*0.7)));

    QColor highlight = option->palette.highlight().color();

    bool active = (option->titleBarState & QStyle::State_Active);
    QColor titleBarHighlight(255, 255, 255, 60);

    if (sunken)
        painter->fillRect(tmp.adjusted(1, 1, -1, -1), option->palette.highlight().color().darker(120));
    else if (hover)
        painter->fillRect(tmp.adjusted(1, 1, -1, -1), QColor(255, 255, 255, 20));

    QColor mdiButtonGradientStartColor;
    QColor mdiButtonGradientStopColor;

    mdiButtonGradientStartColor = QColor(0, 0, 0, 40);
    mdiButtonGradientStopColor = QColor(255, 255, 255, 60);

    if (sunken)
        titleBarHighlight = highlight.darker(130);

    QLinearGradient gradient(tmp.center().x(), tmp.top(), tmp.center().x(), tmp.bottom());
    gradient.setColorAt(0, mdiButtonGradientStartColor);
    gradient.setColorAt(1, mdiButtonGradientStopColor);
    QColor mdiButtonBorderColor(active ? option->palette.highlight().color().darker(180): dark.darker(110));

    painter->setPen(QPen(mdiButtonBorderColor, 1));
    const QLine lines[4] = {
        QLine(tmp.left() + 2, tmp.top(), tmp.right() - 2, tmp.top()),
        QLine(tmp.left() + 2, tmp.bottom(), tmp.right() - 2, tmp.bottom()),
        QLine(tmp.left(), tmp.top() + 2, tmp.left(), tmp.bottom() - 2),
        QLine(tmp.right(), tmp.top() + 2, tmp.right(), tmp.bottom() - 2)
    };
    painter->drawLines(lines, 4);
    const QPoint points[4] = {
        QPoint(tmp.left() + 1, tmp.top() + 1),
        QPoint(tmp.right() - 1, tmp.top() + 1),
        QPoint(tmp.left() + 1, tmp.bottom() - 1),
        QPoint(tmp.right() - 1, tmp.bottom() - 1)
    };
    painter->drawPoints(points, 4);

    painter->setPen(titleBarHighlight);
    painter->drawLine(tmp.left() + 2, tmp.top() + 1, tmp.right() - 2, tmp.top() + 1);
    painter->drawLine(tmp.left() + 1, tmp.top() + 2, tmp.left() + 1, tmp.bottom() - 2);

    painter->setPen(QPen(gradient, 1));
    painter->drawLine(tmp.right() + 1, tmp.top() + 2, tmp.right() + 1, tmp.bottom() - 2);
    painter->drawPoint(tmp.right() , tmp.top() + 1);

    painter->drawLine(tmp.left() + 2, tmp.bottom() + 1, tmp.right() - 2, tmp.bottom() + 1);
    painter->drawPoint(tmp.left() + 1, tmp.bottom());
    painter->drawPoint(tmp.right() - 1, tmp.bottom());
    painter->drawPoint(tmp.right() , tmp.bottom() - 1);
}

/*!
    \class QCleanlooksStyle
    \brief The QCleanlooksStyle class provides a widget style similar to the
    Clearlooks style available in GNOME.
    \since 4.2

    The Cleanlooks style provides a look and feel for widgets
    that closely resembles the Clearlooks style, introduced by Richard
    Stellingwerff and Daniel Borgmann.

    \sa {Cleanlooks Style Widget Gallery}, QWindowsXPStyle, QMacStyle, QWindowsStyle,
        QCDEStyle, QMotifStyle, QPlastiqueStyle
*/

/*!
    Constructs a QCleanlooksStyle object.
*/
QCleanlooksStyle::QCleanlooksStyle() : QWindowsStyle(*new QCleanlooksStylePrivate)
{
    setObjectName(QLatin1String("CleanLooks"));
}

/*!
    \internal

    Constructs a QCleanlooksStyle object.
*/
QCleanlooksStyle::QCleanlooksStyle(QCleanlooksStylePrivate &dd) : QWindowsStyle(dd)
{
}

/*!
    Destroys the QCleanlooksStyle object.
*/
QCleanlooksStyle::~QCleanlooksStyle()
{
}

/*!
    \fn void QCleanlooksStyle::drawItemText(QPainter *painter, const QRect &rectangle, int alignment, const QPalette &palette,
                                    bool enabled, const QString& text, QPalette::ColorRole textRole) const

    Draws the given \a text in the specified \a rectangle using the
    provided \a painter and \a palette.

    Text is drawn using the painter's pen. If an explicit \a textRole
    is specified, then the text is drawn using the \a palette's color
    for the specified role.  The \a enabled value indicates whether or
    not the item is enabled; when reimplementing, this value should
    influence how the item is drawn.

    The text is aligned and wrapped according to the specified \a
    alignment.

    \sa Qt::Alignment
*/
void QCleanlooksStyle::drawItemText(QPainter *painter, const QRect &rect, int alignment, const QPalette &pal,
                                    bool enabled, const QString& text, QPalette::ColorRole textRole) const
{
    if (text.isEmpty())
        return;

    QPen savedPen = painter->pen();
    if (textRole != QPalette::NoRole) {
        painter->setPen(QPen(pal.brush(textRole), savedPen.widthF()));
    }
    if (!enabled) {
        QPen pen = painter->pen();
        painter->setPen(pen);
    }
    painter->drawText(rect, alignment, text);
    painter->setPen(savedPen);
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

/*!
    \reimp
*/
void QCleanlooksStyle::drawPrimitive(PrimitiveElement elem,
                        const QStyleOption *option,
                        QPainter *painter, const QWidget *widget) const
{
    Q_ASSERT(option);
    QRect rect = option->rect;
    int state = option->state;
    QColor button = option->palette.button().color();
    QColor buttonShadow = option->palette.button().color().darker(110);
    QColor buttonShadowAlpha = buttonShadow;
    buttonShadowAlpha.setAlpha(128);
    QColor darkOutline;
    QColor dark;
    darkOutline.setHsv(button.hue(),
                qMin(255, (int)(button.saturation()*3.0)),
                qMin(255, (int)(button.value()*0.6)));
    dark.setHsv(button.hue(),
                qMin(255, (int)(button.saturation()*1.9)),
                qMin(255, (int)(button.value()*0.7)));
    QColor tabFrameColor = mergedColors(option->palette.background().color(),
                                                dark.lighter(135), 60);

    switch(elem) {
#ifndef QT_NO_TABBAR
    case PE_FrameTabBarBase:
        if (const QStyleOptionTabBarBase *tbb
                = qstyleoption_cast<const QStyleOptionTabBarBase *>(option)) {
            painter->save();
            painter->setPen(QPen(darkOutline.lighter(110), 0));
            switch (tbb->shape) {
            case QTabBar::RoundedNorth: {
                QRegion region(tbb->rect);
                region -= tbb->selectedTabRect;
                painter->drawLine(tbb->rect.topLeft(), tbb->rect.topRight());
                painter->setClipRegion(region);
                painter->setPen(option->palette.light().color());
                painter->drawLine(tbb->rect.topLeft() + QPoint(0, 1),
                                  tbb->rect.topRight()  + QPoint(0, 1));
            }
                break;
            case QTabBar::RoundedWest:
                painter->drawLine(tbb->rect.left(), tbb->rect.top(), tbb->rect.left(), tbb->rect.bottom());
                break;
            case QTabBar::RoundedSouth:
                painter->drawLine(tbb->rect.left(), tbb->rect.bottom(),
                            tbb->rect.right(), tbb->rect.bottom());
                break;
            case QTabBar::RoundedEast:
                painter->drawLine(tbb->rect.topRight(), tbb->rect.bottomRight());
                break;
            case QTabBar::TriangularNorth:
            case QTabBar::TriangularEast:
            case QTabBar::TriangularWest:
            case QTabBar::TriangularSouth:
                painter->restore();
                QWindowsStyle::drawPrimitive(elem, option, painter, widget);
                return;
            }
            painter->restore();
        }
        return;
#endif // QT_NO_TABBAR
    case PE_IndicatorViewItemCheck:
        {
            QStyleOptionButton button;
            button.QStyleOption::operator=(*option);
            button.state &= ~State_MouseOver;
            proxy()->drawPrimitive(PE_IndicatorCheckBox, &button, painter, widget);
        }
        return;
    case PE_IndicatorHeaderArrow:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            QRect r = header->rect;
            QImage arrow;
            if (header->sortIndicator & QStyleOptionHeader::SortUp)
                arrow = QImage(qt_cleanlooks_arrow_up_xpm);
            else if (header->sortIndicator & QStyleOptionHeader::SortDown)
                arrow = QImage(qt_cleanlooks_arrow_down_xpm);
            if (!arrow.isNull()) {
                r.setSize(arrow.size());
                r.moveCenter(header->rect.center());
                arrow.setColor(1, header->palette.foreground().color().rgba());
                painter->drawImage(r, arrow);
            }
        }
        break;
    case PE_IndicatorButtonDropDown:
        proxy()->drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
        break;
    case PE_IndicatorToolBarSeparator:
        {
            QRect rect = option->rect;
            const int margin = 6;
            if (option->state & State_Horizontal) {
                const int offset = rect.width()/2;
                painter->setPen(QPen(option->palette.background().color().darker(110)));
                painter->drawLine(rect.bottomLeft().x() + offset,
                            rect.bottomLeft().y() - margin,
                            rect.topLeft().x() + offset,
                            rect.topLeft().y() + margin);
                painter->setPen(QPen(option->palette.background().color().lighter(110)));
                painter->drawLine(rect.bottomLeft().x() + offset + 1,
                            rect.bottomLeft().y() - margin,
                            rect.topLeft().x() + offset + 1,
                            rect.topLeft().y() + margin);
            } else { //Draw vertical separator
                const int offset = rect.height()/2;
                painter->setPen(QPen(option->palette.background().color().darker(110)));
                painter->drawLine(rect.topLeft().x() + margin ,
                            rect.topLeft().y() + offset,
                            rect.topRight().x() - margin,
                            rect.topRight().y() + offset);
                painter->setPen(QPen(option->palette.background().color().lighter(110)));
                painter->drawLine(rect.topLeft().x() + margin ,
                            rect.topLeft().y() + offset + 1,
                            rect.topRight().x() - margin,
                            rect.topRight().y() + offset + 1);
            }
        }
        break;
    case PE_Frame:
        painter->save();
        painter->setPen(dark.lighter(108));
        painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
        painter->restore();
        break;
    case PE_FrameMenu:
        painter->save();
        {
            painter->setPen(QPen(darkOutline, 1));
            painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
            QColor frameLight = option->palette.background().color().lighter(160);
            QColor frameShadow = option->palette.background().color().darker(110);

            //paint beveleffect
            QRect frame = option->rect.adjusted(1, 1, -1, -1);
            painter->setPen(frameLight);
            painter->drawLine(frame.topLeft(), frame.bottomLeft());
            painter->drawLine(frame.topLeft(), frame.topRight());

            painter->setPen(frameShadow);
            painter->drawLine(frame.topRight(), frame.bottomRight());
            painter->drawLine(frame.bottomLeft(), frame.bottomRight());
        }
        painter->restore();
        break;
    case PE_FrameDockWidget:

        painter->save();
        {
            QColor softshadow = option->palette.background().color().darker(120);

            QRect rect= option->rect;
            painter->setPen(softshadow);
            painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
            painter->setPen(QPen(option->palette.light(), 0));
            painter->drawLine(QPoint(rect.left() + 1, rect.top() + 1), QPoint(rect.left() + 1, rect.bottom() - 1));
            painter->setPen(QPen(option->palette.background().color().darker(120), 0));
            painter->drawLine(QPoint(rect.left() + 1, rect.bottom() - 1), QPoint(rect.right() - 2, rect.bottom() - 1));
            painter->drawLine(QPoint(rect.right() - 1, rect.top() + 1), QPoint(rect.right() - 1, rect.bottom() - 1));

        }
        painter->restore();
        break;
    case PE_PanelButtonTool:
        painter->save();
        if ((option->state & State_Enabled || option->state & State_On) || !(option->state & State_AutoRaise)) {
            QPen oldPen = painter->pen();

            if (widget && widget->inherits("QDockWidgetTitleButton")) {
                   if (option->state & State_MouseOver)
                       proxy()->drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
            } else {
                proxy()->drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
            }
        }
        painter->restore();
        break;
    case PE_IndicatorDockWidgetResizeHandle:
        {
            QStyleOption dockWidgetHandle = *option;
            bool horizontal = option->state & State_Horizontal;
            if (horizontal)
                dockWidgetHandle.state &= ~State_Horizontal;
            else
                dockWidgetHandle.state |= State_Horizontal;
            proxy()->drawControl(CE_Splitter, &dockWidgetHandle, painter, widget);
        }
        break;
    case PE_FrameWindow:
        painter->save();
        {
            QRect rect= option->rect;
            painter->setPen(QPen(dark.darker(150), 0));
            painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
            painter->setPen(QPen(option->palette.light(), 0));
            painter->drawLine(QPoint(rect.left() + 1, rect.top() + 1),
                              QPoint(rect.left() + 1, rect.bottom() - 1));
            painter->setPen(QPen(option->palette.background().color().darker(120), 0));
            painter->drawLine(QPoint(rect.left() + 1, rect.bottom() - 1),
                              QPoint(rect.right() - 2, rect.bottom() - 1));
            painter->drawLine(QPoint(rect.right() - 1, rect.top() + 1),
                              QPoint(rect.right() - 1, rect.bottom() - 1));
        }
        painter->restore();
        break;
#ifndef QT_NO_LINEEDIT
    case PE_FrameLineEdit:
        // fall through
#endif // QT_NO_LINEEDIT
#ifdef QT3_SUPPORT
        if (widget && widget->inherits("Q3ToolBar")) {
            proxy()->drawPrimitive(PE_Q3Separator, option, painter, widget);
            break;
        }
#endif
        {
            QPen oldPen = painter->pen();
            if (option->state & State_Enabled) {
                painter->setPen(QPen(option->palette.background(), 0));
                painter->drawRect(rect.adjusted(0, 0, 0, 0));
                painter->drawRect(rect.adjusted(1, 1, -1, -1));
            } else {
                painter->fillRect(rect, option->palette.background());
            }
            QRect r = rect.adjusted(0, 1, 0, -1);
            painter->setPen(buttonShadowAlpha);
            painter->drawLine(QPoint(r.left() + 2, r.top() - 1), QPoint(r.right() - 2, r.top() - 1));
            const QPoint points[8] = {
                QPoint(r.right() - 1, r.top()),
                QPoint(r.right(), r.top() + 1),
                QPoint(r.right() - 1, r.bottom()),
                QPoint(r.right(), r.bottom() - 1),
                QPoint(r.left() + 1, r.top() ),
                QPoint(r.left(), r.top() + 1),
                QPoint(r.left() + 1, r.bottom() ),
                QPoint(r.left(), r.bottom() - 1)
            };
            painter->drawPoints(points, 8);
            painter->setPen(QPen(option->palette.background().color(), 1));
            painter->drawLine(QPoint(r.left() + 2, r.top() + 1), QPoint(r.right() - 2, r.top() + 1));

            if (option->state & State_HasFocus) {
                QColor darkoutline = option->palette.highlight().color().darker(150);
                QColor innerline = mergedColors(option->palette.highlight().color(), Qt::white);
                painter->setPen(QPen(innerline, 0));
                painter->drawRect(rect.adjusted(1, 2, -2, -3));
                painter->setPen(QPen(darkoutline, 0));
            }
            else {
                QColor highlight = Qt::white;
                highlight.setAlpha(130);
                painter->setPen(option->palette.base().color().darker(120));
                painter->drawLine(QPoint(r.left() + 1, r.top() + 1),
                                  QPoint(r.right() - 1, r.top() + 1));
                painter->drawLine(QPoint(r.left() + 1, r.top() + 1),
                                  QPoint(r.left() + 1, r.bottom() - 1));
                painter->setPen(option->palette.base().color());
                painter->drawLine(QPoint(r.right() - 1, r.top() + 1),
                                  QPoint(r.right() - 1, r.bottom() - 1));
                painter->setPen(highlight);
                painter->drawLine(QPoint(r.left() + 1, r.bottom() + 1),
                                  QPoint(r.right() - 1, r.bottom() + 1));
                painter->drawPoint(QPoint(r.left(), r.bottom()));
                painter->drawPoint(QPoint(r.right(), r.bottom() ));
                painter->setPen(QPen(darkOutline.lighter(115), 1));
            }
            painter->drawLine(QPoint(r.left(), r.top() + 2), QPoint(r.left(), r.bottom() - 2));
            painter->drawLine(QPoint(r.right(), r.top() + 2), QPoint(r.right(), r.bottom() - 2));
            painter->drawLine(QPoint(r.left() + 2, r.bottom()), QPoint(r.right() - 2, r.bottom()));
            const QPoint points2[4] = {
                QPoint(r.right() - 1, r.bottom() - 1),
                QPoint(r.right() - 1, r.top() + 1),
                QPoint(r.left() + 1, r.bottom() - 1),
                QPoint(r.left() + 1, r.top() + 1)
            };
            painter->drawPoints(points2, 4);
            painter->drawLine(QPoint(r.left() + 2, r.top()), QPoint(r.right() - 2, r.top()));
            painter->setPen(oldPen);
        }
        break;
    case PE_IndicatorCheckBox:
        painter->save();
        if (const QStyleOptionButton *checkbox = qstyleoption_cast<const QStyleOptionButton*>(option)) {
            QRect checkRect;
            checkRect.setX(rect.left() );
            checkRect.setY(rect.top() );
            checkRect.setWidth(rect.width() - 1);
            checkRect.setHeight(rect.height() - 1);
            if (state & State_Sunken)
                painter->setBrush(dark.lighter(130));
            else
                painter->setBrush(option->palette.base());
            painter->setPen(QPen(dark.lighter(110), 0));
            painter->drawRect(checkRect);
            if (checkbox->state & (State_On | State_Sunken  | State_NoChange)) {
                QImage image(qt_cleanlooks_checkbox_checked);
                QColor fillColor = option->palette.text().color();
                image.setColor(1, fillColor.rgba()); 
                fillColor.setAlpha(100);
                image.setColor(2, fillColor.rgba()); 
                painter->drawImage(rect, image);
                if (checkbox->state & State_NoChange) {
                    QColor bgc = option->palette.background().color();
                    bgc.setAlpha(127);
                    painter->fillRect(checkRect.adjusted(1, 1, -1, -1), bgc);
                }
            }
        }
        painter->restore();
        break;
    case PE_IndicatorRadioButton:
        painter->save();
        {
            painter->setRenderHint(QPainter::SmoothPixmapTransform);
            QRect checkRect = rect.adjusted(0, 0, 0, 0);
            if (state & (State_On )) {
                painter->drawImage(rect, QImage(qt_cleanlooks_radiobutton));
                painter->drawImage(checkRect, QImage(qt_cleanlooks_radiobutton_checked));
            }
            else if (state & State_Sunken) {
                painter->drawImage(rect, QImage(qt_cleanlooks_radiobutton));
                QColor bgc = buttonShadow;
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setBrush(bgc);
                painter->setPen(Qt::NoPen);
                painter->drawEllipse(rect.adjusted(1, 1, -1, -1));                }
            else {
                painter->drawImage(rect, QImage(qt_cleanlooks_radiobutton));
            }
        }
        painter->restore();
    break;
    case PE_IndicatorToolBarHandle:
        painter->save();
        if (option->state & State_Horizontal) {
            for (int i = rect.height()/5; i <= 4*(rect.height()/5) ; ++i) {
                int y = rect.topLeft().y() + i + 1;
                int x1 = rect.topLeft().x() + 3;
                int x2 = rect.topRight().x() - 2;

                if (i % 2 == 0)
                    painter->setPen(QPen(option->palette.light(), 0));
                else
                    painter->setPen(QPen(dark.lighter(110), 0));
                painter->drawLine(x1, y, x2, y);
            }
        }
        else { //vertical toolbar
            for (int i = rect.width()/5; i <= 4*(rect.width()/5) ; ++i) {
                int x = rect.topLeft().x() + i + 1;
                int y1 = rect.topLeft().y() + 3;
                int y2 = rect.topLeft().y() + 5;

                if (i % 2 == 0)
                    painter->setPen(QPen(option->palette.light(), 0));
                else
                    painter->setPen(QPen(dark.lighter(110), 0));
                painter->drawLine(x, y1, x, y2);
            }
        }
        painter->restore();
        break;
    case PE_FrameDefaultButton:
        case PE_FrameFocusRect:
        if (const QStyleOptionFocusRect *focusFrame = qstyleoption_cast<const QStyleOptionFocusRect *>(option)) {
            if (!(focusFrame->state & State_KeyboardFocusChange))
                return;
            QRect rect = focusFrame->rect;
            painter->save();
            painter->setBackgroundMode(Qt::TransparentMode);
            painter->setBrush(QBrush(dark.darker(120), Qt::Dense4Pattern));
            painter->setBrushOrigin(rect.topLeft());
            painter->setPen(Qt::NoPen);
            const QRect rects[4] = {
                QRect(rect.left(), rect.top(), rect.width(), 1),    // Top
                QRect(rect.left(), rect.bottom(), rect.width(), 1), // Bottom
                QRect(rect.left(), rect.top(), 1, rect.height()),   // Left
                QRect(rect.right(), rect.top(), 1, rect.height())   // Right
            };
            painter->drawRects(rects, 4);
            painter->restore();
        }
        break;
    case PE_PanelButtonCommand:
        {
            bool isDefault = false;
            bool isFlat = false;
            bool isDown = (option->state & State_Sunken) || (option->state & State_On);
            QPen oldPen = painter->pen();
            QBrush oldBrush = painter->brush();
            QRect r;

            if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton*>(option)) {
                isDefault = (button->features & QStyleOptionButton::DefaultButton) && (button->state & State_Enabled);
                isFlat = (button->features & QStyleOptionButton::Flat);
            }

            if (isFlat && !isDown) {
                if (isDefault) {
                    r = option->rect.adjusted(0, 1, 0, -1);
                    painter->setPen(QPen(Qt::black, 0));
                    const QLine lines[4] = {
                        QLine(QPoint(r.left() + 2, r.top()),
                              QPoint(r.right() - 2, r.top())),
                        QLine(QPoint(r.left(), r.top() + 2),
                              QPoint(r.left(), r.bottom() - 2)),
                        QLine(QPoint(r.right(), r.top() + 2),
                              QPoint(r.right(), r.bottom() - 2)),
                        QLine(QPoint(r.left() + 2, r.bottom()),
                              QPoint(r.right() - 2, r.bottom()))
                    };
                    painter->drawLines(lines, 4);
                    const QPoint points[4] = {
                        QPoint(r.right() - 1, r.bottom() - 1),
                        QPoint(r.right() - 1, r.top() + 1),
                        QPoint(r.left() + 1, r.bottom() - 1),
                        QPoint(r.left() + 1, r.top() + 1)
                    };
                    painter->drawPoints(points, 4);
                    painter->setPen(oldPen);
                }
                return;
            }

            BEGIN_STYLE_PIXMAPCACHE(QString::fromLatin1("pushbutton-%1").arg(isDefault))
            r = rect.adjusted(0, 1, 0, -1);

            bool isEnabled = (option->state & State_Enabled);

            QColor highlightedGradientStartColor = option->palette.button().color().lighter(107);
            QColor highlightedGradientMidColor = option->palette.button().color().lighter(105);
            QColor highlightedGradientStopColor = buttonShadow.lighter(107);
            QColor gradientStartColor = option->palette.button().color().lighter(108);

            QColor buttonColor = option->palette.button().color();
            QColor gradientMidColor = option->palette.button().color();
            QColor gradientStopColor;
            gradientStopColor.setHsv(buttonColor.hue(),
                                     qMin(255, (int)(buttonColor.saturation()*1.9)),
                                     qMin(255, (int)(buttonColor.value()*0.96)));

            QRect gradRect = rect.adjusted(1, 2, -1, -2);
            // gradient fill
            QRect innerBorder = r.adjusted(1, 1, -1, 0);

            if (isDown) {
                QBrush fillColor = gradientStopColor.darker(110);
                if (option->palette.button().gradient())
                    fillColor = option->palette.button();
                p->fillRect(gradRect, fillColor);
                p->setPen(gradientStopColor.darker(125));
                p->drawLine(innerBorder.topLeft(), innerBorder.topRight());
                p->drawLine(innerBorder.topLeft(), innerBorder.bottomLeft());
            } else {
                if (isEnabled && option->state & State_MouseOver ) {
                    qt_cleanlooks_draw_buttongradient(p, gradRect,
                                                highlightedGradientStartColor,
                                                highlightedGradientMidColor,
                                                highlightedGradientStopColor, TopDown, option->palette.button());
                } else {
                    qt_cleanlooks_draw_buttongradient(p, gradRect,
                                                gradientStartColor,
                                                gradientMidColor,
                                                gradientStopColor, TopDown, option->palette.button());
                }
            }

            bool hasFocus = option->state & State_HasFocus;

            if (!isEnabled)
                p->setPen(QPen(dark.lighter(115)));
            else if (isDefault)
                p->setPen(QPen(Qt::black, 1));
            else
                p->setPen(QPen(darkOutline, 1));

            p->drawLine(QPoint(r.left(), r.top() + 2),
                              QPoint(r.left(), r.bottom() - 2));
            p->drawLine(QPoint(r.right(), r.top() + 2),
                              QPoint(r.right(), r.bottom() - 2));
            p->drawLine(QPoint(r.left() + 2, r.bottom()),
                              QPoint(r.right() - 2, r.bottom()));
            const QPoint points[4] = {
                QPoint(r.right() - 1, r.bottom() - 1),
                QPoint(r.right() - 1, r.top() + 1),
                QPoint(r.left() + 1, r.bottom() - 1),
                QPoint(r.left() + 1, r.top() + 1)
            };
            p->drawPoints(points, 4);

            if (!isDefault && !hasFocus && isEnabled)
                p->setPen(QPen(darkOutline.darker(110), 0));

            p->drawLine(QPoint(r.left() + 2, r.top()),
                              QPoint(r.right() - 2, r.top()));

            QColor highlight = Qt::white;
            highlight.setAlpha(110);
            p->setPen(highlight);
            p->drawLine(QPoint(r.left() + 1, r.top() + 2),
                              QPoint(r.left() + 1, r.bottom() - 2));
            p->drawLine(QPoint(r.left() + 3, r.bottom() + 1),
                              QPoint(r.right() - 3, r.bottom() + 1));

            QColor topShadow = darkOutline;
            topShadow.setAlpha(60);

            p->setPen(topShadow);
            const QPoint points2[8] = {
                QPoint(r.right(), r.top() + 1),
                QPoint(r.right() - 1, r.top() ),
                QPoint(r.right(), r.bottom() - 1),
                QPoint(r.right() - 1, r.bottom() ),
                QPoint(r.left() + 1, r.bottom()),
                QPoint(r.left(), r.bottom() - 1),
                QPoint(r.left() + 1, r.top()),
                QPoint(r.left(), r.top() + 1)
            };
            p->drawPoints(points2, 8);

            topShadow.setAlpha(30);
            p->setPen(topShadow);

            p->drawLine(QPoint(r.right() - 1, r.top() + 2),
                              QPoint(r.right() - 1, r.bottom() - 2));
            p->drawLine(QPoint(r.left() + 2, r.top() - 1),
                              QPoint(r.right() - 2, r.top() - 1));

            if (isDefault) {
                r.adjust(-1, -1, 1, 1);
                p->setPen(buttonShadowAlpha.darker(120));
                const QLine lines[4] = {
                    QLine(r.topLeft() + QPoint(3, 0), r.topRight() - QPoint(3, 0)),
                    QLine(r.bottomLeft() + QPoint(3, 0), r.bottomRight() - QPoint(3, 0)),
                    QLine(r.topLeft() + QPoint(0, 3), r.bottomLeft() - QPoint(0, 3)),
                    QLine(r.topRight() + QPoint(0, 3), r.bottomRight() - QPoint(0, 3))
                };
                p->drawLines(lines, 4);
                const QPoint points3[8] = {
                    r.topRight() + QPoint(-2, 1),
                    r.topRight() + QPoint(-1, 2),
                    r.bottomRight() + QPoint(-1, -2),
                    r.bottomRight() + QPoint(-2, -1),
                    r.topLeft() + QPoint(1, 2),
                    r.topLeft() + QPoint(2, 1),
                    r.bottomLeft() + QPoint(1, -2),
                    r.bottomLeft() + QPoint(2, -1)
                };
                p->drawPoints(points3, 8);
            }
            painter->setPen(oldPen);
            painter->setBrush(oldBrush);
            END_STYLE_PIXMAPCACHE
        }
        break;
#ifndef QT_NO_TABBAR
        case PE_FrameTabWidget:
            painter->save();
        {
            painter->fillRect(option->rect, tabFrameColor);
        }
#ifndef QT_NO_TABWIDGET
        if (const QStyleOptionTabWidgetFrame *twf = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option)) {
            QColor borderColor = darkOutline.lighter(110);
            QColor alphaCornerColor = mergedColors(borderColor, option->palette.background().color());

            int borderThickness = proxy()->pixelMetric(PM_TabBarBaseOverlap, twf, widget);
            bool reverse = (twf->direction == Qt::RightToLeft);
            QRect tabBarRect;

            switch (twf->shape) {
            case QTabBar::RoundedNorth:
                if (reverse) {
                    tabBarRect = QRect(twf->rect.right() - twf->leftCornerWidgetSize.width()
                                       - twf->tabBarSize.width() + 1,
                                       twf->rect.top(),
                                       twf->tabBarSize.width(), borderThickness);
                } else {
                    tabBarRect = QRect(twf->rect.left() + twf->leftCornerWidgetSize.width(),
                                       twf->rect.top(),
                                       twf->tabBarSize.width(), borderThickness);
                }
                break ;
            case QTabBar::RoundedWest:
                tabBarRect = QRect(twf->rect.left(),
                                   twf->rect.top() + twf->leftCornerWidgetSize.height(),
                                   borderThickness,
                                   twf->tabBarSize.height());
                tabBarRect = tabBarRect; //adjust
                break ;
            case QTabBar::RoundedEast:
                tabBarRect = QRect(twf->rect.right() - borderThickness + 1,
                                   twf->rect.top()  + twf->leftCornerWidgetSize.height(),
                                   0,
                                   twf->tabBarSize.height());
                break ;
            case QTabBar::RoundedSouth:
                if (reverse) {
                    tabBarRect = QRect(twf->rect.right() - twf->leftCornerWidgetSize.width() - twf->tabBarSize.width() + 1,
                                       twf->rect.bottom() + 1,
                                       twf->tabBarSize.width(),
                                       borderThickness);
                } else {
                    tabBarRect = QRect(twf->rect.left() + twf->leftCornerWidgetSize.width(),
                                       twf->rect.bottom() + 1,
                                       twf->tabBarSize.width(),
                                       borderThickness);
                }
                break;
            default:
                break;
            }

            QRegion region(twf->rect);
            region -= tabBarRect;
            painter->setClipRegion(region);

            // Outer border
            QLine leftLine = QLine(twf->rect.topLeft() + QPoint(0, 2), twf->rect.bottomLeft() - QPoint(0, 2));
            QLine rightLine = QLine(twf->rect.topRight(), twf->rect.bottomRight() - QPoint(0, 2));
            QLine bottomLine = QLine(twf->rect.bottomLeft() + QPoint(2, 0), twf->rect.bottomRight() - QPoint(2, 0));
            QLine topLine = QLine(twf->rect.topLeft(), twf->rect.topRight());

            painter->setPen(borderColor);
            painter->drawLine(topLine);

            // Inner border
            QLine innerLeftLine = QLine(leftLine.p1() + QPoint(1, 0), leftLine.p2() + QPoint(1, 0));
            QLine innerRightLine = QLine(rightLine.p1() - QPoint(1, -1), rightLine.p2() - QPoint(1, 0));
            QLine innerBottomLine = QLine(bottomLine.p1() - QPoint(0, 1), bottomLine.p2() - QPoint(0, 1));
            QLine innerTopLine = QLine(topLine.p1() + QPoint(0, 1), topLine.p2() + QPoint(-1, 1));

            // Rounded Corner
            QPoint leftBottomOuterCorner = QPoint(innerLeftLine.p2() + QPoint(0, 1));
            QPoint leftBottomInnerCorner1 = QPoint(leftLine.p2() + QPoint(0, 1));
            QPoint leftBottomInnerCorner2 = QPoint(bottomLine.p1() - QPoint(1, 0));
            QPoint rightBottomOuterCorner = QPoint(innerRightLine.p2() + QPoint(0, 1));
            QPoint rightBottomInnerCorner1 = QPoint(rightLine.p2() + QPoint(0, 1));
            QPoint rightBottomInnerCorner2 = QPoint(bottomLine.p2() + QPoint(1, 0));
            QPoint leftTopOuterCorner = QPoint(innerLeftLine.p1() - QPoint(0, 1));
            QPoint leftTopInnerCorner1 = QPoint(leftLine.p1() - QPoint(0, 1));
            QPoint leftTopInnerCorner2 = QPoint(topLine.p1() - QPoint(1, 0));

            painter->setPen(borderColor);
            painter->drawLine(leftLine);
            painter->drawLine(rightLine);
            painter->drawLine(bottomLine);
            painter->drawPoint(leftBottomOuterCorner);
            painter->drawPoint(rightBottomOuterCorner);
            painter->drawPoint(leftTopOuterCorner);

            painter->setPen(option->palette.light().color());
            painter->drawLine(innerLeftLine);
            painter->drawLine(innerTopLine);

            painter->setPen(buttonShadowAlpha);
            painter->drawLine(innerRightLine);
            painter->drawLine(innerBottomLine);

            painter->setPen(alphaCornerColor);
            const QPoint points[6] = {
                leftBottomInnerCorner1,
                leftBottomInnerCorner2,
                rightBottomInnerCorner1,
                rightBottomInnerCorner2,
                leftTopInnerCorner1,
                leftTopInnerCorner2
            };
            painter->drawPoints(points, 6);
        }
#endif // QT_NO_TABWIDGET
    painter->restore();
    break ;

    case PE_FrameStatusBarItem:
        break;
    case PE_IndicatorTabClose:
        {
            Q_D(const QCleanlooksStyle);
            if (d->tabBarcloseButtonIcon.isNull())
                d->tabBarcloseButtonIcon = standardIcon(SP_DialogCloseButton, option, widget);
            if ((option->state & State_Enabled) && (option->state & State_MouseOver))
                proxy()->drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
            QPixmap pixmap = d->tabBarcloseButtonIcon.pixmap(QSize(16, 16), QIcon::Normal, QIcon::On);
            proxy()->drawItemPixmap(painter, option->rect, Qt::AlignCenter, pixmap);
        }
        break;

#endif // QT_NO_TABBAR
    default:
        QWindowsStyle::drawPrimitive(elem, option, painter, widget);
        break;
    }
}

/*!
  \reimp
*/
void QCleanlooksStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                                   const QWidget *widget) const
{
    QColor button = option->palette.button().color();
    QColor dark;
    dark.setHsv(button.hue(),
                qMin(255, (int)(button.saturation()*1.9)),
                qMin(255, (int)(button.value()*0.7)));
    QColor darkOutline;
    darkOutline.setHsv(button.hue(),
                qMin(255, (int)(button.saturation()*2.0)),
                qMin(255, (int)(button.value()*0.6)));
    QRect rect = option->rect;
    QColor shadow = mergedColors(option->palette.background().color().darker(120),
                                 dark.lighter(130), 60);
    QColor tabFrameColor = mergedColors(option->palette.background().color(),
                                                dark.lighter(135), 60);

    QColor highlight = option->palette.highlight().color();

    switch(element) {
     case CE_RadioButton: //fall through
     case CE_CheckBox:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            bool hover = (btn->state & State_MouseOver && btn->state & State_Enabled);
            if (hover)
                painter->fillRect(rect, btn->palette.background().color().lighter(104));
            QStyleOptionButton copy = *btn;
            copy.rect.adjust(2, 0, -2, 0);
            QWindowsStyle::drawControl(element, &copy, painter, widget);
        }
        break;
    case CE_Splitter:
        painter->save();
        {
            // hover appearance
            QBrush fillColor = option->palette.background().color();
            if (option->state & State_MouseOver && option->state & State_Enabled)
                fillColor = fillColor.color().lighter(106);

            painter->fillRect(option->rect, fillColor);

            QColor grooveColor = mergedColors(dark.lighter(110), option->palette.button().color(),40);
            QColor gripShadow = grooveColor.darker(110);
            QPalette palette = option->palette;
            bool vertical = !(option->state & State_Horizontal);
            QRect scrollBarSlider = option->rect;
            int gripMargin = 4;
            //draw grips
            if (vertical) {
                for( int i = -20; i< 20 ; i += 2) {
                    painter->setPen(QPen(gripShadow, 1));
                    painter->drawLine(
                        QPoint(scrollBarSlider.center().x() + i ,
                               scrollBarSlider.top() + gripMargin),
                        QPoint(scrollBarSlider.center().x() + i,
                               scrollBarSlider.bottom() - gripMargin));
                    painter->setPen(QPen(palette.light(), 1));
                    painter->drawLine(
                        QPoint(scrollBarSlider.center().x() + i + 1,
                               scrollBarSlider.top() + gripMargin  ),
                        QPoint(scrollBarSlider.center().x() + i + 1,
                               scrollBarSlider.bottom() - gripMargin));
                }
            } else {
                for (int i = -20; i < 20 ; i += 2) {
                    painter->setPen(QPen(gripShadow, 1));
                    painter->drawLine(
                        QPoint(scrollBarSlider.left() + gripMargin ,
                               scrollBarSlider.center().y()+ i),
                        QPoint(scrollBarSlider.right() - gripMargin,
                               scrollBarSlider.center().y()+ i));
                    painter->setPen(QPen(palette.light(), 1));
                    painter->drawLine(
                        QPoint(scrollBarSlider.left() + gripMargin,
                               scrollBarSlider.center().y() + 1 + i),
                        QPoint(scrollBarSlider.right() - gripMargin,
                               scrollBarSlider.center().y() + 1 + i));

                }
            }
        }
        painter->restore();
        break;
#ifndef QT_NO_SIZEGRIP
    case CE_SizeGrip:
        painter->save();
        {
            int x, y, w, h;
            option->rect.getRect(&x, &y, &w, &h);
            int sw = qMin(h, w);
            if (h > w)
                painter->translate(0, h - w);
            else
                painter->translate(w - h, 0);

            int sx = x;
            int sy = y;
            int s = 4;
            if (option->direction == Qt::RightToLeft) {
                sx = x + sw;
                for (int i = 0; i < 4; ++i) {
                    painter->setPen(QPen(option->palette.light().color(), 1));
                    painter->drawLine(x, sy - 1 , sx + 1, sw);
                    painter->setPen(QPen(dark.lighter(120), 1));
                    painter->drawLine(x, sy, sx, sw);
                    sx -= s;
                    sy += s;
                }
            } else {
                for (int i = 0; i < 4; ++i) {
                    painter->setPen(QPen(option->palette.light().color(), 1));
                    painter->drawLine(sx - 1, sw, sw, sy - 1);
                    painter->setPen(QPen(dark.lighter(120), 1));
                    painter->drawLine(sx, sw, sw, sy);
                    sx += s;
                    sy += s;
                }
            }
        }
        painter->restore();
        break;
#endif // QT_NO_SIZEGRIP
#ifndef QT_NO_TOOLBAR
    case CE_ToolBar:
        painter->save();
        if (const QStyleOptionToolBar *toolbar = qstyleoption_cast<const QStyleOptionToolBar *>(option)) {
            QRect rect = option->rect;

            bool paintLeftBorder = true;
            bool paintRightBorder = true;
            bool paintBottomBorder = true;

            switch (toolbar->toolBarArea) {
            case Qt::BottomToolBarArea:
                switch(toolbar->positionOfLine) {
                case QStyleOptionToolBar::Beginning:
                case QStyleOptionToolBar::OnlyOne:
                    paintBottomBorder = false;
                default:
                    break;
                }
            case Qt::TopToolBarArea:
                switch (toolbar->positionWithinLine) {
                case QStyleOptionToolBar::Beginning:
                    paintLeftBorder = false;
                    break;
                case QStyleOptionToolBar::End:
                    paintRightBorder = false;
                    break;
                case QStyleOptionToolBar::OnlyOne:
                    paintRightBorder = false;
                    paintLeftBorder = false;
                default:
                    break;
                }
                if (toolbar->direction == Qt::RightToLeft) { //reverse layout changes the order of Beginning/end
                    bool tmp = paintLeftBorder;
                    paintRightBorder=paintLeftBorder;
                    paintLeftBorder=tmp;
                }
                break;
            case Qt::RightToolBarArea:
                switch (toolbar->positionOfLine) {
                case QStyleOptionToolBar::Beginning:
                case QStyleOptionToolBar::OnlyOne:
                    paintRightBorder = false;
                    break;
                default:
                    break;
                }
                break;
            case Qt::LeftToolBarArea:
                switch (toolbar->positionOfLine) {
                case QStyleOptionToolBar::Beginning:
                case QStyleOptionToolBar::OnlyOne:
                    paintLeftBorder = false;
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }

            QColor light = option->palette.background().color().lighter(110);

            //draw top border
            painter->setPen(QPen(light));
            painter->drawLine(rect.topLeft().x(),
                        rect.topLeft().y(),
                        rect.topRight().x(),
                        rect.topRight().y());

            if (paintLeftBorder) {
                painter->setPen(QPen(light));
                painter->drawLine(rect.topLeft().x(),
                            rect.topLeft().y(),
                            rect.bottomLeft().x(),
                            rect.bottomLeft().y());
            }

            if (paintRightBorder) {
                painter->setPen(QPen(shadow));
                painter->drawLine(rect.topRight().x(),
                            rect.topRight().y(),
                            rect.bottomRight().x(),
                            rect.bottomRight().y());
            }

            if (paintBottomBorder) {
                painter->setPen(QPen(shadow));
                painter->drawLine(rect.bottomLeft().x(),
                            rect.bottomLeft().y(),
                            rect.bottomRight().x(),
                            rect.bottomRight().y());
            }
        }
        painter->restore();
        break;
#endif // QT_NO_TOOLBAR
#ifndef QT_NO_DOCKWIDGET
    case CE_DockWidgetTitle:
        painter->save();
        if (const QStyleOptionDockWidget *dwOpt = qstyleoption_cast<const QStyleOptionDockWidget *>(option)) {
            const QStyleOptionDockWidgetV2 *v2
                = qstyleoption_cast<const QStyleOptionDockWidgetV2*>(dwOpt);
            bool verticalTitleBar = v2 == 0 ? false : v2->verticalTitleBar;

            QRect rect = dwOpt->rect;
            QRect titleRect = subElementRect(SE_DockWidgetTitleBarText, option, widget);
            QRect r = rect.adjusted(0, 0, -1, 0);
            if (verticalTitleBar)
                r.adjust(0, 0, 0, -1);
            painter->setPen(option->palette.light().color());
            painter->drawRect(r.adjusted(1, 1, 1, 1));
            painter->setPen(shadow);
            painter->drawRect(r);

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

            if (!dwOpt->title.isEmpty()) {
                QString titleText
                    = painter->fontMetrics().elidedText(dwOpt->title,
                                            Qt::ElideRight, titleRect.width());
                proxy()->drawItemText(painter,
                             titleRect,
                             Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic, dwOpt->palette,
                             dwOpt->state & State_Enabled, titleText,
                             QPalette::WindowText);
                }
        }
        painter->restore();
        break;
#endif // QT_NO_DOCKWIDGET
    case CE_HeaderSection:
        painter->save();
        // Draws the header in tables.
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            QPixmap cache;
            QString pixmapName = QStyleHelper::uniqueName(QLatin1String("headersection"), option, option->rect.size());
            pixmapName += QString::number(- int(header->position));
            pixmapName += QString::number(- int(header->orientation));
            QRect r = option->rect;
            QColor gradientStopColor;
            QColor gradientStartColor = option->palette.button().color();
            gradientStopColor.setHsv(gradientStartColor.hue(),
                                     qMin(255, (int)(gradientStartColor.saturation()*2)),
                                     qMin(255, (int)(gradientStartColor.value()*0.96)));
            QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
            if (option->palette.background().gradient()) {
                gradient.setStops(option->palette.background().gradient()->stops());
            } else {
                gradient.setColorAt(0, gradientStartColor);
                gradient.setColorAt(0.8, gradientStartColor);
                gradient.setColorAt(1, gradientStopColor);
            }
            painter->fillRect(r, gradient);

            if (!QPixmapCache::find(pixmapName, cache)) {
                cache = QPixmap(r.size());
                cache.fill(Qt::transparent);
                QRect pixmapRect(0, 0, r.width(), r.height());
                QPainter cachePainter(&cache);
                if (header->orientation == Qt::Vertical) {
                    cachePainter.setPen(QPen(dark));
                    cachePainter.drawLine(pixmapRect.topRight(), pixmapRect.bottomRight());
                    if (header->position != QStyleOptionHeader::End) {
                        cachePainter.setPen(QPen(shadow));
                        cachePainter.drawLine(pixmapRect.bottomLeft() + QPoint(3, -1), pixmapRect.bottomRight() + QPoint(-3, -1));                                cachePainter.setPen(QPen(option->palette.light().color()));
                        cachePainter.drawLine(pixmapRect.bottomLeft() + QPoint(3, 0), pixmapRect.bottomRight() + QPoint(-3, 0));                              }
                } else {
                    cachePainter.setPen(QPen(dark));
                    cachePainter.drawLine(pixmapRect.bottomLeft(), pixmapRect.bottomRight());
                    cachePainter.setPen(QPen(shadow));
                    cachePainter.drawLine(pixmapRect.topRight() + QPoint(-1, 3), pixmapRect.bottomRight() + QPoint(-1, -3));                                  cachePainter.setPen(QPen(option->palette.light().color()));
                    cachePainter.drawLine(pixmapRect.topRight() + QPoint(0, 3), pixmapRect.bottomRight() + QPoint(0, -3));                                }
                cachePainter.end();
                QPixmapCache::insert(pixmapName, cache);
            }
            painter->drawPixmap(r.topLeft(), cache);
        }
        painter->restore();
        break;
    case CE_ProgressBarGroove:
        painter->save();
        {
            painter->fillRect(rect, option->palette.base());
            QColor borderColor = dark.lighter(110);
            painter->setPen(QPen(borderColor, 0));
            const QLine lines[4] = {
                QLine(QPoint(rect.left() + 1, rect.top()), QPoint(rect.right() - 1, rect.top())),
                QLine(QPoint(rect.left() + 1, rect.bottom()), QPoint(rect.right() - 1, rect.bottom())),
                QLine(QPoint(rect.left(), rect.top() + 1), QPoint(rect.left(), rect.bottom() - 1)),
                QLine(QPoint(rect.right(), rect.top() + 1), QPoint(rect.right(), rect.bottom() - 1))
            };
            painter->drawLines(lines, 4);
            QColor alphaCorner = mergedColors(borderColor, option->palette.background().color());
            QColor innerShadow = mergedColors(borderColor, option->palette.base().color());

            //corner smoothing
            painter->setPen(alphaCorner);
            const QPoint points[4] = {
                rect.topRight(),
                rect.topLeft(),
                rect.bottomRight(),
                rect.bottomLeft()
            };
            painter->drawPoints(points, 4);

            //inner shadow
            painter->setPen(innerShadow);
            painter->drawLine(QPoint(rect.left() + 1, rect.top() + 1),
                              QPoint(rect.right() - 1, rect.top() + 1));
            painter->drawLine(QPoint(rect.left() + 1, rect.top() + 1),
                              QPoint(rect.left() + 1, rect.bottom() + 1));

        }
        painter->restore();
        break;
    case CE_ProgressBarContents:
        painter->save();
        if (const QStyleOptionProgressBar *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            QRect rect = bar->rect;
            bool vertical = false;
            bool inverted = false;
            bool indeterminate = (bar->minimum == 0 && bar->maximum == 0);

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
                QTransform m = QTransform::fromTranslate(rect.height()-1, -1.0);
                m.rotate(90.0);
                painter->setTransform(m, true);
            }

            int maxWidth = rect.width() - 4;
            int minWidth = 4;
            qreal progress = qMax(bar->progress, bar->minimum); // workaround for bug in QProgressBar
            int progressBarWidth = (progress - bar->minimum) * qreal(maxWidth) / qMax(qreal(1.0), qreal(bar->maximum) - bar->minimum);
            int width = indeterminate ? maxWidth : qMax(minWidth, progressBarWidth);

            bool reverse = (!vertical && (bar->direction == Qt::RightToLeft)) || vertical;
            if (inverted)
                reverse = !reverse;

            QRect progressBar;
            if (!indeterminate) {
                if (!reverse) {
                    progressBar.setRect(rect.left() + 1, rect.top() + 1, width + 1, rect.height() - 3);
                } else {
                    progressBar.setRect(rect.right() - 1 - width, rect.top() + 1, width + 1, rect.height() - 3);
                }
            } else {
                Q_D(const QCleanlooksStyle);
                int slideWidth = ((rect.width() - 4) * 2) / 3;
                int step = ((d->animateStep * slideWidth) / d->animationFps) % slideWidth;
                if ((((d->animateStep * slideWidth) / d->animationFps) % (2 * slideWidth)) >= slideWidth)
                    step = slideWidth - step;
                progressBar.setRect(rect.left() + 1 + step, rect.top() + 1,
                                    slideWidth / 2, rect.height() - 3);
            }
            QColor highlight = option->palette.color(QPalette::Normal, QPalette::Highlight);
            painter->setPen(QPen(highlight.darker(140), 0));

            QColor highlightedGradientStartColor = highlight.lighter(100);
            QColor highlightedGradientStopColor  = highlight.lighter(130);

            QLinearGradient gradient(rect.topLeft(), QPoint(rect.bottomLeft().x(),
                                                            rect.bottomLeft().y()*2));

            gradient.setColorAt(0, highlightedGradientStartColor);
            gradient.setColorAt(1, highlightedGradientStopColor);

            painter->setBrush(gradient);
            painter->drawRect(progressBar);

            painter->setPen(QPen(highlight.lighter(120), 0));
            painter->drawLine(QPoint(progressBar.left() + 1, progressBar.top() + 1),
                              QPoint(progressBar.right(), progressBar.top() + 1));
            painter->drawLine(QPoint(progressBar.left() + 1, progressBar.top() + 1),
                              QPoint(progressBar.left() + 1, progressBar.bottom() - 1));

            painter->setPen(QPen(highlightedGradientStartColor, 7.0));//QPen(option->palette.highlight(), 3));

            painter->save();
            painter->setClipRect(progressBar.adjusted(2, 2, -1, -1));
            for (int x = progressBar.left() - 32; x < rect.right() ; x+=18) {
                painter->drawLine(x, progressBar.bottom() + 1, x + 23, progressBar.top() - 2);
            }
            painter->restore();

        }
        painter->restore();
        break;
    case CE_MenuBarItem:
        painter->save();
        if (const QStyleOptionMenuItem *mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option))
        {
            QStyleOptionMenuItem item = *mbi;
            item.rect = mbi->rect.adjusted(0, 3, 0, -1);
            QColor highlightOutline = highlight.darker(125);
            QLinearGradient gradient(rect.topLeft(), QPoint(rect.bottomLeft().x(), rect.bottomLeft().y()*2));

            if (option->palette.button().gradient()) {
                gradient.setStops(option->palette.button().gradient()->stops());
            } else {
                gradient.setColorAt(0, option->palette.button().color());
                gradient.setColorAt(1, option->palette.button().color().darker(110));
            }
            painter->fillRect(rect, gradient);

            QCommonStyle::drawControl(element, &item, painter, widget);

            bool act = mbi->state & State_Selected && mbi->state & State_Sunken;
            bool dis = !(mbi->state & State_Enabled);

            QRect r = option->rect;
            if (act) {
                qt_cleanlooks_draw_gradient(painter, r.adjusted(1, 1, -1, -1),
                                            highlight,
                                            highlightOutline, TopDown,
                                            option->palette.highlight());

                painter->setPen(QPen(highlightOutline, 0));
                const QLine lines[4] = {
                    QLine(QPoint(r.left(), r.top() + 1), QPoint(r.left(), r.bottom())),
                    QLine(QPoint(r.right(), r.top() + 1), QPoint(r.right(), r.bottom())),
                    QLine(QPoint(r.left() + 1, r.bottom()), QPoint(r.right() - 1, r.bottom())),
                    QLine(QPoint(r.left() + 1, r.top()), QPoint(r.right() - 1, r.top()))
                };
                painter->drawLines(lines, 4);

                //draw text
                QPalette::ColorRole textRole = dis ? QPalette::Text : QPalette::HighlightedText;
                uint alignment = Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                if (!styleHint(SH_UnderlineShortcut, mbi, widget))
                    alignment |= Qt::TextHideMnemonic;
                proxy()->drawItemText(painter, item.rect, alignment, mbi->palette, mbi->state & State_Enabled, mbi->text, textRole);
            }

        }
        painter->restore();
        break;
    case CE_MenuItem:
        painter->save();
        // Draws one item in a popup menu.
        if (const QStyleOptionMenuItem *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            QColor highlightOutline = highlight.darker(125);
            QColor menuBackground = option->palette.background().color().lighter(104);
            QColor borderColor = option->palette.background().color().darker(160);
            QColor alphaCornerColor;

            if (widget) {
                // ### backgroundrole/foregroundrole should be part of the style option
                alphaCornerColor = mergedColors(option->palette.color(widget->backgroundRole()), borderColor);
            } else {
                alphaCornerColor = mergedColors(option->palette.background().color(), borderColor);
            }
            if (menuItem->menuItemType == QStyleOptionMenuItem::Separator) {
                painter->fillRect(menuItem->rect, menuBackground);
                int w = 0;
                if (!menuItem->text.isEmpty()) {
                    painter->setFont(menuItem->font);
                    proxy()->drawItemText(painter, menuItem->rect.adjusted(5, 0, -5, 0), Qt::AlignLeft | Qt::AlignVCenter,
                                 menuItem->palette, menuItem->state & State_Enabled, menuItem->text,
                                 QPalette::Text);
                    w = menuItem->fontMetrics.width(menuItem->text) + 5;
                }
                painter->setPen(shadow.lighter(106));
                bool reverse = menuItem->direction == Qt::RightToLeft;
                painter->drawLine(menuItem->rect.left() + 5 + (reverse ? 0 : w), menuItem->rect.center().y(),
                                  menuItem->rect.right() - 5 - (reverse ? w : 0), menuItem->rect.center().y());
                painter->restore();
                break;
            }
            bool selected = menuItem->state & State_Selected && menuItem->state & State_Enabled;
            if (selected) {
                QRect r = option->rect.adjusted(1, 0, -2, -1);
                qt_cleanlooks_draw_gradient(painter, r, highlight,
                                            highlightOutline, TopDown,
                                            highlight);
                r = r.adjusted(-1, 0, 1, 0);
                painter->setPen(QPen(highlightOutline, 0));
                const QLine lines[4] = {
                    QLine(QPoint(r.left(), r.top() + 1), QPoint(r.left(), r.bottom() - 1)),
                    QLine(QPoint(r.right(), r.top() + 1), QPoint(r.right(), r.bottom() - 1)),
                    QLine(QPoint(r.left() + 1, r.bottom()), QPoint(r.right() - 1, r.bottom())),
                    QLine(QPoint(r.left() + 1, r.top()), QPoint(r.right() - 1, r.top()))
                };
                painter->drawLines(lines, 4);
            } else {
                painter->fillRect(option->rect, menuBackground);
            }

            bool checkable = menuItem->checkType != QStyleOptionMenuItem::NotCheckable;
            bool checked = menuItem->checked;
            bool sunken = menuItem->state & State_Sunken;
            bool enabled = menuItem->state & State_Enabled;

            bool ignoreCheckMark = false;
            int checkcol = qMax(menuItem->maxIconWidth, 20);

#ifndef QT_NO_COMBOBOX
            if (qobject_cast<const QComboBox*>(widget))
                ignoreCheckMark = true; //ignore the checkmarks provided by the QComboMenuDelegate
#endif

            if (!ignoreCheckMark) {
                // Check
                QRect checkRect(option->rect.left() + 7, option->rect.center().y() - 6, 13, 13);
                checkRect = visualRect(menuItem->direction, menuItem->rect, checkRect);
                if (checkable) {
                    if (menuItem->checkType & QStyleOptionMenuItem::Exclusive) {
                        // Radio button
                        if (checked || sunken) {
                            painter->setRenderHint(QPainter::Antialiasing);
                            painter->setPen(Qt::NoPen);

                            QPalette::ColorRole textRole = !enabled ? QPalette::Text:
                                                        selected ? QPalette::HighlightedText : QPalette::ButtonText;
                            painter->setBrush(option->palette.brush( option->palette.currentColorGroup(), textRole));
                            painter->drawEllipse(checkRect.adjusted(4, 4, -4, -4));
                        }
                    } else {
                        // Check box
                        if (menuItem->icon.isNull()) {
                            if (checked || sunken) {
                                QImage image(qt_cleanlooks_menuitem_checkbox_checked);
                                if (enabled && (menuItem->state & State_Selected)) {
                                    image.setColor(1, 0x55ffffff);
                                    image.setColor(2, 0xAAffffff);
                                    image.setColor(3, 0xBBffffff);
                                    image.setColor(4, 0xFFffffff);
                                    image.setColor(5, 0x33ffffff);
                                } else {
                                    image.setColor(1, 0x55000000);
                                    image.setColor(2, 0xAA000000);
                                    image.setColor(3, 0xBB000000);
                                    image.setColor(4, 0xFF000000);
                                    image.setColor(5, 0x33000000);
                                }
                                painter->drawImage(QPoint(checkRect.center().x() - image.width() / 2,
                                                        checkRect.center().y() - image.height() / 2), image);
                            }
                        }
                    }
                }
            } else { //ignore checkmark
                if (menuItem->icon.isNull())
                    checkcol = 0;
                else
                    checkcol = menuItem->maxIconWidth;
            }

            // Text and icon, ripped from windows style
            bool dis = !(menuItem->state & State_Enabled);
            bool act = menuItem->state & State_Selected;
            const QStyleOption *opt = option;
            const QStyleOptionMenuItem *menuitem = menuItem;

            QPainter *p = painter;
            QRect vCheckRect = visualRect(opt->direction, menuitem->rect,
                                          QRect(menuitem->rect.x(), menuitem->rect.y(),
                                                checkcol, menuitem->rect.height()));
            if (!menuItem->icon.isNull()) {
                QIcon::Mode mode = dis ? QIcon::Disabled : QIcon::Normal;
                if (act && !dis)
                    mode = QIcon::Active;
                QPixmap pixmap;

                int smallIconSize = proxy()->pixelMetric(PM_SmallIconSize, option, widget);
                QSize iconSize(smallIconSize, smallIconSize);
#ifndef QT_NO_COMBOBOX
                if (const QComboBox *combo = qobject_cast<const QComboBox*>(widget))
                    iconSize = combo->iconSize();
#endif // QT_NO_COMBOBOX
                if (checked)
                    pixmap = menuItem->icon.pixmap(iconSize, mode, QIcon::On);
                else
                    pixmap = menuItem->icon.pixmap(iconSize, mode);

                int pixw = pixmap.width();
                int pixh = pixmap.height();

                QRect pmr(0, 0, pixw, pixh);
                pmr.moveCenter(vCheckRect.center());
                painter->setPen(menuItem->palette.text().color());
                if (checkable && checked) {
                    QStyleOption opt = *option;
                    if (act) {
                        QColor activeColor = mergedColors(option->palette.background().color(),
                                                        option->palette.highlight().color());
                        opt.palette.setBrush(QPalette::Button, activeColor);
                    }
                    opt.state |= State_Sunken;
                    opt.rect = vCheckRect;
                    proxy()->drawPrimitive(PE_PanelButtonCommand, &opt, painter, widget);
                }
                painter->drawPixmap(pmr.topLeft(), pixmap);
            }
            if (selected) {
                painter->setPen(menuItem->palette.highlightedText().color());
            } else {
                painter->setPen(menuItem->palette.text().color());
            }
            int x, y, w, h;
            menuitem->rect.getRect(&x, &y, &w, &h);
            int tab = menuitem->tabWidth;
            QColor discol;
            if (dis) {
                discol = menuitem->palette.text().color();
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
                    if (dis && !act && proxy()->styleHint(SH_EtchDisabledText, option, widget)) {
                        p->setPen(menuitem->palette.light().color());
                        p->drawText(vShortcutRect.adjusted(1, 1, 1, 1), text_flags, s.mid(t + 1));
                        p->setPen(discol);
                    }
                    p->drawText(vShortcutRect, text_flags, s.mid(t + 1));
                    s = s.left(t);
                }
                QFont font = menuitem->font;
                // font may not have any "hard" flags set. We override
                // the point size so that when it is resolved against the device, this font will win.
                // This is mainly to handle cases where someone sets the font on the window
                // and then the combo inherits it and passes it onward. At that point the resolve mask
                // is very, very weak. This makes it stonger.
                font.setPointSizeF(QFontInfo(menuItem->font).pointSizeF());

                if (menuitem->menuItemType == QStyleOptionMenuItem::DefaultItem)
                    font.setBold(true);

                p->setFont(font);
                if (dis && !act && proxy()->styleHint(SH_EtchDisabledText, option, widget)) {
                    p->setPen(menuitem->palette.light().color());
                    p->drawText(vTextRect.adjusted(1, 1, 1, 1), text_flags, s.left(t));
                    p->setPen(discol);
                }
                p->drawText(vTextRect, text_flags, s.left(t));
                p->restore();
            }

            // Arrow
            if (menuItem->menuItemType == QStyleOptionMenuItem::SubMenu) {// draw sub menu arrow
                int dim = (menuItem->rect.height() - 4) / 2;
                PrimitiveElement arrow;
                arrow = QApplication::isRightToLeft() ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight;
                int xpos = menuItem->rect.left() + menuItem->rect.width() - 3 - dim;
                QRect  vSubMenuRect = visualRect(option->direction, menuItem->rect,
                                                 QRect(xpos, menuItem->rect.top() + menuItem->rect.height() / 2 - dim / 2, dim, dim));
                QStyleOptionMenuItem newMI = *menuItem;
                newMI.rect = vSubMenuRect;
                newMI.state = !enabled ? State_None : State_Enabled;
                if (selected)
                    newMI.palette.setColor(QPalette::ButtonText,
                                           newMI.palette.highlightedText().color());
                proxy()->drawPrimitive(arrow, &newMI, painter, widget);
            }
        }
        painter->restore();
        break;
    case CE_MenuHMargin:
    case CE_MenuVMargin:
        break;
    case CE_MenuEmptyArea:
        break;
    case CE_PushButtonLabel:
        if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            QRect ir = button->rect;
            uint tf = Qt::AlignVCenter;
            if (styleHint(SH_UnderlineShortcut, button, widget))
                tf |= Qt::TextShowMnemonic;
            else
               tf |= Qt::TextHideMnemonic;

            if (!button->icon.isNull()) {
                //Center both icon and text
                QPoint point;

                QIcon::Mode mode = button->state & State_Enabled ? QIcon::Normal
                                                              : QIcon::Disabled;
                if (mode == QIcon::Normal && button->state & State_HasFocus)
                    mode = QIcon::Active;
                QIcon::State state = QIcon::Off;
                if (button->state & State_On)
                    state = QIcon::On;

                QPixmap pixmap = button->icon.pixmap(button->iconSize, mode, state);
                int w = pixmap.width();
                int h = pixmap.height();

                if (!button->text.isEmpty())
                    w += button->fontMetrics.boundingRect(option->rect, tf, button->text).width() + 2;

                point = QPoint(ir.x() + ir.width() / 2 - w / 2,
                               ir.y() + ir.height() / 2 - h / 2);

                if (button->direction == Qt::RightToLeft)
                    point.rx() += pixmap.width();

                painter->drawPixmap(visualPos(button->direction, button->rect, point), pixmap);

                if (button->direction == Qt::RightToLeft)
                    ir.translate(-point.x() - 2, 0);
                else
                    ir.translate(point.x() + pixmap.width(), 0);

                // left-align text if there is
                if (!button->text.isEmpty())
                    tf |= Qt::AlignLeft;

            } else {
                tf |= Qt::AlignHCenter;
            }

            if (button->features & QStyleOptionButton::HasMenu)
                ir = ir.adjusted(0, 0, -proxy()->pixelMetric(PM_MenuButtonIndicator, button, widget), 0);
            proxy()->drawItemText(painter, ir, tf, button->palette, (button->state & State_Enabled),
                         button->text, QPalette::ButtonText);
        }
        break;
    case CE_MenuBarEmptyArea:
        painter->save();
        {
            QColor shadow = mergedColors(option->palette.background().color().darker(120),
                                 dark.lighter(140), 60);

            QLinearGradient gradient(rect.topLeft(), QPoint(rect.bottomLeft().x(), rect.bottomLeft().y()*2));
            gradient.setColorAt(0, option->palette.button().color());
            gradient.setColorAt(1, option->palette.button().color().darker(110));
            painter->fillRect(rect, gradient);

#ifndef QT_NO_MAINWINDOW
            if (widget && qobject_cast<const QMainWindow *>(widget->parentWidget())) {
                QPen oldPen = painter->pen();
                painter->setPen(QPen(shadow));
                painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
            }
#endif // QT_NO_MAINWINDOW
        }
        painter->restore();
        break;
#ifndef QT_NO_TABBAR
	case CE_TabBarTabShape:
        painter->save();
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {

            bool rtlHorTabs = (tab->direction == Qt::RightToLeft
                               && (tab->shape == QTabBar::RoundedNorth
                                   || tab->shape == QTabBar::RoundedSouth));
            bool selected = tab->state & State_Selected;
            bool lastTab = ((!rtlHorTabs && tab->position == QStyleOptionTab::End)
                            || (rtlHorTabs
                                && tab->position == QStyleOptionTab::Beginning));
            bool onlyTab = tab->position == QStyleOptionTab::OnlyOneTab;
            bool leftCornerWidget = (tab->cornerWidgets & QStyleOptionTab::LeftCornerWidget);

            bool atBeginning = ((tab->position == (tab->direction == Qt::LeftToRight ?
                                QStyleOptionTab::Beginning : QStyleOptionTab::End)) || onlyTab);

            bool onlyOne = tab->position == QStyleOptionTab::OnlyOneTab;
            bool previousSelected =
                ((!rtlHorTabs
                  && tab->selectedPosition == QStyleOptionTab::PreviousIsSelected)
                 || (rtlHorTabs
                     && tab->selectedPosition == QStyleOptionTab::NextIsSelected));
            bool nextSelected =
                ((!rtlHorTabs
                  && tab->selectedPosition == QStyleOptionTab::NextIsSelected)
                 || (rtlHorTabs
                     && tab->selectedPosition
                     == QStyleOptionTab::PreviousIsSelected));
            int tabBarAlignment = proxy()->styleHint(SH_TabBar_Alignment, tab, widget);
            bool leftAligned = (!rtlHorTabs && tabBarAlignment == Qt::AlignLeft)
                               || (rtlHorTabs
                                   && tabBarAlignment == Qt::AlignRight);

            bool rightAligned = (!rtlHorTabs && tabBarAlignment == Qt::AlignRight)
                                || (rtlHorTabs
                                    && tabBarAlignment == Qt::AlignLeft);

            QColor light = tab->palette.light().color();

            QColor background = tab->palette.background().color();
            int borderThinkness = proxy()->pixelMetric(PM_TabBarBaseOverlap, tab, widget);
            if (selected)
                borderThinkness /= 2;
            QRect r2(option->rect);
            int x1 = r2.left();
            int x2 = r2.right();
            int y1 = r2.top();
            int y2 = r2.bottom();

            QTransform rotMatrix;
            bool flip = false;
            painter->setPen(shadow);
            QColor activeHighlight = option->palette.color(QPalette::Normal, QPalette::Highlight);
            switch (tab->shape) {
            case QTabBar::RoundedNorth:
                break;
            case QTabBar::RoundedSouth:
                rotMatrix.rotate(180);
                rotMatrix.translate(0, -rect.height() + 1);
                rotMatrix.scale(-1, 1);
                painter->setTransform(rotMatrix, true);
                break;
            case QTabBar::RoundedWest:
                rotMatrix.rotate(180 + 90);
                rotMatrix.scale(-1, 1);
                flip = true;
                painter->setTransform(rotMatrix, true);
                break;
            case QTabBar::RoundedEast:
                rotMatrix.rotate(90);
                rotMatrix.translate(0, - rect.width() + 1);
                flip = true;
                painter->setTransform(rotMatrix, true);
                break;
            default:
                painter->restore();
                QWindowsStyle::drawControl(element, tab, painter, widget);
                return;
            }

            if (flip) {
                QRect tmp = rect;
                rect = QRect(tmp.y(), tmp.x(), tmp.height(), tmp.width());
                int temp = x1;
                x1 = y1;
                y1 = temp;
                temp = x2;
                x2 = y2;
                y2 = temp;
            }

            QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
            if (option->palette.button().gradient()) {
                if (selected)
                    gradient.setStops(option->palette.background().gradient()->stops());
                else
                    gradient.setStops(option->palette.background().gradient()->stops());
            }
            else if (selected) {
                gradient.setColorAt(0, option->palette.background().color().lighter(104));
                gradient.setColorAt(1, tabFrameColor);
                painter->fillRect(rect.adjusted(0, 2, 0, -1), gradient);
            } else {
                y1 += 2;
                gradient.setColorAt(0, option->palette.background().color());
                gradient.setColorAt(1, dark.lighter(120));
                painter->fillRect(rect.adjusted(0, 2, 0, -2), gradient);
            }

            // Delete border
            if (selected) {
                painter->setPen(QPen(activeHighlight, 0));
                painter->drawLine(x1 + 1, y1 + 1, x2 - 1, y1 + 1);
                painter->drawLine(x1 , y1 + 2, x2 , y1 + 2);
            } else {
                painter->setPen(dark);
                painter->drawLine(x1, y2 - 1, x2 + 2, y2 - 1 );
                if (tab->shape == QTabBar::RoundedNorth || tab->shape == QTabBar::RoundedWest) {
                    painter->setPen(light);
                    painter->drawLine(x1, y2 , x2, y2 );
                }
            }
            // Left
            if (atBeginning || selected ) {
                painter->setPen(light);
                painter->drawLine(x1 + 1, y1 + 2 + 1, x1 + 1, y2 - ((onlyOne || atBeginning) && selected && leftAligned ? 0 : borderThinkness) - (atBeginning && leftCornerWidget ? 1 : 0));
                painter->drawPoint(x1 + 1, y1 + 1);
                painter->setPen(dark);
                painter->drawLine(x1, y1 + 2, x1, y2 - ((onlyOne || atBeginning)  && leftAligned ? 0 : borderThinkness) - (atBeginning && leftCornerWidget ? 1 : 0));
            }
            // Top
            {
                int beg = x1 + (previousSelected ? 0 : 2);
                int end = x2 - (nextSelected ? 0 : 2);
                painter->setPen(light);

                if (!selected)painter->drawLine(beg - 2, y1 + 1, end, y1 + 1);

                if (selected)
                    painter->setPen(QPen(activeHighlight.darker(150), 0));
                else
                    painter->setPen(darkOutline);
                painter->drawLine(beg, y1 , end, y1);

                if (atBeginning|| selected) {
                    painter->drawPoint(beg - 1, y1 + 1);
                } else if (!atBeginning) {
                    painter->drawPoint(beg - 1, y1);
                    painter->drawPoint(beg - 2, y1);
                    if (!lastTab) {
                        painter->setPen(dark.lighter(130));
                        painter->drawPoint(end + 1, y1);
                        painter->drawPoint(end + 2 , y1);
                        painter->drawPoint(end + 2, y1 + 1);
                    }
                }
            }
            // Right
            if (lastTab || selected || onlyOne || !nextSelected) {
                painter->setPen(darkOutline);
                painter->drawLine(x2, y1 + 2, x2, y2 - ((onlyOne || lastTab) && selected && rightAligned ? 0 : borderThinkness));
                if (selected)
                    painter->setPen(QPen(activeHighlight.darker(150), 0));
                else
                    painter->setPen(darkOutline);
                painter->drawPoint(x2 - 1, y1 + 1);

                if (selected) {
                    painter->setPen(background.darker(110));
                    painter->drawLine(x2 - 1, y1 + 3, x2 - 1, y2 - ((onlyOne || lastTab) && selected && rightAligned ? 0 : borderThinkness));
                }
            }
        }
        painter->restore();
        break;

#endif // QT_NO_TABBAR
    default:
        QWindowsStyle::drawControl(element,option,painter,widget);
        break;
    }
}

/*!
  \reimp
*/
QPalette QCleanlooksStyle::standardPalette () const
{
    QPalette palette = QWindowsStyle::standardPalette();
    palette.setBrush(QPalette::Active, QPalette::Highlight, QColor(98, 140, 178));
    palette.setBrush(QPalette::Inactive, QPalette::Highlight, QColor(145, 141, 126));
    palette.setBrush(QPalette::Disabled, QPalette::Highlight, QColor(145, 141, 126));

    QColor backGround(239, 235, 231);

    QColor light = backGround.lighter(150);
    QColor base = Qt::white;
    QColor dark = QColor(170, 156, 143).darker(110);
    dark = backGround.darker(150);
    QColor darkDisabled = QColor(209, 200, 191).darker(110);

    //### Find the correct disabled text color
    palette.setBrush(QPalette::Disabled, QPalette::Text, QColor(190, 190, 190));

    palette.setBrush(QPalette::Window, backGround);
    palette.setBrush(QPalette::Mid, backGround.darker(130));
    palette.setBrush(QPalette::Light, light);

    palette.setBrush(QPalette::Active, QPalette::Base, base);
    palette.setBrush(QPalette::Inactive, QPalette::Base, base);
    palette.setBrush(QPalette::Disabled, QPalette::Base, backGround);

    palette.setBrush(QPalette::Midlight, palette.mid().color().lighter(110));

    palette.setBrush(QPalette::All, QPalette::Dark, dark);
    palette.setBrush(QPalette::Disabled, QPalette::Dark, darkDisabled);

    QColor button = backGround;

    palette.setBrush(QPalette::Button, button);

    QColor shadow = dark.darker(135);
    palette.setBrush(QPalette::Shadow, shadow);
    palette.setBrush(QPalette::Disabled, QPalette::Shadow, shadow.lighter(150));
    palette.setBrush(QPalette::HighlightedText, QColor(QRgb(0xffffffff)));
    return palette;
}

/*!
  \reimp
*/
void QCleanlooksStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                         QPainter *painter, const QWidget *widget) const
{
    QColor button = option->palette.button().color();
    QColor dark;
    QColor grooveColor;
    QColor darkOutline;
    dark.setHsv(button.hue(),
                qMin(255, (int)(button.saturation()*1.9)),
                qMin(255, (int)(button.value()*0.7)));
    grooveColor.setHsv(button.hue(),
                qMin(255, (int)(button.saturation()*2.6)),
                qMin(255, (int)(button.value()*0.9)));
    darkOutline.setHsv(button.hue(),
                qMin(255, (int)(button.saturation()*3.0)),
                qMin(255, (int)(button.value()*0.6)));

    QColor alphaCornerColor;
    if (widget) {
        // ### backgroundrole/foregroundrole should be part of the style option
        alphaCornerColor = mergedColors(option->palette.color(widget->backgroundRole()), darkOutline);
    } else {
        alphaCornerColor = mergedColors(option->palette.background().color(), darkOutline);
    }
    QColor gripShadow = grooveColor.darker(110);
    QColor buttonShadow = option->palette.button().color().darker(110);

    QColor gradientStartColor = option->palette.button().color().lighter(108);
    QColor gradientStopColor = mergedColors(option->palette.button().color().darker(108), dark.lighter(150), 70);

    QPalette palette = option->palette;

    switch (control) {
#ifndef QT_NO_SPINBOX
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            QPixmap cache;
            QString pixmapName = QStyleHelper::uniqueName(QLatin1String("spinbox"), spinBox, spinBox->rect.size());
            if (!QPixmapCache::find(pixmapName, cache)) {
                cache = QPixmap(spinBox->rect.size());
                cache.fill(Qt::transparent);
                QRect pixmapRect(0, 0, spinBox->rect.width(), spinBox->rect.height());
                QPainter cachePainter(&cache);

                bool isEnabled = (spinBox->state & State_Enabled);
                //bool focus = isEnabled && (spinBox->state & State_HasFocus);
                bool hover = isEnabled && (spinBox->state & State_MouseOver);
                bool sunken = (spinBox->state & State_Sunken);
                bool upIsActive = (spinBox->activeSubControls == SC_SpinBoxUp);
                bool downIsActive = (spinBox->activeSubControls == SC_SpinBoxDown);

                QRect rect = pixmapRect;
                QStyleOptionSpinBox spinBoxCopy = *spinBox;
                spinBoxCopy.rect = pixmapRect;
                QRect upRect = proxy()->subControlRect(CC_SpinBox, &spinBoxCopy, SC_SpinBoxUp, widget);
                QRect downRect = proxy()->subControlRect(CC_SpinBox, &spinBoxCopy, SC_SpinBoxDown, widget);

                int fw = spinBoxCopy.frame ? proxy()->pixelMetric(PM_SpinBoxFrameWidth, &spinBoxCopy, widget) : 0;
                cachePainter.fillRect(rect.adjusted(1, qMax(fw - 1, 0), -1, -fw),
                                      option->palette.base());

                QRect r = rect.adjusted(0, 1, 0, -1);
                if (spinBox->frame) {

                    QColor topShadow = darkOutline;
                    topShadow.setAlpha(60);
                    cachePainter.setPen(topShadow);

                    // antialias corners
                    const QPoint points[8] = {
                        QPoint(r.right(), r.top() + 1),
                        QPoint(r.right() - 1, r.top() ),
                        QPoint(r.right(), r.bottom() - 1),
                        QPoint(r.right() - 1, r.bottom() ),
                        QPoint(r.left() + 1, r.bottom()),
                        QPoint(r.left(), r.bottom() - 1),
                        QPoint(r.left() + 1, r.top()),
                        QPoint(r.left(), r.top() + 1)
                    };
                    cachePainter.drawPoints(points, 8);

                    // draw frame
                    topShadow.setAlpha(30);
                    cachePainter.setPen(topShadow);
                    cachePainter.drawLine(QPoint(r.left() + 2, r.top() - 1), QPoint(r.right() - 2, r.top() - 1));

                    cachePainter.setPen(QPen(option->palette.background().color(), 1));
                    cachePainter.drawLine(QPoint(r.left() + 2, r.top() + 1), QPoint(r.right() - 2, r.top() + 1));
                    QColor highlight = Qt::white;
                    highlight.setAlpha(130);
                    cachePainter.setPen(option->palette.base().color().darker(120));
                    cachePainter.drawLine(QPoint(r.left() + 1, r.top() + 1),
                                  QPoint(r.right() - 1, r.top() + 1));
                    cachePainter.drawLine(QPoint(r.left() + 1, r.top() + 1),
                                  QPoint(r.left() + 1, r.bottom() - 1));
                    cachePainter.setPen(option->palette.base().color());
                    cachePainter.drawLine(QPoint(r.right() - 1, r.top() + 1),
                                  QPoint(r.right() - 1, r.bottom() - 1));
                    cachePainter.drawLine(QPoint(r.left() + 1, r.bottom() - 1),
                                  QPoint(r.right() - 1, r.bottom() - 1));
                    cachePainter.setPen(highlight);
                    cachePainter.drawLine(QPoint(r.left() + 3, r.bottom() + 1),
                                  QPoint(r.right() - 3, r.bottom() + 1));

                    cachePainter.setPen(QPen(darkOutline, 1));

                    // top and bottom lines
                    const QLine lines[4] = {
                        QLine(QPoint(r.left() + 2, r.bottom()), QPoint(r.right()- 2, r.bottom())),
                        QLine(QPoint(r.left() + 2, r.top()), QPoint(r.right() - 2, r.top())),
                        QLine(QPoint(r.right(), r.top() + 2), QPoint(r.right(), r.bottom() - 2)),
                        QLine(QPoint(r.left(), r.top() + 2), QPoint(r.left(), r.bottom() - 2))
                    };
                    cachePainter.drawLines(lines, 4);
                }

                    // gradients
                    qt_cleanlooks_draw_gradient(&cachePainter, upRect,
                                            gradientStartColor.darker(106),
                                            gradientStopColor, TopDown, option->palette.button());
                    qt_cleanlooks_draw_gradient(&cachePainter, downRect.adjusted(0, 0, 0, 1),
                                            gradientStartColor.darker(106),
                                            gradientStopColor, TopDown, option->palette.button());
                if (isEnabled) {
                    if(upIsActive) {
                        if (sunken) {
                            cachePainter.fillRect(upRect.adjusted(1, 0, 0, 0), gradientStopColor.darker(110));
                        } else if (hover) {
                            qt_cleanlooks_draw_gradient(&cachePainter, upRect.adjusted(1, 0, 0, 0),
                                                    gradientStartColor.lighter(110),
                                                    gradientStopColor.lighter(110), TopDown, option->palette.button());
                        }
                    }
                    if(downIsActive) {
                        if (sunken) {
                            cachePainter.fillRect(downRect.adjusted(1, 0, 0, 1), gradientStopColor.darker(110));

                        } else if (hover) {
                                qt_cleanlooks_draw_gradient(&cachePainter, downRect.adjusted(1, 0, 0, 1),
                                                        gradientStartColor.lighter(110),
                                                        gradientStopColor.lighter(110), TopDown, option->palette.button());
                        }
                    }
                }

                if (spinBox->frame) {
                    // rounded corners
                    const QPoint points[4] = {
                        QPoint(r.left() + 1, r.bottom() - 1),
                        QPoint(r.left() + 1, r.top() + 1),
                        QPoint(r.right() - 1, r.bottom() - 1),
                        QPoint(r.right() - 1, r.top() + 1)
                    };
                    cachePainter.drawPoints(points, 4);

                    if (option->state & State_HasFocus) {
                        QColor darkoutline = option->palette.highlight().color().darker(150);
                        QColor innerline = mergedColors(option->palette.highlight().color(), Qt::white);
                        cachePainter.setPen(QPen(innerline, 0));
                        if (spinBox->direction == Qt::LeftToRight) {
                            cachePainter.drawRect(rect.adjusted(1, 2, -3 -downRect.width(), -3));
                            cachePainter.setPen(QPen(darkoutline, 0));
                            const QLine lines[4] = {
                                QLine(QPoint(r.left() + 2, r.bottom()), QPoint(r.right()- downRect.width() - 1, r.bottom())),
                                QLine(QPoint(r.left() + 2, r.top()), QPoint(r.right() - downRect.width() - 1, r.top())),
                                QLine(QPoint(r.right() - downRect.width() - 1, r.top() + 1), QPoint(r.right()- downRect.width() - 1, r.bottom() - 1)),
                                QLine(QPoint(r.left(), r.top() + 2), QPoint(r.left(), r.bottom() - 2))
                            };
                            cachePainter.drawLines(lines, 4);
                            cachePainter.drawPoint(QPoint(r.left() + 1, r.bottom() - 1));
                            cachePainter.drawPoint(QPoint(r.left() + 1, r.top() + 1));
                            cachePainter.drawLine(QPoint(r.left(), r.top() + 2), QPoint(r.left(), r.bottom() - 2));
                        } else {
                            cachePainter.drawRect(rect.adjusted(downRect.width() + 2, 2, -2, -3));
                            cachePainter.setPen(QPen(darkoutline, 0));
                            cachePainter.drawLine(QPoint(r.left() + downRect.width(), r.bottom()), QPoint(r.right()- 2 - 1, r.bottom()));
                            cachePainter.drawLine(QPoint(r.left() + downRect.width(), r.top()), QPoint(r.right() - 2 - 1, r.top()));

                            cachePainter.drawLine(QPoint(r.right(), r.top() + 2), QPoint(r.right(), r.bottom() - 2));
                            cachePainter.drawPoint(QPoint(r.right() - 1, r.bottom() - 1));
                            cachePainter.drawPoint(QPoint(r.right() - 1, r.top() + 1));
                            cachePainter.drawLine(QPoint(r.left() + downRect.width() + 1, r.top()),
                                                  QPoint(r.left() + downRect.width() + 1, r.bottom()));
                        }
                    }
                }

                // outline the up/down buttons
                cachePainter.setPen(darkOutline);
                QColor light = option->palette.light().color().lighter();

                if (spinBox->direction == Qt::RightToLeft) {
                    cachePainter.drawLine(upRect.right(), upRect.top() - 1, upRect.right(), downRect.bottom() + 1);
                    cachePainter.setPen(light);
                    cachePainter.drawLine(upRect.right() - 1, upRect.top() + 3, upRect.right() - 1, downRect.bottom() );
                } else {
                    cachePainter.drawLine(upRect.left(), upRect.top() - 1, upRect.left(), downRect.bottom() + 1);
                    cachePainter.setPen(light);
                    cachePainter.drawLine(upRect.left() + 1, upRect.top() , upRect.left() + 1, downRect.bottom() );
                }
                if (upIsActive && sunken) {
                    cachePainter.setPen(gradientStopColor.darker(130));
                    cachePainter.drawLine(upRect.left() + 1, upRect.top(), upRect.left() + 1, upRect.bottom());
                    cachePainter.drawLine(upRect.left(), upRect.top() - 1, upRect.right(), upRect.top() - 1);
                } else {
                    cachePainter.setPen(light);
                    cachePainter.drawLine(upRect.topLeft() + QPoint(1, -1), upRect.topRight() + QPoint(-1, -1));
                    cachePainter.setPen(darkOutline);
                    cachePainter.drawLine(upRect.bottomLeft(), upRect.bottomRight());
                }
                if (downIsActive && sunken) {
                    cachePainter.setPen(gradientStopColor.darker(130));
                    cachePainter.drawLine(downRect.left() + 1, downRect.top(), downRect.left() + 1, downRect.bottom() + 1);
                    cachePainter.drawLine(downRect.left(), downRect.top(), downRect.right(), downRect.top());
                    cachePainter.setPen(gradientStopColor.darker(110));
                    cachePainter.drawLine(downRect.left(), downRect.bottom() + 1, downRect.right(), downRect.bottom() + 1);
                } else {
                    cachePainter.setPen(light);
                    cachePainter.drawLine(downRect.topLeft() + QPoint(2,0), downRect.topRight());
                }

                if (spinBox->buttonSymbols == QAbstractSpinBox::PlusMinus) {
                    int centerX = upRect.center().x();
                    int centerY = upRect.center().y();
                    cachePainter.setPen(spinBox->palette.foreground().color());

                    // plus/minus
                    if (spinBox->activeSubControls == SC_SpinBoxUp && sunken) {
                        cachePainter.drawLine(1 + centerX - 2, 1 + centerY, 1 + centerX + 2, 1 + centerY);
                        cachePainter.drawLine(1 + centerX, 1 + centerY - 2, 1 + centerX, 1 + centerY + 2);
                    } else {
                        cachePainter.drawLine(centerX - 2, centerY, centerX + 2, centerY);
                        cachePainter.drawLine(centerX, centerY - 2, centerX, centerY + 2);
                    }

                    centerX = downRect.center().x();
                    centerY = downRect.center().y();
                    if (spinBox->activeSubControls == SC_SpinBoxDown && sunken) {
                        cachePainter.drawLine(1 + centerX - 2, 1 + centerY, 1 + centerX + 2, 1 + centerY);
                    } else {
                        cachePainter.drawLine(centerX - 2, centerY, centerX + 2, centerY);
                    }
                } else if (spinBox->buttonSymbols == QAbstractSpinBox::UpDownArrows){
                    // arrows
                    QImage upArrow(qt_spinbox_button_arrow_up);
                    upArrow.setColor(1, spinBox->palette.foreground().color().rgba());

                    cachePainter.drawImage(upRect.center().x() - upArrow.width() / 2,
                                            upRect.center().y() - upArrow.height() / 2,
                                            upArrow);

                    QImage downArrow(qt_spinbox_button_arrow_down);
                    downArrow.setColor(1, spinBox->palette.foreground().color().rgba());

                    cachePainter.drawImage(downRect.center().x() - downArrow.width() / 2,
                                            downRect.center().y() - downArrow.height() / 2 + 1,
                                            downArrow);
                }

                QColor disabledColor = option->palette.background().color();
                disabledColor.setAlpha(150);
                if (!(spinBox->stepEnabled & QAbstractSpinBox::StepUpEnabled))
                    cachePainter.fillRect(upRect.adjusted(1, 0, 0, 0), disabledColor);
                if (!(spinBox->stepEnabled & QAbstractSpinBox::StepDownEnabled)) {
                    cachePainter.fillRect(downRect.adjusted(1, 0, 0, 0), disabledColor);
                }
                cachePainter.end();
                QPixmapCache::insert(pixmapName, cache);
            }
            painter->drawPixmap(spinBox->rect.topLeft(), cache);
        }
        break;
#endif // QT_NO_SPINBOX
    case CC_TitleBar:
        painter->save();
        if (const QStyleOptionTitleBar *titleBar = qstyleoption_cast<const QStyleOptionTitleBar *>(option)) {
            const int buttonMargin = 5;
            bool active = (titleBar->titleBarState & State_Active);
            QRect fullRect = titleBar->rect;
            QPalette palette = option->palette;
            QColor highlight = option->palette.highlight().color();

            QColor titleBarFrameBorder(active ? highlight.darker(180): dark.darker(110));
            QColor titleBarHighlight(active ? highlight.lighter(120): palette.background().color().lighter(120));
            QColor textColor(active ? 0xffffff : 0xff000000);
            QColor textAlphaColor(active ? 0xffffff : 0xff000000 );

#ifdef  QT3_SUPPORT
            if (widget && widget->inherits("Q3DockWindowTitleBar")) {
                QStyleOptionDockWidgetV2 dockwidget;
                dockwidget.QStyleOption::operator=(*option);
                proxy()->drawControl(CE_DockWidgetTitle, &dockwidget, painter, widget);
            } else
#endif // QT3_SUPPORT
            {
                // Fill title bar gradient
                QColor titlebarColor = QColor(active ? highlight: palette.background().color());
                QLinearGradient gradient(option->rect.center().x(), option->rect.top(),
                                         option->rect.center().x(), option->rect.bottom());

                gradient.setColorAt(0, titlebarColor.lighter(114));
                gradient.setColorAt(0.5, titlebarColor.lighter(102));
                gradient.setColorAt(0.51, titlebarColor.darker(104));
                gradient.setColorAt(1, titlebarColor);
                painter->fillRect(option->rect.adjusted(1, 1, -1, 0), gradient);

                // Frame and rounded corners
                painter->setPen(titleBarFrameBorder);

                // top outline
                painter->drawLine(fullRect.left() + 5, fullRect.top(), fullRect.right() - 5, fullRect.top());
                painter->drawLine(fullRect.left(), fullRect.top() + 4, fullRect.left(), fullRect.bottom());
                const QPoint points[5] = {
                    QPoint(fullRect.left() + 4, fullRect.top() + 1),
                    QPoint(fullRect.left() + 3, fullRect.top() + 1),
                    QPoint(fullRect.left() + 2, fullRect.top() + 2),
                    QPoint(fullRect.left() + 1, fullRect.top() + 3),
                    QPoint(fullRect.left() + 1, fullRect.top() + 4)
                };
                painter->drawPoints(points, 5);

                painter->drawLine(fullRect.right(), fullRect.top() + 4, fullRect.right(), fullRect.bottom());
                const QPoint points2[5] = {
                    QPoint(fullRect.right() - 3, fullRect.top() + 1),
                    QPoint(fullRect.right() - 4, fullRect.top() + 1),
                    QPoint(fullRect.right() - 2, fullRect.top() + 2),
                    QPoint(fullRect.right() - 1, fullRect.top() + 3),
                    QPoint(fullRect.right() - 1, fullRect.top() + 4)
                };
                painter->drawPoints(points2, 5);

                // draw bottomline
                painter->drawLine(fullRect.right(), fullRect.bottom(), fullRect.left(), fullRect.bottom());

                // top highlight
                painter->setPen(titleBarHighlight);
                painter->drawLine(fullRect.left() + 6, fullRect.top() + 1, fullRect.right() - 6, fullRect.top() + 1);
            }
            // draw title
            QRect textRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarLabel, widget);
            QFont font = painter->font();
            font.setBold(true);
            painter->setFont(font);
            painter->setPen(active? (titleBar->palette.text().color().lighter(120)) :
                                     titleBar->palette.text().color() );
            // Note workspace also does elliding but it does not use the correct font
            QString title = QFontMetrics(font).elidedText(titleBar->text, Qt::ElideRight, textRect.width() - 14);
            painter->drawText(textRect.adjusted(1, 1, 1, 1), title, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));
            painter->setPen(Qt::white);
            if (active)
                painter->drawText(textRect, title, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));
            // min button
            if ((titleBar->subControls & SC_TitleBarMinButton) && (titleBar->titleBarFlags & Qt::WindowMinimizeButtonHint) &&
                !(titleBar->titleBarState& Qt::WindowMinimized)) {
                QRect minButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarMinButton, widget);
                if (minButtonRect.isValid()) {
                    bool hover = (titleBar->activeSubControls & SC_TitleBarMinButton) && (titleBar->state & State_MouseOver);
                    bool sunken = (titleBar->activeSubControls & SC_TitleBarMinButton) && (titleBar->state & State_Sunken);
                    qt_cleanlooks_draw_mdibutton(painter, titleBar, minButtonRect, hover, sunken);
                    QRect minButtonIconRect = minButtonRect.adjusted(buttonMargin ,buttonMargin , -buttonMargin, -buttonMargin);
                    painter->setPen(textColor);
                    painter->drawLine(minButtonIconRect.center().x() - 2, minButtonIconRect.center().y() + 3,
                                    minButtonIconRect.center().x() + 3, minButtonIconRect.center().y() + 3);
                    painter->drawLine(minButtonIconRect.center().x() - 2, minButtonIconRect.center().y() + 4,
                                    minButtonIconRect.center().x() + 3, minButtonIconRect.center().y() + 4);
                    painter->setPen(textAlphaColor);
                    painter->drawLine(minButtonIconRect.center().x() - 3, minButtonIconRect.center().y() + 3,
                                    minButtonIconRect.center().x() - 3, minButtonIconRect.center().y() + 4);
                    painter->drawLine(minButtonIconRect.center().x() + 4, minButtonIconRect.center().y() + 3,
                                    minButtonIconRect.center().x() + 4, minButtonIconRect.center().y() + 4);
                }
            }
            // max button
            if ((titleBar->subControls & SC_TitleBarMaxButton) && (titleBar->titleBarFlags & Qt::WindowMaximizeButtonHint) &&
                !(titleBar->titleBarState & Qt::WindowMaximized)) {
                QRect maxButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarMaxButton, widget);
                if (maxButtonRect.isValid()) {
                    bool hover = (titleBar->activeSubControls & SC_TitleBarMaxButton) && (titleBar->state & State_MouseOver);
                    bool sunken = (titleBar->activeSubControls & SC_TitleBarMaxButton) && (titleBar->state & State_Sunken);
                    qt_cleanlooks_draw_mdibutton(painter, titleBar, maxButtonRect, hover, sunken);

                    QRect maxButtonIconRect = maxButtonRect.adjusted(buttonMargin, buttonMargin, -buttonMargin, -buttonMargin);

                    painter->setPen(textColor);
                    painter->drawRect(maxButtonIconRect.adjusted(0, 0, -1, -1));
                    painter->drawLine(maxButtonIconRect.left() + 1, maxButtonIconRect.top() + 1,
                                    maxButtonIconRect.right() - 1, maxButtonIconRect.top() + 1);
                    painter->setPen(textAlphaColor);
                    const QPoint points[4] = {
                        maxButtonIconRect.topLeft(),
                        maxButtonIconRect.topRight(),
                        maxButtonIconRect.bottomLeft(),
                        maxButtonIconRect.bottomRight()
                    };
                    painter->drawPoints(points, 4);
                }
            }

            // close button
            if ((titleBar->subControls & SC_TitleBarCloseButton) && (titleBar->titleBarFlags & Qt::WindowSystemMenuHint)) {
                QRect closeButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarCloseButton, widget);
                if (closeButtonRect.isValid()) {
                    bool hover = (titleBar->activeSubControls & SC_TitleBarCloseButton) && (titleBar->state & State_MouseOver);
                    bool sunken = (titleBar->activeSubControls & SC_TitleBarCloseButton) && (titleBar->state & State_Sunken);
                    qt_cleanlooks_draw_mdibutton(painter, titleBar, closeButtonRect, hover, sunken);
                    QRect closeIconRect = closeButtonRect.adjusted(buttonMargin, buttonMargin, -buttonMargin, -buttonMargin);
                    painter->setPen(textAlphaColor);
                    const QLine lines[4] = {
                        QLine(closeIconRect.left() + 1, closeIconRect.top(),
                              closeIconRect.right(), closeIconRect.bottom() - 1),
                        QLine(closeIconRect.left(), closeIconRect.top() + 1,
                              closeIconRect.right() - 1, closeIconRect.bottom()),
                        QLine(closeIconRect.right() - 1, closeIconRect.top(),
                              closeIconRect.left(), closeIconRect.bottom() - 1),
                        QLine(closeIconRect.right(), closeIconRect.top() + 1,
                              closeIconRect.left() + 1, closeIconRect.bottom())
                    };
                    painter->drawLines(lines, 4);
                    const QPoint points[4] = {
                        closeIconRect.topLeft(),
                        closeIconRect.topRight(),
                        closeIconRect.bottomLeft(),
                        closeIconRect.bottomRight()
                    };
                    painter->drawPoints(points, 4);

                    painter->setPen(textColor);
                    painter->drawLine(closeIconRect.left() + 1, closeIconRect.top() + 1,
                                    closeIconRect.right() - 1, closeIconRect.bottom() - 1);
                    painter->drawLine(closeIconRect.left() + 1, closeIconRect.bottom() - 1,
                                    closeIconRect.right() - 1, closeIconRect.top() + 1);
                }
            }

            // normalize button
            if ((titleBar->subControls & SC_TitleBarNormalButton) &&
               (((titleBar->titleBarFlags & Qt::WindowMinimizeButtonHint) &&
               (titleBar->titleBarState & Qt::WindowMinimized)) ||
               ((titleBar->titleBarFlags & Qt::WindowMaximizeButtonHint) &&
               (titleBar->titleBarState & Qt::WindowMaximized)))) {
                QRect normalButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarNormalButton, widget);
                if (normalButtonRect.isValid()) {

                    bool hover = (titleBar->activeSubControls & SC_TitleBarNormalButton) && (titleBar->state & State_MouseOver);
                    bool sunken = (titleBar->activeSubControls & SC_TitleBarNormalButton) && (titleBar->state & State_Sunken);
                    QRect normalButtonIconRect = normalButtonRect.adjusted(buttonMargin, buttonMargin, -buttonMargin, -buttonMargin);
                    qt_cleanlooks_draw_mdibutton(painter, titleBar, normalButtonRect, hover, sunken);

                    QRect frontWindowRect = normalButtonIconRect.adjusted(0, 3, -3, 0);
                    painter->setPen(textColor);
                    painter->drawRect(frontWindowRect.adjusted(0, 0, -1, -1));
                    painter->drawLine(frontWindowRect.left() + 1, frontWindowRect.top() + 1,
                                    frontWindowRect.right() - 1, frontWindowRect.top() + 1);
                    painter->setPen(textAlphaColor);
                    const QPoint points[4] = {
                        frontWindowRect.topLeft(),
                        frontWindowRect.topRight(),
                        frontWindowRect.bottomLeft(),
                        frontWindowRect.bottomRight()
                    };
                    painter->drawPoints(points, 4);

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
                    const QPoint points2[4] = {
                        backWindowRect.topLeft(),
                        backWindowRect.topRight(),
                        backWindowRect.bottomLeft(),
                        backWindowRect.bottomRight()
                    };
                    painter->drawPoints(points2, 4);
                    painter->restore();
                }
            }

            // context help button
            if (titleBar->subControls & SC_TitleBarContextHelpButton
                && (titleBar->titleBarFlags & Qt::WindowContextHelpButtonHint)) {
                QRect contextHelpButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarContextHelpButton, widget);
                if (contextHelpButtonRect.isValid()) {
                    bool hover = (titleBar->activeSubControls & SC_TitleBarContextHelpButton) && (titleBar->state & State_MouseOver);
                    bool sunken = (titleBar->activeSubControls & SC_TitleBarContextHelpButton) && (titleBar->state & State_Sunken);
                    qt_cleanlooks_draw_mdibutton(painter, titleBar, contextHelpButtonRect, hover, sunken);

                    QColor blend;
                    QImage image(qt_titlebar_context_help);
                    QColor alpha = textColor;
                    alpha.setAlpha(128);
                    image.setColor(1, textColor.rgba());
                    image.setColor(2, alpha.rgba());
                    painter->setRenderHint(QPainter::SmoothPixmapTransform);
                    painter->drawImage(contextHelpButtonRect.adjusted(4, 4, -4, -4), image);
                }
            }

            // shade button
            if (titleBar->subControls & SC_TitleBarShadeButton && (titleBar->titleBarFlags & Qt::WindowShadeButtonHint)) {
                QRect shadeButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarShadeButton, widget);
                if (shadeButtonRect.isValid()) {
                    bool hover = (titleBar->activeSubControls & SC_TitleBarShadeButton) && (titleBar->state & State_MouseOver);
                    bool sunken = (titleBar->activeSubControls & SC_TitleBarShadeButton) && (titleBar->state & State_Sunken);
                    qt_cleanlooks_draw_mdibutton(painter, titleBar, shadeButtonRect, hover, sunken);
                    QImage image(qt_scrollbar_button_arrow_up);
                    image.setColor(1, textColor.rgba());
                    painter->drawImage(shadeButtonRect.adjusted(5, 7, -5, -7), image);
                }
            }

            // unshade button
            if (titleBar->subControls & SC_TitleBarUnshadeButton && (titleBar->titleBarFlags & Qt::WindowShadeButtonHint)) {
                QRect unshadeButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarUnshadeButton, widget);
                if (unshadeButtonRect.isValid()) {
                    bool hover = (titleBar->activeSubControls & SC_TitleBarUnshadeButton) && (titleBar->state & State_MouseOver);
                    bool sunken = (titleBar->activeSubControls & SC_TitleBarUnshadeButton) && (titleBar->state & State_Sunken);
                    qt_cleanlooks_draw_mdibutton(painter, titleBar, unshadeButtonRect, hover, sunken);
                    QImage image(qt_scrollbar_button_arrow_down);
                    image.setColor(1, textColor.rgba());
                    painter->drawImage(unshadeButtonRect.adjusted(5, 7, -5, -7), image);
                }
            }

            if ((titleBar->subControls & SC_TitleBarSysMenu) && (titleBar->titleBarFlags & Qt::WindowSystemMenuHint)) {
                QRect iconRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarSysMenu, widget);
                if (iconRect.isValid()) {
                    if (!titleBar->icon.isNull()) {
                        titleBar->icon.paint(painter, iconRect);
                    } else {
                        QStyleOption tool(0);
                        tool.palette = titleBar->palette;
                        QPixmap pm = standardIcon(SP_TitleBarMenuButton, &tool, widget).pixmap(16, 16);
                        tool.rect = iconRect;
                        painter->save();
                        proxy()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, pm);
                        painter->restore();
                    }
                }
            }
        }
        painter->restore();
        break;
#ifndef QT_NO_SCROLLBAR
    case CC_ScrollBar:
        painter->save();
        if (const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            bool isEnabled = scrollBar->state & State_Enabled;
            bool reverse = scrollBar->direction == Qt::RightToLeft;
            bool horizontal = scrollBar->orientation == Qt::Horizontal;
            bool sunken = scrollBar->state & State_Sunken;

            painter->fillRect(option->rect, option->palette.background());

            QRect scrollBarSubLine = proxy()->subControlRect(control, scrollBar, SC_ScrollBarSubLine, widget);
            QRect scrollBarAddLine = proxy()->subControlRect(control, scrollBar, SC_ScrollBarAddLine, widget);
            QRect scrollBarSlider = proxy()->subControlRect(control, scrollBar, SC_ScrollBarSlider, widget);
            QRect grooveRect = proxy()->subControlRect(control, scrollBar, SC_ScrollBarGroove, widget);

            // paint groove
            if (scrollBar->subControls & SC_ScrollBarGroove) {
                painter->setBrush(grooveColor);
                painter->setPen(Qt::NoPen);
                if (horizontal) {
                    painter->drawRect(grooveRect);
                    painter->setPen(darkOutline);
                    painter->drawLine(grooveRect.topLeft(), grooveRect.topRight());
                    painter->drawLine(grooveRect.bottomLeft(), grooveRect.bottomRight());
                } else {
                    painter->drawRect(grooveRect);
                    painter->setPen(darkOutline);
                    painter->drawLine(grooveRect.topLeft(), grooveRect.bottomLeft());
                    painter->drawLine(grooveRect.topRight(), grooveRect.bottomRight());
                }
            }
            //paint slider
            if (scrollBar->subControls & SC_ScrollBarSlider) {
                QRect pixmapRect = scrollBarSlider;
                if (horizontal)
                    pixmapRect.adjust(-1, 0, 0, -1);
                else
                    pixmapRect.adjust(0, -1, -1, 0);

                if (isEnabled) {
                    QLinearGradient gradient(pixmapRect.center().x(), pixmapRect.top(),
                                             pixmapRect.center().x(), pixmapRect.bottom());
                    if (!horizontal)
                        gradient = QLinearGradient(pixmapRect.left(), pixmapRect.center().y(),
                                                   pixmapRect.right(), pixmapRect.center().y());

                    if (option->palette.button().gradient()) {
                        gradient.setStops(option->palette.button().gradient()->stops());
                    } else {
                        if (sunken || (option->state & State_MouseOver &&
                            (scrollBar->activeSubControls & SC_ScrollBarSlider))) {
                            gradient.setColorAt(0, gradientStartColor.lighter(110));
                            gradient.setColorAt(1, gradientStopColor.lighter(110));
                        } else {
                            gradient.setColorAt(0, gradientStartColor);
                            gradient.setColorAt(1, gradientStopColor);
                        }
                    }
                    painter->setPen(QPen(darkOutline, 0));
                    painter->setBrush(gradient);
                    painter->drawRect(pixmapRect);


                    //calculate offsets used by highlight and shadow
                    int yoffset, xoffset;
                    if (option->state & State_Horizontal) {
                        xoffset = 0;
                        yoffset = 1;
                    } else {
                        xoffset = 1;
                        yoffset = 0;
                    }
                    //draw slider highlights
                    painter->setPen(QPen(gradientStopColor, 0));
                    painter->drawLine(scrollBarSlider.left() + xoffset,
                                      scrollBarSlider.bottom() - yoffset,
                                      scrollBarSlider.right() - xoffset,
                                      scrollBarSlider.bottom() - yoffset);
                    painter->drawLine(scrollBarSlider.right() - xoffset,
                                      scrollBarSlider.top() + yoffset,
                                      scrollBarSlider.right() - xoffset,
                                      scrollBarSlider.bottom() - yoffset);

                    //draw slider shadow
                    painter->setPen(QPen(gradientStartColor, 0));
                    painter->drawLine(scrollBarSlider.left() + xoffset,
                                      scrollBarSlider.top() + yoffset,
                                      scrollBarSlider.right() - xoffset,
                                      scrollBarSlider.top() + yoffset);
                    painter->drawLine(scrollBarSlider.left() + xoffset,
                                      scrollBarSlider.top() + yoffset,
                                      scrollBarSlider.left() + xoffset,
                                      scrollBarSlider.bottom() - yoffset);
                } else {
                    QLinearGradient gradient(pixmapRect.center().x(), pixmapRect.top(),
                                             pixmapRect.center().x(), pixmapRect.bottom());
                    if (!horizontal) {
                        gradient = QLinearGradient(pixmapRect.left(), pixmapRect.center().y(),
                                                   pixmapRect.right(), pixmapRect.center().y());
                    }
                    if (sunken) {
                        gradient.setColorAt(0, gradientStartColor.lighter(110));
                        gradient.setColorAt(1, gradientStopColor.lighter(110));
                    } else {
                        gradient.setColorAt(0, gradientStartColor);
                        gradient.setColorAt(1, gradientStopColor);
                    }
                    painter->setPen(darkOutline);
                    painter->setBrush(gradient);
                    painter->drawRect(pixmapRect);
                }
                int gripMargin = 4;
                //draw grips
                if (horizontal) {
                    for (int i = -3; i< 6 ; i += 3) {
                        painter->setPen(QPen(gripShadow, 1));
                        painter->drawLine(
                            QPoint(scrollBarSlider.center().x() + i ,
                                   scrollBarSlider.top() + gripMargin),
                            QPoint(scrollBarSlider.center().x() + i,
                                   scrollBarSlider.bottom() - gripMargin));
                        painter->setPen(QPen(palette.light(), 1));
                        painter->drawLine(
                            QPoint(scrollBarSlider.center().x() + i + 1,
                                   scrollBarSlider.top() + gripMargin  ),
                            QPoint(scrollBarSlider.center().x() + i + 1,
                                   scrollBarSlider.bottom() - gripMargin));
                    }
                } else {
                    for (int i = -3; i < 6 ; i += 3) {
                        painter->setPen(QPen(gripShadow, 1));
                        painter->drawLine(
                            QPoint(scrollBarSlider.left() + gripMargin ,
                                   scrollBarSlider.center().y()+ i),
                            QPoint(scrollBarSlider.right() - gripMargin,
                                   scrollBarSlider.center().y()+ i));
                        painter->setPen(QPen(palette.light(), 1));
                        painter->drawLine(
                            QPoint(scrollBarSlider.left() + gripMargin,
                                   scrollBarSlider.center().y() + 1 + i),
                            QPoint(scrollBarSlider.right() - gripMargin,
                                   scrollBarSlider.center().y() + 1 + i));
                    }
                }
            }

            // The SubLine (up/left) buttons
            if (scrollBar->subControls & SC_ScrollBarSubLine) {
                //int scrollBarExtent = proxy()->pixelMetric(PM_ScrollBarExtent, option, widget);
                QRect pixmapRect = scrollBarSubLine;
                if (isEnabled ) {
                    QRect fillRect = pixmapRect.adjusted(1, 1, -1, -1);
                    // Gradients
                    if ((scrollBar->activeSubControls & SC_ScrollBarSubLine) && sunken) {
                        qt_cleanlooks_draw_gradient(painter,
                                                    QRect(fillRect),
                                                    gradientStopColor.darker(120),
                                                    gradientStopColor.darker(120),
                                                    horizontal ? TopDown : FromLeft, option->palette.button());
                    } else {
                        qt_cleanlooks_draw_gradient(painter,
                                                    QRect(fillRect),
                                                    gradientStartColor.lighter(105),
                                                    gradientStopColor,
                                                    horizontal ? TopDown : FromLeft, option->palette.button());
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
                subButton.setColor(2, darkOutline.rgba());
                if ((scrollBar->activeSubControls & SC_ScrollBarSubLine) && sunken) {
                    subButton.setColor(3, gradientStopColor.darker(140).rgba());
                    subButton.setColor(4, gradientStopColor.darker(120).rgba());
                } else {
                    subButton.setColor(3, gradientStartColor.lighter(105).rgba());
                    subButton.setColor(4, gradientStopColor.rgba());
                }
                subButton.setColor(5, scrollBar->palette.text().color().rgba());
                painter->drawImage(pixmapRect, subButton);

                // Arrows
                PrimitiveElement arrow;
                if (option->state & State_Horizontal)
                    arrow = option->direction == Qt::LeftToRight ? PE_IndicatorArrowLeft: PE_IndicatorArrowRight;
                else
                    arrow = PE_IndicatorArrowUp;
                QStyleOption arrowOpt = *option;
                arrowOpt.rect = scrollBarSubLine.adjusted(3, 3, -2, -2);
                proxy()->drawPrimitive(arrow, &arrowOpt, painter, widget);


                // The AddLine (down/right) button
                if (scrollBar->subControls & SC_ScrollBarAddLine) {
                    QString addLinePixmapName = QStyleHelper::uniqueName(QLatin1String("scrollbar_addline"), option, QSize(16, 16));
                    QRect pixmapRect = scrollBarAddLine;
                    if (isEnabled) {
                        QRect fillRect = pixmapRect.adjusted(1, 1, -1, -1);
                        // Gradients
                        if ((scrollBar->activeSubControls & SC_ScrollBarAddLine) && sunken) {
                            qt_cleanlooks_draw_gradient(painter,
                                                        fillRect,
                                                        gradientStopColor.darker(120),
                                                        gradientStopColor.darker(120),
                                                        horizontal ? TopDown: FromLeft, option->palette.button());
                        } else {
                            qt_cleanlooks_draw_gradient(painter,
                                                        fillRect,
                                                        gradientStartColor.lighter(105),
                                                        gradientStopColor,
                                                        horizontal ? TopDown : FromLeft, option->palette.button());
                        }
                    }
                    // Details
                    QImage addButton;
                    if (horizontal) {
                        addButton = QImage(reverse ? qt_scrollbar_button_left : qt_scrollbar_button_right);
                    } else {
                        addButton = QImage(qt_scrollbar_button_down);
                    }
                    addButton.setColor(1, alphaCornerColor.rgba());
                    addButton.setColor(2, darkOutline.rgba());
                    if ((scrollBar->activeSubControls & SC_ScrollBarAddLine) && sunken) {
                        addButton.setColor(3, gradientStopColor.darker(140).rgba());
                        addButton.setColor(4, gradientStopColor.darker(120).rgba());
                    } else {
                        addButton.setColor(3, gradientStartColor.lighter(105).rgba());
                        addButton.setColor(4, gradientStopColor.rgba());
                    }
                    addButton.setColor(5, scrollBar->palette.text().color().rgba());
                    painter->drawImage(pixmapRect, addButton);

                    PrimitiveElement arrow;
                    if (option->state & State_Horizontal)
                        arrow = option->direction == Qt::LeftToRight ? PE_IndicatorArrowRight : PE_IndicatorArrowLeft;
                    else
                        arrow = PE_IndicatorArrowDown;

                    QStyleOption arrowOpt = *option;
                    arrowOpt.rect = scrollBarAddLine.adjusted(3, 3, -2, -2);
                    proxy()->drawPrimitive(arrow, &arrowOpt, painter, widget);
                }
            }
        }
        painter->restore();
        break;;
#endif // QT_NO_SCROLLBAR
#ifndef QT_NO_COMBOBOX
    case CC_ComboBox:
        painter->save();
        if (const QStyleOptionComboBox *comboBox = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            bool sunken = comboBox->state & State_On; // play dead, if combobox has no items
            bool isEnabled = (comboBox->state & State_Enabled);
            bool focus = isEnabled && (comboBox->state & State_HasFocus);
            QPixmap cache;
            QString pixmapName = QStyleHelper::uniqueName(QLatin1String("combobox"), option, comboBox->rect.size());
            if (sunken)
                pixmapName += QLatin1String("-sunken");
            if (comboBox->editable)
                pixmapName += QLatin1String("-editable");
            if (isEnabled)
                pixmapName += QLatin1String("-enabled");

            if (!QPixmapCache::find(pixmapName, cache)) {
                cache = QPixmap(comboBox->rect.size());
                cache.fill(Qt::transparent);
                QPainter cachePainter(&cache);
                QRect pixmapRect(0, 0, comboBox->rect.width(), comboBox->rect.height());
                QStyleOptionComboBox comboBoxCopy = *comboBox;
                comboBoxCopy.rect = pixmapRect;

                QRect rect = pixmapRect;
                QRect downArrowRect = proxy()->subControlRect(CC_ComboBox, &comboBoxCopy,
                                                     SC_ComboBoxArrow, widget);
                QRect editRect = proxy()->subControlRect(CC_ComboBox, &comboBoxCopy,
                                                     SC_ComboBoxEditField, widget);
                // Draw a push button
                if (comboBox->editable) {
                    QStyleOptionFrame  buttonOption;
                    buttonOption.QStyleOption::operator=(*comboBox);
                    buttonOption.rect = rect;
                    buttonOption.state = comboBox->state & (State_Enabled | State_MouseOver);

                    if (sunken) {
                        buttonOption.state |= State_Sunken;
                        buttonOption.state &= ~State_MouseOver;
                    }

                    proxy()->drawPrimitive(PE_PanelButtonCommand, &buttonOption, &cachePainter, widget);

                    //remove shadow from left side of edit field when pressed:
                    if (comboBox->direction != Qt::RightToLeft)
                        cachePainter.fillRect(editRect.left() - 1, editRect.top() + 1, editRect.left(),
                        editRect.bottom() - 3, option->palette.base());

                    cachePainter.setPen(dark.lighter(110));
                    if (!sunken) {
                        int borderSize = 2;
                        if (comboBox->direction == Qt::RightToLeft) {
                            cachePainter.drawLine(QPoint(downArrowRect.right() - 1, downArrowRect.top() + borderSize ),
                                                  QPoint(downArrowRect.right() - 1, downArrowRect.bottom() - borderSize));
                            cachePainter.setPen(option->palette.light().color());
                            cachePainter.drawLine(QPoint(downArrowRect.right(), downArrowRect.top() + borderSize),
                                                  QPoint(downArrowRect.right(), downArrowRect.bottom() - borderSize));
                        } else {
                            cachePainter.drawLine(QPoint(downArrowRect.left() , downArrowRect.top() + borderSize),
                                                  QPoint(downArrowRect.left() , downArrowRect.bottom() - borderSize));
                            cachePainter.setPen(option->palette.light().color());
                            cachePainter.drawLine(QPoint(downArrowRect.left() + 1, downArrowRect.top() + borderSize),
                                                  QPoint(downArrowRect.left() + 1, downArrowRect.bottom() - borderSize));
                        }
                    } else {
                        if (comboBox->direction == Qt::RightToLeft) {
                            cachePainter.drawLine(QPoint(downArrowRect.right(), downArrowRect.top() + 2),
                                                  QPoint(downArrowRect.right(), downArrowRect.bottom() - 2));

                        } else {
                            cachePainter.drawLine(QPoint(downArrowRect.left(), downArrowRect.top() + 2),
                                                  QPoint(downArrowRect.left(), downArrowRect.bottom() - 2));
                        }
                    }
                } else {
                    QStyleOptionButton buttonOption;
                    buttonOption.QStyleOption::operator=(*comboBox);
                    buttonOption.rect = rect;
                    buttonOption.state = comboBox->state & (State_Enabled | State_MouseOver);
                    if (sunken) {
                        buttonOption.state |= State_Sunken;
                        buttonOption.state &= ~State_MouseOver;
                    }
                    proxy()->drawPrimitive(PE_PanelButtonCommand, &buttonOption, &cachePainter, widget);

                    cachePainter.setPen(buttonShadow.darker(102));
                    int borderSize = 4;

                    if (!sunken) {
                        if (comboBox->direction == Qt::RightToLeft) {
                            cachePainter.drawLine(QPoint(downArrowRect.right() + 1, downArrowRect.top() + borderSize),
                                                  QPoint(downArrowRect.right() + 1, downArrowRect.bottom() - borderSize));
                            cachePainter.setPen(option->palette.light().color());
                            cachePainter.drawLine(QPoint(downArrowRect.right(), downArrowRect.top() + borderSize),
                                                  QPoint(downArrowRect.right(), downArrowRect.bottom() - borderSize));
                        } else {
                            cachePainter.drawLine(QPoint(downArrowRect.left() - 1, downArrowRect.top() + borderSize),
                                                  QPoint(downArrowRect.left() - 1, downArrowRect.bottom() - borderSize));
                            cachePainter.setPen(option->palette.light().color());
                            cachePainter.drawLine(QPoint(downArrowRect.left() , downArrowRect.top() + borderSize),
                                                  QPoint(downArrowRect.left() , downArrowRect.bottom() - borderSize));
                        }
                    } else {
                        cachePainter.setPen(dark.lighter(110));
                        if (comboBox->direction == Qt::RightToLeft) {
                            cachePainter.drawLine(QPoint(downArrowRect.right() + 1, downArrowRect.top() + borderSize),
                                                  QPoint(downArrowRect.right() + 1, downArrowRect.bottom() - borderSize));

                        } else {
                            cachePainter.drawLine(QPoint(downArrowRect.left() - 1, downArrowRect.top() + borderSize),
                                                  QPoint(downArrowRect.left() - 1, downArrowRect.bottom() - borderSize));
                        }
                    }
                }


                if (comboBox->subControls & SC_ComboBoxArrow) {
                    if (comboBox->editable) {
                        // Draw the down arrow
                        QImage downArrow(qt_cleanlooks_arrow_down_xpm);
                        downArrow.setColor(1, comboBox->palette.foreground().color().rgba());
                        cachePainter.drawImage(downArrowRect.center().x() - downArrow.width() / 2,
                                               downArrowRect.center().y() - downArrow.height() / 2 + 1, downArrow);
                    } else {
                        // Draw the up/down arrow
                        QImage upArrow(qt_scrollbar_button_arrow_up);
                        upArrow.setColor(1, comboBox->palette.foreground().color().rgba());
                        QImage downArrow(qt_scrollbar_button_arrow_down);
                        downArrow.setColor(1, comboBox->palette.foreground().color().rgba());
                        cachePainter.drawImage(downArrowRect.center().x() - downArrow.width() / 2,
                                               downArrowRect.center().y() - upArrow.height() - 1 , upArrow);
                        cachePainter.drawImage(downArrowRect.center().x() - downArrow.width() / 2,
                                               downArrowRect.center().y()  + 2, downArrow);
                    }
                }
                // Draw the focus rect
                if (focus && !comboBox->editable
                    && ((option->state & State_KeyboardFocusChange) || styleHint(SH_UnderlineShortcut, option, widget))) {
                    QStyleOptionFocusRect focus;
                    focus.rect = proxy()->subControlRect(CC_ComboBox, &comboBoxCopy, SC_ComboBoxEditField, widget)
                                 .adjusted(0, 2, option->direction == Qt::RightToLeft ? 1 : -1, -2);
                    proxy()->drawPrimitive(PE_FrameFocusRect, &focus, &cachePainter, widget);
                }
                cachePainter.end();
                QPixmapCache::insert(pixmapName, cache);
            }
            painter->drawPixmap(comboBox->rect.topLeft(), cache);
        }
        painter->restore();
        break;
#endif // QT_NO_COMBOBOX
#ifndef QT_NO_GROUPBOX
    case CC_GroupBox:
        painter->save();
        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            QRect textRect = proxy()->subControlRect(CC_GroupBox, groupBox, SC_GroupBoxLabel, widget);
            QRect checkBoxRect = proxy()->subControlRect(CC_GroupBox, groupBox, SC_GroupBoxCheckBox, widget);
            bool flat = groupBox->features & QStyleOptionFrameV2::Flat;

            if(!flat) {
                if (groupBox->subControls & QStyle::SC_GroupBoxFrame) {
                    QStyleOptionFrameV2 frame;
                    frame.QStyleOption::operator=(*groupBox);
                    frame.features = groupBox->features;
                    frame.lineWidth = groupBox->lineWidth;
                    frame.midLineWidth = groupBox->midLineWidth;
                    frame.rect = proxy()->subControlRect(CC_GroupBox, option, SC_GroupBoxFrame, widget);

                    painter->save();
                    QRegion region(groupBox->rect);
                    bool ltr = groupBox->direction == Qt::LeftToRight;
                    region -= checkBoxRect.united(textRect).adjusted(ltr ? -4 : 0, 0, ltr ? 0 : 4, 0);
                    if (!groupBox->text.isEmpty() ||  groupBox->subControls & SC_GroupBoxCheckBox)
                        painter->setClipRegion(region);
                    frame.palette.setBrush(QPalette::Dark, option->palette.mid().color().lighter(110));
                    proxy()->drawPrimitive(PE_FrameGroupBox, &frame, painter);
                    painter->restore();
                }
            }
            // Draw title
            if ((groupBox->subControls & QStyle::SC_GroupBoxLabel) && !groupBox->text.isEmpty()) {
                if (!groupBox->text.isEmpty()) {
                    QColor textColor = groupBox->textColor;
                    if (textColor.isValid())
                        painter->setPen(textColor);
                    int alignment = int(groupBox->textAlignment);
                    if (!styleHint(QStyle::SH_UnderlineShortcut, option, widget))
                        alignment |= Qt::TextHideMnemonic;
                    if (flat) {
                        QFont font = painter->font();
                        font.setBold(true);
                        painter->setFont(font);
                        if (groupBox->subControls & SC_GroupBoxCheckBox) {
                            textRect.adjust(checkBoxRect.right() + 4, 0, checkBoxRect.right() + 4, 0);
                        }
                    }
                    painter->drawText(textRect, Qt::TextShowMnemonic | Qt::AlignLeft| alignment, groupBox->text);
                }
            }
            if (groupBox->subControls & SC_GroupBoxCheckBox) {
                QStyleOptionButton box;
                box.QStyleOption::operator=(*groupBox);
                box.rect = checkBoxRect;
                proxy()->drawPrimitive(PE_IndicatorCheckBox, &box, painter, widget);
            }
        }
        painter->restore();
        break;
#endif // QT_NO_GROUPBOX
#ifndef QT_NO_SLIDER
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            QRect groove = proxy()->subControlRect(CC_Slider, option, SC_SliderGroove, widget);
            QRect handle = proxy()->subControlRect(CC_Slider, option, SC_SliderHandle, widget);

            bool horizontal = slider->orientation == Qt::Horizontal;
            bool ticksAbove = slider->tickPosition & QSlider::TicksAbove;
            bool ticksBelow = slider->tickPosition & QSlider::TicksBelow;
            QColor activeHighlight = option->palette.color(QPalette::Normal, QPalette::Highlight);
            QPixmap cache;

            QBrush oldBrush = painter->brush();
            QPen oldPen = painter->pen();

            QColor shadowAlpha(Qt::black);
            shadowAlpha.setAlpha(10);
            QColor highlightAlpha(Qt::white);
            highlightAlpha.setAlpha(80);

            if ((option->subControls & SC_SliderGroove) && groove.isValid()) {
                QString groovePixmapName = QStyleHelper::uniqueName(QLatin1String("slider_groove"), option, groove.size());
                QRect pixmapRect(0, 0, groove.width(), groove.height());

                // draw background groove
                if (!QPixmapCache::find(groovePixmapName, cache)) {
                    cache = QPixmap(pixmapRect.size());
                    cache.fill(Qt::transparent);
                    QPainter groovePainter(&cache);

                    groovePainter.setPen(shadowAlpha);
                    groovePainter.drawLine(1, 0, groove.width(), 0);
                    groovePainter.drawLine(0, 0, 0, groove.height() - 1);

                    groovePainter.setPen(highlightAlpha);
                    groovePainter.drawLine(1, groove.height() - 1, groove.width() - 1, groove.height() - 1);
                    groovePainter.drawLine(groove.width() - 1, 1, groove.width() - 1, groove.height() - 1);
                    QLinearGradient gradient;
                    if (horizontal) {
                        gradient.setStart(pixmapRect.center().x(), pixmapRect.top());
                        gradient.setFinalStop(pixmapRect.center().x(), pixmapRect.bottom());
                    }
                    else {
                        gradient.setStart(pixmapRect.left(), pixmapRect.center().y());
                        gradient.setFinalStop(pixmapRect.right(), pixmapRect.center().y());
                    }
                    groovePainter.setPen(QPen(darkOutline.darker(110), 0));
                    gradient.setColorAt(0, grooveColor.darker(110));//dark.lighter(120));
                    gradient.setColorAt(1, grooveColor.lighter(110));//palette.button().color().darker(115));
                    groovePainter.setBrush(gradient);
                    groovePainter.drawRect(pixmapRect.adjusted(1, 1, -2, -2));
                    groovePainter.end();
                    QPixmapCache::insert(groovePixmapName, cache);
                }
                painter->drawPixmap(groove.topLeft(), cache);

                // draw blue groove highlight
                QRect clipRect;
                groovePixmapName += QLatin1String("_blue");
                if (!QPixmapCache::find(groovePixmapName, cache)) {
                    cache = QPixmap(pixmapRect.size());
                    cache.fill(Qt::transparent);
                    QPainter groovePainter(&cache);
                    QLinearGradient gradient;
                    if (horizontal) {
                        gradient.setStart(pixmapRect.center().x(), pixmapRect.top());
                        gradient.setFinalStop(pixmapRect.center().x(), pixmapRect.bottom());
                    }
                    else {
                        gradient.setStart(pixmapRect.left(), pixmapRect.center().y());
                        gradient.setFinalStop(pixmapRect.right(), pixmapRect.center().y());
                    }
                    groovePainter.setPen(QPen(activeHighlight.darker(150), 0));
                    gradient.setColorAt(0, activeHighlight.darker(120));
                    gradient.setColorAt(1, activeHighlight.lighter(160));
                    groovePainter.setBrush(gradient);
                    groovePainter.drawRect(pixmapRect.adjusted(1, 1, -2, -2));
                    groovePainter.end();
                    QPixmapCache::insert(groovePixmapName, cache);
                }
                if (horizontal) {
                    if (slider->upsideDown)
                        clipRect = QRect(handle.right(), groove.top(), groove.right() - handle.right(), groove.height());
                    else
                        clipRect = QRect(groove.left(), groove.top(), handle.left(), groove.height());
                } else {
                    if (slider->upsideDown)
                        clipRect = QRect(groove.left(), handle.bottom(), groove.width(), groove.height() - handle.bottom());
                    else
                        clipRect = QRect(groove.left(), groove.top(), groove.width(), handle.top() - groove.top());
                }
                painter->save();
                painter->setClipRect(clipRect.adjusted(0, 0, 1, 1));
                painter->drawPixmap(groove.topLeft(), cache);
                painter->restore();
            }

            // draw handle
            if ((option->subControls & SC_SliderHandle) ) {
                QString handlePixmapName = QStyleHelper::uniqueName(QLatin1String("slider_handle"), option, handle.size());
                if (!QPixmapCache::find(handlePixmapName, cache)) {
                    cache = QPixmap(handle.size());
                    cache.fill(Qt::transparent);
                    QRect pixmapRect(0, 0, handle.width(), handle.height());
                    QPainter handlePainter(&cache);

                    QColor gradientStartColor = mergedColors(option->palette.button().color().lighter(155),
                                                             dark.lighter(155), 50);
                    QColor gradientStopColor = gradientStartColor.darker(108);
                    QRect gradRect = pixmapRect.adjusted(2, 2, -2, -2);

                    QColor gradientBgStartColor = gradientStartColor;
                    QColor gradientBgStopColor = gradientStopColor;

                    QColor outline = option->state & State_Enabled ? dark : dark.lighter(130);
                    if (option->state & State_Enabled && option->activeSubControls & SC_SliderHandle) {
                        gradientBgStartColor = option->palette.highlight().color().lighter(180);
                        gradientBgStopColor = option->palette.highlight().color().lighter(110);
                        outline = option->palette.highlight().color().darker(130);
                    }

                    // gradient fill
                    QRect r = pixmapRect.adjusted(1, 1, -1, -1);

                    qt_cleanlooks_draw_gradient(&handlePainter, gradRect,
                                                gradientBgStartColor,
                                                gradientBgStopColor,
                                                horizontal ? TopDown : FromLeft, option->palette.button());

                    handlePainter.setPen(QPen(outline.darker(110), 1));
                    handlePainter.drawLine(QPoint(r.left(), r.top() + 3), QPoint(r.left(), r.bottom() - 3));
                    handlePainter.drawLine(QPoint(r.right(), r.top() + 3), QPoint(r.right(), r.bottom() - 3));
                    handlePainter.drawLine(QPoint(r.left() + 3, r.bottom()), QPoint(r.right() - 3, r.bottom()));

                    handlePainter.save();
                    handlePainter.setRenderHint(QPainter::Antialiasing);
                    handlePainter.translate(0.5, 0.5);
                    const QLine lines[4] = {
                        QLine(QPoint(r.left(), r.bottom() - 2), QPoint(r.left() + 2, r.bottom())),
                        QLine(QPoint(r.left(), r.top() + 2), QPoint(r.left() + 2, r.top())),
                        QLine(QPoint(r.right(), r.bottom() - 2), QPoint(r.right() - 2, r.bottom())),
                        QLine(QPoint(r.right(), r.top() + 2), QPoint(r.right() - 2, r.top()))
                    };
                    handlePainter.drawLines(lines, 4);
                    handlePainter.restore();;
                    handlePainter.setPen(QPen(outline.darker(130), 1));
                    handlePainter.drawLine(QPoint(r.left() + 3, r.top()), QPoint(r.right() - 3, r.top()));
                    QColor cornerAlpha = outline.darker(120);
                    cornerAlpha.setAlpha(80);

                    handlePainter.setPen(cornerAlpha);
                    if (horizontal) {
                        handlePainter.drawLine(QPoint(r.left() + 6, r.top()), QPoint(r.left() + 6, r.bottom()));
                        handlePainter.drawLine(QPoint(r.right() - 6, r.top()), QPoint(r.right() - 6, r.bottom()));
                    } else {
                        handlePainter.drawLine(QPoint(r.left(), r.top() + 6), QPoint(r.right(), r.top() + 6));
                        handlePainter.drawLine(QPoint(r.left(), r.bottom() - 6), QPoint(r.right(), r.bottom() - 6));
                    }

                    //handle shadow
                    handlePainter.setPen(shadowAlpha);
                    handlePainter.drawLine(QPoint(r.left() + 2, r.bottom() + 1), QPoint(r.right() - 2, r.bottom() + 1));
                    handlePainter.drawLine(QPoint(r.right() + 1, r.bottom() - 3), QPoint(r.right() + 1, r.top() + 4));
                    handlePainter.drawLine(QPoint(r.right() - 1, r.bottom()), QPoint(r.right() + 1, r.bottom() - 2));

                    qt_cleanlooks_draw_gradient(&handlePainter, horizontal ?
                        gradRect.adjusted(6, 0, -6, 0) : gradRect.adjusted(0, 6, 0, -6),
                        gradientStartColor,
                        gradientStopColor.darker(106),
                        horizontal ? TopDown : FromLeft,
                        option->palette.button());

                    //draw grips
                    for (int i = -3; i< 6 ; i += 3) {
                        for (int j = -3; j< 6 ; j += 3) {
                            handlePainter.fillRect(r.center().x() + i, r.center().y() + j, 2, 2, highlightAlpha);
                            handlePainter.setPen(gripShadow);
                            handlePainter.drawPoint(r.center().x() + i, r.center().y() + j );
                        }
                    }
                    handlePainter.end();
                    QPixmapCache::insert(handlePixmapName, cache);
                }

                painter->drawPixmap(handle.topLeft(), cache);

                if (slider->state & State_HasFocus) {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=(*slider);
                    fropt.rect = slider->rect;
                    proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
                }
            }
            if (option->subControls & SC_SliderTickmarks) {
                painter->setPen(darkOutline);
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
                            painter->drawLine(pos, slider->rect.top() + extra,
                                pos, slider->rect.top() + tickSize);
                        }
                        if (ticksBelow) {
                            painter->drawLine(pos, slider->rect.bottom() - extra,
                                            pos, slider->rect.bottom() - tickSize);
                        }
                    } else {
                        if (ticksAbove) {
                            painter->drawLine(slider->rect.left() + extra, pos,
                                            slider->rect.left() + tickSize, pos);
                        }
                        if (ticksBelow) {
                            painter->drawLine(slider->rect.right() - extra, pos,
                                            slider->rect.right() - tickSize, pos);
                        }
                    }
                    // in the case where maximum is max int
                    int nextInterval = v + interval;
                    if (nextInterval < v)
                        break;
                    v = nextInterval;
                }
            }
            painter->setBrush(oldBrush);
            painter->setPen(oldPen);
        }
        break;
#endif // QT_NO_SLIDER
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
int QCleanlooksStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    int ret = -1;
    switch (metric) {
    case PM_ToolTipLabelFrameWidth:
        ret = 2;
        break;
    case PM_ButtonDefaultIndicator:
        ret = 0;
        break;
    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical:
        ret = 0;
        break;
    case PM_MessageBoxIconSize:
        ret = 48;
        break;
    case PM_ListViewIconSize:
        ret = 24;
        break;
    case PM_DialogButtonsSeparator:
    case PM_SplitterWidth:
        ret = 6;
        break;
    case PM_ScrollBarSliderMin:
        ret = 26;
        break;
    case PM_MenuPanelWidth: //menu framewidth
        ret = 2;
        break;
    case PM_TitleBarHeight:
        ret = 24;
        break;
    case PM_ScrollBarExtent:
        ret = 15;
        break;
    case PM_SliderThickness:
        ret = 15;
        break;
    case PM_SliderLength:
        ret = 27;
        break;
    case PM_DockWidgetTitleMargin:
        ret = 1;
        break;
    case PM_MenuBarVMargin:
        ret = 1;
        break;
    case PM_DefaultFrameWidth:
        ret = 2;
        break;
    case PM_SpinBoxFrameWidth:
        ret = 3;
        break;
    case PM_MenuBarItemSpacing:
        ret = 6;
        break;
    case PM_MenuBarHMargin:
        ret = 0;
        break;
    case PM_ToolBarHandleExtent:
        ret = 9;
        break;
    case PM_ToolBarItemSpacing:
        ret = 2;
        break;
    case PM_ToolBarFrameWidth:
        ret = 0;
        break;
    case PM_ToolBarItemMargin:
        ret = 1;
        break;
    case PM_SmallIconSize:
        ret = 16;
        break;
    case PM_ButtonIconSize:
        ret = 24;
        break;
    case PM_MenuVMargin:
    case PM_MenuHMargin:
        ret = 0;
        break;
    case PM_DockWidgetTitleBarButtonMargin:
        ret = 4;
        break;
    case PM_MaximumDragDistance:
        return -1;
    case PM_TabCloseIndicatorWidth:
    case PM_TabCloseIndicatorHeight:
        return 20;
    default:
        break;
    }

    return ret != -1 ? ret : QWindowsStyle::pixelMetric(metric, option, widget);
}

/*!
  \reimp
*/
QSize QCleanlooksStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
                                        const QSize &size, const QWidget *widget) const
{
    QSize newSize = QWindowsStyle::sizeFromContents(type, option, size, widget);
    switch (type) {
    case CT_PushButton:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            if (!btn->text.isEmpty() && newSize.width() < 80)
                newSize.setWidth(80);
            if (!btn->icon.isNull() && btn->iconSize.height() > 16)
                newSize -= QSize(0, 2);
            newSize += QSize(0, 1);
        }
        if (const QPushButton *button = qobject_cast<const QPushButton *>(widget)) {
            if (qobject_cast<const QDialogButtonBox *>(button->parentWidget())) {
                if (newSize.height() < 32)
                    newSize.setHeight(32);
            }
        }
        break;
#ifndef QT_NO_GROUPBOX
    case CT_GroupBox:
        // Since we use a bold font we have to recalculate base width
        if (const QGroupBox *gb = qobject_cast<const QGroupBox*>(widget)) {
            QFont font = gb->font();
            font.setBold(true);
            QFontMetrics metrics(font);
            int baseWidth = metrics.width(gb->title()) + metrics.width(QLatin1Char(' '));
            if (gb->isCheckable()) {
                baseWidth += proxy()->pixelMetric(QStyle::PM_IndicatorWidth, option, widget);
                baseWidth += proxy()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, option, widget);
            }
            newSize.setWidth(qMax(baseWidth, newSize.width()));
        }
        newSize += QSize(0, 1);
        break;
#endif //QT_NO_GROUPBOX
    case CT_RadioButton:
    case CT_CheckBox:
        newSize += QSize(0, 1);
        break;
    case CT_ToolButton:
#ifndef QT_NO_TOOLBAR
        if (widget && qobject_cast<QToolBar *>(widget->parentWidget()))
            newSize += QSize(4, 6);
#endif // QT_NO_TOOLBAR
        break;
    case CT_SpinBox:
        newSize += QSize(0, -2);
        break;
    case CT_ComboBox:
        newSize += QSize(2, 4);
        break;
    case CT_LineEdit:
        newSize += QSize(0, 4);
        break;
    case CT_MenuBarItem:
	    newSize += QSize(0, 2);
	    break;
    case CT_MenuItem:
        if (const QStyleOptionMenuItem *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            if (menuItem->menuItemType == QStyleOptionMenuItem::Separator) {
                if (!menuItem->text.isEmpty()) {
                    newSize.setHeight(menuItem->fontMetrics.height());
                }
            }
#ifndef QT_NO_COMBOBOX
            else if (!menuItem->icon.isNull()) {
                if (const QComboBox *combo = qobject_cast<const QComboBox*>(widget)) {
                    newSize.setHeight(qMax(combo->iconSize().height() + 2, newSize.height()));
                }
            }
#endif // QT_NO_COMBOBOX
        }
        break;
    case CT_SizeGrip:
	    newSize += QSize(4, 4);
	break;
    case CT_MdiControls:
        if (const QStyleOptionComplex *styleOpt = qstyleoption_cast<const QStyleOptionComplex *>(option)) {
            int width = 0;
            if (styleOpt->subControls & SC_MdiMinButton)
                width += 19 + 1;
            if (styleOpt->subControls & SC_MdiNormalButton)
                width += 19 + 1;
            if (styleOpt->subControls & SC_MdiCloseButton)
                width += 19 + 1;
            newSize = QSize(width, 19);
        } else {
            newSize = QSize(60, 19);
        }
        break;
    default:
        break;
    }
    return newSize;
}

/*!
  \reimp
*/
void QCleanlooksStyle::polish(QApplication *app)
{
    QWindowsStyle::polish(app);
}

/*!
  \reimp
*/
void QCleanlooksStyle::polish(QWidget *widget)
{
    QWindowsStyle::polish(widget);
    if (qobject_cast<QAbstractButton*>(widget)
#ifndef QT_NO_COMBOBOX
        || qobject_cast<QComboBox *>(widget)
#endif
#ifndef QT_NO_PROGRESSBAR
        || qobject_cast<QProgressBar *>(widget)
#endif
#ifndef QT_NO_SCROLLBAR
        || qobject_cast<QScrollBar *>(widget)
#endif
#ifndef QT_NO_SPLITTER
        || qobject_cast<QSplitterHandle *>(widget)
#endif
        || qobject_cast<QAbstractSlider *>(widget)
#ifndef QT_NO_SPINBOX
        || qobject_cast<QAbstractSpinBox *>(widget)
#endif
        || (widget->inherits("QWorkspaceChild"))
        || (widget->inherits("QDockSeparator"))
        || (widget->inherits("QDockWidgetSeparator"))
        ) {
        widget->setAttribute(Qt::WA_Hover, true);
    }
}

/*!
  \reimp
*/
void QCleanlooksStyle::polish(QPalette &pal)
{
    QWindowsStyle::polish(pal);
    //this is a workaround for some themes such as Human, where the contrast
    //between text and background is too low.
    QColor highlight = pal.highlight().color();
    QColor highlightText = pal.highlightedText().color();
    if (qAbs(qGray(highlight.rgb()) - qGray(highlightText.rgb())) < 150) {
        if (qGray(highlightText.rgb()) < 128)
            pal.setBrush(QPalette::Highlight, highlight.lighter(145));
    }
}

/*!
  \reimp
*/
void QCleanlooksStyle::unpolish(QWidget *widget)
{
    QWindowsStyle::unpolish(widget);
    if (qobject_cast<QAbstractButton*>(widget)
#ifndef QT_NO_COMBOBOX
        || qobject_cast<QComboBox *>(widget)
#endif
#ifndef QT_NO_PROGRESSBAR
        || qobject_cast<QProgressBar *>(widget)
#endif
#ifndef QT_NO_SCROLLBAR
        || qobject_cast<QScrollBar *>(widget)
#endif
#ifndef QT_NO_SPLITTER
        || qobject_cast<QSplitterHandle *>(widget)
#endif
        || qobject_cast<QAbstractSlider *>(widget)
#ifndef QT_NO_SPINBOX
        || qobject_cast<QAbstractSpinBox *>(widget)
#endif
        || (widget->inherits("QWorkspaceChild"))
        || (widget->inherits("QDockSeparator"))
        || (widget->inherits("QDockWidgetSeparator"))
        ) {
        widget->setAttribute(Qt::WA_Hover, false);
    }
}

/*!
  \reimp
*/
void QCleanlooksStyle::unpolish(QApplication *app)
{
    QWindowsStyle::unpolish(app);
}

/*!
  \reimp
*/
QRect QCleanlooksStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                                       SubControl subControl, const QWidget *widget) const
{
    QRect rect = QWindowsStyle::subControlRect(control, option, subControl, widget);

    switch (control) {
#ifndef QT_NO_SLIDER
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            int tickSize = proxy()->pixelMetric(PM_SliderTickmarkOffset, option, widget);
            switch (subControl) {
            case SC_SliderHandle: {
                if (slider->orientation == Qt::Horizontal) {
                    rect.setHeight(proxy()->pixelMetric(PM_SliderThickness));
                    rect.setWidth(proxy()->pixelMetric(PM_SliderLength));
                    int centerY = slider->rect.center().y() - rect.height() / 2;
                    if (slider->tickPosition & QSlider::TicksAbove)
                        centerY += tickSize;
                    if (slider->tickPosition & QSlider::TicksBelow)
                        centerY -= tickSize;
                    rect.moveTop(centerY);
                } else {
                    rect.setWidth(proxy()->pixelMetric(PM_SliderThickness));
                    rect.setHeight(proxy()->pixelMetric(PM_SliderLength));
                    int centerX = slider->rect.center().x() - rect.width() / 2;
                    if (slider->tickPosition & QSlider::TicksAbove)
                        centerX += tickSize;
                    if (slider->tickPosition & QSlider::TicksBelow)
                        centerX -= tickSize;
                    rect.moveLeft(centerX);
                }
            }
                break;
            case SC_SliderGroove: {
                QPoint grooveCenter = slider->rect.center();
                if (slider->orientation == Qt::Horizontal) {
                    rect.setHeight(7);
                    if (slider->tickPosition & QSlider::TicksAbove)
                        grooveCenter.ry() += tickSize;
                    if (slider->tickPosition & QSlider::TicksBelow)
                        grooveCenter.ry() -= tickSize;
                } else {
                    rect.setWidth(7);
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
    case CC_ScrollBar:
        break;
#ifndef QT_NO_SPINBOX
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spinbox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            QSize bs;
            int center = spinbox->rect.height() / 2;
            int fw = spinbox->frame ? proxy()->pixelMetric(PM_SpinBoxFrameWidth, spinbox, widget) : 0;
            int y = fw;
            bs.setHeight(qMax(8, spinbox->rect.height()/2 - y));
            bs.setWidth(15);
            int x, lx, rx;
            x = spinbox->rect.width() - y - bs.width() + 2;
            lx = fw;
            rx = x - fw;
            switch (subControl) {
            case SC_SpinBoxUp:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();
                rect = QRect(x, fw, bs.width(), center - fw);
                break;
            case SC_SpinBoxDown:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();

                rect = QRect(x, center, bs.width(), spinbox->rect.bottom() - center - fw + 1);
                break;
            case SC_SpinBoxEditField:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons) {
                    rect = QRect(lx, fw, spinbox->rect.width() - 2*fw, spinbox->rect.height() - 2*fw);
                } else {
                    rect = QRect(lx, fw, rx - qMax(fw - 1, 0), spinbox->rect.height() - 2*fw);
                }
                break;
            case SC_SpinBoxFrame:
                rect = spinbox->rect;
            default:
                break;
            }
            rect = visualRect(spinbox->direction, spinbox->rect, rect);
        }
        break;
#endif // Qt_NO_SPINBOX
#ifndef QT_NO_GROUPBOX
    case CC_GroupBox:
        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            int topMargin = 0;
            int topHeight = 0;
            int verticalAlignment = proxy()->styleHint(SH_GroupBox_TextLabelVerticalAlignment, groupBox, widget);
            bool flat = groupBox->features & QStyleOptionFrameV2::Flat;
            if (!groupBox->text.isEmpty()) {
                topHeight = groupBox->fontMetrics.height();
                if (verticalAlignment & Qt::AlignVCenter)
                    topMargin = topHeight / 2;
                else if (verticalAlignment & Qt::AlignTop)
                    topMargin = topHeight;
            }
            QRect frameRect = groupBox->rect;
            frameRect.setTop(topMargin);
            if (subControl == SC_GroupBoxFrame) {
                return rect;
            }
            else if (subControl == SC_GroupBoxContents) {
                if( flat ) {
                    int margin = 0;
                    int leftMarginExtension = 16;
                    rect = frameRect.adjusted(leftMarginExtension + margin, margin + topHeight, -margin, -margin);
                }
                break;
            }
            if(flat) {
                if (const QGroupBox *groupBoxWidget = qobject_cast<const QGroupBox *>(widget)) {
                    //Prepare metrics for a bold font
                    QFont font = widget->font();
                    font.setBold(true);
                    QFontMetrics fontMetrics(font);

                    QSize textRect = fontMetrics.boundingRect(groupBoxWidget->title()).size() + QSize(2, 2);
                    if (subControl == SC_GroupBoxCheckBox) {
                        int indicatorWidth = proxy()->pixelMetric(PM_IndicatorWidth, option, widget);
                        int indicatorHeight = proxy()->pixelMetric(PM_IndicatorHeight, option, widget);
                        rect.setWidth(indicatorWidth);
                        rect.setHeight(indicatorHeight);
                        rect.moveTop((fontMetrics.height() - indicatorHeight) / 2 + 2);
                    } else if (subControl == SC_GroupBoxLabel) {
                        rect.setSize(textRect);
                    }
                }
            }
        }
        return rect;
#ifndef QT_NO_COMBOBOX
    case CC_ComboBox:
        switch (subControl) {
        case SC_ComboBoxArrow:
            rect = visualRect(option->direction, option->rect, rect);
            rect.setRect(rect.right() - 18, rect.top() - 2,
                         19, rect.height() + 4);
            rect = visualRect(option->direction, option->rect, rect);
            break;
        case SC_ComboBoxEditField: {
            int frameWidth = proxy()->pixelMetric(PM_DefaultFrameWidth);
            rect = visualRect(option->direction, option->rect, rect);
            rect.setRect(option->rect.left() + frameWidth, option->rect.top() + frameWidth,
                         option->rect.width() - 19 - 2 * frameWidth,
                         option->rect.height() - 2 * frameWidth);
            if (const QStyleOptionComboBox *box = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
                if (!box->editable) {
                    rect.adjust(2, 0, 0, 0);
                    if (box->state & (State_Sunken | State_On))
                        rect.translate(1, 1);
                }
            }
            rect = visualRect(option->direction, option->rect, rect);
            break;
        }
        default:
            break;
        }
        break;
#endif // QT_NO_COMBOBOX
#endif //QT_NO_GROUPBOX
        case CC_TitleBar:
        if (const QStyleOptionTitleBar *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(option)) {
            SubControl sc = subControl;
            QRect &ret = rect;
            const int indent = 3;
            const int controlTopMargin = 3;
            const int controlBottomMargin = 3;
            const int controlWidthMargin = 2;
            const int controlHeight = tb->rect.height() - controlTopMargin - controlBottomMargin ;
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
QRect QCleanlooksStyle::itemPixmapRect(const QRect &r, int flags, const QPixmap &pixmap) const
{
    return QWindowsStyle::itemPixmapRect(r, flags, pixmap);
}

/*!
  \reimp
*/
void QCleanlooksStyle::drawItemPixmap(QPainter *painter, const QRect &rect,
                            int alignment, const QPixmap &pixmap) const
{
    QWindowsStyle::drawItemPixmap(painter, rect, alignment, pixmap);
}

/*!
  \reimp
*/
QStyle::SubControl QCleanlooksStyle::hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                              const QPoint &pt, const QWidget *w) const
{
    return QWindowsStyle::hitTestComplexControl(cc, opt, pt, w);
}

/*!
  \reimp
*/
QPixmap QCleanlooksStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
                                        const QStyleOption *opt) const
{
    return QWindowsStyle::generatedIconPixmap(iconMode, pixmap, opt);
}

/*!
  \reimp
*/
int QCleanlooksStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                               QStyleHintReturn *returnData) const
{
    int ret = 0;
    switch (hint) {
    case SH_ScrollBar_MiddleClickAbsolutePosition:
        ret = true;
        break;
    case SH_EtchDisabledText:
        ret = 1;
        break;
    case SH_Menu_AllowActiveAndDisabled:
        ret = false;
        break;
    case SH_MainWindow_SpaceBelowMenuBar:
        ret = 0;
        break;
    case SH_MenuBar_MouseTracking:
        ret = 1;
        break;
    case SH_TitleBar_AutoRaise:
        ret = 1;
        break;
    case SH_TitleBar_NoBorder:
        ret = 1;
        break;
    case SH_ItemView_ShowDecorationSelected:
        ret = true;
        break;
    case SH_Table_GridLineColor:
        if (option) {
            ret = option->palette.background().color().darker(120).rgb();
            break;
        }
    case SH_ComboBox_Popup:
#ifdef QT3_SUPPORT
        if (widget && widget->inherits("Q3ComboBox"))
            return 0;
#endif
        if (const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(option))
            ret = !cmb->editable;
        else
            ret = 0;
        break;
    case SH_WindowFrame_Mask:
        ret = 1;
        if (QStyleHintReturnMask *mask = qstyleoption_cast<QStyleHintReturnMask *>(returnData)) {
            //left rounded corner
            mask->region = option->rect;
            mask->region -= QRect(option->rect.left(), option->rect.top(), 5, 1);
            mask->region -= QRect(option->rect.left(), option->rect.top() + 1, 3, 1);
            mask->region -= QRect(option->rect.left(), option->rect.top() + 2, 2, 1);
            mask->region -= QRect(option->rect.left(), option->rect.top() + 3, 1, 2);

            //right rounded corner
            mask->region -= QRect(option->rect.right() - 4, option->rect.top(), 5, 1);
            mask->region -= QRect(option->rect.right() - 2, option->rect.top() + 1, 3, 1);
            mask->region -= QRect(option->rect.right() - 1, option->rect.top() + 2, 2, 1);
            mask->region -= QRect(option->rect.right() , option->rect.top() + 3, 1, 2);
        }
        break;
    case SH_MessageBox_TextInteractionFlags:
        ret = Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse;
        break;
    case SH_DialogButtonBox_ButtonsHaveIcons:
        ret = true;
        break;
    case SH_MessageBox_CenterButtons:
        ret = false;
        break;
#ifndef QT_NO_WIZARD
    case SH_WizardStyle:
        ret = QWizard::ClassicStyle;
        break;
#endif
    case SH_ItemView_ArrowKeysNavigateIntoChildren:
        ret = false;
        break;
    case SH_Menu_SubMenuPopupDelay:
        ret = 225; // default from GtkMenu
        break;
    default:
        ret = QWindowsStyle::styleHint(hint, option, widget, returnData);
        break;
    }
    return ret;
}

/*! \reimp */
QRect QCleanlooksStyle::subElementRect(SubElement sr, const QStyleOption *opt, const QWidget *w) const
{
    QRect r = QWindowsStyle::subElementRect(sr, opt, w);
    switch (sr) {
    case SE_PushButtonFocusRect:
        r.adjust(0, 1, 0, -1);
        break;
    case SE_DockWidgetTitleBarText: {
        const QStyleOptionDockWidgetV2 *v2
            = qstyleoption_cast<const QStyleOptionDockWidgetV2*>(opt);
        bool verticalTitleBar = v2 == 0 ? false : v2->verticalTitleBar;
        if (verticalTitleBar) {
            r.adjust(0, 0, 0, -4);
        } else {
            if (opt->direction == Qt::LeftToRight)
                r.adjust(4, 0, 0, 0);
            else
                r.adjust(0, 0, -4, 0);
        }

        break;
    }
    case SE_ProgressBarContents:
        r = subElementRect(SE_ProgressBarGroove, opt, w);
        break;
    default:
        break;
    }
    return r;
}

/*!
    \internal
*/
QIcon QCleanlooksStyle::standardIconImplementation(StandardPixmap standardIcon,
                                                  const QStyleOption *option,
                                                  const QWidget *widget) const
{
    return QWindowsStyle::standardIconImplementation(standardIcon, option, widget);
}

/*!
 \reimp
 */
QPixmap QCleanlooksStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt,
                                      const QWidget *widget) const
{
    QPixmap pixmap;

#ifndef QT_NO_IMAGEFORMAT_XPM
    switch (standardPixmap) {
    case SP_TitleBarNormalButton:
        return QPixmap((const char **)dock_widget_restore_xpm);
    case SP_TitleBarMinButton:
        return QPixmap((const char **)workspace_minimize);
    case SP_TitleBarCloseButton:
    case SP_DockWidgetCloseButton:
        return QPixmap((const char **)dock_widget_close_xpm);

    default:
        break;
    }
#endif //QT_NO_IMAGEFORMAT_XPM

    return QWindowsStyle::standardPixmap(standardPixmap, opt, widget);
}

QT_END_NAMESPACE

#endif // QT_NO_STYLE_CLEANLOOKS || QT_PLUGIN
