/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebKitDLL.h"
#include "WebGeolocationPosition.h"
#include <WebCore/COMPtr.h>

#include <WebCore/GeolocationPosition.h>

using namespace WebCore;

COMPtr<WebGeolocationPosition> WebGeolocationPosition::createInstance()
{
    return new WebGeolocationPosition;
}

WebGeolocationPosition::WebGeolocationPosition()
    : m_refCount(0)
{
    gClassCount++;
    gClassNameCount.add("WebGeolocationPosition");
}

WebGeolocationPosition::~WebGeolocationPosition()
{
    gClassCount--;
    gClassNameCount.remove("WebGeolocationPosition");
}

HRESULT WebGeolocationPosition::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualIID(riid, __uuidof(WebGeolocationPosition)))
        *ppvObject = this;
    else if (IsEqualIID(riid, __uuidof(IUnknown)))
        *ppvObject = static_cast<IWebGeolocationPosition*>(this);
    else if (IsEqualIID(riid, __uuidof(IWebGeolocationPosition)))
        *ppvObject = static_cast<IWebGeolocationPosition*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG WebGeolocationPosition::AddRef()
{
    return ++m_refCount;
}

ULONG WebGeolocationPosition::Release()
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete this;

    return newRef;
}

HRESULT WebGeolocationPosition::initWithTimestamp(double timestamp, double latitude, double longitude, double accuracy)
{
    m_position = GeolocationPosition::create(timestamp, latitude, longitude, accuracy);
    return S_OK;
}

GeolocationPosition* core(IWebGeolocationPosition* position)
{
    if (!position)
        return 0;

    COMPtr<WebGeolocationPosition> webGeolocationPosition(Query, position);
    if (!webGeolocationPosition)
        return 0;

    return webGeolocationPosition->impl();
}
