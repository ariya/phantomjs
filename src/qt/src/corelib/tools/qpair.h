/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QPAIR_H
#define QPAIR_H

#include <QtCore/qdatastream.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

template <class T1, class T2>
struct QPair
{
    typedef T1 first_type;
    typedef T2 second_type;

    QPair() : first(T1()), second(T2()) {}
    QPair(const T1 &t1, const T2 &t2) : first(t1), second(t2) {}

    QPair<T1, T2> &operator=(const QPair<T1, T2> &other)
    { first = other.first; second = other.second; return *this; }

    T1 first;
    T2 second;
};

template <class T1, class T2>
Q_INLINE_TEMPLATE bool operator==(const QPair<T1, T2> &p1, const QPair<T1, T2> &p2)
{ return p1.first == p2.first && p1.second == p2.second; }

template <class T1, class T2>
Q_INLINE_TEMPLATE bool operator!=(const QPair<T1, T2> &p1, const QPair<T1, T2> &p2)
{ return !(p1 == p2); }

template <class T1, class T2>
Q_INLINE_TEMPLATE bool operator<(const QPair<T1, T2> &p1, const QPair<T1, T2> &p2)
{
    return p1.first < p2.first || (!(p2.first < p1.first) && p1.second < p2.second);
}

template <class T1, class T2>
Q_INLINE_TEMPLATE bool operator>(const QPair<T1, T2> &p1, const QPair<T1, T2> &p2)
{
    return p2 < p1;
}

template <class T1, class T2>
Q_INLINE_TEMPLATE bool operator<=(const QPair<T1, T2> &p1, const QPair<T1, T2> &p2)
{
    return !(p2 < p1);
}

template <class T1, class T2>
Q_INLINE_TEMPLATE bool operator>=(const QPair<T1, T2> &p1, const QPair<T1, T2> &p2)
{
    return !(p1 < p2);
}

template <class T1, class T2>
Q_OUTOFLINE_TEMPLATE QPair<T1, T2> qMakePair(const T1 &x, const T2 &y)
{
    return QPair<T1, T2>(x, y);
}

#ifndef QT_NO_DATASTREAM
template <class T1, class T2>
inline QDataStream& operator>>(QDataStream& s, QPair<T1, T2>& p)
{
    s >> p.first >> p.second;
    return s;
}

template <class T1, class T2>
inline QDataStream& operator<<(QDataStream& s, const QPair<T1, T2>& p)
{
    s << p.first << p.second;
    return s;
}
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPAIR_H
