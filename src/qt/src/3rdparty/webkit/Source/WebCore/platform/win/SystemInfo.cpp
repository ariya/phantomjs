/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
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
#include "SystemInfo.h"

#include <windows.h>
#include <wtf/text/StringConcatenate.h>

namespace WebCore {

WindowsVersion windowsVersion(int* major, int* minor)
{
    static bool initialized = false;
    static WindowsVersion version;
    static int majorVersion, minorVersion;

    if (!initialized) {
        initialized = true;
#if OS(WINCE)
        OSVERSIONINFO versionInfo;
#else
        OSVERSIONINFOEX versionInfo;
#endif
        ZeroMemory(&versionInfo, sizeof(versionInfo));
        versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
        GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&versionInfo));
        majorVersion = versionInfo.dwMajorVersion;
        minorVersion = versionInfo.dwMinorVersion;

#if OS(WINCE)
        if (majorVersion >= 1 && majorVersion <= 7)
            version = static_cast<WindowsVersion>(WindowsCE1 + (majorVersion - 1));
        else
            version = (majorVersion < 1) ? WindowsCE1 : WindowsCE7;
#else
        if (versionInfo.dwPlatformId == VER_PLATFORM_WIN32s)
            version = Windows3_1;
        else if (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
            if (!minorVersion)
                version = Windows95;
            else
                version = (minorVersion == 10) ? Windows98 : WindowsME;
        } else {
            if (majorVersion == 5) {
                if (!minorVersion)
                    version = Windows2000;
                else
                    version = (minorVersion == 1) ? WindowsXP : WindowsServer2003;
            } else if (majorVersion >= 6) {
                if (versionInfo.wProductType == VER_NT_WORKSTATION)
                    version = (majorVersion == 6 && !minorVersion) ? WindowsVista : Windows7;
                else
                    version = WindowsServer2008;
            } else
                version = (majorVersion == 4) ? WindowsNT4 : WindowsNT3;
        }
#endif
    }

    if (major)
        *major = majorVersion;
    if (minor)
        *minor = minorVersion;
    return version;
}

static String osVersionForUAString()
{
    int major, minor;
    WindowsVersion version = windowsVersion(&major, &minor);
    switch (version) {
    case WindowsCE1:
    case WindowsCE2:
    case WindowsCE3:
        return "Windows CE";
    case WindowsCE4:
        return "Windows CE .NET";
    case Windows3_1:
        return "Windows 3.1";
    case Windows95:
        return "Windows 95";
    case Windows98:
        return "Windows 98";
    case WindowsME:
        return "Windows 98; Win 9x 4.90";
    case WindowsNT4:
        return "WinNT4.0";
    }

    const char* familyName = (version >= WindowsNT3) ? "Windows NT " : "Windows CE ";
    return makeString(familyName, String::number(major), '.', String::number(minor));
}

#if !OS(WINCE)
static bool isWOW64()
{
    static bool initialized = false;
    static bool wow64 = false;

    if (!initialized) {
        initialized = true;
        HMODULE kernel32Module = GetModuleHandleA("kernel32.dll");
        if (!kernel32Module)
            return wow64;
        typedef BOOL (WINAPI* IsWow64ProcessFunc)(HANDLE, PBOOL);
        IsWow64ProcessFunc isWOW64Process = reinterpret_cast<IsWow64ProcessFunc>(GetProcAddress(kernel32Module, "IsWow64Process"));
        if (isWOW64Process) {
            BOOL result = FALSE;
            wow64 = isWOW64Process(GetCurrentProcess(), &result) && result;
        }
    }

    return wow64;
}

static WORD processorArchitecture()
{
    static bool initialized = false;
    static WORD architecture = PROCESSOR_ARCHITECTURE_INTEL;

    if (!initialized) {
        initialized = true;
        HMODULE kernel32Module = GetModuleHandleA("kernel32.dll");
        if (!kernel32Module)
            return architecture;
        typedef VOID (WINAPI* GetNativeSystemInfoFunc)(LPSYSTEM_INFO);
        GetNativeSystemInfoFunc getNativeSystemInfo = reinterpret_cast<GetNativeSystemInfoFunc>(GetProcAddress(kernel32Module, "GetNativeSystemInfo"));
        if (getNativeSystemInfo) {
            SYSTEM_INFO systemInfo;
            ZeroMemory(&systemInfo, sizeof(systemInfo));
            getNativeSystemInfo(&systemInfo);
            architecture = systemInfo.wProcessorArchitecture;
        }
    }

    return architecture;
}
#endif

static String architectureTokenForUAString()
{
#if !OS(WINCE)
    if (isWOW64())
        return "; WOW64";
    if (processorArchitecture() == PROCESSOR_ARCHITECTURE_AMD64)
        return "; Win64; x64";
    if (processorArchitecture() == PROCESSOR_ARCHITECTURE_IA64)
        return "; Win64; IA64";
#endif
    return String();
}

String windowsVersionForUAString()
{
    return makeString(osVersionForUAString(), architectureTokenForUAString());
}

} // namespace WebCore
