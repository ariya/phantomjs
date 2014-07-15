/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QTCONCURRENT_MEDIAN_H
#define QTCONCURRENT_MEDIAN_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_CONCURRENT

#include <QtCore/qvector.h>
#include <QtCore/qalgorithms.h>

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#ifndef qdoc

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
            QVector<T> sorted = values;
            qSort(sorted);
            currentMedian = sorted.at(bufferSize / 2 + 1);
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

#endif //qdoc

QT_END_NAMESPACE
QT_END_HEADER

#endif // QT_NO_CONCURRENT

#endif
