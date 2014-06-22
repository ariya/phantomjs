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
#include <wtf/Vector.h>

#if PLATFORM(MAC)

#include <wtf/RetainPtr.h>
#include "Timer.h"

typedef const struct __CFArray * CFArrayRef;
typedef const struct __SCDynamicStore * SCDynamicStoreRef;

#elif PLATFORM(WIN)

#include <windows.h>

#elif PLATFORM(QT)

#include <QtCore/qglobal.h>

#elif PLATFORM(EFL)

typedef struct _Ecore_Fd_Handler Ecore_Fd_Handler;
typedef unsigned char Eina_Bool;

#endif

namespace WebCore {

#if (PLATFORM(QT) && !defined(QT_NO_BEARERMANAGEMENT))
class NetworkStateNotifierPrivate;
#endif

class NetworkStateNotifier {
    WTF_MAKE_NONCOPYABLE(NetworkStateNotifier); WTF_MAKE_FAST_ALLOCATED;
public:
    NetworkStateNotifier();
#if PLATFORM(EFL)
    ~NetworkStateNotifier();
#endif
    typedef void (*NetworkStateChangeListener)(bool m_isOnLine);
    void addNetworkStateChangeListener(NetworkStateChangeListener);

    bool onLine() const { return m_isOnLine; }

#if (PLATFORM(QT) && !defined(QT_NO_BEARERMANAGEMENT))
    void setNetworkAccessAllowed(bool);
#endif

#if PLATFORM(BLACKBERRY)
    void networkStateChange(bool online);
#endif

private:
    bool m_isOnLine;
    Vector<NetworkStateChangeListener> m_listeners;

    void notifyNetworkStateChange();
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

#elif PLATFORM(EFL)
    void networkInterfaceChanged();
    static Eina_Bool readSocketCallback(void* userData, Ecore_Fd_Handler*);

    int m_netlinkSocket;
    Ecore_Fd_Handler* m_fdHandler;

#elif (PLATFORM(QT) && !defined(QT_NO_BEARERMANAGEMENT))
    friend class NetworkStateNotifierPrivate;
    NetworkStateNotifierPrivate* p;
#endif
};

#if !PLATFORM(MAC) && !PLATFORM(WIN) && !(PLATFORM(QT) && !defined(QT_NO_BEARERMANAGEMENT)) && !PLATFORM(BLACKBERRY) && !PLATFORM(EFL)

inline NetworkStateNotifier::NetworkStateNotifier()
    : m_isOnLine(true)
{
}

inline void NetworkStateNotifier::updateState() { }

#endif

NetworkStateNotifier& networkStateNotifier();

};

#endif // NetworkStateNotifier_h
