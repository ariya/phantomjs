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
#include "qgtkstyle_p.h"

#if !defined(QT_NO_STYLE_GTK)

#include <private/qapplication_p.h>
#include <QtCore/QLibrary>
#include <QtCore/QSettings>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QWidget>
#include <QtWidgets/QListView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleOption>
#include <QtWidgets/QPushButton>
#include <QtGui/QPainter>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QStyledItemDelegate>
#include <QtWidgets/QWizard>

#include <qpixmapcache.h>
#include <private/qstyleanimation_p.h>
#undef signals // Collides with GTK stymbols
#include <private/qgtkpainter_p.h>
#include <private/qstylehelper_p.h>
#include <private/qgtkstyle_p_p.h>

QT_BEGIN_NAMESPACE

static GtkStateType qt_gtk_state(const QStyleOption *option)
{
    GtkStateType state = GTK_STATE_NORMAL;
    if (!(option->state & QStyle::State_Enabled))
        state = GTK_STATE_INSENSITIVE;
    else if (option->state & QStyle::State_MouseOver)
        state = GTK_STATE_PRELIGHT;

    return state;
}

static QPixmap qt_gtk_get_icon(const char* iconName, GtkIconSize size = GTK_ICON_SIZE_BUTTON)
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

static void qt_gtk_draw_mdibutton(QPainter *painter, const QStyleOptionTitleBar *option, const QRect &tmp, bool hover, bool sunken)
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

static const char * const dock_widget_close_xpm[] =
    {
        "11 13 5 1",
        "  c None",
        ". c #D5CFCB",
        "+ c #6C6A67",
        "@ c #6C6A67",
        "$ c #B5B0AC",
        "           ",
        " @@@@@@@@@ ",
        "@+       +@",
        "@ +@   @+ @",
        "@ @@@ @@@ @",
        "@  @@@@@  @",
        "@   @@@   @",
        "@  @@@@@  @",
        "@ @@@ @@@ @",
        "@ +@   @+ @",
        "@+       +@",
        " @@@@@@@@@ ",
        "           "
    };

static const char * const dock_widget_restore_xpm[] =
    {
        "11 13 5 1",
        "  c None",
        ". c #D5CFCB",
        "+ c #6C6A67",
        "@ c #6C6A67",
        "# c #6C6A67",
        "           ",
        " @@@@@@@@@ ",
        "@+       +@",
        "@   #@@@# @",
        "@   @   @ @",
        "@ #@@@# @ @",
        "@ @   @ @ @",
        "@ @   @@@ @",
        "@ @   @   @",
        "@ #@@@@   @",
        "@+       +@",
        " @@@@@@@@@ ",
        "           "
    };

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

static const int groupBoxBottomMargin    =  2;  // space below the groupbox
static const int groupBoxTitleMargin     =  6;  // space between contents and title
static const int groupBoxTopMargin       =  2;

/*!
  Returns the configuration string for \a value.
  Returns \a fallback if \a value is not found.
 */
QString QGtkStyle::getGConfString(const QString &value, const QString &fallback)
{
    return QGtkStylePrivate::getGConfString(value, fallback);
}

/*!
  Returns the configuration boolean for \a key.
  Returns \a fallback if \a key is not found.
 */
bool QGtkStyle::getGConfBool(const QString &key, bool fallback)
{
    return QGtkStylePrivate::getGConfBool(key, fallback);
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

static GdkColor fromQColor(const QColor &color)
{
    GdkColor retval;
    retval.red = color.red() * 255;
    retval.green = color.green() * 255;
    retval.blue = color.blue() * 255;
    return retval;
}

/*!
    \class QGtkStyle
    \brief The QGtkStyle class provides a widget style rendered by GTK+
    \since 4.5

    \internal
    \inmodule QtWidgets

    The QGtkStyle style provides a look and feel that integrates well
    into GTK-based desktop environments such as the XFCe and GNOME.

    It does this by making use of the GTK+ theme engine, ensuring
    that Qt applications look and feel native on these platforms.

    Note: The style requires GTK+ version 2.18 or later.
          The Qt3-based "Qt" GTK+ theme engine will not work with QGtkStyle.

    \sa QWindowsXPStyle, QMacStyle, QWindowsStyle, QFusionStyle
*/

/*!
    Constructs a QGtkStyle object.
*/
QGtkStyle::QGtkStyle()
    : QCommonStyle(*new QGtkStylePrivate)
{
    Q_D(QGtkStyle);
    d->init();
}

/*!
    \internal

    Constructs a QGtkStyle object.
*/
QGtkStyle::QGtkStyle(QGtkStylePrivate &dd)
     : QCommonStyle(dd)
{
    Q_D(QGtkStyle);
    d->init();
}


/*!
    Destroys the QGtkStyle object.
*/
QGtkStyle::~QGtkStyle()
{
}

/*!
    \reimp
*/
QPalette QGtkStyle::standardPalette() const
{
    Q_D(const QGtkStyle);

    QPalette palette = QCommonStyle::standardPalette();
    if (d->isThemeAvailable()) {
        GtkStyle *style = d->gtkStyle();
        GtkWidget *gtkButton = d->gtkWidget("GtkButton");
        GtkWidget *gtkEntry = d->getTextColorWidget();
        GdkColor gdkBg, gdkBase, gdkText, gdkForeground, gdkSbg, gdkSfg, gdkaSbg, gdkaSfg;
        QColor bg, base, text, fg, highlight, highlightText, inactiveHighlight, inactiveHighlightedTExt;
        gdkBg = style->bg[GTK_STATE_NORMAL];
        gdkForeground = d->gtk_widget_get_style(gtkButton)->fg[GTK_STATE_NORMAL];

        // Our base and selected color is primarily used for text
        // so we assume a gtkEntry will have the most correct value
        GtkStyle *gtkEntryStyle = d->gtk_widget_get_style(gtkEntry);
        gdkBase = gtkEntryStyle->base[GTK_STATE_NORMAL];
        gdkText = gtkEntryStyle->text[GTK_STATE_NORMAL];
        gdkSbg = gtkEntryStyle->base[GTK_STATE_SELECTED];
        gdkSfg = gtkEntryStyle->text[GTK_STATE_SELECTED];

        // The ACTIVE base color is really used for inactive windows
        gdkaSbg = gtkEntryStyle->base[GTK_STATE_ACTIVE];
        gdkaSfg = gtkEntryStyle->text[GTK_STATE_ACTIVE];

        bg = QColor(gdkBg.red>>8, gdkBg.green>>8, gdkBg.blue>>8);
        text = QColor(gdkText.red>>8, gdkText.green>>8, gdkText.blue>>8);
        fg = QColor(gdkForeground.red>>8, gdkForeground.green>>8, gdkForeground.blue>>8);
        base = QColor(gdkBase.red>>8, gdkBase.green>>8, gdkBase.blue>>8);
        highlight = QColor(gdkSbg.red>>8, gdkSbg.green>>8, gdkSbg.blue>>8);
        highlightText = QColor(gdkSfg.red>>8, gdkSfg.green>>8, gdkSfg.blue>>8);
        inactiveHighlight = QColor(gdkaSbg.red>>8, gdkaSbg.green>>8, gdkaSbg.blue>>8);
        inactiveHighlightedTExt = QColor(gdkaSfg.red>>8, gdkaSfg.green>>8, gdkaSfg.blue>>8);

        palette.setColor(QPalette::HighlightedText, highlightText);


        palette.setColor(QPalette::Light, bg.lighter(125));
        palette.setColor(QPalette::Shadow, bg.darker(130));
        palette.setColor(QPalette::Dark, bg.darker(120));
        palette.setColor(QPalette::Text, text);
        palette.setColor(QPalette::WindowText, fg);
        palette.setColor(QPalette::ButtonText, fg);
        palette.setColor(QPalette::Base, base);

        QColor alternateRowColor = palette.base().color().lighter(93); // ref gtkstyle.c draw_flat_box
        GtkWidget *gtkTreeView = d->gtkWidget("GtkTreeView");
        GdkColor *gtkAltBase = NULL;
        d->gtk_widget_style_get(gtkTreeView, "odd-row-color", &gtkAltBase, NULL);
        if (gtkAltBase) {
            alternateRowColor = QColor(gtkAltBase->red>>8, gtkAltBase->green>>8, gtkAltBase->blue>>8);
            d->gdk_color_free(gtkAltBase);
        }
        palette.setColor(QPalette::AlternateBase, alternateRowColor);

        palette.setColor(QPalette::Window, bg);
        palette.setColor(QPalette::Button, bg);
        palette.setColor(QPalette::Background, bg);
        QColor disabled((fg.red()   + bg.red())  / 2,
                        (fg.green() + bg.green())/ 2,
                        (fg.blue()  + bg.blue()) / 2);
        palette.setColor(QPalette::Disabled, QPalette::Text, disabled);
        palette.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
        palette.setColor(QPalette::Disabled, QPalette::Foreground, disabled);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
        palette.setColor(QPalette::Highlight, highlight);
        // calculate disabled colors by removing saturation
        highlight.setHsv(highlight.hue(), 0, highlight.value(), highlight.alpha());
        highlightText.setHsv(highlightText.hue(), 0, highlightText.value(), highlightText.alpha());
        palette.setColor(QPalette::Disabled, QPalette::Highlight, highlight);
        palette.setColor(QPalette::Disabled, QPalette::HighlightedText, highlightText);

        palette.setColor(QPalette::Inactive, QPalette::HighlightedText, inactiveHighlightedTExt);
        palette.setColor(QPalette::Inactive, QPalette::Highlight, inactiveHighlight);

        style = d->gtk_rc_get_style_by_paths(d->gtk_settings_get_default(), "gtk-tooltips", "GtkWindow",
                d->gtk_window_get_type());
        if (style) {
            gdkText = style->fg[GTK_STATE_NORMAL];
            text = QColor(gdkText.red>>8, gdkText.green>>8, gdkText.blue>>8);
            palette.setColor(QPalette::ToolTipText, text);
        }
    }
    return palette;
}

/*!
    \reimp
*/
void QGtkStyle::polish(QPalette &palette)
{
    Q_D(QGtkStyle);

    if (!d->isThemeAvailable())
        QCommonStyle::polish(palette);
    else
        palette = palette.resolve(standardPalette());
}

/*!
    \reimp
*/
void QGtkStyle::polish(QApplication *app)
{
    Q_D(QGtkStyle);

    QCommonStyle::polish(app);
    // Custom fonts and palettes with QtConfig are intentionally
    // not supported as these should be entirely determined by
    // current Gtk settings
    if (app->desktopSettingsAware() && d->isThemeAvailable()) {
        QApplicationPrivate::setSystemPalette(standardPalette());
        QApplicationPrivate::setSystemFont(d->getThemeFont());
        d->applyCustomPaletteHash();
        if (!d->isKDE4Session())
            qApp->installEventFilter(&d->filter);
    }
}

/*!
    \reimp
*/
void QGtkStyle::unpolish(QApplication *app)
{
    Q_D(QGtkStyle);

    QCommonStyle::unpolish(app);
    QPixmapCache::clear();

    if (app->desktopSettingsAware() && d->isThemeAvailable() && !d->isKDE4Session())
        qApp->removeEventFilter(&d->filter);
}

/*!
    \reimp
*/

void QGtkStyle::polish(QWidget *widget)
{
    Q_D(QGtkStyle);

    QCommonStyle::polish(widget);
    if (!d->isThemeAvailable())
        return;
    if (qobject_cast<QAbstractButton*>(widget)
            || qobject_cast<QToolButton*>(widget)
            || qobject_cast<QComboBox*>(widget)
            || qobject_cast<QGroupBox*>(widget)
            || qobject_cast<QScrollBar*>(widget)
            || qobject_cast<QSlider*>(widget)
            || qobject_cast<QAbstractSpinBox*>(widget)
            || qobject_cast<QSpinBox*>(widget)
            || qobject_cast<QHeaderView*>(widget))
        widget->setAttribute(Qt::WA_Hover);
#ifndef QT_NO_TREEVIEW
    else if (QTreeView *tree = qobject_cast<QTreeView *> (widget))
        tree->viewport()->setAttribute(Qt::WA_Hover);
#endif
}

/*!
    \reimp
*/
void QGtkStyle::unpolish(QWidget *widget)
{
    QCommonStyle::unpolish(widget);
}

/*!
    \reimp
*/
int QGtkStyle::pixelMetric(PixelMetric metric,
                           const QStyleOption *option,
                           const QWidget *widget) const
{
    Q_D(const QGtkStyle);

    if (!d->isThemeAvailable())
        return QCommonStyle::pixelMetric(metric, option, widget);

    switch (metric) {
    case PM_DefaultFrameWidth:
        if (qobject_cast<const QFrame*>(widget)) {
            if (GtkStyle *style =
                d->gtk_rc_get_style_by_paths(d->gtk_settings_get_default(),
                                                "*.GtkScrolledWindow",
                                                "*.GtkScrolledWindow",
                                                d->gtk_window_get_type()))
                return qMax(style->xthickness, style->ythickness);
        }
        return 2;

    case PM_MenuButtonIndicator:
        return 20;

    case PM_TabBarBaseOverlap:
        return 1;

    case PM_ToolBarSeparatorExtent:
        return 11;

    case PM_ToolBarFrameWidth:
        return 1;

    case PM_ToolBarItemSpacing:
        return 0;

    case PM_ButtonShiftHorizontal: {
        GtkWidget *gtkButton = d->gtkWidget("GtkButton");
        guint horizontal_shift;
        d->gtk_widget_style_get(gtkButton, "child-displacement-x", &horizontal_shift, NULL);
        return horizontal_shift;
    }

    case PM_ButtonShiftVertical: {
        GtkWidget *gtkButton = d->gtkWidget("GtkButton");
        guint vertical_shift;
        d->gtk_widget_style_get(gtkButton, "child-displacement-y", &vertical_shift, NULL);
        return vertical_shift;
    }

    case PM_MenuBarPanelWidth:
        return 0;

    case PM_MenuPanelWidth: {
        GtkWidget *gtkMenu = d->gtkWidget("GtkMenu");
        guint horizontal_padding = 0;
        // horizontal-padding is used by Maemo to get thicker borders
        if (!d->gtk_check_version(2, 10, 0))
            d->gtk_widget_style_get(gtkMenu, "horizontal-padding", &horizontal_padding, NULL);
        int padding = qMax<int>(d->gtk_widget_get_style(gtkMenu)->xthickness, horizontal_padding);
        return padding;
    }

    case PM_ButtonIconSize: {
        int retVal = 24;
        GtkSettings *settings = d->gtk_settings_get_default();
        gchararray icon_sizes;
        g_object_get(settings, "gtk-icon-sizes", &icon_sizes, NULL);
        QStringList values = QString(QLS(icon_sizes)).split(QLatin1Char(':'));
        g_free(icon_sizes);
        QChar splitChar(QLatin1Char(','));
        foreach (const QString &value, values) {
            if (value.startsWith(QLS("gtk-button="))) {
                QString iconSize = value.right(value.size() - 11);

                if (iconSize.contains(splitChar))
                    retVal = iconSize.split(splitChar)[0].toInt();
                break;
            }
        }
        return retVal;
    }

    case PM_MenuVMargin:

    case PM_MenuHMargin:
        return 0;

    case PM_DockWidgetTitleMargin:
        return 0;

    case PM_DockWidgetTitleBarButtonMargin:
        return 5;

    case PM_TabBarTabVSpace:
        return 12;

    case PM_TabBarTabHSpace:
        return 14;

    case PM_TabBarTabShiftVertical:
        return 2;

    case PM_ToolBarHandleExtent:
        return 9;

    case PM_SplitterWidth:
        return 6;

    case PM_SliderThickness:
    case PM_SliderControlThickness: {
        GtkWidget *gtkScale = d->gtkWidget("GtkHScale");
        gint val;
        d->gtk_widget_style_get(gtkScale, "slider-width", &val, NULL);
        if (metric == PM_SliderControlThickness)
            return val + 2*d->gtk_widget_get_style(gtkScale)->ythickness;
        return val;
    }

    case PM_ScrollBarExtent: {
        gint sliderLength;
        gint trough_border;
        GtkWidget *hScrollbar = d->gtkWidget("GtkHScrollbar");
        d->gtk_widget_style_get(hScrollbar,
                               "trough-border",   &trough_border,
                               "slider-width",    &sliderLength,
                               NULL);
        return sliderLength + trough_border*2;
    }

    case PM_ScrollBarSliderMin:
        return 34;

    case PM_SliderLength:
        gint val;
        d->gtk_widget_style_get(d->gtkWidget("GtkHScale"), "slider-length", &val, NULL);
        return val;

    case PM_ExclusiveIndicatorWidth:
    case PM_ExclusiveIndicatorHeight:
    case PM_IndicatorWidth:
    case PM_IndicatorHeight: {
        GtkWidget *gtkCheckButton = d->gtkWidget("GtkCheckButton");
        gint size, spacing;
        d->gtk_widget_style_get(gtkCheckButton, "indicator-spacing", &spacing, "indicator-size", &size, NULL);
        return size + 2 * spacing;
    }

    case PM_MenuBarVMargin: {
        GtkWidget *gtkMenubar = d->gtkWidget("GtkMenuBar");
        return  qMax(0, d->gtk_widget_get_style(gtkMenubar)->ythickness);
    }
    case PM_ScrollView_ScrollBarSpacing:
    {
        gint spacing = 3;
        GtkWidget *gtkScrollWindow = d->gtkWidget("GtkScrolledWindow");
        Q_ASSERT(gtkScrollWindow);
        d->gtk_widget_style_get(gtkScrollWindow, "scrollbar-spacing", &spacing, NULL);
        return spacing;
    }
    case PM_SubMenuOverlap: {
        gint offset = 0;
        GtkWidget *gtkMenu = d->gtkWidget("GtkMenu");
        d->gtk_widget_style_get(gtkMenu, "horizontal-offset", &offset, NULL);
        return offset;
    }
    case PM_ToolTipLabelFrameWidth:
        return 2;
    case PM_ButtonDefaultIndicator:
        return 0;
    case PM_ListViewIconSize:
        return 24;
    case PM_DialogButtonsSeparator:
        return 6;
    case PM_TitleBarHeight:
        return 24;
    case PM_SpinBoxFrameWidth:
        return 3;
    case PM_MenuBarItemSpacing:
        return 6;
    case PM_MenuBarHMargin:
        return 0;
    case PM_ToolBarItemMargin:
        return 1;
    case PM_SmallIconSize:
        return 16;
    case PM_MaximumDragDistance:
        return -1;
    case PM_TabCloseIndicatorWidth:
    case PM_TabCloseIndicatorHeight:
        return 20;
    default:
        return QCommonStyle::pixelMetric(metric, option, widget);
    }
}

/*!
    \reimp
*/
int QGtkStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,

                         QStyleHintReturn *returnData = 0) const
{
    Q_D(const QGtkStyle);

    if (!d->isThemeAvailable())
        return QCommonStyle::styleHint(hint, option, widget, returnData);

    switch (hint) {
    case SH_ItemView_ChangeHighlightOnFocus:
        return true;
    case SH_ScrollBar_MiddleClickAbsolutePosition:
        return true;
    case SH_Menu_AllowActiveAndDisabled:
        return false;
    case SH_MainWindow_SpaceBelowMenuBar:
        return false;
    case SH_MenuBar_MouseTracking:
        return true;
    case SH_Menu_MouseTracking:
        return true;
    case SH_TitleBar_AutoRaise:
        return true;
    case SH_TitleBar_NoBorder:
        return true;
    case SH_ItemView_ShowDecorationSelected:
        return true;
    case SH_Table_GridLineColor:
        if (option)
            return option->palette.background().color().darker(120).rgb();
        break;
    case SH_WindowFrame_Mask:
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
        return QCommonStyle::styleHint(hint, option, widget, returnData);
    case SH_MessageBox_TextInteractionFlags:
        return Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse;
    case SH_MessageBox_CenterButtons:
        return false;
#ifndef QT_NO_WIZARD
    case SH_WizardStyle:
        return  QWizard::ClassicStyle;
#endif
    case SH_ItemView_ArrowKeysNavigateIntoChildren:
        return false;
    case SH_DialogButtonLayout: {
        int ret = QDialogButtonBox::GnomeLayout;
        gboolean alternateOrder = 0;
        GtkSettings *settings = d->gtk_settings_get_default();
        g_object_get(settings, "gtk-alternative-button-order", &alternateOrder, NULL);

        if (alternateOrder)
            ret = QDialogButtonBox::WinLayout;

        return ret;
    }
    break;

    case SH_ToolButtonStyle:
    {
        if (d->isKDE4Session())
            return QCommonStyle::styleHint(hint, option, widget, returnData);
        GtkWidget *gtkToolbar = d->gtkWidget("GtkToolbar");
        GtkToolbarStyle toolbar_style = GTK_TOOLBAR_ICONS;
        g_object_get(gtkToolbar, "toolbar-style", &toolbar_style, NULL);
        switch (toolbar_style) {
        case GTK_TOOLBAR_TEXT:
            return Qt::ToolButtonTextOnly;
        case GTK_TOOLBAR_BOTH:
            return Qt::ToolButtonTextUnderIcon;
        case GTK_TOOLBAR_BOTH_HORIZ:
            return Qt::ToolButtonTextBesideIcon;
        case GTK_TOOLBAR_ICONS:
        default:
            return Qt::ToolButtonIconOnly;
        }
    }
    break;
    case SH_SpinControls_DisableOnBounds:
        return int(true);

    case SH_DitherDisabledText:
        return int(false);

    case SH_ComboBox_Popup: {
        GtkWidget *gtkComboBox = d->gtkWidget("GtkComboBox");
        gboolean appears_as_list;
        d->gtk_widget_style_get((GtkWidget*)gtkComboBox, "appears-as-list", &appears_as_list, NULL);
        return appears_as_list ? 0 : 1;
    }

    case SH_MenuBar_AltKeyNavigation:
        return int(false);

    case SH_EtchDisabledText:
        return int(false);

    case SH_Menu_SubMenuPopupDelay: {
        gint delay = 225;
        GtkSettings *settings = d->gtk_settings_get_default();
        g_object_get(settings, "gtk-menu-popup-delay", &delay, NULL);
        return delay;
    }

    case SH_ScrollView_FrameOnlyAroundContents: {
        gboolean scrollbars_within_bevel = false;
        if (widget && widget->isWindow())
            scrollbars_within_bevel = true;
        else if (!d->gtk_check_version(2, 12, 0)) {
            GtkWidget *gtkScrollWindow = d->gtkWidget("GtkScrolledWindow");
            d->gtk_widget_style_get(gtkScrollWindow, "scrollbars-within-bevel", &scrollbars_within_bevel, NULL);
        }
        return !scrollbars_within_bevel;
    }

    case SH_DialogButtonBox_ButtonsHaveIcons: {
        static bool buttonsHaveIcons = d->getGConfBool(QLS("/desktop/gnome/interface/buttons_have_icons"));
        return buttonsHaveIcons;
    }

    case SH_UnderlineShortcut: {
        gboolean underlineShortcut = true;
        if (!d->gtk_check_version(2, 12, 0)) {
            GtkSettings *settings = d->gtk_settings_get_default();
            g_object_get(settings, "gtk-enable-mnemonics", &underlineShortcut, NULL);
        }
        return underlineShortcut;
    }

    default:
        break;
    }
    return QCommonStyle::styleHint(hint, option, widget, returnData);
}

/*!
    \reimp
*/
void QGtkStyle::drawPrimitive(PrimitiveElement element,
                              const QStyleOption *option,
                              QPainter *painter,
                              const QWidget *widget) const
{
    Q_D(const QGtkStyle);

    if (!d->isThemeAvailable()) {
        QCommonStyle::drawPrimitive(element, option, painter, widget);
        return;
    }

    GtkStyle* style = d->gtkStyle();
    QGtkPainter* gtkPainter = d->gtkPainter(painter);

    switch (element) {
      case PE_Frame: {
        if (widget && widget->inherits("QComboBoxPrivateContainer")){
            QStyleOption copy = *option;
            copy.state |= State_Raised;
            proxy()->drawPrimitive(PE_PanelMenu, &copy, painter, widget);
            break;
        }
        // Drawing the entire itemview frame is very expensive, especially on the native X11 engine
        // Instead we cheat a bit and draw a border image without the center part, hence only scaling
        // thin rectangular images
        const int pmSize = 64;
        const int border = proxy()->pixelMetric(PM_DefaultFrameWidth, option, widget);
        const QString pmKey = QLatin1String("windowframe") % HexString<uint>(option->state);

        QPixmap pixmap;
        QRect pmRect(QPoint(0,0), QSize(pmSize, pmSize));

        // Only draw through style once
        if (!QPixmapCache::find(pmKey, pixmap)) {
            pixmap = QPixmap(pmSize, pmSize);
            pixmap.fill(Qt::transparent);
            QPainter pmPainter(&pixmap);
            gtkPainter->reset(&pmPainter);
            gtkPainter->setUsePixmapCache(false); // Don't cache twice

            GtkShadowType shadow_type = GTK_SHADOW_NONE;
            if (option->state & State_Sunken)
                shadow_type = GTK_SHADOW_IN;
            else if (option->state & State_Raised)
                shadow_type = GTK_SHADOW_OUT;

            GtkStyle *style = d->gtk_rc_get_style_by_paths(d->gtk_settings_get_default(),
                                     "*.GtkScrolledWindow", "*.GtkScrolledWindow", d->gtk_window_get_type());
            if (style)
                gtkPainter->paintShadow(d->gtkWidget("GtkFrame"), "viewport", pmRect,
                                        option->state & State_Enabled ? GTK_STATE_NORMAL : GTK_STATE_INSENSITIVE,
                                        shadow_type, style);
            QPixmapCache::insert(pmKey, pixmap);
            gtkPainter->reset(painter);
        }

        QRect rect = option->rect;
        const int rw = rect.width() - border;
        const int rh = rect.height() - border;
        const int pw = pmRect.width() - border;
        const int ph = pmRect.height() - border;

        // Sidelines
        painter->drawPixmap(rect.adjusted(border, 0, -border, -rh), pixmap, pmRect.adjusted(border, 0, -border,-ph));
        painter->drawPixmap(rect.adjusted(border, rh, -border, 0), pixmap, pmRect.adjusted(border, ph,-border,0));
        painter->drawPixmap(rect.adjusted(0, border, -rw, -border), pixmap, pmRect.adjusted(0, border, -pw, -border));
        painter->drawPixmap(rect.adjusted(rw, border, 0, -border), pixmap, pmRect.adjusted(pw, border, 0, -border));

        // Corners
        painter->drawPixmap(rect.adjusted(0, 0, -rw, -rh), pixmap, pmRect.adjusted(0, 0, -pw,-ph));
        painter->drawPixmap(rect.adjusted(rw, 0, 0, -rh), pixmap, pmRect.adjusted(pw, 0, 0,-ph));
        painter->drawPixmap(rect.adjusted(0, rh, -rw, 0), pixmap, pmRect.adjusted(0, ph, -pw,0));
        painter->drawPixmap(rect.adjusted(rw, rh, 0, 0), pixmap, pmRect.adjusted(pw, ph, 0,0));
    }
    break;
    case PE_FrameWindow:
        painter->save();
        {
            QRect rect= option->rect;
            painter->setPen(QPen(option->palette.dark().color().darker(150), 0));
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

    case PE_PanelTipLabel: {
        GtkWidget *gtkWindow = d->gtkWidget("GtkWindow"); // The Murrine Engine currently assumes a widget is passed
        style = d->gtk_rc_get_style_by_paths(d->gtk_settings_get_default(), "gtk-tooltips", "GtkWindow",
                d->gtk_window_get_type());
        gtkPainter->paintFlatBox(gtkWindow, "tooltip", option->rect, GTK_STATE_NORMAL, GTK_SHADOW_NONE, style);
    }
    break;

    case PE_PanelStatusBar: {
        if (widget && widget->testAttribute(Qt::WA_SetPalette) &&
            option->palette.resolve() & (1 << QPalette::Window)) {
            // Respect custom palette
            painter->fillRect(option->rect, option->palette.window());
            break;
        }
        GtkShadowType shadow_type;
        GtkWidget *gtkStatusbarFrame = d->gtkWidget("GtkStatusbar.GtkFrame");
        d->gtk_widget_style_get(d->gtk_widget_get_parent(gtkStatusbarFrame), "shadow-type", &shadow_type, NULL);
        gtkPainter->paintShadow(gtkStatusbarFrame, "frame", option->rect, GTK_STATE_NORMAL,
                                shadow_type, d->gtk_widget_get_style(gtkStatusbarFrame));
    }
    break;

    case PE_IndicatorHeaderArrow:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            GtkWidget *gtkTreeHeader = d->gtkWidget("GtkTreeView.GtkButton");
            GtkStateType state = qt_gtk_state(option);
            style = d->gtk_widget_get_style(gtkTreeHeader);
            GtkArrowType type = GTK_ARROW_UP;
            // This sorting indicator inversion is intentional, and follows the GNOME HIG.
            // See http://library.gnome.org/devel/hig-book/stable/controls-lists.html.en#controls-lists-sortable
            if (header->sortIndicator & QStyleOptionHeader::SortUp)
                type = GTK_ARROW_UP;
            else if (header->sortIndicator & QStyleOptionHeader::SortDown)
                type = GTK_ARROW_DOWN;

            gtkPainter->paintArrow(gtkTreeHeader, "button", option->rect.adjusted(1, 1, -1, -1), type, state,
                                   GTK_SHADOW_NONE, false, style);
        }
        break;

    case PE_FrameDefaultButton: // fall through
    case PE_FrameFocusRect: {
            QRect frameRect = option->rect.adjusted(1, 1, -2, -2); // ### this mess should move to subcontrolrect
            if (qobject_cast<const QAbstractItemView*>(widget)) {
                // Don't draw anything
            } else if (qobject_cast<const QTabBar*>(widget)) {
                GtkWidget *gtkNotebook = d->gtkWidget("GtkNotebook");
                style = d->gtk_widget_get_style(gtkNotebook);
                gtkPainter->paintFocus(gtkNotebook, "tab", frameRect.adjusted(-1, 1, 1, 1), GTK_STATE_ACTIVE, style);
            } else {
                GtkWidget *gtkRadioButton = d->gtkWidget("GtkRadioButton");
                gtkPainter->paintFocus(gtkRadioButton, "radiobutton", frameRect, GTK_STATE_ACTIVE, style);
            }
        }
        break;

    case PE_IndicatorBranch:
        if (option->state & State_Children) {
            QRect rect = option->rect;
            rect = QRect(0, 0, 12, 12);
            rect.moveCenter(option->rect.center());
            rect.translate(2, 0);
            GtkExpanderStyle openState = GTK_EXPANDER_EXPANDED;
            GtkExpanderStyle closedState = GTK_EXPANDER_COLLAPSED;
            GtkWidget *gtkTreeView = d->gtkWidget("GtkTreeView");

            GtkStateType state = GTK_STATE_NORMAL;
            if (!(option->state & State_Enabled))
                state = GTK_STATE_INSENSITIVE;
            else if (option->state & State_MouseOver)
                state = GTK_STATE_PRELIGHT;

            gtkPainter->paintExpander(gtkTreeView, "treeview", rect, state,
                                      option->state & State_Open ? openState : closedState , d->gtk_widget_get_style(gtkTreeView));
        }
        break;

    case PE_PanelItemViewRow:
        // This primitive is only used to draw selection behind selected expander arrows.
        // We try not to decorate the tree branch background unless you inherit from StyledItemDelegate
        // The reason for this is that a lot of code that relies on custom item delegates will look odd having
        // a gradient on the branch but a flat shaded color on the item itself.
        QCommonStyle::drawPrimitive(element, option, painter, widget);
        if (!option->state & State_Selected) {
            break;
        } else {
            if (const QAbstractItemView *view = qobject_cast<const QAbstractItemView*>(widget)) {
                if (!qobject_cast<QStyledItemDelegate*>(view->itemDelegate()))
                    break;
            }
        } // fall through

    case PE_PanelItemViewItem:
        if (const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
            uint resolve_mask = vopt->palette.resolve();
            if (vopt->backgroundBrush.style() != Qt::NoBrush
                    || (resolve_mask & (1 << QPalette::Base)))
            {
                QPointF oldBO = painter->brushOrigin();
                painter->setBrushOrigin(vopt->rect.topLeft());
                painter->fillRect(vopt->rect, vopt->backgroundBrush);
                painter->setBrushOrigin(oldBO);
                if (!(option->state & State_Selected))
                    break;
            }
            if (GtkWidget *gtkTreeView = d->gtkWidget("GtkTreeView")) {
                const char *detail = "cell_even_ruled";
                if (vopt && vopt->features & QStyleOptionViewItem::Alternate)
                    detail = "cell_odd_ruled";
                bool isActive = option->state & State_Active;
                QString key;
                if (isActive ) {
                    // Required for active/non-active window appearance
                    key = QLS("a");
                    QGtkStylePrivate::gtkWidgetSetFocus(gtkTreeView, true);
                }
                bool isEnabled = (widget ? widget->isEnabled() : (vopt->state & QStyle::State_Enabled));
                gtkPainter->paintFlatBox(gtkTreeView, detail, option->rect,
                                         option->state & State_Selected ? GTK_STATE_SELECTED :
                                         isEnabled ? GTK_STATE_NORMAL : GTK_STATE_INSENSITIVE,
                                         GTK_SHADOW_OUT, d->gtk_widget_get_style(gtkTreeView), key);
                if (isActive )
                    QGtkStylePrivate::gtkWidgetSetFocus(gtkTreeView, false);
            }
        }
        break;
    case PE_IndicatorToolBarSeparator:
        {
            const int margin = 6;
            GtkWidget *gtkSeparator = d->gtkWidget("GtkToolbar.GtkSeparatorToolItem");
            if (option->state & State_Horizontal) {
                const int offset = option->rect.width()/2;
                QRect rect = option->rect.adjusted(offset, margin, 0, -margin);
                painter->setPen(QPen(option->palette.background().color().darker(110)));
                gtkPainter->paintVline(gtkSeparator, "vseparator",
                                       rect, GTK_STATE_NORMAL, d->gtk_widget_get_style(gtkSeparator),
                                       0, rect.height(), 0);
            } else { //Draw vertical separator
                const int offset = option->rect.height()/2;
                QRect rect = option->rect.adjusted(margin, offset, -margin, 0);
                painter->setPen(QPen(option->palette.background().color().darker(110)));
                gtkPainter->paintHline(gtkSeparator, "hseparator",
                                       rect, GTK_STATE_NORMAL, d->gtk_widget_get_style(gtkSeparator),
                                       0, rect.width(), 0);
            }
       }
       break;

    case PE_IndicatorToolBarHandle: {
        GtkWidget *gtkToolbar = d->gtkWidget("GtkToolbar");
        GtkShadowType shadow_type;
        d->gtk_widget_style_get(gtkToolbar, "shadow-type", &shadow_type, NULL);
        //Note when the toolbar is horizontal, the handle is vertical
        painter->setClipRect(option->rect);
        gtkPainter->paintHandle(gtkToolbar, "toolbar", option->rect.adjusted(-1, -1 ,0 ,1),
                                GTK_STATE_NORMAL, shadow_type, !(option->state & State_Horizontal) ?
                                GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL, d->gtk_widget_get_style(gtkToolbar));
    }
    break;

    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowLeft:
    case PE_IndicatorArrowRight: {


        GtkArrowType type = GTK_ARROW_UP;

        switch (element) {

        case PE_IndicatorArrowDown:
            type = GTK_ARROW_DOWN;
            break;

        case PE_IndicatorArrowLeft:
            type = GTK_ARROW_LEFT;
            break;

        case PE_IndicatorArrowRight:
            type = GTK_ARROW_RIGHT;
            break;

        default:
            break;
        }
        int size = qMin(option->rect.height(), option->rect.width());
        int border = (size > 9) ? (size/4) : 0; //Allow small arrows to have exact dimensions
        int bsx = 0, bsy = 0;
        if (option->state & State_Sunken) {
            bsx = proxy()->pixelMetric(PM_ButtonShiftHorizontal);
            bsy = proxy()->pixelMetric(PM_ButtonShiftVertical);
        }
        QRect arrowRect = option->rect.adjusted(border + bsx, border + bsy, -border + bsx, -border + bsy);
        GtkShadowType shadow = option->state & State_Sunken ? GTK_SHADOW_IN : GTK_SHADOW_OUT;
        GtkStateType state = qt_gtk_state(option);

        QColor arrowColor = option->palette.buttonText().color();
        GtkWidget *gtkArrow = d->gtkWidget("GtkArrow");
        GdkColor color = fromQColor(arrowColor);
        d->gtk_widget_modify_fg (gtkArrow, state, &color);
        gtkPainter->paintArrow(gtkArrow, "button", arrowRect,
                               type, state, shadow, false, d->gtk_widget_get_style(gtkArrow),
                               QString::number(arrowColor.rgba(), 16));
        // Passing NULL will revert the color change
        d->gtk_widget_modify_fg (gtkArrow, state, NULL);
    }
    break;

    case PE_FrameGroupBox:
        // Do nothing here, the GNOME groupboxes are flat
        break;

    case PE_PanelMenu: {
            GtkWidget *gtkMenu = d->gtkWidget("GtkMenu");
            gtkPainter->setAlphaSupport(false); // Note, alpha disabled for performance reasons
            gtkPainter->paintBox(gtkMenu, "menu", option->rect, GTK_STATE_NORMAL, GTK_SHADOW_OUT, d->gtk_widget_get_style(gtkMenu), QString());
        }
        break;

    case PE_FrameMenu:
        //This is actually done by PE_Widget due to a clipping issue
        //Otherwise Menu items will not be able to span the entire menu width

        // This is only used by floating tool bars
        if (qobject_cast<const QToolBar *>(widget)) {
            GtkWidget *gtkMenubar = d->gtkWidget("GtkMenuBar");
            gtkPainter->paintBox(gtkMenubar, "toolbar", option->rect,
                                 GTK_STATE_NORMAL, GTK_SHADOW_OUT, style);
            gtkPainter->paintBox(gtkMenubar, "menu", option->rect,
                                 GTK_STATE_NORMAL, GTK_SHADOW_OUT, style);
        }
        break;

    case PE_FrameLineEdit: {
        GtkWidget *gtkEntry = d->gtkWidget("GtkEntry");


        gboolean interior_focus;
        gint focus_line_width;
        QRect rect = option->rect;
        d->gtk_widget_style_get(gtkEntry,
                               "interior-focus", &interior_focus,
                               "focus-line-width", &focus_line_width, NULL);

        // See https://bugzilla.mozilla.org/show_bug.cgi?id=405421 for info about this hack
        g_object_set_data(G_OBJECT(gtkEntry), "transparent-bg-hint", GINT_TO_POINTER(true));

        if (!interior_focus && option->state & State_HasFocus)
            rect.adjust(focus_line_width, focus_line_width, -focus_line_width, -focus_line_width);

        if (option->state & State_HasFocus)
            QGtkStylePrivate::gtkWidgetSetFocus(gtkEntry, true);
        gtkPainter->paintShadow(gtkEntry, "entry", rect, option->state & State_Enabled ?
                                GTK_STATE_NORMAL : GTK_STATE_INSENSITIVE,
                                GTK_SHADOW_IN, d->gtk_widget_get_style(gtkEntry),
                                option->state & State_HasFocus ? QLS("focus") : QString());
        if (!interior_focus && option->state & State_HasFocus)
            gtkPainter->paintShadow(gtkEntry, "entry", option->rect, option->state & State_Enabled ?
                                    GTK_STATE_ACTIVE : GTK_STATE_INSENSITIVE,
                                    GTK_SHADOW_IN, d->gtk_widget_get_style(gtkEntry), QLS("GtkEntryShadowIn"));

        if (option->state & State_HasFocus)
            QGtkStylePrivate::gtkWidgetSetFocus(gtkEntry, false);
    }
    break;

    case PE_PanelLineEdit:
        if (const QStyleOptionFrame *panel = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            GtkWidget *gtkEntry = d->gtkWidget("GtkEntry");
            if (panel->lineWidth > 0)
                proxy()->drawPrimitive(PE_FrameLineEdit, option, painter, widget);
            uint resolve_mask = option->palette.resolve();
            GtkStyle *gtkEntryStyle = d->gtk_widget_get_style(gtkEntry);
            QRect textRect = option->rect.adjusted(gtkEntryStyle->xthickness, gtkEntryStyle->ythickness,
                                                   -gtkEntryStyle->xthickness, -gtkEntryStyle->ythickness);

            if (widget && widget->testAttribute(Qt::WA_SetPalette) &&
                resolve_mask & (1 << QPalette::Base)) // Palette overridden by user
                painter->fillRect(textRect, option->palette.base());
            else
                gtkPainter->paintFlatBox(gtkEntry, "entry_bg", textRect,
                                         option->state & State_Enabled ? GTK_STATE_NORMAL : GTK_STATE_INSENSITIVE, GTK_SHADOW_NONE, gtkEntryStyle);
        }
        break;

    case PE_FrameTabWidget:
        if (const QStyleOptionTabWidgetFrame *frame = qstyleoption_cast<const QStyleOptionTabWidgetFrame*>(option)) {
            GtkWidget *gtkNotebook = d->gtkWidget("GtkNotebook");
            style = d->gtk_widget_get_style(gtkNotebook);
            gtkPainter->setAlphaSupport(false);
            GtkShadowType shadow = GTK_SHADOW_OUT;
            GtkStateType state = GTK_STATE_NORMAL; // Only state supported by gtknotebook
            bool reverse = (option->direction == Qt::RightToLeft);
            QGtkStylePrivate::gtk_widget_set_direction(gtkNotebook, reverse ? GTK_TEXT_DIR_RTL : GTK_TEXT_DIR_LTR);
            if (const QStyleOptionTabWidgetFrameV2 *tabframe = qstyleoption_cast<const QStyleOptionTabWidgetFrameV2*>(option)) {
                GtkPositionType frameType = GTK_POS_TOP;
                QTabBar::Shape shape = frame->shape;
                int gapStart = 0;
                int gapSize = 0;
                if (shape == QTabBar::RoundedNorth || shape == QTabBar::RoundedSouth) {
                    frameType = (shape == QTabBar::RoundedNorth) ? GTK_POS_TOP : GTK_POS_BOTTOM;
                    gapStart = tabframe->selectedTabRect.left();
                    gapSize = tabframe->selectedTabRect.width();
                } else {
                    frameType = (shape == QTabBar::RoundedWest) ? GTK_POS_LEFT : GTK_POS_RIGHT;
                    gapStart = tabframe->selectedTabRect.y();
                    gapSize = tabframe->selectedTabRect.height();
                }
                gtkPainter->paintBoxGap(gtkNotebook, "notebook", option->rect, state, shadow, frameType,
                                        gapStart, gapSize, style);
                break; // done
            }

            // Note this is only the fallback option
            gtkPainter->paintBox(gtkNotebook, "notebook", option->rect, state, shadow, style);
        }
        break;

    case PE_PanelButtonCommand:
    case PE_PanelButtonTool: {
        bool isDefault = false;
        bool isTool = (element == PE_PanelButtonTool);
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton*>(option))
            isDefault = btn->features & QStyleOptionButton::DefaultButton;

        // don't draw a frame for tool buttons that have the autoRaise flag and are not enabled or on
        if (isTool && !(option->state & State_Enabled || option->state & State_On) && (option->state & State_AutoRaise))
            break;
        // don't draw a frame for dock widget buttons, unless we are hovering
        if (widget && widget->inherits("QDockWidgetTitleButton") && !(option->state & State_MouseOver))
            break;

        GtkStateType state = qt_gtk_state(option);
        if (option->state & State_On || option->state & State_Sunken)
            state = GTK_STATE_ACTIVE;
        GtkWidget *gtkButton = isTool ? d->gtkWidget("GtkToolButton.GtkButton") : d->gtkWidget("GtkButton");
        gint focusWidth, focusPad;
        gboolean interiorFocus = false;
        d->gtk_widget_style_get (gtkButton,
                                "focus-line-width", &focusWidth,
                                "focus-padding", &focusPad,
                                "interior-focus", &interiorFocus, NULL);

        style = d->gtk_widget_get_style(gtkButton);

        QRect buttonRect = option->rect;

        QString key;
        if (isDefault) {
            key += QLS("def");
            QGtkStylePrivate::gtk_widget_set_can_default(gtkButton, true);
            QGtkStylePrivate::gtk_window_set_default((GtkWindow*)QGtkStylePrivate::gtk_widget_get_toplevel(gtkButton), gtkButton);
            gtkPainter->paintBox(gtkButton, "buttondefault", buttonRect, state, GTK_SHADOW_IN,
                                 style, isDefault ? QLS("d") : QString());
        }

        bool hasFocus = option->state & State_HasFocus;

        if (hasFocus) {
            key += QLS("def");
            QGtkStylePrivate::gtkWidgetSetFocus(gtkButton, true);
        }

        if (!interiorFocus)
            buttonRect = buttonRect.adjusted(focusWidth, focusWidth, -focusWidth, -focusWidth);

        GtkShadowType shadow = (option->state & State_Sunken || option->state & State_On ) ?
                               GTK_SHADOW_IN : GTK_SHADOW_OUT;

        gtkPainter->paintBox(gtkButton, "button", buttonRect, state, shadow,
                             style, key);
        if (isDefault)
            QGtkStylePrivate::gtk_window_set_default((GtkWindow*)QGtkStylePrivate::gtk_widget_get_toplevel(gtkButton), 0);
        if (hasFocus)
            QGtkStylePrivate::gtkWidgetSetFocus(gtkButton, false);
    }
    break;

    case PE_IndicatorRadioButton: {
        GtkShadowType shadow = GTK_SHADOW_OUT;
        GtkStateType state = qt_gtk_state(option);

        if (option->state & State_Sunken)
            state = GTK_STATE_ACTIVE;

        if (option->state & State_NoChange)
            shadow = GTK_SHADOW_ETCHED_IN;
        else if (option->state & State_On)
            shadow = GTK_SHADOW_IN;
        else
            shadow = GTK_SHADOW_OUT;

        GtkWidget *gtkRadioButton = d->gtkWidget("GtkRadioButton");
        gint spacing;
        d->gtk_widget_style_get(gtkRadioButton, "indicator-spacing", &spacing, NULL);
        QRect buttonRect = option->rect.adjusted(spacing, spacing, -spacing, -spacing);
        gtkPainter->setClipRect(option->rect);
        // ### Note: Ubuntulooks breaks when the proper widget is passed
        //           Murrine engine requires a widget not to get RGBA check - warnings
        GtkWidget *gtkCheckButton = d->gtkWidget("GtkCheckButton");
        QString key(QLS("radiobutton"));
        if (option->state & State_HasFocus) { // Themes such as Nodoka check this flag
            key += QLatin1Char('f');
            QGtkStylePrivate::gtkWidgetSetFocus(gtkCheckButton, true);
        }
        gtkPainter->paintOption(gtkCheckButton , buttonRect, state, shadow, d->gtk_widget_get_style(gtkRadioButton), key);
        if (option->state & State_HasFocus)
            QGtkStylePrivate::gtkWidgetSetFocus(gtkCheckButton, false);
    }
    break;

    case PE_IndicatorCheckBox: {
        GtkShadowType shadow = GTK_SHADOW_OUT;
        GtkStateType state = qt_gtk_state(option);

        if (option->state & State_Sunken)
            state = GTK_STATE_ACTIVE;

        if (option->state & State_NoChange)
            shadow = GTK_SHADOW_ETCHED_IN;
        else if (option->state & State_On)
            shadow = GTK_SHADOW_IN;
        else
            shadow = GTK_SHADOW_OUT;

        int spacing;

        GtkWidget *gtkCheckButton = d->gtkWidget("GtkCheckButton");
        QString key(QLS("checkbutton"));
        if (option->state & State_HasFocus) { // Themes such as Nodoka checks this flag
            key += QLatin1Char('f');
            QGtkStylePrivate::gtkWidgetSetFocus(gtkCheckButton, true);
        }

        // Some styles such as aero-clone assume they can paint in the spacing area
        gtkPainter->setClipRect(option->rect);

        d->gtk_widget_style_get(gtkCheckButton, "indicator-spacing", &spacing, NULL);

        QRect checkRect = option->rect.adjusted(spacing, spacing, -spacing, -spacing);

        gtkPainter->paintCheckbox(gtkCheckButton, checkRect, state, shadow, d->gtk_widget_get_style(gtkCheckButton),
                                  key);
        if (option->state & State_HasFocus)
            QGtkStylePrivate::gtkWidgetSetFocus(gtkCheckButton, false);

    }
    break;

#ifndef QT_NO_TABBAR

    case PE_FrameTabBarBase:
        if (const QStyleOptionTabBarBase *tbb
                = qstyleoption_cast<const QStyleOptionTabBarBase *>(option)) {
            QRect tabRect = tbb->rect;
            painter->save();
            painter->setPen(QPen(option->palette.dark().color().dark(110), 0));
            switch (tbb->shape) {

            case QTabBar::RoundedNorth:
                painter->drawLine(tabRect.topLeft(), tabRect.topRight());
                break;

            case QTabBar::RoundedWest:
                painter->drawLine(tabRect.left(), tabRect.top(), tabRect.left(), tabRect.bottom());
                break;

            case QTabBar::RoundedSouth:
                painter->drawLine(tbb->rect.left(), tbb->rect.bottom(),
                                  tabRect.right(), tabRect.bottom());
                break;

            case QTabBar::RoundedEast:
                painter->drawLine(tabRect.topRight(), tabRect.bottomRight());
                break;

            case QTabBar::TriangularNorth:
            case QTabBar::TriangularEast:
            case QTabBar::TriangularWest:
            case QTabBar::TriangularSouth:
                painter->restore();
                QCommonStyle::drawPrimitive(element, option, painter, widget);
                return;
            }

            painter->restore();
        }
        return;

#endif // QT_NO_TABBAR

    case PE_Widget:
        break;

    default:
        QCommonStyle::drawPrimitive(element, option, painter, widget);
    }
}

/*!
    \reimp
*/
void QGtkStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,

                                   QPainter *painter, const QWidget *widget) const
{
    Q_D(const QGtkStyle);

    if (!d->isThemeAvailable()) {
        QCommonStyle::drawComplexControl(control, option, painter, widget);
        return;
    }

    GtkStyle* style = d->gtkStyle();
    QGtkPainter* gtkPainter = d->gtkPainter(painter);
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

    if (widget)
        alphaCornerColor = mergedColors(option->palette.color(widget->backgroundRole()), darkOutline);
    else
        alphaCornerColor = mergedColors(option->palette.background().color(), darkOutline);

    switch (control) {

    case CC_TitleBar:
        painter->save();
        if (const QStyleOptionTitleBar *titleBar = qstyleoption_cast<const QStyleOptionTitleBar *>(option)) {
            // Since this is drawn by metacity and not Gtk we
            // have to do custom drawing

            GdkColor gdkBg = style->bg[GTK_STATE_SELECTED];
            QColor bgColor(gdkBg.red>>8, gdkBg.green>>8, gdkBg.blue>>8);

            const int buttonMargin = 5;
            bool active = (titleBar->titleBarState & State_Active);
            QRect fullRect = titleBar->rect;
            QPalette palette = option->palette;
            QColor highlight = bgColor;

            QColor titleBarFrameBorder(active ? highlight.darker(180): dark.darker(110));
            QColor titleBarHighlight(active ? highlight.lighter(120): palette.background().color().lighter(120));
            QColor textColor(active ? 0xffffff : 0xff000000);
            QColor textAlphaColor(active ? 0xffffff : 0xff000000 );

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
                    qt_gtk_draw_mdibutton(painter, titleBar, minButtonRect, hover, sunken);
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
                    qt_gtk_draw_mdibutton(painter, titleBar, maxButtonRect, hover, sunken);

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
                    qt_gtk_draw_mdibutton(painter, titleBar, closeButtonRect, hover, sunken);
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
                    qt_gtk_draw_mdibutton(painter, titleBar, normalButtonRect, hover, sunken);

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
                    qt_gtk_draw_mdibutton(painter, titleBar, contextHelpButtonRect, hover, sunken);

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
                    qt_gtk_draw_mdibutton(painter, titleBar, shadeButtonRect, hover, sunken);
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
                    qt_gtk_draw_mdibutton(painter, titleBar, unshadeButtonRect, hover, sunken);
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

#ifndef QT_NO_GROUPBOX

    case CC_GroupBox:
        painter->save();

        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            QRect textRect = proxy()->subControlRect(CC_GroupBox, groupBox, SC_GroupBoxLabel, widget);
            QRect checkBoxRect = proxy()->subControlRect(CC_GroupBox, groupBox, SC_GroupBoxCheckBox, widget);
            // Draw title

            if ((groupBox->subControls & QStyle::SC_GroupBoxLabel) && !groupBox->text.isEmpty()) {
                // Draw prelight background
                GtkWidget *gtkCheckButton = d->gtkWidget("GtkCheckButton");

                if (option->state & State_MouseOver) {
                    QRect bgRect = textRect | checkBoxRect;
                    gtkPainter->paintFlatBox(gtkCheckButton, "checkbutton", bgRect.adjusted(0, 0, 0, -2),
                                             GTK_STATE_PRELIGHT, GTK_SHADOW_ETCHED_OUT, d->gtk_widget_get_style(gtkCheckButton));
                }

                if (!groupBox->text.isEmpty()) {
                    int alignment = int(groupBox->textAlignment);
                    if (!proxy()->styleHint(QStyle::SH_UnderlineShortcut, option, widget))
                        alignment |= Qt::TextHideMnemonic;
                    QColor textColor = groupBox->textColor; // Note: custom textColor is currently ignored
                    int labelState = GTK_STATE_INSENSITIVE;

                    if (option->state & State_Enabled)
                        labelState = (option->state & State_MouseOver) ? GTK_STATE_PRELIGHT : GTK_STATE_NORMAL;

                    GdkColor gdkText = d->gtk_widget_get_style(gtkCheckButton)->fg[labelState];
                    textColor = QColor(gdkText.red>>8, gdkText.green>>8, gdkText.blue>>8);
                    painter->setPen(textColor);
                    QFont font = painter->font();
                    font.setBold(true);
                    painter->setFont(font);
                    painter->drawText(textRect, Qt::TextShowMnemonic | Qt::AlignLeft| alignment, groupBox->text);

                    if (option->state & State_HasFocus)
                        gtkPainter->paintFocus(gtkCheckButton, "checkbutton", textRect.adjusted(-4, -1, 0, -3), GTK_STATE_ACTIVE, style);
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

#ifndef QT_NO_COMBOBOX

    case CC_ComboBox:
        // See: http://live.gnome.org/GnomeArt/Tutorials/GtkThemes/GtkComboBox
        // and http://live.gnome.org/GnomeArt/Tutorials/GtkThemes/GtkComboBoxEntry
        if (const QStyleOptionComboBox *comboBox = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            bool sunken = comboBox->state & State_On; // play dead, if combobox has no items
            BEGIN_STYLE_PIXMAPCACHE(QString::fromLatin1("cb-%0-%1").arg(sunken).arg(comboBox->editable));
            gtkPainter->reset(p);
            gtkPainter->setUsePixmapCache(false); // cached externally

            bool isEnabled = (comboBox->state & State_Enabled);
            bool focus = isEnabled && (comboBox->state & State_HasFocus);
            GtkStateType state = qt_gtk_state(option);
            int appears_as_list = !proxy()->styleHint(QStyle::SH_ComboBox_Popup, comboBox, widget);
            QStyleOptionComboBox comboBoxCopy = *comboBox;
            comboBoxCopy.rect = option->rect;

            bool reverse = (option->direction == Qt::RightToLeft);
            QRect rect = option->rect;
            QRect arrowButtonRect = proxy()->subControlRect(CC_ComboBox, &comboBoxCopy,
                                                   SC_ComboBoxArrow, widget);

            GtkShadowType shadow = (option->state & State_Sunken || option->state & State_On ) ?
                                   GTK_SHADOW_IN : GTK_SHADOW_OUT;
            const QHashableLatin1Literal comboBoxPath = comboBox->editable ? QHashableLatin1Literal("GtkComboBoxEntry") : QHashableLatin1Literal("GtkComboBox");

            // We use the gtk widget to position arrows and separators for us
            GtkWidget *gtkCombo = d->gtkWidget(comboBoxPath);
            GtkAllocation geometry = {0, 0, option->rect.width(), option->rect.height()};
            d->gtk_widget_set_direction(gtkCombo, reverse ? GTK_TEXT_DIR_RTL : GTK_TEXT_DIR_LTR);
            d->gtk_widget_size_allocate(gtkCombo, &geometry);

            QHashableLatin1Literal buttonPath = comboBox->editable ? QHashableLatin1Literal("GtkComboBoxEntry.GtkToggleButton")
                                : QHashableLatin1Literal("GtkComboBox.GtkToggleButton");
            GtkWidget *gtkToggleButton = d->gtkWidget(buttonPath);
            d->gtk_widget_set_direction(gtkToggleButton, reverse ? GTK_TEXT_DIR_RTL : GTK_TEXT_DIR_LTR);
            if (gtkToggleButton && (appears_as_list || comboBox->editable)) {
                if (focus)
                    QGtkStylePrivate::gtkWidgetSetFocus(gtkToggleButton, true);
                // Draw the combo box as a line edit with a button next to it
                if (comboBox->editable || appears_as_list) {
                    GtkStateType frameState = (state == GTK_STATE_PRELIGHT) ? GTK_STATE_NORMAL : state;
                    QHashableLatin1Literal entryPath = comboBox->editable ? QHashableLatin1Literal("GtkComboBoxEntry.GtkEntry") : QHashableLatin1Literal("GtkComboBox.GtkFrame");
                    GtkWidget *gtkEntry = d->gtkWidget(entryPath);
                    d->gtk_widget_set_direction(gtkEntry, reverse ? GTK_TEXT_DIR_RTL : GTK_TEXT_DIR_LTR);
                    QRect frameRect = option->rect;

                    if (reverse)
                        frameRect.setLeft(arrowButtonRect.right());
                    else
                        frameRect.setRight(arrowButtonRect.left());

                    // Fill the line edit background
                    // We could have used flat_box with "entry_bg" but that is probably not worth the overhead
                    uint resolve_mask = option->palette.resolve();
                    GtkStyle *gtkEntryStyle = d->gtk_widget_get_style(gtkEntry);
                    QRect contentRect = frameRect.adjusted(gtkEntryStyle->xthickness, gtkEntryStyle->ythickness,
                                                           -gtkEntryStyle->xthickness, -gtkEntryStyle->ythickness);
                    // Required for inner blue highlight with clearlooks
                    if (focus)
                        QGtkStylePrivate::gtkWidgetSetFocus(gtkEntry, true);

                    if (widget && widget->testAttribute(Qt::WA_SetPalette) &&
                        resolve_mask & (1 << QPalette::Base)) // Palette overridden by user
                        p->fillRect(contentRect, option->palette.base().color());
                    else {
                        gtkPainter->paintFlatBox(gtkEntry, "entry_bg", contentRect,
                                                 option->state & State_Enabled ? GTK_STATE_NORMAL : GTK_STATE_INSENSITIVE,
                                                 GTK_SHADOW_NONE, gtkEntryStyle, entryPath.toString() + QString::number(focus));
                    }

                    gtkPainter->paintShadow(gtkEntry, comboBox->editable ? "entry" : "frame", frameRect, frameState,
                                            GTK_SHADOW_IN, gtkEntryStyle, entryPath.toString() +
                                            QString::number(focus) + QString::number(comboBox->editable) +
                                            QString::number(option->direction));
                    if (focus)
                        QGtkStylePrivate::gtkWidgetSetFocus(gtkEntry, false);
                }

                GtkStateType buttonState = GTK_STATE_NORMAL;

                if (!(option->state & State_Enabled))
                    buttonState = GTK_STATE_INSENSITIVE;
                else if (option->state & State_Sunken || option->state & State_On)
                    buttonState = GTK_STATE_ACTIVE;
                else if (option->state & State_MouseOver && comboBox->activeSubControls & SC_ComboBoxArrow)
                    buttonState = GTK_STATE_PRELIGHT;

                Q_ASSERT(gtkToggleButton);
                gtkPainter->paintBox(gtkToggleButton, "button", arrowButtonRect, buttonState,
                                     shadow, d->gtk_widget_get_style(gtkToggleButton), buttonPath.toString() +
                                     QString::number(focus) + QString::number(option->direction));
                if (focus)
                    QGtkStylePrivate::gtkWidgetSetFocus(gtkToggleButton, false);
            } else {
                // Draw combo box as a button
                QRect buttonRect = option->rect;
                GtkStyle *gtkToggleButtonStyle = d->gtk_widget_get_style(gtkToggleButton);

                if (focus) // Clearlooks actually check the widget for the default state
                    QGtkStylePrivate::gtkWidgetSetFocus(gtkToggleButton, true);
                gtkPainter->paintBox(gtkToggleButton, "button",
                                     buttonRect, state,
                                     shadow, gtkToggleButtonStyle,
                                     buttonPath.toString() + QString::number(focus));
                if (focus)
                    QGtkStylePrivate::gtkWidgetSetFocus(gtkToggleButton, false);


                // Draw the separator between label and arrows
                QHashableLatin1Literal vSeparatorPath = comboBox->editable
                    ? QHashableLatin1Literal("GtkComboBoxEntry.GtkToggleButton.GtkHBox.GtkVSeparator")
                    : QHashableLatin1Literal("GtkComboBox.GtkToggleButton.GtkHBox.GtkVSeparator");

                if (GtkWidget *gtkVSeparator = d->gtkWidget(vSeparatorPath)) {
                    GtkAllocation allocation;
                    d->gtk_widget_get_allocation(gtkVSeparator, &allocation);
                    QRect vLineRect(allocation.x, allocation.y, allocation.width, allocation.height);

                    gtkPainter->paintVline(gtkVSeparator, "vseparator",
                                           vLineRect, state, d->gtk_widget_get_style(gtkVSeparator),
                                           0, vLineRect.height(), 0, vSeparatorPath.toString());


                    gint interiorFocus = true;
                    d->gtk_widget_style_get(gtkToggleButton, "interior-focus", &interiorFocus, NULL);
                    int xt = interiorFocus ? gtkToggleButtonStyle->xthickness : 0;
                    int yt = interiorFocus ? gtkToggleButtonStyle->ythickness : 0;
                    if (focus && ((option->state & State_KeyboardFocusChange) || styleHint(SH_UnderlineShortcut, option, widget)))
                        gtkPainter->paintFocus(gtkToggleButton, "button",
                                               option->rect.adjusted(xt, yt, -xt, -yt),
                                               option->state & State_Sunken ? GTK_STATE_ACTIVE : GTK_STATE_NORMAL,
                                               gtkToggleButtonStyle);
                }
            }

            if (comboBox->subControls & SC_ComboBoxArrow) {
                if (!isEnabled)
                    state = GTK_STATE_INSENSITIVE;
                else if (sunken)
                    state = GTK_STATE_ACTIVE;
                else if (option->state & State_MouseOver)
                    state = GTK_STATE_PRELIGHT;
                else
                    state = GTK_STATE_NORMAL;

                QHashableLatin1Literal arrowPath("");
                if (comboBox->editable) {
                    if (appears_as_list)
                        arrowPath = QHashableLatin1Literal("GtkComboBoxEntry.GtkToggleButton.GtkArrow");
                    else
                        arrowPath = QHashableLatin1Literal("GtkComboBoxEntry.GtkToggleButton.GtkHBox.GtkArrow");
                } else {
                    if (appears_as_list)
                        arrowPath = QHashableLatin1Literal("GtkComboBox.GtkToggleButton.GtkArrow");
                    else
                        arrowPath = QHashableLatin1Literal("GtkComboBox.GtkToggleButton.GtkHBox.GtkArrow");
                }

                GtkWidget *gtkArrow = d->gtkWidget(arrowPath);
                gfloat scale = 0.7;
                gint minSize = 15;
                QRect arrowWidgetRect;

                if (gtkArrow && !d->gtk_check_version(2, 12, 0)) {
                    d->gtk_widget_style_get(gtkArrow, "arrow-scaling", &scale, NULL);
                    d->gtk_widget_style_get(gtkCombo, "arrow-size", &minSize, NULL);
                }
                if (gtkArrow) {
                    GtkAllocation allocation;
                    d->gtk_widget_get_allocation(gtkArrow, &allocation);
                    arrowWidgetRect = QRect(allocation.x, allocation.y, allocation.width, allocation.height);
                    style = d->gtk_widget_get_style(gtkArrow);
                }

                // Note that for some reason the arrow-size is not properly respected with Hildon
                // Hence we enforce the minimum "arrow-size" ourselves
                int arrowSize = qMax(qMin(rect.height() - d->gtk_widget_get_style(gtkCombo)->ythickness * 2, minSize),
                                     qMin(arrowWidgetRect.width(), arrowWidgetRect.height()));
                QRect arrowRect(0, 0, static_cast<int>(arrowSize * scale), static_cast<int>(arrowSize * scale));

                arrowRect.moveCenter(arrowWidgetRect.center());

                if (sunken) {
                    int xoff, yoff;
                    const QHashableLatin1Literal toggleButtonPath = comboBox->editable
                            ? QHashableLatin1Literal("GtkComboBoxEntry.GtkToggleButton")
                            : QHashableLatin1Literal("GtkComboBox.GtkToggleButton");

                    GtkWidget *gtkButton = d->gtkWidget(toggleButtonPath);
                    d->gtk_widget_style_get(gtkButton, "child-displacement-x", &xoff, NULL);
                    d->gtk_widget_style_get(gtkButton, "child-displacement-y", &yoff, NULL);
                    arrowRect = arrowRect.adjusted(xoff, yoff, xoff, yoff);
                }

                // Some styles such as Nimbus paint outside the arrowRect
                // hence we have provide the whole widget as the cliprect
                if (gtkArrow) {
                    gtkPainter->setClipRect(option->rect);
                    gtkPainter->paintArrow(gtkArrow, "arrow", arrowRect,
                                           GTK_ARROW_DOWN, state, GTK_SHADOW_NONE, true,
                                           style, arrowPath.toString() + QString::number(option->direction));
                }
            }
            END_STYLE_PIXMAPCACHE;
        }
        break;
#endif // QT_NO_COMBOBOX
#ifndef QT_NO_TOOLBUTTON

    case CC_ToolButton:
        if (const QStyleOptionToolButton *toolbutton
                = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            QRect button, menuarea;
            button = proxy()->subControlRect(control, toolbutton, SC_ToolButton, widget);
            menuarea = proxy()->subControlRect(control, toolbutton, SC_ToolButtonMenu, widget);
            State bflags = toolbutton->state & ~(State_Sunken | State_MouseOver);

            if (bflags & State_AutoRaise)
                if (!(bflags & State_MouseOver))
                    bflags &= ~State_Raised;

            State mflags = bflags;

            if (toolbutton->state & State_Sunken) {
                if (toolbutton->activeSubControls & SC_ToolButton)
                    bflags |= State_Sunken;
                if (toolbutton->activeSubControls & SC_ToolButtonMenu)
                    mflags |= State_Sunken;
            } else if (toolbutton->state & State_MouseOver) {
                if (toolbutton->activeSubControls & SC_ToolButton)
                    bflags |= State_MouseOver;
                if (toolbutton->activeSubControls & SC_ToolButtonMenu)
                    mflags |= State_MouseOver;
            }

            QStyleOption tool(0);

            tool.palette = toolbutton->palette;

            if (toolbutton->subControls & SC_ToolButton) {
                if (bflags & (State_Sunken | State_On | State_Raised | State_MouseOver)) {
                    tool.rect = button;
                    tool.state = bflags;
                    proxy()->drawPrimitive(PE_PanelButtonTool, &tool, painter, widget);
                }
            }

            bool drawMenuArrow = toolbutton->features & QStyleOptionToolButton::HasMenu &&
                                 !(toolbutton->features & QStyleOptionToolButton::MenuButtonPopup);
            int popupArrowSize = drawMenuArrow ? 7 : 0;

            if (toolbutton->state & State_HasFocus) {
                QStyleOptionFocusRect fr;
                fr.QStyleOption::operator=(*toolbutton);
                fr.rect = proxy()->subControlRect(CC_ToolButton, toolbutton, SC_ToolButton, widget);
                fr.rect.adjust(1, 1, -1, -1);
                proxy()->drawPrimitive(PE_FrameFocusRect, &fr, painter, widget);
            }

            QStyleOptionToolButton label = *toolbutton;
            label.state = bflags;
            GtkWidget *gtkButton = d->gtkWidget("GtkToolButton.GtkButton");
            QPalette pal = toolbutton->palette;
            if (option->state & State_Enabled &&
                option->state & State_MouseOver && !(widget && widget->testAttribute(Qt::WA_SetPalette))) {
                GdkColor gdkText = d->gtk_widget_get_style(gtkButton)->fg[GTK_STATE_PRELIGHT];
                QColor textColor = QColor(gdkText.red>>8, gdkText.green>>8, gdkText.blue>>8);
                pal.setBrush(QPalette::All, QPalette::ButtonText, textColor);
                label.palette = pal;
            }
            label.rect = button.adjusted(style->xthickness, style->ythickness,
                                        -style->xthickness - popupArrowSize, -style->ythickness);
            proxy()->drawControl(CE_ToolButtonLabel, &label, painter, widget);

            if (toolbutton->subControls & SC_ToolButtonMenu) {
                tool.rect = menuarea;
                tool.state = mflags;
                if ((mflags & State_Enabled && (mflags & (State_Sunken | State_Raised | State_MouseOver))) || !(mflags & State_AutoRaise))
                    proxy()->drawPrimitive(PE_IndicatorButtonDropDown, &tool, painter, widget);

                proxy()->drawPrimitive(PE_IndicatorArrowDown, &tool, painter, widget);

            } else if (drawMenuArrow) {
                QRect ir = toolbutton->rect;
                QStyleOptionToolButton newBtn = *toolbutton;
                newBtn.rect = QRect(ir.right() - popupArrowSize - style->xthickness - 3, ir.height()/2 - 1, popupArrowSize, popupArrowSize);
                proxy()->drawPrimitive(PE_IndicatorArrowDown, &newBtn, painter, widget);
            }
        }
        break;

#endif // QT_NO_TOOLBUTTON
#ifndef QT_NO_SCROLLBAR

    case CC_ScrollBar:
        if (const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            GtkWidget *gtkHScrollBar = d->gtkWidget("GtkHScrollbar");
            GtkWidget *gtkVScrollBar = d->gtkWidget("GtkVScrollbar");

            // Fill background in case the scrollbar is partially transparent
            painter->fillRect(option->rect, option->palette.background());

            QRect rect = scrollBar->rect;
            QRect scrollBarSubLine = proxy()->subControlRect(control, scrollBar, SC_ScrollBarSubLine, widget);
            QRect scrollBarAddLine = proxy()->subControlRect(control, scrollBar, SC_ScrollBarAddLine, widget);
            QRect scrollBarSlider = proxy()->subControlRect(control, scrollBar, SC_ScrollBarSlider, widget);
            QRect grooveRect = proxy()->subControlRect(control, scrollBar, SC_ScrollBarGroove, widget);
            bool horizontal = scrollBar->orientation == Qt::Horizontal;
            GtkWidget * scrollbarWidget = horizontal ? gtkHScrollBar : gtkVScrollBar;
            style = d->gtk_widget_get_style(scrollbarWidget);
            gboolean trough_under_steppers = true;
            gboolean trough_side_details = false;
            gboolean activate_slider = false;
            gboolean stepper_size = 14;
            gint trough_border = 1;
            if (!d->gtk_check_version(2, 10, 0)) {
                d->gtk_widget_style_get((GtkWidget*)(scrollbarWidget),
                                           "trough-border",         &trough_border,
                                           "trough-side-details",   &trough_side_details,
                                           "trough-under-steppers", &trough_under_steppers,
                                           "activate-slider",       &activate_slider,
                                           "stepper-size",          &stepper_size, NULL);
            }
            if (trough_under_steppers) {
                scrollBarAddLine.adjust(trough_border, trough_border, -trough_border, -trough_border);
                scrollBarSubLine.adjust(trough_border, trough_border, -trough_border, -trough_border);
                scrollBarSlider.adjust(horizontal ? -trough_border : 0, horizontal ? 0 : -trough_border,
                                       horizontal ? trough_border : 0, horizontal ? 0 : trough_border);
            }

            // Some styles check the position of scrollbars in order to determine
            // if lines should be painted when the scrollbar is in max or min positions.
            int maximum = 2;
            int fakePos = 0;
            bool reverse = (option->direction == Qt::RightToLeft);
            if (scrollBar->minimum == scrollBar->maximum)
                maximum = 0;
            if (scrollBar->sliderPosition == scrollBar->maximum)
                fakePos = maximum;
            else if (scrollBar->sliderPosition > scrollBar->minimum)
                fakePos = maximum - 1;


            GtkRange *range = (GtkRange*)(horizontal ? gtkHScrollBar : gtkVScrollBar);
            GtkAdjustment *adjustment = 0;

            if (d->gtk_adjustment_configure)
                adjustment = d->gtk_range_get_adjustment(range);
            if (adjustment) {
                d->gtk_adjustment_configure(adjustment, fakePos, 0, maximum, 0, 0, 0);
            } else {
                adjustment = (GtkAdjustment*)d->gtk_adjustment_new(fakePos, 0, maximum, 0, 0, 0);
                d->gtk_range_set_adjustment(range, adjustment);
            }

            if (scrollBar->subControls & SC_ScrollBarGroove) {
                GtkStateType state = GTK_STATE_ACTIVE;

                if (!(option->state & State_Enabled))
                    state = GTK_STATE_INSENSITIVE;

                if (trough_under_steppers)
                    grooveRect = option->rect;

                gtkPainter->paintBox(scrollbarWidget, "trough", grooveRect, state, GTK_SHADOW_IN, style);
            }

            //paint slider
            if (scrollBar->subControls & SC_ScrollBarSlider) {
                GtkStateType state = GTK_STATE_NORMAL;

                if (!(option->state & State_Enabled))
                    state = GTK_STATE_INSENSITIVE;
                else if (activate_slider &&
                         option->state & State_Sunken && (scrollBar->activeSubControls & SC_ScrollBarSlider))
                    state = GTK_STATE_ACTIVE;
                else if (option->state & State_MouseOver && (scrollBar->activeSubControls & SC_ScrollBarSlider))
                    state = GTK_STATE_PRELIGHT;

                GtkShadowType shadow = GTK_SHADOW_OUT;

                if (trough_under_steppers) {
                    if (!horizontal)
                        scrollBarSlider.adjust(trough_border, 0, -trough_border, 0);
                    else
                        scrollBarSlider.adjust(0, trough_border, 0, -trough_border);
                }

                gtkPainter->paintSlider(scrollbarWidget, "slider", scrollBarSlider, state, shadow, style,
                                        horizontal ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL, QString(QLS("%0%1")).arg(fakePos).arg(maximum));
            }

            if (scrollBar->subControls & SC_ScrollBarAddLine) {
                GtkAllocation vAllocation;
                vAllocation.y = scrollBarAddLine.top();
                vAllocation.height = scrollBarAddLine.height() - rect.height() + 6;
                d->gtk_widget_set_allocation(gtkVScrollBar, &vAllocation);

                GtkAllocation hAllocation;
                hAllocation.x = scrollBarAddLine.right();
                hAllocation.width = scrollBarAddLine.width() - rect.width();
                d->gtk_widget_set_allocation(gtkHScrollBar, &hAllocation);

                GtkShadowType shadow = GTK_SHADOW_OUT;
                GtkStateType state = GTK_STATE_NORMAL;

                if (!(option->state & State_Enabled) || (fakePos == maximum))
                    state = GTK_STATE_INSENSITIVE;
                else if (option->state & State_Sunken && (scrollBar->activeSubControls & SC_ScrollBarAddLine)) {
                    state = GTK_STATE_ACTIVE;
                    shadow = GTK_SHADOW_IN;

                } else if (option->state & State_MouseOver && (scrollBar->activeSubControls & SC_ScrollBarAddLine))
                    state = GTK_STATE_PRELIGHT;

                gtkPainter->paintBox(scrollbarWidget,
                                     horizontal ? "hscrollbar" : "vscrollbar", scrollBarAddLine,
                                     state, shadow, style, QLS("add"));

                gtkPainter->paintArrow(scrollbarWidget, horizontal ? "hscrollbar" : "vscrollbar", scrollBarAddLine.adjusted(4, 4, -4, -4),
                                       horizontal ? (reverse ? GTK_ARROW_LEFT : GTK_ARROW_RIGHT) :
                                       GTK_ARROW_DOWN, state, GTK_SHADOW_NONE, false, style);
            }

            if (scrollBar->subControls & SC_ScrollBarSubLine) {
                GtkAllocation vAllocation;
                vAllocation.y = 0;
                vAllocation.height = scrollBarSubLine.height();
                d->gtk_widget_set_allocation(gtkVScrollBar, &vAllocation);

                GtkAllocation hAllocation;
                hAllocation.x = 0;
                hAllocation.width = scrollBarSubLine.width();
                d->gtk_widget_set_allocation(gtkHScrollBar, &hAllocation);

                GtkShadowType shadow = GTK_SHADOW_OUT;
                GtkStateType state = GTK_STATE_NORMAL;

                if (!(option->state & State_Enabled) || (fakePos == 0))
                    state = GTK_STATE_INSENSITIVE;
                else if (option->state & State_Sunken && (scrollBar->activeSubControls & SC_ScrollBarSubLine)) {
                    shadow = GTK_SHADOW_IN;
                    state = GTK_STATE_ACTIVE;

                } else if (option->state & State_MouseOver && (scrollBar->activeSubControls & SC_ScrollBarSubLine))
                    state = GTK_STATE_PRELIGHT;

                gtkPainter->paintBox(scrollbarWidget, horizontal ? "hscrollbar" : "vscrollbar", scrollBarSubLine,
                                     state, shadow, style, QLS("sub"));

                gtkPainter->paintArrow(scrollbarWidget, horizontal ? "hscrollbar" : "vscrollbar", scrollBarSubLine.adjusted(4, 4, -4, -4),
                                       horizontal ? (reverse ? GTK_ARROW_RIGHT : GTK_ARROW_LEFT) :
                                       GTK_ARROW_UP, state, GTK_SHADOW_NONE, false, style);
            }
        }
        break;

#endif //QT_NO_SCROLLBAR
#ifndef QT_NO_SPINBOX

    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {

            GtkWidget *gtkSpinButton = spinBox->buttonSymbols == QAbstractSpinBox::NoButtons
                        ? d->gtkWidget("GtkEntry")
                        : d->gtkWidget("GtkSpinButton");
            bool isEnabled = (spinBox->state & State_Enabled);
            bool hover = isEnabled && (spinBox->state & State_MouseOver);
            bool sunken = (spinBox->state & State_Sunken);
            bool upIsActive = (spinBox->activeSubControls == SC_SpinBoxUp);
            bool downIsActive = (spinBox->activeSubControls == SC_SpinBoxDown);
            bool reverse = (spinBox->direction == Qt::RightToLeft);

            QRect editArea = option->rect;
            QRect editRect = proxy()->subControlRect(CC_SpinBox, option, SC_SpinBoxEditField, widget);
            QRect upRect, downRect, buttonRect;
            if (spinBox->buttonSymbols != QAbstractSpinBox::NoButtons) {
                upRect = proxy()->subControlRect(CC_SpinBox, option, SC_SpinBoxUp, widget);
                downRect = proxy()->subControlRect(CC_SpinBox, option, SC_SpinBoxDown, widget);

                //### Move this to subControlRect
                upRect.setTop(option->rect.top());

                if (reverse)
                    upRect.setLeft(option->rect.left());
                else
                    upRect.setRight(option->rect.right());

                downRect.setBottom(option->rect.bottom());

                if (reverse)
                    downRect.setLeft(option->rect.left());
                else
                    downRect.setRight(option->rect.right());

                buttonRect = upRect | downRect;

                if (reverse)
                    editArea.setLeft(upRect.right());
                else
                    editArea.setRight(upRect.left());
            }
            if (spinBox->frame) {
                GtkStateType state = qt_gtk_state(option);

                if (!(option->state & State_Enabled))
                    state = GTK_STATE_INSENSITIVE;
                else if (option->state & State_HasFocus)
                    state = GTK_STATE_NORMAL;
                else if (state == GTK_STATE_PRELIGHT)
                    state = GTK_STATE_NORMAL;

                style = d->gtk_widget_get_style(gtkSpinButton);


                QString key;

                if (option->state & State_HasFocus) {
                    key += QLatin1Char('f');
                    QGtkStylePrivate::gtkWidgetSetFocus(gtkSpinButton, true);
                }

                uint resolve_mask = option->palette.resolve();

                if (resolve_mask & (1 << QPalette::Base)) // Palette overridden by user
                    painter->fillRect(editRect, option->palette.base().color());
                else
                    gtkPainter->paintFlatBox(gtkSpinButton, "entry_bg", editArea.adjusted(style->xthickness, style->ythickness,
                                             -style->xthickness, -style->ythickness),
                                             option->state & State_Enabled ?
                                             GTK_STATE_NORMAL : GTK_STATE_INSENSITIVE, GTK_SHADOW_NONE, style, key);

                gtkPainter->paintShadow(gtkSpinButton, "entry", editArea, state, GTK_SHADOW_IN, d->gtk_widget_get_style(gtkSpinButton), key);
                if (spinBox->buttonSymbols != QAbstractSpinBox::NoButtons) {
                    gtkPainter->paintBox(gtkSpinButton, "spinbutton", buttonRect, state, GTK_SHADOW_IN, style, key);

                    upRect.setSize(downRect.size());
                    if (!(option->state & State_Enabled))
                        gtkPainter->paintBox(gtkSpinButton, "spinbutton_up", upRect, GTK_STATE_INSENSITIVE, GTK_SHADOW_IN, style, key);
                    else if (upIsActive && sunken)
                        gtkPainter->paintBox(gtkSpinButton, "spinbutton_up", upRect, GTK_STATE_ACTIVE, GTK_SHADOW_IN, style, key);
                    else if (upIsActive && hover)
                        gtkPainter->paintBox(gtkSpinButton, "spinbutton_up", upRect, GTK_STATE_PRELIGHT, GTK_SHADOW_OUT, style, key);
                    else
                        gtkPainter->paintBox(gtkSpinButton, "spinbutton_up", upRect, GTK_STATE_NORMAL, GTK_SHADOW_OUT, style, key);

                    if (!(option->state & State_Enabled))
                        gtkPainter->paintBox(gtkSpinButton, "spinbutton_down", downRect, GTK_STATE_INSENSITIVE, GTK_SHADOW_IN, style, key);
                    else if (downIsActive && sunken)
                        gtkPainter->paintBox(gtkSpinButton, "spinbutton_down", downRect, GTK_STATE_ACTIVE, GTK_SHADOW_IN, style, key);
                    else if (downIsActive && hover)
                        gtkPainter->paintBox(gtkSpinButton, "spinbutton_down", downRect, GTK_STATE_PRELIGHT, GTK_SHADOW_OUT, style, key);
                    else
                        gtkPainter->paintBox(gtkSpinButton, "spinbutton_down", downRect, GTK_STATE_NORMAL, GTK_SHADOW_OUT, style, key);

                    if (option->state & State_HasFocus)
                        QGtkStylePrivate::gtkWidgetSetFocus(gtkSpinButton, false);
                }
            }

            if (spinBox->buttonSymbols == QAbstractSpinBox::PlusMinus) {
                int centerX = upRect.center().x();
                int centerY = upRect.center().y();
                // plus/minus

                if (spinBox->activeSubControls == SC_SpinBoxUp && sunken) {
                    painter->drawLine(1 + centerX - 2, 1 + centerY, 1 + centerX + 2, 1 + centerY);
                    painter->drawLine(1 + centerX, 1 + centerY - 2, 1 + centerX, 1 + centerY + 2);

                } else {
                    painter->drawLine(centerX - 2, centerY, centerX + 2, centerY);
                    painter->drawLine(centerX, centerY - 2, centerX, centerY + 2);
                }
                centerX = downRect.center().x();
                centerY = downRect.center().y();

                if (spinBox->activeSubControls == SC_SpinBoxDown && sunken) {
                    painter->drawLine(1 + centerX - 2, 1 + centerY, 1 + centerX + 2, 1 + centerY);
                } else {
                    painter->drawLine(centerX - 2, centerY, centerX + 2, centerY);
                }

            } else if (spinBox->buttonSymbols == QAbstractSpinBox::UpDownArrows) {
                int size = d->getSpinboxArrowSize();
                int w = size / 2 - 1;
                w -= w % 2 - 1; // force odd
                int h = (w + 1)/2;
                QRect arrowRect(0, 0, w, h);
                arrowRect.moveCenter(upRect.center());
                // arrows
                GtkStateType state = GTK_STATE_NORMAL;

                if (!(option->state & State_Enabled) || !(spinBox->stepEnabled & QAbstractSpinBox::StepUpEnabled))
                    state = GTK_STATE_INSENSITIVE;

                gtkPainter->paintArrow(gtkSpinButton, "spinbutton", arrowRect, GTK_ARROW_UP, state,
                                       GTK_SHADOW_NONE, false, style);

                arrowRect.moveCenter(downRect.center());

                if (!(option->state & State_Enabled) || !(spinBox->stepEnabled & QAbstractSpinBox::StepDownEnabled))
                    state = GTK_STATE_INSENSITIVE;

                gtkPainter->paintArrow(gtkSpinButton, "spinbutton", arrowRect, GTK_ARROW_DOWN, state,
                                       GTK_SHADOW_NONE, false, style);
            }
        }
        break;

#endif // QT_NO_SPINBOX

#ifndef QT_NO_SLIDER

    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            GtkWidget *hScaleWidget = d->gtkWidget("GtkHScale");
            GtkWidget *vScaleWidget = d->gtkWidget("GtkVScale");

            QRect groove = proxy()->subControlRect(CC_Slider, option, SC_SliderGroove, widget);
            QRect handle = proxy()->subControlRect(CC_Slider, option, SC_SliderHandle, widget);

            bool horizontal = slider->orientation == Qt::Horizontal;
            bool ticksAbove = slider->tickPosition & QSlider::TicksAbove;
            bool ticksBelow = slider->tickPosition & QSlider::TicksBelow;

            QBrush oldBrush = painter->brush();
            QPen oldPen = painter->pen();

            QColor shadowAlpha(Qt::black);
            shadowAlpha.setAlpha(10);
            QColor highlightAlpha(Qt::white);
            highlightAlpha.setAlpha(80);

            QGtkStylePrivate::gtk_widget_set_direction(hScaleWidget, slider->upsideDown ?
                                                       GTK_TEXT_DIR_RTL : GTK_TEXT_DIR_LTR);
            GtkWidget *scaleWidget = horizontal ? hScaleWidget : vScaleWidget;
            style = d->gtk_widget_get_style(scaleWidget);

            if ((option->subControls & SC_SliderGroove) && groove.isValid()) {

                GtkRange *range = (GtkRange*)scaleWidget;
                GtkAdjustment *adjustment = 0;
                if (d->gtk_adjustment_configure)
                    adjustment = d->gtk_range_get_adjustment(range);
                if (adjustment) {
                    d->gtk_adjustment_configure(adjustment,
                                                slider->sliderPosition,
                                                slider->minimum,
                                                slider->maximum,
                                                slider->singleStep,
                                                slider->singleStep,
                                                slider->pageStep);
                } else {
                    adjustment = (GtkAdjustment*)d->gtk_adjustment_new(slider->sliderPosition,
                                                                       slider->minimum,
                                                                       slider->maximum,
                                                                       slider->singleStep,
                                                                       slider->singleStep,
                                                                       slider->pageStep);
                    d->gtk_range_set_adjustment(range, adjustment);
                }

                int outerSize;
                d->gtk_range_set_inverted(range, !horizontal);
                d->gtk_widget_style_get(scaleWidget, "trough-border", &outerSize, NULL);
                outerSize++;

                GtkStateType state = qt_gtk_state(option);
                int focusFrameMargin = 2;
                QRect grooveRect = option->rect.adjusted(focusFrameMargin, outerSize + focusFrameMargin,
                                   -focusFrameMargin, -outerSize - focusFrameMargin);

                gboolean trough_side_details = false; // Indicates if the upper or lower scale background differs
                if (!d->gtk_check_version(2, 10, 0))
                    d->gtk_widget_style_get((GtkWidget*)(scaleWidget), "trough-side-details", &trough_side_details, NULL);

                if (!trough_side_details) {
                    gtkPainter->paintBox(scaleWidget, "trough", grooveRect, state,
                                         GTK_SHADOW_IN, style, QString(QLS("p%0")).arg(slider->sliderPosition));
                } else {
                    QRect upperGroove = grooveRect;
                    QRect lowerGroove = grooveRect;

                    if (horizontal) {
                        if (slider->upsideDown) {
                            lowerGroove.setLeft(handle.center().x());
                            upperGroove.setRight(handle.center().x());
                        } else {
                            upperGroove.setLeft(handle.center().x());
                            lowerGroove.setRight(handle.center().x());
                        }
                    } else {
                        if (!slider->upsideDown) {
                            lowerGroove.setBottom(handle.center().y());
                            upperGroove.setTop(handle.center().y());
                        } else {
                            upperGroove.setBottom(handle.center().y());
                            lowerGroove.setTop(handle.center().y());
                        }
                    }

                    gtkPainter->paintBox(scaleWidget, "trough-upper", upperGroove, state,
                                         GTK_SHADOW_IN, style, QString(QLS("p%0")).arg(slider->sliderPosition));
                    gtkPainter->paintBox(scaleWidget, "trough-lower", lowerGroove, state,
                                         GTK_SHADOW_IN, style, QString(QLS("p%0")).arg(slider->sliderPosition));
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
                        if (ticksAbove)
                            painter->drawLine(pos, slider->rect.top() + extra,
                                              pos, slider->rect.top() + tickSize);
                        if (ticksBelow)
                            painter->drawLine(pos, slider->rect.bottom() - extra,
                                              pos, slider->rect.bottom() - tickSize);

                    } else {
                        if (ticksAbove)
                            painter->drawLine(slider->rect.left() + extra, pos,
                                              slider->rect.left() + tickSize, pos);
                        if (ticksBelow)
                            painter->drawLine(slider->rect.right() - extra, pos,
                                              slider->rect.right() - tickSize, pos);
                    }

                    // In the case where maximum is max int
                    int nextInterval = v + interval;
                    if (nextInterval < v)
                        break;
                    v = nextInterval;
                }
            }

            // Draw slider handle
            if (option->subControls & SC_SliderHandle) {
                GtkShadowType shadow =  GTK_SHADOW_OUT;
                GtkStateType state = GTK_STATE_NORMAL;

                if (!(option->state & State_Enabled))
                    state = GTK_STATE_INSENSITIVE;
                else if (option->state & State_MouseOver && option->activeSubControls & SC_SliderHandle)
                    state = GTK_STATE_PRELIGHT;

                bool horizontal = option->state & State_Horizontal;

                if (slider->state & State_HasFocus) {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=(*slider);
                    fropt.rect = slider->rect.adjusted(-1, -1 ,1, 1);

                    if (horizontal) {
                        fropt.rect.setTop(handle.top() - 3);
                        fropt.rect.setBottom(handle.bottom() + 4);

                    } else {
                        fropt.rect.setLeft(handle.left() - 3);
                        fropt.rect.setRight(handle.right() + 3);
                    }
                    proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
                }
                gtkPainter->paintSlider(scaleWidget, horizontal ? "hscale" : "vscale", handle, state, shadow, style,
                                        horizontal ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);
            }
            painter->setBrush(oldBrush);
            painter->setPen(oldPen);
        }
        break;
    case CC_Dial:
        if (const QStyleOptionSlider *dial = qstyleoption_cast<const QStyleOptionSlider *>(option))
            QStyleHelper::drawDial(dial, painter);
        break;

#endif // QT_NO_SLIDER

    default:
        QCommonStyle::drawComplexControl(control, option, painter, widget);

        break;
    }
}


/*!
    \reimp
*/
void QGtkStyle::drawControl(ControlElement element,
                            const QStyleOption *option,
                            QPainter *painter,
                            const QWidget *widget) const
{
    Q_D(const QGtkStyle);

    if (!d->isThemeAvailable()) {
        QCommonStyle::drawControl(element, option, painter, widget);
        return;
    }

    GtkStyle* style = d->gtkStyle();
    QGtkPainter* gtkPainter = d->gtkPainter(painter);

    switch (element) {
    case CE_ProgressBarLabel:
        if (const QStyleOptionProgressBar *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            GtkWidget *gtkProgressBar = d->gtkWidget("GtkProgressBar");
            if (!gtkProgressBar)
                return;

            QRect leftRect;
            QRect rect = bar->rect;
            GtkStyle *gtkProgressBarStyle = d->gtk_widget_get_style(gtkProgressBar);
            GdkColor gdkText = gtkProgressBarStyle->fg[GTK_STATE_NORMAL];
            QColor textColor = QColor(gdkText.red>>8, gdkText.green>>8, gdkText.blue>>8);
            gdkText = gtkProgressBarStyle->fg[GTK_STATE_PRELIGHT];
            QColor alternateTextColor= QColor(gdkText.red>>8, gdkText.green>>8, gdkText.blue>>8);

            painter->save();
            bool vertical = false, inverted = false;
            if (const QStyleOptionProgressBarV2 *bar2 = qstyleoption_cast<const QStyleOptionProgressBarV2 *>(option)) {
                vertical = (bar2->orientation == Qt::Vertical);
                inverted = bar2->invertedAppearance;
            }
            if (vertical)
                rect = QRect(rect.left(), rect.top(), rect.height(), rect.width()); // flip width and height
            const int progressIndicatorPos = (bar->progress - qreal(bar->minimum)) * rect.width() /
                                              qMax(qreal(1.0), qreal(bar->maximum) - bar->minimum);
            if (progressIndicatorPos >= 0 && progressIndicatorPos <= rect.width())
                leftRect = QRect(rect.left(), rect.top(), progressIndicatorPos, rect.height());
            if (vertical)
                leftRect.translate(rect.width() - progressIndicatorPos, 0);

            bool flip = (!vertical && (((bar->direction == Qt::RightToLeft) && !inverted) ||
                                       ((bar->direction == Qt::LeftToRight) && inverted)));

            QRegion rightRect = rect;
            rightRect = rightRect.subtracted(leftRect);
            painter->setClipRegion(rightRect);
            painter->setPen(flip ? alternateTextColor : textColor);
            painter->drawText(rect, bar->text, QTextOption(Qt::AlignAbsolute | Qt::AlignHCenter | Qt::AlignVCenter));
            if (!leftRect.isNull()) {
                painter->setPen(flip ? textColor : alternateTextColor);
                painter->setClipRect(leftRect);
                painter->drawText(rect, bar->text, QTextOption(Qt::AlignAbsolute | Qt::AlignHCenter | Qt::AlignVCenter));
            }
            painter->restore();
        }
        break;
    case CE_PushButtonLabel:
        if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            QRect ir = button->rect;
            uint tf = Qt::AlignVCenter | Qt::TextShowMnemonic;
            QPoint buttonShift;

            if (option->state & State_Sunken)
                buttonShift = QPoint(pixelMetric(PM_ButtonShiftHorizontal, option, widget),
                                     proxy()->pixelMetric(PM_ButtonShiftVertical, option, widget));

            if (proxy()->styleHint(SH_UnderlineShortcut, button, widget))
                tf |= Qt::TextShowMnemonic;
            else
                tf |= Qt::TextHideMnemonic;

            if (!button->icon.isNull()) {
                //Center both icon and text
                QPoint point;

                QIcon::Mode mode = button->state & State_Enabled ? QIcon::Normal : QIcon::Disabled;
                if (mode == QIcon::Normal && button->state & State_HasFocus)
                    mode = QIcon::Active;

                QIcon::State state = QIcon::Off;

                if (button->state & State_On)
                    state = QIcon::On;

                QPixmap pixmap = button->icon.pixmap(button->iconSize, mode, state);
                int w = pixmap.width();
                int h = pixmap.height();

                if (!button->text.isEmpty())
                    w += button->fontMetrics.boundingRect(option->rect, tf, button->text).width() + 4;

                point = QPoint(ir.x() + ir.width() / 2 - w / 2,
                               ir.y() + ir.height() / 2 - h / 2);

                if (button->direction == Qt::RightToLeft)
                    point.rx() += pixmap.width();

                painter->drawPixmap(visualPos(button->direction, button->rect, point + buttonShift), pixmap);

                if (button->direction == Qt::RightToLeft)
                    ir.translate(-point.x() - 2, 0);
                else
                    ir.translate(point.x() + pixmap.width() + 2, 0);

                // left-align text if there is
                if (!button->text.isEmpty())
                    tf |= Qt::AlignLeft;

            } else {
                tf |= Qt::AlignHCenter;
            }

            ir.translate(buttonShift);

            if (button->features & QStyleOptionButton::HasMenu)
                ir = ir.adjusted(0, 0, -pixelMetric(PM_MenuButtonIndicator, button, widget), 0);

            GtkWidget *gtkButton = d->gtkWidget("GtkButton");
            QPalette pal = button->palette;
            int labelState = GTK_STATE_INSENSITIVE;
            if (option->state & State_Enabled)
                labelState = (option->state & State_MouseOver && !(option->state & State_Sunken)) ?
                             GTK_STATE_PRELIGHT : GTK_STATE_NORMAL;

            GdkColor gdkText = d->gtk_widget_get_style(gtkButton)->fg[labelState];
            QColor textColor = QColor(gdkText.red>>8, gdkText.green>>8, gdkText.blue>>8);
            pal.setBrush(QPalette::ButtonText, textColor);
            proxy()->drawItemText(painter, ir, tf, pal, (button->state & State_Enabled),
                         button->text, QPalette::ButtonText);
        }
        break;

    case CE_RadioButton: // Fall through
    case CE_CheckBox:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            bool isRadio = (element == CE_RadioButton);

            // Draw prelight background
            GtkWidget *gtkRadioButton = d->gtkWidget("GtkRadioButton");

            if (option->state & State_MouseOver) {
                gtkPainter->paintFlatBox(gtkRadioButton, "checkbutton", option->rect,
                                         GTK_STATE_PRELIGHT, GTK_SHADOW_ETCHED_OUT, d->gtk_widget_get_style(gtkRadioButton));
            }

            QStyleOptionButton subopt = *btn;
            subopt.rect = subElementRect(isRadio ? SE_RadioButtonIndicator
                                         : SE_CheckBoxIndicator, btn, widget);
            proxy()->drawPrimitive(isRadio ? PE_IndicatorRadioButton : PE_IndicatorCheckBox,
                          &subopt, painter, widget);
            subopt.rect = subElementRect(isRadio ? SE_RadioButtonContents
                                         : SE_CheckBoxContents, btn, widget);
            // Get label text color
            QPalette pal = subopt.palette;
            int labelState = GTK_STATE_INSENSITIVE;
            if (option->state & State_Enabled)
                labelState = (option->state & State_MouseOver) ? GTK_STATE_PRELIGHT : GTK_STATE_NORMAL;

            GdkColor gdkText = d->gtk_widget_get_style(gtkRadioButton)->fg[labelState];
            QColor textColor = QColor(gdkText.red>>8, gdkText.green>>8, gdkText.blue>>8);
            pal.setBrush(QPalette::WindowText, textColor);
            subopt.palette = pal;
            proxy()->drawControl(isRadio ? CE_RadioButtonLabel : CE_CheckBoxLabel, &subopt, painter, widget);

            if (btn->state & State_HasFocus) {
                QStyleOptionFocusRect fropt;
                fropt.QStyleOption::operator=(*btn);
                fropt.rect = subElementRect(isRadio ? SE_RadioButtonFocusRect
                                            : SE_CheckBoxFocusRect, btn, widget);
                proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
            }
        }
        break;

#ifndef QT_NO_COMBOBOX

    case CE_ComboBoxLabel:
        if (const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            QRect editRect = proxy()->subControlRect(CC_ComboBox, cb, SC_ComboBoxEditField, widget);
            bool appearsAsList = !proxy()->styleHint(QStyle::SH_ComboBox_Popup, cb, widget);
            painter->save();
            painter->setClipRect(editRect);

            if (!cb->currentIcon.isNull()) {
                QIcon::Mode mode = cb->state & State_Enabled ? QIcon::Normal
                                   : QIcon::Disabled;
                QPixmap pixmap = cb->currentIcon.pixmap(cb->iconSize, mode);
                QRect iconRect(editRect);
                iconRect.setWidth(cb->iconSize.width() + 4);

                iconRect = alignedRect(cb->direction,
                                       Qt::AlignLeft | Qt::AlignVCenter,
                                       iconRect.size(), editRect);

                if (cb->editable)
                    painter->fillRect(iconRect, option->palette.brush(QPalette::Base));

                proxy()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, pixmap);

                if (cb->direction == Qt::RightToLeft)
                    editRect.translate(-4 - cb->iconSize.width(), 0);
                else
                    editRect.translate(cb->iconSize.width() + 4, 0);
            }

            if (!cb->currentText.isEmpty() && !cb->editable) {
                GtkWidget *gtkCombo = d->gtkWidget("GtkComboBox");
                QPalette pal = cb->palette;
                int labelState = GTK_STATE_INSENSITIVE;

                if (option->state & State_Enabled)
                    labelState = (option->state & State_MouseOver && !appearsAsList) ? GTK_STATE_PRELIGHT : GTK_STATE_NORMAL;

                GdkColor gdkText = d->gtk_widget_get_style(gtkCombo)->fg[labelState];

                QColor textColor = QColor(gdkText.red>>8, gdkText.green>>8, gdkText.blue>>8);

                pal.setBrush(QPalette::ButtonText, textColor);

                proxy()->drawItemText(painter, editRect.adjusted(1, 0, -1, 0),
                             visualAlignment(cb->direction, Qt::AlignLeft | Qt::AlignVCenter),
                             pal, cb->state & State_Enabled, cb->currentText, QPalette::ButtonText);
            }

            painter->restore();
        }
        break;

#endif // QT_NO_COMBOBOX

    case CE_DockWidgetTitle:
        painter->save();
        if (const QStyleOptionDockWidget *dwOpt = qstyleoption_cast<const QStyleOptionDockWidget *>(option)) {
            const QStyleOptionDockWidgetV2 *v2
                = qstyleoption_cast<const QStyleOptionDockWidgetV2*>(dwOpt);
            bool verticalTitleBar = v2 == 0 ? false : v2->verticalTitleBar;

            QRect rect = dwOpt->rect;
            QRect titleRect = subElementRect(SE_DockWidgetTitleBarText, option, widget).adjusted(-2, 0, -2, 0);
            QRect r = rect.adjusted(0, 0, -1, -1);
            if (verticalTitleBar)
                r.adjust(0, 0, 0, -1);

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



    case CE_HeaderSection:
        painter->save();

        // Draws the header in tables.
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            Q_UNUSED(header);
            GtkWidget *gtkTreeView = d->gtkWidget("GtkTreeView");
            // Get the middle column
            GtkTreeViewColumn *column = d->gtk_tree_view_get_column((GtkTreeView*)gtkTreeView, 1);
            Q_ASSERT(column);

            GtkWidget *gtkTreeHeader = column->button;
            GtkStateType state = qt_gtk_state(option);
            GtkShadowType shadow = GTK_SHADOW_OUT;

            if (option->state & State_Sunken)
                shadow = GTK_SHADOW_IN;

            gtkPainter->paintBox(gtkTreeHeader, "button", option->rect.adjusted(-1, 0, 0, 0), state, shadow, d->gtk_widget_get_style(gtkTreeHeader));
        }

        painter->restore();
        break;

#ifndef QT_NO_SIZEGRIP

    case CE_SizeGrip: {
        GtkWidget *gtkStatusbar = d->gtkWidget("GtkStatusbar.GtkFrame");
        GtkStyle *gtkStatusbarStyle = d->gtk_widget_get_style(gtkStatusbar);
        QRect gripRect = option->rect.adjusted(0, 0, -gtkStatusbarStyle->xthickness, -gtkStatusbarStyle->ythickness);
        gtkPainter->paintResizeGrip(gtkStatusbar, "statusbar", gripRect, GTK_STATE_NORMAL,
                                    GTK_SHADOW_OUT, option->direction == Qt::RightToLeft ?
                                    GDK_WINDOW_EDGE_SOUTH_WEST : GDK_WINDOW_EDGE_SOUTH_EAST,
                                    gtkStatusbarStyle);
    }
    break;

#endif // QT_NO_SIZEGRIP

    case CE_MenuBarEmptyArea: {
        GtkWidget *gtkMenubar = d->gtkWidget("GtkMenuBar");
        GdkColor gdkBg = d->gtk_widget_get_style(gtkMenubar)->bg[GTK_STATE_NORMAL]; // Theme can depend on transparency
        painter->fillRect(option->rect, QColor(gdkBg.red>>8, gdkBg.green>>8, gdkBg.blue>>8));
        if (widget) { // See CE_MenuBarItem
            QRect menuBarRect = widget->rect();
            QPixmap pixmap(menuBarRect.size());
            pixmap.fill(Qt::transparent);
            QPainter pmPainter(&pixmap);
            gtkPainter->reset(&pmPainter);
            GtkShadowType shadow_type;
            d->gtk_widget_style_get(gtkMenubar, "shadow-type", &shadow_type, NULL);
            gtkPainter->paintBox(gtkMenubar, "menubar", menuBarRect,
                                 GTK_STATE_NORMAL, shadow_type, d->gtk_widget_get_style(gtkMenubar));
            pmPainter.end();
            painter->drawPixmap(option->rect, pixmap, option->rect);
            gtkPainter->reset(painter);
        }
    }
    break;

    case CE_MenuBarItem:
        painter->save();

        if (const QStyleOptionMenuItem *mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            GtkWidget *gtkMenubarItem = d->gtkWidget("GtkMenuBar.GtkMenuItem");
            GtkWidget *gtkMenubar = d->gtkWidget("GtkMenuBar");

            style = d->gtk_widget_get_style(gtkMenubarItem);

            if (widget) {
                // Since Qt does not currently allow filling the entire background
                // we use a hack for this by making a complete menubar each time and
                // paint with the correct offset inside it. Pixmap caching should resolve
                // most of the performance penalty.
                QRect menuBarRect = widget->rect();
                QPixmap pixmap(menuBarRect.size());
                pixmap.fill(Qt::transparent);
                QPainter pmPainter(&pixmap);
                gtkPainter->reset(&pmPainter);
                GtkShadowType shadow_type;
                d->gtk_widget_style_get(gtkMenubar, "shadow-type", &shadow_type, NULL);
                GdkColor gdkBg = d->gtk_widget_get_style(gtkMenubar)->bg[GTK_STATE_NORMAL]; // Theme can depend on transparency
                painter->fillRect(option->rect, QColor(gdkBg.red>>8, gdkBg.green>>8, gdkBg.blue>>8));
                gtkPainter->paintBox(gtkMenubar, "menubar", menuBarRect,
                                     GTK_STATE_NORMAL, shadow_type, d->gtk_widget_get_style(gtkMenubar));
                pmPainter.end();
                painter->drawPixmap(option->rect, pixmap, option->rect);
                gtkPainter->reset(painter);
            }

            QStyleOptionMenuItem item = *mbi;
            bool act = mbi->state & State_Selected && mbi->state & State_Sunken;
            bool dis = !(mbi->state & State_Enabled);
            item.rect = mbi->rect;
            GdkColor gdkText = style->fg[dis ? GTK_STATE_INSENSITIVE : GTK_STATE_NORMAL];
            GdkColor gdkHText = style->fg[GTK_STATE_PRELIGHT];
            QColor normalTextColor = QColor(gdkText.red>>8, gdkText.green>>8, gdkText.blue>>8);
            QColor highlightedTextColor = QColor(gdkHText.red>>8, gdkHText.green>>8, gdkHText.blue>>8);
            item.palette.setBrush(QPalette::HighlightedText, highlightedTextColor);
            item.palette.setBrush(QPalette::Text, normalTextColor);
            item.palette.setBrush(QPalette::ButtonText, normalTextColor);
            QCommonStyle::drawControl(element, &item, painter, widget);

            if (act) {
                GtkShadowType shadowType = GTK_SHADOW_NONE;
                d->gtk_widget_style_get (gtkMenubarItem, "selected-shadow-type", &shadowType, NULL);
                gtkPainter->paintBox(gtkMenubarItem, "menuitem", option->rect.adjusted(0, 0, 0, 3),
                                     GTK_STATE_PRELIGHT, shadowType, style);
                //draw text
                QPalette::ColorRole textRole = dis ? QPalette::Text : QPalette::HighlightedText;
                uint alignment = Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;

                if (!proxy()->styleHint(SH_UnderlineShortcut, mbi, widget))
                    alignment |= Qt::TextHideMnemonic;

                proxy()->drawItemText(painter, item.rect, alignment, item.palette, mbi->state & State_Enabled, mbi->text, textRole);
            }
        }
        painter->restore();
        break;

    case CE_Splitter: {
        GtkWidget *gtkWindow = d->gtkWidget("GtkWindow"); // The Murrine Engine currently assumes a widget is passed
        gtkPainter->paintHandle(gtkWindow, "splitter", option->rect, qt_gtk_state(option), GTK_SHADOW_NONE,
                                !(option->state & State_Horizontal) ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL,
                                style);
    }
    break;

#ifndef QT_NO_TOOLBAR

    case CE_ToolBar:
        if (const QStyleOptionToolBar *toolbar = qstyleoption_cast<const QStyleOptionToolBar *>(option)) {
            // Reserve the beveled appearance only for mainwindow toolbars
            if (!(widget && qobject_cast<const QMainWindow*> (widget->parentWidget())))
                break;

            QRect rect = option->rect;
            // There is a 1 pixel gap between toolbar lines in some styles (i.e Human)
            if (toolbar->positionWithinLine != QStyleOptionToolBar::End)
                rect.adjust(0, 0, 1, 0);

            GtkWidget *gtkToolbar = d->gtkWidget("GtkToolbar");
            GtkShadowType shadow_type = GTK_SHADOW_NONE;
            d->gtk_widget_style_get(gtkToolbar, "shadow-type", &shadow_type, NULL);
            gtkPainter->paintBox(gtkToolbar, "toolbar", rect,
                                 GTK_STATE_NORMAL, shadow_type, d->gtk_widget_get_style(gtkToolbar));
        }
        break;

#endif // QT_NO_TOOLBAR

    case CE_MenuItem:
        painter->save();

        // Draws one item in a popup menu.
        if (const QStyleOptionMenuItem *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            const int windowsItemHMargin      =  3; // menu item hor text margin
            const int windowsItemVMargin      = 26; // menu item ver text margin
            GtkWidget *gtkMenuItem = menuItem->checked ? d->gtkWidget("GtkMenu.GtkCheckMenuItem") :
                                     d->gtkWidget("GtkMenu.GtkMenuItem");

            style = d->gtk_widget_get_style(gtkMenuItem);
            QColor shadow = option->palette.dark().color();

            if (menuItem->menuItemType == QStyleOptionMenuItem::Separator) {
                GtkWidget *gtkMenuSeparator = d->gtkWidget("GtkMenu.GtkSeparatorMenuItem");
                painter->setPen(shadow.lighter(106));
                gboolean wide_separators = 0;
                gint     separator_height = 0;
                guint    horizontal_padding = 3;
                QRect separatorRect = option->rect;
                if (!d->gtk_check_version(2, 10, 0)) {
                    d->gtk_widget_style_get(gtkMenuSeparator,
                                           "wide-separators",    &wide_separators,
                                           "separator-height",   &separator_height,
                                           "horizontal-padding", &horizontal_padding,
                                           NULL);
                }
                GtkStyle *gtkMenuSeparatorStyle = d->gtk_widget_get_style(gtkMenuSeparator);
                separatorRect.setHeight(option->rect.height() - 2 * gtkMenuSeparatorStyle->ythickness);
                separatorRect.setWidth(option->rect.width() - 2 * (horizontal_padding + gtkMenuSeparatorStyle->xthickness));
                separatorRect.moveCenter(option->rect.center());
                if (wide_separators)
                   gtkPainter->paintBox(gtkMenuSeparator, "hseparator",
                                        separatorRect, GTK_STATE_NORMAL, GTK_SHADOW_NONE, gtkMenuSeparatorStyle);
                else
                    gtkPainter->paintHline(gtkMenuSeparator, "hseparator",
                                           separatorRect, GTK_STATE_NORMAL, gtkMenuSeparatorStyle,
                                           0, option->rect.right() - 1, 1);
                painter->restore();
                break;
            }

            bool selected = menuItem->state & State_Selected && menuItem->state & State_Enabled;

            if (selected) {
                QRect rect = option->rect;
#ifndef QT_NO_COMBOBOX
                if (qobject_cast<const QComboBox*>(widget))
                    rect = option->rect;
#endif
                gtkPainter->paintBox(gtkMenuItem, "menuitem", rect, GTK_STATE_PRELIGHT, GTK_SHADOW_OUT, style);
            }

            bool checkable = menuItem->checkType != QStyleOptionMenuItem::NotCheckable;
            bool checked = menuItem->checked;
            bool enabled = menuItem->state & State_Enabled;
            bool ignoreCheckMark = false;

            gint checkSize;
            d->gtk_widget_style_get(d->gtkWidget("GtkMenu.GtkCheckMenuItem"), "indicator-size", &checkSize, NULL);

            int checkcol = qMax(menuItem->maxIconWidth, qMax(20, checkSize));

#ifndef QT_NO_COMBOBOX

            if (qobject_cast<const QComboBox*>(widget) ||
                (option->styleObject && option->styleObject->property("_q_isComboBoxPopupItem").toBool()))
                ignoreCheckMark = true; // Ignore the checkmarks provided by the QComboMenuDelegate

#endif
            if (!ignoreCheckMark) {
                // Check
                QRect checkRect(option->rect.left() + 7, option->rect.center().y() - checkSize/2 + 1, checkSize, checkSize);
                checkRect = visualRect(menuItem->direction, menuItem->rect, checkRect);

                if (checkable && menuItem->icon.isNull()) {
                    // Some themes such as aero-clone draw slightly outside the paint rect
                    int spacing = 1; // ### Consider using gtkCheckBox : "indicator-spacing" instead

                    if (menuItem->checkType & QStyleOptionMenuItem::Exclusive) {
                        // Radio button
                        GtkShadowType shadow = GTK_SHADOW_OUT;
                        GtkStateType state = qt_gtk_state(option);

                        if (selected)
                            state = GTK_STATE_PRELIGHT;
                        if (checked)
                            shadow = GTK_SHADOW_IN;

                        gtkPainter->setClipRect(checkRect.adjusted(-spacing, -spacing, spacing, spacing));
                        gtkPainter->paintOption(gtkMenuItem, checkRect.translated(-spacing, -spacing), state, shadow,
                                                style, QLS("option"));
                        gtkPainter->setClipRect(QRect());

                    } else {
                        // Check box
                        if (menuItem->icon.isNull()) {
                            GtkShadowType shadow = GTK_SHADOW_OUT;
                            GtkStateType state = qt_gtk_state(option);

                            if (selected)
                                state = GTK_STATE_PRELIGHT;
                            if (checked)
                                shadow = GTK_SHADOW_IN;

                            gtkPainter->setClipRect(checkRect.adjusted(-spacing, -spacing, -spacing, -spacing));
                            gtkPainter->paintCheckbox(gtkMenuItem, checkRect.translated(-spacing, -spacing), state, shadow,
                                                      style, QLS("check"));
                            gtkPainter->setClipRect(QRect());
                        }
                    }
                }

            } else {
                // Ignore checkmark
                if (menuItem->icon.isNull())
                    checkcol = 0;
                else
                    checkcol = menuItem->maxIconWidth;
            }

            bool dis = !(menuItem->state & State_Enabled);
            bool act = menuItem->state & State_Selected;
            const QStyleOption *opt = option;
            const QStyleOptionMenuItem *menuitem = menuItem;
            QPainter *p = painter;
            QRect vCheckRect = visualRect(opt->direction, menuitem->rect,
                                          QRect(menuitem->rect.x() + 3, menuitem->rect.y(),
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
                pmr.moveCenter(vCheckRect.center() - QPoint(0, 1));
                painter->setPen(menuItem->palette.text().color());
                if (!ignoreCheckMark && checkable && checked) {
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

            GdkColor gdkText = style->fg[GTK_STATE_NORMAL];
            GdkColor gdkDText = style->fg[GTK_STATE_INSENSITIVE];
            GdkColor gdkHText = style->fg[GTK_STATE_PRELIGHT];
            uint resolve_mask = option->palette.resolve();
            QColor textColor = QColor(gdkText.red>>8, gdkText.green>>8, gdkText.blue>>8);
            QColor disabledTextColor = QColor(gdkDText.red>>8, gdkDText.green>>8, gdkDText.blue>>8);
            if (resolve_mask & (1 << QPalette::ButtonText)) {
                textColor = option->palette.buttonText().color();
                disabledTextColor = option->palette.brush(QPalette::Disabled, QPalette::ButtonText).color();
            }

            QColor highlightedTextColor = QColor(gdkHText.red>>8, gdkHText.green>>8, gdkHText.blue>>8);
            if (resolve_mask & (1 << QPalette::HighlightedText)) {
                highlightedTextColor = option->palette.highlightedText().color();
            }

            if (selected)
                painter->setPen(highlightedTextColor);
            else
                painter->setPen(textColor);

            int x, y, w, h;
            menuitem->rect.getRect(&x, &y, &w, &h);
            int tab = menuitem->tabWidth;
            int xm = QGtkStylePrivate::menuItemFrame + checkcol + windowsItemHMargin;
            int xpos = menuitem->rect.x() + xm + 1;
            QRect textRect(xpos, y + windowsItemVMargin, w - xm - QGtkStylePrivate::menuRightBorder - tab + 1, h - 2 * windowsItemVMargin);
            QRect vTextRect = visualRect(opt->direction, menuitem->rect, textRect);
            QString s = menuitem->text;

            if (!s.isEmpty()) { // Draw text
                p->save();
                int t = s.indexOf(QLatin1Char('\t'));
                int text_flags = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;

                if (!proxy()->styleHint(SH_UnderlineShortcut, menuitem, widget))
                    text_flags |= Qt::TextHideMnemonic;

                // Draw shortcut right aligned
                text_flags |= Qt::AlignRight;

                if (t >= 0) {
                    int rightMargin = 12; // Hardcode for now
                    QRect vShortcutRect = visualRect(opt->direction, menuitem->rect,
                                                     QRect(textRect.topRight(), QPoint(menuitem->rect.right() - rightMargin, textRect.bottom())));

                    if (dis)
                        p->setPen(disabledTextColor);
                    p->drawText(vShortcutRect, text_flags , s.mid(t + 1));
                    s = s.left(t);
                }

                text_flags &= ~Qt::AlignRight;
                text_flags |= Qt::AlignLeft;
                QFont font = menuitem->font;
                if (menuitem->menuItemType == QStyleOptionMenuItem::DefaultItem)
                    font.setBold(true);
                p->setFont(font);

                if (dis)
                    p->setPen(disabledTextColor);
                p->drawText(vTextRect, text_flags, s.left(t));
                p->restore();
            }

            // Arrow
            if (menuItem->menuItemType == QStyleOptionMenuItem::SubMenu) {// draw sub menu arrow

                QFontMetrics fm(menuitem->font);
                int arrow_size = fm.ascent() + fm.descent() - 2 * style->ythickness;
                gfloat arrow_scaling = 0.8;
                int extra = 0;
                if (!d->gtk_check_version(2, 16, 0)) {
                    // "arrow-scaling" is actually hardcoded and fails on hardy (see gtk+-2.12/gtkmenuitem.c)
                    // though the current documentation states otherwise
                    d->gtk_widget_style_get(gtkMenuItem, "arrow-scaling", &arrow_scaling, NULL);
                    // in versions < 2.16 ythickness was previously subtracted from the arrow_size
                    extra = 2 * style->ythickness;
                }

                int horizontal_padding;
                d->gtk_widget_style_get(gtkMenuItem, "horizontal-padding", &horizontal_padding, NULL);

                const int dim = static_cast<int>(arrow_size * arrow_scaling) + extra;
                int xpos = menuItem->rect.left() + menuItem->rect.width() - horizontal_padding - dim;
                QRect  vSubMenuRect = visualRect(option->direction, menuItem->rect,
                                                 QRect(xpos, menuItem->rect.top() +
                                                       menuItem->rect.height() / 2 - dim / 2, dim, dim));
                GtkStateType state = enabled ? (act ? GTK_STATE_PRELIGHT: GTK_STATE_NORMAL) : GTK_STATE_INSENSITIVE;
                GtkShadowType shadowType = (state == GTK_STATE_PRELIGHT) ? GTK_SHADOW_OUT : GTK_SHADOW_IN;
                gtkPainter->paintArrow(gtkMenuItem, "menuitem", vSubMenuRect, option->direction == Qt::RightToLeft ? GTK_ARROW_LEFT : GTK_ARROW_RIGHT, state,
                                       shadowType, false, style);
            }
        }
        painter->restore();
        break;

    case CE_PushButton:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            GtkWidget *gtkButton = d->gtkWidget("GtkButton");
            proxy()->drawControl(CE_PushButtonBevel, btn, painter, widget);
            QStyleOptionButton subopt = *btn;
            subopt.rect = subElementRect(SE_PushButtonContents, btn, widget);
            gint interiorFocus = true;
            d->gtk_widget_style_get(gtkButton, "interior-focus", &interiorFocus, NULL);
            GtkStyle *gtkButtonStyle = d->gtk_widget_get_style(gtkButton);
            int xt = interiorFocus ? gtkButtonStyle->xthickness : 0;
            int yt = interiorFocus ? gtkButtonStyle->ythickness : 0;

            if (btn->features & QStyleOptionButton::Flat && btn->state & State_HasFocus)
                // The normal button focus rect does not work well for flat buttons in Clearlooks
                proxy()->drawPrimitive(PE_FrameFocusRect, option, painter, widget);
            else if (btn->state & State_HasFocus)
                gtkPainter->paintFocus(gtkButton, "button",
                                       option->rect.adjusted(xt, yt, -xt, -yt),
                                       btn->state & State_Sunken ? GTK_STATE_ACTIVE : GTK_STATE_NORMAL,
                                       gtkButtonStyle);

            proxy()->drawControl(CE_PushButtonLabel, &subopt, painter, widget);
        }
        break;

#ifndef QT_NO_TABBAR

    case CE_TabBarTabShape:
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {
            GtkWidget *gtkNotebook = d->gtkWidget("GtkNotebook");
            style = d->gtk_widget_get_style(gtkNotebook);

            QRect rect = option->rect;
            GtkShadowType shadow = GTK_SHADOW_OUT;
            GtkStateType state = GTK_STATE_ACTIVE;
            if (tab->state & State_Selected)
                state = GTK_STATE_NORMAL;

            bool selected = (tab->state & State_Selected);
            bool first = false, last = false;
            if (widget) {
                // This is most accurate and avoids resizing tabs while moving
                first = tab->rect.left() == widget->rect().left();
                last = tab->rect.right() == widget->rect().right();
            } else if (option->direction == Qt::RightToLeft) {
                bool tmp = first;
                first = last;
                last = tmp;
            }
            int topIndent = 3;
            int bottomIndent = 1;
            int tabOverlap = 1;
            painter->save();

            switch (tab->shape) {
            case QTabBar::RoundedNorth:
                if (!selected)
                    rect.adjust(first ? 0 : -tabOverlap, topIndent, last ? 0 : tabOverlap, -bottomIndent);
                gtkPainter->paintExtention(gtkNotebook, "tab", rect,
                                           state, shadow, GTK_POS_BOTTOM, style);
                break;

            case QTabBar::RoundedSouth:
                if (!selected)
                    rect.adjust(first ? 0 : -tabOverlap, 0, last ? 0 : tabOverlap, -topIndent);
                gtkPainter->paintExtention(gtkNotebook, "tab", rect.adjusted(0, 1, 0, 0),
                                           state, shadow, GTK_POS_TOP, style);
                break;

            case QTabBar::RoundedWest:
                if (!selected)
                    rect.adjust(topIndent, 0, -bottomIndent, 0);
                gtkPainter->paintExtention(gtkNotebook, "tab", rect, state, shadow, GTK_POS_RIGHT, style);
                break;

            case QTabBar::RoundedEast:
                if (!selected)
                    rect.adjust(bottomIndent, 0, -topIndent, 0);
                gtkPainter->paintExtention(gtkNotebook, "tab", rect, state, shadow, GTK_POS_LEFT, style);
                break;

            default:
                QCommonStyle::drawControl(element, option, painter, widget);
                break;
            }

            painter->restore();
        }

        break;

#endif //QT_NO_TABBAR

    case CE_ProgressBarGroove:
        if (const QStyleOptionProgressBar *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            Q_UNUSED(bar);
            GtkWidget *gtkProgressBar = d->gtkWidget("GtkProgressBar");
            GtkStateType state = qt_gtk_state(option);
            gtkPainter->paintBox(gtkProgressBar, "trough", option->rect, state, GTK_SHADOW_IN, d->gtk_widget_get_style(gtkProgressBar));
        }

        break;

    case CE_ProgressBarContents:
        if (const QStyleOptionProgressBar *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            GtkStateType state = option->state & State_Enabled ? GTK_STATE_NORMAL : GTK_STATE_INSENSITIVE;
            GtkWidget *gtkProgressBar = d->gtkWidget("GtkProgressBar");
            style = d->gtk_widget_get_style(gtkProgressBar);
            gtkPainter->paintBox(gtkProgressBar, "trough", option->rect, state, GTK_SHADOW_IN, style);
            int xt = style->xthickness;
            int yt = style->ythickness;
            QRect rect = bar->rect.adjusted(xt, yt, -xt, -yt);
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
                rect.translate(xt, -yt * 2);
                rect = QRect(rect.left(), rect.top(), rect.height(), rect.width()); // Flip width and height
                QTransform m = QTransform::fromTranslate(rect.height(), 0);
                m.rotate(90.0);
                painter->setTransform(m);
            }

            int maxWidth = rect.width();
            int minWidth = 4;

            qint64 progress = (qint64)qMax(bar->progress, bar->minimum); // Workaround for bug in QProgressBar
            double vc6_workaround = ((progress - qint64(bar->minimum)) / double(qint64(bar->maximum) - qint64(bar->minimum))) * maxWidth;
            int progressBarWidth = (int(vc6_workaround) > minWidth ) ? int(vc6_workaround) : minWidth;
            int width = indeterminate ? maxWidth : progressBarWidth;
            bool reverse = (!vertical && (bar->direction == Qt::RightToLeft)) || vertical;

            if (inverted)
                reverse = !reverse;

            int maximum = 2;
            int fakePos = 0;
            if (bar->minimum == bar->maximum)
                maximum = 0;
            if (bar->progress == bar->maximum)
                fakePos = maximum;
            else if (bar->progress > bar->minimum)
                fakePos = maximum - 1;

            QRect progressBar;

            if (!indeterminate) {
                if (!reverse)
                    progressBar.setRect(rect.left(), rect.top(), width, rect.height());
                else
                    progressBar.setRect(rect.right() - width, rect.top(), width, rect.height());
                d->stopAnimation(option->styleObject);
            } else {
                Q_D(const QGtkStyle);
                int slideWidth = ((rect.width() - 4) * 2) / 3;
                int step = 0;
                if (QProgressStyleAnimation *animation = qobject_cast<QProgressStyleAnimation*>(d->animation(option->styleObject)))
                    step = animation->progressStep(slideWidth);
                else
                    d->startAnimation(new QProgressStyleAnimation(d->animationFps, option->styleObject));
                progressBar.setRect(rect.left() + step, rect.top(), slideWidth / 2, rect.height());
            }

            QString key = QString(QLS("%0")).arg(fakePos);
            if (inverted) {
                key += QLatin1String("inv");
                gtkPainter->setFlipHorizontal(true);
            }
            gtkPainter->paintBox(gtkProgressBar, "bar", progressBar, GTK_STATE_SELECTED, GTK_SHADOW_OUT, style, key);
        }

        break;

    default:
        QCommonStyle::drawControl(element, option, painter, widget);
    }
}

/*!
  \reimp
*/
QRect QGtkStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                                SubControl subControl, const QWidget *widget) const
{
    Q_D(const QGtkStyle);

    QRect rect = QCommonStyle::subControlRect(control, option, subControl, widget);
    if (!d->isThemeAvailable())
        return QCommonStyle::subControlRect(control, option, subControl, widget);

    switch (control) {
    case CC_ScrollBar:
        break;
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

#ifndef QT_NO_GROUPBOX

    case CC_GroupBox:
        if (const QStyleOptionGroupBox * groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            rect = option->rect.adjusted(0, groupBoxTopMargin, 0, -groupBoxBottomMargin);
            int topMargin = 0;
            int topHeight = 0;
            topHeight = 10;
            QRect frameRect = rect;
            frameRect.setTop(topMargin);

            if (subControl == SC_GroupBoxFrame)
                return rect;
            else if (subControl == SC_GroupBoxContents) {
                int margin = 0;
                int leftMarginExtension = 8;
                return frameRect.adjusted(leftMarginExtension + margin, margin + topHeight + groupBoxTitleMargin, -margin, -margin);
            }

            QFontMetrics fontMetrics = option->fontMetrics;
            if (qobject_cast<const QGroupBox *>(widget)) {
                //Prepare metrics for a bold font
                QFont font = widget->font();
                font.setBold(true);
                fontMetrics = QFontMetrics(font);
            }

            QSize textRect = fontMetrics.boundingRect(groupBox->text).size() + QSize(4, 4);
            int indicatorWidth = proxy()->pixelMetric(PM_IndicatorWidth, option, widget);
            int indicatorHeight = proxy()->pixelMetric(PM_IndicatorHeight, option, widget);

            if (subControl == SC_GroupBoxCheckBox) {
                rect.setWidth(indicatorWidth);
                rect.setHeight(indicatorHeight);
                rect.moveTop((textRect.height() - indicatorHeight) / 2);

            } else if (subControl == SC_GroupBoxLabel) {
                if (groupBox->subControls & SC_GroupBoxCheckBox)
                    rect.adjust(indicatorWidth + 4, 0, 0, 0);
                rect.setSize(textRect);
            }
            rect = visualRect(option->direction, option->rect, rect);
        }

        return rect;

#endif
#ifndef QT_NO_SPINBOX

    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spinbox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            GtkWidget *gtkSpinButton = d->gtkWidget("GtkSpinButton");
            int center = spinbox->rect.height() / 2;
            GtkStyle *gtkSpinButtonStyle = d->gtk_widget_get_style(gtkSpinButton);
            int xt = spinbox->frame ? gtkSpinButtonStyle->xthickness : 0;
            int yt = spinbox->frame ? gtkSpinButtonStyle->ythickness : 0;
            int y = yt;

            QSize bs;
            bs.setHeight(qMax(8, spinbox->rect.height()/2 - y));
            bs.setWidth(d->getSpinboxArrowSize());
            int x, lx, rx;
            x = spinbox->rect.width() - y - bs.width() + 2;
            lx = xt;
            rx = x - xt;

            switch (subControl) {

            case SC_SpinBoxUp:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();
                rect = QRect(x, xt, bs.width(), center - yt);
                break;

            case SC_SpinBoxDown:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();
                rect = QRect(x, center, bs.width(), spinbox->rect.bottom() - center - yt + 1);
                break;

            case SC_SpinBoxEditField:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    rect = QRect(lx, yt, spinbox->rect.width() - 2*xt, spinbox->rect.height() - 2*yt);
                else
                    rect = QRect(lx, yt, rx - qMax(xt - 1, 0), spinbox->rect.height() - 2*yt);
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
#ifndef QT_NO_COMBOBOX

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
    case CC_ComboBox:
        if (const QStyleOptionComboBox *box = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            // We employ the gtk widget to position arrows and separators for us
            GtkWidget *gtkCombo = box->editable ? d->gtkWidget("GtkComboBoxEntry")
                                                : d->gtkWidget("GtkComboBox");
            d->gtk_widget_set_direction(gtkCombo, (option->direction == Qt::RightToLeft) ? GTK_TEXT_DIR_RTL : GTK_TEXT_DIR_LTR);
            GtkAllocation geometry = {0, 0, qMax(0, option->rect.width()), qMax(0, option->rect.height())};
            d->gtk_widget_size_allocate(gtkCombo, &geometry);
            int appears_as_list = !proxy()->styleHint(QStyle::SH_ComboBox_Popup, option, widget);
            QHashableLatin1Literal arrowPath("GtkComboBoxEntry.GtkToggleButton");
            if (!box->editable) {
                if (appears_as_list)
                    arrowPath = "GtkComboBox.GtkToggleButton";
                else
                    arrowPath = "GtkComboBox.GtkToggleButton.GtkHBox.GtkArrow";
            }

            GtkWidget *arrowWidget = d->gtkWidget(arrowPath);
            if (!arrowWidget)
                return QCommonStyle::subControlRect(control, option, subControl, widget);

            GtkAllocation allocation;
            d->gtk_widget_get_allocation(arrowWidget, &allocation);
            QRect buttonRect(option->rect.left() + allocation.x,
                             option->rect.top() + allocation.y,
                             allocation.width, allocation.height);

            switch (subControl) {

            case SC_ComboBoxArrow: // Note: this indicates the arrowbutton for editable combos
                rect = buttonRect;
                break;

            case SC_ComboBoxEditField: {
                rect = visualRect(option->direction, option->rect, rect);
                int xMargin = box->editable ? 1 : 4, yMargin = 2;
                GtkStyle *gtkComboStyle = d->gtk_widget_get_style(gtkCombo);
                rect.setRect(option->rect.left() + gtkComboStyle->xthickness + xMargin,
                             option->rect.top()  + gtkComboStyle->ythickness + yMargin,
                             option->rect.width() - buttonRect.width() - 2*(gtkComboStyle->xthickness + xMargin),
                             option->rect.height() - 2*(gtkComboStyle->ythickness + yMargin));
                rect = visualRect(option->direction, option->rect, rect);
                break;
            }

            default:
                break;
            }
        }

        break;
#endif // QT_NO_COMBOBOX

    default:
        break;
    }

    return rect;
}

/*!
  \reimp
*/
QSize QGtkStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
                                  const QSize &size, const QWidget *widget) const
{
    Q_D(const QGtkStyle);

    QSize newSize = QCommonStyle::sizeFromContents(type, option, size, widget);
    if (!d->isThemeAvailable())
        return newSize;

    switch (type) {
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
        newSize += QSize(4, 1 + groupBoxBottomMargin + groupBoxTopMargin + groupBoxTitleMargin); // Add some space below the groupbox
        break;
    case CT_ToolButton:
        if (const QStyleOptionToolButton *toolbutton = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            GtkWidget *gtkButton = d->gtkWidget("GtkToolButton.GtkButton");
            GtkStyle *gtkButtonStyle = d->gtk_widget_get_style(gtkButton);
            newSize = size + QSize(2 * gtkButtonStyle->xthickness, 2 + 2 * gtkButtonStyle->ythickness);
            if (widget && qobject_cast<QToolBar *>(widget->parentWidget())) {
                QSize minSize(0, 25);
                if (toolbutton->toolButtonStyle != Qt::ToolButtonTextOnly)
                    minSize = toolbutton->iconSize + QSize(12, 12);
                newSize = newSize.expandedTo(minSize);
            }

            if (toolbutton->features & QStyleOptionToolButton::HasMenu)
                newSize += QSize(6, 0);
        }
        break;
    case CT_SpinBox:
        // QSpinBox does some nasty things that depends on CT_LineEdit
        newSize = newSize + QSize(0, -d->gtk_widget_get_style(d->gtkWidget("GtkSpinButton"))->ythickness * 2);
        break;
    case CT_RadioButton:
    case CT_CheckBox:
        newSize += QSize(0, 1);
        break;
    case CT_PushButton:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            if (!btn->icon.isNull() && btn->iconSize.height() > 16)
                newSize -= QSize(0, 2); // From cleanlooksstyle
            newSize += QSize(0, 1);
            GtkWidget *gtkButton = d->gtkWidget("GtkButton");
            gint focusPadding, focusWidth;
            d->gtk_widget_style_get(gtkButton, "focus-padding", &focusPadding, NULL);
            d->gtk_widget_style_get(gtkButton, "focus-line-width", &focusWidth, NULL);
            newSize = size;
            GtkStyle *gtkButtonStyle = d->gtk_widget_get_style(gtkButton);
            newSize += QSize(2*gtkButtonStyle->xthickness + 4, 2*gtkButtonStyle->ythickness);
            newSize += QSize(2*(focusWidth + focusPadding + 2), 2*(focusWidth + focusPadding));

            GtkWidget *gtkButtonBox = d->gtkWidget("GtkHButtonBox");
            gint minWidth = 85, minHeight = 0;
            d->gtk_widget_style_get(gtkButtonBox, "child-min-width", &minWidth,
                                   "child-min-height", &minHeight, NULL);
            if (!btn->text.isEmpty() && newSize.width() < minWidth)
                newSize.setWidth(minWidth);
            if (newSize.height() < minHeight)
                newSize.setHeight(minHeight);
        }
        break;
    case CT_Slider: {
        GtkWidget *gtkSlider = d->gtkWidget("GtkHScale");
        GtkStyle *gtkSliderStyle = d->gtk_widget_get_style(gtkSlider);
        newSize = size + QSize(2*gtkSliderStyle->xthickness, 2*gtkSliderStyle->ythickness); }
        break;
    case CT_LineEdit: {
        GtkWidget *gtkEntry = d->gtkWidget("GtkEntry");
        GtkStyle *gtkEntryStyle = d->gtk_widget_get_style(gtkEntry);
        newSize = size + QSize(2*gtkEntryStyle->xthickness, 2 + 2*gtkEntryStyle->ythickness); }
        break;
    case CT_ItemViewItem:
        newSize += QSize(0, 2);
        break;
    case CT_ComboBox:
        if (const QStyleOptionComboBox *combo = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            GtkWidget *gtkCombo = d->gtkWidget("GtkComboBox");
            QRect arrowButtonRect = proxy()->subControlRect(CC_ComboBox, combo, SC_ComboBoxArrow, widget);
            GtkStyle *gtkComboStyle = d->gtk_widget_get_style(gtkCombo);
            newSize = size + QSize(12 + arrowButtonRect.width() + 2*gtkComboStyle->xthickness, 4 + 2*gtkComboStyle->ythickness);

            if (!(widget && qobject_cast<QToolBar *>(widget->parentWidget())))
                newSize += QSize(0, 2);
        }
        break;
    case CT_TabBarTab:
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {
            if (!tab->icon.isNull())
                newSize += QSize(6, 0);
        }
        newSize += QSize(1, 1);
        break;
    case CT_MenuBarItem:
        newSize += QSize(QGtkStylePrivate::menuItemHMargin * 4, QGtkStylePrivate::menuItemVMargin * 2 + 2);
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
    case CT_MenuItem:
        if (const QStyleOptionMenuItem *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            int w = newSize.width();
            int maxpmw = menuItem->maxIconWidth;
            int tabSpacing = 20;
            if (menuItem->text.contains(QLatin1Char('\t')))
                w += tabSpacing;
            else if (menuItem->menuItemType == QStyleOptionMenuItem::SubMenu)
                w += 2 * QGtkStylePrivate::menuArrowHMargin;
            else if (menuItem->menuItemType == QStyleOptionMenuItem::DefaultItem) {
                // adjust the font and add the difference in size.
                // it would be better if the font could be adjusted in the initStyleOption qmenu func!!
                QFontMetrics fm(menuItem->font);
                QFont fontBold = menuItem->font;
                fontBold.setBold(true);
                QFontMetrics fmBold(fontBold);
                w += fmBold.width(menuItem->text) - fm.width(menuItem->text);
            }

            int checkcol = qMax<int>(maxpmw, QGtkStylePrivate::menuCheckMarkWidth); // Windows always shows a check column
            w += checkcol;
            w += int(QGtkStylePrivate::menuRightBorder) + 10;

            newSize.setWidth(w);

            int textMargin = 8;
            if (menuItem->menuItemType == QStyleOptionMenuItem::Separator) {
                GtkWidget *gtkMenuSeparator = d->gtkWidget("GtkMenu.GtkSeparatorMenuItem");
                GtkRequisition sizeReq = {0, 0};
                d->gtk_widget_size_request(gtkMenuSeparator, &sizeReq);
                newSize = QSize(newSize.width(), sizeReq.height);
                break;
            }

            GtkWidget *gtkMenuItem = d->gtkWidget("GtkMenu.GtkCheckMenuItem");
            GtkStyle* style = d->gtk_widget_get_style(gtkMenuItem);

            // Note we get the perfect height for the default font since we
            // set a fake text label on the gtkMenuItem
            // But if custom fonts are used on the widget we need a minimum size
            GtkRequisition sizeReq = {0, 0};
            d->gtk_widget_size_request(gtkMenuItem, &sizeReq);
            newSize.setHeight(qMax(newSize.height() - 4, sizeReq.height));
            newSize += QSize(textMargin + style->xthickness - 1, 0);

            gint checkSize;
            d->gtk_widget_style_get(gtkMenuItem, "indicator-size", &checkSize, NULL);
            newSize.setWidth(newSize.width() + qMax(0, checkSize - 20));
        }
        break;
    default:
        break;
    }

    return newSize;
}


/*! \reimp */
QPixmap QGtkStyle::standardPixmap(StandardPixmap sp, const QStyleOption *option,
                                  const QWidget *widget) const
{
    Q_D(const QGtkStyle);

    if (!d->isThemeAvailable())
        return QCommonStyle::standardPixmap(sp, option, widget);

    QPixmap pixmap;
    switch (sp) {

    case SP_TitleBarNormalButton: {
        QImage restoreButton((const char **)dock_widget_restore_xpm);
        QColor alphaCorner = restoreButton.color(2);
        alphaCorner.setAlpha(80);
        restoreButton.setColor(2, alphaCorner.rgba());
        alphaCorner.setAlpha(180);
        restoreButton.setColor(4, alphaCorner.rgba());
        return QPixmap::fromImage(restoreButton);
    }
    break;

    case SP_TitleBarCloseButton: // Fall through
    case SP_DockWidgetCloseButton: {

        QImage closeButton((const char **)dock_widget_close_xpm);
        QColor alphaCorner = closeButton.color(2);
        alphaCorner.setAlpha(80);
        closeButton.setColor(2, alphaCorner.rgba());
        return QPixmap::fromImage(closeButton);
    }
    break;

    case SP_DialogDiscardButton:
        return qt_gtk_get_icon(GTK_STOCK_DELETE);
    case SP_DialogOkButton:
        return qt_gtk_get_icon(GTK_STOCK_OK);
    case SP_DialogCancelButton:
        return qt_gtk_get_icon(GTK_STOCK_CANCEL);
    case SP_DialogYesButton:
        return qt_gtk_get_icon(GTK_STOCK_YES);
    case SP_DialogNoButton:
        return qt_gtk_get_icon(GTK_STOCK_NO);
    case SP_DialogOpenButton:
        return qt_gtk_get_icon(GTK_STOCK_OPEN);
    case SP_DialogCloseButton:
        return qt_gtk_get_icon(GTK_STOCK_CLOSE);
    case SP_DialogApplyButton:
        return qt_gtk_get_icon(GTK_STOCK_APPLY);
    case SP_DialogSaveButton:
        return qt_gtk_get_icon(GTK_STOCK_SAVE);
    case SP_MessageBoxWarning:
        return qt_gtk_get_icon(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
    case SP_MessageBoxQuestion:
        return qt_gtk_get_icon(GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
    case SP_MessageBoxInformation:
        return qt_gtk_get_icon(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);
    case SP_MessageBoxCritical:
        return qt_gtk_get_icon(GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);
    default:
        return QCommonStyle::standardPixmap(sp, option, widget);
    }
    return pixmap;
}

/*!
    \reimp
*/
QIcon QGtkStyle::standardIcon(StandardPixmap standardIcon,
                              const QStyleOption *option,
                              const QWidget *widget) const
{
    Q_D(const QGtkStyle);

    if (!d->isThemeAvailable())
        return QCommonStyle::standardIcon(standardIcon, option, widget);
    switch (standardIcon) {
    case SP_DialogDiscardButton:
        return qt_gtk_get_icon(GTK_STOCK_DELETE);
    case SP_DialogOkButton:
        return qt_gtk_get_icon(GTK_STOCK_OK);
    case SP_DialogCancelButton:
        return qt_gtk_get_icon(GTK_STOCK_CANCEL);
    case SP_DialogYesButton:
        return qt_gtk_get_icon(GTK_STOCK_YES);
    case SP_DialogNoButton:
        return qt_gtk_get_icon(GTK_STOCK_NO);
    case SP_DialogOpenButton:
        return qt_gtk_get_icon(GTK_STOCK_OPEN);
    case SP_DialogCloseButton:
        return qt_gtk_get_icon(GTK_STOCK_CLOSE);
    case SP_DialogApplyButton:
        return qt_gtk_get_icon(GTK_STOCK_APPLY);
    case SP_DialogSaveButton:
        return qt_gtk_get_icon(GTK_STOCK_SAVE);
    case SP_MessageBoxWarning:
        return qt_gtk_get_icon(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
    case SP_MessageBoxQuestion:
        return qt_gtk_get_icon(GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
    case SP_MessageBoxInformation:
        return qt_gtk_get_icon(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);
    case SP_MessageBoxCritical:
        return qt_gtk_get_icon(GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);
    default:
        return QCommonStyle::standardIcon(standardIcon, option, widget);
    }
}


/*! \reimp */
QRect QGtkStyle::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const
{
    Q_D(const QGtkStyle);

    QRect r = QCommonStyle::subElementRect(element, option, widget);
    if (!d->isThemeAvailable())
        return r;

    switch (element) {
    case SE_PushButtonFocusRect:
        r.adjust(0, 1, 0, -1);
        break;
    case SE_DockWidgetTitleBarText: {
        const QStyleOptionDockWidgetV2 *v2
            = qstyleoption_cast<const QStyleOptionDockWidgetV2*>(option);
        bool verticalTitleBar = v2 == 0 ? false : v2->verticalTitleBar;
        if (verticalTitleBar) {
            r.adjust(0, 0, 0, -4);
        } else {
            if (option->direction == Qt::LeftToRight)
                r.adjust(4, 0, 0, 0);
            else
                r.adjust(0, 0, -4, 0);
        }

        break;
    }
    case SE_ProgressBarLabel:
    case SE_ProgressBarContents:
    case SE_ProgressBarGroove:
        return option->rect;
    case SE_PushButtonContents:
        if (!d->gtk_check_version(2, 10, 0)) {
            GtkWidget *gtkButton = d->gtkWidget("GtkButton");
            GtkBorder *border = 0;
            d->gtk_widget_style_get(gtkButton, "inner-border", &border, NULL);
            if (border) {
                r = option->rect.adjusted(border->left, border->top, -border->right, -border->bottom);
                d->gtk_border_free(border);
            } else {
                r = option->rect.adjusted(1, 1, -1, -1);
            }
            r = visualRect(option->direction, option->rect, r);
        }
        break;
    default:
        break;
    }

    return r;
}

/*!
  \reimp
*/
QRect QGtkStyle::itemPixmapRect(const QRect &r, int flags, const QPixmap &pixmap) const
{
    return QCommonStyle::itemPixmapRect(r, flags, pixmap);
}

/*!
  \reimp
*/
void QGtkStyle::drawItemPixmap(QPainter *painter, const QRect &rect,
                            int alignment, const QPixmap &pixmap) const
{
    QCommonStyle::drawItemPixmap(painter, rect, alignment, pixmap);
}

/*!
  \reimp
*/
QStyle::SubControl QGtkStyle::hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                              const QPoint &pt, const QWidget *w) const
{
    return QCommonStyle::hitTestComplexControl(cc, opt, pt, w);
}

/*!
  \reimp
*/
QPixmap QGtkStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
                                        const QStyleOption *opt) const
{
    return QCommonStyle::generatedIconPixmap(iconMode, pixmap, opt);
}

/*!
  \reimp
*/
void QGtkStyle::drawItemText(QPainter *painter, const QRect &rect, int alignment, const QPalette &pal,
                                    bool enabled, const QString& text, QPalette::ColorRole textRole) const
{
    return QCommonStyle::drawItemText(painter, rect, alignment, pal, enabled, text, textRole);
}

QT_END_NAMESPACE

#endif //!defined(QT_NO_STYLE_QGTK)
