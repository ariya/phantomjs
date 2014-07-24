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

#include <SystemConfiguration/SystemConfiguration.h>


namespace WebCore {

static const double StateChangeTimerInterval = 2.0;

void NetworkStateNotifier::updateState()
{
    // Assume that we're offline until proven otherwise.
    m_isOnLine = false;
    
    RetainPtr<CFStringRef> str = adoptCF(SCDynamicStoreKeyCreateNetworkInterface(0, kSCDynamicStoreDomainState));
    
    RetainPtr<CFPropertyListRef> propertyList = adoptCF(SCDynamicStoreCopyValue(m_store.get(), str.get()));
    
    if (!propertyList)
        return;
    
    if (CFGetTypeID(propertyList.get()) != CFDictionaryGetTypeID())
        return;
    
    CFArrayRef netInterfaces = (CFArrayRef)CFDictionaryGetValue((CFDictionaryRef)propertyList.get(), kSCDynamicStorePropNetInterfaces);
    if (CFGetTypeID(netInterfaces) != CFArrayGetTypeID())
        return;
    
    for (CFIndex i = 0; i < CFArrayGetCount(netInterfaces); i++) {
        CFStringRef interface = (CFStringRef)CFArrayGetValueAtIndex(netInterfaces, i);
        if (CFGetTypeID(interface) != CFStringGetTypeID())
            continue;
        
        // Ignore the loopback interface.
        if (CFStringFind(interface, CFSTR("lo"), kCFCompareAnchored).location != kCFNotFound)
            continue;

        RetainPtr<CFStringRef> key = adoptCF(SCDynamicStoreKeyCreateNetworkInterfaceEntity(0, kSCDynamicStoreDomainState, interface, kSCEntNetIPv4));

        RetainPtr<CFArrayRef> keyList = adoptCF(SCDynamicStoreCopyKeyList(m_store.get(), key.get()));
    
        if (keyList && CFArrayGetCount(keyList.get())) {
            m_isOnLine = true;
            break;
        }
    }
}

void NetworkStateNotifier::dynamicStoreCallback(SCDynamicStoreRef, CFArrayRef, void* info) 
{
    NetworkStateNotifier* notifier = static_cast<NetworkStateNotifier*>(info);
    
    // Calling updateState() could be expensive so we schedule a timer that will do it 
    // when things have cooled down.
    notifier->m_networkStateChangeTimer.startOneShot(StateChangeTimerInterval);
}

void NetworkStateNotifier::networkStateChangeTimerFired(Timer<NetworkStateNotifier>*)
{
    bool oldOnLine = m_isOnLine;
    
    updateState();
    
    if (m_isOnLine == oldOnLine)
        return;

    notifyNetworkStateChange();
}

NetworkStateNotifier::NetworkStateNotifier()
    : m_isOnLine(false)
    , m_networkStateChangeTimer(this, &NetworkStateNotifier::networkStateChangeTimerFired)
{
    SCDynamicStoreContext context = { 0, this, 0, 0, 0 };
    
    m_store = adoptCF(SCDynamicStoreCreate(0, CFSTR("com.apple.WebCore"), dynamicStoreCallback, &context));
    if (!m_store)
        return;

    RetainPtr<CFRunLoopSourceRef> configSource = adoptCF(SCDynamicStoreCreateRunLoopSource(0, m_store.get(), 0));
    if (!configSource)
        return;

    CFRunLoopAddSource(CFRunLoopGetMain(), configSource.get(), kCFRunLoopCommonModes);
    
    RetainPtr<CFMutableArrayRef> keys = adoptCF(CFArrayCreateMutable(0, 0, &kCFTypeArrayCallBacks));
    RetainPtr<CFMutableArrayRef> patterns = adoptCF(CFArrayCreateMutable(0, 0, &kCFTypeArrayCallBacks));

    RetainPtr<CFStringRef> key;
    RetainPtr<CFStringRef> pattern;

    key = adoptCF(SCDynamicStoreKeyCreateNetworkGlobalEntity(0, kSCDynamicStoreDomainState, kSCEntNetIPv4));
    CFArrayAppendValue(keys.get(), key.get());

    pattern = adoptCF(SCDynamicStoreKeyCreateNetworkInterfaceEntity(0, kSCDynamicStoreDomainState, kSCCompAnyRegex, kSCEntNetIPv4));
    CFArrayAppendValue(patterns.get(), pattern.get());

    key = adoptCF(SCDynamicStoreKeyCreateNetworkGlobalEntity(0, kSCDynamicStoreDomainState, kSCEntNetDNS));
    CFArrayAppendValue(keys.get(), key.get());

    SCDynamicStoreSetNotificationKeys(m_store.get(), keys.get(), patterns.get());
    
    updateState();
}
    
}
