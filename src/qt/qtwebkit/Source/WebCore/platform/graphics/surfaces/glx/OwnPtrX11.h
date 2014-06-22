/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#ifndef OwnPtrX11_h
#define OwnPtrX11_h

namespace WebCore {

template<typename T>
struct OwnPtrX11 {

    OwnPtrX11() : m_xResource(0) { }
    OwnPtrX11(T* xResource) : m_xResource(xResource) { }

    ~OwnPtrX11()
    {
        if (m_xResource)
            XFree(m_xResource);
    }

    OwnPtrX11& operator=(T* xResource)
    {
        if (m_xResource)
            XFree(m_xResource);

        m_xResource = xResource;
        return *this;
    }

    operator T*() const { return m_xResource; }
    T* operator->() const { return m_xResource; }
    T* get() const { return m_xResource; }

private:
    OwnPtrX11(const OwnPtrX11&);
    OwnPtrX11& operator=(const OwnPtrX11&);
    static void* operator new (size_t);
    static void operator delete (void*);
    T* m_xResource;
};

}

#endif
