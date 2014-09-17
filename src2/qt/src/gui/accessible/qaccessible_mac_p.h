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

#ifndef QACCESSIBLE_MAC_P_H
#define QACCESSIBLE_MAC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//
//

#include <qglobal.h>
#include <private/qt_mac_p.h>
#include <qaccessible.h>
#include <qwidget.h>
#include <qdebug.h>

//#define Q_ACCESSIBLE_MAC_DEBUG

QT_BEGIN_NAMESPACE

/*
    QAccessibleInterfaceWrapper wraps QAccessibleInterface and adds
    a ref count. QAccessibleInterfaceWrapper is a "by-value" class.
*/
class Q_AUTOTEST_EXPORT QAccessibleInterfaceWrapper
{
public:
    QAccessibleInterfaceWrapper()
    : interface(0), childrenIsRegistered(new bool(false)), refCount(new int(1)) { }

    QAccessibleInterfaceWrapper(QAccessibleInterface *interface)
    :interface(interface), childrenIsRegistered(new bool(false)), refCount(new int(1))  { }

    ~QAccessibleInterfaceWrapper()
    {
        if (--(*refCount) == 0) {
            delete interface;
            delete refCount;
            delete childrenIsRegistered;
        }
    }

    QAccessibleInterfaceWrapper(const QAccessibleInterfaceWrapper &other)
    :interface(other.interface), childrenIsRegistered(other.childrenIsRegistered), refCount(other.refCount)
    {
        ++(*refCount);
    }

    void operator=(const QAccessibleInterfaceWrapper &other)
    {
        if (other.interface == interface)
            return;

        if (--(*refCount) == 0) {
            delete interface;
            delete refCount;
            delete childrenIsRegistered;
        }

        interface = other.interface;
        childrenIsRegistered = other.childrenIsRegistered;
        refCount = other.refCount;
        ++(*refCount);
    }

    QAccessibleInterface *interface;
    bool *childrenIsRegistered;
private:
    int *refCount;
};

/*
    QAInterface represents one accessiblity item. It hides the fact that
    one QAccessibleInterface may represent more than one item, and it also
    automates the memory management for QAccessibleInterfaces using the
    QAccessibleInterfaceWrapper wrapper class.

    It has the same API as QAccessibleInterface, minus the child parameter
    in the functions.
*/
class Q_AUTOTEST_EXPORT QAInterface : public QAccessible
{
public:
    QAInterface()
    : base(QAccessibleInterfaceWrapper())
    { }

    QAInterface(QAccessibleInterface *interface, int child = 0)
    {
        if (interface == 0 || child > interface->childCount()) {
           base = QAccessibleInterfaceWrapper(); 
        } else {
            base = QAccessibleInterfaceWrapper(interface);
            m_cachedObject = interface->object();
            this->child = child;
        }
    }

    QAInterface(QAccessibleInterfaceWrapper wrapper, int child = 0)
    :base(wrapper), m_cachedObject(wrapper.interface->object()), child(child)
    { }

    QAInterface(const QAInterface &other, int child)
    {
        if (other.isValid() == false || child > other.childCount()) {
           base = QAccessibleInterfaceWrapper();
        } else {
            base = other.base;
            m_cachedObject = other.m_cachedObject;
            this->child = child;
        }
    }

    bool operator==(const QAInterface &other) const;
    bool operator!=(const QAInterface &other) const;

    inline QString actionText (int action, Text text) const
    { return base.interface->actionText(action, text, child); }

    QAInterface childAt(int x, int y) const
    {
        if (!checkValid())
            return QAInterface();

        const int foundChild = base.interface->childAt(x, y);

        if (foundChild == -1)
            return QAInterface();

        if (child == 0)
            return navigate(QAccessible::Child, foundChild);

        if (foundChild == child)
            return *this;
        return QAInterface();
    }

    int indexOfChild(const QAInterface &child) const
    {
        if (!checkValid())
            return -1;

        if (*this != child.parent())
            return -1;

        if (object() == child.object())
            return child.id();

        return base.interface->indexOfChild(child.base.interface);
    }

    inline int childCount() const
    {
        if (!checkValid())
            return 0;

        if (child != 0)
            return 0;
        return base.interface->childCount();
    }

    QList<QAInterface> children() const
    {
        if (!checkValid())
            return QList<QAInterface>();

        QList<QAInterface> children;
        for (int i = 1; i <= childCount(); ++i) {
            children.append(navigate(QAccessible::Child, i));
        }
        return children;
    }

    QAInterface childAt(int index) const
    {
        return navigate(QAccessible::Child, index);
    }

    inline void doAction(int action, const QVariantList &params = QVariantList()) const
    {
        if (!checkValid())
            return;

        base.interface->doAction(action, child, params);
    }

    QAInterface navigate(RelationFlag relation, int entry) const;

    inline QObject * object() const
    {
        if (!checkValid())
            return 0;

        return base.interface->object();
    }

    QAInterface objectInterface() const
    {
        if (!checkValid())
            return QAInterface();

        QObject *obj = object();
        QAInterface current = *this;
        while (obj == 0)
        {
            QAInterface parent = current.parent();
            if (parent.isValid() == false)
                break;
            obj = parent.object();
            current = parent;
        }
        return current;
    }

    inline HIObjectRef hiObject() const
    {
        if (!checkValid())
            return 0;
        QWidget * const widget = qobject_cast<QWidget * const>(object());
        if (widget)
            return (HIObjectRef)widget->winId();
        else
            return 0;
    }

    inline QObject * cachedObject() const
    {
        if (!checkValid())
            return 0;
        return m_cachedObject;
    }

    inline QRect rect() const
    {
        if (!checkValid())
            return QRect();
        return base.interface->rect(child);
    }

    inline Role role() const
    {
        if (!checkValid())
            return QAccessible::NoRole;
        return base.interface->role(child);
    }

    inline void setText(Text t, const QString &text) const
    {
        if (!checkValid())
            return;
        base.interface->setText(t, child, text);
    }

    inline State state() const
    {
        if (!checkValid())
            return 0;
        return base.interface->state(child);
    }

    inline QString text (Text text) const
    {
        if (!checkValid())
            return QString();
        return base.interface->text(text, child);
    }

    inline QString value() const
    { return text(QAccessible::Value); }

    inline QString name() const
    { return text(QAccessible::Name); }

    inline int userActionCount() const
    {
        if (!checkValid())
            return 0;
        return base.interface->userActionCount(child);
    }

    inline QString className() const
    {
        if (!checkValid())
            return QString();
        return QLatin1String(base.interface->object()->metaObject()->className());
    }

    inline bool isHIView() const
    { return (child == 0 && object() != 0); }

    inline int id() const
    { return child; }

    inline bool isValid() const
    {
        return (base.interface != 0 && base.interface->isValid());
    }

    QAInterface parent() const
    { return navigate(QAccessible::Ancestor, 1); }

    QAccessibleInterfaceWrapper interfaceWrapper() const
    { return base; }

protected:
    bool checkValid() const
    {
        const bool valid = isValid();
#ifdef Q_ACCESSIBLE_MAC_DEBUG
        if (!valid)
            qFatal("QAccessible_mac: tried to use invalid interface.");
#endif
        return valid;
    }

    QAccessibleInterfaceWrapper base;
    QObject *m_cachedObject;
    int child;
};

Q_AUTOTEST_EXPORT QDebug operator<<(QDebug debug, const QAInterface &interface);

/*
    QAElement is a thin wrapper around an AXUIElementRef that automates
    the ref-counting.
*/
class Q_AUTOTEST_EXPORT QAElement
{
public:
    QAElement();
    explicit QAElement(AXUIElementRef elementRef);
    QAElement(const QAElement &element);
    QAElement(HIObjectRef, int child);
    ~QAElement();

    inline HIObjectRef object() const
    {
#ifndef Q_WS_MAC64
        return AXUIElementGetHIObject(elementRef);
#else
        return 0;
#endif
    }

    inline int id() const
    {
        UInt64 theId;
#ifndef QT_MAC_USE_COCOA
        AXUIElementGetIdentifier(elementRef, &theId);
#else
        theId = 0;
#endif
        return theId;
    }

    inline AXUIElementRef element() const
    {
        return elementRef;
    }

    inline bool isValid() const
    {
        return (elementRef != 0);
    }

    void operator=(const QAElement &other);
    bool operator==(const QAElement &other) const;
private:
    AXUIElementRef elementRef;
};


class QInterfaceFactory
{
public:
    virtual QAInterface interface(UInt64 identifier) = 0;
    virtual QAElement element(int id) = 0;
    virtual QAElement element(const QAInterface &interface)
    {
        return element(interface.id());
    }
    virtual void registerChildren() = 0;
    virtual ~QInterfaceFactory() {}
};

/*
    QAccessibleHierarchyManager bridges the Mac and Qt accessibility hierarchies.
    There is a one-to-one relationship between QAElements on the Mac side
    and QAInterfaces on the Qt side, and this class provides lookup functions
    that translates between these to items.

    The identity of a QAInterface is determined by its QAccessibleInterface and
    child identifier, and the identity of a QAElement is determined by its
    HIObjectRef and identifier.

    QAccessibleHierarchyManager receives QObject::destroyed() signals and deletes
    the accessibility objects for destroyed objects.
*/
class Q_AUTOTEST_EXPORT QAccessibleHierarchyManager : public QObject
{
Q_OBJECT
public:
    ~QAccessibleHierarchyManager() { reset(); }
    static QAccessibleHierarchyManager *instance();
    void reset();

    QAElement registerInterface(QObject *object, int child);
    QAElement registerInterface(const QAInterface &interface);
    void registerInterface(QObject *object, HIObjectRef hiobject, QInterfaceFactory *interfaceFactory);

    void registerChildren(const QAInterface &interface);

    QAInterface lookup(const AXUIElementRef &element);
    QAInterface lookup(const QAElement &element);
    QAElement lookup(const QAInterface &interface);
    QAElement lookup(QObject * const object, int id);
private slots:
    void objectDestroyed(QObject *);
private:
    typedef QHash<QObject *, QInterfaceFactory *> QObjectElementHash;
    typedef QHash<HIObjectRef, QInterfaceFactory *> HIObjectInterfaceHash;
    typedef QHash<QObject *, HIObjectRef> QObjectHIObjectHash;

    QObjectElementHash qobjectElementHash;
    HIObjectInterfaceHash hiobjectInterfaceHash;
    QObjectHIObjectHash qobjectHiobjectHash;
};

Q_AUTOTEST_EXPORT bool isItInteresting(const QAInterface &interface);

QT_END_NAMESPACE

#endif
