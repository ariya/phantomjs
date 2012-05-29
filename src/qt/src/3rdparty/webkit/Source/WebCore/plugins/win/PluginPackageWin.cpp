/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Collabora, Ltd.  All rights reserved.
 * Copyright (C) 2009 Torch Mobile, Inc.  All rights reserved.
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

#include "MIMETypeRegistry.h"
#include "PluginDatabase.h"
#include "PluginDebug.h"
#include "Timer.h"
#include "npruntime_impl.h"
#include <string.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/text/CString.h>
#include <shlwapi.h>

namespace WebCore {

static String getVersionInfo(const LPVOID versionInfoData, const String& info)
{
    LPVOID buffer;
    UINT bufferLength;
    String subInfo = "\\StringfileInfo\\040904E4\\" + info;
    bool retval = VerQueryValueW(versionInfoData,
        const_cast<UChar*>(subInfo.charactersWithNullTermination()),
        &buffer, &bufferLength);
    if (!retval || bufferLength == 0)
        return String();

    // Subtract 1 from the length; we don't want the trailing null character.
    return String(reinterpret_cast<UChar*>(buffer), bufferLength - 1);
}

bool PluginPackage::isPluginBlacklisted()
{
    if (name() == "Citrix ICA Client") {
        // The Citrix ICA Client plug-in requires a Mozilla-based browser; see <rdar://6418681>.
        return true;
    }

    if (name() == "Silverlight Plug-In") {
        // workaround for <rdar://5557379> Crash in Silverlight when opening microsoft.com.
        // the latest 1.0 version of Silverlight does not reproduce this crash, so allow it
        // and any newer versions
        static const PlatformModuleVersion slPluginMinRequired(0x51BE0000, 0x00010000);

        if (compareFileVersion(slPluginMinRequired) < 0)
            return true;
    } else if (fileName() == "npmozax.dll") {
        // Bug 15217: Mozilla ActiveX control complains about missing xpcom_core.dll
        return true;
    } else if (fileName() == "npwpf.dll") {
        // Bug 57119: Microsoft Windows Presentation Foundation (WPF) plug-in complains about missing xpcom.dll
        return true;
    } else if (name() == "Yahoo Application State Plugin") {
        // https://bugs.webkit.org/show_bug.cgi?id=26860
        // Bug in Yahoo Application State plug-in earlier than 1.0.0.6 leads to heap corruption. 
        static const PlatformModuleVersion yahooAppStatePluginMinRequired(0x00000006, 0x00010000);
        if (compareFileVersion(yahooAppStatePluginMinRequired) < 0)
            return true;
    }

    return false;
}

void PluginPackage::determineQuirks(const String& mimeType)
{
    if (mimeType == "application/x-shockwave-flash") {
        static const PlatformModuleVersion flashTenVersion(0x00000000, 0x000a0000);

        // Pre 10 Flash only requests windowless plugins if we return a mozilla user agent
        if (compareFileVersion(flashTenVersion) < 0)
            m_quirks.add(PluginQuirkWantsMozillaUserAgent);

        m_quirks.add(PluginQuirkThrottleInvalidate);
        m_quirks.add(PluginQuirkThrottleWMUserPlusOneMessages);
        m_quirks.add(PluginQuirkFlashURLNotifyBug);
    }

    if (name().contains("Microsoft") && name().contains("Windows Media")) {
        // The WMP plugin sets its size on the first NPP_SetWindow call and never updates its size, so
        // call SetWindow when the plugin view has a correct size
        m_quirks.add(PluginQuirkDeferFirstSetWindowCall);

        // Windowless mode does not work at all with the WMP plugin so just remove that parameter 
        // and don't pass it to the plug-in.
        m_quirks.add(PluginQuirkRemoveWindowlessVideoParam);

        // WMP has a modal message loop that it enters whenever we call it or
        // ask it to paint. This modal loop can deliver messages to other
        // windows in WebKit at times when they are not expecting them (for
        // example, delivering a WM_PAINT message during a layout), and these
        // can cause crashes.
        m_quirks.add(PluginQuirkHasModalMessageLoop);
    }

    if (name() == "VLC Multimedia Plugin" || name() == "VLC Multimedia Plug-in") {
        // VLC hangs on NPP_Destroy if we call NPP_SetWindow with a null window handle
        m_quirks.add(PluginQuirkDontSetNullWindowHandleOnDestroy);

        // VLC 0.8.6d and 0.8.6e crash if multiple instances are created.
        // <rdar://problem/5773070> tracks allowing multiple instances when this
        // bug is fixed.
        m_quirks.add(PluginQuirkDontAllowMultipleInstances);
    }

    // The DivX plugin sets its size on the first NPP_SetWindow call and never updates its size, so
    // call SetWindow when the plugin view has a correct size
    if (mimeType == "video/divx")
        m_quirks.add(PluginQuirkDeferFirstSetWindowCall);

    // FIXME: This is a workaround for a problem in our NPRuntime bindings; if a plug-in creates an
    // NPObject and passes it to a function it's not possible to see what root object that NPObject belongs to.
    // Thus, we don't know that the object should be invalidated when the plug-in instance goes away.
    // See <rdar://problem/5487742>.
    if (mimeType == "application/x-silverlight")
        m_quirks.add(PluginQuirkDontUnloadPlugin);

    if (MIMETypeRegistry::isJavaAppletMIMEType(mimeType)) {
        // Because a single process cannot create multiple VMs, and we cannot reliably unload a
        // Java VM, we cannot unload the Java plugin, or we'll lose reference to our only VM
        m_quirks.add(PluginQuirkDontUnloadPlugin);

        // Setting the window region to an empty region causes bad scrolling repaint problems
        // with the Java plug-in.
        m_quirks.add(PluginQuirkDontClipToZeroRectWhenScrolling);
    }

    if (mimeType == "audio/x-pn-realaudio-plugin") {
        // Prevent the Real plugin from calling the Window Proc recursively, causing the stack to overflow.
        m_quirks.add(PluginQuirkDontCallWndProcForSameMessageRecursively);

        static const PlatformModuleVersion lastKnownUnloadableRealPlayerVersion(0x000B0B24, 0x00060000);

        // Unloading RealPlayer versions newer than 10.5 can cause a hang; see rdar://5669317.
        // FIXME: Resume unloading when this bug in the RealPlayer Plug-In is fixed (rdar://5713147)
        if (compareFileVersion(lastKnownUnloadableRealPlayerVersion) > 0)
            m_quirks.add(PluginQuirkDontUnloadPlugin);
    }
}

bool PluginPackage::fetchInfo()
{
    DWORD versionInfoSize, zeroHandle;
    versionInfoSize = GetFileVersionInfoSizeW(const_cast<UChar*>(m_path.charactersWithNullTermination()), &zeroHandle);
    if (versionInfoSize == 0)
        return false;

    OwnArrayPtr<char> versionInfoData = adoptArrayPtr(new char[versionInfoSize]);

    if (!GetFileVersionInfoW(const_cast<UChar*>(m_path.charactersWithNullTermination()),
            0, versionInfoSize, versionInfoData.get()))
        return false;

    m_name = getVersionInfo(versionInfoData.get(), "ProductName");
    m_description = getVersionInfo(versionInfoData.get(), "FileDescription");
    if (m_name.isNull() || m_description.isNull())
        return false;

    VS_FIXEDFILEINFO* info;
    UINT infoSize;
    if (!VerQueryValueW(versionInfoData.get(), L"\\", (LPVOID*) &info, &infoSize) || infoSize < sizeof(VS_FIXEDFILEINFO))
        return false;
    m_moduleVersion.leastSig = info->dwFileVersionLS;
    m_moduleVersion.mostSig = info->dwFileVersionMS;

    if (isPluginBlacklisted())
        return false;

    Vector<String> types;
    getVersionInfo(versionInfoData.get(), "MIMEType").split('|', types);
    Vector<String> extensionLists;
    getVersionInfo(versionInfoData.get(), "FileExtents").split('|', extensionLists);
    Vector<String> descriptions;
    getVersionInfo(versionInfoData.get(), "FileOpenName").split('|', descriptions);

    for (unsigned i = 0; i < types.size(); i++) {
        String type = types[i].lower();
        String description = i < descriptions.size() ? descriptions[i] : "";
        String extensionList = i < extensionLists.size() ? extensionLists[i] : "";

        Vector<String> extensionsVector;
        extensionList.split(',', extensionsVector);

        // Get rid of the extension list that may be at the end of the description string.
        int pos = description.find("(*");
        if (pos != -1) {
            // There might be a space that we need to get rid of.
            if (pos > 1 && description[pos - 1] == ' ')
                pos--;
            description = description.left(pos);
        }

        // Determine the quirks for the MIME types this plug-in supports
        determineQuirks(type);

        m_mimeToExtensions.add(type, extensionsVector);
        m_mimeToDescriptions.add(type, description);
    }

    return true;
}

bool PluginPackage::load()
{
    if (m_freeLibraryTimer.isActive()) {
        ASSERT(m_module);
        m_freeLibraryTimer.stop();
    } else if (m_isLoaded) {
        if (m_quirks.contains(PluginQuirkDontAllowMultipleInstances))
            return false;
        m_loadCount++;
        return true;
    } else {
#if OS(WINCE)
        m_module = ::LoadLibraryW(m_path.charactersWithNullTermination());
#else
        WCHAR currentPath[MAX_PATH];

        if (!::GetCurrentDirectoryW(MAX_PATH, currentPath))
            return false;

        String path = m_path.substring(0, m_path.reverseFind('\\'));

        if (!::SetCurrentDirectoryW(path.charactersWithNullTermination()))
            return false;

        // Load the library
        m_module = ::LoadLibraryExW(m_path.charactersWithNullTermination(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);

        if (!::SetCurrentDirectoryW(currentPath)) {
            if (m_module)
                ::FreeLibrary(m_module);
            return false;
        }
#endif
    }

    if (!m_module)
        return false;

    m_isLoaded = true;

    NP_GetEntryPointsFuncPtr NP_GetEntryPoints = 0;
    NP_InitializeFuncPtr NP_Initialize = 0;
    NPError npErr;

#if OS(WINCE)
    NP_Initialize = (NP_InitializeFuncPtr)GetProcAddress(m_module, L"NP_Initialize");
    NP_GetEntryPoints = (NP_GetEntryPointsFuncPtr)GetProcAddress(m_module, L"NP_GetEntryPoints");
    m_NPP_Shutdown = (NPP_ShutdownProcPtr)GetProcAddress(m_module, L"NP_Shutdown");
#else
    NP_Initialize = (NP_InitializeFuncPtr)GetProcAddress(m_module, "NP_Initialize");
    NP_GetEntryPoints = (NP_GetEntryPointsFuncPtr)GetProcAddress(m_module, "NP_GetEntryPoints");
    m_NPP_Shutdown = (NPP_ShutdownProcPtr)GetProcAddress(m_module, "NP_Shutdown");
#endif

    if (!NP_Initialize || !NP_GetEntryPoints || !m_NPP_Shutdown)
        goto abort;

    memset(&m_pluginFuncs, 0, sizeof(m_pluginFuncs));
    m_pluginFuncs.size = sizeof(m_pluginFuncs);

    npErr = NP_GetEntryPoints(&m_pluginFuncs);
    LOG_NPERROR(npErr);
    if (npErr != NPERR_NO_ERROR)
        goto abort;

    initializeBrowserFuncs();

    npErr = NP_Initialize(&m_browserFuncs);
    LOG_NPERROR(npErr);

    if (npErr != NPERR_NO_ERROR)
        goto abort;

    m_loadCount++;
    return true;

abort:
    unloadWithoutShutdown();
    return false;
}

unsigned PluginPackage::hash() const
{ 
    const unsigned hashCodes[] = {
        m_name.impl()->hash(),
        m_description.impl()->hash(),
        m_mimeToExtensions.size()
    };

    return StringHasher::hashMemory<sizeof(hashCodes)>(hashCodes);
}

bool PluginPackage::equal(const PluginPackage& a, const PluginPackage& b)
{
    if (a.m_name != b.m_name)
        return false;

    if (a.m_description != b.m_description)
        return false;

    if (a.m_mimeToExtensions.size() != b.m_mimeToExtensions.size())
        return false;

    MIMEToExtensionsMap::const_iterator::Keys end = a.m_mimeToExtensions.end().keys();
    for (MIMEToExtensionsMap::const_iterator::Keys it = a.m_mimeToExtensions.begin().keys(); it != end; ++it) {
        if (!b.m_mimeToExtensions.contains(*it))
            return false;
    }

    return true;
}

uint16_t PluginPackage::NPVersion() const
{
    return NP_VERSION_MINOR;
}
}
