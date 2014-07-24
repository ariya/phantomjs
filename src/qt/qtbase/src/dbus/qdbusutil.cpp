/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtDBus module of the Qt Toolkit.
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

#include "qdbusutil_p.h"

#include "qdbus_symbols_p.h"

#include <QtCore/qstringlist.h>

#include "qdbusargument.h"
#include "qdbusunixfiledescriptor.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

static inline bool isValidCharacterNoDash(QChar c)
{
    ushort u = c.unicode();
    return (u >= 'a' && u <= 'z')
            || (u >= 'A' && u <= 'Z')
            || (u >= '0' && u <= '9')
            || (u == '_');
}

static inline bool isValidCharacter(QChar c)
{
    ushort u = c.unicode();
    return (u >= 'a' && u <= 'z')
            || (u >= 'A' && u <= 'Z')
            || (u >= '0' && u <= '9')
            || (u == '_') || (u == '-');
}

static inline bool isValidNumber(QChar c)
{
    ushort u = c.unicode();
    return (u >= '0' && u <= '9');
}

#ifndef QT_BOOTSTRAPPED
static bool argToString(const QDBusArgument &arg, QString &out);

static bool variantToString(const QVariant &arg, QString &out)
{
    int argType = arg.userType();

    if (argType == QVariant::StringList) {
        out += QLatin1Char('{');
        QStringList list = arg.toStringList();
        foreach (const QString &item, list)
            out += QLatin1Char('\"') + item + QLatin1String("\", ");
        if (!list.isEmpty())
            out.chop(2);
        out += QLatin1Char('}');
    } else if (argType == QVariant::ByteArray) {
        out += QLatin1Char('{');
        QByteArray list = arg.toByteArray();
        for (int i = 0; i < list.count(); ++i) {
            out += QString::number(list.at(i));
            out += QLatin1String(", ");
        }
        if (!list.isEmpty())
            out.chop(2);
        out += QLatin1Char('}');
    } else if (argType == QVariant::List) {
        out += QLatin1Char('{');
        QList<QVariant> list = arg.toList();
        foreach (const QVariant &item, list) {
            if (!variantToString(item, out))
                return false;
            out += QLatin1String(", ");
        }
        if (!list.isEmpty())
            out.chop(2);
        out += QLatin1Char('}');
    } else if (argType == QMetaType::Char || argType == QMetaType::Short || argType == QMetaType::Int
               || argType == QMetaType::Long || argType == QMetaType::LongLong) {
        out += QString::number(arg.toLongLong());
    } else if (argType == QMetaType::UChar || argType == QMetaType::UShort || argType == QMetaType::UInt
               || argType == QMetaType::ULong || argType == QMetaType::ULongLong) {
        out += QString::number(arg.toULongLong());
    } else if (argType == QMetaType::Double) {
        out += QString::number(arg.toDouble());
    } else if (argType == QMetaType::Bool) {
        out += QLatin1String(arg.toBool() ? "true" : "false");
    } else if (argType == qMetaTypeId<QDBusArgument>()) {
        argToString(qvariant_cast<QDBusArgument>(arg), out);
    } else if (argType == qMetaTypeId<QDBusObjectPath>()) {
        const QString path = qvariant_cast<QDBusObjectPath>(arg).path();
        out += QLatin1String("[ObjectPath: ");
        out += path;
        out += QLatin1Char(']');
    } else if (argType == qMetaTypeId<QDBusSignature>()) {
        out += QLatin1String("[Signature: ") + qvariant_cast<QDBusSignature>(arg).signature();
        out += QLatin1Char(']');
    } else if (argType == qMetaTypeId<QDBusUnixFileDescriptor>()) {
        out += QLatin1String("[Unix FD: ");
        out += QLatin1String(qvariant_cast<QDBusUnixFileDescriptor>(arg).isValid() ? "valid" : "not valid");
        out += QLatin1Char(']');
    } else if (argType == qMetaTypeId<QDBusVariant>()) {
        const QVariant v = qvariant_cast<QDBusVariant>(arg).variant();
        out += QLatin1String("[Variant");
        int vUserType = v.userType();
        if (vUserType != qMetaTypeId<QDBusVariant>()
                && vUserType != qMetaTypeId<QDBusSignature>()
                && vUserType != qMetaTypeId<QDBusObjectPath>()
                && vUserType != qMetaTypeId<QDBusArgument>())
            out += QLatin1Char('(') + QLatin1String(v.typeName()) + QLatin1Char(')');
        out += QLatin1String(": ");
        if (!variantToString(v, out))
            return false;
        out += QLatin1Char(']');
    } else if (arg.canConvert(QVariant::String)) {
        out += QLatin1Char('\"') + arg.toString() + QLatin1Char('\"');
    } else {
        out += QLatin1Char('[');
        out += QLatin1String(arg.typeName());
        out += QLatin1Char(']');
    }

    return true;
}

bool argToString(const QDBusArgument &busArg, QString &out)
{
    QString busSig = busArg.currentSignature();
    bool doIterate = false;
    QDBusArgument::ElementType elementType = busArg.currentType();

    if (elementType != QDBusArgument::BasicType && elementType != QDBusArgument::VariantType
            && elementType != QDBusArgument::MapEntryType)
        out += QLatin1String("[Argument: ") + busSig + QLatin1Char(' ');

    switch (elementType) {
        case QDBusArgument::BasicType:
        case QDBusArgument::VariantType:
            if (!variantToString(busArg.asVariant(), out))
                return false;
            break;
        case QDBusArgument::StructureType:
            busArg.beginStructure();
            doIterate = true;
            break;
        case QDBusArgument::ArrayType:
            busArg.beginArray();
            out += QLatin1Char('{');
            doIterate = true;
            break;
        case QDBusArgument::MapType:
            busArg.beginMap();
            out += QLatin1Char('{');
            doIterate = true;
            break;
        case QDBusArgument::MapEntryType:
            busArg.beginMapEntry();
            if (!variantToString(busArg.asVariant(), out))
                return false;
            out += QLatin1String(" = ");
            if (!argToString(busArg, out))
                return false;
            busArg.endMapEntry();
            break;
        case QDBusArgument::UnknownType:
        default:
            out += QLatin1String("<ERROR - Unknown Type>");
            return false;
    }
    if (doIterate && !busArg.atEnd()) {
        while (!busArg.atEnd()) {
            if (!argToString(busArg, out))
                return false;
            out += QLatin1String(", ");
        }
        out.chop(2);
    }
    switch (elementType) {
        case QDBusArgument::BasicType:
        case QDBusArgument::VariantType:
        case QDBusArgument::UnknownType:
        case QDBusArgument::MapEntryType:
            // nothing to do
            break;
        case QDBusArgument::StructureType:
            busArg.endStructure();
            break;
        case QDBusArgument::ArrayType:
            out += QLatin1Char('}');
            busArg.endArray();
            break;
        case QDBusArgument::MapType:
            out += QLatin1Char('}');
            busArg.endMap();
            break;
    }

    if (elementType != QDBusArgument::BasicType && elementType != QDBusArgument::VariantType
            && elementType != QDBusArgument::MapEntryType)
        out += QLatin1Char(']');

    return true;
}
#endif

//------- D-Bus Types --------
static const char oneLetterTypes[] = "vsogybnqiuxtdh";
static const char basicTypes[] =      "sogybnqiuxtdh";
static const char fixedTypes[] =         "ybnqiuxtdh";

static bool isBasicType(int c)
{
    return c != DBUS_TYPE_INVALID && strchr(basicTypes, c) != NULL;
}

static bool isFixedType(int c)
{
    return c != DBUS_TYPE_INVALID && strchr(fixedTypes, c) != NULL;
}

// Returns a pointer to one-past-end of this type if it's valid;
// returns NULL if it isn't valid.
static const char *validateSingleType(const char *signature)
{
    char c = *signature;
    if (c == DBUS_TYPE_INVALID)
        return 0;

    // is it one of the one-letter types?
    if (strchr(oneLetterTypes, c) != NULL)
        return signature + 1;

    // is it an array?
    if (c == DBUS_TYPE_ARRAY) {
        // then it's valid if the next type is valid
        // or if it's a dict-entry
        c = *++signature;
        if (c == DBUS_DICT_ENTRY_BEGIN_CHAR) {
            // beginning of a dictionary entry
            // a dictionary entry has a key which is of basic types
            // and a free value
            c = *++signature;
            if (!isBasicType(c))
                return 0;
            signature = validateSingleType(signature + 1);
            return signature && *signature == DBUS_DICT_ENTRY_END_CHAR ? signature + 1 : 0;
        }

        return validateSingleType(signature);
    }

    if (c == DBUS_STRUCT_BEGIN_CHAR) {
        // beginning of a struct
        ++signature;
        while (true) {
            signature = validateSingleType(signature);
            if (!signature)
                return 0;
            if (*signature == DBUS_STRUCT_END_CHAR)
                return signature + 1;
        }
    }

    // invalid/unknown type
    return 0;
}

/*!
    \namespace QDBusUtil
    \inmodule QtDBus
    \internal

    \brief The QDBusUtil namespace contains a few functions that are of general use when
    dealing with D-Bus strings.
*/
namespace QDBusUtil
{
    /*!
        \internal
        \since 4.5
        Dumps the contents of a Qt D-Bus argument from \a arg into a string.
    */
    QString argumentToString(const QVariant &arg)
    {
        QString out;

#ifndef QT_BOOTSTRAPPED
        variantToString(arg, out);
#else
        Q_UNUSED(arg);
#endif

        return out;
    }

    /*!
        \internal
        \fn bool QDBusUtil::isValidPartOfObjectPath(const QString &part)
        See QDBusUtil::isValidObjectPath
    */
    bool isValidPartOfObjectPath(const QString &part)
    {
        if (part.isEmpty())
            return false;       // can't be valid if it's empty

        const QChar *c = part.unicode();
        for (int i = 0; i < part.length(); ++i)
            if (!isValidCharacterNoDash(c[i]))
                return false;

        return true;
    }

    /*!
        \fn bool QDBusUtil::isValidInterfaceName(const QString &ifaceName)
        Returns \c true if this is \a ifaceName is a valid interface name.

        Valid interface names must:
        \list
          \li not be empty
          \li not exceed 255 characters in length
          \li be composed of dot-separated string components that contain only ASCII letters, digits
             and the underscore ("_") character
          \li contain at least two such components
        \endlist
    */
    bool isValidInterfaceName(const QString& ifaceName)
    {
        if (ifaceName.isEmpty() || ifaceName.length() > DBUS_MAXIMUM_NAME_LENGTH)
            return false;

        QStringList parts = ifaceName.split(QLatin1Char('.'));
        if (parts.count() < 2)
            return false;           // at least two parts

        for (int i = 0; i < parts.count(); ++i)
            if (!isValidMemberName(parts.at(i)))
                return false;

        return true;
    }

    /*!
        \fn bool QDBusUtil::isValidUniqueConnectionName(const QString &connName)
        Returns \c true if \a connName is a valid unique connection name.

        Unique connection names start with a colon (":") and are followed by a list of dot-separated
        components composed of ASCII letters, digits, the hyphen or the underscore ("_") character.
    */
    bool isValidUniqueConnectionName(const QString &connName)
    {
        if (connName.isEmpty() || connName.length() > DBUS_MAXIMUM_NAME_LENGTH ||
            !connName.startsWith(QLatin1Char(':')))
            return false;

        QStringList parts = connName.mid(1).split(QLatin1Char('.'));
        if (parts.count() < 1)
            return false;

        for (int i = 0; i < parts.count(); ++i) {
            const QString &part = parts.at(i);
            if (part.isEmpty())
                 return false;

            const QChar* c = part.unicode();
            for (int j = 0; j < part.length(); ++j)
                if (!isValidCharacter(c[j]))
                    return false;
        }

        return true;
    }

    /*!
        \fn bool QDBusUtil::isValidBusName(const QString &busName)
        Returns \c true if \a busName is a valid bus name.

        A valid bus name is either a valid unique connection name or follows the rules:
        \list
          \li is not empty
          \li does not exceed 255 characters in length
          \li be composed of dot-separated string components that contain only ASCII letters, digits,
             hyphens or underscores ("_"), but don't start with a digit
          \li contains at least two such elements
        \endlist

        \sa isValidUniqueConnectionName()
    */
    bool isValidBusName(const QString &busName)
    {
        if (busName.isEmpty() || busName.length() > DBUS_MAXIMUM_NAME_LENGTH)
            return false;

        if (busName.startsWith(QLatin1Char(':')))
            return isValidUniqueConnectionName(busName);

        QStringList parts = busName.split(QLatin1Char('.'));
        if (parts.count() < 1)
            return false;

        for (int i = 0; i < parts.count(); ++i) {
            const QString &part = parts.at(i);
            if (part.isEmpty())
                return false;

            const QChar *c = part.unicode();
            if (isValidNumber(c[0]))
                return false;
            for (int j = 0; j < part.length(); ++j)
                if (!isValidCharacter(c[j]))
                    return false;
        }

        return true;
    }

    /*!
        \fn bool QDBusUtil::isValidMemberName(const QString &memberName)
        Returns \c true if \a memberName is a valid member name. A valid member name does not exceed
        255 characters in length, is not empty, is composed only of ASCII letters, digits and
        underscores, but does not start with a digit.
    */
    bool isValidMemberName(const QString &memberName)
    {
        if (memberName.isEmpty() || memberName.length() > DBUS_MAXIMUM_NAME_LENGTH)
            return false;

        const QChar* c = memberName.unicode();
        if (isValidNumber(c[0]))
            return false;
        for (int j = 0; j < memberName.length(); ++j)
            if (!isValidCharacterNoDash(c[j]))
                return false;
        return true;
    }

    /*!
        \fn bool QDBusUtil::isValidErrorName(const QString &errorName)
        Returns \c true if \a errorName is a valid error name. Valid error names are valid interface
        names and vice-versa, so this function is actually an alias for isValidInterfaceName.
    */
    bool isValidErrorName(const QString &errorName)
    {
        return isValidInterfaceName(errorName);
    }

    /*!
        \fn bool QDBusUtil::isValidObjectPath(const QString &path)
        Returns \c true if \a path is valid object path.

        Valid object paths follow the rules:
        \list
          \li start with the slash character ("/")
          \li do not end in a slash, unless the path is just the initial slash
          \li do not contain any two slashes in sequence
          \li contain slash-separated parts, each of which is composed of ASCII letters, digits and
             underscores ("_")
        \endlist
    */
    bool isValidObjectPath(const QString &path)
    {
        if (path == QLatin1String("/"))
            return true;

        if (!path.startsWith(QLatin1Char('/')) || path.indexOf(QLatin1String("//")) != -1 ||
            path.endsWith(QLatin1Char('/')))
            return false;

        QStringList parts = path.split(QLatin1Char('/'));
        Q_ASSERT(parts.count() >= 1);
        parts.removeFirst();    // it starts with /, so we get an empty first part

        for (int i = 0; i < parts.count(); ++i)
            if (!isValidPartOfObjectPath(parts.at(i)))
                return false;

        return true;
    }

    /*!
        \fn bool QDBusUtil::isValidBasicType(int type)
        Returns \c true if \a c is a valid, basic D-Bus type.
     */
    bool isValidBasicType(int c)
    {
        return isBasicType(c);
    }

    /*!
        \fn bool QDBusUtil::isValidFixedType(int type)
        Returns \c true if \a c is a valid, fixed D-Bus type.
     */
    bool isValidFixedType(int c)
    {
        return isFixedType(c);
    }


    /*!
        \fn bool QDBusUtil::isValidSignature(const QString &signature)
        Returns \c true if \a signature is a valid D-Bus type signature for one or more types.
        This function returns \c true if it can all of \a signature into valid, individual types and no
        characters remain in \a signature.

        \sa isValidSingleSignature()
    */
    bool isValidSignature(const QString &signature)
    {
        QByteArray ba = signature.toLatin1();
        const char *data = ba.constData();
        while (true) {
            data = validateSingleType(data);
            if (!data)
                return false;
            if (*data == '\0')
                return true;
        }
    }

    /*!
        \fn bool QDBusUtil::isValidSingleSignature(const QString &signature)
        Returns \c true if \a signature is a valid D-Bus type signature for exactly one full type. This
        function tries to convert the type signature into a D-Bus type and, if it succeeds and no
        characters remain in the signature, it returns \c true.
    */
    bool isValidSingleSignature(const QString &signature)
    {
        QByteArray ba = signature.toLatin1();
        const char *data = validateSingleType(ba.constData());
        return data && *data == '\0';
    }

} // namespace QDBusUtil

QT_END_NAMESPACE

#endif // QT_NO_DBUS
