/****************************************************************************
 **
 ** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
 ** Contact: http://www.qt-project.org/legal
 **
 ** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QPPSATTRIBUTE_P_H
#define QPPSATTRIBUTE_P_H

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

#include <QList>
#include <QMap>
#include <QSharedDataPointer>
#include <QVariant>

QT_BEGIN_NAMESPACE

class QPpsAttributePrivate;
class QPpsAttribute;

typedef QList<QPpsAttribute> QPpsAttributeList;
typedef QMap<QString, QPpsAttribute> QPpsAttributeMap;
Q_DECLARE_METATYPE(QPpsAttributeList)
Q_DECLARE_METATYPE(QPpsAttributeMap)

class Q_CORE_EXPORT QPpsAttribute
{
public:

    enum Type {
        None   = 0,
        Number = 1,
        Bool   = 2,
        String = 3,
        Array  = 4,
        Object = 5
    };

    enum Flag {
        Incomplete = 0x01,
        Deleted    = 0x02,
        Created    = 0x04,
        Truncated  = 0x08,
        Purged     = 0x10
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    QPpsAttribute();
    QPpsAttribute(const QPpsAttribute &other);
    ~QPpsAttribute();

    QPpsAttribute &operator=(const QPpsAttribute &other);
    bool operator==(const QPpsAttribute &other) const;
    bool operator!=(const QPpsAttribute &other) const;

#ifdef Q_COMPILER_RVALUE_REFS
    QPpsAttribute(QPpsAttribute &&other);
    QPpsAttribute &operator=(QPpsAttribute &&other);
#endif

    bool isValid() const;
    Type type() const;
    QPpsAttribute::Flags flags() const;

    bool isNumber() const;
    bool isBool() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;

    double toDouble() const;
    qlonglong toLongLong() const;
    int toInt() const;
    bool toBool() const;
    QString toString() const;
    QPpsAttributeList toList() const;
    QPpsAttributeMap toMap() const;
    QVariant toVariant() const;

private:
    QSharedDataPointer<QPpsAttributePrivate> d;
    friend class QPpsAttributePrivate;
};

inline bool QPpsAttribute::operator!=(const QPpsAttribute &other) const
{
    return !(*this == other);
}

Q_CORE_EXPORT QDebug operator<<(QDebug dbg, const QPpsAttribute &attribute);

QT_END_NAMESPACE

#endif // QPPSATTRIBUTE_P_H
