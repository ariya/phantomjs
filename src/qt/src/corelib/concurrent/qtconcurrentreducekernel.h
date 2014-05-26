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

#ifndef QTCONCURRENT_REDUCEKERNEL_H
#define QTCONCURRENT_REDUCEKERNEL_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_CONCURRENT

#include <QtCore/qatomic.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>
#include <QtCore/qthreadpool.h>
#include <QtCore/qvector.h>

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

QT_MODULE(Core)

namespace QtConcurrent {

#ifndef qdoc

/*
    The ReduceQueueStartLimit and ReduceQueueThrottleLimit constants
    limit the reduce queue size for MapReduce. When the number of
    reduce blocks in the queue exceeds ReduceQueueStartLimit,
    MapReduce won't start any new threads, and when it exceeds
    ReduceQueueThrottleLimit running threads will be stopped.
*/
enum {
    ReduceQueueStartLimit = 20,
    ReduceQueueThrottleLimit = 30
};

// IntermediateResults holds a block of intermediate results from a
// map or filter functor. The begin/end offsets indicates the origin
// and range of the block.
template <typename T>
class IntermediateResults
{
public:
    int begin, end;
    QVector<T> vector;
};

#endif // qdoc

enum ReduceOption {
    UnorderedReduce = 0x1,
    OrderedReduce = 0x2,
    SequentialReduce = 0x4
    // ParallelReduce = 0x8
};
Q_DECLARE_FLAGS(ReduceOptions, ReduceOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(ReduceOptions)

#ifndef qdoc

// supports both ordered and out-of-order reduction
template <typename ReduceFunctor, typename ReduceResultType, typename T>
class ReduceKernel
{
    typedef QMap<int, IntermediateResults<T> > ResultsMap;

    const ReduceOptions reduceOptions;

    QMutex mutex;
    int progress, resultsMapSize, threadCount;
    ResultsMap resultsMap;

    bool canReduce(int begin) const
    {
        return (((reduceOptions & UnorderedReduce)
                 && progress == 0)
                || ((reduceOptions & OrderedReduce)
                    && progress == begin));
    }

    void reduceResult(ReduceFunctor &reduce,
                      ReduceResultType &r,
                      const IntermediateResults<T> &result)
    {
        for (int i = 0; i < result.vector.size(); ++i) {
            reduce(r, result.vector.at(i));
        }
    }

    void reduceResults(ReduceFunctor &reduce,
                       ReduceResultType &r,
                       ResultsMap &map)
    {
        typename ResultsMap::iterator it = map.begin();
        while (it != map.end()) {
            reduceResult(reduce, r, it.value());
            ++it;
        }
    }

public:
    ReduceKernel(ReduceOptions _reduceOptions)
        : reduceOptions(_reduceOptions), progress(0), resultsMapSize(0), 
          threadCount(QThreadPool::globalInstance()->maxThreadCount())
    { }

    void runReduce(ReduceFunctor &reduce,
                   ReduceResultType &r,
                   const IntermediateResults<T> &result)
    {
        QMutexLocker locker(&mutex);
        if (!canReduce(result.begin)) {
            ++resultsMapSize;
            resultsMap.insert(result.begin, result);
            return;
        }

        if (reduceOptions & UnorderedReduce) {
            // UnorderedReduce
            progress = -1;

            // reduce this result
            locker.unlock();
            reduceResult(reduce, r, result);
            locker.relock();

            // reduce all stored results as well
            while (!resultsMap.isEmpty()) {
                ResultsMap resultsMapCopy = resultsMap;
                resultsMap.clear();

                locker.unlock();
                reduceResults(reduce, r, resultsMapCopy);
                locker.relock();

                resultsMapSize -= resultsMapCopy.size();
            }

            progress = 0;
        } else {
            // reduce this result
            locker.unlock();
            reduceResult(reduce, r, result);
            locker.relock();

            // OrderedReduce
            progress += result.end - result.begin;

            // reduce as many other results as possible
            typename ResultsMap::iterator it = resultsMap.begin();
            while (it != resultsMap.end()) {
                if (it.value().begin != progress)
                    break;

                locker.unlock();
                reduceResult(reduce, r, it.value());
                locker.relock();

                --resultsMapSize;
                progress += it.value().end - it.value().begin;
                it = resultsMap.erase(it);
            }
        }
    }

    // final reduction
    void finish(ReduceFunctor &reduce, ReduceResultType &r)
    {
        reduceResults(reduce, r, resultsMap);
    }

    inline bool shouldThrottle()
    {
        return (resultsMapSize > (ReduceQueueThrottleLimit * threadCount));
    }

    inline bool shouldStartThread()
    {
        return (resultsMapSize <= (ReduceQueueStartLimit * threadCount));
    }
};

template <typename Sequence, typename Base, typename Functor1, typename Functor2>
struct SequenceHolder2 : public Base
{
    SequenceHolder2(const Sequence &_sequence,
                    Functor1 functor1,
                    Functor2 functor2,
                    ReduceOptions reduceOptions)
        : Base(_sequence.begin(), _sequence.end(), functor1, functor2, reduceOptions),
          sequence(_sequence)
    { }

    Sequence sequence;

    void finish()
    {
        Base::finish();
        // Clear the sequence to make sure all temporaries are destroyed
        // before finished is signaled.
        sequence = Sequence();
    }
};

#endif //qdoc

} // namespace QtConcurrent

QT_END_NAMESPACE
QT_END_HEADER

#endif // QT_NO_CONCURRENT

#endif
