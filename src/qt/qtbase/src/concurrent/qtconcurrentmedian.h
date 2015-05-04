/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QTCONCURRENT_MEDIAN_H
#define QTCONCURRENT_MEDIAN_H

#include <QtConcurrent/qtconcurrent_global.h>

#ifndef QT_NO_CONCURRENT

#include <QtCore/qvector.h>

#include <algorithm>

QT_BEGIN_NAMESPACE


#ifndef Q_QDOC

namespace QtConcurrent {

template <typename T>
class Median
{
public:
    Median(int _bufferSize)
        : currentMedian(), bufferSize(_bufferSize), currentIndex(0), valid(false), dirty(true)
    {
        values.resize(bufferSize);
    }

    void reset()
    {
        values.fill(0);
        currentIndex = 0;
        valid = false;
        dirty = true;
    }

    void addValue(T value)
    {
        currentIndex = ((currentIndex + 1) % bufferSize);
        if (valid == false && currentIndex % bufferSize == 0)
            valid = true;

        // Only update the cached median value when we have to, that
        // is when the new value is on then other side of the median
        // compared to the current value at the index.
        const T currentIndexValue = values[currentIndex];
        if ((currentIndexValue > currentMedian && currentMedian > value)
            || (currentMedian > currentIndexValue && value > currentMedian)) {
            dirty = true;
        }

        values[currentIndex] = value;
    }

    bool isMedianValid() const
    {
        return valid;
    }

    T median()
    {
        if (dirty) {
            dirty = false;

// This is a workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=58800
// Avoid using std::nth_element for the affected stdlibc++ releases 4.7.3 and 4.8.2.
// Note that the official __GLIBCXX__ value of the releases is not used since that
// one might be patched on some GNU/Linux distributions.
#if defined(__GLIBCXX__) && __GLIBCXX__ <= 20140107
            QVector<T> sorted = values;
            std::sort(sorted.begin(), sorted.end());
            currentMedian = sorted.at(bufferSize / 2);
#else
            QVector<T> copy = values;
            typename QVector<T>::iterator begin = copy.begin(), mid = copy.begin() + bufferSize/2, end = copy.end();
            std::nth_element(begin, mid, end);
            currentMedian = *mid;
#endif
        }
        return currentMedian;
    }
private:
    QVector<T> values;
    T currentMedian;
    int bufferSize;
    int currentIndex;
    bool valid;
    bool dirty;
};

} // namespace QtConcurrent

#endif //Q_QDOC

QT_END_NAMESPACE

#endif // QT_NO_CONCURRENT

#endif
