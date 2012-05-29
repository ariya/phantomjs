/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QBYTEARRAYMATCHER_H
#define QBYTEARRAYMATCHER_H

#include <QtCore/qbytearray.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class QByteArrayMatcherPrivate;

class Q_CORE_EXPORT QByteArrayMatcher
{
public:
    QByteArrayMatcher();
    explicit QByteArrayMatcher(const QByteArray &pattern);
    explicit QByteArrayMatcher(const char *pattern, int length);
    QByteArrayMatcher(const QByteArrayMatcher &other);
    ~QByteArrayMatcher();

    QByteArrayMatcher &operator=(const QByteArrayMatcher &other);

    void setPattern(const QByteArray &pattern);

    int indexIn(const QByteArray &ba, int from = 0) const;
    int indexIn(const char *str, int len, int from = 0) const;
    inline QByteArray pattern() const
    {
        if (q_pattern.isNull())
            return QByteArray(reinterpret_cast<const char*>(p.p), p.l);
        return q_pattern;
    }

private:
    QByteArrayMatcherPrivate *d;
    QByteArray q_pattern;
#ifdef Q_CC_RVCT
// explicitly allow anonymous unions for RVCT to prevent compiler warnings
#  pragma push
#  pragma anon_unions
#endif
    struct Data {
        uchar q_skiptable[256];
        const uchar *p;
        int l;
    };
    union {
        uint dummy[256];
        Data p;
    };
#ifdef Q_CC_RVCT
#  pragma pop
#endif
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QBYTEARRAYMATCHER_H
