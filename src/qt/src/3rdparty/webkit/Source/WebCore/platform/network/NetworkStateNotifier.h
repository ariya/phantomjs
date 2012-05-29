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

#ifndef NetworkStateNotifier_h
#define NetworkStateNotifier_h

#include <wtf/FastAllocBase.h>
#include <wtf/Noncopyable.h>

#if PLATFORM(MAC)

#include <wtf/RetainPtr.h>
#include "Timer.h"

typedef const struct __CFArray * CFArrayRef;
typedef const struct __SCDynamicStore * SCDynamicStoreRef;

#elif PLATFORM(WIN)

#include <windows.h>

#elif PLATFORM(QT)

#include <QtCore/qglobal.h>

#ifdef QT_NO_BEARERMANAGEMENT
#undef WTF_USE_QT_BEARER
#define WTF_USE_QT_BEARER 0
#endif

#endif

namespace WebCore {

#if (PLATFORM(QT) && USE(QT_BEARER))
class NetworkStateNotifierPrivate;
#endif

class NetworkStateNotifier {
    WTF_MAKE_NONCOPYABLE(NetworkStateNotifier); WTF_MAKE_FAST_ALLOCATED;
public:
    NetworkStateNotifier();
    void setNetworkStateChangedFunction(void (*)());

    bool onLine() const { return m_isOnLine; }

#if (PLATFORM(QT) && USE(QT_BEARER))
    void setNetworkAccessAllowed(bool);
#elif PLATFORM(ANDROID) || PLATFORM(CHROMIUM)
    void setOnLine(bool);
#endif

#if PLATFORM(ANDROID)
    void networkStateChange(bool online) { setOnLine(online); }
#endif

private:
    bool m_isOnLine;
    void (*m_networkStateChangedFunction)();

    void updateState();

#if PLATFORM(MAC)
    void networkStateChangeTimerFired(Timer<NetworkStateNotifier>*);

    static void dynamicStoreCallback(SCDynamicStoreRef, CFArrayRef changedKeys, void *info); 

    RetainPtr<SCDynamicStoreRef> m_store;
    Timer<NetworkStateNotifier> m_networkStateChangeTimer;

#elif PLATFORM(WIN)
    static void CALLBACK addrChangeCallback(void*, BOOLEAN timedOut);
    static void callAddressChanged(void*);
    void addressChanged();

    void registerForAddressChange();
    HANDLE m_waitHandle;
    OVERLAPPED m_overlapped;

#elif PLATFORM(QT) && USE(QT_BEARER)
    friend class NetworkStateNotifierPrivate;
    NetworkStateNotifierPrivate* p;
#endif
};

#if !PLATFORM(MAC) && !PLATFORM(WIN) && !(PLATFORM(QT) && USE(QT_BEARER))

inline NetworkStateNotifier::NetworkStateNotifier()
    : m_isOnLine(true)
    , m_networkStateChangedFunction(0)
{
}

inline void NetworkStateNotifier::updateState() { }

#endif

NetworkStateNotifier& networkStateNotifier();

};

#endif // NetworkStateNotifier_h
