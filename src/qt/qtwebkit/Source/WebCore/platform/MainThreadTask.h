/*
 * Copyright (C) 2009-2010 Google Inc. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MainThreadTask_h
#define MainThreadTask_h

#include "CrossThreadCopier.h"
#include <memory>
#include <wtf/MainThread.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/TypeTraits.h>

namespace WebCore {

// Traits for the MainThreadTask.
template<typename T> struct MainThreadTaskTraits {
    typedef const T& ParamType;
};

template<typename T> struct MainThreadTaskTraits<T*> {
    typedef T* ParamType;
};

template<typename T> struct MainThreadTaskTraits<PassRefPtr<T> > {
    typedef PassRefPtr<T> ParamType;
};

template<typename T> struct MainThreadTaskTraits<PassOwnPtr<T> > {
    typedef PassOwnPtr<T> ParamType;
};

class MainThreadTaskBase {
WTF_MAKE_NONCOPYABLE(MainThreadTaskBase);
WTF_MAKE_FAST_ALLOCATED;
public:
    MainThreadTaskBase() { }
    virtual void performTask() = 0;
    virtual ~MainThreadTaskBase() { }
};

template<typename P1, typename MP1>
class MainThreadTask1 : public MainThreadTaskBase {
public:
    typedef void (*Method)(MP1);
    typedef MainThreadTask1<P1, MP1> MainThreadTask;
    typedef typename MainThreadTaskTraits<P1>::ParamType Param1;

    static PassOwnPtr<MainThreadTask> create(Method method, Param1 parameter1)
    {
        return adoptPtr(new MainThreadTask(method, parameter1));
    }

private:
    MainThreadTask1(Method method, Param1 parameter1)
        : m_method(method)
        , m_parameter1(parameter1)
    {
    }

    virtual void performTask() OVERRIDE
    {
        (*m_method)(m_parameter1);
    }

private:
    Method m_method;
    P1 m_parameter1;
};

template<typename P1, typename MP1, typename P2, typename MP2>
class MainThreadTask2 : public MainThreadTaskBase {
public:
    typedef void (*Method)(MP1, MP2);
    typedef MainThreadTask2<P1, MP1, P2, MP2> MainThreadTask;
    typedef typename MainThreadTaskTraits<P1>::ParamType Param1;
    typedef typename MainThreadTaskTraits<P2>::ParamType Param2;

    static PassOwnPtr<MainThreadTask> create(Method method, Param1 parameter1, Param2 parameter2)
    {
        return adoptPtr(new MainThreadTask(method, parameter1, parameter2));
    }

private:
    MainThreadTask2(Method method, Param1 parameter1, Param2 parameter2)
        : m_method(method)
        , m_parameter1(parameter1)
        , m_parameter2(parameter2)
    {
    }

    virtual void performTask() OVERRIDE
    {
        (*m_method)(m_parameter1, m_parameter2);
    }

private:
    Method m_method;
    P1 m_parameter1;
    P2 m_parameter2;
};

template<typename P1, typename MP1, typename P2, typename MP2, typename P3, typename MP3>
class MainThreadTask3 : public MainThreadTaskBase {
public:
    typedef void (*Method)(MP1, MP2, MP3);
    typedef MainThreadTask3<P1, MP1, P2, MP2, P3, MP3> MainThreadTask;
    typedef typename MainThreadTaskTraits<P1>::ParamType Param1;
    typedef typename MainThreadTaskTraits<P2>::ParamType Param2;
    typedef typename MainThreadTaskTraits<P3>::ParamType Param3;

    static PassOwnPtr<MainThreadTask> create(Method method, Param1 parameter1, Param2 parameter2, Param3 parameter3)
    {
        return adoptPtr(new MainThreadTask(method, parameter1, parameter2, parameter3));
    }

private:
    MainThreadTask3(Method method, Param1 parameter1, Param2 parameter2, Param3 parameter3)
        : m_method(method)
        , m_parameter1(parameter1)
        , m_parameter2(parameter2)
        , m_parameter3(parameter3)
    {
    }

    virtual void performTask() OVERRIDE
    {
        (*m_method)(m_parameter1, m_parameter2, m_parameter3);
    }

private:
    Method m_method;
    P1 m_parameter1;
    P2 m_parameter2;
    P3 m_parameter3;
};

template<typename P1, typename MP1, typename P2, typename MP2, typename P3, typename MP3, typename P4, typename MP4>
class MainThreadTask4 : public MainThreadTaskBase {
public:
    typedef void (*Method)(MP1, MP2, MP3, MP4);
    typedef MainThreadTask4<P1, MP1, P2, MP2, P3, MP3, P4, MP4> MainThreadTask;
    typedef typename MainThreadTaskTraits<P1>::ParamType Param1;
    typedef typename MainThreadTaskTraits<P2>::ParamType Param2;
    typedef typename MainThreadTaskTraits<P3>::ParamType Param3;
    typedef typename MainThreadTaskTraits<P4>::ParamType Param4;

    static PassOwnPtr<MainThreadTask> create(Method method, Param1 parameter1, Param2 parameter2, Param3 parameter3, Param4 parameter4)
    {
        return adoptPtr(new MainThreadTask(method, parameter1, parameter2, parameter3, parameter4));
    }

private:
    MainThreadTask4(Method method, Param1 parameter1, Param2 parameter2, Param3 parameter3, Param4 parameter4)
        : m_method(method)
        , m_parameter1(parameter1)
        , m_parameter2(parameter2)
        , m_parameter3(parameter3)
        , m_parameter4(parameter4)
    {
    }

    virtual void performTask() OVERRIDE
    {
        (*m_method)(m_parameter1, m_parameter2, m_parameter3, m_parameter4);
    }

private:
    Method m_method;
    P1 m_parameter1;
    P2 m_parameter2;
    P3 m_parameter3;
    P4 m_parameter4;
};

template<typename P1, typename MP1, typename P2, typename MP2, typename P3, typename MP3, typename P4, typename MP4, typename P5, typename MP5>
class MainThreadTask5 : public MainThreadTaskBase {
public:
    typedef void (*Method)(MP1, MP2, MP3, MP4, MP5);
    typedef MainThreadTask5<P1, MP1, P2, MP2, P3, MP3, P4, MP4, P5, MP5> MainThreadTask;
    typedef typename MainThreadTaskTraits<P1>::ParamType Param1;
    typedef typename MainThreadTaskTraits<P2>::ParamType Param2;
    typedef typename MainThreadTaskTraits<P3>::ParamType Param3;
    typedef typename MainThreadTaskTraits<P4>::ParamType Param4;
    typedef typename MainThreadTaskTraits<P5>::ParamType Param5;

    static PassOwnPtr<MainThreadTask> create(Method method, Param1 parameter1, Param2 parameter2, Param3 parameter3, Param4 parameter4, Param5 parameter5)
    {
        return adoptPtr(new MainThreadTask(method, parameter1, parameter2, parameter3, parameter4, parameter5));
    }

private:
    MainThreadTask5(Method method, Param1 parameter1, Param2 parameter2, Param3 parameter3, Param4 parameter4, Param5 parameter5)
        : m_method(method)
        , m_parameter1(parameter1)
        , m_parameter2(parameter2)
        , m_parameter3(parameter3)
        , m_parameter4(parameter4)
        , m_parameter5(parameter5)
    {
    }

    virtual void performTask() OVERRIDE
    {
        (*m_method)(m_parameter1, m_parameter2, m_parameter3, m_parameter4, m_parameter5);
    }

private:
    Method m_method;
    P1 m_parameter1;
    P2 m_parameter2;
    P3 m_parameter3;
    P4 m_parameter4;
    P5 m_parameter5;
};

template<typename P1, typename MP1, typename P2, typename MP2, typename P3, typename MP3, typename P4, typename MP4, typename P5, typename MP5, typename P6, typename MP6>
class MainThreadTask6 : public MainThreadTaskBase {
public:
    typedef void (*Method)(MP1, MP2, MP3, MP4, MP5, MP6);
    typedef MainThreadTask6<P1, MP1, P2, MP2, P3, MP3, P4, MP4, P5, MP5, P6, MP6> MainThreadTask;
    typedef typename MainThreadTaskTraits<P1>::ParamType Param1;
    typedef typename MainThreadTaskTraits<P2>::ParamType Param2;
    typedef typename MainThreadTaskTraits<P3>::ParamType Param3;
    typedef typename MainThreadTaskTraits<P4>::ParamType Param4;
    typedef typename MainThreadTaskTraits<P5>::ParamType Param5;
    typedef typename MainThreadTaskTraits<P6>::ParamType Param6;

    static PassOwnPtr<MainThreadTask> create(Method method, Param1 parameter1, Param2 parameter2, Param3 parameter3, Param4 parameter4, Param5 parameter5, Param6 parameter6)
    {
        return adoptPtr(new MainThreadTask(method, parameter1, parameter2, parameter3, parameter4, parameter5, parameter6));
    }

private:
    MainThreadTask6(Method method, Param1 parameter1, Param2 parameter2, Param3 parameter3, Param4 parameter4, Param5 parameter5, Param6 parameter6)
        : m_method(method)
        , m_parameter1(parameter1)
        , m_parameter2(parameter2)
        , m_parameter3(parameter3)
        , m_parameter4(parameter4)
        , m_parameter5(parameter5)
        , m_parameter6(parameter6)
    {
    }

    virtual void performTask() OVERRIDE
    {
        (*m_method)(m_parameter1, m_parameter2, m_parameter3, m_parameter4, m_parameter5, m_parameter6);
    }

private:
    Method m_method;
    P1 m_parameter1;
    P2 m_parameter2;
    P3 m_parameter3;
    P4 m_parameter4;
    P5 m_parameter5;
    P6 m_parameter6;
};

template<typename P1, typename MP1, typename P2, typename MP2, typename P3, typename MP3, typename P4, typename MP4, typename P5, typename MP5, typename P6, typename MP6, typename P7, typename MP7>
class MainThreadTask7 : public MainThreadTaskBase {
public:
    typedef void (*Method)(MP1, MP2, MP3, MP4, MP5, MP6, MP7);
    typedef MainThreadTask7<P1, MP1, P2, MP2, P3, MP3, P4, MP4, P5, MP5, P6, MP6, P7, MP7> MainThreadTask;
    typedef typename MainThreadTaskTraits<P1>::ParamType Param1;
    typedef typename MainThreadTaskTraits<P2>::ParamType Param2;
    typedef typename MainThreadTaskTraits<P3>::ParamType Param3;
    typedef typename MainThreadTaskTraits<P4>::ParamType Param4;
    typedef typename MainThreadTaskTraits<P5>::ParamType Param5;
    typedef typename MainThreadTaskTraits<P6>::ParamType Param6;
    typedef typename MainThreadTaskTraits<P7>::ParamType Param7;

    static PassOwnPtr<MainThreadTask> create(Method method, Param1 parameter1, Param2 parameter2, Param3 parameter3, Param4 parameter4, Param5 parameter5, Param6 parameter6, Param7 parameter7)
    {
        return adoptPtr(new MainThreadTask(method, parameter1, parameter2, parameter3, parameter4, parameter5, parameter6, parameter7));
    }

private:
    MainThreadTask7(Method method, Param1 parameter1, Param2 parameter2, Param3 parameter3, Param4 parameter4, Param5 parameter5, Param6 parameter6, Param7 parameter7)
        : m_method(method)
        , m_parameter1(parameter1)
        , m_parameter2(parameter2)
        , m_parameter3(parameter3)
        , m_parameter4(parameter4)
        , m_parameter5(parameter5)
        , m_parameter6(parameter6)
        , m_parameter7(parameter7)
    {
    }

    virtual void performTask() OVERRIDE
    {
        (*m_method)(m_parameter1, m_parameter2, m_parameter3, m_parameter4, m_parameter5, m_parameter6, m_parameter7);
    }

private:
    Method m_method;
    P1 m_parameter1;
    P2 m_parameter2;
    P3 m_parameter3;
    P4 m_parameter4;
    P5 m_parameter5;
    P6 m_parameter6;
    P7 m_parameter7;
};

template<typename P1, typename MP1, typename P2, typename MP2, typename P3, typename MP3, typename P4, typename MP4, typename P5, typename MP5, typename P6, typename MP6, typename P7, typename MP7, typename P8, typename MP8>
class MainThreadTask8 : public MainThreadTaskBase {
public:
    typedef void (*Method)(MP1, MP2, MP3, MP4, MP5, MP6, MP7, MP8);
    typedef MainThreadTask8<P1, MP1, P2, MP2, P3, MP3, P4, MP4, P5, MP5, P6, MP6, P7, MP7, P8, MP8> MainThreadTask;
    typedef typename MainThreadTaskTraits<P1>::ParamType Param1;
    typedef typename MainThreadTaskTraits<P2>::ParamType Param2;
    typedef typename MainThreadTaskTraits<P3>::ParamType Param3;
    typedef typename MainThreadTaskTraits<P4>::ParamType Param4;
    typedef typename MainThreadTaskTraits<P5>::ParamType Param5;
    typedef typename MainThreadTaskTraits<P6>::ParamType Param6;
    typedef typename MainThreadTaskTraits<P7>::ParamType Param7;
    typedef typename MainThreadTaskTraits<P8>::ParamType Param8;
    
    static PassOwnPtr<MainThreadTask> create(Method method, Param1 parameter1, Param2 parameter2, Param3 parameter3, Param4 parameter4, Param5 parameter5, Param6 parameter6, Param7 parameter7, Param8 parameter8)
    {
        return adoptPtr(new MainThreadTask(method, parameter1, parameter2, parameter3, parameter4, parameter5, parameter6, parameter7, parameter8));
    }
    
private:
    MainThreadTask8(Method method, Param1 parameter1, Param2 parameter2, Param3 parameter3, Param4 parameter4, Param5 parameter5, Param6 parameter6, Param7 parameter7, Param8 parameter8)
        : m_method(method)
        , m_parameter1(parameter1)
        , m_parameter2(parameter2)
        , m_parameter3(parameter3)
        , m_parameter4(parameter4)
        , m_parameter5(parameter5)
        , m_parameter6(parameter6)
        , m_parameter7(parameter7)
        , m_parameter8(parameter8)
    {
    }
    
    virtual void performTask() OVERRIDE
    {
        (*m_method)(m_parameter1, m_parameter2, m_parameter3, m_parameter4, m_parameter5, m_parameter6, m_parameter7, m_parameter8);
    }
    
private:
    Method m_method;
    P1 m_parameter1;
    P2 m_parameter2;
    P3 m_parameter3;
    P4 m_parameter4;
    P5 m_parameter5;
    P6 m_parameter6;
    P7 m_parameter7;
    P8 m_parameter8;
};

static void executeMainThreadTask(void* context)
{
    OwnPtr<MainThreadTaskBase> task = adoptPtr(static_cast<MainThreadTaskBase*>(context));
    task->performTask();
}

template<typename P1, typename MP1>
void callOnMainThread(
    void (*method)(MP1),
    const P1& parameter1)
{
    WTF::callOnMainThread(executeMainThreadTask, MainThreadTask1<typename CrossThreadCopier<P1>::Type, MP1>::create(
        method,
        CrossThreadCopier<P1>::copy(parameter1)).leakPtr());
}

template<typename P1, typename MP1, typename P2, typename MP2>
void callOnMainThread(
    void (*method)(MP1, MP2),
    const P1& parameter1, const P2& parameter2)
{
    WTF::callOnMainThread(executeMainThreadTask, MainThreadTask2<typename CrossThreadCopier<P1>::Type, MP1, typename CrossThreadCopier<P2>::Type, MP2>::create(
        method,
        CrossThreadCopier<P1>::copy(parameter1), CrossThreadCopier<P2>::copy(parameter2)).leakPtr());
}

template<typename P1, typename MP1, typename P2, typename MP2, typename P3, typename MP3>
void callOnMainThread(
    void (*method)(MP1, MP2, MP3),
    const P1& parameter1, const P2& parameter2, const P3& parameter3)
{
    WTF::callOnMainThread(executeMainThreadTask, MainThreadTask3<typename CrossThreadCopier<P1>::Type, MP1, typename CrossThreadCopier<P2>::Type, MP2, typename CrossThreadCopier<P3>::Type, MP3>::create(
        method,
        CrossThreadCopier<P1>::copy(parameter1), CrossThreadCopier<P2>::copy(parameter2),
        CrossThreadCopier<P3>::copy(parameter3)).leakPtr());
}

template<typename P1, typename MP1, typename P2, typename MP2, typename P3, typename MP3, typename P4, typename MP4>
void callOnMainThread(
    void (*method)(MP1, MP2, MP3, MP4),
    const P1& parameter1, const P2& parameter2, const P3& parameter3, const P4& parameter4)
{
    WTF::callOnMainThread(executeMainThreadTask, MainThreadTask4<typename CrossThreadCopier<P1>::Type, MP1, typename CrossThreadCopier<P2>::Type, MP2, typename CrossThreadCopier<P3>::Type, MP3,
        typename CrossThreadCopier<P4>::Type, MP4>::create(
            method,
            CrossThreadCopier<P1>::copy(parameter1), CrossThreadCopier<P2>::copy(parameter2),
            CrossThreadCopier<P3>::copy(parameter3), CrossThreadCopier<P4>::copy(parameter4)).leakPtr());
}

template<typename P1, typename MP1, typename P2, typename MP2, typename P3, typename MP3, typename P4, typename MP4, typename P5, typename MP5>
void callOnMainThread(
    void (*method)(MP1, MP2, MP3, MP4, MP5),
    const P1& parameter1, const P2& parameter2, const P3& parameter3, const P4& parameter4, const P5& parameter5)
{
    WTF::callOnMainThread(executeMainThreadTask, MainThreadTask5<typename CrossThreadCopier<P1>::Type, MP1, typename CrossThreadCopier<P2>::Type, MP2, typename CrossThreadCopier<P3>::Type, MP3,
        typename CrossThreadCopier<P4>::Type, MP4, typename CrossThreadCopier<P5>::Type, MP5>::create(
            method,
            CrossThreadCopier<P1>::copy(parameter1), CrossThreadCopier<P2>::copy(parameter2),
            CrossThreadCopier<P3>::copy(parameter3), CrossThreadCopier<P4>::copy(parameter4),
            CrossThreadCopier<P5>::copy(parameter5)).leakPtr());
}

template<typename P1, typename MP1, typename P2, typename MP2, typename P3, typename MP3, typename P4, typename MP4, typename P5, typename MP5, typename P6, typename MP6>
void callOnMainThread(
    void (*method)(MP1, MP2, MP3, MP4, MP5, MP6),
    const P1& parameter1, const P2& parameter2, const P3& parameter3, const P4& parameter4, const P5& parameter5, const P6& parameter6)
{
    WTF::callOnMainThread(executeMainThreadTask, MainThreadTask6<typename CrossThreadCopier<P1>::Type, MP1, typename CrossThreadCopier<P2>::Type, MP2, typename CrossThreadCopier<P3>::Type, MP3,
        typename CrossThreadCopier<P4>::Type, MP4, typename CrossThreadCopier<P5>::Type, MP5, typename CrossThreadCopier<P6>::Type, MP6>::create(
            method,
            CrossThreadCopier<P1>::copy(parameter1), CrossThreadCopier<P2>::copy(parameter2),
            CrossThreadCopier<P3>::copy(parameter3), CrossThreadCopier<P4>::copy(parameter4),
            CrossThreadCopier<P5>::copy(parameter5), CrossThreadCopier<P6>::copy(parameter6)).leakPtr());
}

template<typename P1, typename MP1, typename P2, typename MP2, typename P3, typename MP3, typename P4, typename MP4, typename P5, typename MP5, typename P6, typename MP6, typename P7, typename MP7>
void callOnMainThread(
    void (*method)(MP1, MP2, MP3, MP4, MP5, MP6, MP7),
    const P1& parameter1, const P2& parameter2, const P3& parameter3, const P4& parameter4, const P5& parameter5, const P6& parameter6, const P7& parameter7)
{
    WTF::callOnMainThread(executeMainThreadTask, MainThreadTask7<typename CrossThreadCopier<P1>::Type, MP1, typename CrossThreadCopier<P2>::Type, MP2, typename CrossThreadCopier<P3>::Type, MP3,
        typename CrossThreadCopier<P4>::Type, MP4, typename CrossThreadCopier<P5>::Type, MP5, typename CrossThreadCopier<P6>::Type, MP6,
        typename CrossThreadCopier<P7>::Type, MP7>::create(
            method,
            CrossThreadCopier<P1>::copy(parameter1), CrossThreadCopier<P2>::copy(parameter2),
            CrossThreadCopier<P3>::copy(parameter3), CrossThreadCopier<P4>::copy(parameter4),
            CrossThreadCopier<P5>::copy(parameter5), CrossThreadCopier<P6>::copy(parameter6),
            CrossThreadCopier<P7>::copy(parameter7)).leakPtr());
}

template<typename P1, typename MP1, typename P2, typename MP2, typename P3, typename MP3, typename P4, typename MP4, typename P5, typename MP5, typename P6, typename MP6, typename P7, typename MP7, typename P8, typename MP8>
void callOnMainThread(
    void (*method)(MP1, MP2, MP3, MP4, MP5, MP6, MP7, MP8),
    const P1& parameter1, const P2& parameter2, const P3& parameter3, const P4& parameter4, const P5& parameter5, const P6& parameter6, const P7& parameter7, const P8& parameter8)
{
    WTF::callOnMainThread(executeMainThreadTask, MainThreadTask8<typename CrossThreadCopier<P1>::Type, MP1, typename CrossThreadCopier<P2>::Type, MP2, typename CrossThreadCopier<P3>::Type, MP3,
    typename CrossThreadCopier<P4>::Type, MP4, typename CrossThreadCopier<P5>::Type, MP5, typename CrossThreadCopier<P6>::Type, MP6,
    typename CrossThreadCopier<P7>::Type, MP7, typename CrossThreadCopier<P8>::Type, MP8>::create(
                                                       method,
                                                       CrossThreadCopier<P1>::copy(parameter1), CrossThreadCopier<P2>::copy(parameter2),
                                                       CrossThreadCopier<P3>::copy(parameter3), CrossThreadCopier<P4>::copy(parameter4),
                                                       CrossThreadCopier<P5>::copy(parameter5), CrossThreadCopier<P6>::copy(parameter6),
                                                       CrossThreadCopier<P7>::copy(parameter7), CrossThreadCopier<P8>::copy(parameter8)).leakPtr());
}

} // namespace WebCore


#endif // MainThreadTask_h
