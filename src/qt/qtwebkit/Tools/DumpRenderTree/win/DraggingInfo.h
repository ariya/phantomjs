/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DraggingInfo_h
#define DraggingInfo_h

#include <objidl.h>

class DraggingInfo {
public:
    DraggingInfo(IDataObject* object, IDropSource* source)
        : m_object(object)
        , m_source(source)
        , m_performedDropEffect(DROPEFFECT_NONE)
    {
        m_object->AddRef();
        m_source->AddRef();
    }

    ~DraggingInfo()
    {
        if (m_object)
            m_object->Release();
        m_object = 0;
        if (m_source)
            m_source->Release();
        m_source = 0;
    }

    IDataObject* dataObject() const { return m_object; }
    IDropSource* dropSource() const { return m_source; }

    DWORD performedDropEffect() const { return m_performedDropEffect; }
    void setPerformedDropEffect(DWORD effect) { m_performedDropEffect = effect; }

private:
    IDataObject* m_object;
    IDropSource* m_source;
    DWORD m_performedDropEffect;
};

#endif // !defined(DraggingInfo_h)
