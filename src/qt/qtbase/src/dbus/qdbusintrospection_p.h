/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtDBus module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDBUSINTROSPECTION_P_H
#define QDBUSINTROSPECTION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qstring.h>
#include <QtCore/qlist.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qpair.h>
#include <QtCore/qshareddata.h>
#include "qdbusmacros.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class Q_DBUS_EXPORT QDBusIntrospection
{
public:
    // forward declarations
    struct Argument;
    struct Method;
    struct Signal;
    struct Property;
    struct Interface;
    struct Object;
    struct ObjectTree;

    // typedefs
    typedef QMap<QString, QString> Annotations;
    typedef QList<Argument> Arguments;
    typedef QMultiMap<QString, Method> Methods;
    typedef QMultiMap<QString, Signal> Signals;
    typedef QMap<QString, Property> Properties;
    typedef QMap<QString, QSharedDataPointer<Interface> > Interfaces;
    typedef QMap<QString, QSharedDataPointer<ObjectTree> > Objects;

public:
    // the structs

    struct Argument
    {
        QString type;
        QString name;

        inline bool operator==(const Argument& other) const
        { return name == other.name && type == other.type; }
    };

    struct Method
    {
        QString name;
        Arguments inputArgs;
        Arguments outputArgs;
        Annotations annotations;

        inline bool operator==(const Method& other) const
        { return name == other.name && annotations == other.annotations &&
                inputArgs == other.inputArgs && outputArgs == other.outputArgs; }
    };

    struct Signal
    {
        QString name;
        Arguments outputArgs;
        Annotations annotations;

        inline bool operator==(const Signal& other) const
        { return name == other.name && annotations == other.annotations &&
                outputArgs == other.outputArgs; }
    };

    struct Property
    {
        enum Access { Read, Write, ReadWrite };
        QString name;
        QString type;
        Access access;
        Annotations annotations;

        inline bool operator==(const Property& other) const
        { return access == other.access && name == other.name &&
                annotations == other.annotations && type == other.type; }
    };

    struct Interface: public QSharedData
    {
        QString name;
        QString introspection;

        Annotations annotations;
        Methods methods;
        Signals signals_;
        Properties properties;

        inline bool operator==(const Interface &other) const
        { return !name.isEmpty() && name == other.name; }
    };

    struct Object: public QSharedData
    {
        QString service;
        QString path;

        QStringList interfaces;
        QStringList childObjects;
    };

public:
    static Interface parseInterface(const QString &xml);
    static Interfaces parseInterfaces(const QString &xml);
    static Object parseObject(const QString &xml, const QString &service = QString(),
                              const QString &path = QString());

private:
    QDBusIntrospection();
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
