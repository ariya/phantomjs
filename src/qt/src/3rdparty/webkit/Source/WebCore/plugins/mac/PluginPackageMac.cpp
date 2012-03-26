/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Collabora Ltd. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PluginPackage.h"

#include <wtf/RetainPtr.h>
#include "MIMETypeRegistry.h"
#include "npruntime_impl.h"
#include "PluginDatabase.h"
#include "PluginDebug.h"
#include "WebCoreNSStringExtras.h"
#include <wtf/text/CString.h>

#include <CoreFoundation/CoreFoundation.h>

#define PluginNameOrDescriptionStringNumber     126
#define MIMEDescriptionStringNumber             127
#define MIMEListStringStringNumber              128

namespace WebCore {

void PluginPackage::determineQuirks(const String& mimeType)
{
    if (MIMETypeRegistry::isJavaAppletMIMEType(mimeType)) {
        // Because a single process cannot create multiple VMs, and we cannot reliably unload a
        // Java VM, we cannot unload the Java Plugin, or we'll lose reference to our only VM
        m_quirks.add(PluginQuirkDontUnloadPlugin);

        // Setting the window region to an empty region causes bad scrolling repaint problems
        // with the Java plug-in.
        m_quirks.add(PluginQuirkDontClipToZeroRectWhenScrolling);
    }

    if (mimeType == "application/x-shockwave-flash") {
        // The flash plugin only requests windowless plugins if we return a mozilla user agent
        m_quirks.add(PluginQuirkWantsMozillaUserAgent);
        m_quirks.add(PluginQuirkThrottleInvalidate);
        m_quirks.add(PluginQuirkThrottleWMUserPlusOneMessages);
        m_quirks.add(PluginQuirkFlashURLNotifyBug);
    }

}

typedef void (*BP_CreatePluginMIMETypesPreferencesFuncPtr)(void);

static WTF::RetainPtr<CFDictionaryRef> readPListFile(CFStringRef fileName, bool createFile, CFBundleRef bundle)
{
    if (createFile) {
        BP_CreatePluginMIMETypesPreferencesFuncPtr funcPtr =
            (BP_CreatePluginMIMETypesPreferencesFuncPtr)CFBundleGetFunctionPointerForName(bundle, CFSTR("BP_CreatePluginMIMETypesPreferences"));
        if (funcPtr)
            funcPtr();
    }

    WTF::RetainPtr<CFDictionaryRef> map;
    WTF::RetainPtr<CFURLRef> url =
        CFURLCreateWithFileSystemPath(kCFAllocatorDefault, fileName, kCFURLPOSIXPathStyle, false);

    CFDataRef resource = 0;
    SInt32 code;
    if (!CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, url.get(), &resource, 0, 0, &code))
        return map;

    WTF::RetainPtr<CFPropertyListRef> propertyList =
            CFPropertyListCreateFromXMLData(kCFAllocatorDefault, resource, kCFPropertyListImmutable, 0);

    CFRelease(resource);

    if (!propertyList)
        return map;

    if (CFGetTypeID(propertyList.get()) != CFDictionaryGetTypeID())
        return map;

    map = static_cast<CFDictionaryRef>(static_cast<CFPropertyListRef>(propertyList.get()));
    return map;
}

static Vector<String> stringListFromResourceId(SInt16 id)
{
    Vector<String> list;

    Handle handle = Get1Resource('STR#', id);
    if (!handle)
        return list;

    CFStringEncoding encoding = stringEncodingForResource(handle);

    unsigned char* p = (unsigned char*)*handle;
    if (!p)
        return list;

    SInt16 count = *(SInt16*)p;
    p += sizeof(SInt16);

    for (SInt16 i = 0; i < count; ++i) {
        unsigned char length = *p;
        WTF::RetainPtr<CFStringRef> str = CFStringCreateWithPascalString(0, p, encoding);
        list.append(str.get());
        p += 1 + length;
    }

    return list;
}

bool PluginPackage::fetchInfo()
{
    if (!load())
        return false;

    WTF::RetainPtr<CFDictionaryRef> mimeDict;

    WTF::RetainPtr<CFTypeRef> mimeTypesFileName = CFBundleGetValueForInfoDictionaryKey(m_module, CFSTR("WebPluginMIMETypesFilename"));
    if (mimeTypesFileName && CFGetTypeID(mimeTypesFileName.get()) == CFStringGetTypeID()) {

        WTF::RetainPtr<CFStringRef> fileName = (CFStringRef)mimeTypesFileName.get();
        WTF::RetainPtr<CFStringRef> homeDir = homeDirectoryPath().createCFString();
        WTF::RetainPtr<CFStringRef> path = CFStringCreateWithFormat(0, 0, CFSTR("%@/Library/Preferences/%@"), homeDir.get(), fileName.get());

        WTF::RetainPtr<CFDictionaryRef> plist = readPListFile(path.get(), /*createFile*/ false, m_module);
        if (plist) {
            // If the plist isn't localized, have the plug-in recreate it in the preferred language.
            WTF::RetainPtr<CFStringRef> localizationName =
                (CFStringRef)CFDictionaryGetValue(plist.get(), CFSTR("WebPluginLocalizationName"));
            CFLocaleRef locale = CFLocaleCopyCurrent();
            if (localizationName != CFLocaleGetIdentifier(locale))
                plist = readPListFile(path.get(), /*createFile*/ true, m_module);

            CFRelease(locale);
        } else {
            // Plist doesn't exist, ask the plug-in to create it.
            plist = readPListFile(path.get(), /*createFile*/ true, m_module);
        }

        if (plist)
            mimeDict = (CFDictionaryRef)CFDictionaryGetValue(plist.get(), CFSTR("WebPluginMIMETypes"));
    }

    if (!mimeDict)
        mimeDict = (CFDictionaryRef)CFBundleGetValueForInfoDictionaryKey(m_module, CFSTR("WebPluginMIMETypes"));

    if (mimeDict) {
        CFIndex propCount = CFDictionaryGetCount(mimeDict.get());
        Vector<const void*, 128> keys(propCount);
        Vector<const void*, 128> values(propCount);
        CFDictionaryGetKeysAndValues(mimeDict.get(), keys.data(), values.data());
        for (int i = 0; i < propCount; ++i) {
            String mimeType = (CFStringRef)keys[i];
            mimeType = mimeType.lower();

            WTF::RetainPtr<CFDictionaryRef> extensionsDict = (CFDictionaryRef)values[i];

            WTF::RetainPtr<CFNumberRef> enabled = (CFNumberRef)CFDictionaryGetValue(extensionsDict.get(), CFSTR("WebPluginTypeEnabled"));
            if (enabled) {
                int enabledValue = 0;
                if (CFNumberGetValue(enabled.get(), kCFNumberIntType, &enabledValue) && enabledValue == 0)
                    continue;
            }

            Vector<String> mimeExtensions;
            WTF::RetainPtr<CFArrayRef> extensions = (CFArrayRef)CFDictionaryGetValue(extensionsDict.get(), CFSTR("WebPluginExtensions"));
            if (extensions) {
                CFIndex extensionCount = CFArrayGetCount(extensions.get());
                for (CFIndex i = 0; i < extensionCount; ++i) {
                    String extension =(CFStringRef)CFArrayGetValueAtIndex(extensions.get(), i);
                    extension = extension.lower();
                    mimeExtensions.append(extension);
                }
            }
            m_mimeToExtensions.set(mimeType, mimeExtensions);

            String description = (CFStringRef)CFDictionaryGetValue(extensionsDict.get(), CFSTR("WebPluginTypeDescription"));
            m_mimeToDescriptions.set(mimeType, description);
        }

        m_name = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(m_module, CFSTR("WebPluginName"));
        m_description = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(m_module, CFSTR("WebPluginDescription"));

    } else {
        int resFile = CFBundleOpenBundleResourceMap(m_module);

        UseResFile(resFile);

        Vector<String> mimes = stringListFromResourceId(MIMEListStringStringNumber);

        if (mimes.size() % 2 != 0)
            return false;

        Vector<String> descriptions = stringListFromResourceId(MIMEDescriptionStringNumber);
        if (descriptions.size() != mimes.size() / 2)
            return false;

        for (size_t i = 0;  i < mimes.size(); i += 2) {
            String mime = mimes[i].lower();
            Vector<String> extensions;
            mimes[i + 1].lower().split(UChar(','), extensions);

            m_mimeToExtensions.set(mime, extensions);

            m_mimeToDescriptions.set(mime, descriptions[i / 2]);
        }

        Vector<String> names = stringListFromResourceId(PluginNameOrDescriptionStringNumber);
        if (names.size() == 2) {
            m_description = names[0];
            m_name = names[1];
        }

        CFBundleCloseBundleResourceMap(m_module, resFile);
    }

    LOG(Plugins, "PluginPackage::fetchInfo(): Found plug-in '%s'", m_name.utf8().data());
    if (isPluginBlacklisted()) {
        LOG(Plugins, "\tPlug-in is blacklisted!");
        return false;
    }

    return true;
}

bool PluginPackage::isPluginBlacklisted()
{
    return false;
}

bool PluginPackage::load()
{
    if (m_isLoaded) {
        m_loadCount++;
        return true;
    }

    WTF::RetainPtr<CFStringRef> path(AdoptCF, m_path.createCFString());
    WTF::RetainPtr<CFURLRef> url(AdoptCF, CFURLCreateWithFileSystemPath(kCFAllocatorDefault, path.get(),
                                                                        kCFURLPOSIXPathStyle, false));
    m_module = CFBundleCreate(NULL, url.get());
    if (!m_module || !CFBundleLoadExecutable(m_module)) {
        LOG(Plugins, "%s not loaded", m_path.utf8().data());
        return false;
    }

    m_isLoaded = true;

    NP_GetEntryPointsFuncPtr NP_GetEntryPoints = 0;
    NP_InitializeFuncPtr NP_Initialize;
    NPError npErr;

    NP_Initialize = (NP_InitializeFuncPtr)CFBundleGetFunctionPointerForName(m_module, CFSTR("NP_Initialize"));
    NP_GetEntryPoints = (NP_GetEntryPointsFuncPtr)CFBundleGetFunctionPointerForName(m_module, CFSTR("NP_GetEntryPoints"));
    m_NPP_Shutdown = (NPP_ShutdownProcPtr)CFBundleGetFunctionPointerForName(m_module, CFSTR("NP_Shutdown"));

    if (!NP_Initialize || !NP_GetEntryPoints || !m_NPP_Shutdown)
        goto abort;

    memset(&m_pluginFuncs, 0, sizeof(m_pluginFuncs));
    m_pluginFuncs.size = sizeof(m_pluginFuncs);

    initializeBrowserFuncs();

    npErr = NP_Initialize(&m_browserFuncs);
    LOG_NPERROR(npErr);
    if (npErr != NPERR_NO_ERROR)
        goto abort;

    npErr = NP_GetEntryPoints(&m_pluginFuncs);
    LOG_NPERROR(npErr);
    if (npErr != NPERR_NO_ERROR)
        goto abort;

    m_loadCount++;
    return true;

abort:
    unloadWithoutShutdown();
    return false;
}

uint16_t PluginPackage::NPVersion() const
{
    return NP_VERSION_MINOR;
}
} // namespace WebCore
