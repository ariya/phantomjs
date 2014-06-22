/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#import "config.h"
#import "WebPreferences.h"

#import "StringUtilities.h"
#import <wtf/text/StringConcatenate.h>

namespace WebKit {

static inline NSString* makeKey(const String& identifier, const String& baseKey)
{
    return nsStringFromWebCoreString(makeString(identifier, ".WebKit2", baseKey));
}

static void setStringValueIfInUserDefaults(const String& identifier, const String& key, WebPreferencesStore& store)
{
    id object = [[NSUserDefaults standardUserDefaults] objectForKey:makeKey(identifier, key)];
    if (!object)
        return;
    if (![object isKindOfClass:[NSString class]])
        return;

    store.setStringValueForKey(key, (NSString *)object);
}

static void setBoolValueIfInUserDefaults(const String& identifier, const String& key, WebPreferencesStore& store)
{
    id object = [[NSUserDefaults standardUserDefaults] objectForKey:makeKey(identifier, key)];
    if (!object)
        return;
    if (![object respondsToSelector:@selector(boolValue)])
        return;

    store.setBoolValueForKey(key, [object boolValue]);
}

static void setUInt32ValueIfInUserDefaults(const String& identifier, const String& key, WebPreferencesStore& store)
{
    id object = [[NSUserDefaults standardUserDefaults] objectForKey:makeKey(identifier, key)];
    if (!object)
        return;
    if (![object respondsToSelector:@selector(intValue)])
        return;

    store.setUInt32ValueForKey(key, [object intValue]);
}

static void setDoubleValueIfInUserDefaults(const String& identifier, const String& key, WebPreferencesStore& store)
{
    id object = [[NSUserDefaults standardUserDefaults] objectForKey:makeKey(identifier, key)];
    if (!object)
        return;
    if (![object respondsToSelector:@selector(doubleValue)])
        return;

    store.setDoubleValueForKey(key, [object doubleValue]);
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

    [[NSUserDefaults standardUserDefaults] setObject:nsStringFromWebCoreString(value) forKey:makeKey(m_identifier, key)];
}

void WebPreferences::platformUpdateBoolValueForKey(const String& key, bool value)
{
    if (!m_identifier)
        return;

    [[NSUserDefaults standardUserDefaults] setBool:value forKey:makeKey(m_identifier, key)];
}

void WebPreferences::platformUpdateUInt32ValueForKey(const String& key, uint32_t value)
{
    if (!m_identifier)
        return;

    [[NSUserDefaults standardUserDefaults] setInteger:value forKey:makeKey(m_identifier, key)];
}

void WebPreferences::platformUpdateDoubleValueForKey(const String& key, double value)
{
    if (!m_identifier)
        return;

    [[NSUserDefaults standardUserDefaults] setDouble:value forKey:makeKey(m_identifier, key)];
}

void WebPreferences::platformUpdateFloatValueForKey(const String& key, float value)
{
    if (!m_identifier)
        return;

    [[NSUserDefaults standardUserDefaults] setFloat:value forKey:makeKey(m_identifier, key)];
}

} // namespace WebKit
