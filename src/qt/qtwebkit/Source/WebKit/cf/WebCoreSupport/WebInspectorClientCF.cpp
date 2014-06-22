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

// FIXME: On Windows, we require all WebKit source files to include config.h
// before including any other files. Failing to include config.h will leave
// WTF_USE_CF undefined, causing build failures in this 
// file. But Mac doesn't have a config.h for WebKit, so we can't include the 
// Windows one here. For now we can just define WTF_USE_CF and
// WTF_USE_CFNETWORK manually, but we need a better long-term solution.
#ifndef WTF_USE_CF
#define WTF_USE_CF 1
#endif

#include <wtf/Platform.h>

#if PLATFORM(WIN) && !OS(WINCE)
#ifndef WTF_USE_CG
#define WTF_USE_CG 1
#endif
#endif

// NOTE: These need to appear up top, as they declare macros
// used in the JS and WTF headers.
#include <runtime/JSExportMacros.h>
#include <wtf/ExportMacros.h>

#include "WebInspectorClient.h"

#include <CoreFoundation/CoreFoundation.h>

#include <WebCore/Frame.h>
#include <WebCore/InspectorFrontendClientLocal.h>
#include <WebCore/Page.h>

#include <wtf/PassOwnPtr.h>
#include <wtf/RetainPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

static const char* inspectorStartsAttachedSetting = "inspectorStartsAttached";
static const char* inspectorAttachDisabledSetting = "inspectorAttachDisabled";

static inline RetainPtr<CFStringRef> createKeyForPreferences(const String& key)
{
    return adoptCF(CFStringCreateWithFormat(0, 0, CFSTR("WebKit Web Inspector Setting - %@"), key.createCFString().get()));
}

static void populateSetting(const String& key, String* setting)
{
    RetainPtr<CFStringRef> preferencesKey = createKeyForPreferences(key);
    RetainPtr<CFPropertyListRef> value = adoptCF(CFPreferencesCopyAppValue(preferencesKey.get(), kCFPreferencesCurrentApplication));

    if (!value)
        return;

    CFTypeID type = CFGetTypeID(value.get());
    if (type == CFStringGetTypeID())
        *setting = static_cast<String>(static_cast<CFStringRef>(value.get()));
    else if (type == CFBooleanGetTypeID())
        *setting = static_cast<bool>(CFBooleanGetValue(static_cast<CFBooleanRef>(value.get()))) ? "true" : "false";
    else
        *setting = "";
}

static void storeSetting(const String& key, const String& setting)
{
    CFPreferencesSetAppValue(createKeyForPreferences(key).get(), setting.createCFString().get(), kCFPreferencesCurrentApplication);
}

bool WebInspectorClient::sendMessageToFrontend(const String& message)
{
    return doDispatchMessageOnFrontendPage(m_frontendPage, message);
}

bool WebInspectorClient::inspectorAttachDisabled()
{
    String value;
    populateSetting(inspectorAttachDisabledSetting, &value);
    if (value.isEmpty())
        return false;
    return value == "true";
}

void WebInspectorClient::setInspectorAttachDisabled(bool disabled)
{
    storeSetting(inspectorAttachDisabledSetting, disabled ? "true" : "false");
}

bool WebInspectorClient::inspectorStartsAttached()
{
    String value;
    populateSetting(inspectorStartsAttachedSetting, &value);
    if (value.isEmpty())
        return true;
    return value == "true";
}

void WebInspectorClient::setInspectorStartsAttached(bool attached)
{
    storeSetting(inspectorStartsAttachedSetting, attached ? "true" : "false");
}

PassOwnPtr<WebCore::InspectorFrontendClientLocal::Settings> WebInspectorClient::createFrontendSettings()
{
    class InspectorFrontendSettingsCF : public WebCore::InspectorFrontendClientLocal::Settings {
    public:
        virtual ~InspectorFrontendSettingsCF() { }
        virtual String getProperty(const String& name)
        {
            String value;
            populateSetting(name, &value);
            return value;
        }

        virtual void setProperty(const String& name, const String& value)
        {
            storeSetting(name, value);
        }
    };
    return adoptPtr<WebCore::InspectorFrontendClientLocal::Settings>(new InspectorFrontendSettingsCF());
}
