/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "NetworkStateNotifier.h"

#include <wtf/MainThread.h>
#include <wtf/Vector.h>

#include <winsock2.h>
#include <iphlpapi.h>

namespace WebCore {

void NetworkStateNotifier::updateState()
{
    // Assume that we're online until proven otherwise.
    m_isOnLine = true;
    
    Vector<char> buffer;
    DWORD size = 0;

    if (::GetAdaptersAddresses(AF_UNSPEC, 0, 0, 0, &size) != ERROR_BUFFER_OVERFLOW)
        return;

    buffer.resize(size);
    PIP_ADAPTER_ADDRESSES addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

    if (::GetAdaptersAddresses(AF_UNSPEC, 0, 0, addresses, &size) != ERROR_SUCCESS) {
        // We couldn't determine whether we're online or not, so assume that we are.
        return;
    }

    for (; addresses; addresses = addresses->Next) {
        if (addresses->IfType == MIB_IF_TYPE_LOOPBACK)
            continue;

        if (addresses->OperStatus != IfOperStatusUp)
            continue;

        // We found an interface that was up.
        return;
    }
    
    // We didn't find any valid interfaces, so we must be offline.
    m_isOnLine = false;
}

void NetworkStateNotifier::addressChanged()
{
    bool oldOnLine = m_isOnLine;
    
    updateState();

    if (m_isOnLine == oldOnLine)
        return;

    notifyNetworkStateChange();
}

void NetworkStateNotifier::callAddressChanged(void* context)
{
    static_cast<NetworkStateNotifier*>(context)->addressChanged();
}

void CALLBACK NetworkStateNotifier::addrChangeCallback(void* context, BOOLEAN timedOut)
{
    // NotifyAddrChange only notifies us of a single address change. Now that we've been notified,
    // we need to call it again so we'll get notified the *next* time.
    static_cast<NetworkStateNotifier*>(context)->registerForAddressChange();

    callOnMainThread(callAddressChanged, context);
}

void NetworkStateNotifier::registerForAddressChange()
{
    HANDLE handle;
    ::NotifyAddrChange(&handle, &m_overlapped);
}

NetworkStateNotifier::NetworkStateNotifier()
    : m_isOnLine(false)
{
    updateState();

    memset(&m_overlapped, 0, sizeof(m_overlapped));

// FIXME: Check m_overlapped on WinCE.
#if !OS(WINCE)
    m_overlapped.hEvent = ::CreateEvent(0, false, false, 0);

    ::RegisterWaitForSingleObject(&m_waitHandle, m_overlapped.hEvent, addrChangeCallback, this, INFINITE, 0);

    registerForAddressChange();
#endif
}

} // namespace WebCore
