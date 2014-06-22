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

#include "qstyle.h"
#include "qapplication.h"
#include "qpainter.h"
#include "qwidget.h"
#include "qbitmap.h"
#include "qpixmapcache.h"
#include "qstyleoption.h"
#include "private/qstyle_p.h"
#include "private/qguiapplication_p.h"
#ifndef QT_NO_DEBUG
#include "qdebug.h"
#endif

#include <limits.h>
#include <algorithm>

QT_BEGIN_NAMESPACE

static const int MaxBits = 8 * sizeof(QSizePolicy::ControlType);

static int unpackControlTypes(QSizePolicy::ControlTypes controls, QSizePolicy::ControlType *array)
{
    if (!controls)
        return 0;

    // optimization: exactly one bit is set
    if ((controls & (controls - 1)) == 0) {
        array[0] = QSizePolicy::ControlType(uint(controls));
        return 1;
    }

    int count = 0;
    for (int i = 0; i < MaxBits; ++i) {
        if (uint bit = (controls & (0x1 << i)))
            array[count++] = QSizePolicy::ControlType(bit);
    }
    return count;
}

/*!
    \page qwidget-styling.html
    \title Styling

    Qt's built-in widgets use the QStyle class to perform nearly all
    of their drawing.  QStyle is an abstract base class that
    encapsulates the look and feel of a GUI, and can be used to make
    the widgets look exactly like the equivalent native widgets or to
    give the widgets a custom look.

    Qt provides a set of QStyle subclasses that emulate the native
    look of the different platforms supported by Qt (QWindowsStyle,
    QMacStyle, etc.). These styles are built into the
    Qt GUI module, other styles can be made available using Qt's
    plugin mechansim.

    Most functions for drawing style elements take four arguments:

    \list
    \li an enum value specifying which graphical element to draw
    \li a QStyleOption object specifying how and where to render that element
    \li a QPainter object that should be used to draw the element
    \li a QWidget object on which the drawing is performed (optional)
    \endlist

    The style gets all the information it needs to render the
    graphical element from the QStyleOption class. The widget is
    passed as the last argument in case the style needs it to perform
    special effects (such as animated default buttons on Mac OS X),
    but it isn't mandatory. In fact, QStyle can be used to draw on any
    paint device (not just widgets), in which case the widget argument
    is a zero pointer.

    \image paintsystem-stylepainter.png

    The paint system also provides the QStylePainter class inheriting
    from QPainter.  QStylePainter is a convenience class for drawing
    QStyle elements inside a widget, and extends QPainter with a set
    of high-level drawing functions implemented on top of QStyle's
    API. The advantage of using QStylePainter is that the parameter
    lists get considerably shorter.

    \table 100%
    \row
    \li \inlineimage paintsystem-icon.png
    \li \b QIcon

    The QIcon class provides scalable icons in different modes and states.

    QIcon can generate pixmaps reflecting an icon's state, mode and
    size. These pixmaps are generated from the set of pixmaps
    made available to the icon, and are used by Qt widgets to show an
    icon representing a particular action.

    The rendering of a QIcon object is handled by the QIconEngine
    class. Each icon has a corresponding icon engine that is
    responsible for drawing the icon with a requested size, mode and
    state.

    \endtable

    For more information about widget styling and appearance, see the
    \l{Styles and Style Aware Widgets}.
*/


/*!
    \class QStyle
    \brief The QStyle class is an abstract base class that encapsulates the look and feel of a GUI.

    \ingroup appearance
    \inmodule QtWidgets

    Qt contains a set of QStyle subclasses that emulate the styles of
    the different platforms supported by Qt (QWindowsStyle,
    QMacStyle etc.). By default, these styles are built
    into the Qt GUI module. Styles can also be made available as
    plugins.

    Qt's built-in widgets use QStyle to perform nearly all of their
    drawing, ensuring that they look exactly like the equivalent
    native widgets. The diagram below shows a QComboBox in eight
    different styles.

    \image qstyle-comboboxes.png Eight combo boxes

    Topics:

    \tableofcontents

    \section1 Setting a Style

    The style of the entire application can be set using the
    QApplication::setStyle() function. It can also be specified by the
    user of the application, using the \c -style command-line option:

    \snippet code/src_gui_styles_qstyle.cpp 0

    If no style is specified, Qt will choose the most appropriate
    style for the user's platform or desktop environment.

    A style can also be set on an individual widget using the
    QWidget::setStyle() function.

    \section1 Developing Style-Aware Custom Widgets

    If you are developing custom widgets and want them to look good on
    all platforms, you can use QStyle functions to perform parts of
    the widget drawing, such as drawItemText(), drawItemPixmap(),
    drawPrimitive(), drawControl(), and drawComplexControl().

    Most QStyle draw functions take four arguments:
    \list
    \li an enum value specifying which graphical element to draw
    \li a QStyleOption specifying how and where to render that element
    \li a QPainter that should be used to draw the element
    \li a QWidget on which the drawing is performed (optional)
    \endlist

    For example, if you want to draw a focus rectangle on your
    widget, you can write:

    \snippet styles/styles.cpp 1

    QStyle gets all the information it needs to render the graphical
    element from QStyleOption. The widget is passed as the last
    argument in case the style needs it to perform special effects
    (such as animated default buttons on Mac OS X), but it isn't
    mandatory. In fact, you can use QStyle to draw on any paint
    device, not just widgets, by setting the QPainter properly.

    QStyleOption has various subclasses for the various types of
    graphical elements that can be drawn. For example,
    PE_FrameFocusRect expects a QStyleOptionFocusRect argument.

    To ensure that drawing operations are as fast as possible,
    QStyleOption and its subclasses have public data members. See the
    QStyleOption class documentation for details on how to use it.

    For convenience, Qt provides the QStylePainter class, which
    combines a QStyle, a QPainter, and a QWidget. This makes it
    possible to write

    \snippet styles/styles.cpp 5
    \dots
    \snippet styles/styles.cpp 7

    instead of

    \snippet styles/styles.cpp 2
    \dots
    \snippet styles/styles.cpp 3

    \section1 Creating a Custom Style

    You can create a custom look and feel for your application by
    creating a custom style. There are two approaches to creating a
    custom style. In the static approach, you either choose an
    existing QStyle class, subclass it, and reimplement virtual
    functions to provide the custom behavior, or you create an entire
    QStyle class from scratch. In the dynamic approach, you modify the
    behavior of your system style at runtime. The static approach is
    described below. The dynamic approach is described in QProxyStyle.

    The first step in the static approach is to pick one of the styles
    provided by Qt from which you will build your custom style. Your
    choice of QStyle class will depend on which style resembles your
    desired style the most. The most general class that you can use as
    a base is QCommonStyle (not QStyle). This is because Qt requires
    its styles to be \l{QCommonStyle}s.

    Depending on which parts of the base style you want to change,
    you must reimplement the functions that are used to draw those
    parts of the interface. To illustrate this, we will modify the
    look of the spin box arrows drawn by QWindowsStyle. The arrows
    are \e{primitive elements} that are drawn by the drawPrimitive()
    function, so we need to reimplement that function. We need the
    following class declaration:

    \snippet customstyle/customstyle.h 0

    To draw its up and down arrows, QSpinBox uses the
    PE_IndicatorSpinUp and PE_IndicatorSpinDown primitive elements.
    Here's how to reimplement the drawPrimitive() function to draw
    them differently:

    \snippet customstyle/customstyle.cpp 2
    \snippet customstyle/customstyle.cpp 3
    \snippet customstyle/customstyle.cpp 4

    Notice that we don't use the \c widget argument, except to pass it
    on to the QWindowStyle::drawPrimitive() function. As mentioned
    earlier, the information about what is to be drawn and how it
    should be drawn is specified by a QStyleOption object, so there is
    no need to ask the widget.

    If you need to use the \c widget argument to obtain additional
    information, be careful to ensure that it isn't 0 and that it is
    of the correct type before using it. For example:

    \snippet customstyle/customstyle.cpp 0
    \dots
    \snippet customstyle/customstyle.cpp 1

    When implementing a custom style, you cannot assume that the
    widget is a QSpinBox just because the enum value is called
    PE_IndicatorSpinUp or PE_IndicatorSpinDown.

    The documentation for the \l{widgets/styles}{Styles} example
    covers this topic in more detail.

    \warning Qt style sheets are currently not supported for custom QStyle
    subclasses. We plan to address this in some future release.


    \section1 Using a Custom Style

    There are several ways of using a custom style in a Qt
    application. The simplest way is to pass the custom style to the
    QApplication::setStyle() static function before creating the
    QApplication object:

    \snippet customstyle/main.cpp using a custom style

    You can call QApplication::setStyle() at any time, but by calling
    it before the constructor, you ensure that the user's preference,
    set using the \c -style command-line option, is respected.

    You may want to make your custom style available for use in other
    applications, which may not be yours and hence not available for
    you to recompile. The Qt Plugin system makes it possible to create
    styles as plugins. Styles created as plugins are loaded as shared
    objects at runtime by Qt itself. Please refer to the \l{plugins-howto.html}{Qt Plugin} documentation for more
    information on how to go about creating a style plugin.

    Compile your plugin and put it into Qt's \c plugins/styles
    directory. We now have a pluggable style that Qt can load
    automatically. To use your new style with existing applications,
    simply start the application with the following argument:

    \snippet code/src_gui_styles_qstyle.cpp 1

    The application will use the look and feel from the custom style you
    implemented.

    \section1 Right-to-Left Desktops

    Languages written from right to left (such as Arabic and Hebrew)
    usually also mirror the whole layout of widgets, and require the
    light to come from the screen's top-right corner instead of
    top-left.

    If you create a custom style, you should take special care when
    drawing asymmetric elements to make sure that they also look
    correct in a mirrored layout. An easy way to test your styles is
    to run applications with the \c -reverse command-line option or
    to call QApplication::setLayoutDirection() in your \c main()
    function.

    Here are some things to keep in mind when making a style work well in a
    right-to-left environment:

    \list
    \li subControlRect() and subElementRect() return rectangles in screen coordinates
    \li QStyleOption::direction indicates in which direction the item should be drawn in
    \li If a style is not right-to-left aware it will display items as if it were left-to-right
    \li visualRect(), visualPos(), and visualAlignment() are helpful functions that will
       translate from logical to screen representations.
    \li alignedRect() will return a logical rect aligned for the current direction
    \endlist

    \section1 Styles in Item Views

    The painting of items in views is performed by a delegate. Qt's
    default delegate, QStyledItemDelegate, is also used for calculating bounding
    rectangles of items, and their sub-elements for the various kind
    of item \l{Qt::ItemDataRole}{data roles}
    QStyledItemDelegate supports. See the QStyledItemDelegate class
    description to find out which datatypes and roles are supported. You
    can read more about item data roles in \l{Model/View Programming}.

    When QStyledItemDelegate paints its items, it draws
    CE_ItemViewItem, and calculates their size with CT_ItemViewItem.
    Note also that it uses SE_ItemViewItemText to set the size of
    editors. When implementing a style to customize drawing of item
    views, you need to check the implementation of QCommonStyle (and
    any other subclasses from which your style
    inherits). This way, you find out which and how
    other style elements are painted, and you can then reimplement the
    painting of elements that should be drawn differently.

    We include a small example where we customize the drawing of item
    backgrounds.

    \snippet customviewstyle.cpp 0

    The primitive element PE_PanelItemViewItem is responsible for
    painting the background of items, and is called from
    \l{QCommonStyle}'s implementation of CE_ItemViewItem.

    To add support for drawing of new datatypes and item data roles,
    it is necessary to create a custom delegate. But if you only
    need to support the datatypes implemented by the default
    delegate, a custom style does not need an accompanying
    delegate. The QStyledItemDelegate class description gives more
    information on custom delegates.

    The drawing of item view headers is also done by the style, giving
    control over size of header items and row and column sizes.

    \sa QStyleOption, QStylePainter, {Styles Example},
        {Styles and Style Aware Widgets}, QStyledItemDelegate, {Styling}
*/

/*!
    Constructs a style object.
*/
QStyle::QStyle()
    : QObject(*new QStylePrivate)
{
    Q_D(QStyle);
    d->proxyStyle = this;
}

/*!
    \internal

    Constructs a style object.
*/
QStyle::QStyle(QStylePrivate &dd)
    : QObject(dd)
{
  Q_D(QStyle);
  d->proxyStyle = this;
}

/*!
    Destroys the style object.
*/
QStyle::~QStyle()
{
}

/*!
    Initializes the appearance of the given \a widget.

    This function is called for every widget at some point after it
    has been fully created but just \e before it is shown for the very
    first time.

    Note that the default implementation does nothing. Reasonable
    actions in this function might be to call the
    QWidget::setBackgroundMode() function for the widget. Do not use
    the function to set, for example, the geometry. Reimplementing
    this function provides a back-door through which the appearance
    of a widget can be changed, but with Qt's style engine it is
    rarely necessary to implement this function; reimplement
    drawItemPixmap(), drawItemText(), drawPrimitive(), etc. instead.

    The QWidget::inherits() function may provide enough information to
    allow class-specific customizations. But because new QStyle
    subclasses are expected to work reasonably with all current and \e
    future widgets, limited use of hard-coded customization is
    recommended.

    \sa unpolish()
*/
void QStyle::polish(QWidget * /* widget */)
{
}

/*!
    Uninitialize the given \a{widget}'s appearance.

    This function is the counterpart to polish(). It is called for
    every polished widget whenever the style is dynamically changed;
    the former style has to unpolish its settings before the new style
    can polish them again.

    Note that unpolish() will only be called if the widget is
    destroyed.  This can cause problems in some cases, e.g, if you
    remove a widget from the UI, cache it, and then reinsert it after
    the style has changed; some of Qt's classes cache their widgets.

    \sa polish()
*/
void QStyle::unpolish(QWidget * /* widget */)
{
}

/*!
    \fn void QStyle::polish(QApplication * application)
    \overload

    Late initialization of the given \a application object.
*/
void QStyle::polish(QApplication * /* app */)
{
}

/*!
    \fn void QStyle::unpolish(QApplication * application)
    \overload

    Uninitialize the given \a application.
*/
void QStyle::unpolish(QApplication * /* app */)
{
}

/*!
    \fn void QStyle::polish(QPalette & palette)
    \overload

    Changes the \a palette according to style specific requirements
    for color palettes (if any).

    \sa QPalette, QApplication::setPalette()
*/
void QStyle::polish(QPalette & /* pal */)
{
}

/*!
    \fn QRect QStyle::itemTextRect(const QFontMetrics &metrics, const QRect &rectangle, int alignment, bool enabled, const QString &text) const

    Returns the area within the given \a rectangle in which to draw
    the provided \a text according to the specified font \a metrics
    and \a alignment. The \a enabled parameter indicates whether or
    not the associated item is enabled.

    If the given \a rectangle is larger than the area needed to render
    the \a text, the rectangle that is returned will be offset within
    \a rectangle according to the specified \a alignment.  For
    example, if \a alignment is Qt::AlignCenter, the returned
    rectangle will be centered within \a rectangle. If the given \a
    rectangle is smaller than the area needed, the returned rectangle
    will be the smallest rectangle large enough to render the \a text.

    \sa Qt::Alignment
*/
QRect QStyle::itemTextRect(const QFontMetrics &metrics, const QRect &rect, int alignment, bool enabled,
                       const QString &text) const
{
    QRect result;
    int x, y, w, h;
    rect.getRect(&x, &y, &w, &h);
    if (!text.isEmpty()) {
        result = metrics.boundingRect(x, y, w, h, alignment, text);
        if (!enabled && proxy()->styleHint(SH_EtchDisabledText)) {
            result.setWidth(result.width()+1);
            result.setHeight(result.height()+1);
        }
    } else {
        result = QRect(x, y, w, h);
    }
    return result;
}

/*!
    \fn QRect QStyle::itemPixmapRect(const QRect &rectangle, int alignment, const QPixmap &pixmap) const

    Returns the area within the given \a rectangle in which to draw
    the specified \a pixmap according to the defined \a alignment.
*/
QRect QStyle::itemPixmapRect(const QRect &rect, int alignment, const QPixmap &pixmap) const
{
    QRect result;
    int x, y, w, h;
    rect.getRect(&x, &y, &w, &h);
    if ((alignment & Qt::AlignVCenter) == Qt::AlignVCenter)
        y += h/2 - pixmap.height()/2;
    else if ((alignment & Qt::AlignBottom) == Qt::AlignBottom)
        y += h - pixmap.height();
    if ((alignment & Qt::AlignRight) == Qt::AlignRight)
        x += w - pixmap.width();
    else if ((alignment & Qt::AlignHCenter) == Qt::AlignHCenter)
        x += w/2 - pixmap.width()/2;
    else if ((alignment & Qt::AlignLeft) != Qt::AlignLeft && QApplication::isRightToLeft())
        x += w - pixmap.width();
    result = QRect(x, y, pixmap.width(), pixmap.height());
    return result;
}

/*!
    \fn void QStyle::drawItemText(QPainter *painter, const QRect &rectangle, int alignment, const QPalette &palette, bool enabled, const QString& text, QPalette::ColorRole textRole) const

    Draws the given \a text in the specified \a rectangle using the
    provided \a painter and \a palette.

    The text is drawn using the painter's pen, and aligned and wrapped
    according to the specified \a alignment. If an explicit \a
    textRole is specified, the text is drawn using the \a palette's
    color for the given role. The \a enabled parameter indicates
    whether or not the item is enabled; when reimplementing this
    function, the \a enabled parameter should influence how the item is
    drawn.

    \sa Qt::Alignment, drawItemPixmap()
*/
void QStyle::drawItemText(QPainter *painter, const QRect &rect, int alignment, const QPalette &pal,
                          bool enabled, const QString& text, QPalette::ColorRole textRole) const
{
    if (text.isEmpty())
        return;
    QPen savedPen;
    if (textRole != QPalette::NoRole) {
        savedPen = painter->pen();
        painter->setPen(QPen(pal.brush(textRole), savedPen.widthF()));
    }
    if (!enabled) {
        if (proxy()->styleHint(SH_DitherDisabledText)) {
            QRect br;
            painter->drawText(rect, alignment, text, &br);
            painter->fillRect(br, QBrush(painter->background().color(), Qt::Dense5Pattern));
            return;
        } else if (proxy()->styleHint(SH_EtchDisabledText)) {
            QPen pen = painter->pen();
            painter->setPen(pal.light().color());
            painter->drawText(rect.adjusted(1, 1, 1, 1), alignment, text);
            painter->setPen(pen);
        }
    }
    painter->drawText(rect, alignment, text);
    if (textRole != QPalette::NoRole)
        painter->setPen(savedPen);
}

/*!
    \fn void QStyle::drawItemPixmap(QPainter *painter, const QRect &rectangle, int alignment,
                            const QPixmap &pixmap) const

    Draws the given \a pixmap in the specified \a rectangle, according
    to the specified \a alignment, using the provided \a painter.

    \sa drawItemText()
*/

void QStyle::drawItemPixmap(QPainter *painter, const QRect &rect, int alignment,
                            const QPixmap &pixmap) const
{
    qreal scale = pixmap.devicePixelRatio();
    QRect aligned = alignedRect(QApplication::layoutDirection(), QFlag(alignment), pixmap.size() / scale, rect);
    QRect inter = aligned.intersected(rect);

    painter->drawPixmap(inter.x(), inter.y(), pixmap, inter.x() - aligned.x(), inter.y() - aligned.y(), inter.width() * scale, inter.height() *scale);
}

/*!
    \enum QStyle::PrimitiveElement

    This enum describes the various primitive elements. A
    primitive element is a common GUI element, such as a checkbox
    indicator or button bevel.

    \omitvalue PE_IndicatorViewItemCheck
    \value PE_FrameStatusBar Frame

    \value PE_PanelButtonCommand  Button used to initiate an action, for
        example, a QPushButton.

    \value PE_FrameDefaultButton  This frame around a default button, e.g. in a dialog.
    \value PE_PanelButtonBevel  Generic panel with a button bevel.
    \value PE_PanelButtonTool  Panel for a Tool button, used with QToolButton.
    \value PE_PanelLineEdit  Panel for a QLineEdit.
    \value PE_IndicatorButtonDropDown  Indicator for a drop down button, for example, a tool
                                       button that displays a menu.

    \value PE_FrameFocusRect  Generic focus indicator.

    \value PE_IndicatorArrowUp  Generic Up arrow.
    \value PE_IndicatorArrowDown  Generic Down arrow.
    \value PE_IndicatorArrowRight  Generic Right arrow.
    \value PE_IndicatorArrowLeft  Generic Left arrow.

    \value PE_IndicatorSpinUp  Up symbol for a spin widget, for example a QSpinBox.
    \value PE_IndicatorSpinDown  Down symbol for a spin widget.
    \value PE_IndicatorSpinPlus  Increase symbol for a spin widget.
    \value PE_IndicatorSpinMinus  Decrease symbol for a spin widget.

    \value PE_IndicatorItemViewItemCheck On/off indicator for a view item.

    \value PE_IndicatorCheckBox  On/off indicator, for example, a QCheckBox.
    \value PE_IndicatorRadioButton  Exclusive on/off indicator, for example, a QRadioButton.

    \value PE_IndicatorDockWidgetResizeHandle  Resize handle for dock windows.

    \value PE_Frame  Generic frame
    \value PE_FrameMenu  Frame for popup windows/menus; see also QMenu.
    \value PE_PanelMenuBar  Panel for menu bars.
    \value PE_PanelScrollAreaCorner  Panel at the bottom-right (or
        bottom-left) corner of a scroll area.

    \value PE_FrameDockWidget  Panel frame for dock windows and toolbars.
    \value PE_FrameTabWidget  Frame for tab widgets.
    \value PE_FrameLineEdit  Panel frame for line edits.
    \value PE_FrameGroupBox  Panel frame around group boxes.
    \value PE_FrameButtonBevel  Panel frame for a button bevel.
    \value PE_FrameButtonTool  Panel frame for a tool button.

    \value PE_IndicatorHeaderArrow  Arrow used to indicate sorting on a list or table
        header.
    \value PE_FrameStatusBarItem Frame for an item of a status bar; see also QStatusBar.

    \value PE_FrameWindow  Frame around a MDI window or a docking window.

    \value PE_IndicatorMenuCheckMark  Check mark used in a menu.

    \value PE_IndicatorProgressChunk  Section of a progress bar indicator; see also QProgressBar.

    \value PE_IndicatorBranch  Lines used to represent the branch of a tree in a tree view.
    \value PE_IndicatorToolBarHandle  The handle of a toolbar.
    \value PE_IndicatorToolBarSeparator  The separator in a toolbar.
    \value PE_PanelToolBar  The panel for a toolbar.
    \value PE_PanelTipLabel The panel for a tip label.
    \value PE_FrameTabBarBase The frame that is drawn for a tab bar, ususally drawn for a tab bar that isn't part of a tab widget.
    \value PE_IndicatorTabTear An indicator that a tab is partially scrolled out of the visible tab bar when there are many tabs.
    \value PE_IndicatorColumnViewArrow An arrow in a QColumnView.

    \value PE_Widget  A plain QWidget.

    \value PE_CustomBase Base value for custom primitive elements.
    All values above this are reserved for custom use. Custom values
    must be greater than this value.

    \value PE_IndicatorItemViewItemDrop An indicator that is drawn to show where an item in an item view is about to be dropped
    during a drag-and-drop operation in an item view.
    \value PE_PanelItemViewItem The background for an item in an item view.
    \value PE_PanelItemViewRow The background of a row in an item view.

    \value PE_PanelStatusBar The panel for a status bar.

    \value PE_IndicatorTabClose The close button on a tab bar.
    \value PE_PanelMenu The panel for a menu.

    \sa drawPrimitive()
*/


/*!
    \enum QStyle::StateFlag

    This enum describes flags that are used when drawing primitive
    elements.

    Note that not all primitives use all of these flags, and that the
    flags may mean different things to different items.

    \value State_None Indicates that the widget does not have a state.
    \value State_Active Indicates that the widget is active.
    \value State_AutoRaise Used to indicate if auto-raise appearance should be usd on a tool button.
    \value State_Children Used to indicate if an item view branch has children.
    \value State_DownArrow Used to indicate if a down arrow should be visible on the widget.
    \value State_Editing Used to indicate if an editor is opened on the widget.
    \value State_Enabled Used to indicate if the widget is enabled.
    \value State_HasEditFocus Used to indicate if the widget currently has edit focus.
    \value State_HasFocus Used to indicate if the widget has focus.
    \value State_Horizontal Used to indicate if the widget is laid out horizontally, for example. a tool bar.
    \value State_KeyboardFocusChange Used to indicate if the focus was changed with the keyboard, e.g., tab, backtab or shortcut.
    \value State_MouseOver Used to indicate if the widget is under the mouse.
    \value State_NoChange Used to indicate a tri-state checkbox.
    \value State_Off Used to indicate if the widget is not checked.
    \value State_On Used to indicate if the widget is checked.
    \value State_Raised Used to indicate if a button is raised.
    \value State_ReadOnly Used to indicate if a widget is read-only.
    \value State_Selected Used to indicate if a widget is selected.
    \value State_Item Used by item views to indicate if a horizontal branch should be drawn.
    \value State_Open Used by item views to indicate if the tree branch is open.
    \value State_Sibling Used by item views to indicate if a vertical line needs to be drawn (for siblings).
    \value State_Sunken Used to indicate if the widget is sunken or pressed.
    \value State_UpArrow Used to indicate if an up arrow should be visible on the widget.
    \value State_Mini Used to indicate a mini style Mac widget or button.
    \value State_Small Used to indicate a small style Mac widget or button.
    \omitvalue State_Window
    \omitvalue State_Bottom
    \omitvalue State_FocusAtBorder
    \omitvalue State_Top

    \sa drawPrimitive()
*/

/*!
    \fn void QStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option, \
                                   QPainter *painter, const QWidget *widget) const

    Draws the given primitive \a element with the provided \a painter using the style
    options specified by \a option.

    The \a widget argument is optional and may contain a widget that may
    aid in drawing the primitive element.

    The table below is listing the primitive elements and their
    associated style option subclasses. The style options contain all
    the parameters required to draw the elements, including
    QStyleOption::state which holds the style flags that are used when
    drawing. The table also describes which flags that are set when
    casting the given option to the appropriate subclass.

    Note that if a primitive element is not listed here, it is because
    it uses a plain QStyleOption object.

    \table
    \header \li Primitive Element \li QStyleOption Subclass \li Style Flag \li Remark
    \row \li \l PE_FrameFocusRect \li \l QStyleOptionFocusRect
         \li \l State_FocusAtBorder
         \li Whether the focus is is at the border or inside the widget.
    \row \li{1,2} \l PE_IndicatorCheckBox \li{1,2} \l QStyleOptionButton
          \li \l State_NoChange \li Indicates a "tri-state" checkbox.
    \row \li \l State_On \li Indicates the indicator is checked.
    \row \li \l PE_IndicatorRadioButton \li \l QStyleOptionButton
          \li \l State_On \li Indicates that a radio button is selected.
    \row \li \l State_NoChange \li Indicates a "tri-state" controller.
    \row \li \l State_Enabled \li Indicates the controller is enabled.
    \row \li{1,4} \l PE_IndicatorBranch \li{1,4} \l QStyleOption
         \li \l State_Children \li Indicates that the control for expanding the tree to show child items, should be drawn.
    \row \li \l State_Item \li Indicates that a horizontal branch (to show a child item), should be drawn.
    \row \li \l State_Open \li Indicates that the tree branch is expanded.
    \row \li \l State_Sibling \li Indicates that a vertical line (to show a sibling item), should be drawn.
    \row \li \l PE_IndicatorHeaderArrow \li \l QStyleOptionHeader
         \li \l State_UpArrow \li Indicates that the arrow should be drawn up;
         otherwise it should be down.
    \row \li \l PE_FrameGroupBox, \l PE_Frame, \l PE_FrameLineEdit,
            \l PE_FrameMenu, \l PE_FrameDockWidget, \l PE_FrameWindow
         \li \l QStyleOptionFrame \li \l State_Sunken
         \li Indicates that the Frame should be sunken.
    \row \li \l PE_IndicatorToolBarHandle \li \l QStyleOption
         \li \l State_Horizontal \li Indicates that the window handle is horizontal
         instead of vertical.
    \row \li \l PE_IndicatorSpinPlus, \l PE_IndicatorSpinMinus, \l PE_IndicatorSpinUp,
            \l PE_IndicatorSpinDown,
         \li \l QStyleOptionSpinBox
         \li \l State_Sunken \li Indicates that the button is pressed.
    \row \li{1,5} \l PE_PanelButtonCommand
         \li{1,5} \l QStyleOptionButton
         \li \l State_Enabled \li Set if the button is enabled.
    \row \li \l State_HasFocus \li Set if the button has input focus.
    \row \li \l State_Raised \li Set if the button is not down, not on and not flat.
    \row \li \l State_On \li Set if the button is a toggle button and is toggled on.
    \row \li \l State_Sunken
         \li Set if the button is down (i.e., the mouse button or the
         space bar is pressed on the button).
    \endtable

    \sa drawComplexControl(), drawControl()
*/

/*!
    \enum QStyle::ControlElement

    This enum represents a control element. A control element is a
    part of a widget that performs some action or displays information
    to the user.

    \value CE_PushButton  A QPushButton, draws CE_PushButtonBevel, CE_PushButtonLabel and PE_FrameFocusRect.
    \value CE_PushButtonBevel  The bevel and default indicator of a QPushButton.
    \value CE_PushButtonLabel  The label (an icon with text or pixmap) of a QPushButton.

    \value CE_DockWidgetTitle  Dock window title.
    \value CE_Splitter  Splitter handle; see also QSplitter.


    \value CE_CheckBox  A QCheckBox, draws a PE_IndicatorCheckBox, a CE_CheckBoxLabel and a PE_FrameFocusRect.
    \value CE_CheckBoxLabel  The label (text or pixmap) of a QCheckBox.

    \value CE_RadioButton  A QRadioButton, draws a PE_IndicatorRadioButton, a CE_RadioButtonLabel and a PE_FrameFocusRect.
    \value CE_RadioButtonLabel  The label (text or pixmap) of a QRadioButton.

    \value CE_TabBarTab       The tab and label within a QTabBar.
    \value CE_TabBarTabShape  The tab shape within a tab bar.
    \value CE_TabBarTabLabel  The label within a tab.

    \value CE_ProgressBar  A QProgressBar, draws CE_ProgressBarGroove, CE_ProgressBarContents and CE_ProgressBarLabel.
    \value CE_ProgressBarGroove  The groove where the progress
        indicator is drawn in a QProgressBar.
    \value CE_ProgressBarContents  The progress indicator of a QProgressBar.
    \value CE_ProgressBarLabel  The text label of a QProgressBar.

    \value CE_ToolButtonLabel  A tool button's label.

    \value CE_MenuBarItem  A menu item in a QMenuBar.
    \value CE_MenuBarEmptyArea  The empty area of a QMenuBar.

    \value CE_MenuItem  A menu item in a QMenu.
    \value CE_MenuScroller  Scrolling areas in a QMenu when the
        style supports scrolling.
    \value CE_MenuTearoff  A menu item representing the tear off section of
        a QMenu.
    \value CE_MenuEmptyArea  The area in a menu without menu items.
    \value CE_MenuHMargin  The horizontal extra space on the left/right of a menu.
    \value CE_MenuVMargin  The vertical extra space on the top/bottom of a menu.

    \value CE_ToolBoxTab  The toolbox's tab and label within a QToolBox.
    \value CE_SizeGrip  Window resize handle; see also QSizeGrip.

    \value CE_Header         A header.
    \value CE_HeaderSection  A header section.
    \value CE_HeaderLabel    The header's label.

    \value CE_ScrollBarAddLine  Scroll bar line increase indicator.
                                (i.e., scroll down); see also QScrollBar.
    \value CE_ScrollBarSubLine  Scroll bar line decrease indicator (i.e., scroll up).
    \value CE_ScrollBarAddPage  Scolllbar page increase indicator (i.e., page down).
    \value CE_ScrollBarSubPage  Scroll bar page decrease indicator (i.e., page up).
    \value CE_ScrollBarSlider   Scroll bar slider.
    \value CE_ScrollBarFirst    Scroll bar first line indicator (i.e., home).
    \value CE_ScrollBarLast     Scroll bar last line indicator (i.e., end).

    \value CE_RubberBand        Rubber band used in for example an icon view.

    \value CE_FocusFrame        Focus frame that is style controlled.

    \value CE_ItemViewItem      An item inside an item view.

    \value CE_CustomBase  Base value for custom control elements;
    custom values must be greater than this value.
    \value CE_ComboBoxLabel The label of a non-editable QComboBox.
    \value CE_ToolBar A toolbar like QToolBar.
    \value CE_ToolBoxTabShape  The toolbox's tab shape.
    \value CE_ToolBoxTabLabel  The toolbox's tab label.
    \value CE_HeaderEmptyArea  The area of a header view where there are no header sections.

    \value CE_ShapedFrame The frame with the shape specified in the QStyleOptionFrameV3; see QFrame.

    \omitvalue CE_ColumnViewGrip

    \sa drawControl()
*/

/*!
    \fn void QStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const

    Draws the given \a element with the provided  \a painter with the
    style options specified by \a option.

    The \a widget argument is optional and can be used as aid in
    drawing the control. The \a option parameter is a pointer to a
    QStyleOption object that can be cast to the correct subclass
    using the qstyleoption_cast() function.

    The table below is listing the control elements and their
    associated style option subclass. The style options contain all
    the parameters required to draw the controls, including
    QStyleOption::state which holds the style flags that are used when
    drawing. The table also describes which flags that are set when
    casting the given option to the appropriate subclass.

    Note that if a control element is not listed here, it is because
    it uses a plain QStyleOption object.

    \table
    \header \li Control Element \li QStyleOption Subclass \li Style Flag \li Remark
    \row \li{1,5} \l CE_MenuItem, \l CE_MenuBarItem
         \li{1,5} \l QStyleOptionMenuItem
         \li \l State_Selected \li The menu item is currently selected item.
    \row \li \l State_Enabled \li The item is enabled.
    \row \li \l State_DownArrow \li Indicates that a scroll down arrow should be drawn.
    \row \li \l State_UpArrow \li Indicates that a scroll up arrow should be drawn
    \row \li \l State_HasFocus \li Set if the menu bar has input focus.

    \row \li{1,5} \l CE_PushButton, \l CE_PushButtonBevel, \l CE_PushButtonLabel
         \li{1,5} \l QStyleOptionButton
         \li \l State_Enabled \li Set if the button is enabled.
    \row \li \l State_HasFocus \li Set if the button has input focus.
    \row \li \l State_Raised \li Set if the button is not down, not on and not flat.
    \row \li \l State_On \li Set if the button is a toggle button and is toggled on.
    \row \li \l State_Sunken
         \li Set if the button is down (i.e., the mouse button or the
         space bar is pressed on the button).

    \row \li{1,6} \l CE_RadioButton, \l CE_RadioButtonLabel,
                 \l CE_CheckBox, \l CE_CheckBoxLabel
         \li{1,6} \l QStyleOptionButton
         \li \l State_Enabled \li Set if the button is enabled.
    \row \li \l State_HasFocus \li Set if the button has input focus.
    \row \li \l State_On \li Set if the button is checked.
    \row \li \l State_Off \li Set if the button is not checked.
    \row \li \l State_NoChange \li Set if the button is in the NoChange state.
    \row \li \l State_Sunken
         \li Set if the button is down (i.e., the mouse button or
         the space bar is pressed on the button).

   \row \li{1,2} \l CE_ProgressBarContents, \l CE_ProgressBarLabel,
                 \l CE_ProgressBarGroove
         \li{1,2} \l QStyleOptionProgressBar
         \li \l State_Enabled \li Set if the progress bar is enabled.
    \row \li \l State_HasFocus \li Set if the progress bar has input focus.

    \row \li \l CE_Header, \l CE_HeaderSection, \l CE_HeaderLabel \li \l QStyleOptionHeader \li \li

    \row \li{1,3} \l CE_TabBarTab, CE_TabBarTabShape, CE_TabBarTabLabel
        \li{1,3} \l QStyleOptionTab
        \li \l State_Enabled \li Set if the tab bar is enabled.
    \row \li \l State_Selected \li The tab bar is the currently selected tab bar.
    \row \li \l State_HasFocus \li Set if the tab bar tab has input focus.

    \row \li{1,7} \l CE_ToolButtonLabel
         \li{1,7} \l QStyleOptionToolButton
         \li \l State_Enabled \li Set if the tool button is enabled.
    \row \li \l State_HasFocus \li Set if the tool button has input focus.
    \row \li \l State_Sunken
         \li Set if the tool button is down (i.e., a mouse button or
         the space bar is pressed).
    \row \li \l State_On \li Set if the tool button is a toggle button and is toggled on.
    \row \li \l State_AutoRaise \li Set if the tool button has auto-raise enabled.
    \row \li \l State_MouseOver \li Set if the mouse pointer is over the tool button.
    \row \li \l State_Raised \li Set if the button is not down and is not on.

    \row \li \l CE_ToolBoxTab \li \l QStyleOptionToolBox
         \li \l State_Selected \li The tab is the currently selected tab.
    \row \li{1,3} \l CE_HeaderSection \li{1,3} \l QStyleOptionHeader
         \li \l State_Sunken \li Indicates that the section is pressed.
    \row \li \l State_UpArrow \li Indicates that the sort indicator should be pointing up.
    \row \li \l State_DownArrow \li Indicates that the sort indicator should be pointing down.
    \endtable

    \sa drawPrimitive(), drawComplexControl()
*/

/*!
    \enum QStyle::SubElement

    This enum represents a sub-area of a widget. Style implementations
    use these areas to draw the different parts of a widget.

    \value SE_PushButtonContents  Area containing the label (icon
        with text or pixmap).
    \value SE_PushButtonFocusRect  Area for the focus rect (usually
        larger than the contents rect).
    \value SE_PushButtonLayoutItem  Area that counts for the parent layout.

    \value SE_CheckBoxIndicator  Area for the state indicator (e.g., check mark).
    \value SE_CheckBoxContents  Area for the label (text or pixmap).
    \value SE_CheckBoxFocusRect  Area for the focus indicator.
    \value SE_CheckBoxClickRect  Clickable area, defaults to SE_CheckBoxFocusRect.
    \value SE_CheckBoxLayoutItem  Area that counts for the parent layout.

    \value SE_DateTimeEditLayoutItem  Area that counts for the parent layout.

    \value SE_RadioButtonIndicator  Area for the state indicator.
    \value SE_RadioButtonContents  Area for the label.
    \value SE_RadioButtonFocusRect  Area for the focus indicator.
    \value SE_RadioButtonClickRect  Clickable area, defaults to SE_RadioButtonFocusRect.
    \value SE_RadioButtonLayoutItem  Area that counts for the parent layout.

    \value SE_ComboBoxFocusRect  Area for the focus indicator.

    \value SE_SliderFocusRect  Area for the focus indicator.
    \value SE_SliderLayoutItem  Area that counts for the parent layout.

    \value SE_SpinBoxLayoutItem  Area that counts for the parent layout.

    \value SE_ProgressBarGroove  Area for the groove.
    \value SE_ProgressBarContents  Area for the progress indicator.
    \value SE_ProgressBarLabel  Area for the text label.
    \value SE_ProgressBarLayoutItem Area that counts for the parent layout.

    \omitvalue SE_ViewItemCheckIndicator

    \value SE_FrameContents  Area for a frame's contents.
    \value SE_ShapedFrameContents Area for a frame's contents using the shape in QStyleOptionFrameV3; see QFrame
    \value SE_FrameLayoutItem  Area that counts for the parent layout.

    \value SE_HeaderArrow Area for the sort indicator for a header.
    \value SE_HeaderLabel Area for the label in a header.

    \value SE_LabelLayoutItem  Area that counts for the parent layout.

    \value SE_LineEditContents  Area for a line edit's contents.

    \value SE_TabWidgetLeftCorner Area for the left corner widget in a tab widget.
    \value SE_TabWidgetRightCorner Area for the right corner widget in a tab widget.
    \value SE_TabWidgetTabBar Area for the tab bar widget in a tab widget.
    \value SE_TabWidgetTabContents Area for the contents of the tab widget.
    \value SE_TabWidgetTabPane Area for the pane of a tab widget.
    \value SE_TabWidgetLayoutItem  Area that counts for the parent layout.

    \value SE_ToolBoxTabContents  Area for a toolbox tab's icon and label.

    \value SE_ToolButtonLayoutItem  Area that counts for the parent layout.

    \value SE_ItemViewItemCheckIndicator Area for a view item's check mark.

    \value SE_TabBarTearIndicator Area for the tear indicator on a tab bar with scroll arrows.

    \value SE_TreeViewDisclosureItem Area for the actual disclosure item in a tree branch.

    \value SE_DialogButtonBoxLayoutItem  Area that counts for the parent layout.

    \value SE_GroupBoxLayoutItem  Area that counts for the parent layout.

    \value SE_CustomBase  Base value for custom sub-elements.
    Custom values must be greater than this value.

    \value SE_DockWidgetFloatButton The float button of a dock
                                    widget.
    \value SE_DockWidgetTitleBarText The text bounds of the dock
                                     widgets title.
    \value SE_DockWidgetCloseButton The close button of a dock
                                    widget.
    \value SE_DockWidgetIcon The icon of a dock widget.
    \value SE_ComboBoxLayoutItem Area that counts for the parent layout.


    \value SE_ItemViewItemDecoration Area for a view item's decoration (icon).
    \value SE_ItemViewItemText Area for a view item's text.
    \value SE_ItemViewItemFocusRect Area for a view item's focus rect.

    \value SE_TabBarTabLeftButton Area for a widget on the left side of a tab in a tab bar.
    \value SE_TabBarTabRightButton Area for a widget on the right side of a tab in a tab bar.
    \value SE_TabBarTabText Area for the text on a tab in a tab bar.

    \value SE_ToolBarHandle Area for the handle of a tool bar.

    \sa subElementRect()
*/

/*!
    \fn QRect QStyle::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const

    Returns the sub-area for the given \a element as described in the
    provided style \a option. The returned rectangle is defined in
    screen coordinates.

    The \a widget argument is optional and can be used to aid
    determining the area. The QStyleOption object can be cast to the
    appropriate type using the qstyleoption_cast() function. See the
    table below for the appropriate \a option casts:

    \table
    \header \li Sub Element \li QStyleOption Subclass
    \row \li \l SE_PushButtonContents   \li \l QStyleOptionButton
    \row \li \l SE_PushButtonFocusRect  \li \l QStyleOptionButton
    \row \li \l SE_CheckBoxIndicator    \li \l QStyleOptionButton
    \row \li \l SE_CheckBoxContents     \li \l QStyleOptionButton
    \row \li \l SE_CheckBoxFocusRect    \li \l QStyleOptionButton
    \row \li \l SE_RadioButtonIndicator \li \l QStyleOptionButton
    \row \li \l SE_RadioButtonContents  \li \l QStyleOptionButton
    \row \li \l SE_RadioButtonFocusRect \li \l QStyleOptionButton
    \row \li \l SE_ComboBoxFocusRect    \li \l QStyleOptionComboBox
    \row \li \l SE_ProgressBarGroove    \li \l QStyleOptionProgressBar
    \row \li \l SE_ProgressBarContents  \li \l QStyleOptionProgressBar
    \row \li \l SE_ProgressBarLabel     \li \l QStyleOptionProgressBar
    \endtable
*/

/*!
    \enum QStyle::ComplexControl

    This enum describes the available complex controls. Complex
    controls have different behavior depending upon where the user
    clicks on them or which keys are pressed.

    \value CC_SpinBox           A spinbox, like QSpinBox.
    \value CC_ComboBox          A combobox, like QComboBox.
    \value CC_ScrollBar         A scroll bar, like QScrollBar.
    \value CC_Slider            A slider, like QSlider.
    \value CC_ToolButton        A tool button, like QToolButton.
    \value CC_TitleBar          A Title bar, like those used in QMdiSubWindow.
    \value CC_GroupBox          A group box, like QGroupBox.
    \value CC_Dial              A dial, like QDial.
    \value CC_MdiControls       The minimize, close, and normal
                                button in the menu bar for a
                                maximized MDI subwindow.

    \value CC_CustomBase Base value for custom complex controls. Custom
    values must be greater than this value.

    \sa SubControl, drawComplexControl()
*/

/*!
    \enum QStyle::SubControl

    This enum describes the available sub controls. A subcontrol is a
    control element within a complex control (ComplexControl).

    \value SC_None  Special value that matches no other sub control.

    \value SC_ScrollBarAddLine  Scroll bar add line (i.e., down/right
        arrow); see also QScrollBar.
    \value SC_ScrollBarSubLine  Scroll bar sub line (i.e., up/left arrow).
    \value SC_ScrollBarAddPage  Scroll bar add page (i.e., page down).
    \value SC_ScrollBarSubPage  Scroll bar sub page (i.e., page up).
    \value SC_ScrollBarFirst  Scroll bar first line (i.e., home).
    \value SC_ScrollBarLast  Scroll bar last line (i.e., end).
    \value SC_ScrollBarSlider  Scroll bar slider handle.
    \value SC_ScrollBarGroove  Special sub-control which contains the
        area in which the slider handle may move.

    \value SC_SpinBoxUp  Spin widget up/increase; see also QSpinBox.
    \value SC_SpinBoxDown  Spin widget down/decrease.
    \value SC_SpinBoxFrame  Spin widget frame.
    \value SC_SpinBoxEditField  Spin widget edit field.

    \value SC_ComboBoxEditField  Combobox edit field; see also QComboBox.
    \value SC_ComboBoxArrow  Combobox arrow button.
    \value SC_ComboBoxFrame  Combobox frame.
    \value SC_ComboBoxListBoxPopup  The reference rectangle for the combobox popup.
        Used to calculate the position of the popup.

    \value SC_SliderGroove  Special sub-control which contains the area
        in which the slider handle may move.
    \value SC_SliderHandle  Slider handle.
    \value SC_SliderTickmarks  Slider tickmarks.

    \value SC_ToolButton  Tool button (see also QToolButton).
    \value SC_ToolButtonMenu  Sub-control for opening a popup menu in a
        tool button.

    \value SC_TitleBarSysMenu  System menu button (i.e., restore, close, etc.).
    \value SC_TitleBarMinButton  Minimize button.
    \value SC_TitleBarMaxButton  Maximize button.
    \value SC_TitleBarCloseButton  Close button.
    \value SC_TitleBarLabel  Window title label.
    \value SC_TitleBarNormalButton  Normal (restore) button.
    \value SC_TitleBarShadeButton  Shade button.
    \value SC_TitleBarUnshadeButton  Unshade button.
    \value SC_TitleBarContextHelpButton Context Help button.

    \value SC_DialHandle The handle of the dial (i.e. what you use to control the dial).
    \value SC_DialGroove The groove for the dial.
    \value SC_DialTickmarks The tickmarks for the dial.

    \value SC_GroupBoxFrame The frame of a group box.
    \value SC_GroupBoxLabel The title of a group box.
    \value SC_GroupBoxCheckBox The optional check box of a group box.
    \value SC_GroupBoxContents The group box contents.

    \value SC_MdiNormalButton The normal button for a MDI
                              subwindow in the menu bar.
    \value SC_MdiMinButton The minimize button for a MDI
                           subwindow in the menu bar.
    \value SC_MdiCloseButton The close button for a MDI subwindow
                             in the menu bar.

    \value SC_All  Special value that matches all sub-controls.
    \omitvalue SC_CustomBase

    \sa ComplexControl
*/

/*!
    \fn void QStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const

    Draws the given \a control using the provided \a painter with the
    style options specified by \a option.

    The \a widget argument is optional and can be used as aid in
    drawing the control.

    The \a option parameter is a pointer to a QStyleOptionComplex
    object that can be cast to the correct subclass using the
    qstyleoption_cast() function. Note that the \c rect member of the
    specified \a option must be in logical
    coordinates. Reimplementations of this function should use
    visualRect() to change the logical coordinates into screen
    coordinates before calling the drawPrimitive() or drawControl()
    function.

    The table below is listing the complex control elements and their
    associated style option subclass. The style options contain all
    the parameters required to draw the controls, including
    QStyleOption::state which holds the \l {QStyle::StateFlag}{style
    flags} that are used when drawing. The table also describes which
    flags that are set when casting the given \a option to the
    appropriate subclass.

    \table
    \header \li Complex Control \li QStyleOptionComplex Subclass \li Style Flag \li Remark
    \row \li{1,2} \l{CC_SpinBox} \li{1,2} \l QStyleOptionSpinBox
         \li \l State_Enabled \li Set if the spin box is enabled.
    \row \li \l State_HasFocus \li Set if the spin box has input focus.

    \row \li{1,2} \l {CC_ComboBox} \li{1,2} \l QStyleOptionComboBox
         \li \l State_Enabled \li Set if the combobox is enabled.
    \row \li \l State_HasFocus \li Set if the combobox has input focus.

    \row \li{1,2} \l {CC_ScrollBar} \li{1,2} \l QStyleOptionSlider
         \li \l State_Enabled \li Set if the scroll bar is enabled.
    \row \li \l State_HasFocus \li Set if the scroll bar has input focus.

    \row \li{1,2} \l {CC_Slider} \li{1,2} \l QStyleOptionSlider
         \li \l State_Enabled \li Set if the slider is enabled.
    \row \li \l State_HasFocus \li Set if the slider has input focus.

    \row \li{1,2} \l {CC_Dial} \li{1,2} \l QStyleOptionSlider
         \li \l State_Enabled \li Set if the dial is enabled.
    \row \li \l State_HasFocus \li Set if the dial has input focus.

    \row \li{1,6} \l {CC_ToolButton} \li{1,6} \l QStyleOptionToolButton
         \li \l State_Enabled \li Set if the tool button is enabled.
    \row \li \l State_HasFocus \li Set if the tool button has input focus.
    \row \li \l State_DownArrow \li Set if the tool button is down (i.e., a mouse
        button or the space bar is pressed).
    \row \li \l State_On \li Set if the tool button is a toggle button
        and is toggled on.
    \row \li \l State_AutoRaise \li Set if the tool button has auto-raise enabled.
    \row \li \l State_Raised \li Set if the button is not down, not on, and doesn't
        contain the mouse when auto-raise is enabled.

    \row \li \l{CC_TitleBar} \li \l QStyleOptionTitleBar
         \li \l State_Enabled \li Set if the title bar is enabled.

    \endtable

    \sa drawPrimitive(), drawControl()
*/


/*!
    \fn QRect QStyle::subControlRect(ComplexControl control,
        const QStyleOptionComplex *option, SubControl subControl,
        const QWidget *widget) const = 0

    Returns the rectangle containing the specified \a subControl of
    the given complex \a control (with the style specified by \a
    option). The rectangle is defined in screen coordinates.

    The \a option argument is a pointer to QStyleOptionComplex or
    one of its subclasses, and can be cast to the appropriate type
    using the qstyleoption_cast() function. See drawComplexControl()
    for details. The \a widget is optional and can contain additional
    information for the function.

    \sa drawComplexControl()
*/

/*!
    \fn QStyle::SubControl QStyle::hitTestComplexControl(ComplexControl control,
        const QStyleOptionComplex *option, const QPoint &position,
        const QWidget *widget) const = 0

    Returns the sub control at the given \a position in the given
    complex \a control (with the style options specified by \a
    option).

    Note that the \a position is expressed in screen coordinates.

    The \a option argument is a pointer to a QStyleOptionComplex
    object (or one of its subclasses). The object can be cast to the
    appropriate type using the qstyleoption_cast() function. See
    drawComplexControl() for details. The \a widget argument is
    optional and can contain additional information for the function.

    \sa drawComplexControl(), subControlRect()
*/

/*!
    \enum QStyle::PixelMetric

    This enum describes the various available pixel metrics. A pixel
    metric is a style dependent size represented by a single pixel
    value.

    \value PM_ButtonMargin  Amount of whitespace between push button
        labels and the frame.
    \value PM_DockWidgetTitleBarButtonMargin Amount of whitespace between dock widget's
        title bar button labels and the frame.
    \value PM_ButtonDefaultIndicator  Width of the default-button indicator frame.
    \value PM_MenuButtonIndicator  Width of the menu button indicator
        proportional to the widget height.
    \value PM_ButtonShiftHorizontal  Horizontal contents shift of a
        button when the button is down.
    \value PM_ButtonShiftVertical  Vertical contents shift of a button when the
        button is down.

    \value PM_DefaultFrameWidth  Default frame width (usually 2).
    \value PM_SpinBoxFrameWidth  Frame width of a spin box, defaults to PM_DefaultFrameWidth.
    \value PM_ComboBoxFrameWidth Frame width of a combo box, defaults to PM_DefaultFrameWidth.

    \value PM_MDIFrameWidth  Obsolete. Use PM_MdiSubWindowFrameWidth instead.
    \value PM_MdiSubWindowFrameWidth  Frame width of an MDI window.
    \value PM_MDIMinimizedWidth  Obsolete. Use PM_MdiSubWindowMinimizedWidth instead.
    \value PM_MdiSubWindowMinimizedWidth  Width of a minimized MDI window.

    \value PM_LayoutLeftMargin  Default \l{QLayout::setContentsMargins()}{left margin} for a
                                QLayout.
    \value PM_LayoutTopMargin  Default \l{QLayout::setContentsMargins()}{top margin} for a QLayout.
    \value PM_LayoutRightMargin  Default \l{QLayout::setContentsMargins()}{right margin} for a
                                 QLayout.
    \value PM_LayoutBottomMargin  Default \l{QLayout::setContentsMargins()}{bottom margin} for a
                                  QLayout.
    \value PM_LayoutHorizontalSpacing  Default \l{QLayout::spacing}{horizontal spacing} for a
                                       QLayout.
    \value PM_LayoutVerticalSpacing  Default \l{QLayout::spacing}{vertical spacing} for a QLayout.

    \value PM_MaximumDragDistance The maximum allowed distance between
    the mouse and a scrollbar when dragging. Exceeding the specified
    distance will cause the slider to jump back to the original
    position; a value of -1 disables this behavior.

    \value PM_ScrollBarExtent  Width of a vertical scroll bar and the
        height of a horizontal scroll bar.
    \value PM_ScrollBarSliderMin  The minimum height of a vertical
        scroll bar's slider and the minimum width of a horizontal
        scroll bar's slider.

    \value PM_SliderThickness  Total slider thickness.
    \value PM_SliderControlThickness  Thickness of the slider handle.
    \value PM_SliderLength  Length of the slider.
    \value PM_SliderTickmarkOffset  The offset between the tickmarks
        and the slider.
    \value PM_SliderSpaceAvailable  The available space for the slider to move.

    \value PM_DockWidgetSeparatorExtent  Width of a separator in a
        horizontal dock window and the height of a separator in a
        vertical dock window.
    \value PM_DockWidgetHandleExtent  Width of the handle in a
        horizontal dock window and the height of the handle in a
        vertical dock window.
    \value PM_DockWidgetFrameWidth  Frame width of a dock window.
    \value PM_DockWidgetTitleMargin Margin of the dock window title.

    \value PM_MenuBarPanelWidth  Frame width of a menu bar, defaults to PM_DefaultFrameWidth.
    \value PM_MenuBarItemSpacing  Spacing between menu bar items.
    \value PM_MenuBarHMargin  Spacing between menu bar items and left/right of bar.
    \value PM_MenuBarVMargin  Spacing between menu bar items and top/bottom of bar.

    \value PM_ToolBarFrameWidth  Width of the frame around toolbars.
    \value PM_ToolBarHandleExtent Width of a toolbar handle in a
        horizontal toolbar and the height of the handle in a vertical toolbar.
    \value PM_ToolBarItemMargin  Spacing between the toolbar frame and the items.
    \value PM_ToolBarItemSpacing  Spacing between toolbar items.
    \value PM_ToolBarSeparatorExtent Width of a toolbar separator in a
        horizontal toolbar and the height of a separator in a vertical toolbar.
    \value PM_ToolBarExtensionExtent Width of a toolbar extension
         button in a horizontal toolbar and the height of the button in a
         vertical toolbar.

    \value PM_TabBarTabOverlap  Number of pixels the tabs should overlap.
        (Currently only used in styles, not inside of QTabBar)
    \value PM_TabBarTabHSpace  Extra space added to the tab width.
    \value PM_TabBarTabVSpace  Extra space added to the tab height.
    \value PM_TabBarBaseHeight  Height of the area between the tab bar
        and the tab pages.
    \value PM_TabBarBaseOverlap  Number of pixels the tab bar overlaps
        the tab bar base.
    \value PM_TabBarScrollButtonWidth
    \value PM_TabBarTabShiftHorizontal  Horizontal pixel shift when a
        tab is selected.
    \value PM_TabBarTabShiftVertical  Vertical pixel shift when a
        tab is selected.

    \value PM_ProgressBarChunkWidth  Width of a chunk in a progress bar indicator.

    \value PM_SplitterWidth  Width of a splitter.

    \value PM_TitleBarHeight  Height of the title bar.

    \value PM_IndicatorWidth  Width of a check box indicator.
    \value PM_IndicatorHeight  Height of a checkbox indicator.
    \value PM_ExclusiveIndicatorWidth  Width of a radio button indicator.
    \value PM_ExclusiveIndicatorHeight  Height of a radio button indicator.

    \value PM_MenuPanelWidth  Border width (applied on all sides) for a QMenu.
    \value PM_MenuHMargin  Additional border (used on left and right) for a QMenu.
    \value PM_MenuVMargin  Additional border (used for bottom and top) for a QMenu.
    \value PM_MenuScrollerHeight  Height of the scroller area in a QMenu.
    \value PM_MenuTearoffHeight  Height of a tear off area in a QMenu.
    \value PM_MenuDesktopFrameWidth The frame width for the menu on the desktop.

    \omitvalue PM_DialogButtonsSeparator
    \omitvalue PM_DialogButtonsButtonWidth
    \omitvalue PM_DialogButtonsButtonHeight

    \value PM_HeaderMarkSize The size of the sort indicator in a header.
    \value PM_HeaderGripMargin The size of the resize grip in a header.
    \value PM_HeaderMargin The size of the margin between the sort indicator and the text.
    \value PM_SpinBoxSliderHeight The height of the optional spin box slider.

    \value PM_ToolBarIconSize Default tool bar icon size
    \value PM_SmallIconSize Default small icon size
    \value PM_LargeIconSize Default large icon size

    \value PM_FocusFrameHMargin Horizontal margin that the focus frame will outset the widget by.
    \value PM_FocusFrameVMargin Vertical margin that the focus frame will outset the widget by.
    \value PM_IconViewIconSize The default size for icons in an icon view.
    \value PM_ListViewIconSize The default size for icons in a list view.

    \value PM_ToolTipLabelFrameWidth The frame width for a tool tip label.
    \value PM_CheckBoxLabelSpacing The spacing between a check box indicator and its label.
    \value PM_RadioButtonLabelSpacing The spacing between a radio button indicator and its label.
    \value PM_TabBarIconSize The default icon size for a tab bar.
    \value PM_SizeGripSize The size of a size grip.
    \value PM_MessageBoxIconSize The size of the standard icons in a message box
    \value PM_ButtonIconSize The default size of button icons
    \value PM_TextCursorWidth The width of the cursor in a line edit or text edit
    \value PM_TabBar_ScrollButtonOverlap The distance between the left and right buttons in a tab bar.

    \value PM_TabCloseIndicatorWidth The default width of a close button on a tab in a tab bar.
    \value PM_TabCloseIndicatorHeight The default height of a close button on a tab in a tab bar.

    \value PM_CustomBase Base value for custom pixel metrics.  Custom
    values must be greater than this value.

    The following values are obsolete:

    \value PM_DefaultTopLevelMargin  Use PM_LayoutLeftMargin,
                                     PM_LayoutTopMargin,
                                     PM_LayoutRightMargin, and
                                     PM_LayoutBottomMargin instead.
    \value PM_DefaultChildMargin  Use PM_LayoutLeftMargin,
                                  PM_LayoutTopMargin,
                                  PM_LayoutRightMargin, and
                                  PM_LayoutBottomMargin instead.
    \value PM_DefaultLayoutSpacing  Use PM_LayoutHorizontalSpacing
                                    and PM_LayoutVerticalSpacing
                                    instead.

    \value PM_ScrollView_ScrollBarSpacing  Distance between frame and scrollbar
                                                with SH_ScrollView_FrameOnlyAroundContents set.
    \value PM_ScrollView_ScrollBarOverlap  Overlap between scroll bars and scroll content

    \value PM_SubMenuOverlap The horizontal overlap between a submenu and its parent.


    \sa pixelMetric()
*/

/*!
    \fn int QStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const;

    Returns the value of the given pixel \a metric.

    The specified \a option and \a widget can be used for calculating
    the metric. In general, the \a widget argument is not used. The \a
    option can be cast to the appropriate type using the
    qstyleoption_cast() function. Note that the \a option may be zero
    even for PixelMetrics that can make use of it. See the table below
    for the appropriate \a option casts:

    \table
    \header \li Pixel Metric \li QStyleOption Subclass
    \row \li \l PM_SliderControlThickness \li \l QStyleOptionSlider
    \row \li \l PM_SliderLength           \li \l QStyleOptionSlider
    \row \li \l PM_SliderTickmarkOffset   \li \l QStyleOptionSlider
    \row \li \l PM_SliderSpaceAvailable   \li \l QStyleOptionSlider
    \row \li \l PM_ScrollBarExtent        \li \l QStyleOptionSlider
    \row \li \l PM_TabBarTabOverlap       \li \l QStyleOptionTab
    \row \li \l PM_TabBarTabHSpace        \li \l QStyleOptionTab
    \row \li \l PM_TabBarTabVSpace        \li \l QStyleOptionTab
    \row \li \l PM_TabBarBaseHeight       \li \l QStyleOptionTab
    \row \li \l PM_TabBarBaseOverlap      \li \l QStyleOptionTab
    \endtable

    Some pixel metrics are called from widgets and some are only called
    internally by the style. If the metric is not called by a widget, it is the
    discretion of the style author to make use of it.  For some styles, this
    may not be appropriate.
*/

/*!
    \enum QStyle::ContentsType

    This enum describes the available contents types. These are used to
    calculate sizes for the contents of various widgets.

    \value CT_CheckBox A check box, like QCheckBox.
    \value CT_ComboBox A combo box, like QComboBox.
    \omitvalue CT_DialogButtons
    \value CT_HeaderSection A header section, like QHeader.
    \value CT_LineEdit A line edit, like QLineEdit.
    \value CT_Menu A menu, like QMenu.
    \value CT_MenuBar A menu bar, like QMenuBar.
    \value CT_MenuBarItem A menu bar item, like the buttons in a QMenuBar.
    \value CT_MenuItem A menu item, like QMenuItem.
    \value CT_ProgressBar A progress bar, like QProgressBar.
    \value CT_PushButton A push button, like QPushButton.
    \value CT_RadioButton A radio button, like QRadioButton.
    \value CT_SizeGrip A size grip, like QSizeGrip.
    \value CT_Slider A slider, like QSlider.
    \value CT_ScrollBar A scroll bar, like QScrollBar.
    \value CT_SpinBox A spin box, like QSpinBox.
    \value CT_Splitter A splitter, like QSplitter.
    \value CT_TabBarTab A tab on a tab bar, like QTabBar.
    \value CT_TabWidget A tab widget, like QTabWidget.
    \value CT_ToolButton A tool button, like QToolButton.
    \value CT_GroupBox A group box, like QGroupBox.
    \value CT_ItemViewItem An item inside an item view.

    \value CT_CustomBase  Base value for custom contents types.
    Custom values must be greater than this value.

    \value CT_MdiControls The minimize, normal, and close button
                          in the menu bar for a maximized MDI
                          subwindow.

    \sa sizeFromContents()
*/

/*!
    \fn QSize QStyle::sizeFromContents(ContentsType type, const QStyleOption *option, \
                                       const QSize &contentsSize, const QWidget *widget) const

    Returns the size of the element described by the specified
    \a option and \a type, based on the provided \a contentsSize.

    The \a option argument is a pointer to a QStyleOption or one of
    its subclasses. The \a option can be cast to the appropriate type
    using the qstyleoption_cast() function. The \a widget is an
    optional argument and can contain extra information used for
    calculating the size.

    See the table below for the appropriate \a option casts:

    \table
    \header \li Contents Type    \li QStyleOption Subclass
    \row \li \l CT_CheckBox      \li \l QStyleOptionButton
    \row \li \l CT_ComboBox      \li \l QStyleOptionComboBox
    \row \li \l CT_GroupBox      \li \l QStyleOptionGroupBox
    \row \li \l CT_HeaderSection \li \l QStyleOptionHeader
    \row \li \l CT_ItemViewItem  \li \l QStyleOptionViewItem
    \row \li \l CT_LineEdit      \li \l QStyleOptionFrame
    \row \li \l CT_MdiControls   \li \l QStyleOptionComplex
    \row \li \l CT_Menu          \li \l QStyleOption
    \row \li \l CT_MenuItem      \li \l QStyleOptionMenuItem
    \row \li \l CT_MenuBar       \li \l QStyleOptionMenuItem
    \row \li \l CT_MenuBarItem   \li \l QStyleOptionMenuItem
    \row \li \l CT_ProgressBar   \li \l QStyleOptionProgressBar
    \row \li \l CT_PushButton    \li \l QStyleOptionButton
    \row \li \l CT_RadioButton   \li \l QStyleOptionButton
    \row \li \l CT_ScrollBar     \li \l QStyleOptionSlider
    \row \li \l CT_SizeGrip      \li \l QStyleOption
    \row \li \l CT_Slider        \li \l QStyleOptionSlider
    \row \li \l CT_SpinBox       \li \l QStyleOptionSpinBox
    \row \li \l CT_Splitter      \li \l QStyleOption
    \row \li \l CT_TabBarTab     \li \l QStyleOptionTab
    \row \li \l CT_TabWidget     \li \l QStyleOptionTabWidgetFrame
    \row \li \l CT_ToolButton    \li \l QStyleOptionToolButton
    \endtable

    \sa ContentsType, QStyleOption
*/

/*!
    \enum QStyle::RequestSoftwareInputPanel

    This enum describes under what circumstances a software input panel will be
    requested by input capable widgets.

    \value RSIP_OnMouseClickAndAlreadyFocused Requests an input panel if the user
           clicks on the widget, but only if it is already focused.
    \value RSIP_OnMouseClick Requests an input panel if the user clicks on the
           widget.

    \sa QInputMethod
*/

/*!
    \enum QStyle::StyleHint

    This enum describes the available style hints. A style hint is a general look
    and/or feel hint.

    \value SH_EtchDisabledText Disabled text is "etched" as it is on Windows.

    \value SH_DitherDisabledText Disabled text is dithered as it is on Motif.

    \value SH_ScrollBar_ContextMenu Whether or not a scroll bar has a context menu.

    \value SH_ScrollBar_MiddleClickAbsolutePosition  A boolean value.
        If true, middle clicking on a scroll bar causes the slider to
        jump to that position. If false, middle clicking is
        ignored.

    \value SH_ScrollBar_LeftClickAbsolutePosition  A boolean value.
        If true, left clicking on a scroll bar causes the slider to
        jump to that position. If false, left clicking will
        behave as appropriate for each control.

    \value SH_ScrollBar_ScrollWhenPointerLeavesControl  A boolean
        value. If true, when clicking a scroll bar SubControl, holding
        the mouse button down and moving the pointer outside the
        SubControl, the scroll bar continues to scroll. If false, the
        scollbar stops scrolling when the pointer leaves the
        SubControl.

    \value SH_ScrollBar_RollBetweenButtons A boolean value.
        If true, when clicking a scroll bar button (SC_ScrollBarAddLine or
        SC_ScrollBarSubLine) and dragging over to the opposite button (rolling)
        will press the new button and release the old one. When it is false, the
        original button is released and nothing happens (like a push button).

    \value SH_TabBar_Alignment  The alignment for tabs in a
        QTabWidget. Possible values are Qt::AlignLeft,
        Qt::AlignCenter and Qt::AlignRight.

    \value SH_Header_ArrowAlignment The placement of the sorting
        indicator may appear in list or table headers. Possible values
        are Qt::Left or Qt::Right.

    \value SH_Slider_SnapToValue  Sliders snap to values while moving,
        as they do on Windows.

    \value SH_Slider_SloppyKeyEvents  Key presses handled in a sloppy
        manner, i.e., left on a vertical slider subtracts a line.

    \value SH_ProgressDialog_CenterCancelButton  Center button on
        progress dialogs, otherwise right aligned.

    \value SH_ProgressDialog_TextLabelAlignment The alignment for text
    labels in progress dialogs; Qt::AlignCenter on Windows,
    Qt::AlignVCenter otherwise.

    \value SH_PrintDialog_RightAlignButtons  Right align buttons in
        the print dialog, as done on Windows.

    \value SH_MainWindow_SpaceBelowMenuBar One or two pixel space between
        the menu bar and the dockarea, as done on Windows.

    \value SH_FontDialog_SelectAssociatedText Select the text in the
        line edit, or when selecting an item from the listbox, or when
        the line edit receives focus, as done on Windows.

    \value SH_Menu_KeyboardSearch Typing causes a menu to be search
        for relevant items, otherwise only mnemnonic is considered.

    \value SH_Menu_AllowActiveAndDisabled  Allows disabled menu
        items to be active.

    \value SH_Menu_SpaceActivatesItem  Pressing the space bar activates
        the item, as done on Motif.

    \value SH_Menu_SubMenuPopupDelay  The number of milliseconds
        to wait before opening a submenu (256 on Windows, 96 on Motif).

    \value SH_Menu_Scrollable Whether popup menus must support scrolling.

    \value SH_Menu_SloppySubMenus  Whether popup menus must support
        the user moving the mouse cursor to a submenu while crossing
        other items of the menu. This is supported on most modern
        desktop platforms.

    \value SH_ScrollView_FrameOnlyAroundContents  Whether scrollviews
        draw their frame only around contents (like Motif), or around
        contents, scroll bars and corner widgets (like Windows).

    \value SH_MenuBar_AltKeyNavigation  Menu bars items are navigable
        by pressing Alt, followed by using the arrow keys to select
        the desired item.

    \value SH_ComboBox_ListMouseTracking  Mouse tracking in combobox
        drop-down lists.

    \value SH_Menu_MouseTracking  Mouse tracking in popup menus.

    \value SH_MenuBar_MouseTracking  Mouse tracking in menu bars.

    \value SH_Menu_FillScreenWithScroll Whether scrolling popups
       should fill the screen as they are scrolled.

    \value SH_Menu_SelectionWrap Whether popups should allow the selections
        to wrap, that is when selection should the next item be the first item.

    \value SH_ItemView_ChangeHighlightOnFocus  Gray out selected items
        when losing focus.

    \value SH_Widget_ShareActivation  Turn on sharing activation with
        floating modeless dialogs.

    \value SH_TabBar_SelectMouseType  Which type of mouse event should
        cause a tab to be selected.

    \value SH_ListViewExpand_SelectMouseType  Which type of mouse event should
        cause a list view expansion to be selected.

    \value SH_TabBar_PreferNoArrows  Whether a tab bar should suggest a size
        to prevent scoll arrows.

    \value SH_ComboBox_Popup  Allows popups as a combobox drop-down
        menu.

    \omitvalue SH_ComboBox_UseNativePopup  Whether we should use a native popup.
        Only supported for non-editable combo boxes on Mac OS X so far.

    \value SH_Workspace_FillSpaceOnMaximize  The workspace should
        maximize the client area.

    \value SH_TitleBar_NoBorder  The title bar has no border.

    \value SH_ScrollBar_StopMouseOverSlider  Obsolete. Use
        SH_Slider_StopMouseOverSlider instead.

    \value SH_Slider_StopMouseOverSlider  Stops auto-repeat when
        the slider reaches the mouse position.

    \value SH_BlinkCursorWhenTextSelected  Whether cursor should blink
        when text is selected.

    \value SH_RichText_FullWidthSelection  Whether richtext selections
        should extend to the full width of the document.

    \value SH_GroupBox_TextLabelVerticalAlignment  How to vertically align a
        group box's text label.

    \value SH_GroupBox_TextLabelColor  How to paint a group box's text label.

    \value SH_DialogButtons_DefaultButton  Which button gets the
        default status in a dialog's button widget.

    \value SH_ToolBox_SelectedPageTitleBold  Boldness of the selected
    page title in a QToolBox.

    \value SH_LineEdit_PasswordCharacter  The Unicode character to be
    used for passwords.

    \value SH_Table_GridLineColor The RGB value of the grid for a table.

    \value SH_UnderlineShortcut  Whether shortcuts are underlined.

    \value SH_SpellCheckUnderlineStyle  A
        QTextCharFormat::UnderlineStyle value that specifies the way
        misspelled words should be underlined.

    \value SH_SpinBox_AnimateButton  Animate a click when up or down is
    pressed in a spin box.
    \value SH_SpinBox_KeyPressAutoRepeatRate  Auto-repeat interval for
    spinbox key presses.
    \value SH_SpinBox_ClickAutoRepeatRate  Auto-repeat interval for
    spinbox mouse clicks.
    \value SH_SpinBox_ClickAutoRepeatThreshold  Auto-repeat threshold for
    spinbox mouse clicks.
    \value SH_ToolTipLabel_Opacity  An integer indicating the opacity for
    the tip label, 0 is completely transparent, 255 is completely
    opaque.
    \value SH_DrawMenuBarSeparator  Indicates whether or not the menu bar draws separators.
    \value SH_TitleBar_ModifyNotification  Indicates if the title bar should show
    a '*' for windows that are modified.

    \value SH_Button_FocusPolicy The default focus policy for buttons.

    \value SH_CustomBase  Base value for custom style hints.
    Custom values must be greater than this value.

    \value SH_MessageBox_UseBorderForButtonSpacing A boolean indicating what the to
    use the border of the buttons (computed as half the button height) for the spacing
    of the button in a message box.

    \value SH_MessageBox_CenterButtons A boolean indicating whether the buttons in the
    message box should be centered or not (see QDialogButtonBox::setCentered()).

    \value SH_MessageBox_TextInteractionFlags A boolean indicating if
    the text in a message box should allow user interfactions (e.g.
    selection) or not.

    \value SH_TitleBar_AutoRaise A boolean indicating whether
    controls on a title bar ought to update when the mouse is over them.

    \value SH_ToolButton_PopupDelay An int indicating the popup delay in milliseconds
    for menus attached to tool buttons.

    \value SH_FocusFrame_Mask The mask of the focus frame.

    \value SH_RubberBand_Mask The mask of the rubber band.

    \value SH_WindowFrame_Mask The mask of the window frame.

    \value SH_SpinControls_DisableOnBounds Determines if the spin controls will shown
    as disabled when reaching the spin range boundary.

    \value SH_Dial_BackgroundRole Defines the style's preferred
    background role (as QPalette::ColorRole) for a dial widget.

    \value SH_ComboBox_LayoutDirection The layout direction for the
    combo box.  By default it should be the same as indicated by the
    QStyleOption::direction variable.

    \value SH_ItemView_EllipsisLocation The location where ellipses should be
    added for item text that is too long to fit in an view item.

    \value SH_ItemView_ShowDecorationSelected When an item in an item
    view is selected, also highlight the branch or other decoration.

    \value SH_ItemView_ActivateItemOnSingleClick Emit the activated signal
    when the user single clicks on an item in an item in an item view.
    Otherwise the signal is emitted when the user double clicks on an item.

    \value SH_Slider_AbsoluteSetButtons Which mouse buttons cause a slider
    to set the value to the position clicked on.

    \value SH_Slider_PageSetButtons Which mouse buttons cause a slider
    to page step the value.

    \value SH_TabBar_ElideMode The default eliding style for a tab bar.

    \value SH_DialogButtonLayout  Controls how buttons are laid out in a QDialogButtonBox, returns a QDialogButtonBox::ButtonLayout enum.

    \value SH_WizardStyle Controls the look and feel of a QWizard. Returns a QWizard::WizardStyle enum.

    \value SH_FormLayoutWrapPolicy Provides a default for how rows are wrapped in a QFormLayout. Returns a QFormLayout::RowWrapPolicy enum.
    \value SH_FormLayoutFieldGrowthPolicy Provides a default for how fields can grow in a QFormLayout. Returns a QFormLayout::FieldGrowthPolicy enum.
    \value SH_FormLayoutFormAlignment Provides a default for how a QFormLayout aligns its contents within the available space. Returns a Qt::Alignment enum.
    \value SH_FormLayoutLabelAlignment Provides a default for how a QFormLayout aligns labels within the available space. Returns a Qt::Alignment enum.

    \value SH_ItemView_ArrowKeysNavigateIntoChildren Controls whether the tree view will select the first child when it is exapanded and the right arrow key is pressed.
    \value SH_ComboBox_PopupFrameStyle  The frame style used when drawing a combobox popup menu.

    \value SH_DialogButtonBox_ButtonsHaveIcons Indicates whether or not StandardButtons in QDialogButtonBox should have icons or not.
    \value SH_ItemView_MovementWithoutUpdatingSelection The item view is able to indicate a current item without changing the selection.
    \value SH_ToolTip_Mask The mask of a tool tip.

    \value SH_FocusFrame_AboveWidget The FocusFrame is stacked above the widget that it is "focusing on".

    \value SH_TextControl_FocusIndicatorTextCharFormat Specifies the text format used to highlight focused anchors in rich text
    documents displayed for example in QTextBrowser. The format has to be a QTextCharFormat returned in the variant of the
    QStyleHintReturnVariant return value. The QTextFormat::OutlinePen property is used for the outline and QTextFormat::BackgroundBrush
    for the background of the highlighted area.

    \value SH_Menu_FlashTriggeredItem Flash triggered item.
    \value SH_Menu_FadeOutOnHide Fade out the menu instead of hiding it immediately.

    \value SH_TabWidget_DefaultTabPosition Default position of the tab bar in a tab widget.

    \value SH_ToolBar_Movable Determines if the tool bar is movable by default.

    \value SH_ItemView_PaintAlternatingRowColorsForEmptyArea Whether QTreeView paints alternating row colors for the area that does not have any items.

    \value SH_Menu_Mask The mask for a popup menu.

    \value SH_ItemView_DrawDelegateFrame Determines if there should be a frame for a delegate widget.

    \value SH_TabBar_CloseButtonPosition Determines the position of the close button on a tab in a tab bar.

    \value SH_DockWidget_ButtonsHaveFrame Determines if dockwidget buttons should have frames. Default is true.

    \value SH_ToolButtonStyle Determines the default system style for tool buttons that uses Qt::ToolButtonFollowStyle.

    \value SH_RequestSoftwareInputPanel Determines when a software input panel should
           be requested by input widgets. Returns an enum of type QStyle::RequestSoftwareInputPanel.

    \value SH_ScrollBar_Transient Determines if the style supports transient scroll bars. Transient
           scroll bars appear when the content is scrolled and disappear when they are no longer needed.

    \value SH_Menu_SupportsSections Determines if the style displays sections in menus or treat them as
           plain separators. Sections are separators with a text and icon hint.

    \value SH_ToolTip_WakeUpDelay Determines the delay before a tooltip is shown, in milliseconds.

    \value SH_ToolTip_FallAsleepDelay Determines the delay (in milliseconds) before a new wake time is needed when
           a tooltip is shown (notice: shown, not hidden). When a new wake isn't needed, a user-requested tooltip
           will be shown nearly instantly.

    \value SH_Widget_Animate Determines if the widget should show animations or not, for example
           a transition between checked and unchecked statuses in a checkbox.
           This enum value has been introduced in Qt 5.2.

    \value SH_Splitter_OpaqueResize Determines if resizing is opaque
           This enum value has been introduced in Qt 5.2

    \sa styleHint()
*/

/*!
    \fn int QStyle::styleHint(StyleHint hint, const QStyleOption *option, \
                              const QWidget *widget, QStyleHintReturn *returnData) const

    Returns an integer representing the specified style \a hint for
    the given \a widget described by the provided style \a option.

    \a returnData is used when the querying widget needs more detailed data than
    the integer that styleHint() returns. See the QStyleHintReturn class
    description for details.
*/

/*!
    \enum QStyle::StandardPixmap

    This enum describes the available standard pixmaps. A standard pixmap is a pixmap that
    can follow some existing GUI style or guideline.

    \value SP_TitleBarMinButton  Minimize button on title bars (e.g.,
        in QMdiSubWindow).
    \value SP_TitleBarMenuButton Menu button on a title bar.
    \value SP_TitleBarMaxButton  Maximize button on title bars.
    \value SP_TitleBarCloseButton  Close button on title bars.
    \value SP_TitleBarNormalButton  Normal (restore) button on title bars.
    \value SP_TitleBarShadeButton  Shade button on title bars.
    \value SP_TitleBarUnshadeButton  Unshade button on title bars.
    \value SP_TitleBarContextHelpButton The Context help button on title bars.
    \value SP_MessageBoxInformation  The "information" icon.
    \value SP_MessageBoxWarning  The "warning" icon.
    \value SP_MessageBoxCritical  The "critical" icon.
    \value SP_MessageBoxQuestion  The "question" icon.
    \value SP_DesktopIcon The "desktop" icon.
    \value SP_TrashIcon The "trash" icon.
    \value SP_ComputerIcon The "My computer" icon.
    \value SP_DriveFDIcon The floppy icon.
    \value SP_DriveHDIcon The harddrive icon.
    \value SP_DriveCDIcon The CD icon.
    \value SP_DriveDVDIcon The DVD icon.
    \value SP_DriveNetIcon The network icon.
    \value SP_DirHomeIcon The home directory icon.
    \value SP_DirOpenIcon The open directory icon.
    \value SP_DirClosedIcon The closed directory icon.
    \value SP_DirIcon The directory icon.
    \value SP_DirLinkIcon The link to directory icon.
    \value SP_DirLinkOpenIcon The link to open directory icon.
    \value SP_FileIcon The file icon.
    \value SP_FileLinkIcon The link to file icon.
    \value SP_FileDialogStart The "start" icon in a file dialog.
    \value SP_FileDialogEnd The "end" icon in a file dialog.
    \value SP_FileDialogToParent The "parent directory" icon in a file dialog.
    \value SP_FileDialogNewFolder The "create new folder" icon in a file dialog.
    \value SP_FileDialogDetailedView The detailed view icon in a file dialog.
    \value SP_FileDialogInfoView The file info icon in a file dialog.
    \value SP_FileDialogContentsView The contents view icon in a file dialog.
    \value SP_FileDialogListView The list view icon in a file dialog.
    \value SP_FileDialogBack The back arrow in a file dialog.
    \value SP_DockWidgetCloseButton  Close button on dock windows (see also QDockWidget).
    \value SP_ToolBarHorizontalExtensionButton Extension button for horizontal toolbars.
    \value SP_ToolBarVerticalExtensionButton Extension button for vertical toolbars.
    \value SP_DialogOkButton Icon for a standard OK button in a QDialogButtonBox.
    \value SP_DialogCancelButton Icon for a standard Cancel button in a QDialogButtonBox.
    \value SP_DialogHelpButton Icon for a standard Help button in a QDialogButtonBox.
    \value SP_DialogOpenButton Icon for a standard Open button in a QDialogButtonBox.
    \value SP_DialogSaveButton Icon for a standard Save button in a QDialogButtonBox.
    \value SP_DialogCloseButton Icon for a standard Close button in a QDialogButtonBox.
    \value SP_DialogApplyButton Icon for a standard Apply button in a QDialogButtonBox.
    \value SP_DialogResetButton Icon for a standard Reset button in a QDialogButtonBox.
    \value SP_DialogDiscardButton Icon for a standard Discard button in a QDialogButtonBox.
    \value SP_DialogYesButton Icon for a standard Yes button in a QDialogButtonBox.
    \value SP_DialogNoButton Icon for a standard No button in a QDialogButtonBox.
    \value SP_ArrowUp Icon arrow pointing up.
    \value SP_ArrowDown Icon arrow pointing down.
    \value SP_ArrowLeft Icon arrow pointing left.
    \value SP_ArrowRight Icon arrow pointing right.
    \value SP_ArrowBack Equivalent to SP_ArrowLeft when the current layout direction is Qt::LeftToRight, otherwise SP_ArrowRight.
    \value SP_ArrowForward Equivalent to SP_ArrowRight when the current layout direction is Qt::LeftToRight, otherwise SP_ArrowLeft.
    \value SP_CommandLink Icon used to indicate a Vista style command link glyph.
    \value SP_VistaShield Icon used to indicate UAC prompts on Windows Vista. This will return a null pixmap or icon on all other platforms.
    \value SP_BrowserReload  Icon indicating that the current page should be reloaded.
    \value SP_BrowserStop  Icon indicating that the page loading should stop.
    \value SP_MediaPlay   Icon indicating that media should begin playback.
    \value SP_MediaStop   Icon indicating that media should stop playback.
    \value SP_MediaPause  Icon indicating that media should pause playback.
    \value SP_MediaSkipForward Icon indicating that media should skip forward.
    \value SP_MediaSkipBackward Icon indicating that media should skip backward.
    \value SP_MediaSeekForward Icon indicating that media should seek forward.
    \value SP_MediaSeekBackward Icon indicating that media should seek backward.
    \value SP_MediaVolume Icon indicating a volume control.
    \value SP_MediaVolumeMuted Icon indicating a muted volume control.
    \value SP_LineEditClearButton Icon for a standard clear button in a QLineEdit. This enum value was added in Qt 5.2.
    \value SP_CustomBase  Base value for custom standard pixmaps;
    custom values must be greater than this value.

    \sa standardIcon()
*/

/*!
    \fn QPixmap QStyle::generatedIconPixmap(QIcon::Mode iconMode,
        const QPixmap &pixmap, const QStyleOption *option) const

    Returns a copy of the given \a pixmap, styled to conform to the
    specified \a iconMode and taking into account the palette
    specified by \a option.

    The \a option parameter can pass extra information, but
    it must contain a palette.

    Note that not all pixmaps will conform, in which case the returned
    pixmap is a plain copy.

    \sa QIcon
*/

/*!
    \fn QPixmap QStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption *option, \
                                       const QWidget *widget) const

    \obsolete
    Returns a pixmap for the given \a standardPixmap.

    A standard pixmap is a pixmap that can follow some existing GUI
    style or guideline. The \a option argument can be used to pass
    extra information required when defining the appropriate
    pixmap. The \a widget argument is optional and can also be used to
    aid the determination of the pixmap.

    Developers calling standardPixmap() should instead call standardIcon()
    Developers who re-implemented standardPixmap() should instead re-implement
    standardIcon().

    \sa standardIcon()
*/


/*!
    \fn QRect QStyle::visualRect(Qt::LayoutDirection direction, const QRect &boundingRectangle, const QRect &logicalRectangle)

    Returns the given \a logicalRectangle converted to screen
    coordinates based on the specified \a direction. The \a
    boundingRectangle is used when performing the translation.

    This function is provided to support right-to-left desktops, and
    is typically used in implementations of the subControlRect()
    function.

    \sa QWidget::layoutDirection
*/
QRect QStyle::visualRect(Qt::LayoutDirection direction, const QRect &boundingRect, const QRect &logicalRect)
{
    if (direction == Qt::LeftToRight)
        return logicalRect;
    QRect rect = logicalRect;
    rect.translate(2 * (boundingRect.right() - logicalRect.right()) +
                   logicalRect.width() - boundingRect.width(), 0);
    return rect;
}

/*!
    \fn QPoint QStyle::visualPos(Qt::LayoutDirection direction, const QRect &boundingRectangle, const QPoint &logicalPosition)

    Returns the given \a logicalPosition converted to screen
    coordinates based on the specified \a direction.  The \a
    boundingRectangle is used when performing the translation.

    \sa QWidget::layoutDirection
*/
QPoint QStyle::visualPos(Qt::LayoutDirection direction, const QRect &boundingRect, const QPoint &logicalPos)
{
    if (direction == Qt::LeftToRight)
        return logicalPos;
    return QPoint(boundingRect.right() - logicalPos.x(), logicalPos.y());
}

/*!
     Returns a new rectangle of the specified \a size that is aligned to the given \a
     rectangle according to the specified \a alignment and \a direction.
 */
QRect QStyle::alignedRect(Qt::LayoutDirection direction, Qt::Alignment alignment, const QSize &size, const QRect &rectangle)
{
    alignment = visualAlignment(direction, alignment);
    int x = rectangle.x();
    int y = rectangle.y();
    int w = size.width();
    int h = size.height();
    if ((alignment & Qt::AlignVCenter) == Qt::AlignVCenter)
        y += rectangle.size().height()/2 - h/2;
    else if ((alignment & Qt::AlignBottom) == Qt::AlignBottom)
        y += rectangle.size().height() - h;
    if ((alignment & Qt::AlignRight) == Qt::AlignRight)
        x += rectangle.size().width() - w;
    else if ((alignment & Qt::AlignHCenter) == Qt::AlignHCenter)
        x += rectangle.size().width()/2 - w/2;
    return QRect(x, y, w, h);
}

/*!
  Transforms an \a alignment of Qt::AlignLeft or Qt::AlignRight
  without Qt::AlignAbsolute into Qt::AlignLeft or Qt::AlignRight with
  Qt::AlignAbsolute according to the layout \a direction. The other
  alignment flags are left untouched.

  If no horizontal alignment was specified, the function returns the
  default alignment for the given layout \a direction.

  QWidget::layoutDirection
*/
Qt::Alignment QStyle::visualAlignment(Qt::LayoutDirection direction, Qt::Alignment alignment)
{
    return QGuiApplicationPrivate::visualAlignment(direction, alignment);
}

/*!
    Converts the given \a logicalValue to a pixel position. The \a min
    parameter maps to 0, \a max maps to \a span and other values are
    distributed evenly in-between.

    This function can handle the entire integer range without
    overflow, providing that \a span is less than 4096.

    By default, this function assumes that the maximum value is on the
    right for horizontal items and on the bottom for vertical items.
    Set the \a upsideDown parameter to true to reverse this behavior.

    \sa sliderValueFromPosition()
*/

int QStyle::sliderPositionFromValue(int min, int max, int logicalValue, int span, bool upsideDown)
{
    if (span <= 0 || logicalValue < min || max <= min)
        return 0;
    if (logicalValue > max)
        return upsideDown ? span : min;

    uint range = max - min;
    uint p = upsideDown ? max - logicalValue : logicalValue - min;

    if (range > (uint)INT_MAX/4096) {
        double dpos = (double(p))/(double(range)/span);
        return int(dpos);
    } else if (range > (uint)span) {
        return (2 * p * span + range) / (2*range);
    } else {
        uint div = span / range;
        uint mod = span % range;
        return p * div + (2 * p * mod + range) / (2 * range);
    }
    // equiv. to (p * span) / range + 0.5
    // no overflow because of this implicit assumption:
    // span <= 4096
}

/*!
    \fn int QStyle::sliderValueFromPosition(int min, int max, int position, int span, bool upsideDown)

    Converts the given pixel \a position to a logical value. 0 maps to
    the \a min parameter, \a span maps to \a max and other values are
    distributed evenly in-between.

    This function can handle the entire integer range without
    overflow.

    By default, this function assumes that the maximum value is on the
    right for horizontal items and on the bottom for vertical
    items. Set the \a upsideDown parameter to true to reverse this
    behavior.

    \sa sliderPositionFromValue()
*/

int QStyle::sliderValueFromPosition(int min, int max, int pos, int span, bool upsideDown)
{
    if (span <= 0 || pos <= 0)
        return upsideDown ? max : min;
    if (pos >= span)
        return upsideDown ? min : max;

    uint range = max - min;

    if ((uint)span > range) {
        int tmp = (2 * pos * range + span) / (2 * span);
        return upsideDown ? max - tmp : tmp + min;
    } else {
        uint div = range / span;
        uint mod = range % span;
        int tmp = pos * div + (2 * pos * mod + span) / (2 * span);
        return upsideDown ? max - tmp : tmp + min;
    }
    // equiv. to min + (pos*range)/span + 0.5
    // no overflow because of this implicit assumption:
    // pos <= span < sqrt(INT_MAX+0.0625)+0.25 ~ sqrt(INT_MAX)
}

/*!
     Returns the style's standard palette.

    Note that on systems that support system colors, the style's
    standard palette is not used. In particular, the Windows XP,
    Vista, and Mac styles do not use the standard palette, but make
    use of native theme engines. With these styles, you should not set
    the palette with QApplication::setStandardPalette().

 */
QPalette QStyle::standardPalette() const
{
    QColor background = QColor(0xd4, 0xd0, 0xc8); // win 2000 grey

    QColor light(background.lighter());
    QColor dark(background.darker());
    QColor mid(Qt::gray);
    QPalette palette(Qt::black, background, light, dark, mid, Qt::black, Qt::white);
    palette.setBrush(QPalette::Disabled, QPalette::WindowText, dark);
    palette.setBrush(QPalette::Disabled, QPalette::Text, dark);
    palette.setBrush(QPalette::Disabled, QPalette::ButtonText, dark);
    palette.setBrush(QPalette::Disabled, QPalette::Base, background);
    return palette;
}

/*!
    \since 4.1

    \fn QIcon QStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption *option = 0,
                                   const QWidget *widget = 0) const = 0;

    Returns an icon for the given \a standardIcon.

    The \a standardIcon is a standard pixmap which can follow some
    existing GUI style or guideline. The \a option argument can be
    used to pass extra information required when defining the
    appropriate icon. The \a widget argument is optional and can also
    be used to aid the determination of the icon.
*/

/*!
    \since 4.3

    \fn int QStyle::layoutSpacing(QSizePolicy::ControlType control1,
                                  QSizePolicy::ControlType control2, Qt::Orientation orientation,
                                  const QStyleOption *option = 0, const QWidget *widget = 0) const

    Returns the spacing that should be used between \a control1 and
    \a control2 in a layout. \a orientation specifies whether the
    controls are laid out side by side or stacked vertically. The \a
    option parameter can be used to pass extra information about the
    parent widget. The \a widget parameter is optional and can also
    be used if \a option is 0.

    This function is called by the layout system. It is used only if
    PM_LayoutHorizontalSpacing or PM_LayoutVerticalSpacing returns a
    negative value.

    \sa combinedLayoutSpacing()
*/

/*!
    \since 4.3

    Returns the spacing that should be used between \a controls1 and
    \a controls2 in a layout. \a orientation specifies whether the
    controls are laid out side by side or stacked vertically. The \a
    option parameter can be used to pass extra information about the
    parent widget. The \a widget parameter is optional and can also
    be used if \a option is 0.

    \a controls1 and \a controls2 are OR-combination of zero or more
    \l{QSizePolicy::ControlTypes}{control types}.

    This function is called by the layout system. It is used only if
    PM_LayoutHorizontalSpacing or PM_LayoutVerticalSpacing returns a
    negative value.

    \sa layoutSpacing()
*/
int QStyle::combinedLayoutSpacing(QSizePolicy::ControlTypes controls1,
                                  QSizePolicy::ControlTypes controls2, Qt::Orientation orientation,
                                  QStyleOption *option, QWidget *widget) const
{
    QSizePolicy::ControlType array1[MaxBits];
    QSizePolicy::ControlType array2[MaxBits];
    int count1 = unpackControlTypes(controls1, array1);
    int count2 = unpackControlTypes(controls2, array2);
    int result = -1;

    for (int i = 0; i < count1; ++i) {
        for (int j = 0; j < count2; ++j) {
            int spacing = layoutSpacing(array1[i], array2[j], orientation, option, widget);
            result = qMax(spacing, result);
        }
    }
    return result;
}

QT_BEGIN_INCLUDE_NAMESPACE
#include <QDebug>
QT_END_INCLUDE_NAMESPACE

#if !defined(QT_NO_DEBUG_STREAM)
QDebug operator<<(QDebug debug, QStyle::State state)
{
#if !defined(QT_NO_DEBUG)
    debug << "QStyle::State(";

    QStringList states;
    if (state & QStyle::State_Active) states << QLatin1String("Active");
    if (state & QStyle::State_AutoRaise) states << QLatin1String("AutoRaise");
    if (state & QStyle::State_Bottom) states << QLatin1String("Bottom");
    if (state & QStyle::State_Children) states << QLatin1String("Children");
    if (state & QStyle::State_DownArrow) states << QLatin1String("DownArrow");
    if (state & QStyle::State_Editing) states << QLatin1String("Editing");
    if (state & QStyle::State_Enabled) states << QLatin1String("Enabled");
    if (state & QStyle::State_FocusAtBorder) states << QLatin1String("FocusAtBorder");
    if (state & QStyle::State_HasFocus) states << QLatin1String("HasFocus");
    if (state & QStyle::State_Horizontal) states << QLatin1String("Horizontal");
    if (state & QStyle::State_Item) states << QLatin1String("Item");
    if (state & QStyle::State_KeyboardFocusChange) states << QLatin1String("KeyboardFocusChange");
    if (state & QStyle::State_MouseOver) states << QLatin1String("MouseOver");
    if (state & QStyle::State_NoChange) states << QLatin1String("NoChange");
    if (state & QStyle::State_Off) states << QLatin1String("Off");
    if (state & QStyle::State_On) states << QLatin1String("On");
    if (state & QStyle::State_Open) states << QLatin1String("Open");
    if (state & QStyle::State_Raised) states << QLatin1String("Raised");
    if (state & QStyle::State_ReadOnly) states << QLatin1String("ReadOnly");
    if (state & QStyle::State_Selected) states << QLatin1String("Selected");
    if (state & QStyle::State_Sibling) states << QLatin1String("Sibling");
    if (state & QStyle::State_Sunken) states << QLatin1String("Sunken");
    if (state & QStyle::State_Top) states << QLatin1String("Top");
    if (state & QStyle::State_UpArrow) states << QLatin1String("UpArrow");

    std::sort(states.begin(), states.end());
    debug << states.join(QLatin1String(" | "));
    debug << ')';
#else
    Q_UNUSED(state);
#endif
    return debug;
}
#endif

/*!
    \since 4.6

    \fn const QStyle *QStyle::proxy() const

    This function returns the current proxy for this style.
    By default most styles will return themselves. However
    when a proxy style is in use, it will allow the style to
    call back into its proxy.
*/
const QStyle * QStyle::proxy() const
{
    Q_D(const QStyle);
    return d->proxyStyle;
}

/* \internal

    This function sets the base style that style calls will be
    redirected to. Note that ownership is not transferred.
*/
void QStyle::setProxy(QStyle *style)
{
    Q_D(QStyle);
    d->proxyStyle = style;
}

QT_END_NAMESPACE
