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

#ifndef QTCONCURRENT_FILTERKERNEL_H
#define QTCONCURRENT_FILTERKERNEL_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_CONCURRENT

#include <QtCore/qtconcurrentiteratekernel.h>
#include <QtCore/qtconcurrentmapkernel.h>
#include <QtCore/qtconcurrentreducekernel.h>

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#ifndef qdoc

namespace QtConcurrent {

template <typename T>
struct qValueType
{
    typedef typename T::value_type value_type;
};

template <typename T>
struct qValueType<const T*>
{
    typedef T value_type;
};

template <typename T>
struct qValueType<T*>
{
    typedef T value_type;
};

// Implementation of filter
template <typename Sequence, typename KeepFunctor, typename ReduceFunctor>
class FilterKernel : public IterateKernel<typename Sequence::const_iterator, void>
{
    typedef ReduceKernel<ReduceFunctor, Sequence, typename Sequence::value_type> Reducer;
    typedef IterateKernel<typename Sequence::const_iterator, void> IterateKernelType;
    typedef typename ReduceFunctor::result_type T;

    Sequence reducedResult;
    Sequence &sequence;
    KeepFunctor keep;
    ReduceFunctor reduce;
    Reducer reducer;

public:
    FilterKernel(Sequence &_sequence, KeepFunctor _keep, ReduceFunctor _reduce)
        : IterateKernelType(const_cast<const Sequence &>(_sequence).begin(), const_cast<const Sequence &>(_sequence).end()), reducedResult(),
          sequence(_sequence),
          keep(_keep),
          reduce(_reduce),
          reducer(OrderedReduce)
    { }

    bool runIteration(typename Sequence::const_iterator it, int index, T *)
    {
        IntermediateResults<typename Sequence::value_type> results;
        results.begin = index;
        results.end = index + 1;

            if (keep(*it))
                results.vector.append(*it);

            reducer.runReduce(reduce, reducedResult, results);
            return false;
    }

    bool runIterations(typename Sequence::const_iterator sequenceBeginIterator, int begin, int end, T *)
    {
        IntermediateResults<typename Sequence::value_type> results;
        results.begin = begin;
        results.end = end;
        results.vector.reserve(end - begin);


        typename Sequence::const_iterator it = sequenceBeginIterator;
        advance(it, begin);
        for (int i = begin; i < end; ++i) {
            if (keep(*it))
                results.vector.append(*it);
            advance(it, 1);
        }

        reducer.runReduce(reduce, reducedResult, results);
        return false;
    }

    void finish()
    {
        reducer.finish(reduce, reducedResult);
        sequence = reducedResult;
    }

    inline bool shouldThrottleThread()
    {
        return IterateKernelType::shouldThrottleThread() || reducer.shouldThrottle();
    }

    inline bool shouldStartThread()
    {
        return IterateKernelType::shouldStartThread() && reducer.shouldStartThread();
    }

    typedef void ReturnType;
    typedef void ResultType;
};

// Implementation of filter-reduce
template <typename ReducedResultType,
          typename Iterator,
          typename KeepFunctor,
          typename ReduceFunctor,
          typename Reducer = ReduceKernel<ReduceFunctor,
                                          ReducedResultType,
                                          typename qValueType<Iterator>::value_type> >
class FilteredReducedKernel : public IterateKernel<Iterator, ReducedResultType>
{
    ReducedResultType reducedResult;
    KeepFunctor keep;
    ReduceFunctor reduce;
    Reducer reducer;
    typedef IterateKernel<Iterator, ReducedResultType> IterateKernelType;

public:
    FilteredReducedKernel(Iterator begin,
                          Iterator end,
                          KeepFunctor _keep,
                          ReduceFunctor _reduce,
                          ReduceOptions reduceOption)
        : IterateKernelType(begin, end), reducedResult(), keep(_keep), reduce(_reduce), reducer(reduceOption)
    { }

#if 0
    FilteredReducedKernel(ReducedResultType initialValue,
                          KeepFunctor keep,
                          ReduceFunctor reduce,
                          ReduceOption reduceOption)
        : reducedResult(initialValue), keep(keep), reduce(reduce), reducer(reduceOption)
    { }
#endif

    bool runIteration(Iterator it, int index, ReducedResultType *)
    {
        IntermediateResults<typename qValueType<Iterator>::value_type> results;
        results.begin = index;
        results.end = index + 1;

        if (keep(*it))
            results.vector.append(*it);

        reducer.runReduce(reduce, reducedResult, results);
        return false;
    }

    bool runIterations(Iterator sequenceBeginIterator, int begin, int end, ReducedResultType *)
    {
        IntermediateResults<typename qValueType<Iterator>::value_type> results;
        results.begin = begin;
        results.end = end;
        results.vector.reserve(end - begin);

        Iterator it = sequenceBeginIterator;
        advance(it, begin);
        for (int i = begin; i < end; ++i) {
            if (keep(*it))
                results.vector.append(*it);
            advance(it, 1);
        }

        reducer.runReduce(reduce, reducedResult, results);
        return false;
    }

    void finish()
    {
        reducer.finish(reduce, reducedResult);
    }

    inline bool shouldThrottleThread()
    {
        return IterateKernelType::shouldThrottleThread() || reducer.shouldThrottle();
    }

    inline bool shouldStartThread()
    {
        return IterateKernelType::shouldStartThread() && reducer.shouldStartThread();
    }

    typedef ReducedResultType ReturnType;
    typedef ReducedResultType ResultType;
    ReducedResultType *result()
    {
        return &reducedResult;
    }
};

// Implementation of filter that reports individual results via QFutureInterface
template <typename Iterator, typename KeepFunctor>
class FilteredEachKernel : public IterateKernel<Iterator, typename qValueType<Iterator>::value_type>
{
    typedef typename qValueType<Iterator>::value_type T;
    typedef IterateKernel<Iterator, T> IterateKernelType;

    KeepFunctor keep;

public:
    typedef T ReturnType;
    typedef T ResultType;

    FilteredEachKernel(Iterator begin, Iterator end, KeepFunctor _keep)
        : IterateKernelType(begin, end), keep(_keep)
    { }

    void start()
    {
        if (this->futureInterface)
            this->futureInterface->setFilterMode(true);
        IterateKernelType::start();
    }

    bool runIteration(Iterator it, int index, T *)
    {
        if (keep(*it))
            this->reportResult(&(*it), index);
        else
            this->reportResult(0, index);
        return false;
    }

    bool runIterations(Iterator sequenceBeginIterator, int begin, int end, T *)
    {
        const int count = end - begin;
        IntermediateResults<typename qValueType<Iterator>::value_type> results;
        results.begin = begin;
        results.end = end;
        results.vector.reserve(count);

        Iterator it = sequenceBeginIterator;
        advance(it, begin);
        for (int i = begin; i < end; ++i) {
            if (keep(*it))
                results.vector.append(*it);
            advance(it, 1);
        }

        this->reportResults(results.vector, begin, count);
        return false;
    }
};

template <typename Iterator, typename KeepFunctor>
inline
ThreadEngineStarter<typename qValueType<Iterator>::value_type>
startFiltered(Iterator begin, Iterator end, KeepFunctor functor)
{
    return startThreadEngine(new FilteredEachKernel<Iterator, KeepFunctor>(begin, end, functor));
}

template <typename Sequence, typename KeepFunctor>
inline ThreadEngineStarter<typename Sequence::value_type>
startFiltered(const Sequence &sequence, KeepFunctor functor)
{
    typedef SequenceHolder1<Sequence,
                            FilteredEachKernel<typename Sequence::const_iterator, KeepFunctor>,
                            KeepFunctor>
        SequenceHolderType;
        return startThreadEngine(new SequenceHolderType(sequence, functor));
}

template <typename ResultType, typename Sequence, typename MapFunctor, typename ReduceFunctor>
inline ThreadEngineStarter<ResultType> startFilteredReduced(const Sequence & sequence,
                                                           MapFunctor mapFunctor, ReduceFunctor reduceFunctor,
                                                           ReduceOptions options)
{
    typedef typename Sequence::const_iterator Iterator;
    typedef ReduceKernel<ReduceFunctor, ResultType, typename qValueType<Iterator>::value_type > Reducer;
    typedef FilteredReducedKernel<ResultType, Iterator, MapFunctor, ReduceFunctor, Reducer> FilteredReduceType;
    typedef SequenceHolder2<Sequence, FilteredReduceType, MapFunctor, ReduceFunctor> SequenceHolderType;
    return startThreadEngine(new SequenceHolderType(sequence, mapFunctor, reduceFunctor, options));
}


template <typename ResultType, typename Iterator, typename MapFunctor, typename ReduceFunctor>
inline ThreadEngineStarter<ResultType> startFilteredReduced(Iterator begin, Iterator end,
                                                           MapFunctor mapFunctor, ReduceFunctor reduceFunctor,
                                                           ReduceOptions options)
{
    typedef ReduceKernel<ReduceFunctor, ResultType, typename qValueType<Iterator>::value_type> Reducer;
    typedef FilteredReducedKernel<ResultType, Iterator, MapFunctor, ReduceFunctor, Reducer> FilteredReduceType;
    return startThreadEngine(new FilteredReduceType(begin, end, mapFunctor, reduceFunctor, options));
}


} // namespace QtConcurrent

#endif // qdoc

QT_END_NAMESPACE
QT_END_HEADER

#endif // QT_NO_CONCURRENT

#endif
