/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
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
#include "WebKitDLL.h"

#include "ForEachCoClass.h"
#include "resource.h"
#include "WebKit.h"
#include "WebKitClassFactory.h"
#include <WebCore/COMPtr.h>
#include <WebCore/IconDatabase.h>
#include <WebCore/Page.h>
#include <WebCore/PageGroup.h>
#include <WebCore/RenderThemeWin.h>
#include <WebCore/SharedBuffer.h>
#include <WebCore/WebCoreInstanceHandle.h>
#include <WebCore/Widget.h>
#include <olectl.h>
#include <wchar.h>
#include <wtf/Vector.h>

using namespace WebCore;

ULONG gLockCount;
ULONG gClassCount;
HashCountedSet<String> gClassNameCount;
HINSTANCE gInstance;

#define CLSID_FOR_CLASS(cls) CLSID_##cls,
CLSID gRegCLSIDs[] = {
    FOR_EACH_COCLASS(CLSID_FOR_CLASS)
};
#undef CLSID_FOR_CLASS

STDAPI_(BOOL) DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID /*lpReserved*/)
{
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            gLockCount = gClassCount = 0;
            gInstance = hModule;
            WebCore::setInstanceHandle(hModule);
            return TRUE;

        case DLL_PROCESS_DETACH:
            WebCore::RenderThemeWin::setWebKitIsBeingUnloaded();
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return FALSE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    bool found = false;
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(gRegCLSIDs); ++i) {
        if (IsEqualGUID(rclsid, gRegCLSIDs[i])) {
            found = true;
            break;
        }
    }
    if (!found)
        return E_FAIL;

    if (!IsEqualGUID(riid, IID_IUnknown) && !IsEqualGUID(riid, IID_IClassFactory))
        return E_NOINTERFACE;

    WebKitClassFactory* factory = new WebKitClassFactory(rclsid);
    *ppv = reinterpret_cast<LPVOID>(factory);
    if (!factory)
        return E_OUTOFMEMORY;

    factory->AddRef();
    return S_OK;
}

STDAPI DllCanUnloadNow(void)
{
    if (!gClassCount && !gLockCount)
        return S_OK;
    
    return S_FALSE;
}

// deprecated - do not use
STDAPI DllUnregisterServer(void)
{
    return 0;
}

// deprecated - do not use
STDAPI DllRegisterServer(void)
{
    return 0;
}

// deprecated - do not use
STDAPI RunAsLocalServer()
{
    return 0;
}

// deprecated - do not use
STDAPI LocalServerDidDie()
{
    return 0;
}

void shutDownWebKit()
{
    WebCore::iconDatabase().close();
    WebCore::PageGroup::closeLocalStorage();
}

//FIXME: We should consider moving this to a new file for cross-project functionality
PassRefPtr<WebCore::SharedBuffer> loadResourceIntoBuffer(const char* name)
{
    int idr;
    // temporary hack to get resource id
    if (!strcmp(name, "textAreaResizeCorner"))
        idr = IDR_RESIZE_CORNER;
    else if (!strcmp(name, "missingImage"))
        idr = IDR_MISSING_IMAGE;
    else if (!strcmp(name, "nullPlugin"))
        idr = IDR_NULL_PLUGIN;
    else if (!strcmp(name, "panIcon"))
        idr = IDR_PAN_SCROLL_ICON;
    else if (!strcmp(name, "panSouthCursor"))
        idr = IDR_PAN_SOUTH_CURSOR;
    else if (!strcmp(name, "panNorthCursor"))
        idr = IDR_PAN_NORTH_CURSOR;
    else if (!strcmp(name, "panEastCursor"))
        idr = IDR_PAN_EAST_CURSOR;
    else if (!strcmp(name, "panWestCursor"))
        idr = IDR_PAN_WEST_CURSOR;
    else if (!strcmp(name, "panSouthEastCursor"))
        idr = IDR_PAN_SOUTH_EAST_CURSOR;
    else if (!strcmp(name, "panSouthWestCursor"))
        idr = IDR_PAN_SOUTH_WEST_CURSOR;
    else if (!strcmp(name, "panNorthEastCursor"))
        idr = IDR_PAN_NORTH_EAST_CURSOR;
    else if (!strcmp(name, "panNorthWestCursor"))
        idr = IDR_PAN_NORTH_WEST_CURSOR;
    else if (!strcmp(name, "searchMagnifier"))
        idr = IDR_SEARCH_MAGNIFIER;
    else if (!strcmp(name, "searchMagnifierResults"))
        idr = IDR_SEARCH_MAGNIFIER_RESULTS;
    else if (!strcmp(name, "searchCancel"))
        idr = IDR_SEARCH_CANCEL;
    else if (!strcmp(name, "searchCancelPressed"))
        idr = IDR_SEARCH_CANCEL_PRESSED;
    else if (!strcmp(name, "zoomInCursor"))
        idr = IDR_ZOOM_IN_CURSOR;
    else if (!strcmp(name, "zoomOutCursor"))
        idr = IDR_ZOOM_OUT_CURSOR;
    else if (!strcmp(name, "verticalTextCursor"))
        idr = IDR_VERTICAL_TEXT_CURSOR;
    else if (!strcmp(name, "fsVideoAudioVolumeHigh"))
        idr = IDR_FS_VIDEO_AUDIO_VOLUME_HIGH;
    else if (!strcmp(name, "fsVideoAudioVolumeLow"))
        idr = IDR_FS_VIDEO_AUDIO_VOLUME_LOW;
    else if (!strcmp(name, "fsVideoExitFullscreen"))
        idr = IDR_FS_VIDEO_EXIT_FULLSCREEN;
    else if (!strcmp(name, "fsVideoPause"))
        idr = IDR_FS_VIDEO_PAUSE;
    else if (!strcmp(name, "fsVideoPlay"))
        idr = IDR_FS_VIDEO_PLAY;
    else
        return 0;

    HRSRC resInfo = FindResource(gInstance, MAKEINTRESOURCE(idr), L"PNG");
    if (!resInfo)
        return 0;
    HANDLE res = LoadResource(gInstance, resInfo);
    if (!res)
        return 0;
    void* resource = LockResource(res);
    if (!resource)
        return 0;
    int size = SizeofResource(gInstance, resInfo);

    return WebCore::SharedBuffer::create(reinterpret_cast<const char*>(resource), size);
}
