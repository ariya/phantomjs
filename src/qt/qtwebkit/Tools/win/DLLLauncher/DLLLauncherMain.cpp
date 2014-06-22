/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

// This file contains code for a launcher executable for WebKit apps. When compiled into foo.exe, it
// will set PATH so that Apple Application Support DLLs can be found, then will load foo.dll and
// call its dllLauncherEntryPoint function, which should be declared like so:
//     extern "C" __declspec(dllexport) int WINAPI dllLauncherEntryPoint(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpstCmdLine, int nCmdShow);
// If USE_CONSOLE_ENTRY_POINT is defined, this function will be called instead:
//     extern "C" __declspec(dllexport) int WINAPI dllLauncherEntryPoint(int argc, const char* argv[]);

#include <shlwapi.h>
#include <string>
#include <vector>
#include <windows.h>

using namespace std;

#if defined _M_IX86
#define PROCESSORARCHITECTURE "x86"
#elif defined _M_IA64
#define PROCESSORARCHITECTURE "ia64"
#elif defined _M_X64
#define PROCESSORARCHITECTURE "amd64"
#else
#define PROCESSORARCHITECTURE "*"
#endif

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='" PROCESSORARCHITECTURE "' publicKeyToken='6595b64144ccf1df' language='*'\"")
#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.VC80.CRT' version='8.0.50727.6195' processorArchitecture='" PROCESSORARCHITECTURE "' publicKeyToken='1fc8b3b9a1e18e3b' language='*'\"")
#endif

static void enableTerminationOnHeapCorruption()
{
    // Enable termination on heap corruption on OSes that support it (Vista and XPSP3).
    // http://msdn.microsoft.com/en-us/library/aa366705(VS.85).aspx

    HEAP_INFORMATION_CLASS heapEnableTerminationOnCorruption = static_cast<HEAP_INFORMATION_CLASS>(1);

    HMODULE module = ::GetModuleHandleW(L"kernel32.dll");
    if (!module)
        return;

    typedef BOOL (WINAPI*HSI)(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);
    HSI heapSetInformation = reinterpret_cast<HSI>(::GetProcAddress(module, "HeapSetInformation"));
    if (!heapSetInformation)
        return;

    heapSetInformation(0, heapEnableTerminationOnCorruption, 0, 0);
}

static wstring getStringValue(HKEY key, const wstring& valueName)
{
    DWORD type = 0;
    DWORD bufferSize = 0;
    if (::RegQueryValueExW(key, valueName.c_str(), 0, &type, 0, &bufferSize) != ERROR_SUCCESS || type != REG_SZ)
        return wstring();

    vector<wchar_t> buffer(bufferSize / sizeof(wchar_t));
    if (::RegQueryValueExW(key, valueName.c_str(), 0, &type, reinterpret_cast<LPBYTE>(&buffer[0]), &bufferSize) != ERROR_SUCCESS)
        return wstring();

    return &buffer[0];
}

static wstring applePathFromRegistry(const wstring& key, const wstring& value)
{
    HKEY applePathKey = 0;
    if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_READ, &applePathKey) != ERROR_SUCCESS)
        return wstring();
    wstring path = getStringValue(applePathKey, value);
    ::RegCloseKey(applePathKey);
    return path;
}

static wstring appleApplicationSupportDirectory()
{
    return applePathFromRegistry(L"SOFTWARE\\Apple Inc.\\Apple Application Support", L"InstallDir");
}

static wstring copyEnvironmentVariable(const wstring& variable)
{
    DWORD length = ::GetEnvironmentVariableW(variable.c_str(), 0, 0);
    if (!length)
        return wstring();
    vector<wchar_t> buffer(length);
    if (!GetEnvironmentVariable(variable.c_str(), &buffer[0], buffer.size()) || !buffer[0])
        return wstring();
    return &buffer[0];
}

static bool prependPath(const wstring& directoryToPrepend)
{
    wstring pathVariable = L"PATH";
    wstring oldPath = copyEnvironmentVariable(pathVariable);
    wstring newPath = directoryToPrepend + L';' + oldPath;
    return ::SetEnvironmentVariableW(pathVariable.c_str(), newPath.c_str());
}

static int fatalError(const wstring& programName, const wstring& message)
{
    wstring caption = programName + L" can't open.";
    ::MessageBoxW(0, message.c_str(), caption.c_str(), MB_ICONERROR);
    return 1;
}

static bool modifyPath(const wstring& programName)
{
    struct {
        wstring softwareSubKey;
        wstring productName;
    } products[] = {
        { L"Apple Inc.", L"Apple Application Support" },
        { L"Apple Computer, Inc.", L"Safari" },
    };

    wstring pathPrefix;
    for (size_t i = 0; i < _countof(products); ++i) {
        wstring directory = applePathFromRegistry(L"SOFTWARE\\" + products[i].softwareSubKey + L"\\" + products[i].productName, L"InstallDir");
        if (directory.empty()) {
            fatalError(programName, L"Failed to determine path to " + products[i].productName + L" directory.");
            return false;
        }
        if (i)
            pathPrefix += L';';
        pathPrefix += directory;
    }

    if (prependPath(pathPrefix))
        return true;

    fatalError(programName, L"Failed to modify PATH environment variable.");
    return false;
}

#if USE_CONSOLE_ENTRY_POINT
int main(int argc, const char* argv[])
#else
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpstrCmdLine, int nCmdShow)
#endif
{
    enableTerminationOnHeapCorruption();

    // Get the path of our executable.
    wchar_t exePath[MAX_PATH];
    if (!::GetModuleFileNameW(0, exePath, _countof(exePath)))
        return fatalError(L"Unknown Program", L"Failed to determine name of executable.");

    ::PathRemoveExtensionW(exePath);

    wstring programName = ::PathFindFileNameW(exePath);

    if (!modifyPath(programName))
        return 1;

    // Load our corresponding DLL.
    wstring dllName = programName + L".dll";
    if (!::PathRemoveFileSpecW(exePath))
        return fatalError(programName, L"::PathRemoveFileSpecW failed.");
    if (!::PathAppendW(exePath, dllName.c_str()))
        return fatalError(programName, L"::PathAppendW failed.");
    HMODULE module = ::LoadLibraryW(exePath);
    if (!module)
        return fatalError(programName, L"::LoadLibraryW failed.");

#if USE_CONSOLE_ENTRY_POINT
    typedef int (WINAPI*EntryPoint)(int, const char*[]);
#if defined _M_AMD64 || defined _WIN64
    const char* entryPointName = "_dllLauncherEntryPoint";
#else
    const char* entryPointName = "_dllLauncherEntryPoint@8";
#endif
#else
    typedef int (WINAPI*EntryPoint)(HINSTANCE, HINSTANCE, LPWSTR, int);
#if defined _M_AMD64 || defined _WIN64
    const char* entryPointName = "_dllLauncherEntryPoint";
#else
    const char* entryPointName = "_dllLauncherEntryPoint@16";
#endif
#endif

    EntryPoint entryPoint = reinterpret_cast<EntryPoint>(::GetProcAddress(module, entryPointName));
    if (!entryPoint)
        return fatalError(programName, L"Failed to find dllLauncherEntryPoint function.");

#if USE_CONSOLE_ENTRY_POINT
    return entryPoint(argc, argv);
#else
    return entryPoint(hInstance, hPrevInstance, lpstrCmdLine, nCmdShow);
#endif
}
