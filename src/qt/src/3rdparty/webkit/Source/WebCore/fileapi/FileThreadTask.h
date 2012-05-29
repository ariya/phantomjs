/*
 * Copyright (C) 2010 Google Inc.  All rights reserved.
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

#ifndef FileThreadTask_h
#define FileThreadTask_h

#include "CrossThreadCopier.h"
#include "CrossThreadTask.h"
#include "FileThread.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

template<typename T>
class FileThreadTask0 : public FileThread::Task {
public:
    typedef void (T::*Method)();
    typedef FileThreadTask0<T> FileThreadTaskImpl;

    static PassOwnPtr<FileThreadTaskImpl> create(T* instance, Method method)
    {
        return adoptPtr(new FileThreadTaskImpl(instance, method));
    }

private:
    FileThreadTask0(T* instance, Method method)
        : FileThread::Task(instance)
        , m_method(method)
    {
    }

    virtual void performTask()
    {
        (*static_cast<T*>(instance()).*m_method)();
    }

private:
    Method m_method;
};

template<typename T, typename P1, typename MP1>
class FileThreadTask1 : public FileThread::Task {
public:
    typedef void (T::*Method)(MP1);
    typedef FileThreadTask1<T, P1, MP1> FileThreadTaskImpl;
    typedef typename CrossThreadTaskTraits<P1>::ParamType Param1;

    static PassOwnPtr<FileThreadTaskImpl> create(T* instance, Method method, Param1 parameter1)
    {
        return adoptPtr(new FileThreadTaskImpl(instance, method, parameter1));
    }

private:
    FileThreadTask1(T* instance, Method method, Param1 parameter1)
        : FileThread::Task(instance)
        , m_method(method)
        , m_parameter1(parameter1)
    {
    }

    virtual void performTask()
    {
        (*static_cast<T*>(instance()).*m_method)(m_parameter1);
    }

private:
    Method m_method;
    P1 m_parameter1;
};

template<typename T, typename P1, typename MP1, typename P2, typename MP2>
class FileThreadTask2 : public FileThread::Task {
public:
    typedef void (T::*Method)(MP1, MP2);
    typedef FileThreadTask2<T, P1, MP1, P2, MP2> FileThreadTaskImpl;
    typedef typename CrossThreadTaskTraits<P1>::ParamType Param1;
    typedef typename CrossThreadTaskTraits<P2>::ParamType Param2;

    static PassOwnPtr<FileThreadTaskImpl> create(T* instance, Method method, Param1 parameter1, Param2 parameter2)
    {
        return adoptPtr(new FileThreadTaskImpl(instance, method, parameter1, parameter2));
    }

private:
    FileThreadTask2(T* instance, Method method, Param1 parameter1, Param2 parameter2)
        : FileThread::Task(instance)
        , m_method(method)
        , m_parameter1(parameter1)
        , m_parameter2(parameter2)
    {
    }

    virtual void performTask()
    {
        (*static_cast<T*>(instance()).*m_method)(m_parameter1, m_parameter2);
    }

private:
    Method m_method;
    P1 m_parameter1;
    P2 m_parameter2;
};

template<typename T, typename P1, typename MP1, typename P2, typename MP2, typename P3, typename MP3>
class FileThreadTask3 : public FileThread::Task {
public:
    typedef void (T::*Method)(MP1, MP2, MP3);
    typedef FileThreadTask3<T, P1, MP1, P2, MP2, P3, MP3> FileThreadTaskImpl;
    typedef typename CrossThreadTaskTraits<P1>::ParamType Param1;
    typedef typename CrossThreadTaskTraits<P2>::ParamType Param2;
    typedef typename CrossThreadTaskTraits<P3>::ParamType Param3;

    static PassOwnPtr<FileThreadTaskImpl> create(T* instance, Method method, Param1 parameter1, Param2 parameter2, Param3 parameter3)
    {
        return adoptPtr(new FileThreadTaskImpl(instance, method, parameter1, parameter2, parameter3));
    }

private:
    FileThreadTask3(T* instance, Method method, Param1 parameter1, Param2 parameter2, Param3 parameter3)
        : FileThread::Task(instance)
        , m_method(method)
        , m_parameter1(parameter1)
        , m_parameter2(parameter2)
        , m_parameter3(parameter3)
    {
    }

    virtual void performTask()
    {
        (*static_cast<T*>(instance()).*m_method)(m_parameter1, m_parameter2, m_parameter3);
    }

private:
    Method m_method;
    P1 m_parameter1;
    P2 m_parameter2;
    P3 m_parameter3;
};

template<typename T>
PassOwnPtr<FileThread::Task> createFileThreadTask(
    T* const callee,
    void (T::*method)());

template<typename T>
PassOwnPtr<FileThread::Task> createFileThreadTask(
    T* const callee,
    void (T::*method)())
{
    return FileThreadTask0<T>::create(
        callee,
        method);
}

template<typename T, typename P1, typename MP1>
PassOwnPtr<FileThread::Task> createFileThreadTask(
    T* const callee,
    void (T::*method)(MP1),
    const P1& parameter1)
{
    return FileThreadTask1<T, typename CrossThreadCopier<P1>::Type, MP1>::create(
        callee,
        method,
        CrossThreadCopier<P1>::copy(parameter1));
}

template<typename T, typename P1, typename MP1, typename P2, typename MP2>
PassOwnPtr<FileThread::Task> createFileThreadTask(
    T* const callee,
    void (T::*method)(MP1, MP2),
    const P1& parameter1,
    const P2& parameter2)
{
    return FileThreadTask2<T, typename CrossThreadCopier<P1>::Type, MP1, typename CrossThreadCopier<P2>::Type, MP2>::create(
        callee,
        method,
        CrossThreadCopier<P1>::copy(parameter1),
        CrossThreadCopier<P2>::copy(parameter2));
}

template<typename T, typename P1, typename MP1, typename P2, typename MP2, typename P3, typename MP3>
PassOwnPtr<FileThread::Task> createFileThreadTask(
    T* const callee,
    void (T::*method)(MP1, MP2, MP3),
    const P1& parameter1,
    const P2& parameter2,
    const P3& parameter3)
{
    return FileThreadTask3<T, typename CrossThreadCopier<P1>::Type, MP1, typename CrossThreadCopier<P2>::Type, MP2, typename CrossThreadCopier<P3>::Type, MP3>::create(
        callee,
        method,
        CrossThreadCopier<P1>::copy(parameter1),
        CrossThreadCopier<P2>::copy(parameter2),
        CrossThreadCopier<P3>::copy(parameter3));
}

} // namespace WebCore

#endif // FileThreadTask_h
