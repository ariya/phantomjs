/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QACCESSIBLE_H
#define QACCESSIBLE_H

#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qrect.h>
#include <QtCore/qset.h>
#include <QtCore/qvector.h>
#include <QtCore/qvariant.h>
#include <QtGui/qcolor.h>
#include <QtGui/qevent.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_ACCESSIBILITY

class QAccessibleInterface;

class Q_GUI_EXPORT QAccessible
{
public:
    enum Event {
        SoundPlayed          = 0x0001,
        Alert                = 0x0002,
        ForegroundChanged    = 0x0003,
        MenuStart            = 0x0004,
        MenuEnd              = 0x0005,
        PopupMenuStart       = 0x0006,
        PopupMenuEnd         = 0x0007,
        ContextHelpStart     = 0x000C,
        ContextHelpEnd       = 0x000D,
        DragDropStart        = 0x000E,
        DragDropEnd          = 0x000F,
        DialogStart          = 0x0010,
        DialogEnd            = 0x0011,
        ScrollingStart       = 0x0012,
        ScrollingEnd         = 0x0013,

        MenuCommand          = 0x0018,

        // Values from IAccessible2
        ActionChanged                    = 0x0101,
        ActiveDescendantChanged          = 0x0102,
        AttributeChanged                 = 0x0103,
        DocumentContentChanged           = 0x0104,
        DocumentLoadComplete             = 0x0105,
        DocumentLoadStopped              = 0x0106,
        DocumentReload                   = 0x0107,
        HyperlinkEndIndexChanged         = 0x0108,
        HyperlinkNumberOfAnchorsChanged  = 0x0109,
        HyperlinkSelectedLinkChanged     = 0x010A,
        HypertextLinkActivated           = 0x010B,
        HypertextLinkSelected            = 0x010C,
        HyperlinkStartIndexChanged       = 0x010D,
        HypertextChanged                 = 0x010E,
        HypertextNLinksChanged           = 0x010F,
        ObjectAttributeChanged           = 0x0110,
        PageChanged                      = 0x0111,
        SectionChanged                   = 0x0112,
        TableCaptionChanged              = 0x0113,
        TableColumnDescriptionChanged    = 0x0114,
        TableColumnHeaderChanged         = 0x0115,
        TableModelChanged                = 0x0116,
        TableRowDescriptionChanged       = 0x0117,
        TableRowHeaderChanged            = 0x0118,
        TableSummaryChanged              = 0x0119,
        TextAttributeChanged             = 0x011A,
        TextCaretMoved                   = 0x011B,
        // TextChanged = 0x011C, is deprecated in IA2, use TextUpdated
        TextColumnChanged                = 0x011D,
        TextInserted                     = 0x011E,
        TextRemoved                      = 0x011F,
        TextUpdated                      = 0x0120,
        TextSelectionChanged             = 0x0121,
        VisibleDataChanged               = 0x0122,

        ObjectCreated        = 0x8000,
        ObjectDestroyed      = 0x8001,
        ObjectShow           = 0x8002,
        ObjectHide           = 0x8003,
        ObjectReorder        = 0x8004,
        Focus                = 0x8005,
        Selection            = 0x8006,
        SelectionAdd         = 0x8007,
        SelectionRemove      = 0x8008,
        SelectionWithin      = 0x8009,
        StateChanged         = 0x800A,
        LocationChanged      = 0x800B,
        NameChanged          = 0x800C,
        DescriptionChanged   = 0x800D,
        ValueChanged         = 0x800E,
        ParentChanged        = 0x800F,
        HelpChanged          = 0x80A0,
        DefaultActionChanged = 0x80B0,
        AcceleratorChanged   = 0x80C0
    };

    enum StateFlag {
        Normal          = 0x00000000,
        Unavailable     = 0x00000001,
        Selected        = 0x00000002,
        Focused         = 0x00000004,
        Pressed         = 0x00000008,
        Checked         = 0x00000010,
        Mixed           = 0x00000020,
        ReadOnly        = 0x00000040,
        HotTracked      = 0x00000080,
        DefaultButton   = 0x00000100,
        // #### Qt5 Expandable
        Expanded        = 0x00000200,
        Collapsed       = 0x00000400,
        Busy            = 0x00000800,
        // Floating        = 0x00001000,
        Marqueed        = 0x00002000,
        Animated        = 0x00004000,
        Invisible       = 0x00008000,
        Offscreen       = 0x00010000,
        Sizeable        = 0x00020000,
        Movable         = 0x00040000,
#ifdef QT3_SUPPORT
        Moveable        = Movable,
#endif
        SelfVoicing     = 0x00080000,
        Focusable       = 0x00100000,
        Selectable      = 0x00200000,
        Linked          = 0x00400000,
        Traversed       = 0x00800000,
        MultiSelectable = 0x01000000,
        ExtSelectable   = 0x02000000,
        //AlertLow        = 0x04000000,
        //AlertMedium     = 0x08000000,
        //AlertHigh       = 0x10000000, /* reused for HasInvokeExtension */
        Protected       = 0x20000000,
        HasPopup        = 0x40000000,
        Modal           = 0x80000000,

        // #### Qt5 ManagesDescendants
        // #### Qt5 remove HasInvokeExtension
        HasInvokeExtension = 0x10000000 // internal
    };
    Q_DECLARE_FLAGS(State, StateFlag)

    enum Role {
        NoRole         = 0x00000000,
        TitleBar       = 0x00000001,
        MenuBar        = 0x00000002,
        ScrollBar      = 0x00000003,
        Grip           = 0x00000004,
        Sound          = 0x00000005,
        Cursor         = 0x00000006,
        Caret          = 0x00000007,
        AlertMessage   = 0x00000008,
        Window         = 0x00000009,
        Client         = 0x0000000A,
        PopupMenu      = 0x0000000B,
        MenuItem       = 0x0000000C,
        ToolTip        = 0x0000000D,
        Application    = 0x0000000E,
        Document       = 0x0000000F,
        Pane           = 0x00000010,
        Chart          = 0x00000011,
        Dialog         = 0x00000012,
        Border         = 0x00000013,
        Grouping       = 0x00000014,
        Separator      = 0x00000015,
        ToolBar        = 0x00000016,
        StatusBar      = 0x00000017,
        Table          = 0x00000018,
        ColumnHeader   = 0x00000019,
        RowHeader      = 0x0000001A,
        Column         = 0x0000001B,
        Row            = 0x0000001C,
        Cell           = 0x0000001D,
        Link           = 0x0000001E,
        HelpBalloon    = 0x0000001F,
        Assistant      = 0x00000020,
        List           = 0x00000021,
        ListItem       = 0x00000022,
        Tree           = 0x00000023,
        TreeItem       = 0x00000024,
        PageTab        = 0x00000025,
        PropertyPage   = 0x00000026,
        Indicator      = 0x00000027,
        Graphic        = 0x00000028,
        StaticText     = 0x00000029,
        EditableText   = 0x0000002A,  // Editable, selectable, etc.
        PushButton     = 0x0000002B,
        CheckBox       = 0x0000002C,
        RadioButton    = 0x0000002D,
        ComboBox       = 0x0000002E,
        // DropList       = 0x0000002F,
        ProgressBar    = 0x00000030,
        Dial           = 0x00000031,
        HotkeyField    = 0x00000032,
        Slider         = 0x00000033,
        SpinBox        = 0x00000034,
        Canvas         = 0x00000035,
        Animation      = 0x00000036,
        Equation       = 0x00000037,
        ButtonDropDown = 0x00000038,
        ButtonMenu     = 0x00000039,
        ButtonDropGrid = 0x0000003A,
        Whitespace     = 0x0000003B,
        PageTabList    = 0x0000003C,
        Clock          = 0x0000003D,
        Splitter       = 0x0000003E,
        // Additional Qt roles where enum value does not map directly to MSAA:
        LayeredPane    = 0x0000003F,
        UserRole       = 0x0000ffff
    };

    enum Text {
        Name         = 0,
        Description,
        Value,
        Help,
        Accelerator,
        UserText     = 0x0000ffff
    };

    enum RelationFlag {
        Unrelated     = 0x00000000,
        Self          = 0x00000001,
        Ancestor      = 0x00000002,
        Child         = 0x00000004,
        Descendent    = 0x00000008,
        Sibling       = 0x00000010,
        HierarchyMask = 0x000000ff,

        Up            = 0x00000100,
        Down          = 0x00000200,
        Left          = 0x00000400,
        Right         = 0x00000800,
        Covers        = 0x00001000,
        Covered       = 0x00002000,
        GeometryMask  = 0x0000ff00,

        FocusChild    = 0x00010000,
        Label         = 0x00020000,
        Labelled      = 0x00040000,
        Controller    = 0x00080000,
        Controlled    = 0x00100000,
        LogicalMask   = 0x00ff0000
    };
    Q_DECLARE_FLAGS(Relation, RelationFlag)

    enum Action {
        DefaultAction       = 0,
        Press               = -1,
        FirstStandardAction = Press,
        SetFocus            = -2,
        Increase            = -3,
        Decrease            = -4,
        Accept              = -5,
        Cancel	            = -6,
        Select              = -7,
        ClearSelection      = -8,
        RemoveSelection     = -9,
        ExtendSelection     = -10,
        AddToSelection      = -11,
        LastStandardAction  = AddToSelection
    };

    enum Method {
        ListSupportedMethods      = 0,
        SetCursorPosition         = 1,
        GetCursorPosition         = 2,
        ForegroundColor           = 3,
        BackgroundColor           = 4
    };

    typedef QAccessibleInterface*(*InterfaceFactory)(const QString &key, QObject*);
    typedef void(*UpdateHandler)(QObject*, int who, Event reason);
    typedef void(*RootObjectHandler)(QObject*);

    static void installFactory(InterfaceFactory);
    static void removeFactory(InterfaceFactory);
    static UpdateHandler installUpdateHandler(UpdateHandler);
    static RootObjectHandler installRootObjectHandler(RootObjectHandler);

    static QAccessibleInterface *queryAccessibleInterface(QObject *);
    static void updateAccessibility(QObject *, int who, Event reason);
    static bool isActive();
    static void setRootObject(QObject*);

    static void initialize();
    static void cleanup();

private:
    static UpdateHandler updateHandler;
    static RootObjectHandler rootObjectHandler;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QAccessible::State)
Q_DECLARE_OPERATORS_FOR_FLAGS(QAccessible::Relation)
QT_END_NAMESPACE
Q_DECLARE_METATYPE(QSet<QAccessible::Method>)
QT_BEGIN_NAMESPACE

namespace QAccessible2
{
    enum InterfaceType
    {
        TextInterface,
        EditableTextInterface,
        ValueInterface,
        TableInterface,
        ActionInterface,
        ImageInterface,
        Table2Interface
    };
}

class QAccessible2Interface;
class QAccessibleTextInterface;
class QAccessibleEditableTextInterface;
class QAccessibleValueInterface;
class QAccessibleTableInterface;
class QAccessibleActionInterface;
class QAccessibleImageInterface;
class QAccessibleTable2Interface;

class Q_GUI_EXPORT QAccessibleInterface : public QAccessible
{
public:
    virtual ~QAccessibleInterface() {}
    // check for valid pointers
    virtual bool isValid() const = 0;
    virtual QObject *object() const = 0;

    // hierarchy
    virtual int childCount() const = 0;
    virtual int indexOfChild(const QAccessibleInterface *) const = 0;

    // relations
    virtual Relation relationTo(int child, const QAccessibleInterface *other,
                                int otherChild) const = 0;
    virtual int childAt(int x, int y) const = 0;

    // navigation
    virtual int navigate(RelationFlag relation, int index, QAccessibleInterface **iface) const = 0;

    // properties and state
    virtual QString text(Text t, int child) const = 0;
    virtual void setText(Text t, int child, const QString &text) = 0;
    virtual QRect rect(int child) const = 0;
    virtual Role role(int child) const = 0;
    virtual State state(int child) const = 0;

    // action
    virtual int userActionCount(int child) const = 0;
    virtual QString actionText(int action, Text t, int child) const = 0;
    virtual bool doAction(int action, int child, const QVariantList &params = QVariantList()) = 0;

    QVariant invokeMethod(Method method, int child = 0,
                          const QVariantList &params = QVariantList());

    inline QSet<Method> supportedMethods()
    { return qvariant_cast<QSet<Method> >(invokeMethod(ListSupportedMethods)); }

    inline QColor foregroundColor()
    { return qvariant_cast<QColor>(invokeMethod(ForegroundColor)); }

    inline QColor backgroundColor()
    { return qvariant_cast<QColor>(invokeMethod(BackgroundColor)); }

    inline QAccessibleTextInterface *textInterface()
    { return reinterpret_cast<QAccessibleTextInterface *>(cast_helper(QAccessible2::TextInterface)); }

    inline QAccessibleEditableTextInterface *editableTextInterface()
    { return reinterpret_cast<QAccessibleEditableTextInterface *>(cast_helper(QAccessible2::EditableTextInterface)); }

    inline QAccessibleValueInterface *valueInterface()
    { return reinterpret_cast<QAccessibleValueInterface *>(cast_helper(QAccessible2::ValueInterface)); }

    inline QAccessibleTableInterface *tableInterface()
    { return reinterpret_cast<QAccessibleTableInterface *>(cast_helper(QAccessible2::TableInterface)); }

    inline QAccessibleActionInterface *actionInterface()
    { return reinterpret_cast<QAccessibleActionInterface *>(cast_helper(QAccessible2::ActionInterface)); }

    inline QAccessibleImageInterface *imageInterface()
    { return reinterpret_cast<QAccessibleImageInterface *>(cast_helper(QAccessible2::ImageInterface)); }

    inline QAccessibleTable2Interface *table2Interface()
    { return reinterpret_cast<QAccessibleTable2Interface *>(cast_helper(QAccessible2::Table2Interface)); }

private:
    QAccessible2Interface *cast_helper(QAccessible2::InterfaceType);
};

class Q_GUI_EXPORT QAccessibleInterfaceEx: public QAccessibleInterface
{
public:
    virtual QVariant invokeMethodEx(Method method, int child, const QVariantList &params) = 0;
    virtual QVariant virtual_hook(const QVariant &data);
    virtual QAccessible2Interface *interface_cast(QAccessible2::InterfaceType)
    { return 0; }
};


class Q_GUI_EXPORT QAccessibleEvent : public QEvent
{
public:
    inline QAccessibleEvent(Type type, int child);
    inline int child() const { return c; }
    inline QString value() const { return val; }
    inline void setValue(const QString &aText) { val = aText; }

private:
    int c;
    QString val;
};

inline QAccessibleEvent::QAccessibleEvent(Type atype, int achild)
    : QEvent(atype), c(achild) {}

#define QAccessibleInterface_iid "com.trolltech.Qt.QAccessibleInterface"
Q_DECLARE_INTERFACE(QAccessibleInterface, QAccessibleInterface_iid)

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE

QT_END_HEADER

#endif // QACCESSIBLE_H
