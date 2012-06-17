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

#include "qaccessible.h"

#ifndef QT_NO_ACCESSIBILITY

#include "qaccessibleplugin.h"
#include "qaccessiblewidget.h"
#include "qapplication.h"
#include "qhash.h"
#include "qmetaobject.h"
#include "qmutex.h"
#include <private/qfactoryloader_p.h>

#include "qwidget.h"

QT_BEGIN_NAMESPACE

/*!
    \class QAccessible
    \brief The QAccessible class provides enums and static functions
    relating to accessibility.

    \ingroup accessibility


    Accessible applications can be used by people who are not able to
    use applications by conventional means.

    The functions in this class are used for communication between
    accessible applications (also called AT Servers) and
    accessibility tools (AT Clients), such as screen readers and
    braille displays. Clients and servers communicate in the following way:

    \list
    \o  \e{AT Servers} notify the clients about events through calls to the
        updateAccessibility() function.

    \o  \e{AT Clients} request information about the objects in the server.
        The QAccessibleInterface class is the core interface, and encapsulates
        this information in a pure virtual API. Implementations of the interface
        are provided by Qt through the queryAccessibleInterface() API.
    \endlist

    The communication between servers and clients is initialized by
    the setRootObject() function. Function pointers can be installed
    to replace or extend the default behavior of the static functions
    in QAccessible.

    Qt supports Microsoft Active Accessibility (MSAA), Mac OS X
    Accessibility, and the Unix/X11 AT-SPI standard. Other backends
    can be supported using QAccessibleBridge.

    In addition to QAccessible's static functions, Qt offers one
    generic interface, QAccessibleInterface, that can be used to wrap
    all widgets and objects (e.g., QPushButton). This single
    interface provides all the metadata necessary for the assistive
    technologies. Qt provides implementations of this interface for
    its built-in widgets as plugins.

    When you develop custom widgets, you can create custom subclasses
    of QAccessibleInterface and distribute them as plugins (using
    QAccessiblePlugin) or compile them into the application.
    Likewise, Qt's predefined accessibility support can be built as
    plugin (the default) or directly into the Qt library. The main
    advantage of using plugins is that the accessibility classes are
    only loaded into memory if they are actually used; they don't
    slow down the common case where no assistive technology is being
    used.

    Qt also includes two convenience classes, QAccessibleObject and
    QAccessibleWidget, that inherit from QAccessibleInterface and
    provide the lowest common denominator of metadata (e.g., widget
    geometry, window title, basic help text). You can use them as
    base classes when wrapping your custom QObject or QWidget
    subclasses.

    \sa QAccessibleInterface
*/

/*!
    \enum QAccessible::Action

    This enum describes the possible types of action that can occur.

    \value DefaultAction
    \value Press
    \value SetFocus
    \value Increase
    \value Decrease
    \value Accept
    \value Cancel
    \value Select
    \value ClearSelection
    \value RemoveSelection
    \value ExtendSelection
    \value AddToSelection

    \value FirstStandardAction
    \value LastStandardAction
*/

/*!
    \enum QAccessible::Method

    This enum describes the possible types of methods that can be
    invoked on an accessible object.

    \value ListSupportedMethods
    \value SetCursorPosition
    \value GetCursorPosition

    \omitvalue ForegroundColor
    \omitvalue BackgroundColor

    \sa QAccessibleInterface::invokeMethod()
*/

/*!
    \fn QSet<Method> QAccessibleInterface::supportedMethods()
    \since 4.3

    Returns a QSet of \l{QAccessible::}{Method}s that are supported by this
    accessible interface.

    \sa QAccessible::Method invokeMethod()
*/

/*!
    \enum QAccessible::StateFlag

    This enum type defines bit flags that can be combined to indicate
    the state of an accessible object. The values are:

    \value Animated         The object's appearance changes frequently.
    \value Busy             The object cannot accept input at the moment.
    \value Checked          The object's check box is checked.
    \value Collapsed        The object is collapsed, e.g. a closed listview item, or an iconified window.
    \value DefaultButton    The object represents the default button in a dialog.
    \value Expanded         The object is expandable, and currently the children are visible.
    \value ExtSelectable    The object supports extended selection.
    \value Focusable        The object can receive focus. Only objects in the active window can receive focus.
    \value Focused          The object has keyboard focus.
    \value HasPopup         The object opens a popup.
    \value HotTracked       The object's appearance is sensitive to the mouse cursor position.
    \value Invisible        The object is not visible to the user.
    \value Linked           The object is linked to another object, e.g. a hyperlink.
    \value Marqueed         The object displays scrolling contents, e.g. a log view.
    \value Mixed            The state of the object is not determined, e.g. a tri-state check box that is neither checked nor unchecked.
    \value Modal            The object blocks input from other objects.
    \value Movable          The object can be moved.
    \value MultiSelectable  The object supports multiple selected items.
    \value Normal           The normal state.
    \value Offscreen        The object is clipped by the visible area. Objects that are off screen are also invisible.
    \value Pressed          The object is pressed.
    \value Protected        The object is password protected, e.g. a line edit for entering a Password.
    \value ReadOnly         The object can usually be edited, but is explicitly set to read-only.
    \value Selectable       The object is selectable.
    \value Selected         The object is selected.
    \value SelfVoicing      The object describes itself through speech or sound.
    \value Sizeable         The object can be resized, e.g. top-level windows.
    \value Traversed        The object is linked and has been visited.
    \value Unavailable      The object is unavailable to the user, e.g. a disabled widget.
    \omitvalue Moveable
    \omitvalue HasInvokeExtension

    Implementations of QAccessibleInterface::state() return a combination
    of these flags.
*/

/*!
    \enum QAccessible::Event

    This enum type defines accessible event types.

    \value AcceleratorChanged               The keyboard accelerator for an action has been changed.
    \value ActionChanged                    An action has been changed.
    \value ActiveDescendantChanged
    \value Alert                            A system alert (e.g., a message from a QMessageBox)
    \value AttributeChanged
    \value ContextHelpEnd                   Context help (QWhatsThis) for an object is finished.
    \value ContextHelpStart                 Context help (QWhatsThis) for an object is initiated.
    \value DefaultActionChanged             The default QAccessible::Action for the accessible
                                            object has changed.
    \value DescriptionChanged               The object's QAccessible::Description changed.
    \value DialogEnd                        A dialog (QDialog) has been hidden
    \value DialogStart                      A dialog (QDialog) has been set visible.
    \value DocumentContentChanged           The contents of a text document have changed.
    \value DocumentLoadComplete             A document has been loaded.
    \value DocumentLoadStopped              A document load has been stopped.
    \value DocumentReload                   A document reload has been initiated.
    \value DragDropEnd                      A drag and drop operation is about to finished.
    \value DragDropStart                    A drag and drop operation is about to be initiated.
    \value Focus                            An object has gained keyboard focus.
    \value ForegroundChanged                A window has been activated (i.e., a new window has
                                            gained focus on the desktop).
    \value HelpChanged                      The QAccessible::Help text property of an object has
                                            changed.
    \value HyperlinkEndIndexChanged         The end position of the display text for a hypertext
                                            link has changed.
    \value HyperlinkNumberOfAnchorsChanged  The number of anchors in a hypertext link has changed,
                                            perhaps because the display text has been split to
                                            provide more than one link.
    \value HyperlinkSelectedLinkChanged     The link for the selected hypertext link has changed.
    \value HyperlinkStartIndexChanged       The start position of the display text for a hypertext
                                            link has changed.
    \value HypertextChanged                 The display text for a hypertext link has changed.
    \value HypertextLinkActivated           A hypertext link has been activated, perhaps by being
                                            clicked or via a key press.
    \value HypertextLinkSelected            A hypertext link has been selected.
    \value HypertextNLinksChanged
    \value LocationChanged                  An object's location on the screen has changed.
    \value MenuCommand                      A menu item is triggered.
    \value MenuEnd                          A menu has been closed (Qt uses PopupMenuEnd for all
                                            menus).
    \value MenuStart                        A menu has been opened on the menubar (Qt uses
                                            PopupMenuStart for all menus).
    \value NameChanged                      The QAccessible::Name property of an object has changed.
    \value ObjectAttributeChanged
    \value ObjectCreated                    A new object is created.
    \value ObjectDestroyed                  An object is deleted.
    \value ObjectHide                       An object is hidden; for example, with QWidget::hide().
                                            Any children the object that is hidden has do not send
                                            this event. It is not sent when an object is hidden as
                                            it is being obcured by others.
    \value ObjectReorder                    A layout or item view  has added, removed, or moved an
                                            object (Qt does not use this event).
    \value ObjectShow                       An object is displayed; for example, with
                                            QWidget::show().
    \value PageChanged
    \value ParentChanged                    An object's parent object changed.
    \value PopupMenuEnd                     A pop-up menu has closed.
    \value PopupMenuStart                   A pop-up menu has opened.
    \value ScrollingEnd                     A scrollbar scroll operation has ended (the mouse has
                                            released the slider handle).
    \value ScrollingStart                   A scrollbar scroll operation is about to start; this may
                                            be caused by a mouse press on the slider handle, for
                                            example.
    \value SectionChanged
    \value SelectionAdd                     An item has been added to the selection in an item view.
    \value SelectionRemove                  An item has been removed from an item view selection.
    \value Selection                        The selection has changed in a menu or item view.
    \value SelectionWithin                  Several changes to a selection has occurred in an item
                                            view.
    \value SoundPlayed                      A sound has been played by an object
    \value StateChanged                     The QAccessible::State of an object has changed.
    \value TableCaptionChanged              A table caption has been changed.
    \value TableColumnDescriptionChanged    The description of a table column, typically found in
                                            the column's header, has been changed.
    \value TableColumnHeaderChanged         A table column header has been changed.
    \value TableModelChanged                The model providing data for a table has been changed.
    \value TableRowDescriptionChanged       The description of a table row, typically found in the
                                            row's header, has been changed.
    \value TableRowHeaderChanged            A table row header has been changed.
    \value TableSummaryChanged              The summary of a table has been changed.
    \value TextAttributeChanged
    \value TextCaretMoved                   The caret has moved in an editable widget.
                                            The caret represents the cursor position in an editable
                                            widget with the input focus.
    \value TextColumnChanged                A text column has been changed.
    \value TextInserted                     Text has been inserted into an editable widget.
    \value TextRemoved                      Text has been removed from an editable widget.
    \value TextSelectionChanged             The selected text has changed in an editable widget.
    \value TextUpdated                      The text has been update in an editable widget.
    \value ValueChanged                     The QAccessible::Value of an object has changed.
    \value VisibleDataChanged

    The values for this enum are defined to be the same as those defined in the
    \l{AccessibleEventID.idl File Reference}{IAccessible2} and
    \l{Microsoft Active Accessibility Event Constants}{MSAA} specifications.
*/

/*!
    \enum QAccessible::Role

    This enum defines the role of an accessible object. The roles are:

    \value AlertMessage     An object that is used to alert the user.
    \value Animation        An object that displays an animation.
    \value Application      The application's main window.
    \value Assistant        An object that provids interactive help.
    \value Border           An object that represents a border.
    \value ButtonDropDown   A button that drops down a list of items.
    \value ButtonDropGrid   A button that drops down a grid.
    \value ButtonMenu       A button that drops down a menu.
    \value Canvas           An object that displays graphics that the user can interact with.
    \value Caret            An object that represents the system caret (text cursor).
    \value Cell             A cell in a table.
    \value Chart            An object that displays a graphical representation of data.
    \value CheckBox         An object that represents an option that can be checked or unchecked. Some options provide a "mixed" state, e.g. neither checked nor unchecked.
    \value Client           The client area in a window.
    \value Clock            A clock displaying time.
    \value Column           A column of cells, usually within a table.
    \value ColumnHeader     A header for a column of data.
    \value ComboBox         A list of choices that the user can select from.
    \value Cursor           An object that represents the mouse cursor.
    \value Desktop          The object represents the desktop or workspace.
    \value Dial             An object that represents a dial or knob.
    \value Dialog           A dialog box.
    \value Document         A document window, usually in an MDI environment.
    \value EditableText     Editable text
    \value Equation         An object that represents a mathematical equation.
    \value Graphic          A graphic or picture, e.g. an icon.
    \value Grip             A grip that the user can drag to change the size of widgets.
    \value Grouping         An object that represents a logical grouping of other objects.
    \value HelpBalloon      An object that displays help in a separate, short lived window.
    \value HotkeyField      A hotkey field that allows the user to enter a key sequence.
    \value Indicator        An indicator that represents a current value or item.
    \value LayeredPane      An object that can contain layered children, e.g. in a stack.
    \value Link             A link to something else.
    \value List             A list of items, from which the user can select one or more items.
    \value ListItem         An item in a list of items.
    \value MenuBar          A menu bar from which menus are opened by the user.
    \value MenuItem         An item in a menu or menu bar.
    \value NoRole           The object has no role. This usually indicates an invalid object.
    \value PageTab          A page tab that the user can select to switch to a different page in a dialog.
    \value PageTabList      A list of page tabs.
    \value Pane             A generic container.
    \value PopupMenu        A menu which lists options that the user can select to perform an action.
    \value ProgressBar      The object displays the progress of an operation in progress.
    \value PropertyPage     A property page where the user can change options and settings.
    \value PushButton       A button.
    \value RadioButton      An object that represents an option that is mutually exclusive with other options.
    \value Row              A row of cells, usually within a table.
    \value RowHeader        A header for a row of data.
    \value ScrollBar        A scroll bar, which allows the user to scroll the visible area.
    \value Separator        A separator that divides space into logical areas.
    \value Slider           A slider that allows the user to select a value within a given range.
    \value Sound            An object that represents a sound.
    \value SpinBox          A spin box widget that allows the user to enter a value within a given range.
    \value Splitter         A splitter distributing available space between its child widgets.
    \value StaticText       Static text, such as labels for other widgets.
    \value StatusBar        A status bar.
    \value Table            A table representing data in a grid of rows and columns.
    \value Terminal         A terminal or command line interface.
    \value TitleBar         The title bar caption of a window.
    \value ToolBar          A tool bar, which groups widgets that the user accesses frequently.
    \value ToolTip          A tool tip which provides information about other objects.
    \value Tree             A list of items in a tree structure.
    \value TreeItem         An item in a tree structure.
    \value UserRole         The first value to be used for user defined roles.
    \value Whitespace       Blank space between other objects.
    \value Window           A top level window.
*/

/*!
    \enum QAccessible::RelationFlag

    This enum type defines bit flags that can be combined to indicate
    the relationship between two accessible objects.

    \value Unrelated        The objects are unrelated.
    \value Self             The objects are the same.
    \value Ancestor         The first object is a parent of the second object.
    \value Child            The first object is a direct child of the second object.
    \value Descendent       The first object is an indirect child of the second object.
    \value Sibling          The objects are siblings.

    \value Up               The first object is above the second object.
    \value Down             The first object is below the second object.
    \value Left             The first object is left of the second object.
    \value Right            The first object is right of the second object.
    \value Covers           The first object covers the second object.
    \value Covered          The first object is covered by the second object.

    \value FocusChild       The first object is the second object's focus child.
    \value Label            The first object is the label of the second object.
    \value Labelled         The first object is labelled by the second object.
    \value Controller       The first object controls the second object.
    \value Controlled       The first object is controlled by the second object.

    \omitvalue HierarchyMask
    \omitvalue GeometryMask
    \omitvalue LogicalMask

    Implementations of relationTo() return a combination of these flags.
    Some values are mutually exclusive.

    Implementations of navigate() can accept only one distinct value.
*/

/*!
    \enum QAccessible::Text

    This enum specifies string information that an accessible object
    returns.

    \value Name         The name of the object. This can be used both
                        as an identifier or a short description by
                        accessible clients.
    \value Description  A short text describing the object.
    \value Value        The value of the object.
    \value Help         A longer text giving information about how to use the object.
    \value Accelerator  The keyboard shortcut that executes the object's default action.
    \value UserText     The first value to be used for user defined text.
*/

/*!
    \fn QAccessibleInterface::~QAccessibleInterface()

    Destroys the object.
*/

/*!
    \fn void QAccessible::initialize()
    \internal
*/

/*!
    \fn void QAccessible::cleanup()
    \internal
*/

#ifndef QT_NO_LIBRARY
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QAccessibleFactoryInterface_iid, QLatin1String("/accessible")))
#endif

Q_GLOBAL_STATIC(QList<QAccessible::InterfaceFactory>, qAccessibleFactories)

QAccessible::UpdateHandler QAccessible::updateHandler = 0;
QAccessible::RootObjectHandler QAccessible::rootObjectHandler = 0;

static bool accessibility_active = false;
static bool cleanupAdded = false;
static void qAccessibleCleanup()
{
    qAccessibleFactories()->clear();
}

/*!
    \typedef QAccessible::InterfaceFactory

    This is a typedef for a pointer to a function with the following
    signature:

    \snippet doc/src/snippets/code/src_gui_accessible_qaccessible.cpp 1

    The function receives a QString and a QObject pointer, where the
    QString is the key identifying the interface. The QObject is used
    to pass on to the QAccessibleInterface so that it can hold a reference
    to it.

    If the key and the QObject does not have a corresponding
    QAccessibleInterface, a null-pointer will be returned.

    Installed factories are called by queryAccessibilityInterface() until
    one provides an interface.
*/

/*!
    \typedef QAccessible::UpdateHandler

    \internal

    A function pointer type. Use a function with this prototype to install
    your own update function.

    The function is called by updateAccessibility().
*/

/*!
    \typedef QAccessible::RootObjectHandler

    \internal

    A function pointer type. Use a function with this prototype to install
    your own root object handler.

    The function is called by setRootObject().
*/

/*!
    Installs the InterfaceFactory \a factory. The last factory added
    is the first one used by queryAccessibleInterface().
*/
void QAccessible::installFactory(InterfaceFactory factory)
{
    if (!factory)
        return;

    if (!cleanupAdded) {
        qAddPostRoutine(qAccessibleCleanup);
        cleanupAdded = true;
    }
    if (qAccessibleFactories()->contains(factory))
        return;
    qAccessibleFactories()->append(factory);
}

/*!
    Removes \a factory from the list of installed InterfaceFactories.
*/
void QAccessible::removeFactory(InterfaceFactory factory)
{
    qAccessibleFactories()->removeAll(factory);
}

/*!
    \internal

    Installs the given \a handler as the function to be used by
    updateAccessibility(), and returns the previously installed
    handler.
*/
QAccessible::UpdateHandler QAccessible::installUpdateHandler(UpdateHandler handler)
{
    UpdateHandler old = updateHandler;
    updateHandler = handler;
    return old;
}

/*!
    Installs the given \a handler as the function to be used by setRootObject(),
    and returns the previously installed handler.
*/
QAccessible::RootObjectHandler QAccessible::installRootObjectHandler(RootObjectHandler handler)
{
    RootObjectHandler old = rootObjectHandler;
    rootObjectHandler = handler;
    return old;
}

/*!
    If a QAccessibleInterface implementation exists for the given \a object,
    this function returns a pointer to the implementation; otherwise it
    returns 0.

    The function calls all installed factory functions (from most
    recently installed to least recently installed) until one is found
    that provides an interface for the class of \a object. If no
    factory can provide an accessibility implementation for the class
    the function loads installed accessibility plugins, and tests if
    any of the plugins can provide the implementation.

    If no implementation for the object's class is available, the
    function tries to find an implementation for the object's parent
    class, using the above strategy.

    \warning The caller is responsible for deleting the returned
    interface after use.
*/
QAccessibleInterface *QAccessible::queryAccessibleInterface(QObject *object)
{
    accessibility_active = true;
    QAccessibleInterface *iface = 0;
    if (!object)
        return 0;

    const QMetaObject *mo = object->metaObject();
    while (mo) {
        const QLatin1String cn(mo->className());
        for (int i = qAccessibleFactories()->count(); i > 0; --i) {
            InterfaceFactory factory = qAccessibleFactories()->at(i - 1);
            iface = factory(cn, object);
            if (iface)
                return iface;
        }
#ifndef QT_NO_LIBRARY
        QAccessibleFactoryInterface *factory = qobject_cast<QAccessibleFactoryInterface*>(loader()->instance(cn));
        if (factory) {
            iface = factory->create(cn, object);
            if (iface)
                return iface;
        }
#endif
        mo = mo->superClass();
    }

    QWidget *widget = qobject_cast<QWidget*>(object);
    if (widget)
        return new QAccessibleWidget(widget);
    else if (object == qApp)
        return new QAccessibleApplication();

    return 0;
}

/*!
    Returns true if an accessibility implementation has been requested
    during the runtime of the application; otherwise returns false.

    Use this function to prevent potentially expensive notifications via
    updateAccessibility().
*/
bool QAccessible::isActive()
{
    return accessibility_active;
}

/*!
  \fn void QAccessible::setRootObject(QObject *object)

  Sets the root accessible object of this application to \a object.
  All other accessible objects in the application can be reached by the
  client using object navigation.

  You should never need to call this function. Qt sets the QApplication
  object as the root object immediately before the event loop is entered
  in QApplication::exec().

  Use QAccessible::installRootObjectHandler() to redirect the function
  call to a customized handler function.

  \sa queryAccessibleInterface()
*/

/*!
  \fn void QAccessible::updateAccessibility(QObject *object, int child, Event reason)

  Notifies accessibility clients about a change in \a object's
  accessibility information.

  \a reason specifies the cause of the change, for example,
  \c ValueChange when the position of a slider has been changed. \a
  child is the (1-based) index of the child element that has changed.
  When \a child is 0, the object itself has changed.

  Call this function whenever the state of your accessible object or
  one of its sub-elements has been changed either programmatically
  (e.g. by calling QLabel::setText()) or by user interaction.

  If there are no accessibility tools listening to this event, the
  performance penalty for calling this function is small, but if determining
  the parameters of the call is expensive you can test isActive() to
  avoid unnecessary computations.
*/


/*!
    \class QAccessibleInterface
    \brief The QAccessibleInterface class defines an interface that exposes information
    about accessible objects.

    \ingroup accessibility

    Accessibility tools (also called AT Clients), such as screen readers
    or braille displays, require high-level information about
    accessible objects in an application. Accessible objects provide
    specialized input and output methods, making it possible for users
    to use accessibility tools with enabled applications (AT Servers).

    Every element that the user needs to interact with or react to is
    an accessible object, and should provide this information. These
    are mainly visual objects, such as widgets and widget elements, but
    can also be content, such as sounds.

    The AT client uses three basic concepts to acquire information
    about any accessible object in an application:
    \list
    \i \e Properties The client can read information about
    accessible objects. In some cases the client can also modify these
    properties; such as text in a line edit.
    \i \e Actions The client can invoke actions like pressing a button
    or .
    \i \e{Relationships and Navigation} The client can traverse from one
    accessible object to another, using the relationships between objects.
    \endlist

    The QAccessibleInterface defines the API for these three concepts.

    \section1 Relationships and Navigation

    The functions childCount() and indexOfChild() return the number of
    children of an accessible object and the index a child object has
    in its parent. The childAt() function returns the index of a child
    at a given position.

    The relationTo() function provides information about how two
    different objects relate to each other, and navigate() allows
    traversing from one object to another object with a given
    relationship.

    \section1 Properties

    The central property of an accessible objects is what role() it
    has. Different objects can have the same role, e.g. both the "Add
    line" element in a scroll bar and the \c OK button in a dialog have
    the same role, "button". The role implies what kind of
    interaction the user can perform with the user interface element.

    An object's state() property is a combination of different state
    flags and can describe both how the object's state differs from a
    "normal" state, e.g. it might be unavailable, and also how it
    behaves, e.g. it might be selectable.

    The text() property provides textual information about the object.
    An object usually has a name, but can provide extended information
    such as a description, help text, or information about any
    keyboard accelerators it provides. Some objects allow changing the
    text() property through the setText() function, but this
    information is in most cases read-only.

    The rect() property provides information about the geometry of an
    accessible object. This information is usually only available for
    visual objects.

    \section1 Actions and Selection

    To enable the user to interact with an accessible object the
    object must expose information about the actions that it can
    perform. userActionCount() returns the number of actions supported by
    an accessible object, and actionText() returns textual information
    about those actions. doAction() invokes an action.

    Objects that support selections can define actions to change the selection.

    \section2 Objects and children

    A QAccessibleInterface provides information about the accessible
    object, and can also provide information for the children of that
    object if those children don't provide a QAccessibleInterface
    implementation themselves. This is practical if the object has
    many similar children (e.g. items in a list view), or if the
    children are an integral part of the object itself, for example, the
    different sections in a scroll bar.

    If an accessible object provides information about its children
    through one QAccessibleInterface, the children are referenced
    using indexes. The index is 1-based for the children, i.e. 0
    refers to the object itself, 1 to the first child, 2 to the second
    child, and so on.

    All functions in QAccessibleInterface that take a child index
    relate to the object itself if the index is 0, or to the child
    specified. If a child provides its own interface implementation
    (which can be retrieved through navigation) asking the parent for
    information about that child will usually not succeed.

    \sa QAccessible
*/

/*!
    \fn bool QAccessibleInterface::isValid() const

    Returns true if all the data necessary to use this interface
    implementation is valid (e.g. all pointers are non-null);
    otherwise returns false.

    \sa object()
*/

/*!
    \fn QObject *QAccessibleInterface::object() const

    Returns a pointer to the QObject this interface implementation provides
    information for.

    \sa isValid()
*/

/*!
    \fn int QAccessibleInterface::childCount() const

    Returns the number of children that belong to this object. A child
    can provide accessibility information on its own (e.g. a child
    widget), or be a sub-element of this accessible object.

    All objects provide this information.

    \sa indexOfChild()
*/

/*!
    \fn int QAccessibleInterface::indexOfChild(const QAccessibleInterface *child) const

    Returns the 1-based index of the object \a child in this object's
    children list, or -1 if \a child is not a child of this object. 0
    is not a possible return value.

    All objects provide this information about their children.

    \sa childCount()
*/

/*!
    \fn QAccessible::Relation QAccessibleInterface::relationTo(int child,
const QAccessibleInterface *other, int otherChild) const

    Returns the relationship between this object's \a child and the \a
    other object's \a otherChild. If \a child is 0 the object's own relation
    is returned.

    The returned value indicates the relation of the called object to
    the \a other object, e.g. if this object is a child of \a other
    the return value will be \c Child.

    The return value is a combination of the bit flags in the
    QAccessible::Relation enumeration.

    All objects provide this information.

    \sa indexOfChild(), navigate()
*/

/*!
    \fn int QAccessibleInterface::childAt(int x, int y) const

    Returns the 1-based index of the child that contains the screen
    coordinates (\a x, \a y). This function returns 0 if the point is
    positioned on the object itself. If the tested point is outside
    the boundaries of the object this function returns -1.

    This function is only relyable for visible objects (invisible
    object might not be laid out correctly).

    All visual objects provide this information.

    \sa rect()
*/

/*!
    \fn int QAccessibleInterface::navigate(RelationFlag relation, int entry, QAccessibleInterface
**target) const

    Navigates from this object to an object that has a relationship
    \a relation to this object, and returns the respective object in
    \a target. It is the caller's responsibility to delete *\a target
    after use.

    If an object is found, \a target is set to point to the object, and
    the index of the child of \a target is returned. The return value
    is 0 if \a target itself is the requested object. \a target is set
    to null if this object is the target object (i.e. the requested
    object is a handled by this object).

    If no object is found \a target is set to null, and the return
    value is -1.

    The \a entry parameter has two different meanings:
    \list
    \i \e{Hierarchical and Logical relationships} -- if multiple objects with
    the requested relationship exist \a entry specifies which one to
    return. \a entry is 1-based, e.g. use 1 to get the first (and
    possibly only) object with the requested relationship.

    The following code demonstrates how to use this function to
    navigate to the first child of an object:

    \snippet doc/src/snippets/code/src_gui_accessible_qaccessible.cpp 0

    \i \e{Geometric relationships} -- the index of the child from
    which to start navigating in the specified direction. \a entry
    can be 0 to navigate to a sibling of this object, or non-null to
    navigate within contained children that don't provide their own
    accessible information.
    \endlist

    Note that the \c Descendent value for \a relation is not supported.

    All objects support navigation.

    \sa relationTo(), childCount()
*/

/*!
    \fn QString QAccessibleInterface::text(Text t, int child) const

    Returns the value of the text property \a t of the object, or of
    the object's child if \a child is not 0.

    The \l Name is a string used by clients to identify, find, or
    announce an accessible object for the user. All objects must have
    a name that is unique within their container. The name can be
    used differently by clients, so the name should both give a
    short description of the object and be unique.

    An accessible object's \l Description provides textual information
    about an object's visual appearance. The description is primarily
    used to provide greater context for vision-impaired users, but is
    also used for context searching or other applications. Not all
    objects have a description. An "OK" button would not need a
    description, but a tool button that shows a picture of a smiley
    would.

    The \l Value of an accessible object represents visual information
    contained by the object, e.g. the text in a line edit. Usually,
    the value can be modified by the user. Not all objects have a
    value, e.g. static text labels don't, and some objects have a
    state that already is the value, e.g. toggle buttons.

    The \l Help text provides information about the function and
    usage of an accessible object. Not all objects provide this
    information.

    The \l Accelerator is a keyboard shortcut that activates the
    object's default action. A keyboard shortcut is the underlined
    character in the text of a menu, menu item or widget, and is
    either the character itself, or a combination of this character
    and a modifier key like Alt, Ctrl or Shift. Command controls like
    tool buttons also have shortcut keys and usually display them in
    their tooltip.

    All objects provide a string for \l Name.

    \sa role(), state()
*/

/*!
    \fn void QAccessibleInterface::setText(Text t, int child, const QString &text)

    Sets the text property \a t of the object, or of the object's
    child if \a child is not 0, to \a text.

    Note that the text properties of most objects are read-only.

    \sa text()
*/

/*!
    \fn QRect QAccessibleInterface::rect(int child) const

    Returns the geometry of the object, or of the object's child if \a child
    is not 0. The geometry is in screen coordinates.

    This function is only reliable for visible objects (invisible
    objects might not be laid out correctly).

    All visual objects provide this information.

    \sa childAt()
*/

/*!
    \fn QAccessible::Role QAccessibleInterface::role(int child) const

    Returns the role of the object, or of the object's child if \a child
    is not 0. The role of an object is usually static.

    All accessible objects have a role.

    \sa text(), state()
*/

/*!
    \fn QAccessible::State QAccessibleInterface::state(int child) const

    Returns the current state of the object, or of the object's child if
    \a child is not 0. The returned value is a combination of the flags in
    the QAccessible::StateFlag enumeration.

    All accessible objects have a state.

    \sa text(), role()
*/

/*!
    \fn int QAccessibleInterface::userActionCount(int child) const

    Returns the number of custom actions of the object, or of the
    object's child if \a child is not 0.

    The \c Action type enumerates predefined actions: these
    are not included in the returned value.

    \sa actionText(), doAction()
*/

/*!
    \fn QString QAccessibleInterface::actionText(int action, Text t, int child) const

    Returns the text property \a t of the action \a action supported by
    the object, or of the object's child if \a child is not 0.

    \sa text(), userActionCount()
*/

/*!
    \fn bool QAccessibleInterface::doAction(int action, int child, const QVariantList &params)

    Asks the object, or the object's \a child if \a child is not 0, to
    execute \a action using the parameters, \a params. Returns true if
    the action could be executed; otherwise returns false.

    \a action can be a predefined or a custom action.

    \sa userActionCount(), actionText()
*/

/*!
    \fn QColor QAccessibleInterface::backgroundColor()
    \internal
*/

/*!
    \fn QAccessibleEditableTextInterface *QAccessibleInterface::editableTextInterface()
    \internal
*/

/*!
    \fn QColor QAccessibleInterface::foregroundColor()
    \internal
*/

/*!
    \fn QAccessibleTextInterface *QAccessibleInterface::textInterface()
    \internal
*/

/*!
    \fn QAccessibleValueInterface *QAccessibleInterface::valueInterface()
    \internal
*/

/*!
    \fn QAccessibleTableInterface *QAccessibleInterface::tableInterface()
    \internal
*/

/*!
    \fn QAccessibleTable2Interface *QAccessibleInterface::table2Interface()
    \internal
*/

/*!
    \fn QAccessibleActionInterface *QAccessibleInterface::actionInterface()
    \internal
*/

/*!
    \fn QAccessibleImageInterface *QAccessibleInterface::imageInterface()
    \internal
*/

/*!
    \class QAccessibleEvent
    \brief The QAccessibleEvent class is used to query addition
    accessibility information about complex widgets.

    The event can be of type QEvent::AccessibilityDescription or
    QEvent::AccessibilityHelp.

    Some QAccessibleInterface implementations send QAccessibleEvents
    to the widget they wrap to obtain the description or help text of
    a widget or of its children. The widget can answer by calling
    setValue() with the requested information.

    The default QWidget::event() implementation simply sets the text
    to be the widget's \l{QWidget::toolTip}{tooltip} (for \l
    AccessibilityDescription event) or its
    \l{QWidget::whatsThis}{"What's This?" text} (for \l
    AccessibilityHelp event).

    \ingroup accessibility
    \ingroup events
*/

/*!
    \fn QAccessibleEvent::QAccessibleEvent(Type type, int child)

    Constructs an accessibility event of the given \a type, which
    must be QEvent::AccessibilityDescription or
    QEvent::AccessibilityHelp.

    \a child is the (1-based) index of the child to which the request
    applies. If \a child is 0, the request is for the widget itself.

    \sa child()
*/

/*!
    \fn int QAccessibleEvent::child() const

    Returns the (1-based) index of the child to which the request
    applies. If the child is 0, the request is for the widget itself.
*/

/*!
    \fn QString QAccessibleEvent::value() const

    Returns the text set using setValue().

    \sa setValue()
*/

/*!
    \fn void QAccessibleEvent::setValue(const QString &text)

    Set the description or help text for the given child() to \a
    text, thereby answering the request.

    \sa value()
*/

/*!
    \since 4.2

    Invokes a \a method on \a child with the given parameters \a params
    and returns the result of the operation as QVariant.

    Note that the type of the returned QVariant depends on the action.

    Returns an invalid QVariant if the object doesn't support the action.
*/
QVariant QAccessibleInterface::invokeMethod(Method method, int child, const QVariantList &params)
{
    if (!(state(0) & HasInvokeExtension))
        return QVariant();

    return static_cast<QAccessibleInterfaceEx *>(this)->invokeMethodEx(method, child, params);
}

QVariant QAccessibleInterfaceEx::virtual_hook(const QVariant &)
{
    return QVariant();
}

/*! \internal */
QAccessible2Interface *QAccessibleInterface::cast_helper(QAccessible2::InterfaceType t)
{
    if (state(0) & HasInvokeExtension)
        return static_cast<QAccessibleInterfaceEx *>(this)->interface_cast(t);
    return 0;
}

QT_END_NAMESPACE

#endif
