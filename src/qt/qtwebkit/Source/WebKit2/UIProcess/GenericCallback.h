/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GenericCallback_h
#define GenericCallback_h

#include "ShareableBitmap.h"
#include "WKAPICast.h"
#include "WebError.h"
#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebKit {

class CallbackBase : public RefCounted<CallbackBase> {
public:
    virtual ~CallbackBase()
    {
    }

    uint64_t callbackID() const { return m_callbackID; }

protected:
    explicit CallbackBase(void* context)
        : m_context(context)
        , m_callbackID(generateCallbackID())
    {
    }

    void* context() const { return m_context; }

private:
    static uint64_t generateCallbackID()
    {
        static uint64_t uniqueCallbackID = 1;
        return uniqueCallbackID++;
    }

    void* m_context;
    uint64_t m_callbackID;
};

class VoidCallback : public CallbackBase {
public:
    typedef void (*CallbackFunction)(WKErrorRef, void*);

    static PassRefPtr<VoidCallback> create(void* context, CallbackFunction callback)
    {
        return adoptRef(new VoidCallback(context, callback));
    }

    virtual ~VoidCallback()
    {
        ASSERT(!m_callback);
    }

    void performCallback()
    {
        ASSERT(m_callback);

        m_callback(0, context());

        m_callback = 0;
    }
    
    void invalidate()
    {
        ASSERT(m_callback);

        RefPtr<WebError> error = WebError::create();
        m_callback(toAPI(error.get()), context());
        
        m_callback = 0;
    }

private:
    VoidCallback(void* context, CallbackFunction callback)
        : CallbackBase(context)
        , m_callback(callback)
    {
    }

    CallbackFunction m_callback;
};

template<typename APIReturnValueType, typename InternalReturnValueType = typename APITypeInfo<APIReturnValueType>::ImplType>
class GenericCallback : public CallbackBase {
public:
    typedef void (*CallbackFunction)(APIReturnValueType, WKErrorRef, void*);

    static PassRefPtr<GenericCallback> create(void* context, CallbackFunction callback)
    {
        return adoptRef(new GenericCallback(context, callback));
    }

    virtual ~GenericCallback()
    {
        ASSERT(!m_callback);
    }

    void performCallbackWithReturnValue(InternalReturnValueType returnValue)
    {
        ASSERT(m_callback);

        m_callback(toAPI(returnValue), 0, context());

        m_callback = 0;
    }
    
    void invalidate()
    {
        ASSERT(m_callback);

        RefPtr<WebError> error = WebError::create();
        m_callback(0, toAPI(error.get()), context());
        
        m_callback = 0;
    }

private:
    GenericCallback(void* context, CallbackFunction callback)
        : CallbackBase(context)
        , m_callback(callback)
    {
    }

    CallbackFunction m_callback;
};

// FIXME: Make a version of CallbackBase with two arguments, and define ComputedPagesCallback as a specialization.
class ComputedPagesCallback : public CallbackBase {
public:
    typedef void (*CallbackFunction)(const Vector<WebCore::IntRect>&, double, WKErrorRef, void*);

    static PassRefPtr<ComputedPagesCallback> create(void* context, CallbackFunction callback)
    {
        return adoptRef(new ComputedPagesCallback(context, callback));
    }

    virtual ~ComputedPagesCallback()
    {
        ASSERT(!m_callback);
    }

    void performCallbackWithReturnValue(const Vector<WebCore::IntRect>& returnValue1, double returnValue2)
    {
        ASSERT(m_callback);

        m_callback(returnValue1, returnValue2, 0, context());

        m_callback = 0;
    }
    
    void invalidate()
    {
        ASSERT(m_callback);

        RefPtr<WebError> error = WebError::create();
        m_callback(Vector<WebCore::IntRect>(), 0, toAPI(error.get()), context());
        
        m_callback = 0;
    }

private:

    ComputedPagesCallback(void* context, CallbackFunction callback)
        : CallbackBase(context)
        , m_callback(callback)
    {
    }

    CallbackFunction m_callback;
};

class ImageCallback : public CallbackBase {
public:
    typedef void (*CallbackFunction)(const ShareableBitmap::Handle&, WKErrorRef, void*);

    static PassRefPtr<ImageCallback> create(void* context, CallbackFunction callback)
    {
        return adoptRef(new ImageCallback(context, callback));
    }

    virtual ~ImageCallback()
    {
        ASSERT(!m_callback);
    }

    void performCallbackWithReturnValue(const ShareableBitmap::Handle& returnValue1)
    {
        ASSERT(m_callback);

        m_callback(returnValue1, 0, context());

        m_callback = 0;
    }

    void invalidate()
    {
        ASSERT(m_callback);

        RefPtr<WebError> error = WebError::create();
        ShareableBitmap::Handle handle;
        m_callback(handle, toAPI(error.get()), context());

        m_callback = 0;
    }

private:

    ImageCallback(void* context, CallbackFunction callback)
        : CallbackBase(context)
        , m_callback(callback)
    {
    }
    
    CallbackFunction m_callback;
};

template<typename T>
void invalidateCallbackMap(HashMap<uint64_t, T>& map)
{
    Vector<T> callbacksVector;
    copyValuesToVector(map, callbacksVector);
    for (size_t i = 0, size = callbacksVector.size(); i < size; ++i)
        callbacksVector[i]->invalidate();
    map.clear();
}

} // namespace WebKit

#endif // GenericCallback_h
