/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QBYTEARRAYMATCHER_H
#define QBYTEARRAYMATCHER_H

#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE


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

#endif // QBYTEARRAYMATCHER_H
