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

#include "qstyleoption.h"
#include "qapplication.h"
#ifdef Q_OS_MAC
# include "qmacstyle_mac_p.h"
#endif
#include <qdebug.h>
#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

/*!
    \class QStyleOption
    \brief The QStyleOption class stores the parameters used by QStyle functions.

    \ingroup appearance
    \inmodule QtWidgets

    QStyleOption and its subclasses contain all the information that
    QStyle functions need to draw a graphical element.

    For performance reasons, there are few member functions and the
    access to the member variables is direct (i.e., using the \c . or
    \c -> operator). This low-level feel makes the structures
    straightforward to use and emphasizes that these are simply
    parameters used by the style functions.

    The caller of a QStyle function usually creates QStyleOption
    objects on the stack. This combined with Qt's extensive use of
    \l{implicit sharing} for types such as QString, QPalette, and
    QColor ensures that no memory allocation needlessly takes place.

    The following code snippet shows how to use a specific
    QStyleOption subclass to paint a push button:

    \snippet qstyleoption/main.cpp 0

    In our example, the control is a QStyle::CE_PushButton, and
    according to the QStyle::drawControl() documentation the
    corresponding class is QStyleOptionButton.

    When reimplementing QStyle functions that take a QStyleOption
    parameter, you often need to cast the QStyleOption to a subclass.
    For safety, you can use qstyleoption_cast() to ensure that the
    pointer type is correct. For example:

    \snippet qstyleoption/main.cpp 4

    The qstyleoption_cast() function will return 0 if the object to
    which \c option points is not of the correct type.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyle, QStylePainter
*/

/*!
    \enum QStyleOption::OptionType

    This enum is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \value SO_Button \l QStyleOptionButton
    \value SO_ComboBox \l QStyleOptionComboBox
    \value SO_Complex \l QStyleOptionComplex
    \value SO_Default QStyleOption
    \value SO_DockWidget \l QStyleOptionDockWidget
    \value SO_FocusRect \l QStyleOptionFocusRect
    \value SO_Frame \l QStyleOptionFrame
    \value SO_GraphicsItem \l QStyleOptionGraphicsItem
    \value SO_GroupBox \l QStyleOptionGroupBox
    \value SO_Header \l QStyleOptionHeader
    \value SO_MenuItem \l QStyleOptionMenuItem
    \value SO_ProgressBar \l QStyleOptionProgressBar
    \value SO_RubberBand \l QStyleOptionRubberBand
    \value SO_SizeGrip \l QStyleOptionSizeGrip
    \value SO_Slider \l QStyleOptionSlider
    \value SO_SpinBox \l QStyleOptionSpinBox
    \value SO_Tab \l QStyleOptionTab
    \value SO_TabBarBase \l QStyleOptionTabBarBase
    \value SO_TabWidgetFrame \l QStyleOptionTabWidgetFrame
    \value SO_TitleBar \l QStyleOptionTitleBar
    \value SO_ToolBar \l QStyleOptionToolBar
    \value SO_ToolBox \l QStyleOptionToolBox
    \value SO_ToolButton \l QStyleOptionToolButton
    \value SO_ViewItem \l QStyleOptionViewItem (used in Interviews)

    The following values are used for custom controls:

    \value SO_CustomBase Reserved for custom QStyleOptions;
                         all custom controls values must be above this value
    \value SO_ComplexCustomBase Reserved for custom QStyleOptions;
                         all custom complex controls values must be above this value

    \sa type
*/

/*!
    Constructs a QStyleOption with the specified \a version and \a
    type.

    The version has no special meaning for QStyleOption; it can be
    used by subclasses to distinguish between different version of
    the same option type.

    The \l state member variable is initialized to
    QStyle::State_None.

    \sa version, type
*/

QStyleOption::QStyleOption(int version, int type)
    : version(version), type(type), state(QStyle::State_None),
      direction(QApplication::layoutDirection()), fontMetrics(QFont()), styleObject(0)
{
}


/*!
    Destroys this style option object.
*/
QStyleOption::~QStyleOption()
{
}

/*!
    \fn void QStyleOption::initFrom(const QWidget *widget)
    \since 4.1

    Initializes the \l state, \l direction, \l rect, \l palette, \l fontMetrics
    and \l styleObject member variables based on the specified \a widget.

    This is a convenience function; the member variables can also be
    initialized manually.

    \sa QWidget::layoutDirection(), QWidget::rect(),
        QWidget::palette(), QWidget::fontMetrics()
*/

/*!
    \obsolete

    Use initFrom(\a widget) instead.
*/
void QStyleOption::init(const QWidget *widget)
{
    QWidget *window = widget->window();
    state = QStyle::State_None;
    if (widget->isEnabled())
        state |= QStyle::State_Enabled;
    if (widget->hasFocus())
        state |= QStyle::State_HasFocus;
    if (window->testAttribute(Qt::WA_KeyboardFocusChange))
        state |= QStyle::State_KeyboardFocusChange;
    if (widget->underMouse())
        state |= QStyle::State_MouseOver;
    if (window->isActiveWindow())
        state |= QStyle::State_Active;
    if (widget->isWindow())
        state |= QStyle::State_Window;
#ifdef Q_WS_MAC
    extern bool qt_mac_can_clickThrough(const QWidget *w); //qwidget_mac.cpp
    if (!(state & QStyle::State_Active) && !qt_mac_can_clickThrough(widget))
        state &= ~QStyle::State_Enabled;
#endif
#if defined(Q_OS_MACX)
    switch (QMacStyle::widgetSizePolicy(widget)) {
    case QMacStyle::SizeSmall:
        state |= QStyle::State_Small;
        break;
    case QMacStyle::SizeMini:
        state |= QStyle::State_Mini;
        break;
    default:
        ;
    }
#endif
#ifdef QT_KEYPAD_NAVIGATION
    if (widget->hasEditFocus())
        state |= QStyle::State_HasEditFocus;
#endif

    direction = widget->layoutDirection();
    rect = widget->rect();
    palette = widget->palette();
    fontMetrics = widget->fontMetrics();
    styleObject = const_cast<QWidget*>(widget);
}

/*!
   Constructs a copy of \a other.
*/
QStyleOption::QStyleOption(const QStyleOption &other)
    : version(Version), type(Type), state(other.state),
      direction(other.direction), rect(other.rect), fontMetrics(other.fontMetrics),
      palette(other.palette), styleObject(other.styleObject)
{
}

/*!
    Assign \a other to this QStyleOption.
*/
QStyleOption &QStyleOption::operator=(const QStyleOption &other)
{
    state = other.state;
    direction = other.direction;
    rect = other.rect;
    fontMetrics = other.fontMetrics;
    palette = other.palette;
    styleObject = other.styleObject;
    return *this;
}

/*!
    \enum QStyleOption::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_Default} for
           this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOption::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 1

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \variable QStyleOption::palette
    \brief the palette that should be used when painting the control

    By default, the application's default palette is used.

    \sa initFrom()
*/

/*!
    \variable QStyleOption::direction
    \brief the text layout direction that should be used when drawing text in the control

    By default, the layout direction is Qt::LeftToRight.

    \sa initFrom()
*/

/*!
    \variable QStyleOption::fontMetrics
    \brief the font metrics that should be used when drawing text in the control

    By default, the application's default font is used.

    \sa initFrom()
*/

/*!
    \variable QStyleOption::styleObject
    \brief the object being styled

    The built-in styles support the following types: QWidget, QGraphicsObject and QQuickItem.

    \sa initFrom()
*/

/*!
    \variable QStyleOption::rect
    \brief the area that should be used for various calculations and painting

    This can have different meanings for different types of elements.
    For example, for a \l QStyle::CE_PushButton element it would be
    the rectangle for the entire button, while for a \l
    QStyle::CE_PushButtonLabel element it would be just the area for
    the push button label.

    The default value is a null rectangle, i.e. a rectangle with both
    the width and the height set to 0.

    \sa initFrom()
*/

/*!
    \variable QStyleOption::state
    \brief the style flags that are used when drawing the control

    The default value is QStyle::State_None.

    \sa initFrom(), QStyle::drawPrimitive(), QStyle::drawControl(),
    QStyle::drawComplexControl(), QStyle::State
*/

/*!
    \variable QStyleOption::type
    \brief the option type of the style option

    The default value is SO_Default.

    \sa OptionType
*/

/*!
    \variable QStyleOption::version
    \brief the version of the style option

    This value can be used by subclasses to implement extensions
    without breaking compatibility. If you use the qstyleoption_cast()
    function, you normally do not need to check it.

    The default value is 1.
*/

/*!
    \class QStyleOptionFocusRect
    \brief The QStyleOptionFocusRect class is used to describe the
    parameters for drawing a focus rectangle with QStyle.

    \inmodule QtWidgets

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator). This low-level feel
    makes the structures straightforward to use and emphasizes that
    these are simply parameters used by the style functions.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption
*/

/*!
    Constructs a QStyleOptionFocusRect, initializing the members
    variables to their default values.
*/

QStyleOptionFocusRect::QStyleOptionFocusRect()
    : QStyleOption(Version, SO_FocusRect)
{
    state |= QStyle::State_KeyboardFocusChange; // assume we had one, will be corrected in initFrom()
}

/*!
    \internal
*/
QStyleOptionFocusRect::QStyleOptionFocusRect(int version)
    : QStyleOption(version, SO_FocusRect)
{
    state |= QStyle::State_KeyboardFocusChange;  // assume we had one, will be corrected in initFrom()
}

/*!
    \enum QStyleOptionFocusRect::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_FocusRect} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionFocusRect::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 1

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \fn QStyleOptionFocusRect::QStyleOptionFocusRect(const QStyleOptionFocusRect &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \variable QStyleOptionFocusRect::backgroundColor
    \brief the background color on which the focus rectangle is being drawn

    The default value is an invalid color with the RGB value (0, 0,
    0). An invalid color is a color that is not properly set up for
    the underlying window system.
*/

/*!
    \class QStyleOptionFrame
    \brief The QStyleOptionFrame class is used to describe the
    parameters for drawing a frame.

    \inmodule QtWidgets

    QStyleOptionFrame is used for drawing several built-in Qt widgets,
    including QFrame, QGroupBox, QLineEdit, and QMenu.

    An instance of the QStyleOptionFrame class has
    \l{QStyleOption::type} {type} SO_Frame and \l{QStyleOption::version}
    {version} 3.

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.  The
    version is used by QStyleOption subclasses to implement extensions
    without breaking compatibility. If you use qstyleoption_cast(),
    you normally do not need to check it.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption
*/

/*!
    \typedef QStyleOptionFrameV2
    \relates QStyleOptionFrame

    Synonym for QStyleOptionFrame.
*/

/*!
    \typedef QStyleOptionFrameV3
    \relates QStyleOptionFrame

    Synonym for QStyleOptionFrame.
*/

/*!
    Constructs a QStyleOptionFrame, initializing the members
    variables to their default values.
*/

QStyleOptionFrame::QStyleOptionFrame()
    : QStyleOption(Version, SO_Frame), lineWidth(0), midLineWidth(0),
      features(None), frameShape(QFrame::NoFrame)
{
}

/*!
    \internal
*/
QStyleOptionFrame::QStyleOptionFrame(int version)
    : QStyleOption(version, SO_Frame), lineWidth(0), midLineWidth(0),
      features(None), frameShape(QFrame::NoFrame)
{
}

/*!
    \fn QStyleOptionFrame::QStyleOptionFrame(const QStyleOptionFrame &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \enum QStyleOptionFrame::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_Frame} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionFrame::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 3

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \variable QStyleOptionFrame::lineWidth
    \brief the line width for drawing the frame

    The default value is 0.

    \sa QFrame::lineWidth
*/

/*!
    \variable QStyleOptionFrame::midLineWidth
    \brief the mid-line width for drawing the frame

    This is usually used in drawing sunken or raised frames.

    The default value is 0.

    \sa QFrame::midLineWidth
*/

/*!
    \enum QStyleOptionFrame::FrameFeature

    This enum describes the different types of features a frame can have.

    \value None Indicates a normal frame.
    \value Flat Indicates a flat frame.
    \value Rounded Indicates a rounded frame.
*/

/*!
    \variable QStyleOptionFrame::features
    \brief a bitwise OR of the features that describe this frame.

    \sa FrameFeature
*/

/*!
    \variable QStyleOptionFrame::frameShape
    \brief This property holds the frame shape value of the frame.

    \sa QFrame::frameShape
*/

/*!
    \class QStyleOptionGroupBox
    \brief The QStyleOptionGroupBox class describes the parameters for
    drawing a group box.

    \since 4.1
    \inmodule QtWidgets

    QStyleOptionButton contains all the information that QStyle
    functions need the various graphical elements of a group box.

    It holds the lineWidth and the midLineWidth for drawing the panel,
    the group box's \l {text}{title} and the title's \l
    {textAlignment}{alignment} and \l {textColor}{color}.

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator). This low-level feel
    makes the structures straightforward to use and emphasizes that
    these are simply parameters used by the style functions.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption, QStyleOptionComplex, QGroupBox
*/

/*!
    \enum QStyleOptionGroupBox::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_GroupBox} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionGroupBox::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 1

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \variable QStyleOptionGroupBox::lineWidth
    \brief the line width for drawing the panel

    The value of this variable is, currently, always 1.

    \sa QFrame::lineWidth
*/

/*!
    \variable QStyleOptionGroupBox::midLineWidth
    \brief the mid-line width for drawing the panel

    The mid-line width is usually used when drawing sunken or raised
    group box frames. The value of this variable is, currently, always 0.

    \sa QFrame::midLineWidth
*/

/*!
    \variable QStyleOptionGroupBox::text
    \brief the text of the group box

    The default value is an empty string.

    \sa QGroupBox::title
*/

/*!
    \variable QStyleOptionGroupBox::textAlignment
    \brief the alignment of the group box title

    The default value is Qt::AlignLeft.

    \sa QGroupBox::alignment
*/

/*!
    \variable QStyleOptionGroupBox::features
    \brief the features of the group box frame

    The frame is flat by default.

    \sa QStyleOptionFrame::FrameFeature
*/

/*!
    \variable QStyleOptionGroupBox::textColor
    \brief the color of the group box title

    The default value is an invalid color with the RGB value (0, 0,
    0). An invalid color is a color that is not properly set up for
    the underlying window system.
*/

/*!
    Constructs a QStyleOptionGroupBox, initializing the members
    variables to their default values.
*/
QStyleOptionGroupBox::QStyleOptionGroupBox()
    : QStyleOptionComplex(Version, Type), features(QStyleOptionFrame::None),
      textAlignment(Qt::AlignLeft), lineWidth(0), midLineWidth(0)
{
}

/*!
    \fn QStyleOptionGroupBox::QStyleOptionGroupBox(const QStyleOptionGroupBox &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \internal
*/
QStyleOptionGroupBox::QStyleOptionGroupBox(int version)
    : QStyleOptionComplex(version, Type), features(QStyleOptionFrame::None),
      textAlignment(Qt::AlignLeft), lineWidth(0), midLineWidth(0)
{
}

/*!
    \class QStyleOptionHeader
    \brief The QStyleOptionHeader class is used to describe the
    parameters for drawing a header.

    \inmodule QtWidgets

    QStyleOptionHeader contains all the information that QStyle
    functions need to draw the item views' header pane, header sort
    arrow, and header label.

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator). This low-level feel
    makes the structures straightforward to use and emphasizes that
    these are simply parameters used by the style functions.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption
*/

/*!
    Constructs a QStyleOptionHeader, initializing the members
    variables to their default values.
*/

QStyleOptionHeader::QStyleOptionHeader()
    : QStyleOption(QStyleOptionHeader::Version, SO_Header),
      section(0), textAlignment(Qt::AlignLeft), iconAlignment(Qt::AlignLeft),
      position(QStyleOptionHeader::Beginning),
      selectedPosition(QStyleOptionHeader::NotAdjacent), sortIndicator(None),
      orientation(Qt::Horizontal)
{
}

/*!
    \internal
*/
QStyleOptionHeader::QStyleOptionHeader(int version)
    : QStyleOption(version, SO_Header),
      section(0), textAlignment(Qt::AlignLeft), iconAlignment(Qt::AlignLeft),
      position(QStyleOptionHeader::Beginning),
      selectedPosition(QStyleOptionHeader::NotAdjacent), sortIndicator(None),
      orientation(Qt::Horizontal)
{
}

/*!
    \variable QStyleOptionHeader::orientation
    \brief the header's orientation (horizontal or vertical)

    The default orientation is Qt::Horizontal
*/

/*!
    \fn QStyleOptionHeader::QStyleOptionHeader(const QStyleOptionHeader &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \enum QStyleOptionHeader::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_Header} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionHeader::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 1

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \variable QStyleOptionHeader::section
    \brief which section of the header is being painted

    The default value is 0.
*/

/*!
    \variable QStyleOptionHeader::text
    \brief the text of the header

    The default value is an empty string.
*/

/*!
    \variable QStyleOptionHeader::textAlignment
    \brief the alignment flags for the text of the header

    The default value is Qt::AlignLeft.
*/

/*!
    \variable QStyleOptionHeader::icon
    \brief the icon of the header

    The default value is an empty icon, i.e. an icon with neither a
    pixmap nor a filename.
*/

/*!
    \variable QStyleOptionHeader::iconAlignment
    \brief the alignment flags for the icon of the header

    The default value is Qt::AlignLeft.
*/

/*!
    \variable QStyleOptionHeader::position
    \brief the section's position in relation to the other sections

    The default value is QStyleOptionHeader::Beginning.
*/

/*!
    \variable QStyleOptionHeader::selectedPosition
    \brief the section's position in relation to the selected section

    The default value is QStyleOptionHeader::NotAdjacent
*/

/*!
    \variable QStyleOptionHeader::sortIndicator
    \brief the direction the sort indicator should be drawn

    The default value is QStyleOptionHeader::None.
*/

/*!
    \enum QStyleOptionHeader::SectionPosition

    This enum lets you know where the section's position is in relation to the other sections.

    \value Beginning At the beginining of the header
    \value Middle In the middle of the header
    \value End At the end of the header
    \value OnlyOneSection Only one header section

    \sa position
*/

/*!
    \enum QStyleOptionHeader::SelectedPosition

    This enum lets you know where the section's position is in relation to the selected section.

    \value NotAdjacent Not adjacent to the selected section
    \value NextIsSelected The next section is selected
    \value PreviousIsSelected The previous section is selected
    \value NextAndPreviousAreSelected Both the next and previous section are selected

    \sa selectedPosition
*/

/*!
    \enum QStyleOptionHeader::SortIndicator

    Indicates which direction the sort indicator should be drawn

    \value None No sort indicator is needed
    \value SortUp Draw an up indicator
    \value SortDown Draw a down indicator

    \sa sortIndicator
*/

/*!
    \class QStyleOptionButton
    \brief The QStyleOptionButton class is used to describe the
    parameters for drawing buttons.

    \inmodule QtWidgets

    QStyleOptionButton contains all the information that QStyle
    functions need to draw graphical elements like QPushButton,
    QCheckBox, and QRadioButton.

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator). This low-level feel
    makes the structures straightforward to use and emphasizes that
    these are simply parameters used by the style functions.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption, QStyleOptionToolButton
*/

/*!
    \enum QStyleOptionButton::ButtonFeature

    This enum describes the different types of features a push button can have.

    \value None Indicates a normal push button.
    \value Flat Indicates a flat push button.
    \value HasMenu Indicates that the button has a drop down menu.
    \value DefaultButton Indicates that the button is a default button.
    \value AutoDefaultButton Indicates that the button is an auto default button.
    \value CommandLinkButton Indicates that the button is a Windows Vista type command link.

    \sa features
*/

/*!
    Constructs a QStyleOptionButton, initializing the members
    variables to their default values.
*/

QStyleOptionButton::QStyleOptionButton()
    : QStyleOption(QStyleOptionButton::Version, SO_Button), features(None)
{
}

/*!
    \internal
*/
QStyleOptionButton::QStyleOptionButton(int version)
    : QStyleOption(version, SO_Button), features(None)
{
}

/*!
    \fn QStyleOptionButton::QStyleOptionButton(const QStyleOptionButton &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \enum QStyleOptionButton::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_Button} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionButton::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 1

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \variable QStyleOptionButton::features
    \brief a bitwise OR of the features that describe this button

    \sa ButtonFeature
*/

/*!
    \variable QStyleOptionButton::text
    \brief the text of the button

    The default value is an empty string.
*/

/*!
    \variable QStyleOptionButton::icon
    \brief the icon of the button

    The default value is an empty icon, i.e. an icon with neither a
    pixmap nor a filename.

    \sa iconSize
*/

/*!
    \variable QStyleOptionButton::iconSize
    \brief the size of the icon for the button

    The default value is QSize(-1, -1), i.e. an invalid size.
*/


#ifndef QT_NO_TOOLBAR
/*!
    \class QStyleOptionToolBar
    \brief The QStyleOptionToolBar class is used to describe the
    parameters for drawing a toolbar.

    \since 4.1
    \inmodule QtWidgets

    QStyleOptionToolBar contains all the information that QStyle
    functions need to draw QToolBar.

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator). This low-level feel
    makes the structures straightforward to use and emphasizes that
    these are simply parameters used by the style functions.

    The QStyleOptionToolBar class holds the lineWidth and the
    midLineWidth for drawing the widget. It also stores information
    about which \l {toolBarArea}{area} the toolbar should be located
    in, whether it is movable or not, which position the toolbar line
    should have (positionOfLine), and the toolbar's position within
    the line (positionWithinLine).

    In addition, the class provides a couple of enums: The
    ToolBarFeature enum is used to describe whether a toolbar is
    movable or not, and the ToolBarPosition enum is used to describe
    the position of a toolbar line, as well as the toolbar's position
    within the line.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption
*/

/*!
    Constructs a QStyleOptionToolBar, initializing the members
    variables to their default values.
*/

QStyleOptionToolBar::QStyleOptionToolBar()
    : QStyleOption(Version, SO_ToolBar), positionOfLine(OnlyOne), positionWithinLine(OnlyOne),
      toolBarArea(Qt::TopToolBarArea), features(None), lineWidth(0), midLineWidth(0)
{
}

/*!
    \fn QStyleOptionToolBar::QStyleOptionToolBar(const QStyleOptionToolBar &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \internal
*/
QStyleOptionToolBar::QStyleOptionToolBar(int version)
: QStyleOption(version, SO_ToolBar), positionOfLine(OnlyOne), positionWithinLine(OnlyOne),
  toolBarArea(Qt::TopToolBarArea), features(None), lineWidth(0), midLineWidth(0)
{

}

/*!
    \enum QStyleOptionToolBar::ToolBarPosition

    \image qstyleoptiontoolbar-position.png

    This enum is used to describe the position of a toolbar line, as
    well as the toolbar's position within the line.

    The order of the positions within a line starts at the top of a
    vertical line, and from the left within a horizontal line. The
    order of the positions for the lines is always from the
    parent widget's boundary edges.

    \value Beginning The toolbar is located at the beginning of the line,
           or the toolbar line is the first of several lines. There can
           only be one toolbar (and only one line) with this position.
    \value Middle The toolbar is located in the middle of the line,
           or the toolbar line is in the middle of several lines. There can
           several toolbars (and lines) with this position.
    \value End The toolbar is located at the end of the line,
           or the toolbar line is the last of several lines. There can
           only be one toolbar (and only one line) with this position.
    \value OnlyOne There is only one toolbar or line. This is the default value
           of the positionOfLine and positionWithinLine variables.

    \sa positionWithinLine, positionOfLine
*/

/*!
    \enum QStyleOptionToolBar::ToolBarFeature

    This enum is used to describe whether a toolbar is movable or not.

    \value None The toolbar cannot be moved. The default value.
    \value Movable The toolbar is movable, and a handle will appear when
           holding the cursor over the toolbar's boundary.

    \sa features, QToolBar::isMovable()
*/

/*!
    \variable QStyleOptionToolBar::positionOfLine

    This variable holds the position of the toolbar line.

    The default value is QStyleOptionToolBar::OnlyOne.
*/

/*!
    \variable QStyleOptionToolBar::positionWithinLine

    This variable holds the position of the toolbar within a line.

    The default value is QStyleOptionToolBar::OnlyOne.
*/

/*!
    \variable QStyleOptionToolBar::toolBarArea

    This variable holds the location for drawing the toolbar.

    The default value is Qt::TopToolBarArea.

    \sa Qt::ToolBarArea
*/

/*!
    \variable QStyleOptionToolBar::features

    This variable holds whether the toolbar is movable or not.

    The default value is \l None.
*/

/*!
    \variable QStyleOptionToolBar::lineWidth

    This variable holds the line width for drawing the toolbar.

    The default value is 0.
*/

/*!
    \variable QStyleOptionToolBar::midLineWidth

    This variable holds the mid-line width for drawing the toolbar.

    The default value is 0.
*/

/*!
    \enum QStyleOptionToolBar::StyleOptionType

    This enum is used to hold information about the type of the style
    option, and is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_ToolBar} for
    this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionToolBar::StyleOptionVersion

    This enum is used to hold information about the version of the
    style option, and is defined for each QStyleOption subclass.

    \value Version 1

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

#endif

#ifndef QT_NO_TABBAR
/*!
    \class QStyleOptionTab
    \brief The QStyleOptionTab class is used to describe the
    parameters for drawing a tab bar.

    \inmodule QtWidgets

    The QStyleOptionTab class is used for drawing several built-in Qt
    widgets including \l QTabBar and the panel for \l QTabWidget.

    An instance of the QStyleOptionTab class has
    \l{QStyleOption::type} {type} \l SO_Tab and
    \l{QStyleOption::version} {version} 3. The type is used internally
    by QStyleOption, its subclasses, and qstyleoption_cast() to
    determine the type of style option. In general you do not need to
    worry about this unless you want to create your own QStyleOption
    subclass and your own styles. The version is used by QStyleOption
    subclasses to implement extensions without breaking
    compatibility. If you use qstyleoption_cast(), you normally do not
    need to check it.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption
*/

/*!
    \typedef QStyleOptionTabV2
    \relates QStyleOptionTab

    Synonym for QStyleOptionTab.
*/

/*!
    \typedef QStyleOptionTabV3
    \relates QStyleOptionTab

    Synonym for QStyleOptionTab.
*/

/*!
    Constructs a QStyleOptionTab object, initializing the members
    variables to their default values.
*/

QStyleOptionTab::QStyleOptionTab()
    : QStyleOption(QStyleOptionTab::Version, SO_Tab),
      shape(QTabBar::RoundedNorth),
      row(0),
      position(Beginning),
      selectedPosition(NotAdjacent), cornerWidgets(QStyleOptionTab::NoCornerWidgets),
      documentMode(false),
      features(QStyleOptionTab::None)
{
}

/*!
    \internal
*/
QStyleOptionTab::QStyleOptionTab(int version)
    : QStyleOption(version, SO_Tab),
      shape(QTabBar::RoundedNorth),
      row(0),
      position(Beginning),
      selectedPosition(NotAdjacent), cornerWidgets(QStyleOptionTab::NoCornerWidgets),
      documentMode(false),
      features(QStyleOptionTab::None)
{
}

/*!
    \fn QStyleOptionTab::QStyleOptionTab(const QStyleOptionTab &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \enum QStyleOptionTab::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_Tab} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionTab::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 3

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \enum QStyleOptionTab::TabPosition

    This enum describes the position of the tab.

    \value Beginning The tab is the first tab in the tab bar.
    \value Middle The tab is neither the first nor the last tab in the tab bar.
    \value End The tab is the last tab in the tab bar.
    \value OnlyOneTab The tab is both the first and the last tab in the tab bar.

    \sa position
*/

/*!
    \enum QStyleOptionTab::CornerWidget

    These flags indicate the corner widgets in a tab.

    \value NoCornerWidgets  There are no corner widgets
    \value LeftCornerWidget  Left corner widget
    \value RightCornerWidget Right corner widget

    \sa cornerWidgets
*/

/*! \enum QStyleOptionTab::SelectedPosition

    This enum describes the position of the selected tab. Some styles
    need to draw a tab differently depending on whether or not it is
    adjacent to the selected tab.

    \value NotAdjacent The tab is not adjacent to a selected tab (or is the selected tab).
    \value NextIsSelected The next tab (typically the tab on the right) is selected.
    \value PreviousIsSelected The previous tab (typically the tab on the left) is selected.

    \sa selectedPosition
*/

/*!
    \variable QStyleOptionTab::selectedPosition
    \brief the position of the selected tab in relation to this tab

    The default value is NotAdjacent, i.e. the tab is not adjacent to
    a selected tab nor is it the selected tab.
*/

/*!
    \variable QStyleOptionTab::cornerWidgets
    \brief an OR combination of CornerWidget values indicating the
    corner widgets of the tab bar

    The default value is NoCornerWidgets.

    \sa CornerWidget
*/


/*!
    \variable QStyleOptionTab::shape
    \brief the tab shape used to draw the tab; by default
    QTabBar::RoundedNorth

    \sa QTabBar::Shape
*/

/*!
    \variable QStyleOptionTab::text
    \brief the text of the tab

    The default value is an empty string.
*/

/*!
    \variable QStyleOptionTab::icon
    \brief the icon for the tab

    The default value is an empty icon, i.e. an icon with neither a
    pixmap nor a filename.
*/

/*!
    \variable QStyleOptionTab::row
    \brief which row the tab is currently in

    The default value is 0, indicating the front row.  Currently this
    property can only be 0.
*/

/*!
    \variable QStyleOptionTab::position
    \brief the position of the tab in the tab bar

    The default value is \l Beginning, i.e. the tab is the first tab
    in the tab bar.
*/
/*!
    \variable QStyleOptionTab::iconSize
    \brief the size for the icons

    The default value is QSize(-1, -1), i.e. an invalid size; use
    QStyle::pixelMetric() to find the default icon size for tab bars.

    \sa QTabBar::iconSize()
*/

/*!
    \variable QStyleOptionTab::documentMode
    \brief whether the tabbar is in document mode.

    The default value is false;
*/

/*!
    \enum QStyleOptionTab::TabFeature

    Describes the various features that a tab button can have.

    \value None A normal tab button.
    \value HasFrame The tab button is positioned on a tab frame

    \sa features
*/

/*!
    \variable QStyleOptionTab::leftButtonSize
    \brief the size for the left widget on the tab.

    The default value is QSize(-1, -1), i.e. an invalid size;
*/

/*!
    \variable QStyleOptionTab::rightButtonSize
    \brief the size for the right widget on the tab.

    The default value is QSize(-1, -1), i.e. an invalid size;
*/

#endif // QT_NO_TABBAR

/*!
    \class QStyleOptionProgressBar
    \brief The QStyleOptionProgressBar class is used to describe the
    parameters necessary for drawing a progress bar.

    \inmodule QtWidgets

    An instance of the QStyleOptionProgressBar class has type
    SO_ProgressBar and version 2.

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.  The
    version is used by QStyleOption subclasses to implement extensions
    without breaking compatibility. If you use qstyleoption_cast(),
    you normally do not need to check it.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption
*/

/*!
    \typedef QStyleOptionProgressBarV2
    \relates QStyleOptionProgressBar

    Synonym for QStyleOptionProgressBar.
*/

/*!
    Constructs a QStyleOptionProgressBar, initializing the members
    variables to their default values.
*/

QStyleOptionProgressBar::QStyleOptionProgressBar()
    : QStyleOption(QStyleOptionProgressBar::Version, SO_ProgressBar),
      minimum(0), maximum(0), progress(0), textAlignment(Qt::AlignLeft), textVisible(false),
      orientation(Qt::Horizontal), invertedAppearance(false), bottomToTop(false)
{
}

/*!
    \internal
*/
QStyleOptionProgressBar::QStyleOptionProgressBar(int version)
    : QStyleOption(version, SO_ProgressBar),
      minimum(0), maximum(0), progress(0), textAlignment(Qt::AlignLeft), textVisible(false),
      orientation(Qt::Horizontal), invertedAppearance(false), bottomToTop(false)
{
}

/*!
    \fn QStyleOptionProgressBar::QStyleOptionProgressBar(const QStyleOptionProgressBar &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \enum QStyleOptionProgressBar::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_ProgressBar} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionProgressBar::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 2

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \variable QStyleOptionProgressBar::minimum
    \brief the minimum value for the progress bar

    This is the minimum value in the progress bar. The default value
    is 0.

    \sa QProgressBar::minimum
*/

/*!
    \variable QStyleOptionProgressBar::maximum
    \brief the maximum value for the progress bar

    This is the maximum value in the progress bar. The default value
    is 0.

    \sa QProgressBar::maximum
*/

/*!
    \variable QStyleOptionProgressBar::text
    \brief the text for the progress bar

    The progress bar text is usually just the progress expressed as a
    string.  An empty string indicates that the progress bar has not
    started yet. The default value is an empty string.

    \sa QProgressBar::text
*/

/*!
    \variable QStyleOptionProgressBar::textVisible
    \brief a flag indicating whether or not text is visible

    If this flag is true then the text is visible. Otherwise, the text
    is not visible. The default value is false.

    \sa QProgressBar::textVisible
*/


/*!
    \variable QStyleOptionProgressBar::textAlignment
    \brief the text alignment for the text in the QProgressBar

    This can be used as a guide on where the text should be in the
    progress bar. The default value is Qt::AlignLeft.
*/

/*!
    \variable QStyleOptionProgressBar::progress
    \brief the current progress for the progress bar

    The current progress. A value of QStyleOptionProgressBar::minimum
    - 1 indicates that the progress hasn't started yet. The default
    value is 0.

    \sa QProgressBar::value
*/

/*!
    \variable QStyleOptionProgressBar::orientation
    \brief the progress bar's orientation (horizontal or vertical);
    the default orentation is Qt::Horizontal

    \sa QProgressBar::orientation
*/

/*!
    \variable QStyleOptionProgressBar::invertedAppearance
    \brief whether the progress bar's appearance is inverted

    The default value is false.

    \sa QProgressBar::invertedAppearance
*/

/*!
    \variable QStyleOptionProgressBar::bottomToTop
    \brief whether the text reads from bottom to top when the progress
    bar is vertical

    The default value is false.

    \sa QProgressBar::textDirection
*/

/*!
    \class QStyleOptionMenuItem
    \brief The QStyleOptionMenuItem class is used to describe the
    parameter necessary for drawing a menu item.

    \inmodule QtWidgets

    QStyleOptionMenuItem contains all the information that QStyle
    functions need to draw the menu items from \l QMenu. It is also
    used for drawing other menu-related widgets.

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator). This low-level feel
    makes the structures straightforward to use and emphasizes that
    these are simply parameters used by the style functions.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption
*/

/*!
    Constructs a QStyleOptionMenuItem, initializing the members
    variables to their default values.
*/

QStyleOptionMenuItem::QStyleOptionMenuItem()
    : QStyleOption(QStyleOptionMenuItem::Version, SO_MenuItem), menuItemType(Normal),
      checkType(NotCheckable), checked(false), menuHasCheckableItems(true), maxIconWidth(0), tabWidth(0)
{
}

/*!
    \internal
*/
QStyleOptionMenuItem::QStyleOptionMenuItem(int version)
    : QStyleOption(version, SO_MenuItem), menuItemType(Normal),
      checkType(NotCheckable), checked(false), menuHasCheckableItems(true), maxIconWidth(0), tabWidth(0)
{
}

/*!
    \fn QStyleOptionMenuItem::QStyleOptionMenuItem(const QStyleOptionMenuItem &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \enum QStyleOptionMenuItem::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_MenuItem} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionMenuItem::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 1

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \enum QStyleOptionMenuItem::MenuItemType

    This enum indicates the type of menu item that the structure describes.

    \value Normal A normal menu item.
    \value DefaultItem A menu item that is the default action as specified with \l QMenu::defaultAction().
    \value Separator A menu separator.
    \value SubMenu Indicates the menu item points to a sub-menu.
    \value Scroller A popup menu scroller (currently only used on Mac OS X).
    \value TearOff A tear-off handle for the menu.
    \value Margin The margin of the menu.
    \value EmptyArea The empty area of the menu.

    \sa menuItemType
*/

/*!
    \enum QStyleOptionMenuItem::CheckType

    This enum is used to indicate whether or not a check mark should be
    drawn for the item, or even if it should be drawn at all.

    \value NotCheckable The item is not checkable.
    \value Exclusive The item is an exclusive check item (like a radio button).
    \value NonExclusive The item is a non-exclusive check item (like a check box).

    \sa checkType, QAction::checkable, QAction::checked, QActionGroup::exclusive
*/

/*!
    \variable QStyleOptionMenuItem::menuItemType
    \brief the type of menu item

    The default value is \l Normal.

    \sa MenuItemType
*/

/*!
    \variable QStyleOptionMenuItem::checkType
    \brief the type of checkmark of the menu item

    The default value is \l NotCheckable.

    \sa CheckType
*/

/*!
    \variable QStyleOptionMenuItem::checked
    \brief whether the menu item is checked or not

    The default value is false.
*/

/*!
    \variable QStyleOptionMenuItem::menuHasCheckableItems
    \brief whether the menu as a whole has checkable items or not

    The default value is true.

    If this option is set to false, then the menu has no checkable
    items. This makes it possible for GUI styles to save some
    horizontal space that would normally be used for the check column.
*/

/*!
    \variable QStyleOptionMenuItem::menuRect
    \brief the rectangle for the entire menu

    The default value is a null rectangle, i.e. a rectangle with both
    the width and the height set to 0.
*/

/*!
    \variable QStyleOptionMenuItem::text
    \brief the text for the menu item

    Note that the text format is something like this "Menu
    text\b{\\t}Shortcut".

    If the menu item doesn't have a shortcut, it will just contain the
    menu item's text. The default value is an empty string.
*/

/*!
    \variable QStyleOptionMenuItem::icon
    \brief the icon for the menu item

    The default value is an empty icon, i.e. an icon with neither a
    pixmap nor a filename.
*/

/*!
    \variable QStyleOptionMenuItem::maxIconWidth
    \brief the maximum icon width for the icon in the menu item

    This can be used for drawing the icon into the correct place or
    properly aligning items. The variable must be set regardless of
    whether or not the menu item has an icon. The default value is 0.
*/

/*!
    \variable QStyleOptionMenuItem::tabWidth
    \brief the tab width for the menu item

    The tab width is the distance between the text of the menu item
    and the shortcut. The default value is 0.
*/


/*!
    \variable QStyleOptionMenuItem::font
    \brief the font used for the menu item text

    This is the font that should be used for drawing the menu text
    minus the shortcut. The shortcut is usually drawn using the
    painter's font. By default, the application's default font is
    used.
*/

/*!
    \class QStyleOptionComplex
    \brief The QStyleOptionComplex class is used to hold parameters that are
    common to all complex controls.

    \inmodule QtWidgets

    This class is not used on its own. Instead it is used to derive
    other complex control options, for example QStyleOptionSlider and
    QStyleOptionSpinBox.

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator).

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption
*/

/*!
    Constructs a QStyleOptionComplex of the specified \a type and \a
    version, initializing the member variables to their default
    values. This constructor is usually called by subclasses.
*/

QStyleOptionComplex::QStyleOptionComplex(int version, int type)
    : QStyleOption(version, type), subControls(QStyle::SC_All), activeSubControls(QStyle::SC_None)
{
}

/*!
    \fn QStyleOptionComplex::QStyleOptionComplex(const QStyleOptionComplex &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \enum QStyleOptionComplex::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_Complex} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionComplex::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 1

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \variable QStyleOptionComplex::subControls

    This variable holds a bitwise OR of the \l{QStyle::SubControl}
    {sub-controls} to be drawn for the complex control.

    The default value is QStyle::SC_All.

    \sa QStyle::SubControl
*/

/*!
    \variable QStyleOptionComplex::activeSubControls

    This variable holds a bitwise OR of the \l{QStyle::SubControl}
    {sub-controls} that are active for the complex control.

    The default value is QStyle::SC_None.

    \sa QStyle::SubControl
*/

#ifndef QT_NO_SLIDER
/*!
    \class QStyleOptionSlider
    \brief The QStyleOptionSlider class is used to describe the
    parameters needed for drawing a slider.

    \inmodule QtWidgets

    QStyleOptionSlider contains all the information that QStyle
    functions need to draw QSlider and QScrollBar.

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator). This low-level feel
    makes the structures straightforward to use and emphasizes that
    these are simply parameters used by the style functions.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOptionComplex, QSlider, QScrollBar
*/

/*!
    Constructs a QStyleOptionSlider, initializing the members
    variables to their default values.
*/

QStyleOptionSlider::QStyleOptionSlider()
    : QStyleOptionComplex(Version, SO_Slider), orientation(Qt::Horizontal), minimum(0), maximum(0),
      tickPosition(QSlider::NoTicks), tickInterval(0), upsideDown(false),
      sliderPosition(0), sliderValue(0), singleStep(0), pageStep(0), notchTarget(0.0),
      dialWrapping(false)
{
}

/*!
    \internal
*/
QStyleOptionSlider::QStyleOptionSlider(int version)
    : QStyleOptionComplex(version, SO_Slider), orientation(Qt::Horizontal), minimum(0), maximum(0),
      tickPosition(QSlider::NoTicks), tickInterval(0), upsideDown(false),
      sliderPosition(0), sliderValue(0), singleStep(0), pageStep(0), notchTarget(0.0),
      dialWrapping(false)
{
}

/*!
    \fn QStyleOptionSlider::QStyleOptionSlider(const QStyleOptionSlider &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \enum QStyleOptionSlider::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_Slider} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionSlider::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 1

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \variable QStyleOptionSlider::orientation
    \brief the slider's orientation (horizontal or vertical)

    The default orientation is Qt::Horizontal.

    \sa Qt::Orientation
*/

/*!
    \variable QStyleOptionSlider::minimum
    \brief the minimum value for the slider

    The default value is 0.
*/

/*!
    \variable QStyleOptionSlider::maximum
    \brief the maximum value for the slider

    The default value is 0.
*/

/*!
    \variable QStyleOptionSlider::tickPosition
    \brief the position of the slider's tick marks, if any

    The default value is QSlider::NoTicks.

    \sa QSlider::TickPosition
*/

/*!
    \variable QStyleOptionSlider::tickInterval
    \brief the interval that should be drawn between tick marks

    The default value is 0.
*/

/*!
    \variable QStyleOptionSlider::notchTarget
    \brief the number of pixel between notches

    The default value is 0.0.

    \sa QDial::notchTarget()
*/

/*!
    \variable QStyleOptionSlider::dialWrapping
    \brief whether the dial should wrap or not

    The default value is false, i.e. the dial is not wrapped.

    \sa QDial::wrapping()
*/

/*!
    \variable QStyleOptionSlider::upsideDown
    \brief the slider control orientation

    Normally a slider increases as it moves up or to the right;
    upsideDown indicates that it should do the opposite (increase as
    it moves down or to the left).  The default value is false,
    i.e. the slider increases as it moves up or to the right.

    \sa QStyle::sliderPositionFromValue(),
    QStyle::sliderValueFromPosition(),
    QAbstractSlider::invertedAppearance
*/

/*!
    \variable QStyleOptionSlider::sliderPosition
    \brief the position of the slider handle

    If the slider has active feedback (i.e.,
    QAbstractSlider::tracking is true), this value will be the same as
    \l sliderValue. Otherwise, it will have the current position of
    the handle. The default value is 0.

    \sa QAbstractSlider::tracking, sliderValue
*/

/*!
    \variable QStyleOptionSlider::sliderValue
    \brief the value of the slider

    If the slider has active feedback (i.e.,
    QAbstractSlider::tracking is true), this value will be the same
    as \l sliderPosition. Otherwise, it will have the value the
    slider had before the mouse was pressed.

    The default value is 0.

    \sa QAbstractSlider::tracking, sliderPosition
*/

/*!
    \variable QStyleOptionSlider::singleStep
    \brief the size of the single step of the slider

    The default value is 0.

    \sa QAbstractSlider::singleStep
*/

/*!
    \variable QStyleOptionSlider::pageStep
    \brief the size of the page step of the slider

    The default value is 0.

    \sa QAbstractSlider::pageStep
*/
#endif // QT_NO_SLIDER

#ifndef QT_NO_SPINBOX
/*!
    \class QStyleOptionSpinBox
    \brief The QStyleOptionSpinBox class is used to describe the
    parameters necessary for drawing a spin box.

    \inmodule QtWidgets

    QStyleOptionSpinBox contains all the information that QStyle
    functions need to draw QSpinBox and QDateTimeEdit.

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator). This low-level feel
    makes the structures straightforward to use and emphasizes that
    these are simply parameters used by the style functions.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption, QStyleOptionComplex
*/

/*!
    Constructs a QStyleOptionSpinBox, initializing the members
    variables to their default values.
*/

QStyleOptionSpinBox::QStyleOptionSpinBox()
    : QStyleOptionComplex(Version, SO_SpinBox), buttonSymbols(QAbstractSpinBox::UpDownArrows),
      stepEnabled(QAbstractSpinBox::StepNone), frame(false)
{
}

/*!
    \internal
*/
QStyleOptionSpinBox::QStyleOptionSpinBox(int version)
    : QStyleOptionComplex(version, SO_SpinBox), buttonSymbols(QAbstractSpinBox::UpDownArrows),
      stepEnabled(QAbstractSpinBox::StepNone), frame(false)
{
}

/*!
    \fn QStyleOptionSpinBox::QStyleOptionSpinBox(const QStyleOptionSpinBox &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \enum QStyleOptionSpinBox::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_SpinBox} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionSpinBox::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 1

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \variable QStyleOptionSpinBox::buttonSymbols
    \brief the type of button symbols to draw for the spin box

    The default value is QAbstractSpinBox::UpDownArrows specufying
    little arrows in the classic style.

    \sa QAbstractSpinBox::ButtonSymbols
*/

/*!
    \variable QStyleOptionSpinBox::stepEnabled
    \brief which buttons of the spin box that are enabled

    The default value is QAbstractSpinBox::StepNone.

    \sa QAbstractSpinBox::StepEnabled
*/

/*!
    \variable QStyleOptionSpinBox::frame
    \brief whether the spin box has a frame

    The default value is false, i.e. the spin box has no frame.
*/
#endif // QT_NO_SPINBOX

/*!
    \class QStyleOptionDockWidget
    \brief The QStyleOptionDockWidget class is used to describe the
    parameters for drawing a dock widget.

    \inmodule QtWidgets

    QStyleOptionDockWidget contains all the information that QStyle
    functions need to draw graphical elements like QDockWidget.

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator). This low-level feel
    makes the structures straightforward to use and emphasizes that
    these are simply parameters used by the style functions.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption
*/

/*!
    \typedef QStyleOptionDockWidgetV2
    \relates QStyleOptionDockWidget

    Synonym for QStyleOptionDockWidget.
*/

/*!
    Constructs a QStyleOptionDockWidget, initializing the member
    variables to their default values.
*/

QStyleOptionDockWidget::QStyleOptionDockWidget()
    : QStyleOption(Version, SO_DockWidget), closable(false),
      movable(false), floatable(false), verticalTitleBar(false)
{
}

/*!
    \internal
*/
QStyleOptionDockWidget::QStyleOptionDockWidget(int version)
    : QStyleOption(version, SO_DockWidget), closable(false),
      movable(false), floatable(false), verticalTitleBar(false)
{
}

/*!
    \fn QStyleOptionDockWidget::QStyleOptionDockWidget(const QStyleOptionDockWidget &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \enum QStyleOptionDockWidget::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_DockWidget} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionDockWidget::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 2

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \variable QStyleOptionDockWidget::title
    \brief the title of the dock window

    The default value is an empty string.
*/

/*!
    \variable QStyleOptionDockWidget::closable
    \brief whether the dock window is closable

    The default value is true.
*/

/*!
    \variable QStyleOptionDockWidget::movable
    \brief whether the dock window is movable

    The default value is false.
*/

/*!
    \variable QStyleOptionDockWidget::floatable
    \brief whether the dock window is floatable

    The default value is true.
*/

/*!
    \class QStyleOptionToolButton
    \brief The QStyleOptionToolButton class is used to describe the
    parameters for drawing a tool button.

    \inmodule QtWidgets

    QStyleOptionToolButton contains all the information that QStyle
    functions need to draw QToolButton.

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator). This low-level feel
    makes the structures straightforward to use and emphasizes that
    these are simply parameters used by the style functions.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption, QStyleOptionComplex, QStyleOptionButton
*/

/*!
    \enum QStyleOptionToolButton::ToolButtonFeature
    Describes the various features that a tool button can have.

    \value None A normal tool button.
    \value Arrow The tool button is an arrow.
    \value Menu The tool button has a menu.
    \value PopupDelay There is a delay to showing the menu.
    \value HasMenu The button has a popup menu.
    \value MenuButtonPopup The button should display an arrow to
           indicate that a menu is present.

    \sa features, QToolButton::toolButtonStyle(), QToolButton::popupMode()
*/

/*!
    Constructs a QStyleOptionToolButton, initializing the members
    variables to their default values.
*/

QStyleOptionToolButton::QStyleOptionToolButton()
    : QStyleOptionComplex(Version, SO_ToolButton), features(None), arrowType(Qt::DownArrow)
    , toolButtonStyle(Qt::ToolButtonIconOnly)
{
}

/*!
    \internal
*/
QStyleOptionToolButton::QStyleOptionToolButton(int version)
    : QStyleOptionComplex(version, SO_ToolButton), features(None), arrowType(Qt::DownArrow)
    , toolButtonStyle(Qt::ToolButtonIconOnly)

{
}

/*!
    \fn QStyleOptionToolButton::QStyleOptionToolButton(const QStyleOptionToolButton &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \enum QStyleOptionToolButton::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_ToolButton} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionToolButton::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 1

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \variable QStyleOptionToolButton::features
    \brief an OR combination of the tool button's features

    The default value is \l None.

    \sa ToolButtonFeature
*/

/*!
    \variable QStyleOptionToolButton::icon
    \brief the icon for the tool button

    The default value is an empty icon, i.e. an icon with neither a
    pixmap nor a filename.

    \sa iconSize
*/

/*!
    \variable QStyleOptionToolButton::iconSize
    \brief the size of the icon for the tool button

    The default value is QSize(-1, -1), i.e. an invalid size.
*/

/*!
    \variable QStyleOptionToolButton::text
    \brief the text of the tool button

    This value is only used if toolButtonStyle is
    Qt::ToolButtonTextUnderIcon, Qt::ToolButtonTextBesideIcon, or
    Qt::ToolButtonTextOnly. The default value is an empty string.
*/

/*!
    \variable QStyleOptionToolButton::arrowType
    \brief the direction of the arrow for the tool button

    This value is only used if \l features includes \l Arrow. The
    default value is Qt::DownArrow.
*/

/*!
    \variable QStyleOptionToolButton::toolButtonStyle
    \brief a Qt::ToolButtonStyle value describing the appearance of
    the tool button

    The default value is Qt::ToolButtonIconOnly.

    \sa QToolButton::toolButtonStyle()
*/

/*!
    \variable QStyleOptionToolButton::pos
    \brief the position of the tool button

    The default value is a null point, i.e. (0, 0)
*/

/*!
    \variable QStyleOptionToolButton::font
    \brief the font that is used for the text

    This value is only used if toolButtonStyle is
    Qt::ToolButtonTextUnderIcon, Qt::ToolButtonTextBesideIcon, or
    Qt::ToolButtonTextOnly. By default, the application's default font
    is used.
*/

/*!
    \class QStyleOptionComboBox
    \brief The QStyleOptionComboBox class is used to describe the
    parameter for drawing a combobox.

    \inmodule QtWidgets

    QStyleOptionButton contains all the information that QStyle
    functions need to draw QComboBox.

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator). This low-level feel
    makes the structures straightforward to use and emphasizes that
    these are simply parameters used by the style functions.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption, QStyleOptionComplex, QComboBox
*/

/*!
    Creates a QStyleOptionComboBox, initializing the members variables
    to their default values.
*/

QStyleOptionComboBox::QStyleOptionComboBox()
    : QStyleOptionComplex(Version, SO_ComboBox), editable(false), frame(true)
{
}

/*!
    \internal
*/
QStyleOptionComboBox::QStyleOptionComboBox(int version)
    : QStyleOptionComplex(version, SO_ComboBox), editable(false), frame(true)
{
}

/*!
    \fn QStyleOptionComboBox::QStyleOptionComboBox(const QStyleOptionComboBox &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \enum QStyleOptionComboBox::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_ComboBox} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionComboBox::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 1

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \variable QStyleOptionComboBox::editable
    \brief whether or not the combobox is editable or not

    the default
    value is false

    \sa QComboBox::isEditable()
*/


/*!
    \variable QStyleOptionComboBox::frame
    \brief whether the combo box has a frame

    The default value is true.
*/

/*!
    \variable QStyleOptionComboBox::currentText
    \brief the text for the current item of the combo box

    The default value is an empty string.
*/

/*!
    \variable QStyleOptionComboBox::currentIcon
    \brief the icon for the current item of the combo box

    The default value is an empty icon, i.e. an icon with neither a
    pixmap nor a filename.
*/

/*!
    \variable QStyleOptionComboBox::iconSize
    \brief the icon size for the current item of the combo box

    The default value is QSize(-1, -1), i.e. an invalid size.
*/

/*!
    \variable QStyleOptionComboBox::popupRect
    \brief the popup rectangle for the combobox

    The default value is a null rectangle, i.e. a rectangle with both
    the width and the height set to 0.

    This variable is currently unused. You can safely ignore it.

    \sa QStyle::SC_ComboBoxListBoxPopup
*/

/*!
    \class QStyleOptionToolBox
    \brief The QStyleOptionToolBox class is used to describe the
    parameters needed for drawing a tool box.

    \inmodule QtWidgets

    QStyleOptionToolBox contains all the information that QStyle
    functions need to draw QToolBox.

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator). This low-level feel
    makes the structures straightforward to use and emphasizes that
    these are simply parameters used by the style functions.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption, QToolBox
*/

/*!
    \typedef QStyleOptionToolBoxV2
    \relates QStyleOptionToolBox

    Synonym for QStyleOptionToolBox.
*/

/*!
    Creates a QStyleOptionToolBox, initializing the members variables
    to their default values.
*/

QStyleOptionToolBox::QStyleOptionToolBox()
    : QStyleOption(Version, SO_ToolBox), position(Beginning), selectedPosition(NotAdjacent)
{
}

/*!
    \internal
*/
QStyleOptionToolBox::QStyleOptionToolBox(int version)
    : QStyleOption(version, SO_ToolBox), position(Beginning), selectedPosition(NotAdjacent)
{
}

/*!
    \fn QStyleOptionToolBox::QStyleOptionToolBox(const QStyleOptionToolBox &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \enum QStyleOptionToolBox::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_ToolBox} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionToolBox::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 2

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \variable QStyleOptionToolBox::icon
    \brief the icon for the tool box tab

   The default value is an empty icon, i.e. an icon with neither a
   pixmap nor a filename.
*/

/*!
    \variable QStyleOptionToolBox::text
    \brief the text for the tool box tab

    The default value is an empty string.
*/

/*!
    \enum QStyleOptionToolBox::SelectedPosition

    This enum describes the position of the selected tab. Some styles
    need to draw a tab differently depending on whether or not it is
    adjacent to the selected tab.

    \value NotAdjacent The tab is not adjacent to a selected tab (or is the selected tab).
    \value NextIsSelected The next tab (typically the tab on the right) is selected.
    \value PreviousIsSelected The previous tab (typically the tab on the left) is selected.

    \sa selectedPosition
*/

/*!
    \enum QStyleOptionToolBox::TabPosition

    This enum describes tab positions relative to other tabs.

    \value Beginning The tab is the first (i.e., top-most) tab in
           the toolbox.
    \value Middle The tab is placed in the middle of the toolbox.
    \value End The tab is placed at the bottom of the toolbox.
    \value OnlyOneTab There is only one tab in the toolbox.
*/

/*!
    \variable QStyleOptionToolBox::selectedPosition
    \brief the position of the selected tab in relation to this tab

    The default value is NotAdjacent, i.e. the tab is not adjacent to
    a selected tab nor is it the selected tab.
*/

#ifndef QT_NO_RUBBERBAND
/*!
    \class QStyleOptionRubberBand
    \brief The QStyleOptionRubberBand class is used to describe the
    parameters needed for drawing a rubber band.

    \inmodule QtWidgets

    QStyleOptionRubberBand contains all the information that
    QStyle functions need to draw QRubberBand.

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator). This low-level feel
    makes the structures straightforward to use and emphasizes that
    these are simply parameters used by the style functions.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption, QRubberBand
*/

/*!
    Creates a QStyleOptionRubberBand, initializing the members
    variables to their default values.
*/

QStyleOptionRubberBand::QStyleOptionRubberBand()
    : QStyleOption(Version, SO_RubberBand), shape(QRubberBand::Line), opaque(false)
{
}

/*!
    \internal
*/
QStyleOptionRubberBand::QStyleOptionRubberBand(int version)
    : QStyleOption(version, SO_RubberBand), shape(QRubberBand::Line), opaque(false)
{
}

/*!
    \fn QStyleOptionRubberBand::QStyleOptionRubberBand(const QStyleOptionRubberBand &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \enum QStyleOptionRubberBand::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_RubberBand} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionRubberBand::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 1

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \variable QStyleOptionRubberBand::shape
    \brief the shape of the rubber band

    The default shape is QRubberBand::Line.
*/

/*!
    \variable QStyleOptionRubberBand::opaque
    \brief whether the rubber band is required to be drawn in an opaque style

    The default value is true.
*/
#endif // QT_NO_RUBBERBAND

/*!
    \class QStyleOptionTitleBar
    \brief The QStyleOptionTitleBar class is used to describe the
    parameters for drawing a title bar.

    \inmodule QtWidgets

    QStyleOptionTitleBar contains all the information that QStyle
    functions need to draw the title bar of a QMdiSubWindow.

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator). This low-level feel
    makes the structures straightforward to use and emphasizes that
    these are simply parameters used by the style functions.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption, QStyleOptionComplex, QMdiSubWindow
*/

/*!
    Constructs a QStyleOptionTitleBar, initializing the members
    variables to their default values.
*/

QStyleOptionTitleBar::QStyleOptionTitleBar()
    : QStyleOptionComplex(Version, SO_TitleBar), titleBarState(0), titleBarFlags(0)
{
}

/*!
    \fn QStyleOptionTitleBar::QStyleOptionTitleBar(const QStyleOptionTitleBar &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \enum QStyleOptionTitleBar::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_TitleBar} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionTitleBar::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 1

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \internal
*/
QStyleOptionTitleBar::QStyleOptionTitleBar(int version)
    : QStyleOptionComplex(version, SO_TitleBar), titleBarState(0), titleBarFlags(0)
{
}


/*!
    \variable QStyleOptionTitleBar::text
    \brief the text of the title bar

    The default value is an empty string.
*/

/*!
    \variable QStyleOptionTitleBar::icon
    \brief the icon for the title bar

    The default value is an empty icon, i.e. an icon with neither a
    pixmap nor a filename.
*/

/*!
    \variable QStyleOptionTitleBar::titleBarState
    \brief the state of the title bar

    This is basically the window state of the underlying widget. The
    default value is 0.

    \sa QWidget::windowState()
*/

/*!
    \variable QStyleOptionTitleBar::titleBarFlags
    \brief the widget flags for the title bar

    The default value is Qt::Widget.

    \sa Qt::WindowFlags
*/

#ifndef QT_NO_ITEMVIEWS
/*!
    \class QStyleOptionViewItem
    \brief The QStyleOptionViewItem class is used to describe the
    parameters used to draw an item in a view widget.

    \inmodule QtWidgets

    QStyleOptionViewItem contains all the information that QStyle
    functions need to draw the items for Qt's model/view classes.

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator). This low-level feel
    makes the structures straightforward to use and emphasizes that
    these are simply parameters used by the style functions.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption, {model-view-programming.html}{Model/View
    Programming}
*/

/*!
    \typedef QStyleOptionViewItemV2
    \relates QStyleOptionViewItem

    Synonym for QStyleOptionViewItem.
*/

/*!
    \typedef QStyleOptionViewItemV3
    \relates QStyleOptionViewItem

    Synonym for QStyleOptionViewItem.
*/

/*!
    \typedef QStyleOptionViewItemV4
    \relates QStyleOptionViewItem

    Synonym for QStyleOptionViewItem.
*/

/*!
    \enum QStyleOptionViewItem::Position

    This enum describes the position of the item's decoration.

    \value Left On the left of the text.
    \value Right On the right of the text.
    \value Top Above the text.
    \value Bottom Below the text.

    \sa decorationPosition
*/

/*!
    \variable QStyleOptionViewItem::showDecorationSelected
    \brief whether the decoration should be highlighted on selected
    items

    If this option is true, the branch and any decorations on selected items
    should be highlighted, indicating that the item is selected; otherwise, no
    highlighting is required. The default value is false.

    \sa QStyle::SH_ItemView_ShowDecorationSelected, QAbstractItemView
*/

/*!
    \variable QStyleOptionViewItem::textElideMode
    \brief where ellipsis should be added for text that is too long to fit
    into an item

    The default value is Qt::ElideMiddle, i.e. the ellipsis appears in
    the middle of the text.

    \sa Qt::TextElideMode, QStyle::SH_ItemView_EllipsisLocation
*/

/*!
    Constructs a QStyleOptionViewItem, initializing the members
    variables to their default values.
*/

QStyleOptionViewItem::QStyleOptionViewItem()
    : QStyleOption(Version, SO_ViewItem),
      displayAlignment(Qt::AlignLeft), decorationAlignment(Qt::AlignLeft),
      textElideMode(Qt::ElideMiddle), decorationPosition(Left),
      showDecorationSelected(false), features(None), widget(0),
      checkState(Qt::Unchecked), viewItemPosition(QStyleOptionViewItem::Invalid)
{
}

/*!
    \internal
*/
QStyleOptionViewItem::QStyleOptionViewItem(int version)
    : QStyleOption(version, SO_ViewItem),
      displayAlignment(Qt::AlignLeft), decorationAlignment(Qt::AlignLeft),
      textElideMode(Qt::ElideMiddle), decorationPosition(Left),
      showDecorationSelected(false), features(None), widget(0),
      checkState(Qt::Unchecked), viewItemPosition(QStyleOptionViewItem::Invalid)
{
}

/*!
    \fn QStyleOptionViewItem::QStyleOptionViewItem(const QStyleOptionViewItem &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \enum QStyleOptionViewItem::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_ViewItem} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionViewItem::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 4

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \variable QStyleOptionViewItem::displayAlignment
    \brief the alignment of the display value for the item

    The default value is Qt::AlignLeft.
*/

/*!
    \variable QStyleOptionViewItem::decorationAlignment
    \brief the alignment of the decoration for the item

    The default value is Qt::AlignLeft.
*/

/*!
    \variable QStyleOptionViewItem::decorationPosition
    \brief the position of the decoration for the item

    The default value is \l Left.

    \sa Position
*/

/*!
    \variable QStyleOptionViewItem::decorationSize
    \brief the size of the decoration for the item

    The default value is QSize(-1, -1), i.e. an invalid size.

    \sa decorationAlignment, decorationPosition
*/

/*!
    \variable QStyleOptionViewItem::font
    \brief the font used for the item

    By default, the application's default font is used.

    \sa QFont
*/

/*!
    \variable QStyleOptionViewItem::features
    \brief a bitwise OR of the features that describe this view item

    \sa ViewItemFeature
*/

/*!
    \enum QStyleOptionViewItem::ViewItemFeature

    This enum describes the different types of features an item can have.

    \value None      Indicates a normal item.
    \value WrapText  Indicates an item with wrapped text.
    \value Alternate Indicates that the item's background is rendered using alternateBase.
    \value HasCheckIndicator Indicates that the item has a check state indicator.
    \value HasDisplay        Indicates that the item has a display role.
    \value HasDecoration     Indicates that the item has a decoration role.
*/

/*!
    \variable QStyleOptionViewItem::index

    The model index that is to be drawn.
*/

/*!
    \variable QStyleOptionViewItem::checkState

    If this view item is checkable, i.e.,
    ViewItemFeature::HasCheckIndicator is true, \c checkState is true
    if the item is checked; otherwise, it is false.

*/

/*!
    \variable QStyleOptionViewItem::icon

    The icon (if any) to be drawn in the view item.
*/


/*!
    \variable QStyleOptionViewItem::text

    The text (if any) to be drawn in the view item.
*/

/*!
    \variable QStyleOptionViewItem::backgroundBrush

    The QBrush that should be used to paint the view items
    background.
*/

/*!
    \variable QStyleOptionViewItem::viewItemPosition

    Gives the position of this view item relative to other items. See
    the \l{QStyleOptionViewItem::}{ViewItemPosition} enum for the
    details.
*/

/*!
    \enum QStyleOptionViewItem::ViewItemPosition

    This enum is used to represent the placement of the item on
    a row. This can be used to draw items differently depending
    on their placement, for example by putting rounded edges at
    the beginning and end, and straight edges in between.

    \value Invalid   The ViewItemPosition is unknown and should be
                     disregarded.
    \value Beginning The item appears at the beginning of the row.
    \value Middle    The item appears in the middle of the row.
    \value End       The item appears at the end of the row.
    \value OnlyOne   The item is the only one on the row, and is
                     therefore both at the beginning and the end.
*/

#endif // QT_NO_ITEMVIEWS
/*!
    \fn T qstyleoption_cast<T>(const QStyleOption *option)
    \relates QStyleOption

    Returns a T or 0 depending on the \l{QStyleOption::type}{type} and
    \l{QStyleOption::version}{version} of the given \a option.

    Example:

    \snippet qstyleoption/main.cpp 4

    \sa QStyleOption::type, QStyleOption::version
*/

/*!
    \fn T qstyleoption_cast<T>(QStyleOption *option)
    \overload
    \relates QStyleOption

    Returns a T or 0 depending on the type of the given \a option.
*/

#ifndef QT_NO_TABWIDGET
/*!
    \class QStyleOptionTabWidgetFrame
    \brief The QStyleOptionTabWidgetFrame class is used to describe the
    parameters for drawing the frame around a tab widget.

    \inmodule QtWidgets

    QStyleOptionTabWidgetFrame contains all the information that
    QStyle functions need to draw the frame around QTabWidget.

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator). This low-level feel
    makes the structures straightforward to use and emphasizes that
    these are simply parameters used by the style functions.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption, QTabWidget
*/

/*!
    \typedef QStyleOptionTabWidgetFrameV2
    \relates QStyleOptionTabWidgetFrame

    Synonym for QStyleOptionTabWidgetFrame.
*/

/*!
    Constructs a QStyleOptionTabWidgetFrame, initializing the members
    variables to their default values.
*/
QStyleOptionTabWidgetFrame::QStyleOptionTabWidgetFrame()
    : QStyleOption(Version, SO_TabWidgetFrame), lineWidth(0), midLineWidth(0),
      shape(QTabBar::RoundedNorth)
{
}

/*!
    \fn QStyleOptionTabWidgetFrame::QStyleOptionTabWidgetFrame(const QStyleOptionTabWidgetFrame &other)

    Constructs a copy of \a other.
*/

/*! \internal */
QStyleOptionTabWidgetFrame::QStyleOptionTabWidgetFrame(int version)
    : QStyleOption(version, SO_TabWidgetFrame), lineWidth(0), midLineWidth(0),
      shape(QTabBar::RoundedNorth)
{
}

/*!
    \enum QStyleOptionTabWidgetFrame::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_TabWidgetFrame} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionTabWidgetFrame::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 2

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \variable QStyleOptionTabWidgetFrame::lineWidth
    \brief the line width for drawing the panel

    The default value is 0.
*/

/*!
    \variable QStyleOptionTabWidgetFrame::midLineWidth
    \brief the mid-line width for drawing the panel

    The mid line width is usually used in drawing sunken or raised
    frames. The default value is 0.
*/

/*!
    \variable QStyleOptionTabWidgetFrame::shape
    \brief the tab shape used to draw the tabs

    The default value is QTabBar::RoundedNorth.
*/

/*!
    \variable QStyleOptionTabWidgetFrame::tabBarSize
    \brief the size of the tab bar

    The default value is QSize(-1, -1), i.e. an invalid size.
*/

/*!
    \variable QStyleOptionTabWidgetFrame::rightCornerWidgetSize
    \brief the size of the right-corner widget

    The default value is QSize(-1, -1), i.e. an invalid size.
*/

/*! \variable QStyleOptionTabWidgetFrame::leftCornerWidgetSize
    \brief the size of the left-corner widget

    The default value is QSize(-1, -1), i.e. an invalid size.
*/


/*!
    \variable QStyleOptionTabWidgetFrame::tabBarRect
    \brief the rectangle containing all the tabs

    The default value is a null rectangle, i.e. a rectangle with both
    the width and the height set to 0.
*/

/*!
    \variable QStyleOptionTabWidgetFrame::selectedTabRect
    \brief the rectangle containing the selected tab

    This rectangle is contained within the tabBarRect. The default
    value is a null rectangle, i.e. a rectangle with both the width
    and the height set to 0.
*/

#endif // QT_NO_TABWIDGET

#ifndef QT_NO_TABBAR

/*!
    \class QStyleOptionTabBarBase
    \brief The QStyleOptionTabBarBase class is used to describe
    the base of a tab bar, i.e. the part that the tab bar usually
    overlaps with.

    \inmodule QtWidgets

    QStyleOptionTabBarBase  contains all the information that QStyle
    functions need to draw the tab bar base. Note that this is only
    drawn for a standalone QTabBar (one that isn't part of a
    QTabWidget).

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator). This low-level feel
    makes the structures straightforward to use and emphasizes that
    these are simply parameters used by the style functions.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption, QTabBar::drawBase()
*/

/*!
    \typedef QStyleOptionTabBarBaseV2
    \relates QStyleOptionTabBarBase

    Synonym for QStyleOptionTabBarBase.
*/

/*!
    Construct a QStyleOptionTabBarBase, initializing the members
    vaiables to their default values.
*/
QStyleOptionTabBarBase::QStyleOptionTabBarBase()
    : QStyleOption(Version, SO_TabBarBase), shape(QTabBar::RoundedNorth),
      documentMode(false)
{
}

/*! \internal */
QStyleOptionTabBarBase::QStyleOptionTabBarBase(int version)
    : QStyleOption(version, SO_TabBarBase), shape(QTabBar::RoundedNorth),
      documentMode(false)
{
}

/*!
    \fn QStyleOptionTabBarBase::QStyleOptionTabBarBase(const QStyleOptionTabBarBase &other)

    Constructs a copy of \a other.
*/

/*!
    \enum QStyleOptionTabBarBase::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_TabBarBase} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionTabBarBase::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 2

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \variable QStyleOptionTabBarBase::shape
    \brief the shape of the tab bar

    The default value is QTabBar::RoundedNorth.
*/

/*!
    \variable QStyleOptionTabBarBase::tabBarRect
    \brief the rectangle containing all the tabs

    The default value is a null rectangle, i.e. a rectangle with both
    the width and the height set to 0.
*/

/*!
    \variable QStyleOptionTabBarBase::selectedTabRect
    \brief the rectangle containing the selected tab

    This rectangle is contained within the tabBarRect. The default
    value is a null rectangle, i.e. a rectangle with both the width
    and the height set to 0.
*/


/*!
    \variable QStyleOptionTabBarBase::documentMode
    \brief whether the tabbar is in document mode.

    The default value is false;
*/

#endif // QT_NO_TABBAR

#ifndef QT_NO_SIZEGRIP
/*!
    \class QStyleOptionSizeGrip
    \brief The QStyleOptionSizeGrip class is used to describe the
    parameter for drawing a size grip.
    \since 4.2
    \inmodule QtWidgets

    QStyleOptionButton contains all the information that QStyle
    functions need to draw QSizeGrip.

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator). This low-level feel
    makes the structures straightforward to use and emphasizes that
    these are simply parameters used by the style functions.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption, QStyleOptionComplex, QSizeGrip
*/

/*!
    Constructs a QStyleOptionSizeGrip.
*/
QStyleOptionSizeGrip::QStyleOptionSizeGrip()
    : QStyleOptionComplex(Version, Type), corner(Qt::BottomRightCorner)
{
}

/*!
    \fn QStyleOptionSizeGrip::QStyleOptionSizeGrip(const QStyleOptionSizeGrip &other)

    Constructs a copy of the \a other style option.
*/

/*!
    \internal
*/
QStyleOptionSizeGrip::QStyleOptionSizeGrip(int version)
    : QStyleOptionComplex(version, Type), corner(Qt::BottomRightCorner)
{
}

/*!
    \variable QStyleOptionSizeGrip::corner

    The corner in which the size grip is located.
*/

/*!
    \enum QStyleOptionSizeGrip::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l{SO_TabBarBase} for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionSizeGrip::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 1

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/
#endif // QT_NO_SIZEGRIP

/*!
    \class QStyleOptionGraphicsItem
    \brief The QStyleOptionGraphicsItem class is used to describe
    the parameters needed to draw a QGraphicsItem.
    \since 4.2
    \ingroup graphicsview-api
    \inmodule QtWidgets

    For performance reasons, the access to the member variables is
    direct (i.e., using the \c . or \c -> operator). This low-level feel
    makes the structures straightforward to use and emphasizes that
    these are simply parameters.

    For an example demonstrating how style options can be used, see
    the \l {widgets/styles}{Styles} example.

    \sa QStyleOption, QGraphicsItem::paint()
*/

/*!
    \enum QStyleOptionGraphicsItem::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleOption subclass.

    \value Type The type of style option provided (\l SO_GraphicsItem for this class).

    The type is used internally by QStyleOption, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleOption subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleOptionGraphicsItem::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleOption subclass.

    \value Version 1

    The version is used by QStyleOption subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    Constructs a QStyleOptionGraphicsItem.
*/
QStyleOptionGraphicsItem::QStyleOptionGraphicsItem()
    : QStyleOption(Version, Type), levelOfDetail(1)
{
}

/*!
    \internal
*/
QStyleOptionGraphicsItem::QStyleOptionGraphicsItem(int version)
    : QStyleOption(version, Type), levelOfDetail(1)
{
}

/*!
    \since 4.6

    Returns the level of detail from the \a worldTransform.

    Its value represents the maximum value of the height and
    width of a unity rectangle, mapped using the \a worldTransform
    of the painter used to draw the item. By default, if no
    transformations are applied, its value is 1. If zoomed out 1:2, the level
    of detail will be 0.5, and if zoomed in 2:1, its value is 2.
*/
qreal QStyleOptionGraphicsItem::levelOfDetailFromTransform(const QTransform &worldTransform)
{
    if (worldTransform.type() <= QTransform::TxTranslate)
        return 1; // Translation only? The LOD is 1.

    // Two unit vectors.
    QLineF v1(0, 0, 1, 0);
    QLineF v2(0, 0, 0, 1);
    // LOD is the transformed area of a 1x1 rectangle.
    return qSqrt(worldTransform.map(v1).length() * worldTransform.map(v2).length());
}

/*!
    \fn QStyleOptionGraphicsItem::QStyleOptionGraphicsItem(const QStyleOptionGraphicsItem &other)

    Constructs a copy of \a other.
*/

/*!
    \variable QStyleOptionGraphicsItem::exposedRect
    \brief the exposed rectangle, in item coordinates

    Make use of this rectangle to speed up item drawing when only parts of the
    item are exposed. If the whole item is exposed, this rectangle will be the
    same as QGraphicsItem::boundingRect().

    This member is only initialized for items that have the
    QGraphicsItem::ItemUsesExtendedStyleOption flag set.
*/

/*!
     \variable QStyleOptionGraphicsItem::matrix
     \brief the complete transformation matrix for the item
     \obsolete

     The QMatrix provided through this member does include information about
     any perspective transformations applied to the view or item. To get the
     correct transformation matrix, use QPainter::transform() on the painter
     passed into the QGraphicsItem::paint() implementation.

     This matrix is the combination of the item's scene matrix and the matrix
     of the painter used for drawing the item. It is provided for convenience,
     allowing anvanced level-of-detail metrics that can be used to speed up
     item drawing.

     To find the dimensions of an item in screen coordinates (i.e., pixels),
     you can use the mapping functions of QMatrix, such as QMatrix::map().

     This member is only initialized for items that have the
     QGraphicsItem::ItemUsesExtendedStyleOption flag set.

     \sa QStyleOptionGraphicsItem::levelOfDetailFromTransform()
*/

/*!
    \variable QStyleOptionGraphicsItem::levelOfDetail
    \obsolete

    Use QStyleOptionGraphicsItem::levelOfDetailFromTransform()
    together with QPainter::worldTransform() instead.
*/

/*!
    \class QStyleHintReturn
    \brief The QStyleHintReturn class provides style hints that return more
    than basic data types.

    \ingroup appearance
    \inmodule QtWidgets

    QStyleHintReturn and its subclasses are used to pass information
    from a style back to the querying widget. This is most useful
    when the return value from QStyle::styleHint() does not provide enough
    detail; for example, when a mask is to be returned.

    \omit
    ### --Sam
    \endomit
*/

/*!
    \enum QStyleHintReturn::HintReturnType

    \value SH_Default QStyleHintReturn
    \value SH_Mask \l QStyle::SH_RubberBand_Mask QStyle::SH_FocusFrame_Mask
    \value SH_Variant \l QStyle::SH_TextControl_FocusIndicatorTextCharFormat
*/

/*!
    \enum QStyleHintReturn::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleHintReturn subclass.

    \value Type The type of style option provided (\l SH_Default for
           this class).

    The type is used internally by QStyleHintReturn, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleHintReturn subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleHintReturn::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleHintReturn subclass.

    \value Version 1

    The version is used by QStyleHintReturn subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \variable QStyleHintReturn::type
    \brief the type of the style hint container

    \sa HintReturnType
*/

/*!
    \variable QStyleHintReturn::version
    \brief the version of the style hint return container

    This value can be used by subclasses to implement extensions
    without breaking compatibility. If you use qstyleoption_cast<T>(), you
    normally do not need to check it.
*/

/*!
    Constructs a QStyleHintReturn with version \a version and type \a
    type.

    The version has no special meaning for QStyleHintReturn; it can be
    used by subclasses to distinguish between different version of
    the same hint type.

    \sa QStyleOption::version, QStyleOption::type
*/

QStyleHintReturn::QStyleHintReturn(int version, int type)
    : version(version), type(type)
{
}

/*!
    \internal
*/

QStyleHintReturn::~QStyleHintReturn()
{

}

/*!
    \class QStyleHintReturnMask
    \brief The QStyleHintReturnMask class provides style hints that return a QRegion.

    \ingroup appearance
    \inmodule QtWidgets

    \omit
    ### --Sam
    \endomit
*/

/*!
    \variable QStyleHintReturnMask::region
    \brief the region for style hints that return a QRegion
*/

/*!
    Constructs a QStyleHintReturnMask. The member variables are
    initialized to default values.
*/
QStyleHintReturnMask::QStyleHintReturnMask() : QStyleHintReturn(Version, Type)
{
}

/*!
    Destructor.
*/
QStyleHintReturnMask::~QStyleHintReturnMask()
{
}

/*!
    \enum QStyleHintReturnMask::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleHintReturn subclass.

    \value Type The type of style option provided (\l{SH_Mask} for
           this class).

    The type is used internally by QStyleHintReturn, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleHintReturn subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleHintReturnMask::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleHintReturn subclass.

    \value Version 1

    The version is used by QStyleHintReturn subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \class QStyleHintReturnVariant
    \brief The QStyleHintReturnVariant class provides style hints that return a QVariant.
    \since 4.3
    \ingroup appearance
    \inmodule QtWidgets
*/

/*!
    \variable QStyleHintReturnVariant::variant
    \brief the variant for style hints that return a QVariant
*/

/*!
    Constructs a QStyleHintReturnVariant. The member variables are
    initialized to default values.
*/
QStyleHintReturnVariant::QStyleHintReturnVariant() : QStyleHintReturn(Version, Type)
{
}

/*!
    Destructor.
*/
QStyleHintReturnVariant::~QStyleHintReturnVariant()
{
}

/*!
    \enum QStyleHintReturnVariant::StyleOptionType

    This enum is used to hold information about the type of the style option, and
    is defined for each QStyleHintReturn subclass.

    \value Type The type of style option provided (\l{SH_Variant} for
           this class).

    The type is used internally by QStyleHintReturn, its subclasses, and
    qstyleoption_cast() to determine the type of style option. In
    general you do not need to worry about this unless you want to
    create your own QStyleHintReturn subclass and your own styles.

    \sa StyleOptionVersion
*/

/*!
    \enum QStyleHintReturnVariant::StyleOptionVersion

    This enum is used to hold information about the version of the style option, and
    is defined for each QStyleHintReturn subclass.

    \value Version 1

    The version is used by QStyleHintReturn subclasses to implement
    extensions without breaking compatibility. If you use
    qstyleoption_cast(), you normally do not need to check it.

    \sa StyleOptionType
*/

/*!
    \fn T qstyleoption_cast<T>(const QStyleHintReturn *hint)
    \relates QStyleHintReturn

    Returns a T or 0 depending on the \l{QStyleHintReturn::type}{type}
    and \l{QStyleHintReturn::version}{version} of \a hint.

    Example:

    \snippet code/src_gui_styles_qstyleoption.cpp 0

    \sa QStyleHintReturn::type, QStyleHintReturn::version
*/

/*!
    \fn T qstyleoption_cast<T>(QStyleHintReturn *hint)
    \overload
    \relates QStyleHintReturn

    Returns a T or 0 depending on the type of \a hint.
*/

#if !defined(QT_NO_DEBUG_STREAM)
QDebug operator<<(QDebug debug, const QStyleOption::OptionType &optionType)
{
#if !defined(QT_NO_DEBUG)
    switch (optionType) {
    case QStyleOption::SO_Default:
        debug << "SO_Default"; break;
    case QStyleOption::SO_FocusRect:
        debug << "SO_FocusRect"; break;
    case QStyleOption::SO_Button:
        debug << "SO_Button"; break;
    case QStyleOption::SO_Tab:
        debug << "SO_Tab"; break;
    case QStyleOption::SO_MenuItem:
        debug << "SO_MenuItem"; break;
    case QStyleOption::SO_Frame:
        debug << "SO_Frame"; break;
    case QStyleOption::SO_ProgressBar:
        debug << "SO_ProgressBar"; break;
    case QStyleOption::SO_ToolBox:
        debug << "SO_ToolBox"; break;
    case QStyleOption::SO_Header:
        debug << "SO_Header"; break;
    case QStyleOption::SO_DockWidget:
        debug << "SO_DockWidget"; break;
    case QStyleOption::SO_ViewItem:
        debug << "SO_ViewItem"; break;
    case QStyleOption::SO_TabWidgetFrame:
        debug << "SO_TabWidgetFrame"; break;
    case QStyleOption::SO_TabBarBase:
        debug << "SO_TabBarBase"; break;
    case QStyleOption::SO_RubberBand:
        debug << "SO_RubberBand"; break;
    case QStyleOption::SO_Complex:
        debug << "SO_Complex"; break;
    case QStyleOption::SO_Slider:
        debug << "SO_Slider"; break;
    case QStyleOption::SO_SpinBox:
        debug << "SO_SpinBox"; break;
    case QStyleOption::SO_ToolButton:
        debug << "SO_ToolButton"; break;
    case QStyleOption::SO_ComboBox:
        debug << "SO_ComboBox"; break;
    case QStyleOption::SO_TitleBar:
        debug << "SO_TitleBar"; break;
    case QStyleOption::SO_CustomBase:
        debug << "SO_CustomBase"; break;
    case QStyleOption::SO_GroupBox:
        debug << "SO_GroupBox"; break;
    case QStyleOption::SO_ToolBar:
        debug << "SO_ToolBar"; break;
    case QStyleOption::SO_ComplexCustomBase:
        debug << "SO_ComplexCustomBase"; break;
    case QStyleOption::SO_SizeGrip:
        debug << "SO_SizeGrip"; break;
    case QStyleOption::SO_GraphicsItem:
        debug << "SO_GraphicsItem"; break;
    }
#else
    Q_UNUSED(optionType);
#endif
    return debug;
}

QDebug operator<<(QDebug debug, const QStyleOption &option)
{
#if !defined(QT_NO_DEBUG)
    debug << "QStyleOption(";
    debug << QStyleOption::OptionType(option.type);
    debug << ',' << (option.direction == Qt::RightToLeft ? "RightToLeft" : "LeftToRight");
    debug << ',' << option.state;
    debug << ',' << option.rect;
    debug << ',' << option.styleObject;
    debug << ')';
#else
    Q_UNUSED(option);
#endif
    return debug;
}
#endif

QT_END_NAMESPACE
