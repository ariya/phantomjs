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

#ifndef QPODLIST_P_H
#define QPODLIST_P_H

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

#include <QtCore/qvarlengtharray.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

template <typename T, int Prealloc>
class QPodList : public QVarLengthArray<T, Prealloc>
{
    using QVarLengthArray<T, Prealloc>::s;
    using QVarLengthArray<T, Prealloc>::a;
    using QVarLengthArray<T, Prealloc>::ptr;
    using QVarLengthArray<T, Prealloc>::realloc;
public:
    inline explicit QPodList(int size = 0)
        : QVarLengthArray<T, Prealloc>(size)
    {}

    inline void insert(int idx, const T &t)
    {
        const int sz = s++;
        if (s == a)
            realloc(s, s << 1);
        ::memmove(ptr + idx + 1, ptr + idx, (sz - idx) * sizeof(T));
        ptr[idx] = t;
    }

    inline void removeAll(const T &t)
    {
        int i = 0;
        for (int j = 0; j < s; ++j) {
            if (ptr[j] != t)
                ptr[i++] = ptr[j];
        }
        s = i;
    }

    inline void removeAt(int idx)
    {
        Q_ASSERT(idx >= 0 && idx < s);
        ::memmove(ptr + idx, ptr + idx + 1, (s - idx - 1) * sizeof(T));
        --s;
    }

    inline T takeFirst()
    {
        Q_ASSERT(s > 0);
        T tmp = ptr[0];
        removeAt(0);
        return tmp;
    }
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPODLIST_P_H
