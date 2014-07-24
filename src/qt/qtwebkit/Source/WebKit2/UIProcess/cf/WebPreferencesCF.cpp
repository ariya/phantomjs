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

#include "config.h"
#include "WebPreferences.h"

#include <wtf/RetainPtr.h>
#include <wtf/text/StringConcatenate.h>

#if !PLATFORM(MAC)

namespace WebKit {

static RetainPtr<CFStringRef> cfStringFromWebCoreString(const String& string)
{
    return RetainPtr<CFStringRef> = adoptCF(CFStringCreateWithCharacters(0, reinterpret_cast<const UniChar*>(string.characters()), string.length()));
}

static inline RetainPtr<CFStringRef> makeKey(const String& identifier, const String& baseKey)
{
    return cfStringFromWebCoreString(makeString(identifier, ".WebKit2", baseKey));
}

static void setStringValueIfInUserDefaults(const String& identifier, const String& baseKey, WebPreferencesStore& store)
{
    RetainPtr<CFPropertyListRef> value = adoptCF(CFPreferencesCopyAppValue(makeKey(identifier, baseKey).get(), kCFPreferencesCurrentApplication));
    if (!value)
        return;
    if (CFGetTypeID(value.get()) != CFStringGetTypeID())
        return;

    store.setStringValueForKey(baseKey, (CFStringRef)value.get());
}

static void setBoolValueIfInUserDefaults(const String& identifier, const String& baseKey, WebPreferencesStore& store)
{
    RetainPtr<CFPropertyListRef> value = adoptCF(CFPreferencesCopyAppValue(makeKey(identifier, baseKey).get(), kCFPreferencesCurrentApplication));
    if (!value)
        return;
    if (CFGetTypeID(value.get()) != CFBooleanGetTypeID())
        return;

    store.setBoolValueForKey(baseKey, CFBooleanGetValue((CFBooleanRef)value.get()) ? true : false);
}

static void setUInt32ValueIfInUserDefaults(const String& identifier, const String& baseKey, WebPreferencesStore& store)
{
    RetainPtr<CFPropertyListRef> value = adoptCF(CFPreferencesCopyAppValue(makeKey(identifier, baseKey).get(), kCFPreferencesCurrentApplication));
    if (!value)
        return;
    if (CFGetTypeID(value.get()) != CFNumberGetTypeID())
        return;
    int32_t intValue = 0;
    if (!CFNumberGetValue((CFNumberRef)value.get(), kCFNumberSInt32Type, &intValue))
        return;

    store.setUInt32ValueForKey(baseKey, intValue);
}

static void setDoubleValueIfInUserDefaults(const String& identifier, const String& baseKey, WebPreferencesStore& store)
{
    RetainPtr<CFPropertyListRef> value = adoptCF(CFPreferencesCopyAppValue(makeKey(identifier, baseKey).get(), kCFPreferencesCurrentApplication));
    if (!value)
        return;
    if (CFGetTypeID(value.get()) != CFNumberGetTypeID())
        return;
    double doubleValue = 0;
    if (!CFNumberGetValue((CFNumberRef)value.get(), kCFNumberDoubleType, &doubleValue))
        return;

    store.setDoubleValueForKey(baseKey, doubleValue);
}

void WebPreferences::platformInitializeStore()
{
    if (!m_identifier)
        return;

#define INITIALIZE_PREFERENCE_FROM_NSUSERDEFAULTS(KeyUpper, KeyLower, TypeName, Type, DefaultValue) \
    set##TypeName##ValueIfInUserDefaults(m_identifier, WebPreferencesKey::KeyLower##Key(), m_store);

    FOR_EACH_WEBKIT_PREFERENCE(INITIALIZE_PREFERENCE_FROM_NSUSERDEFAULTS)

#undef INITIALIZE_PREFERENCE_FROM_NSUSERDEFAULTS

}

void WebPreferences::platformUpdateStringValueForKey(const String& key, const String& value)
{
    if (!m_identifier)
        return;

    CFPreferencesSetAppValue(makeKey(m_identifier, key).get(), cfStringFromWebCoreString(value).get(), kCFPreferencesCurrentApplication);
}

void WebPreferences::platformUpdateBoolValueForKey(const String& key, bool value)
{
    if (!m_identifier)
        return;

    CFPreferencesSetAppValue(makeKey(m_identifier, key).get(), value ? kCFBooleanTrue : kCFBooleanFalse, kCFPreferencesCurrentApplication);
}

void WebPreferences::platformUpdateUInt32ValueForKey(const String& key, uint32_t value)
{
    if (!m_identifier)
        return;

    RetainPtr<CFNumberRef> number = adoptCF(CFNumberCreate(0, kCFNumberSInt32Type, &value));
    CFPreferencesSetAppValue(makeKey(m_identifier, key).get(), number.get(), kCFPreferencesCurrentApplication);
}

void WebPreferences::platformUpdateDoubleValueForKey(const String& key, double value)
{
    if (!m_identifier)
        return;

    RetainPtr<CFNumberRef> number = adoptCF(CFNumberCreate(0, kCFNumberDoubleType, &value));
    CFPreferencesSetAppValue(makeKey(m_identifier, key).get(), number.get(), kCFPreferencesCurrentApplication);
}

void WebPreferences::platformUpdateFloatValueForKey(const String& key, float value)
{
    if (!m_identifier)
        return;

    RetainPtr<CFNumberRef> number = adoptCF(CFNumberCreate(0, kCFNumberFloatType, &value));
    CFPreferencesSetAppValue(makeKey(m_identifier, key).get(), number.get(), kCFPreferencesCurrentApplication);
}

} // namespace WebKit

#endif // !PLATFORM(MAC)
